#!/usr/bin/env python3
"""
Performance benchmark for M15.243 Connected Entity State Dispatch Subsystem.
Benchmarks Mode A vs Mode B on the connected entity dispatcher (0x0059A8 -> State 12 accumulate)
across 100,000 iterations under identical execution conditions.
Strictly <= 500 lines per AGENTS.md Rule 3.
"""

from pathlib import Path
import subprocess
import sys


def to_wsl_path(path: Path) -> str:
    resolved = path.resolve()
    drive = resolved.drive.replace(":", "").lower()
    posix = resolved.as_posix()
    if resolved.drive:
        posix = posix[len(resolved.drive):]
    return f"/mnt/{drive}{posix}"


def benchmark_entity_dispatch_subsystem():
    print("=== M15.243 Connected Entity Dispatch Subsystem Benchmark ===")
    root = Path(__file__).resolve().parent.parent

    src = """
#include "tools/thor_ir/portable_guest_env.hpp"
#include "tools/thor_ir/generated/entity_dispatch_portable_routines.hpp"
#include "data/thor-fixtures/m15_243_entity_dispatch_fixtures.hpp"

#include <chrono>
#include <iostream>
#include <vector>

using namespace oasis::recompiler::ir;
using namespace oasis::recompiler::fixtures;

static const std::uint8_t kJumpTableBytes[118] = {
    0x08, 0x3c, 0x09, 0x28, 0x0b, 0x58, 0x14, 0x12, 0x1e, 0x90, 0x1c, 0x92, 0x0c, 0x78, 0x18, 0x12,
    0x12, 0x7c, 0x12, 0x6e, 0x20, 0x42, 0x17, 0x42, 0x02, 0xf6, 0x03, 0x24, 0x03, 0xf2, 0x04, 0x0c,
    0x03, 0x1e, 0x03, 0xec, 0x05, 0x92, 0x14, 0xfa, 0x01, 0xc0, 0x01, 0xe6, 0x01, 0xfa, 0x02, 0x32,
    0x09, 0x4e, 0x0e, 0x8a, 0x0f, 0x4e, 0x1f, 0x08, 0x0c, 0xb8, 0x20, 0x34, 0x22, 0x46, 0x0e, 0x08,
    0x1e, 0x14, 0x13, 0x7e, 0x21, 0x2c, 0x1a, 0x3c, 0x15, 0x3a, 0x13, 0x9e, 0x18, 0x5c, 0x18, 0xe2,
    0x19, 0x5a, 0x19, 0xe0, 0x1b, 0x42, 0x15, 0x7c, 0x11, 0x18, 0x11, 0x34, 0x1b, 0x6e, 0x05, 0x7a,
    0x05, 0xd0, 0x15, 0xc8, 0x15, 0xfc, 0x16, 0x30, 0x16, 0x38, 0x0a, 0x40, 0x06, 0xf8, 0x0d, 0xb6,
    0x05, 0xb8, 0x1e, 0xfe, 0x0d, 0x0a
};

int main() {
    const auto& fixtures = get_entity_dispatch_fixtures();
    const auto& fix = fixtures[0]; // Path 1: State 12 branch taken accumulate
    constexpr int kIterations = 100000;

    std::cout << "Benchmarking Entity Dispatch Subsystem: " << fix.test_id << " (" << kIterations << " iterations)\\n";

    PortableGuestEnvironment env;
    env.rom_data.resize(0x0059BAU + sizeof(kJumpTableBytes) + 16, 0);
    for (std::size_t i = 0; i < sizeof(kJumpTableBytes); ++i) env.rom_data[0x0059BAU + i] = kJumpTableBytes[i];
    env.a_regs[6] = fix.a6;

    // Mode A benchmark
    std::vector<double> mode_a_times;
    for (int rep = 0; rep < 5; ++rep) {
        auto t0 = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < kIterations; ++i) {
            env.a_regs[7] = fix.a7 - 4U;
            env.write_memory(env.a_regs[7], IrOpSize::Long, fix.expected_exit_pc);
            env.write_memory(fix.a6 + 0x04U, IrOpSize::Word, fix.state_idx);
            env.write_memory(fix.a6 + 0x83U, IrOpSize::Byte, fix.timer_83);
            run_entity_dispatch_mode_a(env, fix.entry_pc, fix.expected_exit_pc);
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
            env.write_memory(fix.a6 + 0x04U, IrOpSize::Word, fix.state_idx);
            env.write_memory(fix.a6 + 0x83U, IrOpSize::Byte, fix.timer_83);
            run_entity_dispatch_mode_b(env, fix.entry_pc, fix.expected_exit_pc);
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
    bench_cpp = root / "build" / "bench_entity_dispatch_m15_243.cpp"
    bench_cpp.parent.mkdir(parents=True, exist_ok=True)
    bench_cpp.write_text(src, encoding="utf-8")

    wsl_src = to_wsl_path(bench_cpp)
    wsl_exe = "/mnt/c/Github/Sega-Thor/build/bench_entity_dispatch_m15_243"
    cmd = f"g++ -std=c++20 -O3 -I/mnt/c/Github/Sega-Thor/src -I/mnt/c/Github/Sega-Thor {wsl_src} -o {wsl_exe} && {wsl_exe}"
    cmd_exec = ["wsl", "bash", "-c", cmd] if sys.platform == "win32" else ["bash", "-c", cmd]

    res = subprocess.run(cmd_exec, capture_output=True, text=True)
    print(res.stdout)
    if res.stderr:
        print(res.stderr, file=sys.stderr)


if __name__ == "__main__":
    benchmark_entity_dispatch_subsystem()
