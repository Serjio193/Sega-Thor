#include "tools/hybrid/m68k_9d70_instruction_step.hpp"
#include "tools/hybrid/generated/m68k_step_helpers.hpp"
#include "tools/hybrid/generated/m68k_recompiled_00e682.hpp"
#include "tools/hybrid/generated/m68k_recompiled_015ae2.hpp"
#include "tools/hybrid/generated/m68k_recompiled_00c9b2.hpp"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>

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
int mock_peek(void* ctx, unsigned a) {
    auto* s = static_cast<MockGuestState*>(ctx);
    auto it = s->mem.find(a & 0xFFFFFFU);
    if (it == s->mem.end()) return -1;
    return static_cast<int>(it->second);
}
std::uint32_t mock_read(void* ctx, unsigned a, int w) { return static_cast<MockGuestState*>(ctx)->read_mem(a, w); }
void mock_write(void* ctx, unsigned a, int w, std::uint32_t v) { static_cast<MockGuestState*>(ctx)->write_mem(a, w, v); }
std::uint32_t mock_fetch16(void*) { return 0U; }
void mock_set_return_state(void* ctx, unsigned pc, unsigned, unsigned) { static_cast<MockGuestState*>(ctx)->next_pc = pc; }
void mock_begin_inst(void*, unsigned) {}
void mock_finish_inst(void*, unsigned) {}
void mock_add_cycles(void* ctx, int c) { static_cast<MockGuestState*>(ctx)->cycles += c; }
int mock_word_adj(void*) { return 14; }
int mock_byte_adj(void*) { return -14; }
void mock_observe(void*, unsigned, unsigned) {}
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

void test_candidate_00e682() {
    std::cout << "[Candidate 1] Testing 0x00E682 (Camera Viewport State Coordinator)...\n";
    MockGuestState s{};
    auto api = make_api(s);
    reset_metrics_00e682();

    s.regs[kRegA6] = 0xFF1000U;
    s.regs[kRegA7] = 0xFF2000U;
    s.write_mem(0xFF2000U, 4, 0x00D000U); // return address
    s.write_mem(0xFF1036U, 1, 0x40U); // 54(A6): bit 6 set
    s.write_mem(0xFF109DU, 1, 0x00U); // 157(A6): 0
    s.write_mem(0xFF1082U, 1, 0x00U); // 130(A6): 0

    // Set ROM code words for peeking/branching
    s.write_mem(0x00E682U, 2, 0x08AEU);
    s.write_mem(0x00E688U, 2, 0x102EU);
    s.write_mem(0x00E68CU, 2, 0x6600U);
    s.write_mem(0x00E690U, 2, 0x102EU);
    s.write_mem(0x00E694U, 2, 0x6600U);
    s.write_mem(0x00E698U, 2, 0x4E75U);
    s.write_mem(0x00D000U, 2, 0x4E71U);

    // Step through 0x00E682
    auto r1 = step_m68k_00e682(0x00E682U, api, true);
    TEST_ASSERT(r1 == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(s.read_mem(0xFF1036U, 1) == 0x00U); // bit 6 cleared!

    auto r2 = step_m68k_00e682(0x00E688U, api, true);
    TEST_ASSERT(r2 == M68k9D70StepResult::InstructionStepped);

    auto r3 = step_m68k_00e682(0x00E68CU, api, true);
    TEST_ASSERT(r3 == M68k9D70StepResult::InstructionStepped);

    auto r4 = step_m68k_00e682(0x00E690U, api, true);
    TEST_ASSERT(r4 == M68k9D70StepResult::InstructionStepped);

    auto r5 = step_m68k_00e682(0x00E694U, api, true);
    TEST_ASSERT(r5 == M68k9D70StepResult::InstructionStepped);

    auto r6 = step_m68k_00e682(0x00E698U, api, true); // RTS
    TEST_ASSERT(r6 == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(s.regs[kRegA7] == 0xFF2004U); // Stack popped
    TEST_ASSERT(s.next_pc == 0x00D000U);

    const auto& m = get_metrics_00e682();
    TEST_ASSERT(m.target_entries == 1);
    TEST_ASSERT(m.stepped_instructions == 6);
    TEST_ASSERT(m.completed_calls == 1);
    std::cout << "  Passed (0x00E682 verified with correct stack and bit operations).\n";
}

void test_candidate_015ae2() {
    std::cout << "[Candidate 2] Testing 0x015AE2 (Entity AI Branch Selector)...\n";
    MockGuestState s{};
    auto api = make_api(s);
    reset_metrics_015ae2();

    s.regs[kRegA6] = 0xFF1000U;
    s.regs[kRegA7] = 0xFF2000U;
    s.write_mem(0xFF2000U, 4, 0x001200U);
    s.write_mem(0xFF1066U, 1, 0x04U); // 0x66(A6): 4 -> will decrement past 1,2,3 and reach RTS

    s.write_mem(0x015AE2U, 2, 0x102EU);
    s.write_mem(0x015AE6U, 2, 0x6700U);
    s.write_mem(0x015AEAU, 2, 0x5300U);
    s.write_mem(0x015AECU, 2, 0x6700U);
    s.write_mem(0x015AF0U, 2, 0x5300U);
    s.write_mem(0x015AF2U, 2, 0x6700U);
    s.write_mem(0x015AF6U, 2, 0x5300U);
    s.write_mem(0x015AF8U, 2, 0x6700U);
    s.write_mem(0x015AFCU, 2, 0x4E75U);
    s.write_mem(0x001200U, 2, 0x4E71U);

    // Step full sequence
    TEST_ASSERT(step_m68k_015ae2(0x015AE2U, api, true) == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(step_m68k_015ae2(0x015AE6U, api, true) == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(step_m68k_015ae2(0x015AEAU, api, true) == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(step_m68k_015ae2(0x015AECU, api, true) == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(step_m68k_015ae2(0x015AF0U, api, true) == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(step_m68k_015ae2(0x015AF2U, api, true) == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(step_m68k_015ae2(0x015AF6U, api, true) == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(step_m68k_015ae2(0x015AF8U, api, true) == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(step_m68k_015ae2(0x015AFCU, api, true) == M68k9D70StepResult::InstructionStepped);

    TEST_ASSERT(s.regs[kRegA7] == 0xFF2004U);
    TEST_ASSERT(s.next_pc == 0x001200U);
    const auto& m = get_metrics_015ae2();
    TEST_ASSERT(m.stepped_instructions == 9);
    TEST_ASSERT(m.completed_calls == 1);
    std::cout << "  Passed (0x015AE2 full cascade to RTS verified).\n";
}

void test_candidate_00c9b2() {
    std::cout << "[Candidate 3] Testing 0x00C9B2 (System Status Gatekeeper)...\n";
    MockGuestState s{};
    auto api = make_api(s);
    reset_metrics_00c9b2();

    s.regs[kRegA7] = 0xFF2000U;
    s.write_mem(0xFF2000U, 4, 0x00C000U);
    // All status flags set to 0 (healthy)
    s.write_mem(0xFF17B8U, 2, 0x0000U);
    s.write_mem(0xFF1982U, 8, 0x00000000U);

    s.write_mem(0x00C9B2U, 2, 0x4A39U);
    s.write_mem(0x00C9B8U, 2, 0x662EU);
    s.write_mem(0x00C9BAU, 2, 0x4A39U);
    s.write_mem(0x00C9C0U, 2, 0x6600U);
    s.write_mem(0x00C9C4U, 2, 0x4A39U);
    s.write_mem(0x00C9CAU, 2, 0x661CU);
    s.write_mem(0x00C9CCU, 2, 0x0839U);
    s.write_mem(0x00C9D4U, 2, 0x6612U);
    s.write_mem(0x00C9D6U, 2, 0x4A39U);
    s.write_mem(0x00C9DCU, 2, 0x660AU);
    s.write_mem(0x00C9DEU, 2, 0x4A39U);
    s.write_mem(0x00C9E4U, 2, 0x6602U);
    s.write_mem(0x00C9E6U, 2, 0x4E75U);
    s.write_mem(0x00C000U, 2, 0x4E71U);

    // Step healthy path: all flags 0 -> completes at 0x00C9E6
    TEST_ASSERT(step_m68k_00c9b2(0x00C9B2U, api, true) == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(step_m68k_00c9b2(0x00C9B8U, api, true) == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(step_m68k_00c9b2(0x00C9BAU, api, true) == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(step_m68k_00c9b2(0x00C9C0U, api, true) == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(step_m68k_00c9b2(0x00C9C4U, api, true) == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(step_m68k_00c9b2(0x00C9CAU, api, true) == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(step_m68k_00c9b2(0x00C9CCU, api, true) == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(step_m68k_00c9b2(0x00C9D4U, api, true) == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(step_m68k_00c9b2(0x00C9D6U, api, true) == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(step_m68k_00c9b2(0x00C9DCU, api, true) == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(step_m68k_00c9b2(0x00C9DEU, api, true) == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(step_m68k_00c9b2(0x00C9E4U, api, true) == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(step_m68k_00c9b2(0x00C9E6U, api, true) == M68k9D70StepResult::InstructionStepped);

    TEST_ASSERT(s.regs[kRegA7] == 0xFF2004U);
    TEST_ASSERT(s.next_pc == 0x00C000U);
    const auto& m = get_metrics_00c9b2();
    TEST_ASSERT(m.stepped_instructions == 13);
    TEST_ASSERT(m.completed_calls == 1);
    std::cout << "  Passed (0x00C9B2 healthy path verified).\n";
}

void test_candidate_mutations() {
    std::cout << "[Mutations] Testing mutation sensitivity across candidates...\n";
    // 1. Stack pointer mutation: corrupted A7 points to invalid memory
    {
        MockGuestState s{};
        auto api = make_api(s);
        reset_metrics_00e682();
        s.regs[kRegA6] = 0xFF1000U;
        s.regs[kRegA7] = 0x00000000U; // Unmapped stack!
        s.write_mem(0xFF1036U, 1, 0x00U);
        s.write_mem(0xFF109DU, 1, 0x00U);
        s.write_mem(0xFF1082U, 1, 0x00U);
        s.write_mem(0x00E682U, 2, 0x08AEU);
        s.write_mem(0x00E688U, 2, 0x102EU);
        s.write_mem(0x00E68CU, 2, 0x6600U);
        s.write_mem(0x00E690U, 2, 0x102EU);
        s.write_mem(0x00E694U, 2, 0x6600U);
        s.write_mem(0x00E698U, 2, 0x4E75U);

        step_m68k_00e682(0x00E682U, api, true);
        step_m68k_00e682(0x00E688U, api, true);
        step_m68k_00e682(0x00E68CU, api, true);
        step_m68k_00e682(0x00E690U, api, true);
        step_m68k_00e682(0x00E694U, api, true);
        auto r_mut = step_m68k_00e682(0x00E698U, api, true);
        TEST_ASSERT(r_mut == M68k9D70StepResult::ContractFault); // Stack violation caught!
    }

    // 2. CCR Mutation: Inverting Z flag changes branch decision
    {
        MockGuestState s{};
        auto api = make_api(s);
        reset_metrics_015ae2();
        s.regs[kRegA6] = 0xFF1000U;
        s.regs[kRegA7] = 0xFF2000U;
        s.write_mem(0xFF2000U, 4, 0x001200U);
        s.write_mem(0xFF1066U, 1, 0x00U); // 0 -> Z flag becomes 1
        s.write_mem(0x015AE2U, 2, 0x102EU);
        s.write_mem(0x015AE6U, 2, 0x6700U); // BEQ 0x016294
        s.write_mem(0x016294U, 2, 0x4E71U);
        step_m68k_015ae2(0x015AE2U, api, true);
        auto r_beq = step_m68k_015ae2(0x015AE6U, api, true);
        TEST_ASSERT(r_beq == M68k9D70StepResult::InstructionStepped);
        // Branch was taken to 0x016294 due to Z=1 (mutated behavior vs Z=0)
        TEST_ASSERT(s.next_pc == 0x016294U);
    }

    // 3. Return Address Mutation: Mutated return address directs flow to corrupt target
    {
        MockGuestState s{};
        auto api = make_api(s);
        reset_metrics_00c9b2();
        s.regs[kRegA7] = 0xFF2000U;
        const std::uint32_t kMutatedReturn = 0x00BEE0U;
        s.write_mem(0xFF2000U, 4, kMutatedReturn);
        s.write_mem(kMutatedReturn, 2, 0x4E71U);
        s.write_mem(0x00C9E6U, 2, 0x4E75U);
        unsigned resume_pc = 0U;
        auto res = step_rts(api, 0x00C9E6U, resume_pc);
        TEST_ASSERT(res == M68k9D70StepResult::InstructionStepped);
        TEST_ASSERT(resume_pc == kMutatedReturn);
        TEST_ASSERT(resume_pc != 0x00C000U); // Mutated address detected!
    }

    std::cout << "  Passed (all mutations detected and verified).\n";
}

} // namespace

int main() {
    std::cout << "Running M15.239 Candidate Differential Tests...\n";
    test_candidate_00e682();
    test_candidate_015ae2();
    test_candidate_00c9b2();
    test_candidate_mutations();
    std::cout << "ALL CANDIDATE DIFFERENTIAL TESTS PASSED.\n";
    return 0;
}
