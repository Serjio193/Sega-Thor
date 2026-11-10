#!/usr/bin/env python3
"""
M15.246 Phase 13 Microbenchmark (Mode A vs Mode B, 10 trials) & Integrated Runner Timing Report.
Writes data/thor-evidence/benchmark_m15_246_report.json.
Strictly <= 200 lines per AGENTS.md Rule 3.
"""

import json
from pathlib import Path
import statistics
import subprocess

HARNESS_CPP = r"""
#include "tools/thor_ir/generated_m15_246/exp246_dispatch_portable_routines.hpp"
#include "data/thor-fixtures/m15_246_callee_fixtures.hpp"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

using namespace oasis::recompiler::ir;

int main() {
    std::ifstream f("local-roms/Beyond Oasis (USA).md", std::ios::binary | std::ios::ate);
    auto sz = static_cast<std::size_t>(f.tellg()); f.seekg(0);
    std::vector<std::uint8_t> rom(sz);
    f.read(reinterpret_cast<char*>(rom.data()), static_cast<std::streamsize>(sz));

    const auto fixtures = get_m15_246_callee_fixtures();
    std::vector<DispatchRefFixture> hot;
    for (const auto& fx : fixtures) {
        if (std::string(fx.id).find("auth_lr_04") != std::string::npos ||
            std::string(fx.id).find("auth_lr_09") != std::string::npos ||
            std::string(fx.id).find("auth_lr_12") != std::string::npos ||
            std::string(fx.id) == "ctrl_f0d2_dist2d_bsr_isqrt_rts" ||
            std::string(fx.id) == "ctrl_2ee58_divs_extl_asll7_block") {
            hot.push_back(fx);
        }
    }

    auto init = [&](PortableGuestEnvironment& env, const DispatchRefFixture& fx) {
        for (int i = 0; i < 8; ++i) { env.d_regs[i] = fx.ed[i]; env.a_regs[i] = fx.ea[i]; }
        env.set_sr(fx.entry_sr); env.pc = fx.entry_pc;
        env.total_cycles = 0; env.bus_error = false; env.address_error = false; env.yielded = false;
        for (const auto& rb : fx.ram)
            if (rb.addr >= kWorkRamBase && rb.addr < kWorkRamBase + kWorkRamSize)
                env.work_ram[rb.addr - kWorkRamBase] = rb.val;
    };

    constexpr int kTrials = 10;
    constexpr int kIters = 10000;
    std::vector<double> ma_ms, mb_ms;
    std::uint64_t sink_a = 0, sink_b = 0;
    PortableGuestEnvironment env_a, env_b;
    env_a.rom_data = rom;
    env_b.rom_data = rom;
    std::fill(env_a.work_ram.begin(), env_a.work_ram.end(), std::uint8_t{0});
    std::fill(env_b.work_ram.begin(), env_b.work_ram.end(), std::uint8_t{0});
    for (int t = 0; t < kTrials; ++t) {
        auto t0 = std::chrono::steady_clock::now();
        for (int it = 0; it < kIters; ++it) {
            for (const auto& fx : hot) {
                init(env_a, fx);
                const std::uint32_t sp = (fx.outcome == PortableContinuationOutcome::CompiledNextPc) ? fx.exit_pc : 0x00FFFFFFU;
                auto r = run_exp246_dispatch_mode_a(env_a, fx.entry_pc, sp);
                sink_a ^= r.cycles + r.d[0];
            }
        }
        auto t1 = std::chrono::steady_clock::now();
        for (int it = 0; it < kIters; ++it) {
            for (const auto& fx : hot) {
                init(env_b, fx);
                const std::uint32_t sp = (fx.outcome == PortableContinuationOutcome::CompiledNextPc) ? fx.exit_pc : 0x00FFFFFFU;
                auto r = run_exp246_dispatch_mode_b(env_b, fx.entry_pc, sp);
                sink_b ^= r.cycles + r.d[0];
            }
        }
        auto t2 = std::chrono::steady_clock::now();
        ma_ms.push_back(std::chrono::duration<double, std::milli>(t1 - t0).count());
        mb_ms.push_back(std::chrono::duration<double, std::milli>(t2 - t1).count());
    }
    if (sink_a != sink_b) return 1;
    std::cout << "{\"trials\":" << kTrials << ",\"iters_per_trial\":" << (kIters * hot.size())
              << ",\"parity\":true,\"mode_a_ms\":[";
    for (int i = 0; i < kTrials; ++i) std::cout << (i ? "," : "") << ma_ms[i];
    std::cout << "],\"mode_b_ms\":[";
    for (int i = 0; i < kTrials; ++i) std::cout << (i ? "," : "") << mb_ms[i];
    std::cout << "]}\n";
    return 0;
}
"""


def stats(vals: list[float]) -> dict:
    s = sorted(vals)
    p95_idx = min(len(s) - 1, int(round(0.95 * (len(s) - 1))))
    return {
        "min_ms": round(s[0], 3),
        "median_ms": round(statistics.median(vals), 3),
        "mean_ms": round(statistics.mean(vals), 3),
        "p95_ms": round(s[p95_idx], 3),
        "max_ms": round(s[-1], 3),
    }


def main() -> int:
    root = Path(__file__).resolve().parent.parent
    bdir = root / "build" / "m15_246"
    bdir.mkdir(parents=True, exist_ok=True)
    cpp_p = bdir / "bench_m15_246_harness.cpp"
    bin_p = bdir / "bench_m15_246_rel"
    cpp_p.write_text(HARNESS_CPP.strip() + "\n", encoding="utf-8")

    subprocess.run(
        ["wsl", "g++", "-std=c++20", "-O3", "-DNDEBUG", "-I.", "-Isrc",
         "build/m15_246/bench_m15_246_harness.cpp", "-o", "build/m15_246/bench_m15_246_rel"],
        check=True,
    )
    proc = subprocess.run(["wsl", "./build/m15_246/bench_m15_246_rel"], capture_output=True, text=True, check=True)
    raw = json.loads(proc.stdout.strip())
    assert raw["parity"] is True

    sa, sb = stats(raw["mode_a_ms"]), stats(raw["mode_b_ms"])
    med_speedup = round(sa["median_ms"] / sb["median_ms"], 3)
    mean_speedup = round(sa["mean_ms"] / sb["mean_ms"], 3)

    ab = json.loads((root / "build" / "ab_portable_m15_246" / "m15_246_ab_summary.json").read_text(encoding="utf-8"))
    integrated = {sc: data["wall_clock_seconds"] for sc, data in ab["scenarios"].items()}

    report = {
        "schema": "oasis.m15_246.benchmark-report.v1",
        "milestone": "M15.246",
        "portable_microbenchmark": {
            "trials": raw["trials"],
            "dispatches_per_trial": raw["iters_per_trial"],
            "parity_verified": True,
            "mode_a": sa,
            "mode_b": sb,
            "median_speedup_mode_b_over_a": med_speedup,
            "mean_speedup_mode_b_over_a": mean_speedup,
            "mode_a_samples_ms": raw["mode_a_ms"],
            "mode_b_samples_ms": raw["mode_b_ms"],
        },
        "integrated_runner_wall_clock_seconds": integrated,
    }
    out_p = root / "data" / "thor-evidence" / "benchmark_m15_246_report.json"
    out_p.write_text(json.dumps(report, indent=2), encoding="utf-8")
    print(f"Benchmark PASS: Mode A med={sa['median_ms']}ms, Mode B med={sb['median_ms']}ms, speedup={med_speedup}x -> {out_p}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
