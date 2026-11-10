#!/usr/bin/env python3
"""
M15.245 Phase 12: Reproducible Mode A vs Mode B & Integrated Gameplay Benchmark.
Compiles a standalone Release C++20 harness over all 28 real ROM entity-handler fixtures,
runs 10 repeated trials of Mode A (per-instruction router) vs Mode B (fused CFG block
dispatch), records min/median/max/stddev, binary & generated source sizes,
guest instructions executed, transitions, continuations, RAM writes, and
integrated runner timings across Scenarios C, A, and B.
Strictly <= 300 lines per AGENTS.md.
"""

import json
from pathlib import Path
import subprocess
import sys

CPP_HARNESS = r"""#include "tools/thor_ir/generated/entity_dispatch_portable_routines.hpp"
#include "tools/thor_ir/generated_m15_245/exp_dispatch_portable_routines.hpp"
#include "data/thor-fixtures/m15_245_entity_dispatch_fixtures.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <numeric>
#include <vector>

using namespace oasis::recompiler::ir;

static std::vector<std::uint8_t> load_rom(const char* path) {
  std::ifstream f(path, std::ios::binary | std::ios::ate);
  if (!f) return {};
  const auto sz = static_cast<std::size_t>(f.tellg());
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
  for (const auto& rb : f.ram) {
    if (rb.addr >= kWorkRamBase && rb.addr < kWorkRamBase + kWorkRamSize)
      env.work_ram[rb.addr - kWorkRamBase] = rb.val;
  }
}

static bool is_any_pc(std::uint32_t pc) noexcept {
  return is_compiled_entity_dispatch_pc(pc) || is_compiled_exp_dispatch_pc(pc);
}

static PortableExecutionResult run_mode_a(PortableGuestEnvironment& env, std::uint32_t ep, std::uint32_t sp) {
  PortableIrBus bus{env}; env.pc = ep & kBusAddressMask;
  PortableExecutionResult res{}; res.outcome = PortableContinuationOutcome::CompiledNextPc;
  std::uint32_t next_pc = env.pc; int guard = 0;
  while (!bus.faulted() && !env.yielded && env.pc != sp && ++guard < 8192) {
    if (is_compiled_entity_dispatch_pc(env.pc)) { if (!step_one_entity_dispatch_ir(bus, env.pc, next_pc)) break; }
    else if (is_compiled_exp_dispatch_pc(env.pc)) { if (!step_one_exp_dispatch_ir(bus, env.pc, next_pc)) break; }
    else { res.outcome = PortableContinuationOutcome::ValidUncompiledPc; break; }
  }
  if (bus.faulted()) res.outcome = PortableContinuationOutcome::OriginalCpuException;
  else if (env.yielded || (env.pc != sp && !is_any_pc(env.pc))) res.outcome = PortableContinuationOutcome::ValidUncompiledPc;
  res.pc = env.pc; res.cycles = env.total_cycles; res.writes = bus.writes;
  return res;
}

static PortableExecutionResult run_mode_b(PortableGuestEnvironment& env, std::uint32_t ep, std::uint32_t sp) {
  PortableIrBus bus{env}; env.pc = ep & kBusAddressMask;
  PortableExecutionResult res{}; res.outcome = PortableContinuationOutcome::CompiledNextPc;
  std::uint32_t next_pc = env.pc; int guard = 0;
  while (!bus.faulted() && !env.yielded && env.pc != sp && ++guard < 8192) {
    if (is_compiled_entity_dispatch_pc(env.pc)) { if (!step_one_entity_dispatch_ir(bus, env.pc, next_pc)) break; }
    else if (is_compiled_exp_dispatch_pc(env.pc)) {
      bool h = false;
      if (env.pc >= 0x005E78U && env.pc <= 0x0066C2U) h = run_exp_mb_chunk_0(bus, env.pc, sp, next_pc);
      else if (env.pc >= 0x0066CEU && env.pc <= 0x006F28U) h = run_exp_mb_chunk_1(bus, env.pc, sp, next_pc);
      else if (env.pc >= 0x006F2CU && env.pc <= 0x00770CU) h = run_exp_mb_chunk_2(bus, env.pc, sp, next_pc);
      else if (env.pc >= 0x007740U && env.pc <= 0x007E70U) h = run_exp_mb_chunk_3(bus, env.pc, sp, next_pc);
      else if (env.pc >= 0x007E78U && env.pc <= 0x007E96U) h = run_exp_mb_chunk_4(bus, env.pc, sp, next_pc);
      if (!h && !step_one_exp_dispatch_ir(bus, env.pc, next_pc)) break;
    } else { res.outcome = PortableContinuationOutcome::ValidUncompiledPc; break; }
  }
  if (bus.faulted()) res.outcome = PortableContinuationOutcome::OriginalCpuException;
  else if (env.yielded || (env.pc != sp && !is_any_pc(env.pc))) res.outcome = PortableContinuationOutcome::ValidUncompiledPc;
  res.pc = env.pc; res.cycles = env.total_cycles; res.writes = bus.writes;
  return res;
}

struct BenchStats { double min_ms{}, median_ms{}, max_ms{}, stddev_ms{}; std::uint64_t sink{}; };

template <typename Runner>
static BenchStats run_trials(const std::vector<std::uint8_t>& rom,
                             const std::vector<DispatchRefFixture>& fixtures,
                             int trials, int sweeps, Runner&& fn) {
  PortableGuestEnvironment env;
  env.rom_data = rom;
  for (int s = 0; s < 150; ++s) {
    for (const auto& f : fixtures) { reset_env(env, f); fn(env, f.entry_pc, 0x00FFFFFFU); }
  }
  std::vector<double> samples;
  samples.reserve(static_cast<std::size_t>(trials));
  std::uint64_t sink = 0;
  for (int t = 0; t < trials; ++t) {
    const auto t0 = std::chrono::steady_clock::now();
    for (int s = 0; s < sweeps; ++s) {
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
  const double mean = std::accumulate(samples.begin(), samples.end(), 0.0) / static_cast<double>(samples.size());
  double var = 0.0;
  for (double x : samples) var += (x - mean) * (x - mean);
  var /= static_cast<double>(samples.size());
  return {samples.front(), samples[samples.size() / 2], samples.back(), std::sqrt(var), sink};
}

int main(int argc, char** argv) {
  if (argc < 2) return 2;
  auto rom = load_rom(argv[1]);
  if (rom.empty()) return 2;
  auto fix = get_m15_245_entity_dispatch_fixtures();
  const int trials = 10, sweeps = 2500;
  std::uint64_t insns = 0, ram_writes = 0, conts = 0;
  {
    PortableGuestEnvironment env; env.rom_data = rom;
    for (const auto& f : fix) {
      std::fill(env.work_ram.begin(), env.work_ram.end(), std::uint8_t{0});
      reset_env(env, f); PortableIrBus bus{env}; std::uint32_t npc = f.entry_pc; int g = 0;
      while (!bus.faulted() && !env.yielded && ++g < 8192) {
        if (is_compiled_entity_dispatch_pc(env.pc)) { if (!step_one_entity_dispatch_ir(bus, env.pc, npc)) break; }
        else if (is_compiled_exp_dispatch_pc(env.pc)) { if (!step_one_exp_dispatch_ir(bus, env.pc, npc)) break; }
        else break;
        insns++;
      }
      ram_writes += bus.writes.size();
      if (f.outcome == PortableContinuationOutcome::ValidUncompiledPc) conts++;
    }
  }
  auto a = run_trials(rom, fix, trials, sweeps, run_mode_a);
  auto b = run_trials(rom, fix, trials, sweeps, run_mode_b);
  std::cout << "{\n"
            << "  \"fixture_count\": " << fix.size() << ",\n"
            << "  \"trials\": " << trials << ",\n"
            << "  \"sweeps_per_trial\": " << sweeps << ",\n"
            << "  \"transitions_per_trial\": " << (fix.size() * sweeps) << ",\n"
            << "  \"guest_instructions_per_trial\": " << (insns * sweeps) << ",\n"
            << "  \"uncompiled_continuations_per_trial\": " << (conts * sweeps) << ",\n"
            << "  \"ram_writes_per_trial\": " << (ram_writes * sweeps) << ",\n"
            << "  \"mode_a\": {\"min_ms\": " << a.min_ms << ", \"median_ms\": " << a.median_ms
            << ", \"max_ms\": " << a.max_ms << ", \"stddev_ms\": " << a.stddev_ms << ", \"sink\": " << a.sink << "},\n"
            << "  \"mode_b\": {\"min_ms\": " << b.min_ms << ", \"median_ms\": " << b.median_ms
            << ", \"max_ms\": " << b.max_ms << ", \"stddev_ms\": " << b.stddev_ms << ", \"sink\": " << b.sink << "},\n"
            << "  \"speedup_median\": " << (a.median_ms / b.median_ms) << "\n"
            << "}\n";
  return 0;
}
"""


def main() -> int:
    root = Path(__file__).resolve().parent.parent
    gen_dir = root / "src" / "tools" / "thor_ir" / "generated_m15_245"
    gen_files = sorted(gen_dir.glob("*.hpp"))
    src_bytes = sum(p.stat().st_size for p in gen_files)
    src_lines = sum(len(p.read_text(encoding="utf-8").splitlines()) for p in gen_files)

    build_dir = root / "build" / "m15_245"
    build_dir.mkdir(parents=True, exist_ok=True)
    harness_cpp = build_dir / "benchmark_m15_245_harness.cpp"
    harness_cpp.write_text(CPP_HARNESS, encoding="utf-8")

    bench_bin = build_dir / "bench_m15_245_rel"
    res_c = subprocess.run(
        ["wsl", "bash", "-lc",
         "cd /mnt/c/Github/Sega-Thor && g++ -std=c++20 -O2 -DNDEBUG -I. -Isrc "
         "build/m15_245/benchmark_m15_245_harness.cpp -o build/m15_245/bench_m15_245_rel"],
        capture_output=True, text=True,
    )
    if res_c.returncode != 0:
        print(res_c.stderr, file=sys.stderr)
        return res_c.returncode

    res_r = subprocess.run(
        ["wsl", "bash", "-lc", "cd /mnt/c/Github/Sega-Thor && ./build/m15_245/bench_m15_245_rel \"local-roms/Beyond Oasis (USA).md\""],
        capture_output=True, text=True,
    )
    if res_r.returncode != 0:
        print(res_r.stderr, file=sys.stderr)
        return res_r.returncode

    raw = json.loads(res_r.stdout)
    runner_bin = root / "build" / "auto-migrator" / "m15_245" / "linux-runner" / "oasis_gameplay_session"
    ab_sum_p = root / "build" / "ab_portable_m15_245" / "m15_245_ab_summary.json"
    ab_sum = json.loads(ab_sum_p.read_text(encoding="utf-8")) if ab_sum_p.exists() else {}
    integrated = {}
    for sc, sv in ab_sum.get("scenarios", {}).items():
        wc = sv.get("wall_clock_seconds", {})
        integrated[sc] = {
            "run_a_prod_sec": wc.get("run_a"),
            "run_b_m15_244_sec": wc.get("run_b"),
            "run_c_m15_245_sec": wc.get("run_c"),
        }

    report = {
        "schema": "oasis.m15_245.benchmark.v1",
        "milestone": "M15.245",
        "generated_source_files": len(gen_files),
        "generated_source_bytes": src_bytes,
        "generated_source_lines": src_lines,
        "benchmark_binary_bytes": bench_bin.stat().st_size,
        "portable_gameplay_runner_bytes": runner_bin.stat().st_size if runner_bin.exists() else 0,
        "isolated_backend_results": raw,
        "integrated_runner_wall_clock": integrated,
    }
    out_path = root / "data" / "thor-evidence" / "benchmark_m15_245_report.json"
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(report, indent=2), encoding="utf-8")
    print(f"Generated source: {len(gen_files)} files, {src_lines} lines, {src_bytes} bytes")
    print(f"Benchmark binary: {report['benchmark_binary_bytes']} bytes | Runner binary: {report['portable_gameplay_runner_bytes']} bytes")
    print(f"Mode A median: {raw['mode_a']['median_ms']:.3f} ms (min={raw['mode_a']['min_ms']:.3f}, max={raw['mode_a']['max_ms']:.3f}, sd={raw['mode_a']['stddev_ms']:.3f})")
    print(f"Mode B median: {raw['mode_b']['median_ms']:.3f} ms (min={raw['mode_b']['min_ms']:.3f}, max={raw['mode_b']['max_ms']:.3f}, sd={raw['mode_b']['stddev_ms']:.3f})")
    print(f"Speedup (Mode A / Mode B): {raw['speedup_median']:.3f}x | Insns/trial={raw['guest_instructions_per_trial']} | SinkMatch={raw['mode_a']['sink']==raw['mode_b']['sink']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
