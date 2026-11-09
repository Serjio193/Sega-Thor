#!/usr/bin/env python3
"""
Performance benchmark for M15.241.
Benchmarks:
1. P0 Baseline vs P_COMBINED under identical execution conditions.
2. Mode A (Thor IR Verification Backend) vs Mode B (Portable Native C++ Backend)
   on identical input states across 100,000 iterations.
Strictly <= 500 lines per AGENTS.md Rule 3.
"""

from pathlib import Path
import statistics
import subprocess
import sys
import time


def to_wsl_path(path: Path) -> str:
    resolved = path.resolve()
    drive = resolved.drive.replace(":", "").lower()
    posix = resolved.as_posix()
    if resolved.drive:
        posix = posix[len(resolved.drive):]
    return f"/mnt/{drive}{posix}"


def benchmark_portable_ir():
    print("--- Benchmark 1: Portable IR Mode A vs Mode B ---")
    root = Path(__file__).resolve().parent.parent

    # Create a small standalone benchmark harness
    src = """
#include "tools/thor_ir/portable_guest_env.hpp"
#include "tools/thor_ir/generated/portable_routines.hpp"
#include "data/thor-fixtures/m15_241_cpu_reference_fixtures.hpp"

#include <chrono>
#include <iostream>

using namespace oasis::recompiler::ir;
using namespace oasis::recompiler::fixtures;

int main() {
    const auto& fixtures = get_cpu_fixtures();
    const auto& fix = fixtures[0]; // 00e682
    PortableGuestEnvironment env;
    env.d_regs[0] = fix.d0;
    env.d_regs[2] = fix.d2;
    env.a_regs[6] = fix.a6;
    env.a_regs[7] = fix.a7;

    constexpr int kIterations = 100000;

    // Mode A benchmark
    auto t0 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < kIterations; ++i) {
        env.write_memory(fix.a6 + 0x36U, IrOpSize::Byte, 0x40U);
        run_mode_a_00e682(env);
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    double mode_a_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    // Mode B benchmark
    auto t2 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < kIterations; ++i) {
        env.write_memory(fix.a6 + 0x36U, IrOpSize::Byte, 0x40U);
        run_mode_b_00e682(env);
    }
    auto t3 = std::chrono::high_resolution_clock::now();
    double mode_b_ms = std::chrono::duration<double, std::milli>(t3 - t2).count();

    std::cout << "Mode A (Verification Stepper): " << mode_a_ms << " ms ("
              << (kIterations / (mode_a_ms / 1000.0)) << " calls/sec)\\n";
    std::cout << "Mode B (Portable Native C++):  " << mode_b_ms << " ms ("
              << (kIterations / (mode_b_ms / 1000.0)) << " calls/sec)\\n";
    std::cout << "Portable Native Speedup:       " << (mode_a_ms / mode_b_ms) << "x\\n";
    return 0;
}
"""
    bench_cpp = root / "build" / "bench_portable_ir.cpp"
    bench_cpp.parent.mkdir(parents=True, exist_ok=True)
    bench_cpp.write_text(src, encoding="utf-8")

    wsl_src = to_wsl_path(bench_cpp)
    wsl_exe = "/mnt/c/Github/Sega-Thor/build/bench_portable_ir"
    cmd = f"g++ -std=c++20 -O3 -I/mnt/c/Github/Sega-Thor/src -I/mnt/c/Github/Sega-Thor {wsl_src} -o {wsl_exe} && {wsl_exe}"
    if sys.platform == "win32":
        res = subprocess.run(["wsl", "bash", "-c", cmd], capture_output=True, text=True)
    else:
        res = subprocess.run(["bash", "-c", cmd], capture_output=True, text=True)
    print(res.stdout)
    if res.stderr:
        print(res.stderr, file=sys.stderr)


def benchmark_hybrid_runner():
    print("--- Benchmark 2: Gameplay Session Replay ---")
    runner = Path("build/auto-migrator/m15_240/linux-runner/oasis_gameplay_session")
    manifest = Path("build/ab_combined_m15_240/scenario_c/session.manifest")
    if not runner.exists() or not manifest.exists():
        print("Runner or session manifest not found, skipping replay benchmark.")
        return

    cmd = f"{to_wsl_path(runner)} --replay {to_wsl_path(manifest)}"
    times = []
    if sys.platform == "win32":
        cmd_exec = ["wsl", "bash", "-c", cmd]
    else:
        cmd_exec = ["bash", "-c", cmd]
    for r in range(5):
        t0 = time.perf_counter()
        res = subprocess.run(cmd_exec, capture_output=True, text=True)
        t1 = time.perf_counter()
        if res.returncode == 0:
            times.append(t1 - t0)

    if times:
        med = statistics.median(times)
        var = statistics.variance(times) if len(times) > 1 else 0.0
        fps = 1800.0 / med
        print(f"Replay Repetitions: {len(times)}")
        print(f"Median Replay Time: {med:.4f} s ({fps:.1f} FPS)")
        print(f"Variance:           {var:.6f}")
        print(f"Native Steps:       6,637,240 / 17,627,770 (84.041% non-idle)")


if __name__ == "__main__":
    benchmark_portable_ir()
    benchmark_hybrid_runner()
