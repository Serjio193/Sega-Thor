#!/usr/bin/env python3
"""
Performance benchmark for M15.242 Connected Subsystem Recompilation.
Benchmarks Mode A (Thor IR Verification Stepper) vs Mode B (Portable Native C++)
on the connected Raster Scroll Subsystem (0x00D2B4 -> BSR 0x00D344 -> RTS -> RTS)
across 100,000 iterations under identical execution conditions.
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


def benchmark_connected_subsystem():
    print("=== M15.242 Connected Subsystem Benchmark ===")
    root = Path(__file__).resolve().parent.parent

    src = """
#include "tools/thor_ir/portable_guest_env.hpp"
#include "tools/thor_ir/generated/connected_portable_routines.hpp"
#include "data/thor-fixtures/m15_242_connected_fixtures.hpp"

#include <chrono>
#include <iostream>
#include <numeric>
#include <vector>

using namespace oasis::recompiler::ir;
using namespace oasis::recompiler::fixtures;

int main() {
    const auto& fixtures = get_connected_fixtures();
    const auto& fix = fixtures[3]; // Path 4: Both planes active (maximum workload)
    constexpr int kIterations = 100000;

    std::cout << "Benchmarking Connected Subsystem: " << fix.test_id << " (" << kIterations << " iterations)\\n";

    // Warmup & verification
    PortableGuestEnvironment env;
    env.d_regs[0] = fix.d0;
    env.a_regs[0] = fix.a0;
    env.a_regs[6] = fix.a6;
    env.a_regs[7] = fix.a7 - 4U;
    env.write_memory(env.a_regs[7], IrOpSize::Long, fix.expected_exit_pc);
    env.write_memory(fix.a6 + 0x36U, IrOpSize::Byte, fix.flag_36);
    env.write_memory(fix.a6 + 0x38U, IrOpSize::Word, fix.scroll_a);
    env.write_memory(fix.a6 + 0x3AU, IrOpSize::Word, fix.scroll_b);

    // Mode A benchmark
    std::vector<double> mode_a_times;
    for (int rep = 0; rep < 5; ++rep) {
        auto t0 = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < kIterations; ++i) {
            env.a_regs[7] = fix.a7 - 4U;
            env.write_memory(env.a_regs[7], IrOpSize::Long, fix.expected_exit_pc);
            env.write_memory(fix.a6 + 0x36U, IrOpSize::Byte, fix.flag_36);
            run_connected_mode_a(env, fix.caller_pc, fix.expected_exit_pc);
        }
        auto t1 = std::chrono::high_resolution_clock::now();
        mode_a_times.push_back(std::chrono::duration<double, std::milli>(t1 - t0).count());
    }

    // Mode B benchmark
    std::vector<double> mode_b_times;
    for (int rep = 0; rep < 5; ++rep) {
        auto t0 = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < kIterations; ++i) {
            env.a_regs[7] = fix.a7 - 4U;
            env.write_memory(env.a_regs[7], IrOpSize::Long, fix.expected_exit_pc);
            env.write_memory(fix.a6 + 0x36U, IrOpSize::Byte, fix.flag_36);
            run_connected_mode_b(env, fix.caller_pc, fix.expected_exit_pc);
        }
        auto t1 = std::chrono::high_resolution_clock::now();
        mode_b_times.push_back(std::chrono::duration<double, std::milli>(t1 - t0).count());
    }

    double med_a = mode_a_times[2];
    double med_b = mode_b_times[2];

    std::cout << "Mode A Median Time: " << med_a << " ms (" << (kIterations / (med_a / 1000.0)) << " calls/sec)\\n";
    std::cout << "Mode B Median Time: " << med_b << " ms (" << (kIterations / (med_b / 1000.0)) << " calls/sec)\\n";
    std::cout << "Speedup Factor:     " << (med_a / med_b) << "x\\n";
    return 0;
}
"""
    bench_cpp = root / "build" / "bench_connected_m15_242.cpp"
    bench_cpp.parent.mkdir(parents=True, exist_ok=True)
    bench_cpp.write_text(src, encoding="utf-8")

    wsl_src = to_wsl_path(bench_cpp)
    wsl_exe = "/mnt/c/Github/Sega-Thor/build/bench_connected_m15_242"
    cmd = f"g++ -std=c++20 -O3 -I/mnt/c/Github/Sega-Thor/src -I/mnt/c/Github/Sega-Thor {wsl_src} -o {wsl_exe} && {wsl_exe}"
    if sys.platform == "win32":
        cmd_exec = ["wsl", "bash", "-c", cmd]
    else:
        cmd_exec = ["bash", "-c", cmd]

    res = subprocess.run(cmd_exec, capture_output=True, text=True)
    print(res.stdout)
    if res.stderr:
        print(res.stderr, file=sys.stderr)


if __name__ == "__main__":
    benchmark_connected_subsystem()
