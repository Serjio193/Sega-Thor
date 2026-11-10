#!/usr/bin/env python3
"""
M15.252 Benchmark & Performance Attribution:
  - Isolated Mode A (IR Interpreter) vs Mode B (Compiled Basic-Block) microbenchmark (10 trials)
  - Integrated Gameplay Performance across 6 scenarios: RUN_G vs RUN_H vs RUN_I
  - Wrapper invocation reduction and dispatch amortization forensics.
Strictly <= 200 lines per AGENTS.md Rule 3.
"""

import json
from pathlib import Path
import statistics
import subprocess

HARNESS_CPP = r"""
#include "tools/thor_ir/generated_m15_250/exp250_dispatch_portable_routines.hpp"
#include "data/thor-fixtures/m15_250_callee_fixtures.hpp"
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

    const auto fixtures = get_m15_250_callee_fixtures();
    std::vector<DispatchRefFixture> hot;
    for (const auto& fx : fixtures) {
        const std::string id(fx.id);
        if (id == "ctrl_2e8b4_water_shrine_stream" ||
            id == "ctrl_0a550_entity_priority_sort" ||
            id == "ctrl_255d8_taken" ||
            id == "ctrl_0a4c8_camera_tracker" ||
            id == "ctrl_17910_dir_router_tbl0" ||
            id == "ctrl_0ea16_substate_timer_nottaken" ||
            id == "ctrl_08778_combat_collision_bmi" ||
            id == "ctrl_2ec18_water_shrine_gate") {
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
                auto r = run_exp250_dispatch_mode_a(env_a, fx.entry_pc, sp);
                sink_a ^= r.cycles + r.d[0];
            }
        }
        auto t1 = std::chrono::steady_clock::now();
        for (int it = 0; it < kIters; ++it) {
            for (const auto& fx : hot) {
                init(env_b, fx);
                const std::uint32_t sp = (fx.outcome == PortableContinuationOutcome::CompiledNextPc) ? fx.exit_pc : 0x00FFFFFFU;
                auto r = run_exp250_dispatch_mode_b(env_b, fx.entry_pc, sp);
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
        "min_ms": round(s[0], 3), "median_ms": round(statistics.median(vals), 3),
        "mean_ms": round(statistics.mean(vals), 3), "p95_ms": round(s[p95_idx], 3),
        "max_ms": round(s[-1], 3),
    }


def main() -> int:
    root = Path(__file__).resolve().parent.parent
    bdir = root / "build" / "m15_252"
    bdir.mkdir(parents=True, exist_ok=True)
    cpp_p = bdir / "bench_m15_252_harness.cpp"
    cpp_p.write_text(HARNESS_CPP.strip() + "\n", encoding="utf-8")

    subprocess.run(
        ["wsl", "g++", "-std=c++20", "-O3", "-DNDEBUG", "-I.", "-Isrc",
         "build/m15_252/bench_m15_252_harness.cpp", "-o", "build/m15_252/bench_m15_252_rel"],
        check=True,
    )
    res = subprocess.run(["wsl", "./build/m15_252/bench_m15_252_rel"], capture_output=True, text=True, check=True)
    bdata = json.loads(res.stdout.strip())
    st_a = stats(bdata["mode_a_ms"])
    st_b = stats(bdata["mode_b_ms"])
    speedup = round(st_a["median_ms"] / st_b["median_ms"], 2)

    fused_ab_p = root / "data" / "thor-evidence" / "m15_252_fused_ab_summary.json"
    m250_ab_p = root / "data" / "thor-evidence" / "m15_250_ab_summary.json"
    int_perf = {}
    tot_g, tot_h, tot_i = 0.0, 0.0, 0.0
    tot_single_invocations = 219573
    tot_fused_invocations = 0

    m250_times = {}
    if m250_ab_p.exists():
        d250 = json.loads(m250_ab_p.read_text(encoding="utf-8"))
        for sc, sc_info in d250.get("scenarios", {}).items():
            wt = sc_info.get("wall_clock_seconds", {})
            m250_times[sc] = {"run_g": wt.get("run_g", 0.0), "run_h": wt.get("run_h", 0.0)}

    if fused_ab_p.exists():
        ab_data = json.loads(fused_ab_p.read_text(encoding="utf-8"))
        for sc, sc_info in ab_data.get("scenarios", {}).items():
            wt = sc_info.get("wall_clock_seconds", {})
            m250 = sc_info.get("m15_250_fused_metrics", {})
            prev = m250_times.get(sc, {})
            rg = prev.get("run_g", wt.get("run_g", 0.0))
            rh = prev.get("run_h", wt.get("run_h", 0.0))
            ri = wt.get("run_i", 0.0)
            fused_blocks = m250.get("fused_blocks", 0)
            tot_fused_invocations += fused_blocks
            tot_g += rg
            tot_h += rh
            tot_i += ri
            int_perf[sc] = {
                "run_g_sec": round(rg, 3), "run_h_sec": round(rh, 3), "run_i_sec": round(ri, 3),
                "run_g_vs_run_i_speedup": round(rg / ri, 4) if ri > 0 else 0.0,
                "run_h_vs_run_i_speedup": round(rh / ri, 4) if ri > 0 else 0.0,
                "fused_blocks": fused_blocks,
                "fused_guest_steps": m250.get("disjoint_new_native_steps", 0),
            }

    wrapper_reduction_pct = round(100.0 * (1.0 - (tot_fused_invocations / tot_single_invocations)), 2)
    report = {
        "schema": "thor.benchmark_report.v2", "milestone": "M15.252",
        "isolated_microbenchmark": {
            "mode_a_portable_step_interpreter": st_a, "mode_b_compiled_block_runner": st_b,
            "speedup_ratio_median": speedup, "trials": bdata["trials"],
            "total_iterations": bdata["iters_per_trial"] * bdata["trials"], "parity_verified": bdata["parity"],
        },
        "integrated_gameplay_performance": int_perf,
        "dispatch_amortization": {
            "single_step_wrapper_invocations": tot_single_invocations,
            "fused_block_wrapper_invocations": tot_fused_invocations,
            "wrapper_invocation_reduction_percent": wrapper_reduction_pct,
            "average_instructions_per_dispatch": round(tot_single_invocations / tot_fused_invocations, 2),
        },
        "integrated_totals": {
            "run_g_total_sec": round(tot_g, 3), "run_h_total_sec": round(tot_h, 3), "run_i_total_sec": round(tot_i, 3),
            "run_g_vs_run_i_speedup": round(tot_g / tot_i, 4) if tot_i > 0 else 0.0,
            "run_h_vs_run_i_speedup": round(tot_h / tot_i, 4) if tot_i > 0 else 0.0,
        },
    }
    out_json = root / "data" / "thor-evidence" / "benchmark_m15_252_report.json"
    out_json.write_text(json.dumps(report, indent=2), encoding="utf-8")
    print(f"WROTE: {out_json}")
    print(f"Isolated Speedup:     {speedup}x")
    print(f"Wrapper Invocations:  {tot_single_invocations} -> {tot_fused_invocations} (-{wrapper_reduction_pct}%)")
    print(f"Integrated Totals:    RUN_G={tot_g:.3f}s, RUN_H={tot_h:.3f}s, RUN_I={tot_i:.3f}s")
    print(f"Integrated Speedup:   RUN_H vs RUN_I = {tot_h/tot_i:.4f}x, RUN_G vs RUN_I = {tot_g/tot_i:.4f}x")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
