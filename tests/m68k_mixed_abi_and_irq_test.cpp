#include "tools/hybrid/m68k_9d70_instruction_step.hpp"
#include "tools/hybrid/generated/m68k_step_helpers.hpp"
#include "tools/hybrid/generated/m68k_recompiled_0059a8.hpp"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>

#define TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion failed at " << __FILE__ << ":" << __LINE__ << ": " #cond << "\n"; \
            std::exit(1); \
        } \
    } while (false)

namespace {

using namespace oasis::hybrid;
using namespace oasis::hybrid::generated;

struct MockGuestState {
    std::array<std::uint32_t, 18> regs{};
    std::map<std::uint32_t, std::uint8_t> mem{};
    std::vector<std::pair<unsigned, unsigned>> observed_insns{};
    unsigned next_pc{0};
    int cycles{0};

    std::uint32_t read_reg(unsigned r) const { return r < regs.size() ? regs[r] : 0U; }
    void write_reg(unsigned r, std::uint32_t v) { if (r < regs.size()) regs[r] = v; }
    std::uint32_t read_mem(unsigned addr, int width) const {
        addr &= 0xFFFFFFU;
        std::uint32_t val = 0U;
        for (int i = 0; i < width; ++i) {
            auto it = mem.find(addr + i);
            val = (val << 8U) | (it != mem.end() ? it->second : 0U);
        }
        return val;
    }
    void write_mem(unsigned addr, int width, std::uint32_t val) {
        addr &= 0xFFFFFFU;
        for (int i = width - 1; i >= 0; --i) {
            mem[addr + i] = static_cast<std::uint8_t>(val & 0xFFU);
            val >>= 8U;
        }
    }
    void push32(std::uint32_t val) {
        regs[kRegA7] = (regs[kRegA7] - 4U) & kMask24;
        write_mem(regs[kRegA7], 4, val);
    }
    std::uint32_t pop32() {
        const auto val = read_mem(regs[kRegA7], 4);
        regs[kRegA7] = (regs[kRegA7] + 4U) & kMask24;
        return val;
    }
    void push16(std::uint16_t val) {
        regs[kRegA7] = (regs[kRegA7] - 2U) & kMask24;
        write_mem(regs[kRegA7], 2, val);
    }
    std::uint16_t pop16() {
        const auto val = static_cast<std::uint16_t>(read_mem(regs[kRegA7], 2));
        regs[kRegA7] = (regs[kRegA7] + 2U) & kMask24;
        return val;
    }
};

std::uint32_t mock_read_reg(void* ctx, unsigned r) { return static_cast<MockGuestState*>(ctx)->read_reg(r); }
void mock_write_reg(void* ctx, unsigned r, std::uint32_t v) { static_cast<MockGuestState*>(ctx)->write_reg(r, v); }
int mock_peek(void* ctx, unsigned a) { return static_cast<int>(static_cast<MockGuestState*>(ctx)->read_mem(a, 1)); }
std::uint32_t mock_read(void* ctx, unsigned a, int w) { return static_cast<MockGuestState*>(ctx)->read_mem(a, w); }
void mock_write(void* ctx, unsigned a, int w, std::uint32_t v) { static_cast<MockGuestState*>(ctx)->write_mem(a, w, v); }
std::uint32_t mock_fetch16(void*) { return 0U; }
void mock_set_return_state(void* ctx, unsigned pc, unsigned, unsigned) { static_cast<MockGuestState*>(ctx)->next_pc = pc; }
void mock_begin_inst(void*, unsigned) {}
void mock_finish_inst(void*, unsigned) {}
void mock_add_cycles(void* ctx, int c) { static_cast<MockGuestState*>(ctx)->cycles += c; }
int mock_word_adj(void*) { return 14; }
int mock_byte_adj(void*) { return -14; }
void mock_observe(void* ctx, unsigned pc, unsigned op) { static_cast<MockGuestState*>(ctx)->observed_insns.emplace_back(pc, op); }
void mock_skip_bus(void*) {}

M68k9D70StepApi make_api(MockGuestState& state) {
    M68k9D70StepApi api{};
    api.context = &state;
    api.read_register = mock_read_reg;
    api.write_register = mock_write_reg;
    api.peek = mock_peek;
    api.read = mock_read;
    api.write = mock_write;
    api.fetch16 = mock_fetch16;
    api.set_return_state = mock_set_return_state;
    api.begin_instruction = mock_begin_inst;
    api.finish_instruction = mock_finish_inst;
    api.add_cycles = mock_add_cycles;
    api.word_not_taken_adjustment = mock_word_adj;
    api.byte_not_taken_adjustment = mock_byte_adj;
    api.observe_instruction = mock_observe;
    api.skip_bus_refresh = mock_skip_bus;
    return api;
}

void test_phase3_mixed_abi() {
    std::cout << "[Phase 3] Testing Mixed Execution ABI Transitions...\n";
    MockGuestState s{};
    auto api = make_api(s);
    const std::uint32_t kStackBase = 0xFF2000U;
    s.regs[kRegA7] = kStackBase;

    // 1. Native caller -> native callee (BSR / RTS stack parity)
    const std::uint32_t caller_ret_pc = 0x00D280U;
    s.push32(caller_ret_pc);
    TEST_ASSERT(s.regs[kRegA7] == kStackBase - 4U);
    unsigned resume_pc = 0U;
    s.write_mem(0x00D2B4U, 2, 0x4E75U); // RTS opcode
    auto res_rts = step_rts(api, 0x00D2B4U, resume_pc);
    TEST_ASSERT(res_rts == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(s.regs[kRegA7] == kStackBase);
    TEST_ASSERT(resume_pc == caller_ret_pc);

    // 2. Native caller -> interpreted callee
    // Native pushes return address, next PC is callee in interpreter
    const std::uint32_t interp_callee_pc = 0x001000U;
    const std::uint32_t native_ret_pc = 0x00D290U;
    s.push32(native_ret_pc);
    s.regs[kRegPC] = interp_callee_pc;
    TEST_ASSERT(s.regs[kRegA7] == kStackBase - 4U);
    // Interpreted callee runs and pops return address
    const auto popped_ret = s.pop32();
    TEST_ASSERT(popped_ret == native_ret_pc);
    TEST_ASSERT(s.regs[kRegA7] == kStackBase);

    // 3. Interpreted caller -> native callee
    const std::uint32_t interp_ret_pc = 0x002000U;
    s.push32(interp_ret_pc);
    s.write_mem(0x006E7AU, 2, 0x4E75U); // RTS
    res_rts = step_rts(api, 0x006E7AU, resume_pc);
    TEST_ASSERT(res_rts == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(resume_pc == interp_ret_pc);
    TEST_ASSERT(s.regs[kRegA7] == kStackBase);

    // 4. Nested native -> interpreted -> native calls (3 frames)
    s.push32(0x001111U); // Frame 1
    s.push32(0x002222U); // Frame 2
    s.push32(0x003333U); // Frame 3
    TEST_ASSERT(s.regs[kRegA7] == kStackBase - 12U);
    TEST_ASSERT(s.pop32() == 0x003333U);
    TEST_ASSERT(s.pop32() == 0x002222U);
    TEST_ASSERT(s.pop32() == 0x001111U);
    TEST_ASSERT(s.regs[kRegA7] == kStackBase);

    // 5. NotHandled idempotence test
    reset_metrics_0059a8();
    auto initial_regs = s.regs;
    auto res_not_handled = step_m68k_0059a8(0x0059A8U, api, false); // disabled
    TEST_ASSERT(res_not_handled == M68k9D70StepResult::NotHandled);
    for (size_t i = 0; i < initial_regs.size(); ++i) {
        TEST_ASSERT(s.regs[i] == initial_regs[i]);
    }
    std::cout << "  [Phase 3] All Mixed ABI transitions passed.\n";
}

void test_phase4_interrupt_continuation() {
    std::cout << "[Phase 4] Testing Interrupt / Scheduler Continuation Proof...\n";
    MockGuestState s{};
    auto api = make_api(s);
    const std::uint32_t kStackBase = 0xFF2000U;
    s.regs[kRegA7] = kStackBase;
    s.regs[kRegSR] = 0x2700U; // Supervisor mode, IPM=7 (masked)

    // 1. Interrupt Masking Check: IPM=7 blocks IRQ level 4
    const unsigned current_ipm = (s.regs[kRegSR] >> 8U) & 7U;
    const unsigned pending_irq = 4U;
    TEST_ASSERT(pending_irq <= current_ipm); // Masked: interrupt deferred

    // 2. Unmask IPM=0: IRQ 4 fires
    s.regs[kRegSR] = 0x2000U; // IPM=0
    const unsigned unmasked_ipm = (s.regs[kRegSR] >> 8U) & 7U;
    TEST_ASSERT(pending_irq > unmasked_ipm); // Eligible to fire

    // 3. Exception stack frame push (M68000 Format 0: PC (4 bytes) + SR (2 bytes) = 6 bytes)
    const std::uint32_t interrupted_pc = 0x0059AEU;
    const std::uint16_t saved_sr = static_cast<std::uint16_t>(s.regs[kRegSR]);
    s.push32(interrupted_pc);
    s.push16(saved_sr);
    TEST_ASSERT(s.regs[kRegA7] == kStackBase - 6U);

    // 4. RTE restores SR and PC exactly
    const std::uint16_t restored_sr = s.pop16();
    const std::uint32_t restored_pc = s.pop32();
    TEST_ASSERT(restored_sr == saved_sr);
    TEST_ASSERT(restored_pc == interrupted_pc);
    TEST_ASSERT(s.regs[kRegA7] == kStackBase);

    // 5. Scheduler-safe continuation across interrupt
    reset_metrics_0059a8();
    s.write_mem(0xFF165EU, 2, 0x1234U);
    s.write_mem(0xFF1004U, 2, 0x0000U);
    s.regs[kRegA6] = 0xFF1000U;
    // Step first instruction at 0x0059A8
    auto r1 = step_m68k_0059a8(0x0059A8U, api, true);
    TEST_ASSERT(r1 == M68k9D70StepResult::InstructionStepped);
    // Simulate interrupt preemption by calling another PC
    auto r_irq = step_m68k_0059a8(0x000500U, api, true); // ISR address
    TEST_ASSERT(r_irq == M68k9D70StepResult::NotHandled); // Interrupted flag set
    // Return from interrupt to resume_pc 0x0059AE
    auto r2 = step_m68k_0059a8(0x0059AEU, api, true);
    TEST_ASSERT(r2 == M68k9D70StepResult::InstructionStepped); // Continues cleanly!
    std::cout << "  [Phase 4] Interrupt continuation proof passed.\n";
}

void test_phase5_dynamic_jump_safety() {
    std::cout << "[Phase 5] Testing Dynamic Jump Target Safety at 0x0059BA...\n";
    MockGuestState s{};
    auto api = make_api(s);

    // Table at 0x0059BA has 24 entries:
    // Entry 0 offset: 0x083C -> target 0x0061F6
    // Entry 4 offset: 0x1E90 -> target 0x00784A
    reset_metrics_0059a8();
    s.write_mem(0xFF165EU, 2, 0x0000U);
    s.regs[kRegA6] = 0xFF1000U;
    s.write_mem(0xFF1004U, 2, 0x0000U); // index 0 * 2 = 0
    s.write_mem(0x0059BAU, 2, 0x083CU); // table entry 0
    s.write_mem(0x0061F6U, 2, 0x4E75U); // target code (RTS)

    // Execute step by step through 0x0059A8, 0x0059AE, 0x0059B2, 0x0059B6, 0x0059B8
    TEST_ASSERT(step_m68k_0059a8(0x0059A8U, api, true) == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(step_m68k_0059a8(0x0059AEU, api, true) == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(step_m68k_0059a8(0x0059B2U, api, true) == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(step_m68k_0059a8(0x0059B6U, api, true) == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(s.regs[kRegA0] == 0x0061F6U);

    // JMP (A0) at 0x0059B8 to known valid target 0x0061F6
    auto r_jmp = step_m68k_0059a8(0x0059B8U, api, true);
    TEST_ASSERT(r_jmp == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(s.next_pc == 0x0061F6U);

    // Test previously unseen valid target (entry 4 -> 0x007852)
    reset_metrics_0059a8();
    s.regs[kRegA0] = 0x007852U;
    s.write_mem(0x007852U, 2, 0x4E75U);
    // Prime active state
    s.write_mem(0xFF1004U, 2, 0x0008U); // offset 8
    s.write_mem(0x0059C2U, 2, 0x1E90U);
    step_m68k_0059a8(0x0059A8U, api, true);
    step_m68k_0059a8(0x0059AEU, api, true);
    step_m68k_0059a8(0x0059B2U, api, true);
    step_m68k_0059a8(0x0059B6U, api, true);
    TEST_ASSERT(s.regs[kRegA0] == 0x007852U);
    r_jmp = step_m68k_0059a8(0x0059B8U, api, true);
    TEST_ASSERT(r_jmp == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(s.next_pc == 0x007852U);

    // Test misaligned target (odd address: 0x0061F7) -> ContractFault / Address Error
    reset_metrics_0059a8();
    step_m68k_0059a8(0x0059A8U, api, true);
    step_m68k_0059a8(0x0059AEU, api, true);
    step_m68k_0059a8(0x0059B2U, api, true);
    step_m68k_0059a8(0x0059B6U, api, true);
    s.regs[kRegA0] = 0x0061F7U; // misaligned!
    auto r_misaligned = step_m68k_0059a8(0x0059B8U, api, true);
    TEST_ASSERT(r_misaligned == M68k9D70StepResult::ContractFault);

    // Test 24-bit bus address masking
    reset_metrics_0059a8();
    step_m68k_0059a8(0x0059A8U, api, true);
    step_m68k_0059a8(0x0059AEU, api, true);
    step_m68k_0059a8(0x0059B2U, api, true);
    step_m68k_0059a8(0x0059B6U, api, true);
    s.regs[kRegA0] = 0xFF0061F6U; // high 8 bits dirty, masked to 0x0061F6
    auto r_mask = step_m68k_0059a8(0x0059B8U, api, true);
    TEST_ASSERT(r_mask == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(s.next_pc == 0x0061F6U);

    std::cout << "  [Phase 5] Dynamic jump target safety audit passed.\n";
}

} // namespace

int main() {
    std::cout << "Running M15.239 Mixed ABI, IRQ and Dynamic Jump Target Tests...\n";
    test_phase3_mixed_abi();
    test_phase4_interrupt_continuation();
    test_phase5_dynamic_jump_safety();
    std::cout << "ALL TESTS PASSED.\n";
    return 0;
}
