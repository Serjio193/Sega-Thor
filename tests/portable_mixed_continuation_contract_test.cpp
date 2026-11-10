#include "tools/thor_ir/portable_guest_env.hpp"
#include "tools/thor_ir/generated/connected_portable_routines.hpp"

#include <cassert>
#include <iostream>
#include <vector>

using namespace oasis::recompiler::ir;

void test_native_to_native_connected_call() {
    PortableGuestEnvironment env;
    env.d_regs[0] = 0;
    env.d_regs[1] = 0;
    env.a_regs[6] = 0x00FF8000U;
    env.a_regs[7] = 0x00FFFE00U;
    env.sr = 0x2700U;

    // Set 0x36(A6) = 0x01 (Plane A update requested, Plane B unclamped)
    env.write_memory(0x00FF8036U, IrOpSize::Byte, 0x01U);
    env.write_memory(0x00FF8038U, IrOpSize::Word, 0x0042U); // Plane A scroll val
    env.write_memory(0x00FF1654U, IrOpSize::Byte, 0x00U);   // Single line mode

    // Sentinel return address on stack for top-level caller
    constexpr std::uint32_t kTopCallerPc = 0x02CAD8U;
    env.a_regs[7] -= 4U;
    env.write_memory(env.a_regs[7], IrOpSize::Long, kTopCallerPc);

    // Run connected subsystem starting at caller 0x00D2B4 until it returns to kTopCallerPc
    const auto res = run_connected_mode_a(env, 0x00D2B4U, kTopCallerPc);

    assert(res.outcome == PortableContinuationOutcome::CompiledNextPc);
    assert(env.pc == kTopCallerPc);
    assert(env.a_regs[7] == 0x00FFFE00U); // Stack balanced back to initial
    // Verify Plane A was written at 0x00FFA002 (since CMPA FFA200 != FFA000 -> ADDQ #2 -> MOVE D0, (A0))
    const auto written_val = env.read_memory(0x00FFA002U, IrOpSize::Word);
    assert(written_val == 0x0042U);
    std::cout << "PASS: test_native_to_native_connected_call\n";
}

void test_native_to_uncompiled_continuation() {
    PortableGuestEnvironment env;
    env.d_regs[0] = 0;
    env.a_regs[6] = 0x00FF8000U;
    env.a_regs[7] = 0x00FFFE00U;
    env.sr = 0x2700U;

    // Put an uncompiled target address directly on stack
    constexpr std::uint32_t kUncompiledTarget = 0x050000U;
    env.a_regs[7] -= 4U;
    env.write_memory(env.a_regs[7], IrOpSize::Long, kUncompiledTarget);

    // Run 0x00D344 with fast exit (0x36(A6) = 0)
    env.write_memory(0x00FF8036U, IrOpSize::Byte, 0x00U);
    const auto res = run_connected_mode_a(env, 0x00D344U, 0x000000U);

    // Hits uncompiled return address and safely halts
    assert(res.outcome == PortableContinuationOutcome::ValidUncompiledPc);
    assert(env.pc == kUncompiledTarget);
    assert(env.a_regs[7] == 0x00FFFE00U);
    std::cout << "PASS: test_native_to_uncompiled_continuation\n";
}

void test_uncompiled_to_native_entry() {
    PortableGuestEnvironment env;
    env.a_regs[6] = 0x00FF8000U;
    env.a_regs[7] = 0x00FFFE00U;

    // Simulate returning into 0x00D2B8 directly from uncompiled interpreter
    constexpr std::uint32_t kTopCallerPc = 0x02CAD8U;
    env.a_regs[7] -= 4U;
    env.write_memory(env.a_regs[7], IrOpSize::Long, kTopCallerPc);

    // Set 0x36(A6) bit 1 = 0 (Plane B not requested)
    env.write_memory(0x00FF8036U, IrOpSize::Byte, 0x00U);

    const auto res = run_connected_mode_a(env, 0x00D2B8U, kTopCallerPc);
    assert(res.outcome == PortableContinuationOutcome::CompiledNextPc);
    assert(env.pc == kTopCallerPc);
    assert(env.a_regs[7] == 0x00FFFE00U);
    std::cout << "PASS: test_uncompiled_to_native_entry\n";
}

void test_nested_call_stack_conservation() {
    PortableGuestEnvironment env;
    env.a_regs[6] = 0x00FF8000U;
    env.a_regs[7] = 0x00FFFE00U;

    // Top caller 1 -> Top caller 2 -> 0x00D2B4 -> 0x00D344
    constexpr std::uint32_t kGrandParent = 0x020000U;
    constexpr std::uint32_t kParent = 0x02CAD8U;

    env.a_regs[7] -= 4U;
    env.write_memory(env.a_regs[7], IrOpSize::Long, kGrandParent);
    env.a_regs[7] -= 4U;
    env.write_memory(env.a_regs[7], IrOpSize::Long, kParent);

    // Disable Plane A and B updates to test pure call/return conservation
    env.write_memory(0x00FF8036U, IrOpSize::Byte, 0x00U);

    // 0x00D2B4 calls 0x00D344 (pushes return 0x00D2B8), 0x00D344 returns to 0x00D2B8, 0x00D2B4 returns to kParent
    const auto res = run_connected_mode_a(env, 0x00D2B4U, kParent);
    assert(res.outcome == PortableContinuationOutcome::CompiledNextPc);
    assert(env.pc == kParent);
    assert(env.a_regs[7] == 0x00FFFDFCU); // Still has kGrandParent on stack

    // Now pop parent frame
    const std::uint32_t popped_gp = env.read_memory(env.a_regs[7], IrOpSize::Long);
    env.a_regs[7] += 4U;
    assert(popped_gp == kGrandParent);
    assert(env.a_regs[7] == 0x00FFFE00U); // Fully balanced
    std::cout << "PASS: test_nested_call_stack_conservation\n";
}

void test_interrupt_preemption_and_rte() {
    PortableGuestEnvironment env;
    env.pc = 0x00D344U;
    env.sr = 0x2000U; // Supervisor mode
    env.ssp = 0x00FFFF00U;
    env.a_regs[7] = env.ssp;

    // Preempt at 0x00D344 with Level 6 VBlank IRQ (vector 0x00000600)
    constexpr std::uint32_t kVBlankVector = 0x00000600U;
    env.enter_interrupt(6, kVBlankVector);

    assert(env.pc == kVBlankVector);
    assert((env.sr & kSrIrqMask) == 0x0600U);
    assert((env.sr & kSrSupervisor) != 0U);
    assert(env.a_regs[7] == 0x00FFFEFAU); // 6 bytes pushed (4 PC + 2 SR: 0x00FFFF00 - 6 = 0x00FFFEFA)

    // Execute RTE to return to interrupted instruction
    env.execute_rte();

    assert(env.pc == 0x00D344U);
    assert(env.sr == 0x2000U);
    assert(env.a_regs[7] == 0x00FFFF00U);
    std::cout << "PASS: test_interrupt_preemption_and_rte\n";
}

void test_original_cpu_exceptions() {
    PortableGuestEnvironment env;

    // Test 1: Misaligned Word Write -> Address Error Vector 3
    env.address_error = false;
    env.write_memory(0x00FF8001U, IrOpSize::Word, 0x1234U);
    assert(env.address_error);

    // Test 2: Unmapped cartridge bus read -> Bus Error Vector 2
    env.bus_error = false;
    env.read_memory(0x00380000U, IrOpSize::Word);
    assert(env.bus_error);

    // Test 3: Read-only ROM write violation
    env.rom_write_violation = false;
    env.write_memory(0x00001000U, IrOpSize::Word, 0x1234U);
    assert(env.rom_write_violation);

    std::cout << "PASS: test_original_cpu_exceptions\n";
}

int main() {
    std::cout << "=== Running M15.242 Safe Mixed Continuation Contract Tests ===\n";
    test_native_to_native_connected_call();
    test_native_to_uncompiled_continuation();
    test_uncompiled_to_native_entry();
    test_nested_call_stack_conservation();
    test_interrupt_preemption_and_rte();
    test_original_cpu_exceptions();
    std::cout << "ALL MIXED CONTINUATION CONTRACT TESTS PASSED!\n";
    return 0;
}
