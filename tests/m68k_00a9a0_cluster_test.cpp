#include "tools/hybrid/m68k_00a9a0_instruction_step.hpp"
#include "tools/hybrid/m68k_00a9a0_step_ops.hpp"

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
using namespace oasis::hybrid::step_00a9a0;

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

void init_test_rom() {
    // 0x00A9A0: 082E 0001 0036 (btst.b #1, 54(a6))
    g_mem[0x00A9A0U] = 0x08U; g_mem[0x00A9A1U] = 0x2EU;
    g_mem[0x00A9A2U] = 0x00U; g_mem[0x00A9A3U] = 0x01U;
    g_mem[0x00A9A4U] = 0x00U; g_mem[0x00A9A5U] = 0x36U;
}

void test_disabled_and_corrupt() {
    g_regs.fill(0U); g_mem.clear();
    reset_00a9a0_step_metrics();
    reset_00a9a0_state();
    auto api = make_test_api();

    auto res = step_m68k_00a9a0(kStartPc, api, false);
    TEST_ASSERT(res == M68k9D70StepResult::NotHandled);

    // Corrupt ROM test
    res = step_m68k_00a9a0(kStartPc, api, true);
    TEST_ASSERT(res == M68k9D70StepResult::NotHandled);
    std::cout << "[PASS] test_disabled_and_corrupt passed\n";
}

void test_fast_path_hidden() {
    g_regs.fill(0U); g_mem.clear();
    init_test_rom();
    reset_00a9a0_step_metrics();
    reset_00a9a0_state();

    const unsigned sp = 0x00FFFE00U;
    const unsigned a6 = 0x00FF2000U;
    g_regs[kRegSP] = sp;
    g_regs[kRegA6] = a6;
    mock_write_mem(nullptr, sp, 4, 0x0000A1F8U); // caller return
    mock_write_mem(nullptr, a6 + 54U, 1, 0x02U); // hidden

    auto api = make_test_api();
    auto res = step_m68k_00a9a0(kStartPc, api, true);
    TEST_ASSERT(res == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(g_last_return_pc == 0x00A9A6U);

    res = step_m68k_00a9a0(0x00A9A6U, api, true);
    TEST_ASSERT(res == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(g_last_return_pc == kExitRtsPc);

    res = step_m68k_00a9a0(kExitRtsPc, api, true);
    TEST_ASSERT(res == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(g_last_return_pc == 0x0000A1F8U);
    TEST_ASSERT(g_regs[kRegSP] == sp + 4U);

    const auto& m = get_00a9a0_step_metrics();
    TEST_ASSERT(m.target_entries == 1);
    TEST_ASSERT(m.completed_calls == 1);
    std::cout << "[PASS] test_fast_path_hidden passed\n";
}

void test_full_render_piece_loop() {
    g_regs.fill(0U); g_mem.clear();
    init_test_rom();
    reset_00a9a0_step_metrics();
    reset_00a9a0_state();

    const unsigned sp = 0x00FFFE00U;
    const unsigned a6 = 0x00FF2000U;
    g_regs[kRegSP] = sp;
    g_regs[kRegA6] = a6;
    g_regs[kRegD7] = 0x12345678U;

    mock_write_mem(nullptr, sp, 4, 0x0000A1F8U); // caller return address

    // Entity setup for Mode 1
    mock_write_mem(nullptr, a6 + 22U, 2, 0U); // direction/anim = 0
    mock_write_mem(nullptr, a6 + 50U, 2, 0U);
    mock_write_mem(nullptr, a6 + 52U, 2, 0U);
    mock_write_mem(nullptr, a6 + 6U, 2, 0U);
    mock_write_mem(nullptr, a6 + 54U, 1, 0x01U); // visible, mode 1
    mock_write_mem(nullptr, a6 + 16U, 2, 0U); // height = 0
    mock_write_mem(nullptr, a6 + 12U, 2, 100U); // Y = 100
    mock_write_mem(nullptr, a6 + 8U, 2, 200U);  // X = 200
    mock_write_mem(nullptr, a6 + 23U, 1, 0U);
    mock_write_mem(nullptr, a6 + 57U, 1, 0U); // no weapon part
    mock_write_mem(nullptr, a6 + 62U, 2, 0x8000U); // negative -> BMI taken
    mock_write_mem(nullptr, a6 + 24U, 2, 0x1000U); // base tile attribute
    mock_write_mem(nullptr, a6 + 26U, 4, 0x00020000U); // piece table pointer
    mock_write_mem(nullptr, a6 + 30U, 4, 0x00040000U);

    // Camera at 0x00FF1716: X=100, Y=50
    mock_write_mem(nullptr, 0x00FF1716U, 2, 100U);
    mock_write_mem(nullptr, 0x00FF171AU, 2, 50U);
    mock_write_mem(nullptr, 0x00FF16F0U, 1, 0U);

    // Piece table at 0x00020000:
    mock_write_mem(nullptr, 0x00020000U, 2, 2U); // offset = 2
    mock_write_mem(nullptr, 0x00020002U, 2, 0U); // 1 piece (DBF counter 0)
    mock_write_mem(nullptr, 0x00020004U, 2, 10U); // piece Y offset = 10
    mock_write_mem(nullptr, 0x00020006U, 2, 0x0123U); // piece tile = 0x0123
    mock_write_mem(nullptr, 0x00020008U, 2, 20U); // piece X offset = 20

    // VDP Tile size table at 0x00AD56 (referenced via PC rel in piece loop)
    mock_write_mem(nullptr, 0x00AD56U, 2, 0x0000U);

    // SAT buffer state
    mock_write_mem(nullptr, 0x00FF188AU, 2, 0U); // sprite count = 0
    mock_write_mem(nullptr, 0x00FF188CU, 2, 0U); // SAT offset = 0
    mock_write_mem(nullptr, 0x00FF1892U, 4, 0x00FF3000U);

    auto api = make_test_api();
    unsigned cur_pc = kStartPc;
    unsigned step_count = 0U;

    while (step_count < 120U) {
        step_count++;
        auto res = step_m68k_00a9a0(cur_pc, api, true);
        TEST_ASSERT(res == M68k9D70StepResult::InstructionStepped);
        if (cur_pc == kExitRtsPc) {
            break;
        }
        cur_pc = g_last_return_pc;
    }

    TEST_ASSERT(g_last_return_pc == 0x0000A1F8U);
    TEST_ASSERT(g_regs[kRegSP] == sp + 4U);
    TEST_ASSERT(g_regs[kRegD7] == 0x12345678U);

    // Verify SAT entry written at 0x00FF13CC
    const unsigned sat_y = mock_read_mem(nullptr, 0x00FF13CCU, 2);
    TEST_ASSERT(sat_y == 188U);

    // Verify updated SAT offset (8) and sprite count (1)
    const unsigned new_count = mock_read_mem(nullptr, 0x00FF188AU, 2);
    const unsigned new_offset = mock_read_mem(nullptr, 0x00FF188CU, 2);
    TEST_ASSERT(new_count == 1U);
    TEST_ASSERT(new_offset == 8U);

    const auto& m = get_00a9a0_step_metrics();
    TEST_ASSERT(m.target_entries == 1);
    TEST_ASSERT(m.completed_calls == 1);
    std::cout << "[PASS] test_full_render_piece_loop passed (stepped " << m.stepped_instructions << " instructions)\n";
}

void test_weapon_hooks() {
    g_regs.fill(0U); g_mem.clear();
    init_test_rom();
    reset_00a9a0_step_metrics();
    reset_00a9a0_state();

    const unsigned sp = 0x00FFFE00U;
    const unsigned a6 = 0x00FF2000U;
    g_regs[kRegSP] = sp;
    g_regs[kRegA6] = a6;

    mock_write_mem(nullptr, sp, 4, 0x0000A1F8U);

    // Entity setup with 57(a6) = 1 (weapon part attached)
    mock_write_mem(nullptr, a6 + 22U, 2, 0U);
    mock_write_mem(nullptr, a6 + 50U, 2, 0U);
    mock_write_mem(nullptr, a6 + 52U, 2, 0U);
    mock_write_mem(nullptr, a6 + 6U, 2, 0U);
    mock_write_mem(nullptr, a6 + 54U, 1, 0x01U); // visible, mode 1
    mock_write_mem(nullptr, a6 + 16U, 2, 0U);
    mock_write_mem(nullptr, a6 + 12U, 2, 100U);
    mock_write_mem(nullptr, a6 + 8U, 2, 200U);
    mock_write_mem(nullptr, a6 + 23U, 1, 0U);
    mock_write_mem(nullptr, a6 + 57U, 1, 0x01U); // weapon hook!
    mock_write_mem(nullptr, a6 + 62U, 2, 0x8000U);
    mock_write_mem(nullptr, a6 + 24U, 2, 0x1000U);
    mock_write_mem(nullptr, a6 + 26U, 4, 0x00020000U);
    mock_write_mem(nullptr, a6 + 30U, 4, 0x00040000U);

    mock_write_mem(nullptr, 0x00FF1716U, 2, 100U);
    mock_write_mem(nullptr, 0x00FF171AU, 2, 50U);
    mock_write_mem(nullptr, 0x00FF16F0U, 1, 0U);
    mock_write_mem(nullptr, 0x00FF188AU, 2, 0U);
    mock_write_mem(nullptr, 0x00FF188CU, 2, 0U);
    mock_write_mem(nullptr, 0x00FF1892U, 4, 0x00FF3000U);

    auto api = make_test_api();
    unsigned cur_pc = kStartPc;
    unsigned step_count = 0U;

    // Step until weapon hook at 0x00AD42
    while (step_count < 100U) {
        step_count++;
        auto res = step_m68k_00a9a0(cur_pc, api, true);
        TEST_ASSERT(res == M68k9D70StepResult::InstructionStepped);
        if (cur_pc == 0x00AD42U) {
            break;
        }
        cur_pc = g_last_return_pc;
    }

    TEST_ASSERT(cur_pc == 0x00AD42U);
    TEST_ASSERT(g_last_return_pc == 0x00B34CU); // calls child $00B34C!
    TEST_ASSERT(g_regs[kRegSP] == sp - 8U - 4U);
    TEST_ASSERT(mock_read_mem(nullptr, sp - 8U - 4U, 4) == 0x00AD46U);

    const auto& m = get_00a9a0_step_metrics();
    TEST_ASSERT(m.weapon_00b34c_calls == 1);
    std::cout << "[PASS] test_weapon_hooks passed (reached hook at step " << step_count << ")\n";
}

} // namespace

int main() {
    std::cout << "Running M68K 0x00A9A0 Cluster Unit Tests...\n";
    test_disabled_and_corrupt();
    test_fast_path_hidden();
    test_full_render_piece_loop();
    test_weapon_hooks();
    std::cout << "All M68K 0x00A9A0 Cluster Tests Passed Successfully.\n";
    return 0;
}
