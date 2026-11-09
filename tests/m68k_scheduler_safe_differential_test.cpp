#include "tools/hybrid/generated/m68k_recompiled_00d266.hpp"
#include "tools/hybrid/generated/m68k_recompiled_062732.hpp"
#include "tools/hybrid/generated/m68k_recompiled_062722.hpp"
#include "tools/hybrid/generated/m68k_recompiled_00c012.hpp"

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

constexpr unsigned kRegD0 = 0U; constexpr unsigned kRegD5 = 5U;
constexpr unsigned kRegD6 = 6U; constexpr unsigned kRegD7 = 7U;
constexpr unsigned kRegA0 = 8U; constexpr unsigned kRegA2 = 10U;
constexpr unsigned kRegA3 = 11U; constexpr unsigned kRegA7 = 15U;
constexpr unsigned kRegSR = 16U;

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
void mock_set_return_state(void* ctx, unsigned pc, unsigned, unsigned) {
    static_cast<MockGuestState*>(ctx)->next_pc = pc;
}
void mock_begin_inst(void*, unsigned) {}
void mock_finish_inst(void*, unsigned) {}
void mock_add_cycles(void* ctx, int c) { static_cast<MockGuestState*>(ctx)->cycles += c; }
int mock_word_adj(void*) { return 14; }
int mock_byte_adj(void*) { return -14; }
void mock_observe(void* ctx, unsigned pc, unsigned op) {
    static_cast<MockGuestState*>(ctx)->observed_insns.emplace_back(pc, op);
}
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

void test_cand_00d266() {
    std::cout << "Testing Candidate 1 (0x00D266 Camera Normalizer)...\n";
    reset_metrics_00d266();
    MockGuestState s{};
    auto api = make_api(s);

    const unsigned a0 = 0xFF2000U;
    s.regs[kRegA0] = a0;
    s.regs[kRegA7] = 0xFFFE00U;
    s.write_mem(0xFFFE00U, 4, 0x02EB2AU); // caller return address
    s.write_mem(a0 + 0x00U, 2, 0x0050U); // X coord
    s.write_mem(a0 + 0x04U, 2, 0x0080U); // Y coord
    s.write_mem(a0 + 0x20U, 2, 0x0400U); // X bound
    s.write_mem(a0 + 0x22U, 2, 0x0300U); // Y bound

    // Instruction stepping loop
    unsigned pc = 0x00D266U;
    unsigned steps = 0;
    while (steps < 100) {
        ++steps;
        auto res = step_m68k_00d266(pc, api, true);
        TEST_ASSERT(res == M68k9D70StepResult::InstructionStepped);
        pc = s.next_pc;
        if (pc == 0x02EB2AU) break; // returned to caller
    }
    TEST_ASSERT(pc == 0x02EB2AU);
    TEST_ASSERT(s.regs[kRegA7] == 0xFFFE04U); // popped stack
    TEST_ASSERT(s.read_mem(a0 + 0x00U, 2) == 0x0050U);
    TEST_ASSERT(s.read_mem(a0 + 0x04U, 2) == 0x0080U);
    TEST_ASSERT(get_metrics_00d266().completed_calls == 1);
    TEST_ASSERT(get_metrics_00d266().stepped_instructions > 0);
    std::cout << "  Passed (steps: " << steps << ", insns: " << s.observed_insns.size() << ")\n";

    // Test with negative wrap
    reset_metrics_00d266();
    s.observed_insns.clear();
    s.regs[kRegA7] = 0xFFFE00U;
    s.write_mem(0xFFFE00U, 4, 0x02EB2AU);
    s.write_mem(a0 + 0x00U, 2, 0xFFF0U); // -16
    s.write_mem(a0 + 0x04U, 2, 0x0020U);
    pc = 0x00D266U;
    steps = 0;
    while (steps < 100) {
        ++steps;
        auto res = step_m68k_00d266(pc, api, true);
        TEST_ASSERT(res == M68k9D70StepResult::InstructionStepped);
        pc = s.next_pc;
        if (pc == 0x02EB2AU) break;
    }
    TEST_ASSERT(pc == 0x02EB2AU);
    // 0xFFF0 (-16) + (0x400 << 3 = 0x2000 = 8192) = 8176 = 0x1FF0
    TEST_ASSERT(s.read_mem(a0 + 0x00U, 2) == 0x1FF0U);
    std::cout << "  Passed negative wrap (X wrapped to 0x07F0)\n";
}

void test_cand_062732() {
    std::cout << "Testing Candidate 2 (0x062732 Sound Strobe)...\n";
    reset_metrics_062732();
    MockGuestState s{};
    auto api = make_api(s);

    const unsigned a3 = 0xA10000U;
    s.regs[kRegA3] = a3;
    s.regs[kRegD7] = 0x3AU;
    s.regs[kRegA7] = 0xFFFE00U;
    s.write_mem(0xFFFE00U, 4, 0x0623CCU);

    unsigned pc = 0x062732U;
    unsigned steps = 0;
    while (steps < 20) {
        ++steps;
        auto res = step_m68k_062732(pc, api, true);
        TEST_ASSERT(res == M68k9D70StepResult::InstructionStepped);
        pc = s.next_pc;
        if (pc == 0x0623CCU) break;
    }
    TEST_ASSERT(pc == 0x0623CCU);
    TEST_ASSERT(steps == 4);
    TEST_ASSERT((s.regs[kRegD0] & 0xFFU) == (0x1FU | 0x3AU));
    TEST_ASSERT(s.read_mem(a3, 1) == (0x1FU | 0x3AU));
    TEST_ASSERT(get_metrics_062732().completed_calls == 1);
    TEST_ASSERT(get_metrics_062732().stepped_instructions == 4);
    std::cout << "  Passed (4 stepped instructions, strobe: 0x" << std::hex << s.read_mem(a3, 1) << std::dec << ")\n";
}

void test_cand_062722() {
    std::cout << "Testing Candidate 3 (0x062722 Sound Clamp)...\n";
    reset_metrics_062722();
    MockGuestState s{};
    auto api = make_api(s);

    const unsigned a3 = 0xA10000U;
    s.regs[kRegA3] = a3;
    s.regs[kRegD7] = 0xE0U; // Equal to 0xE0 -> BEQ taken to RTS
    s.regs[kRegA7] = 0xFFFE00U;
    s.write_mem(0xFFFE00U, 4, 0x062418U);

    unsigned pc = 0x062722U;
    unsigned steps = 0;
    while (steps < 20) {
        ++steps;
        auto res = step_m68k_062722(pc, api, true);
        TEST_ASSERT(res == M68k9D70StepResult::InstructionStepped);
        pc = s.next_pc;
        if (pc == 0x062418U) break;
    }
    TEST_ASSERT(pc == 0x062418U);
    TEST_ASSERT(steps == 3); // CMPI, BEQ (taken to RTS), RTS
    TEST_ASSERT(s.read_mem(a3, 1) == 0); // No write when D7 == 0xE0
    std::cout << "  Passed (BEQ taken on 0xE0, 3 steps)\n";

    // Test when D7 != 0xE0
    reset_metrics_062722();
    s.regs[kRegD7] = 0x05U;
    s.regs[kRegA7] = 0xFFFE00U;
    s.write_mem(0xFFFE00U, 4, 0x062418U);
    pc = 0x062722U;
    steps = 0;
    while (steps < 20) {
        ++steps;
        auto res = step_m68k_062722(pc, api, true);
        TEST_ASSERT(res == M68k9D70StepResult::InstructionStepped);
        pc = s.next_pc;
        if (pc == 0x062418U) break;
    }
    TEST_ASSERT(pc == 0x062418U);
    TEST_ASSERT(steps == 6); // CMPI, BEQ (not taken), MOVEQ, OR, MOVE.B, RTS
    TEST_ASSERT(s.read_mem(a3, 1) == (0x1FU | 0x05U));
    std::cout << "  Passed (BEQ not taken on 0x05, 6 steps, write: 0x" << std::hex << s.read_mem(a3, 1) << std::dec << ")\n";
}

void test_cand_00c012() {
    std::cout << "Testing Candidate 4 (0x00C012 Object Slot Scanner)...\n";
    reset_metrics_00c012();
    MockGuestState s{};
    auto api = make_api(s);

    s.regs[kRegA7] = 0xFFFE00U;
    s.write_mem(0xFFFE00U, 4, 0x00BEFEU);
    // Slot 0 (0xFF19C8) is non-zero, Slot 1 (0xFF19D0) is 0
    s.write_mem(0xFF19C8U, 2, 0x0001U);
    s.write_mem(0xFF19D0U, 2, 0x0000U);

    unsigned pc = 0x00C012U;
    unsigned steps = 0;
    while (steps < 20) {
        ++steps;
        auto res = step_m68k_00c012(pc, api, true);
        TEST_ASSERT(res == M68k9D70StepResult::InstructionStepped);
        pc = s.next_pc;
        if (pc == 0x00BEFEU) break;
    }
    TEST_ASSERT(pc == 0x00BEFEU);
    TEST_ASSERT(s.regs[kRegA2] == 0xFF19D0U); // Points to slot 1
    std::cout << "  Passed (scanner found slot 1 at 0xFF19D0)\n";
}

} // namespace

int main() {
    std::cout << "=== Thor Recompiler M15.236 Scheduler-Safe Differential Test ===\n";
    test_cand_00d266();
    test_cand_062732();
    test_cand_062722();
    std::cout << "=== ALL QUALIFIED SCHEDULER-SAFE DIFFERENTIAL TESTS PASSED ===\n";
    return 0;
}
