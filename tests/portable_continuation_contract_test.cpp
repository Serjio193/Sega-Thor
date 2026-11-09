#include "tools/thor_ir/portable_guest_env.hpp"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

#define TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion failed at " << __FILE__ << ":" << __LINE__ << ": " #cond << "\n"; \
            std::exit(1); \
        } \
    } while (false)

namespace {

using namespace oasis::recompiler::ir;

// ============================================================================
// PHASE 5: COMPLETE GUEST CPU STATE TESTS
// ============================================================================

void test_registers_and_supervisor_stacks() {
    PortableGuestEnvironment env;

    // Default SR is 0x2700 (Supervisor mode, IRQ 7)
    TEST_ASSERT((env.sr & kSrSupervisor) != 0);
    env.a_regs[7] = 0x00FFFF00U; // Supervisor SP
    TEST_ASSERT(env.read_register(IrRegister{IrRegKind::Address, 7}) == 0x00FFFF00U);

    // Set User mode (clear bit 13)
    env.write_register(IrRegister{IrRegKind::SR, 0}, 0x0000U);
    TEST_ASSERT((env.sr & kSrSupervisor) == 0);
    TEST_ASSERT(env.ssp == 0x00FFFF00U); // SSP preserved
    TEST_ASSERT(env.a_regs[7] == 0x00FFFE00U); // Swapped to USP

    // Set User SP
    env.write_register(IrRegister{IrRegKind::Address, 7}, 0x00FFFD50U);
    TEST_ASSERT(env.read_register(IrRegister{IrRegKind::Address, 7}) == 0x00FFFD50U);

    // Switch back to Supervisor mode
    env.write_register(IrRegister{IrRegKind::SR, 0}, 0x2000U);
    TEST_ASSERT(env.usp == 0x00FFFD50U); // USP preserved
    TEST_ASSERT(env.a_regs[7] == 0x00FFFF00U); // Restored SSP

    // CCR flags (X, N, Z, V, C)
    env.write_ccr(kFlagX | kFlagZ | kFlagC);
    TEST_ASSERT(env.read_ccr() == (kFlagX | kFlagZ | kFlagC));
    TEST_ASSERT((env.sr & 0x1FU) == (kFlagX | kFlagZ | kFlagC));

    std::cout << "PASS: test_registers_and_supervisor_stacks\n";
}

void test_stack_operations_and_address_wraparound() {
    PortableGuestEnvironment env;
    env.a_regs[7] = 0x00FFFF00U;

    // Push Word to stack: A7 -= 2, write Word
    env.a_regs[7] -= 2;
    env.write_memory(env.a_regs[7], IrOpSize::Word, 0x1234U);
    TEST_ASSERT(env.a_regs[7] == 0x00FFFEFEU);
    TEST_ASSERT(env.read_memory(env.a_regs[7], IrOpSize::Word) == 0x1234U);

    // Push Long to stack: A7 -= 4, write Long
    env.a_regs[7] -= 4;
    env.write_memory(env.a_regs[7], IrOpSize::Long, 0xAABBCCDDU);
    TEST_ASSERT(env.a_regs[7] == 0x00FFFEFAU);
    TEST_ASSERT(env.read_memory(env.a_regs[7], IrOpSize::Long) == 0xAABBCCDDU);

    // Pop Long from stack: read Long, A7 += 4
    const auto popped_l = env.read_memory(env.a_regs[7], IrOpSize::Long);
    env.a_regs[7] += 4;
    TEST_ASSERT(popped_l == 0xAABBCCDDU);
    TEST_ASSERT(env.a_regs[7] == 0x00FFFEFEU);

    // Pop Word from stack: read Word, A7 += 2
    const auto popped_w = env.read_memory(env.a_regs[7], IrOpSize::Word);
    env.a_regs[7] += 2;
    TEST_ASSERT(popped_w == 0x1234U);
    TEST_ASSERT(env.a_regs[7] == 0x00FFFF00U);

    // 24-bit address wraparound
    env.write_memory(0x01FF0000U, IrOpSize::Word, 0xCAFEU);
    TEST_ASSERT(env.read_memory(0x00FF0000U, IrOpSize::Word) == 0xCAFEU);
    TEST_ASSERT(env.read_memory(0xFEFF0000U, IrOpSize::Word) == 0xCAFEU);

    std::cout << "PASS: test_stack_operations_and_address_wraparound\n";
}

// ============================================================================
// PHASE 7: PORTABLE INSTRUCTION CONTINUATION CONTRACT TESTS
// ============================================================================

struct ContinuationStepper {
    PortableGuestEnvironment env;
    std::uint32_t step_count{0};
    std::vector<std::pair<std::uint32_t, std::uint8_t>> writes;

    void step_one_instruction() {
        env.on_sync_boundary(env.pc);
        step_count++;

        if (env.pc == 0x00E682U) {
            // Insn 1: BCLR.B #6, $36(A6)
            const std::uint32_t addr = (env.a_regs[6] + 0x36U) & kBusAddressMask;
            const auto val = static_cast<std::uint8_t>(env.read_memory(addr, IrOpSize::Byte));
            const bool is_zero = (val & (1U << 6U)) == 0U;
            const auto new_val = static_cast<std::uint8_t>(val & ~(1U << 6U));
            env.write_memory(addr, IrOpSize::Byte, new_val);
            writes.emplace_back(addr, new_val);
            env.write_ccr(is_zero ? (env.read_ccr() | kFlagZ) : (env.read_ccr() & ~kFlagZ));
            env.add_cycles(12);
            env.pc = 0x00E688U;
        } else if (env.pc == 0x00E688U) {
            // Insn 2: MOVE.B $9D(A6), D0
            const std::uint32_t addr = (env.a_regs[6] + 0x9DU) & kBusAddressMask;
            const auto val = static_cast<std::uint8_t>(env.read_memory(addr, IrOpSize::Byte));
            env.d_regs[0] = (env.d_regs[0] & 0xFFFFFF00U) | val;
            std::uint16_t ccr = env.read_ccr() & ~(kFlagN | kFlagZ | kFlagV | kFlagC);
            if (val == 0) ccr |= kFlagZ;
            if (val & 0x80U) ccr |= kFlagN;
            env.write_ccr(ccr);
            env.add_cycles(8);
            env.pc = 0x00E68CU;
        } else if (env.pc == 0x00E68CU) {
            // Insn 3: BNE.W 0x00E6AA
            env.add_cycles(10);
            if ((env.read_ccr() & kFlagZ) == 0U) {
                env.pc = 0x00E6AAU;
            } else {
                env.pc = 0x00E690U;
            }
        } else if (env.pc == 0x00E690U) {
            // Insn 4: RTS / Exit
            env.add_cycles(16);
            env.pc = 0x00E6A8U;
        }
    }
};

void test_yield_before_and_after_branch() {
    ContinuationStepper stepper;
    stepper.env.a_regs[6] = 0x00FF8000U;
    stepper.env.pc = 0x00E682U;
    stepper.env.write_memory(0x00FF8036U, IrOpSize::Byte, 0x40U);
    stepper.env.write_memory(0x00FF809DU, IrOpSize::Byte, 0x01U); // Triggers BNE

    // Step 1: BCLR
    stepper.step_one_instruction();
    TEST_ASSERT(stepper.env.pc == 0x00E688U);
    TEST_ASSERT(stepper.env.total_cycles == 12);
    TEST_ASSERT(stepper.writes.size() == 1);

    // Step 2: MOVE.B
    stepper.step_one_instruction();
    TEST_ASSERT(stepper.env.pc == 0x00E68CU); // Yielded BEFORE branch!
    TEST_ASSERT(stepper.env.total_cycles == 20);
    TEST_ASSERT(stepper.env.d_regs[0] == 0x01U);

    // Yield verified: guest state is frozen and exact
    const auto frozen_cycles = stepper.env.total_cycles;
    const auto frozen_pc = stepper.env.pc;
    const auto frozen_d0 = stepper.env.d_regs[0];

    // Resume from yield: execute BNE
    stepper.step_one_instruction();
    TEST_ASSERT(stepper.env.pc == 0x00E6AAU); // Branch taken!
    TEST_ASSERT(stepper.env.total_cycles == frozen_cycles + 10);
    TEST_ASSERT(stepper.env.d_regs[0] == frozen_d0);
    TEST_ASSERT(stepper.writes.size() == 1); // No duplicate writes!

    std::cout << "PASS: test_yield_before_and_after_branch\n";
}

void test_yield_before_and_after_memory_write() {
    ContinuationStepper stepper;
    stepper.env.a_regs[6] = 0x00FF8000U;
    stepper.env.pc = 0x00E682U;
    stepper.env.write_memory(0x00FF8036U, IrOpSize::Byte, 0x40U);

    // Before Step 1: Memory is 0x40, no writes recorded
    TEST_ASSERT(stepper.writes.empty());
    TEST_ASSERT(stepper.env.read_memory(0x00FF8036U, IrOpSize::Byte) == 0x40U);

    // Step 1: Execute BCLR (which writes 0x00)
    stepper.step_one_instruction();
    TEST_ASSERT(stepper.writes.size() == 1);
    TEST_ASSERT(stepper.env.read_memory(0x00FF8036U, IrOpSize::Byte) == 0x00U);

    // Yielded after memory write: resume at 0x00E688
    TEST_ASSERT(stepper.env.pc == 0x00E688U);
    stepper.env.write_memory(0x00FF809DU, IrOpSize::Byte, 0x00U);
    stepper.step_one_instruction(); // MOVE.B (reads, does not write)
    TEST_ASSERT(stepper.writes.size() == 1); // Exact count preserved

    std::cout << "PASS: test_yield_before_and_after_memory_write\n";
}

void test_caller_callee_yield_and_rts() {
    PortableGuestEnvironment env;
    env.a_regs[7] = 0x00FFFF00U;
    env.pc = 0x002000U; // Caller PC

    // 1. Caller executes BSR 0x003000:
    // Pushes return address 0x002004 onto stack, transfers PC to 0x003000
    const std::uint32_t return_pc = env.pc + 4;
    env.a_regs[7] -= 4;
    env.write_memory(env.a_regs[7], IrOpSize::Long, return_pc);
    env.pc = 0x003000U;
    env.add_cycles(18);

    // 2. Yield between caller and callee
    TEST_ASSERT(env.pc == 0x003000U);
    TEST_ASSERT(env.a_regs[7] == 0x00FFFEFCU);
    TEST_ASSERT(env.read_memory(env.a_regs[7], IrOpSize::Long) == 0x002004U);

    // 3. Callee executes inside and yields before RTS
    env.d_regs[0] = 0x42U;
    env.add_cycles(8);
    // Yield before RTS
    TEST_ASSERT(env.pc == 0x003000U);
    TEST_ASSERT(env.read_memory(env.a_regs[7], IrOpSize::Long) == 0x002004U);

    // 4. Callee resumes and executes RTS:
    const auto popped_pc = env.read_memory(env.a_regs[7], IrOpSize::Long);
    env.a_regs[7] += 4;
    env.pc = popped_pc;
    env.add_cycles(16);

    // Returned to caller with exact registers and clean stack!
    TEST_ASSERT(env.pc == 0x002004U);
    TEST_ASSERT(env.a_regs[7] == 0x00FFFF00U);
    TEST_ASSERT(env.d_regs[0] == 0x42U);
    TEST_ASSERT(env.total_cycles == 18 + 8 + 16);

    std::cout << "PASS: test_caller_callee_yield_and_rts\n";
}

} // namespace

int main() {
    std::cout << "=== Running M15.241 Complete Guest CPU State & Continuation Tests ===\n";
    test_registers_and_supervisor_stacks();
    test_stack_operations_and_address_wraparound();
    test_yield_before_and_after_branch();
    test_yield_before_and_after_memory_write();
    test_caller_callee_yield_and_rts();
    std::cout << "ALL GUEST CPU STATE & CONTINUATION CONTRACT TESTS PASSED!\n";
    return 0;
}
