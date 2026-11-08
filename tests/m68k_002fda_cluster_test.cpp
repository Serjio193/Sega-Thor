#include "tools/hybrid/m68k_002fda_instruction_step.hpp"
#include "tools/hybrid/m68k_002fda_step_ops.hpp"

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
using namespace oasis::hybrid::step_002fda;

std::array<std::uint32_t, 18> g_regs{};
std::map<std::uint32_t, std::uint8_t> g_mem{};
unsigned g_last_return_pc{};

std::uint32_t mock_read_reg(void*, unsigned reg) { return reg < g_regs.size() ? g_regs[reg] : 0U; }
void mock_write_reg(void*, unsigned reg, std::uint32_t val) { if (reg < g_regs.size()) g_regs[reg] = val; }

std::uint32_t mock_read_mem(void*, unsigned addr, int width) {
    addr &= 0xFFFFFFU; std::uint32_t val = 0U;
    for (int i = 0; i < width; ++i) val = (val << 8U) | g_mem[addr + i];
    return val;
}

void mock_write_mem(void*, unsigned addr, int width, std::uint32_t val) {
    addr &= 0xFFFFFFU;
    for (int i = width - 1; i >= 0; --i) {
        g_mem[addr + i] = static_cast<std::uint8_t>(val & 0xFFU);
        val >>= 8U;
    }
}

int mock_peek(void*, unsigned addr) { return g_mem[addr & 0xFFFFFFU]; }
void mock_set_return_state(void*, unsigned pc, unsigned, unsigned) { g_last_return_pc = pc; }
std::uint32_t mock_fetch16(void*) { return 0x4E71U; }
void mock_begin_inst(void*, unsigned) {}
void mock_finish_inst(void*, unsigned) {}
void mock_add_cycles(void*, int) {}
int mock_word_not_taken(void*) { return 14; }
int mock_byte_not_taken(void*) { return 12; }
void mock_observe(void*, unsigned, unsigned) {}
void mock_skip_bus(void*) {}

M68k9D70StepApi make_test_api() {
    M68k9D70StepApi api{};
    api.read_register = mock_read_reg;
    api.write_register = mock_write_reg;
    api.peek = mock_peek;
    api.read = mock_read_mem;
    api.write = mock_write_mem;
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

void write_rom_word(unsigned addr, std::uint16_t w) {
    g_mem[addr & 0xFFFFFFU] = static_cast<std::uint8_t>((w >> 8) & 0xFF);
    g_mem[(addr + 1) & 0xFFFFFFU] = static_cast<std::uint8_t>(w & 0xFF);
}

void setup_rom_words() {
    write_rom_word(0x002FDAU, 0x47F9);
    write_rom_word(0x002FDCU, 0x00FF);
    write_rom_word(0x002FDEU, 0x134C);
    write_rom_word(0x002FE0U, 0xD7C6);
    write_rom_word(0x002FE2U, 0x0245);
    write_rom_word(0x002FE4U, 0x0007);
    write_rom_word(0x002FE6U, 0x381E);
    write_rom_word(0x002FE8U, 0x3004);
    write_rom_word(0x002FEAU, 0x0240);
    write_rom_word(0x002FECU, 0x000E);
    write_rom_word(0x002FEEU, 0xC0C5);
    write_rom_word(0x002FF0U, 0x80FC);
    write_rom_word(0x002FF2U, 0x0007);
    write_rom_word(0x002FF4U, 0x0240);
    write_rom_word(0x002FF6U, 0x000E);
    write_rom_word(0x002FF8U, 0x3200);
    write_rom_word(0x002FFAU, 0x3004);
    write_rom_word(0x002FFCU, 0x0240);
    write_rom_word(0x002FFEU, 0x00E0);
    write_rom_word(0x003000U, 0xC0C5);
    write_rom_word(0x003002U, 0x80FC);
    write_rom_word(0x003004U, 0x0007);
    write_rom_word(0x003006U, 0x0240);
    write_rom_word(0x003008U, 0x00E0);
    write_rom_word(0x00300AU, 0x8240);
    write_rom_word(0x00300CU, 0x3004);
    write_rom_word(0x00300EU, 0x0240);
    write_rom_word(0x003010U, 0x0E00);
    write_rom_word(0x003012U, 0xC0C5);
    write_rom_word(0x003014U, 0x80FC);
    write_rom_word(0x003016U, 0x0007);
    write_rom_word(0x003018U, 0x0240);
    write_rom_word(0x00301AU, 0x0E00);
    write_rom_word(0x00301CU, 0x8240);
    write_rom_word(0x00301EU, 0x36C1);
    write_rom_word(0x003020U, 0x51CF);
    write_rom_word(0x003022U, 0xFFC4);
    write_rom_word(0x003024U, 0x08F9);
    write_rom_word(0x003026U, 0x0000);
    write_rom_word(0x003028U, 0x00FF);
    write_rom_word(0x00302AU, 0x164D);
    write_rom_word(0x00302CU, 0x4E75);
}

void run_subroutine(const M68k9D70StepApi& api) {
    unsigned pc = 0x002FDAU;
    g_last_return_pc = pc;
    int steps = 0;
    while (steps < 20000) {
        auto res = step_m68k_002fda(pc, api, true);
        if (res != M68k9D70StepResult::InstructionStepped) {
            std::cerr << "Step returned " << static_cast<int>(res) << " at PC=0x" << std::hex << pc << "\n";
            std::exit(1);
        }
        pc = g_last_return_pc;
        steps++;
        if (pc == 0x001000U) {
            break;
        }
    }
    if (pc != 0x001000U) {
        std::cerr << "Did not reach return address, stopped at PC=0x" << std::hex << pc << " steps=" << std::dec << steps << "\n";
        std::exit(1);
    }
}

} // namespace

int main() {
    setup_rom_words();
    const auto api = make_test_api();

    // Test 1: Full intensity (factor 7) on 4 colors
    {
        reset_002fda_state();
        g_regs.fill(0);
        g_mem.clear();
        setup_rom_words();

        // Stack setup
        g_regs[15] = 0x00FF8000U;
        g_mem[0x00FF8000U] = 0x00; g_mem[0x00FF8001U] = 0x00;
        g_mem[0x00FF8002U] = 0x10; g_mem[0x00FF8003U] = 0x00; // return PC: 0x001000

        // Palette source at 0xFF2000
        g_regs[kRegA6] = 0x00FF2000U;
        g_regs[kRegD6] = 0x00000000U; // offset into FF134C = 0
        g_regs[kRegD5] = 7U;          // full intensity (factor 7)
        g_regs[kRegD7] = 3U;          // 4 colors (d7 = 3)

        // Color 0: 0x0EEE (white: R=E, G=E, B=E)
        write_rom_word(0x00FF2000U, 0x0EEE);
        // Color 1: 0x000E (red: R=E, G=0, B=0)
        write_rom_word(0x00FF2002U, 0x000E);
        // Color 2: 0x00E0 (green: R=0, G=E, B=0)
        write_rom_word(0x00FF2004U, 0x00E0);
        // Color 3: 0x0E00 (blue: R=0, G=0, B=E)
        write_rom_word(0x00FF2006U, 0x0E00);

        // FF164D init
        g_mem[0x00FF164DU] = 0x00;

        run_subroutine(api);

        // Check output in FF134C: full intensity should preserve colors exactly
        const auto c0 = (g_mem[0x00FF134CU] << 8) | g_mem[0x00FF134DU];
        const auto c1 = (g_mem[0x00FF134EU] << 8) | g_mem[0x00FF134FU];
        const auto c2 = (g_mem[0x00FF1350U] << 8) | g_mem[0x00FF1351U];
        const auto c3 = (g_mem[0x00FF1352U] << 8) | g_mem[0x00FF1353U];
        TEST_ASSERT(c0 == 0x0EEE);
        TEST_ASSERT(c1 == 0x000E);
        TEST_ASSERT(c2 == 0x00E0);
        TEST_ASSERT(c3 == 0x0E00);

        // Check dirty bit in FF164D (bit 0 must be set)
        TEST_ASSERT((g_mem[0x00FF164DU] & 1) == 1);
        std::cout << "Test 1 (Full intensity factor 7) PASSED\n";
    }

    // Test 2: Zero intensity (factor 0) -> all black
    {
        reset_002fda_state();
        g_regs.fill(0);
        g_mem.clear();
        setup_rom_words();

        g_regs[15] = 0x00FF8000U;
        g_mem[0x00FF8000U] = 0x00; g_mem[0x00FF8001U] = 0x00;
        g_mem[0x00FF8002U] = 0x10; g_mem[0x00FF8003U] = 0x00;

        g_regs[kRegA6] = 0x00FF2000U;
        g_regs[kRegD6] = 0x00000020U; // offset = 0x20
        g_regs[kRegD5] = 0U;          // factor 0
        g_regs[kRegD7] = 1U;          // 2 colors

        write_rom_word(0x00FF2000U, 0x0EEE);
        write_rom_word(0x00FF2002U, 0x048A);

        g_mem[0x00FF164DU] = 0x00;

        run_subroutine(api);

        const auto c0 = (g_mem[0x00FF136CU] << 8) | g_mem[0x00FF136DU];
        const auto c1 = (g_mem[0x00FF136EU] << 8) | g_mem[0x00FF136FU];
        TEST_ASSERT(c0 == 0x0000);
        TEST_ASSERT(c1 == 0x0000);
        TEST_ASSERT((g_mem[0x00FF164DU] & 1) == 1);
        std::cout << "Test 2 (Zero intensity factor 0) PASSED\n";
    }

    // Test 3: Half intensity (factor 3)
    {
        reset_002fda_state();
        g_regs.fill(0);
        g_mem.clear();
        setup_rom_words();

        g_regs[15] = 0x00FF8000U;
        g_mem[0x00FF8000U] = 0x00; g_mem[0x00FF8001U] = 0x00;
        g_mem[0x00FF8002U] = 0x10; g_mem[0x00FF8003U] = 0x00;

        g_regs[kRegA6] = 0x00FF2000U;
        g_regs[kRegD6] = 0x00000000U;
        g_regs[kRegD5] = 3U; // factor 3
        g_regs[kRegD7] = 0U; // 1 color

        // 0x0EEE: R=14, G=14, B=14
        // R: (14 * 3) / 7 = 42 / 7 = 6 (0x6) & 0xE = 0x6
        // G: (14 * 3) / 7 = 6 -> shifted 0x60
        // B: (14 * 3) / 7 = 6 -> shifted 0x600
        // Expected: 0x0666
        write_rom_word(0x00FF2000U, 0x0EEE);

        g_mem[0x00FF164DU] = 0x02; // already has bit 1 set

        run_subroutine(api);

        const auto c0 = (g_mem[0x00FF134CU] << 8) | g_mem[0x00FF134DU];
        TEST_ASSERT(c0 == 0x0666);
        TEST_ASSERT((g_mem[0x00FF164DU] & 3) == 3);
        std::cout << "Test 3 (Half intensity factor 3) PASSED\n";
    }

    std::cout << "All m68k_002fda_cluster_test cases PASSED!\n";
    return 0;
}
