#!/usr/bin/env python3
"""
M15.244 Phase 10: Reproducible Mode A vs Mode B Portable Backend Benchmark.
Compiles a standalone Release C++20 harness over real ROM entity-dispatch fixtures,
runs 10 repeated trials of Mode A (per-instruction router) vs Mode B (fused CFG block
dispatch), records min/median/max/stddev, binary & generated source sizes,
guest instructions executed, transitions, continuations, and RAM writes.
Strictly <= 350 lines per AGENTS.md.
"""

import json
from pathlib import Path
import statistics
import subprocess
import sys

CPP_HARNESS = r"""#include "tools/thor_ir/generated/entity_dispatch_portable_routines.hpp"
#include "data/thor-fixtures/m15_244_entity_dispatch_fixtures.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

using namespace oasis::recompiler::ir;

static std::vector<std::uint8_t> load_rom(const char* path) {
  std::ifstream f(path, std::ios::binary | std::ios::ate);
  if (!f) return {};
  auto sz = static_cast<std::size_t>(f.tellg());
  f.seekg(0);
  std::vector<std::uint8_t> buf(sz);
  f.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(sz));
  return buf;
}

static void reset_env(PortableGuestEnvironment& env, const DispatchRefFixture& f) {
  for (int i = 0; i < 8; ++i) { env.d_regs[i] = f.ed[i]; env.a_regs[i] = f.ea[i]; }
  env.set_sr(f.entry_sr);
  env.pc = f.entry_pc;
  env.total_cycles = 0; env.bus_error = false; env.address_error = false;
  env.yielded = false; env.sync_boundary_count = 0;
  for (std::uint32_t a = 0xFFFD80U; a < 0xFFFE20U; ++a) env.work_ram[a - kWorkRamBase] = 0;
  for (const auto& rb : f.ram) {
    if (rb.addr >= kWorkRamBase && rb.addr < kWorkRamBase + kWorkRamSize)
      env.work_ram[rb.addr - kWorkRamBase] = rb.val;
  }
}

struct BenchStats {
  double min_ms = 0.0;
  double median_ms = 0.0;
  double max_ms = 0.0;
  double stddev_ms = 0.0;
  std::uint64_t sink = 0;
};

template <typename Runner>
static BenchStats run_trials(const std::vector<std::uint8_t>& rom,
                             const std::vector<DispatchRefFixture>& fixtures,
                             int trials, int sweeps_per_trial, Runner&& fn) {
  PortableGuestEnvironment env;
  env.rom_data = rom;
  // Warmup
  for (int s = 0; s < 200; ++s) {
    for (const auto& f : fixtures) {
      reset_env(env, f);
      fn(env, f.entry_pc, 0x00FFFFFFU);
    }
  }
  std::vector<double> samples;
  samples.reserve(static_cast<std::size_t>(trials));
  std::uint64_t sink = 0;
  for (int t = 0; t < trials; ++t) {
    const auto t0 = std::chrono::steady_clock::now();
    for (int s = 0; s < sweeps_per_trial; ++s) {
      for (const auto& f : fixtures) {
        reset_env(env, f);
        auto r = fn(env, f.entry_pc, 0x00FFFFFFU);
        sink += r.pc ^ r.cycles ^ r.writes.size();
      }
    }
    const auto t1 = std::chrono::steady_clock::now();
    samples.push_back(std::chrono::duration<double, std::milli>(t1 - t0).count());
  }
  std::sort(samples.begin(), samples.end());
  double sum = std::accumulate(samples.begin(), samples.end(), 0.0);
  double mean = sum / static_cast<double>(samples.size());
  double var = 0.0;
  for (double x : samples) var += (x - mean) * (x - mean);
  var /= static_cast<double>(samples.size());
  BenchStats st;
  st.min_ms = samples.front();
  st.median_ms = samples[samples.size() / 2];
  st.max_ms = samples.back();
  st.stddev_ms = std::sqrt(var);
  st.sink = sink;
  return st;
}

int main(int argc, char** argv) {
  if (argc < 2) { std::cerr << "usage: bench <rom>\n"; return 2; }
  auto rom = load_rom(argv[1]);
  if (rom.empty()) { std::cerr << "empty rom\n"; return 2; }
  auto all_fix = get_m15_244_dispatch_fixtures();
  // Keep 9 deterministic multi-block & dispatch paths (excluding hardware address-error trap fixture if desired, or keep non-faulting ones)
  std::vector<DispatchRefFixture> bench_fix;
  for (const auto& f : all_fix) {
    if (f.outcome != PortableContinuationOutcome::OriginalCpuException)
      bench_fix.push_back(f);
  }
  const int trials = 10;
  const int sweeps = 4000;
  // Count single-sweep exact micro metrics via IrBus step counter
  std::uint64_t single_sweep_insns = 0;
  std::uint64_t single_sweep_ram_writes = 0;
  std::uint64_t single_sweep_continuations = 0;
  {
    PortableGuestEnvironment env;
    env.rom_data = rom;
    for (const auto& f : bench_fix) {
      std::fill(env.work_ram.begin(), env.work_ram.end(), std::uint8_t{0});
      reset_env(env, f);
      PortableIrBus bus{env};
      std::uint32_t next_pc = f.entry_pc;
      env.pc = f.entry_pc;
      int guard = 0;
      while (!bus.faulted() && !env.yielded && ++guard < 4096) {
        if (!is_compiled_entity_dispatch_pc(env.pc)) break;
        if (!step_one_entity_dispatch_ir(bus, env.pc, next_pc)) break;
        single_sweep_insns++;
      }
      single_sweep_ram_writes += bus.writes.size();
      if (f.outcome == PortableContinuationOutcome::ValidUncompiledPc) single_sweep_continuations++;
    }
  }
  auto st_a = run_trials(rom, bench_fix, trials, sweeps, [](PortableGuestEnvironment& e, std::uint32_t ep, std::uint32_t sp) {
    return run_entity_dispatch_mode_a(e, ep, sp);
  });
  auto st_b = run_trials(rom, bench_fix, trials, sweeps, [](PortableGuestEnvironment& e, std::uint32_t ep, std::uint32_t sp) {
    return run_entity_dispatch_mode_b(e, ep, sp);
  });
  std::cout << "{\n"
            << "  \"fixture_count\": " << bench_fix.size() << ",\n"
            << "  \"trials\": " << trials << ",\n"
            << "  \"sweeps_per_trial\": " << sweeps << ",\n"
            << "  \"transitions_per_trial\": " << (bench_fix.size() * sweeps) << ",\n"
            << "  \"guest_instructions_per_trial\": " << (single_sweep_insns * sweeps) << ",\n"
            << "  \"uncompiled_continuations_per_trial\": " << (single_sweep_continuations * sweeps) << ",\n"
            << "  \"ram_writes_per_trial\": " << (single_sweep_ram_writes * sweeps) << ",\n"
            << "  \"mode_a\": {\"min_ms\": " << st_a.min_ms << ", \"median_ms\": " << st_a.median_ms
            << ", \"max_ms\": " << st_a.max_ms << ", \"stddev_ms\": " << st_a.stddev_ms << ", \"sink\": " << st_a.sink << "},\n"
            << "  \"mode_b\": {\"min_ms\": " << st_b.min_ms << ", \"median_ms\": " << st_b.median_ms
            << ", \"max_ms\": " << st_b.max_ms << ", \"stddev_ms\": " << st_b.stddev_ms << ", \"sink\": " << st_b.sink << "},\n"
            << "  \"speedup_median\": " << (st_a.median_ms / st_b.median_ms) << "\n"
            << "}\n";
  return 0;
}
"""


def main() -> int:
    root = Path(__file__).resolve().parent.parent
    gen_dir = root / "src" / "tools" / "thor_ir" / "generated"
    gen_files = sorted(gen_dir.glob("entity_dispatch_*.hpp"))
    src_bytes = sum(p.stat().st_size for p in gen_files)
    src_lines = sum(len(p.read_text(encoding="utf-8").splitlines()) for p in gen_files)

    build_dir = root / "build" / "m15_244"
    build_dir.mkdir(parents=True, exist_ok=True)
    harness_cpp = build_dir / "benchmark_m15_244_harness.cpp"
    harness_cpp.write_text(CPP_HARNESS, encoding="utf-8")

    bench_bin = build_dir / "bench_m15_244_rel"
    cmd_compile = [
        "wsl", "bash", "-lc",
        f"cd /mnt/c/Github/Sega-Thor && g++ -std=c++20 -O2 -DNDEBUG -I. -Isrc "
        f"build/m15_244/benchmark_m15_244_harness.cpp -o build/m15_244/bench_m15_244_rel"
    ]
    res_c = subprocess.run(cmd_compile, capture_output=True, text=True)
    if res_c.returncode != 0:
        print(res_c.stderr, file=sys.stderr)
        return res_c.returncode

    cmd_run = [
        "wsl", "bash", "-lc",
        "cd /mnt/c/Github/Sega-Thor && ./build/m15_244/bench_m15_244_rel \"local-roms/Beyond Oasis (USA).md\""
    ]
    res_r = subprocess.run(cmd_run, capture_output=True, text=True)
    if res_r.returncode != 0:
        print(res_r.stderr, file=sys.stderr)
        return res_r.returncode

    raw = json.loads(res_r.stdout)
    runner_bin = root / "build" / "auto-migrator" / "m15_244" / "linux-runner" / "oasis_gameplay_session"
    report = {
        "schema": "oasis.m15_244.benchmark.v1",
        "milestone": "M15.244",
        "generated_source_files": len(gen_files),
        "generated_source_bytes": src_bytes,
        "generated_source_lines": src_lines,
        "benchmark_binary_bytes": bench_bin.stat().st_size,
        "portable_gameplay_runner_bytes": runner_bin.stat().st_size if runner_bin.exists() else 0,
        "isolated_backend_results": raw,
    }
    out_path = root / "data" / "thor-evidence" / "benchmark_m15_244_report.json"
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(report, indent=2), encoding="utf-8")
    print(f"Generated source: {len(gen_files)} files, {src_lines} lines, {src_bytes} bytes")
    print(f"Benchmark binary: {report['benchmark_binary_bytes']} bytes | Runner binary: {report['portable_gameplay_runner_bytes']} bytes")
    print(f"Mode A median: {raw['mode_a']['median_ms']:.3f} ms (min={raw['mode_a']['min_ms']:.3f}, max={raw['mode_a']['max_ms']:.3f}, sd={raw['mode_a']['stddev_ms']:.3f})")
    print(f"Mode B median: {raw['mode_b']['median_ms']:.3f} ms (min={raw['mode_b']['min_ms']:.3f}, max={raw['mode_b']['max_ms']:.3f}, sd={raw['mode_b']['stddev_ms']:.3f})")
    print(f"Speedup (Mode A / Mode B): {raw['speedup_median']:.3f}x | Insns/trial={raw['guest_instructions_per_trial']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
