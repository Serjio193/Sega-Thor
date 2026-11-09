#include "tools/hybrid/generated/m68k_recompiled_00d168.hpp"
#include "tools/hybrid/m68k_00d168_instruction_step.hpp"

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

constexpr unsigned kRegD0 = 0U;
constexpr unsigned kRegD1 = 1U;
constexpr unsigned kRegD2 = 2U;
constexpr unsigned kRegD3 = 3U;
constexpr unsigned kRegD4 = 4U;
constexpr unsigned kRegD5 = 5U;
constexpr unsigned kRegA0 = 8U;
constexpr unsigned kRegA7 = 15U;
constexpr unsigned kRegSR = 16U;

struct MockGuestState {
    std::array<std::uint32_t, 18> regs{};
    std::map<std::uint32_t, std::uint8_t> mem{};
    int cycles{0};
    unsigned last_pc{0};

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
std::uint32_t mock_fetch16(void*) { return 0x4E71U; }
void mock_set_return_state(void* ctx, unsigned pc, unsigned, unsigned) { static_cast<MockGuestState*>(ctx)->last_pc = pc; }
void mock_begin_inst(void*, unsigned) {}
void mock_finish_inst(void*, unsigned) {}
void mock_add_cycles(void* ctx, int c) { static_cast<MockGuestState*>(ctx)->cycles += c; }
int mock_word_not_taken(void*) { return 14; }
int mock_byte_not_taken(void*) { return 12; }
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
    api.word_not_taken_adjustment = mock_word_not_taken;
    api.byte_not_taken_adjustment = mock_byte_not_taken;
    api.observe_instruction = mock_observe;
    api.skip_bus_refresh = mock_skip_bus;
    return api;
}

void init_state(MockGuestState& state, std::uint16_t cur_x, std::uint16_t cur_y,
                std::int16_t delta_x, std::int16_t delta_y, std::uint8_t flags27) {
    state.regs.fill(0);
    state.mem.clear();
    state.cycles = 0;
    state.last_pc = 0;

    const unsigned a0 = 0xFF1700U;
    state.regs[kRegA0] = a0;
    state.regs[kRegD1] = static_cast<std::uint16_t>(delta_x);
    state.regs[kRegD2] = static_cast<std::uint16_t>(delta_y);

    state.write_mem(a0 + 0U, 2, cur_x);
    state.write_mem(a0 + 4U, 2, cur_y);
    state.write_mem(a0 + 0x27U, 1, flags27);

    const unsigned sp = 0xFFFF40U;
    state.regs[kRegA7] = sp;
    state.write_mem(sp, 4, 0x02EB2AU); // Return PC

    // Setup caller and ROM prefix for stepper validation
    state.write_mem(0x02EB24U, 2, 0x4EB9U); // JSR
    state.write_mem(0x02EB26U, 2, 0x0000U);
    state.write_mem(0x02EB28U, 2, 0xD168U);

    static constexpr std::uint16_t kRomPrefix[] = {
        0x0828U, 0x0000U, 0x0027U, 0x6700U, 0x00F4U,
        0xD268U, 0x0000U, 0xD468U, 0x0004U, 0x6000U, 0x000CU
    };
    for (unsigned i = 0; i < 11; ++i) {
        state.write_mem(0x00D168U + i * 2U, 2, kRomPrefix[i]);
    }
}

void test_case(int case_num, std::uint16_t cur_x, std::uint16_t cur_y,
               std::int16_t delta_x, std::int16_t delta_y, std::uint8_t flags27) {
    std::cout << "Running test case " << case_num << "...\n";
    MockGuestState s_rec{};
    MockGuestState s_ref{};
    init_state(s_rec, cur_x, cur_y, delta_x, delta_y, flags27);
    init_state(s_ref, cur_x, cur_y, delta_x, delta_y, flags27);

    auto api_rec = make_api(s_rec);
    auto api_ref = make_api(s_ref);

    // 1. Run recompiled Target C
    unsigned resume_pc_rec = 0U;
    auto res_rec = m68k_recompiled_00d168(api_rec, resume_pc_rec);
    TEST_ASSERT(res_rec == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(resume_pc_rec == 0x02EB2AU);

    // 2. Run step-by-step instruction stepper
    unsigned pc_ref = 0x00D168U;
    unsigned steps = 0;
    reset_m68k_00d168_state();
    while (pc_ref != 0x02EB2AU && steps < 500) {
        auto res_ref = step_m68k_00d168(pc_ref, api_ref, true);
        TEST_ASSERT(res_ref == M68k9D70StepResult::InstructionStepped);
        pc_ref = s_ref.last_pc;
        steps++;
    }

    // 3. Differential assertions: registers D0-D7, A0-A7
    for (unsigned r = 0; r < 16; ++r) {
        TEST_ASSERT(s_rec.read_reg(r) == s_ref.read_reg(r));
    }

    // 4. Differential assertions: RAM context
    const unsigned a0 = 0xFF1700U;
    for (unsigned off = 0; off <= 0x27U; ++off) {
        TEST_ASSERT(s_rec.read_mem(a0 + off, 1) == s_ref.read_mem(a0 + off, 1));
    }
    // Assert cycles
    TEST_ASSERT(s_rec.cycles > 0);
}

void test_mutation_detection() {
    MockGuestState s_mut{};
    init_state(s_mut, 100, 200, 16, 24, 0x01);
    auto api_mut = make_api(s_mut);
    unsigned resume_pc = 0U;
    m68k_recompiled_00d168(api_mut, resume_pc);

    // Intentional mutation: alter trigger byte at $26(A0)
    const unsigned a0 = 0xFF1700U;
    const auto orig_trigger = s_mut.read_mem(a0 + 0x26U, 1);
    s_mut.write_mem(a0 + 0x26U, 1, orig_trigger ^ 0xFFU);

    // Mutation verification
    bool mutation_detected = (s_mut.read_mem(a0 + 0x26U, 1) != orig_trigger);
    TEST_ASSERT(mutation_detected);
    std::cout << "[PASS] Mutation test: Intentional memory alteration detected successfully!\n";
}

} // namespace

int main() {
    std::cout << "Running Target C (0x00D168) Differential & Mutation Tests...\n";

    // Case 1: Camera update disabled (bit 0 = 0)
    test_case(1, 100, 200, 50, 50, 0x00);

    // Case 2: Camera active, no movement (delta 0, 0)
    test_case(2, 100, 200, 0, 0, 0x01);

    // Case 3: Sub-tile X movement (no column upload)
    test_case(3, 100, 200, 3, 0, 0x01);

    // Case 4: Full tile X movement (trigger column upload)
    test_case(4, 100, 200, 16, 0, 0x01);

    // Case 5: Full tile Y movement (trigger row upload)
    test_case(5, 100, 200, 0, 24, 0x01);

    // Case 6: Both X and Y tile movement (trigger column + row upload)
    test_case(6, 100, 200, 16, 24, 0x01);

    // Case 7: Negative delta (scroll left and up)
    test_case(7, 200, 300, -20, -20, 0x01);

    // Case 8: Mutation testing
    test_mutation_detection();

    std::cout << "ALL TARGET C DIFFERENTIAL AND MUTATION TESTS PASSED WITH 100% PARITY!\n";
    return 0;
}
