#include "tools/hybrid/m68k_002512_instruction_step.hpp"
#include "tools/hybrid/m68k_002512_step_ops.hpp"

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
using namespace oasis::hybrid::step_002512;

std::array<std::uint32_t, 18> g_regs{};
std::map<std::uint32_t, std::uint8_t> g_mem{};
unsigned g_last_return_pc{};
std::vector<std::pair<unsigned, std::uint16_t>> g_vdp_writes{};

std::uint32_t mock_read_reg(void*, unsigned reg) { return reg < g_regs.size() ? g_regs[reg] : 0U; }
void mock_write_reg(void*, unsigned reg, std::uint32_t val) { if (reg < g_regs.size()) g_regs[reg] = val; }

std::uint32_t mock_read_mem(void*, unsigned addr, int width) {
    addr &= 0xFFFFFFU; std::uint32_t val = 0U;
    for (int i = 0; i < width; ++i) val = (val << 8U) | g_mem[addr + i];
    return val;
}

void mock_write_mem(void*, unsigned addr, int width, std::uint32_t val) {
    addr &= 0xFFFFFFU;
    if (addr == 0xC00000U || addr == 0xC00004U) {
        g_vdp_writes.push_back({addr, static_cast<std::uint16_t>(val & 0xFFFFU)});
    }
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

void write_mem_word(unsigned addr, std::uint16_t w) {
    write_rom_word(addr, w);
}

void write_mem_long(unsigned addr, std::uint32_t l) {
    write_mem_word(addr, static_cast<std::uint16_t>(l >> 16));
    write_mem_word(addr + 2U, static_cast<std::uint16_t>(l & 0xFFFFU));
}

void setup_rom_words() {
    write_rom_word(0x002512U, 0x206BU);
    write_rom_word(0x002514U, 0x0016U);
    write_rom_word(0x002516U, 0x3A2BU);
    write_rom_word(0x002518U, 0x001AU);
    write_rom_word(0x00251AU, 0x45F9U);
    write_rom_word(0x00251CU, 0x00C0U);
    write_rom_word(0x00251EU, 0x0000U);
    write_rom_word(0x002520U, 0x49EAU);
    write_rom_word(0x002522U, 0x0004U);
    write_rom_word(0x002524U, 0x3C2BU);
    write_rom_word(0x002526U, 0x001EU);
    write_rom_word(0x002528U, 0x6B00U);
    write_rom_word(0x00252AU, 0x006CU);
    write_rom_word(0x00252CU, 0x322BU);
    write_rom_word(0x00252EU, 0x000CU);
    write_rom_word(0x002530U, 0x5346U);
    write_rom_word(0x002532U, 0x3746U);
    write_rom_word(0x002534U, 0x001EU);
    write_rom_word(0x002536U, 0x362BU);
    write_rom_word(0x002538U, 0x0020U);
    write_rom_word(0x00253AU, 0x382BU);
    write_rom_word(0x00253CU, 0x0022U);
    write_rom_word(0x00253EU, 0x342BU);
    write_rom_word(0x002540U, 0x0008U);
    write_rom_word(0x002542U, 0x3002U);
    write_rom_word(0x002544U, 0xD040U);
    write_rom_word(0x002546U, 0x3E2BU);
    write_rom_word(0x002548U, 0x000AU);
    write_rom_word(0x00254AU, 0x5247U);
    write_rom_word(0x00254CU, 0xBE44U);
    write_rom_word(0x00254EU, 0x6D00U);
    write_rom_word(0x002550U, 0x0004U);
    write_rom_word(0x002552U, 0x9E44U);
    write_rom_word(0x002554U, 0x3747U);
    write_rom_word(0x002556U, 0x000AU);
    write_rom_word(0x002558U, 0x0647U);
    write_rom_word(0x00255AU, 0x001CU);
    write_rom_word(0x00255CU, 0xBE44U);
    write_rom_word(0x00255EU, 0x6D00U);
    write_rom_word(0x002560U, 0x0004U);
    write_rom_word(0x002562U, 0x9E44U);
    write_rom_word(0x002564U, 0x3C2BU);
    write_rom_word(0x002566U, 0x0024U);
    write_rom_word(0x002568U, 0xED6FU);
    write_rom_word(0x00256AU, 0xDE40U);
    write_rom_word(0x00256CU, 0xD0C7U);
    write_rom_word(0x00256EU, 0x3801U);
    write_rom_word(0x002570U, 0x0241U);
    write_rom_word(0x002572U, 0x0F80U);
    write_rom_word(0x002574U, 0x0641U);
    write_rom_word(0x002576U, 0x0080U);
    write_rom_word(0x002578U, 0x0244U);
    write_rom_word(0x00257AU, 0x007EU);
    write_rom_word(0x00257CU, 0x0241U);
    write_rom_word(0x00257EU, 0x0F80U);
    write_rom_word(0x002580U, 0x3E01U);
    write_rom_word(0x002582U, 0xDE44U);
    write_rom_word(0x002584U, 0x3747U);
    write_rom_word(0x002586U, 0x000CU);
    write_rom_word(0x002588U, 0x0641U);
    write_rom_word(0x00258AU, 0x0E00U);
    write_rom_word(0x00258CU, 0x0241U);
    write_rom_word(0x00258EU, 0x0F80U);
    write_rom_word(0x002590U, 0xD244U);
    write_rom_word(0x002592U, 0x6000U);
    write_rom_word(0x002594U, 0x004EU);
    write_rom_word(0x002596U, 0x322BU);
    write_rom_word(0x002598U, 0x000CU);
    write_rom_word(0x00259AU, 0x5246U);
    write_rom_word(0x00259CU, 0x3746U);
    write_rom_word(0x00259EU, 0x001EU);
    write_rom_word(0x0025A0U, 0x362BU);
    write_rom_word(0x0025A2U, 0x0020U);
    write_rom_word(0x0025A4U, 0x382BU);
    write_rom_word(0x0025A6U, 0x0022U);
    write_rom_word(0x0025A8U, 0x342BU);
    write_rom_word(0x0025AAU, 0x0008U);
    write_rom_word(0x0025ACU, 0x3002U);
    write_rom_word(0x0025AEU, 0xD040U);
    write_rom_word(0x0025B0U, 0x3E2BU);
    write_rom_word(0x0025B2U, 0x000AU);
    write_rom_word(0x0025B4U, 0x5347U);
    write_rom_word(0x0025B6U, 0x6A00U);
    write_rom_word(0x0025B8U, 0x0004U);
    write_rom_word(0x0025BAU, 0xDE44U);
    write_rom_word(0x0025BCU, 0x3747U);
    write_rom_word(0x0025BEU, 0x000AU);
    write_rom_word(0x0025C0U, 0x3C2BU);
    write_rom_word(0x0025C2U, 0x0024U);
    write_rom_word(0x0025C4U, 0xED6FU);
    write_rom_word(0x0025C6U, 0xDE40U);
    write_rom_word(0x0025C8U, 0xD0C7U);
    write_rom_word(0x0025CAU, 0x3801U);
    write_rom_word(0x0025CCU, 0x0241U);
    write_rom_word(0x0025CEU, 0x0F80U);
    write_rom_word(0x0025D0U, 0x0441U);
    write_rom_word(0x0025D2U, 0x0080U);
    write_rom_word(0x0025D4U, 0x0244U);
    write_rom_word(0x0025D6U, 0x007EU);
    write_rom_word(0x0025D8U, 0x0241U);
    write_rom_word(0x0025DAU, 0x0F80U);
    write_rom_word(0x0025DCU, 0xD244U);
    write_rom_word(0x0025DEU, 0x3741U);
    write_rom_word(0x0025E0U, 0x000CU);
    write_rom_word(0x0025E2U, 0xE24CU);
    write_rom_word(0x0025E4U, 0x3C05U);
    write_rom_word(0x0025E6U, 0xE55DU);
    write_rom_word(0x0025E8U, 0x0245U);
    write_rom_word(0x0025EAU, 0x0003U);
    write_rom_word(0x0025ECU, 0x0246U);
    write_rom_word(0x0025EEU, 0x3FFFU);
    write_rom_word(0x0025F0U, 0x0046U);
    write_rom_word(0x0025F2U, 0x4000U);
    write_rom_word(0x0025F4U, 0x3006U);
    write_rom_word(0x0025F6U, 0xD041U);
    write_rom_word(0x0025F8U, 0x3880U);
    write_rom_word(0x0025FAU, 0x3885U);
    write_rom_word(0x0025FCU, 0x3E3CU);
    write_rom_word(0x0025FEU, 0x0028U);
    write_rom_word(0x002600U, 0x3498U);
    write_rom_word(0x002602U, 0x5242U);
    write_rom_word(0x002604U, 0xB443U);
    write_rom_word(0x002606U, 0x6D00U);
    write_rom_word(0x002608U, 0x0008U);
    write_rom_word(0x00260AU, 0x9443U);
    write_rom_word(0x00260CU, 0x90C3U);
    write_rom_word(0x00260EU, 0x90C3U);
    write_rom_word(0x002610U, 0x0641U);
    write_rom_word(0x002612U, 0x0002U);
    write_rom_word(0x002614U, 0x5244U);
    write_rom_word(0x002616U, 0x0C44U);
    write_rom_word(0x002618U, 0x0040U);
    write_rom_word(0x00261AU, 0x6D00U);
    write_rom_word(0x00261CU, 0x0010U);
    write_rom_word(0x00261EU, 0x0444U);
    write_rom_word(0x002620U, 0x0040U);
    write_rom_word(0x002622U, 0x0441U);
    write_rom_word(0x002624U, 0x0080U);
    write_rom_word(0x002626U, 0xDC41U);
    write_rom_word(0x002628U, 0x3886U);
    write_rom_word(0x00262AU, 0x3885U);
    write_rom_word(0x00262CU, 0x51CFU);
    write_rom_word(0x00262EU, 0xFFD2U);
    write_rom_word(0x002630U, 0x4E75U);
}

void test_initial_disabled() {
    auto api = make_test_api();
    reset_002512_state();
    reset_002512_step_metrics();
    auto res = step_m68k_002512(0x002512U, api, false);
    TEST_ASSERT(res == M68k9D70StepResult::NotHandled);
    std::cout << "[PASS] test_initial_disabled\n";
}

void test_full_scroll_up_row_upload() {
    auto api = make_test_api();
    g_regs.fill(0);
    g_mem.clear();
    g_vdp_writes.clear();
    setup_rom_words();
    reset_002512_state();
    reset_002512_step_metrics();

    const unsigned a3 = 0xFF1700U;
    g_regs[kRegA3] = a3;
    const unsigned tile_data_addr = 0xFF4000U;
    write_mem_long(a3 + 22U, tile_data_addr);
    write_mem_word(a3 + 26U, 0xC000U); // base VRAM destination
    write_mem_word(a3 + 30U, 0x0002U); // scroll up: positive counter
    write_mem_word(a3 + 12U, 0x0000U); // VRAM offset
    write_mem_word(a3 + 32U, 0x0080U); // plane width
    write_mem_word(a3 + 34U, 0x0020U); // plane height
    write_mem_word(a3 + 8U,  0x0004U); // x pos
    write_mem_word(a3 + 10U, 0x0005U); // y pos
    write_mem_word(a3 + 36U, 0x0007U); // shift factor

    // Fill source tile data
    for (unsigned i = 0; i < 64; ++i) {
        write_mem_word(tile_data_addr + i * 2U, static_cast<std::uint16_t>(0x0100U + i));
    }

    // Setup stack for caller return
    const unsigned sp = 0xFFFF00U;
    g_regs[kRegSP] = sp;
    write_mem_long(sp, 0x0020C8U); // return PC

    unsigned pc = 0x002512U;
    unsigned steps = 0;
    while (pc != 0x0020C8U && steps < 500) {
        auto res = step_m68k_002512(pc, api, true);
        TEST_ASSERT(res == M68k9D70StepResult::InstructionStepped);
        pc = g_last_return_pc;
        steps++;
    }

    TEST_ASSERT(pc == 0x0020C8U);
    TEST_ASSERT(steps > 40);
    TEST_ASSERT(g_regs[kRegSP] == sp + 4U); // stack popped by RTS
    TEST_ASSERT(!g_vdp_writes.empty());

    const auto& metrics = get_002512_step_metrics();
    TEST_ASSERT(metrics.target_entries == 1);
    TEST_ASSERT(metrics.native_calls == 1);
    TEST_ASSERT(metrics.completed_calls == 1);
    TEST_ASSERT(metrics.rows_uploaded == 1);
    TEST_ASSERT(metrics.stepped_instructions == steps);

    std::cout << "[PASS] test_full_scroll_up_row_upload (steps=" << steps << ", vdp_writes=" << g_vdp_writes.size() << ")\n";
}

void test_full_scroll_down_row_upload() {
    auto api = make_test_api();
    g_regs.fill(0);
    g_mem.clear();
    g_vdp_writes.clear();
    setup_rom_words();
    reset_002512_state();
    reset_002512_step_metrics();

    const unsigned a3 = 0xFF173EU;
    g_regs[kRegA3] = a3;
    const unsigned tile_data_addr = 0xFF6000U;
    write_mem_long(a3 + 22U, tile_data_addr);
    write_mem_word(a3 + 26U, 0xE000U); // base VRAM destination (Plane B)
    write_mem_word(a3 + 30U, 0xFFFFU); // scroll down: negative counter (-1)
    write_mem_word(a3 + 12U, 0x0100U);
    write_mem_word(a3 + 32U, 0x0080U);
    write_mem_word(a3 + 34U, 0x0020U);
    write_mem_word(a3 + 8U,  0x0008U);
    write_mem_word(a3 + 10U, 0x0000U);
    write_mem_word(a3 + 36U, 0x0007U);

    for (unsigned i = 0; i < 64; ++i) {
        write_mem_word(tile_data_addr + i * 2U, static_cast<std::uint16_t>(0x0200U + i));
    }

    const unsigned sp = 0xFFFF20U;
    g_regs[kRegSP] = sp;
    write_mem_long(sp, 0x00210AU); // second caller return PC

    unsigned pc = 0x002512U;
    unsigned steps = 0;
    while (pc != 0x00210AU && steps < 500) {
        auto res = step_m68k_002512(pc, api, true);
        TEST_ASSERT(res == M68k9D70StepResult::InstructionStepped);
        pc = g_last_return_pc;
        steps++;
    }

    TEST_ASSERT(pc == 0x00210AU);
    TEST_ASSERT(steps > 40);
    TEST_ASSERT(g_regs[kRegSP] == sp + 4U);
    TEST_ASSERT(!g_vdp_writes.empty());

    const auto& metrics = get_002512_step_metrics();
    TEST_ASSERT(metrics.target_entries == 1);
    TEST_ASSERT(metrics.completed_calls == 1);
    TEST_ASSERT(metrics.rows_uploaded == 1);

    std::cout << "[PASS] test_full_scroll_down_row_upload (steps=" << steps << ", vdp_writes=" << g_vdp_writes.size() << ")\n";
}

} // namespace

int main() {
    test_initial_disabled();
    test_full_scroll_up_row_upload();
    test_full_scroll_down_row_upload();
    std::cout << "ALL M68K 0x002512 CLUSTER TESTS PASSED!\n";
    return 0;
}
