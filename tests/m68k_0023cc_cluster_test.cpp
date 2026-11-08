#include "tools/hybrid/m68k_0023cc_instruction_step.hpp"
#include "tools/hybrid/m68k_0023cc_step_ops.hpp"

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
using namespace oasis::hybrid::step_0023cc;

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

void write_mem_word(unsigned addr, std::uint16_t w) { write_rom_word(addr, w); }

void write_mem_long(unsigned addr, std::uint32_t l) {
    write_mem_word(addr, static_cast<std::uint16_t>(l >> 16));
    write_mem_word(addr + 2U, static_cast<std::uint16_t>(l & 0xFFFFU));
}

void setup_rom_words() {
    write_rom_word(0x0023CCU, 0x206BU);
    write_rom_word(0x0023CEU, 0x0016U);
    write_rom_word(0x0023D0U, 0x3A2BU);
    write_rom_word(0x0023D2U, 0x001AU);
    write_rom_word(0x0023D4U, 0x45F9U);
    write_rom_word(0x0023D6U, 0x00C0U);
    write_rom_word(0x0023D8U, 0x0000U);
    write_rom_word(0x0023DAU, 0x49EAU);
    write_rom_word(0x0023DCU, 0x0004U);
    write_rom_word(0x0023DEU, 0x38BCU);
    write_rom_word(0x0023E0U, 0x8F80U);
    write_rom_word(0x0023E2U, 0x3C2BU);
    write_rom_word(0x0023E4U, 0x001CU);
    write_rom_word(0x0023E6U, 0x6B00U);
    write_rom_word(0x0023E8U, 0x0076U);
    write_rom_word(0x0023EAU, 0x322BU);
    write_rom_word(0x0023ECU, 0x000CU);
    write_rom_word(0x0023EEU, 0x5346U);
    write_rom_word(0x0023F0U, 0x3746U);
    write_rom_word(0x0023F2U, 0x001CU);
    write_rom_word(0x0023F4U, 0x3E2BU);
    write_rom_word(0x0023F6U, 0x0024U);
    write_rom_word(0x0023F8U, 0x382BU);
    write_rom_word(0x0023FAU, 0x0020U);
    write_rom_word(0x0023FCU, 0x362BU);
    write_rom_word(0x0023FEU, 0x0022U);
    write_rom_word(0x002400U, 0x3404U);
    write_rom_word(0x002402U, 0xD442U);
    write_rom_word(0x002404U, 0x4842U);
    write_rom_word(0x002406U, 0x342BU);
    write_rom_word(0x002408U, 0x000AU);
    write_rom_word(0x00240AU, 0x3002U);
    write_rom_word(0x00240CU, 0xEF68U);
    write_rom_word(0x00240EU, 0x3E2BU);
    write_rom_word(0x002410U, 0x0008U);
    write_rom_word(0x002412U, 0x5247U);
    write_rom_word(0x002414U, 0xBE44U);
    write_rom_word(0x002416U, 0x6D00U);
    write_rom_word(0x002418U, 0x0004U);
    write_rom_word(0x00241AU, 0x9E44U);
    write_rom_word(0x00241CU, 0x3747U);
    write_rom_word(0x00241EU, 0x0008U);
    write_rom_word(0x002420U, 0x0647U);
    write_rom_word(0x002422U, 0x0028U);
    write_rom_word(0x002424U, 0xBE44U);
    write_rom_word(0x002426U, 0x6D00U);
    write_rom_word(0x002428U, 0x000AU);
    write_rom_word(0x00242AU, 0x9E44U);
    write_rom_word(0x00242CU, 0xBE44U);
    write_rom_word(0x00242EU, 0x6C00U);
    write_rom_word(0x002430U, 0xFFFAU);
    write_rom_word(0x002432U, 0xDE47U);
    write_rom_word(0x002434U, 0xDE40U);
    write_rom_word(0x002436U, 0xD0C7U);
    write_rom_word(0x002438U, 0x3801U);
    write_rom_word(0x00243AU, 0x0241U);
    write_rom_word(0x00243CU, 0x007EU);
    write_rom_word(0x00243EU, 0x5441U);
    write_rom_word(0x002440U, 0x0241U);
    write_rom_word(0x002442U, 0x007EU);
    write_rom_word(0x002444U, 0x0244U);
    write_rom_word(0x002446U, 0x0F80U);
    write_rom_word(0x002448U, 0x3E01U);
    write_rom_word(0x00244AU, 0xDE44U);
    write_rom_word(0x00244CU, 0x3747U);
    write_rom_word(0x00244EU, 0x000CU);
    write_rom_word(0x002450U, 0x0641U);
    write_rom_word(0x002452U, 0x0050U);
    write_rom_word(0x002454U, 0x0241U);
    write_rom_word(0x002456U, 0x007EU);
    write_rom_word(0x002458U, 0xD244U);
    write_rom_word(0x00245AU, 0x6000U);
    write_rom_word(0x00245CU, 0x0052U);
    write_rom_word(0x00245EU, 0x322BU);
    write_rom_word(0x002460U, 0x000CU);
    write_rom_word(0x002462U, 0x5246U);
    write_rom_word(0x002464U, 0x3746U);
    write_rom_word(0x002466U, 0x001CU);
    write_rom_word(0x002468U, 0x3E2BU);
    write_rom_word(0x00246AU, 0x0024U);
    write_rom_word(0x00246CU, 0x382BU);
    write_rom_word(0x00246EU, 0x0020U);
    write_rom_word(0x002470U, 0x362BU);
    write_rom_word(0x002472U, 0x0022U);
    write_rom_word(0x002474U, 0x3404U);
    write_rom_word(0x002476U, 0xD442U);
    write_rom_word(0x002478U, 0x4842U);
    write_rom_word(0x00247AU, 0x342BU);
    write_rom_word(0x00247CU, 0x000AU);
    write_rom_word(0x00247EU, 0x3002U);
    write_rom_word(0x002480U, 0xEF68U);
    write_rom_word(0x002482U, 0x3E2BU);
    write_rom_word(0x002484U, 0x0008U);
    write_rom_word(0x002486U, 0x5347U);
    write_rom_word(0x002488U, 0x6A00U);
    write_rom_word(0x00248AU, 0x0004U);
    write_rom_word(0x00248CU, 0xDE44U);
    write_rom_word(0x00248EU, 0x3747U);
    write_rom_word(0x002490U, 0x0008U);
    write_rom_word(0x002492U, 0xDE47U);
    write_rom_word(0x002494U, 0xDE40U);
    write_rom_word(0x002496U, 0xD0C7U);
    write_rom_word(0x002498U, 0x3801U);
    write_rom_word(0x00249AU, 0x0241U);
    write_rom_word(0x00249CU, 0x007EU);
    write_rom_word(0x00249EU, 0x5541U);
    write_rom_word(0x0024A0U, 0x0241U);
    write_rom_word(0x0024A2U, 0x007EU);
    write_rom_word(0x0024A4U, 0x0244U);
    write_rom_word(0x0024A6U, 0x0F80U);
    write_rom_word(0x0024A8U, 0xD244U);
    write_rom_word(0x0024AAU, 0x3741U);
    write_rom_word(0x0024ACU, 0x000CU);
    write_rom_word(0x0024AEU, 0xEE4CU);
    write_rom_word(0x0024B0U, 0x3C05U);
    write_rom_word(0x0024B2U, 0xE55DU);
    write_rom_word(0x0024B4U, 0x0245U);
    write_rom_word(0x0024B6U, 0x0003U);
    write_rom_word(0x0024B8U, 0x0246U);
    write_rom_word(0x0024BAU, 0x3FFFU);
    write_rom_word(0x0024BCU, 0x0046U);
    write_rom_word(0x0024BEU, 0x4000U);
    write_rom_word(0x0024C0U, 0x3006U);
    write_rom_word(0x0024C2U, 0xD041U);
    write_rom_word(0x0024C4U, 0x3880U);
    write_rom_word(0x0024C6U, 0x3885U);
    write_rom_word(0x0024C8U, 0x3E3CU);
    write_rom_word(0x0024CAU, 0x001CU);
    write_rom_word(0x0024CCU, 0x3490U);
    write_rom_word(0x0024CEU, 0x4842U);
    write_rom_word(0x0024D0U, 0xD0C2U);
    write_rom_word(0x0024D2U, 0x4842U);
    write_rom_word(0x0024D4U, 0x5242U);
    write_rom_word(0x0024D6U, 0xB443U);
    write_rom_word(0x0024D8U, 0x6D00U);
    write_rom_word(0x0024DAU, 0x0012U);
    write_rom_word(0x0024DCU, 0x302BU);
    write_rom_word(0x0024DEU, 0x0024U);
    write_rom_word(0x0024E0U, 0x362BU);
    write_rom_word(0x0024E2U, 0x0022U);
    write_rom_word(0x0024E4U, 0xE16BU);
    write_rom_word(0x0024E6U, 0x90C3U);
    write_rom_word(0x0024E8U, 0x363CU);
    write_rom_word(0x0024EAU, 0x03E8U);
    write_rom_word(0x0024ECU, 0x0641U);
    write_rom_word(0x0024EEU, 0x0080U);
    write_rom_word(0x0024F0U, 0x5244U);
    write_rom_word(0x0024F2U, 0x0C44U);
    write_rom_word(0x0024F4U, 0x0020U);
    write_rom_word(0x0024F6U, 0x6D00U);
    write_rom_word(0x0024F8U, 0x0010U);
    write_rom_word(0x0024FAU, 0x0444U);
    write_rom_word(0x0024FCU, 0x0020U);
    write_rom_word(0x0024FEU, 0x0441U);
    write_rom_word(0x002500U, 0x1000U);
    write_rom_word(0x002502U, 0xDC41U);
    write_rom_word(0x002504U, 0x3886U);
    write_rom_word(0x002506U, 0x3885U);
    write_rom_word(0x002508U, 0x51CFU);
    write_rom_word(0x00250AU, 0xFFC2U);
    write_rom_word(0x00250CU, 0x38BCU);
    write_rom_word(0x00250EU, 0x8F02U);
    write_rom_word(0x002510U, 0x4E75U);
}

void test_initial_disabled() {
    auto api = make_test_api();
    reset_0023cc_state();
    reset_0023cc_step_metrics();
    auto res = step_m68k_0023cc(0x0023CCU, api, false);
    TEST_ASSERT(res == M68k9D70StepResult::NotHandled);
    std::cout << "[PASS] test_initial_disabled\n";
}

void test_full_scroll_right_column_upload() {
    auto api = make_test_api();
    g_regs.fill(0);
    g_mem.clear();
    g_vdp_writes.clear();
    setup_rom_words();
    reset_0023cc_state();
    reset_0023cc_step_metrics();

    const unsigned a3 = 0xFF1700U;
    g_regs[kRegA3] = a3;
    const unsigned tile_data_addr = 0xFF4000U;
    write_mem_long(a3 + 22U, tile_data_addr);
    write_mem_word(a3 + 26U, 0xC000U); // base VRAM destination
    write_mem_word(a3 + 28U, 0x0002U); // scroll right: positive counter
    write_mem_word(a3 + 12U, 0x0000U); // VRAM offset
    write_mem_word(a3 + 32U, 0x0080U); // plane width
    write_mem_word(a3 + 34U, 0x0020U); // plane height
    write_mem_word(a3 + 8U,  0x0004U); // x pos
    write_mem_word(a3 + 10U, 0x0005U); // y pos
    write_mem_word(a3 + 36U, 0x0007U); // shift factor

    for (unsigned i = 0; i < 64; ++i) {
        write_mem_word(tile_data_addr + i * 2U, static_cast<std::uint16_t>(0x0100U + i));
    }

    const unsigned sp = 0xFFFF00U;
    g_regs[kRegSP] = sp;
    write_mem_long(sp, 0x0020B2U); // return PC (caller 0x0020AE)

    unsigned pc = 0x0023CCU;
    unsigned steps = 0;
    while (pc != 0x0020B2U && steps < 500) {
        auto res = step_m68k_0023cc(pc, api, true);
        TEST_ASSERT(res == M68k9D70StepResult::InstructionStepped);
        pc = g_last_return_pc;
        steps++;
    }

    TEST_ASSERT(pc == 0x0020B2U);
    TEST_ASSERT(steps > 40);
    TEST_ASSERT(g_regs[kRegSP] == sp + 4U);
    TEST_ASSERT(!g_vdp_writes.empty());

    const auto& metrics = get_0023cc_step_metrics();
    TEST_ASSERT(metrics.target_entries == 1);
    TEST_ASSERT(metrics.native_calls == 1);
    TEST_ASSERT(metrics.completed_calls == 1);
    TEST_ASSERT(metrics.columns_uploaded == 1);
    TEST_ASSERT(metrics.stepped_instructions == steps);

    std::cout << "[PASS] test_full_scroll_right_column_upload (steps=" << steps << ", vdp_writes=" << g_vdp_writes.size() << ")\n";
}

void test_full_scroll_left_column_upload() {
    auto api = make_test_api();
    g_regs.fill(0);
    g_mem.clear();
    g_vdp_writes.clear();
    setup_rom_words();
    reset_0023cc_state();
    reset_0023cc_step_metrics();

    const unsigned a3 = 0xFF173EU;
    g_regs[kRegA3] = a3;
    const unsigned tile_data_addr = 0xFF6000U;
    write_mem_long(a3 + 22U, tile_data_addr);
    write_mem_word(a3 + 26U, 0xE000U); // base VRAM destination (Plane B)
    write_mem_word(a3 + 28U, 0xFFFFU); // scroll left: negative counter (-1)
    write_mem_word(a3 + 12U, 0x0020U);
    write_mem_word(a3 + 32U, 0x0080U);
    write_mem_word(a3 + 34U, 0x0020U);
    write_mem_word(a3 + 8U,  0x0008U);
    write_mem_word(a3 + 10U, 0x0002U);
    write_mem_word(a3 + 36U, 0x0007U);

    for (unsigned i = 0; i < 64; ++i) {
        write_mem_word(tile_data_addr + i * 2U, static_cast<std::uint16_t>(0x0200U + i));
    }

    const unsigned sp = 0xFFFF20U;
    g_regs[kRegSP] = sp;
    write_mem_long(sp, 0x0020F4U); // second caller return PC (0x0020F0)

    unsigned pc = 0x0023CCU;
    unsigned steps = 0;
    while (pc != 0x0020F4U && steps < 500) {
        auto res = step_m68k_0023cc(pc, api, true);
        TEST_ASSERT(res == M68k9D70StepResult::InstructionStepped);
        pc = g_last_return_pc;
        steps++;
    }

    TEST_ASSERT(pc == 0x0020F4U);
    TEST_ASSERT(steps > 40);
    TEST_ASSERT(g_regs[kRegSP] == sp + 4U);
    TEST_ASSERT(!g_vdp_writes.empty());

    const auto& metrics = get_0023cc_step_metrics();
    TEST_ASSERT(metrics.target_entries == 1);
    TEST_ASSERT(metrics.completed_calls == 1);
    TEST_ASSERT(metrics.columns_uploaded == 1);

    std::cout << "[PASS] test_full_scroll_left_column_upload (steps=" << steps << ", vdp_writes=" << g_vdp_writes.size() << ")\n";
}

} // namespace

int main() {
    test_initial_disabled();
    test_full_scroll_right_column_upload();
    test_full_scroll_left_column_upload();
    std::cout << "ALL M68K 0x0023CC CLUSTER TESTS PASSED!\n";
    return 0;
}
