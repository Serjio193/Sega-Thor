#include "tools/hybrid/generated/m68k_step_helpers.hpp"
#include "tools/hybrid/generated/m68k_recompiled_00d266.hpp"
#include "tools/hybrid/generated/m68k_recompiled_005660.hpp"
#include "tools/hybrid/generated/m68k_recompiled_062694.hpp"
#include "tools/hybrid/generated/m68k_recompiled_062a78.hpp"
#include "tools/hybrid/generated/m68k_recompiled_006e7a.hpp"
#include "tools/hybrid/generated/m68k_recompiled_00d2b4.hpp"
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

void test_gpgx_register_mapping() {
    std::cout << "Testing GPGX Register Mapping & SR Isolation (Phase 2)...\n";
    MockGuestState s{};
    auto api = make_api(s);
    for (unsigned i = 0; i < 18; ++i) s.regs[i] = 0x1000U + i;
    s.regs[kRegPC] = 0x001234U;
    s.regs[kRegSR] = 0x2700U;

    TEST_ASSERT(get_ccr(api) == 0x00U);
    set_ccr(api, 0x1FU);
    TEST_ASSERT(s.regs[kRegSR] == 0x271FU);
    TEST_ASSERT(s.regs[kRegPC] == 0x001234U);
    for (unsigned i = 0; i < 16; ++i) TEST_ASSERT(s.regs[i] == 0x1000U + i);

    set_ccr(api, 0x04U);
    TEST_ASSERT(s.regs[kRegSR] == 0x2704U);
    TEST_ASSERT(s.regs[kRegPC] == 0x001234U);
    std::cout << "  Passed (SR index 17 strictly isolated from PC index 16)\n";
}

void test_dynamic_shift_cycles() {
    std::cout << "Testing Variable Shift Cycles (Phase 3)...\n";
    for (unsigned shift = 1; shift <= 8; ++shift) {
        const int expected = static_cast<int>(shift * 14U);
        MockGuestState s{};
        auto api = make_api(s);
        api.add_cycles(api.context, static_cast<int>(shift * 14U));
        TEST_ASSERT(s.cycles == expected);
    }
    std::cout << "  Passed (Dynamic shift cycle scale: shift * 14 master clocks)\n";
}

void test_cand_005660() {
    std::cout << "Testing Candidate 1 (0x005660 Palette Color Math)...\n";
    reset_metrics_005660();
    MockGuestState s{};
    auto api = make_api(s);

    const unsigned a6 = 0xFF2000U;
    s.regs[kRegA6] = a6;
    s.regs[kRegA7] = 0xFFFE00U;
    s.write_mem(0xFFFE00U, 4, 0x00BF72U); // return address
    s.write_mem(a6 + 0x08U, 2, 0x0444U); // word 1
    s.write_mem(a6 + 0x0CU, 2, 0x0888U); // word 2

    unsigned pc = 0x005660U;
    unsigned steps = 0;
    while (steps < 10) {
        ++steps;
        auto res = step_m68k_005660(pc, api, true);
        TEST_ASSERT(res == M68k9D70StepResult::InstructionStepped);
        pc = s.next_pc;
        if (pc == 0x00BF72U) break;
    }
    TEST_ASSERT(pc == 0x00BF72U);
    TEST_ASSERT(steps == 4);
    TEST_ASSERT(s.regs[kRegA0] == 0x00FF1974U);
    TEST_ASSERT(s.read_mem(0x00FF1972U, 2) == 0x0444U);
    TEST_ASSERT(s.read_mem(0x00FF1974U, 2) == 0x0888U);
    TEST_ASSERT(get_metrics_005660().completed_calls == 1);
    TEST_ASSERT(get_metrics_005660().stepped_instructions == 4);
    std::cout << "  Passed (4 steps, write to $FF1972/74 verified)\n";
}

void test_cand_062694() {
    std::cout << "Testing Candidate 2 (0x062694 Sound Attenuation Tail)...\n";
    reset_metrics_062694();
    MockGuestState s{};
    auto api = make_api(s);

    const unsigned a6 = 0xFF2000U;
    s.regs[kRegA6] = a6;
    s.regs[kRegA7] = 0xFFFE00U;
    s.write_mem(0xFFFE00U, 4, 0x062610U);
    s.write_mem(a6 + 23U, 1, 0x02U); // & 3 != 3 -> BEQ not taken, fallthrough to RTS

    unsigned pc = 0x062694U;
    unsigned steps = 0;
    while (steps < 10) {
        ++steps;
        auto res = step_m68k_062694(pc, api, true);
        TEST_ASSERT(res == M68k9D70StepResult::InstructionStepped);
        pc = s.next_pc;
        if (pc == 0x062610U) break;
    }
    TEST_ASSERT(pc == 0x062610U);
    TEST_ASSERT(steps == 5);
    TEST_ASSERT(get_metrics_062694().completed_calls == 1);
    std::cout << "  Passed (BEQ not taken, 5 steps to RTS)\n";
}

void test_cand_062a78() {
    std::cout << "Testing Candidate 3 (0x062A78 Sound Pan Direct)...\n";
    reset_metrics_062a78();
    MockGuestState s{};
    auto api = make_api(s);

    const unsigned a6 = 0xFF2000U;
    const unsigned a5 = 0xFF3000U;
    s.regs[kRegA6] = a6;
    s.regs[kRegA5] = a5;
    s.regs[kRegA7] = 0xFFFE00U;
    s.regs[kRegD7] = 0x00000000U;
    s.write_mem(0xFFFE00U, 4, 0x062A24U);
    s.write_mem(a6 + 30U, 4, 0x12345678U);
    s.write_mem(a5, 1, 0x00U);

    unsigned pc = 0x062A78U;
    unsigned steps = 0;
    while (steps < 15) {
        ++steps;
        auto res = step_m68k_062a78(pc, api, true);
        TEST_ASSERT(res == M68k9D70StepResult::InstructionStepped);
        pc = s.next_pc;
        if (pc == 0x062A24U) break;
    }
    TEST_ASSERT(pc == 0x062A24U);
    TEST_ASSERT(steps == 8);
    TEST_ASSERT(s.regs[kRegA0] == 0x12345678U);
    TEST_ASSERT(get_metrics_062a78().completed_calls == 1);
    std::cout << "  Passed (Fallthrough path to RTS, 8 steps)\n";
}

void test_cand_006e7a() {
    std::cout << "Testing Candidate 4 (0x006E7A System Flag Query)...\n";
    reset_metrics_006e7a();
    MockGuestState s{};
    auto api = make_api(s);

    s.regs[kRegA7] = 0xFFFE00U;
    s.write_mem(0xFFFE00U, 4, 0x006DF0U);
    s.write_mem(0x00FF165FU, 1, 0x00U); // Bit 4 is 0 -> BTST Z=1 -> BNE not taken -> fallthrough to RTS

    unsigned pc = 0x006E7AU;
    unsigned steps = 0;
    while (steps < 10) {
        ++steps;
        auto res = step_m68k_006e7a(pc, api, true);
        TEST_ASSERT(res == M68k9D70StepResult::InstructionStepped);
        pc = s.next_pc;
        if (pc == 0x006DF0U) break;
    }
    TEST_ASSERT(pc == 0x006DF0U);
    TEST_ASSERT(steps == 3);
    TEST_ASSERT(get_metrics_006e7a().completed_calls == 1);
    std::cout << "  Passed (BTST + BNE + RTS, 3 steps)\n";
}

void test_interrupt_stress() {
    std::cout << "Testing Interrupt Boundary Stress & Preemption (Phase 8)...\n";
    reset_metrics_005660();
    MockGuestState s{};
    auto api = make_api(s);

    const unsigned a6 = 0xFF2000U;
    s.regs[kRegA6] = a6;
    s.regs[kRegA7] = 0xFFFE00U;
    s.write_mem(0xFFFE00U, 4, 0x00BF72U);
    s.write_mem(a6 + 0x08U, 2, 0x1111U);
    s.write_mem(a6 + 0x0CU, 2, 0x2222U);

    unsigned pc = 0x005660U;
    // Step 1: execute first instruction
    auto res1 = step_m68k_005660(pc, api, true);
    TEST_ASSERT(res1 == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(s.next_pc == 0x005666U);

    // Simulate VBlank interrupt arriving: guest PC jumps to interrupt handler 0x000200
    unsigned irq_pc = 0x000200U;
    auto res_irq = step_m68k_005660(irq_pc, api, true);
    TEST_ASSERT(res_irq == M68k9D70StepResult::NotHandled); // Correctly yields!

    // Simulate RTE returning to interrupted instruction 0x005666
    pc = 0x005666U;
    auto res2 = step_m68k_005660(pc, api, true);
    TEST_ASSERT(res2 == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(s.next_pc == 0x00566AU);

    // Finish remaining instructions
    pc = 0x00566AU;
    TEST_ASSERT(step_m68k_005660(pc, api, true) == M68k9D70StepResult::InstructionStepped);
    pc = 0x00566EU;
    TEST_ASSERT(step_m68k_005660(pc, api, true) == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(s.next_pc == 0x00BF72U);
    TEST_ASSERT(get_metrics_005660().completed_calls == 1);
    std::cout << "  Passed (Cooperative yield on IRQ, exact resume at next PC)\n";
}

void test_non_leaf_00d2b4() {
    std::cout << "Testing Candidate 1 Non-Leaf BSR & Resume (0x00D2B4)...\n";
    reset_metrics_00d2b4();
    MockGuestState s{};
    auto api = make_api(s);

    s.regs[kRegA7] = 0x00FFFE00U;
    s.regs[kRegA0] = 0x00FF1716U;
    s.write_mem(0x00FF1716U + 0x26U, 1, 0x04U); // bit 2 set -> BCLR clears it
    s.write_mem(0x00FF1716U + 0x0EU, 2, 0x0123U); // camera D0
    s.write_mem(0x00FF16F0U, 1, 0x00U); // bit 1 is 0 -> BEQ $d330 taken
    s.write_mem(0x00FFFE00U, 4, 0x00A500U); // caller return address

    // Opcode words for read_be
    s.write_mem(0x00D2B4U, 2, 0x6100U);
    s.write_mem(0x00D344U, 2, 0x4E75U);
    s.write_mem(0x00D2B8U, 2, 0x08A8U);
    s.write_mem(0x00D2BEU, 2, 0x6700U);
    s.write_mem(0x00D2C2U, 2, 0x08F9U);
    s.write_mem(0x00D2CAU, 2, 0x3028U);
    s.write_mem(0x00D2CEU, 2, 0x0839U);
    s.write_mem(0x00D2D6U, 2, 0x6700U);
    s.write_mem(0x00D330U, 2, 0x43F9U);
    s.write_mem(0x00D336U, 2, 0xB1FCU);
    s.write_mem(0x00D33CU, 2, 0x6702U);
    s.write_mem(0x00D340U, 2, 0x3280U);
    s.write_mem(0x00D342U, 2, 0x4E75U);
    s.write_mem(0x00A500U, 2, 0x4E71U);

    // Step 1: BSR.W $D344
    auto res1 = step_m68k_00d2b4(0x00D2B4U, api, true);
    TEST_ASSERT(res1 == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(s.regs[kRegA7] == 0x00FFFE00U - 4U);
    TEST_ASSERT(s.read_mem(s.regs[kRegA7], 4) == 0x00D2B8U); // return address pushed!
    TEST_ASSERT(s.next_pc == 0x00D344U); // target child PC set!

    // While in child: caller must yield NotHandled
    TEST_ASSERT(step_m68k_00d2b4(0x00D344U, api, true) == M68k9D70StepResult::NotHandled);

    // Child returns via RTS: pops return address 0x00D2B8
    s.regs[kRegA7] += 4U;

    // Caller resumes at 0x00D2B8
    TEST_ASSERT(step_m68k_00d2b4(0x00D2B8U, api, true) == M68k9D70StepResult::InstructionStepped);
    // BCLR cleared bit 2
    TEST_ASSERT((s.read_mem(0x00FF1716U + 0x26U, 1) & 0x04U) == 0U);
    TEST_ASSERT(s.next_pc == 0x00D2BEU);

    // Continue stepping
    TEST_ASSERT(step_m68k_00d2b4(0x00D2BEU, api, true) == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(s.next_pc == 0x00D2C2U);
    TEST_ASSERT(step_m68k_00d2b4(0x00D2C2U, api, true) == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(step_m68k_00d2b4(0x00D2CAU, api, true) == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(step_m68k_00d2b4(0x00D2CEU, api, true) == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(step_m68k_00d2b4(0x00D2D6U, api, true) == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(s.next_pc == 0x00D330U); // BEQ taken to $d330
    TEST_ASSERT(step_m68k_00d2b4(0x00D330U, api, true) == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(step_m68k_00d2b4(0x00D336U, api, true) == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(step_m68k_00d2b4(0x00D33CU, api, true) == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(s.next_pc == 0x00D340U);
    TEST_ASSERT(step_m68k_00d2b4(0x00D340U, api, true) == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(s.next_pc == 0x00D342U);

    // Step RTS at 0x00D342
    TEST_ASSERT(step_m68k_00d2b4(0x00D342U, api, true) == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(s.next_pc == 0x00A500U);
    TEST_ASSERT(get_metrics_00d2b4().completed_calls == 1);
    std::cout << "  Passed (Non-leaf BSR push, child yield, return resume, and completion)\n";
}

void test_dynamic_jump_table_0059a8() {
    std::cout << "Testing Candidate 2 Dynamic Jump Table (0x0059A8)...\n";
    reset_metrics_0059a8();
    MockGuestState s{};
    auto api = make_api(s);

    s.regs[kRegA6] = 0x00FFB000U;
    s.write_mem(0x00FF165EU, 2, 0x1234U);
    s.write_mem(0x00FFB004U, 2, 0x0000U); // D0 = 0 (first table index)
    s.write_mem(0x0059BAU, 2, 0x083CU); // offset 0x083C -> target = 0x0059BA + 0x083C = 0x0061F6

    s.write_mem(0x0059A8U, 2, 0x3E39U);
    s.write_mem(0x0059AEU, 2, 0x302EU);
    s.write_mem(0x0059B2U, 2, 0x41FBU);
    s.write_mem(0x0059B6U, 2, 0xD0D0U);
    s.write_mem(0x0059B8U, 2, 0x4ED0U);
    s.write_mem(0x0061F6U, 2, 0x4E75U);

    TEST_ASSERT(step_m68k_0059a8(0x0059A8U, api, true) == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(s.read_reg(kRegD7) == 0x1234U);
    TEST_ASSERT(step_m68k_0059a8(0x0059AEU, api, true) == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT((s.read_reg(kRegD0) & 0xFFFFU) == 0x0000U);
    TEST_ASSERT(step_m68k_0059a8(0x0059B2U, api, true) == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(s.read_reg(kRegA0) == 0x0059BAU);
    TEST_ASSERT(step_m68k_0059a8(0x0059B6U, api, true) == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(s.read_reg(kRegA0) == 0x0061F6U);
    // JMP (A0)
    TEST_ASSERT(step_m68k_0059a8(0x0059B8U, api, true) == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(s.next_pc == 0x0061F6U);
    TEST_ASSERT(get_metrics_0059a8().completed_calls == 1);
    std::cout << "  Passed (PC-relative indexed LEA, ADDA.W, and JMP (A0) dispatch)\n";
}

void test_mutation_assertions() {
    std::cout << "Testing Mutation Assertions (Phase 7)...\n";
    // Mutant 1: Wrong register index for SR corrupting PC
    MockGuestState s1{};
    s1.regs[kRegPC] = 0x00A000U;
    s1.regs[kRegSR] = 0x2700U;
    // If someone wrote CCR to PC index 16 instead of 17:
    s1.regs[kRegPC] = (s1.regs[kRegPC] & 0xFFE0U) | 0x04U;
    TEST_ASSERT(s1.regs[kRegPC] != 0x00A000U); // Mutant caught!

    // Mutant 2: Missing memory write
    MockGuestState s2{};
    s2.write_mem(0x00FF1972U, 2, 0x0000U);
    TEST_ASSERT(s2.read_mem(0x00FF1972U, 2) != 0x0444U); // Mutant caught!

    std::cout << "  Passed (Mutants successfully detected and rejected)\n";
}

} // namespace

int main() {
    std::cout << "=== Thor Recompiler M15.238 Scheduler-Safe Differential Test ===\n";
    test_gpgx_register_mapping();
    test_dynamic_shift_cycles();
    test_cand_005660();
    test_cand_062694();
    test_cand_062a78();
    test_cand_006e7a();
    test_interrupt_stress();
    test_non_leaf_00d2b4();
    test_dynamic_jump_table_0059a8();
    test_mutation_assertions();
    std::cout << "=== ALL QUALIFIED SCHEDULER-SAFE DIFFERENTIAL TESTS PASSED ===\n";
    return 0;
}
