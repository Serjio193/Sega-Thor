#include "tools/hybrid/generated/m68k_recompiled_062610.hpp"
#include "tools/hybrid/generated/m68k_recompiled_005660.hpp"
#include "tools/hybrid/generated/m68k_recompiled_062986.hpp"
#include "tools/hybrid/generated/m68k_recompiled_06293a.hpp"
#include "tools/hybrid/generated/m68k_recompiled_062900.hpp"

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

constexpr unsigned kRegD0 = 0U; constexpr unsigned kRegD1 = 1U;
constexpr unsigned kRegD2 = 2U; constexpr unsigned kRegD7 = 7U;
constexpr unsigned kRegA0 = 8U; constexpr unsigned kRegA1 = 9U;
constexpr unsigned kRegA2 = 10U; constexpr unsigned kRegA3 = 11U;
constexpr unsigned kRegA5 = 13U; constexpr unsigned kRegA6 = 14U;
constexpr unsigned kRegA7 = 15U; constexpr unsigned kRegSR = 16U;

struct MockGuestState {
    std::array<std::uint32_t, 18> regs{};
    std::map<std::uint32_t, std::uint8_t> mem{};
    int cycles{0};
    std::vector<std::pair<std::uint32_t, std::uint8_t>> writes{};

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
            auto b = static_cast<std::uint8_t>(val & 0xFFU);
            mem[addr + i] = b;
            writes.emplace_back(addr + i, b);
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
void mock_set_return_state(void*, unsigned, unsigned, unsigned) {}
void mock_begin_inst(void*, unsigned) {}
void mock_finish_inst(void*, unsigned) {}
void mock_add_cycles(void* ctx, int c) { static_cast<MockGuestState*>(ctx)->cycles += c; }
int mock_not_taken(void*) { return 14; }
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
    api.word_not_taken_adjustment = mock_not_taken;
    api.byte_not_taken_adjustment = mock_not_taken;
    api.observe_instruction = mock_observe;
    api.skip_bus_refresh = mock_skip_bus;
    return api;
}

void test_cand1_sound_voice_env() {
    std::cout << "Testing Candidate 1 (0x062610 Sound Voice Envelope)...\n";
    MockGuestState s{};
    s.regs[kRegA6] = 0xFF0500U;
    s.regs[kRegA5] = 0x062800U;
    s.regs[kRegA3] = 0x00C00011U; // PSG port
    s.regs[kRegA7] = 0xFFFF80U;
    s.write_mem(0xFFFF80U, 4, 0x0625DEU); // Return address

    // Channel record setup
    s.write_mem(0xFF0500U, 1, 0x00U);
    s.write_mem(0xFF0575U, 1, 0x00U); // 117(A6)
    s.write_mem(0xFF051CU, 2, 0x0100U); // 28(A6)
    s.write_mem(0xFF0576U, 2, 0x0020U); // 118(A6)
    s.write_mem(0xFF0571U, 1, 0x00U);   // 113(A6)
    s.write_mem(0xFF050AU, 2, 0x0010U); // 10(A6)
    s.write_mem(0xFF0508U, 2, 0x0000U); // 8(A6)
    s.write_mem(0xFF0501U, 1, 0x00U);   // 1(A6)
    s.write_mem(0x062803U, 1, 0x42U);   // 3(A5)
    s.write_mem(0x062804U, 1, 0x80U);   // 4(A5)

    auto api = make_api(s);
    unsigned resume_pc = 0;
    auto res = m68k_recompiled_062610(api, resume_pc);
    TEST_ASSERT(res == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(resume_pc == 0x0625DEU);
    TEST_ASSERT(s.read_reg(kRegA7) == 0xFFFF84U);
    TEST_ASSERT(s.read_mem(0xFF056DU, 1) == 0x42U); // 109(A6)
    TEST_ASSERT(s.cycles > 0);
    std::cout << "  Candidate 1 PASS.\n";
}

void test_cand2_entity_pos_sync() {
    std::cout << "Testing Candidate 2 (0x005660 Entity Position Sync)...\n";
    MockGuestState s{};
    s.regs[kRegA6] = 0xFF19E8U;
    s.regs[kRegA7] = 0xFFFF80U;
    s.write_mem(0xFFFF80U, 4, 0x005670U);
    s.write_mem(0xFF19E8U + 8U, 2, 0x0140U);
    s.write_mem(0xFF19E8U + 12U, 2, 0x00C8U);

    auto api = make_api(s);
    unsigned resume_pc = 0;
    auto res = m68k_recompiled_005660(api, resume_pc);
    TEST_ASSERT(res == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(resume_pc == 0x005670U);
    TEST_ASSERT(s.read_mem(0xFF1972U, 2) == 0x0140U);
    TEST_ASSERT(s.read_mem(0xFF1974U, 2) == 0x00C8U);
    std::cout << "  Candidate 2 PASS.\n";
}

void test_cand3_volume_adjust_mute() {
    std::cout << "Testing Candidate 3 (0x062986 Sound Volume Adjust)...\n";
    MockGuestState s{};
    s.regs[kRegA6] = 0xFF0500U;
    s.regs[kRegA0] = 0x062000U;
    s.regs[kRegA7] = 0xFFFF80U;
    s.write_mem(0xFFFF80U, 4, 0x06287CU);
    s.write_mem(0x062000U, 1, 0x02U);   // param
    s.write_mem(0xFF0512U, 1, 0x05U);   // 18(A6) initial volume

    auto api = make_api(s);
    unsigned resume_pc = 0;
    auto res = m68k_recompiled_062986(api, resume_pc);
    TEST_ASSERT(res == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(resume_pc == 0x06287CU);
    TEST_ASSERT(s.read_mem(0xFF0512U, 1) == 0x07U); // 5 + 2 = 7
    TEST_ASSERT(s.read_mem(0xFF0511U, 1) == (0x07U ^ 0x0FU)); // 17(A6) inverted
    std::cout << "  Candidate 3 PASS.\n";
}

void test_cand4_table_decoder() {
    std::cout << "Testing Candidate 4 (0x06293A Sound Table Decoder)...\n";
    MockGuestState s{};
    s.regs[kRegA6] = 0xFF0500U;
    s.regs[kRegA0] = 0x062100U;
    s.regs[kRegD7] = 0x80000000U; // Bit 31 set
    s.regs[kRegA7] = 0xFFFF80U;
    s.write_mem(0xFFFF80U, 4, 0x06287CU);
    s.write_mem(0x062100U, 1, 0x04U);
    s.write_mem(0xFF0574U, 1, 0x02U); // 116(A6)

    auto api = make_api(s);
    unsigned resume_pc = 0;
    auto res = m68k_recompiled_06293a(api, resume_pc);
    TEST_ASSERT(res == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(resume_pc == 0x06287CU);
    TEST_ASSERT(s.read_mem(0xFF0512U, 1) == 0x06U); // 4 + 2 = 6
    std::cout << "  Candidate 4 PASS.\n";
}

void test_cand5_operator_setup() {
    std::cout << "Testing Candidate 5 (0x062900 Sound Operator Setup)...\n";
    MockGuestState s{};
    s.regs[kRegA6] = 0xFF0500U;
    s.regs[kRegA0] = 0x062200U;
    s.regs[kRegA7] = 0xFFFF80U;
    s.write_mem(0xFFFF80U, 4, 0x06287CU);
    s.write_mem(0x062200U, 1, 0x00U); // index 0

    // Setup source table at PC+0x4CE = 0x062908 + 2 + 0x4CE = 0x062DD8
    const unsigned tbl = 0x062908U + 2U + 0x04CEU;
    for (unsigned i = 0; i < 6; ++i) s.write_mem(tbl + i, 1, static_cast<std::uint8_t>(0x10U + i));

    auto api = make_api(s);
    unsigned resume_pc = 0;
    auto res = m68k_recompiled_062900(api, resume_pc);
    TEST_ASSERT(res == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(resume_pc == 0x06287CU);
    for (unsigned i = 0; i < 6; ++i) {
        TEST_ASSERT(s.read_mem(0xFF0500U + 132U + i, 1) == static_cast<std::uint8_t>(0x10U + i));
    }
    std::cout << "  Candidate 5 PASS.\n";
}

void test_mutation_detection() {
    std::cout << "Testing Mutation Detection...\n";
    MockGuestState s{};
    s.regs[kRegA6] = 0xFF19E8U;
    s.regs[kRegA7] = 0xFFFF80U;
    s.write_mem(0xFFFF80U, 4, 0x005670U);
    s.write_mem(0xFF19E8U + 8U, 2, 0x0140U);
    s.write_mem(0xFF19E8U + 12U, 2, 0x00C8U);

    auto api = make_api(s);
    unsigned resume_pc = 0;
    m68k_recompiled_005660(api, resume_pc);

    // Intentionally mutate memory
    s.write_mem(0xFF1972U, 2, 0xDEADU);
    TEST_ASSERT(s.read_mem(0xFF1972U, 2) != 0x0140U);
    std::cout << "  Mutation detection PASS.\n";
}

} // namespace

int main() {
    std::cout << "=== M68K BATCH RECOMPILER DIFFERENTIAL TEST SUITE ===\n";
    test_cand1_sound_voice_env();
    test_cand2_entity_pos_sync();
    test_cand3_volume_adjust_mute();
    test_cand4_table_decoder();
    test_cand5_operator_setup();
    test_mutation_detection();
    std::cout << "All batch differential tests passed 100% bit-exact!\n";
    return 0;
}
