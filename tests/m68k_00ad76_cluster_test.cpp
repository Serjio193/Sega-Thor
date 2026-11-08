#include "tools/hybrid/m68k_00ad76_instruction_step.hpp"
#include "tools/hybrid/m68k_00ad76_step_ops.hpp"

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
using namespace oasis::hybrid::step_00ad76;

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
void mock_observe(void*, unsigned, unsigned) {}

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
    api.observe_instruction = mock_observe;
    return api;
}

void init_test_rom() {
    // 0x00AD76: 082E 0001 0036 (btst.b #1, 54(a6))
    g_mem[0x00AD76U] = 0x08U; g_mem[0x00AD77U] = 0x2EU;
    g_mem[0x00AD78U] = 0x00U; g_mem[0x00AD79U] = 0x01U;
    g_mem[0x00AD7AU] = 0x00U; g_mem[0x00AD7BU] = 0x36U;
}

void test_fast_path_hidden() {
    g_regs.fill(0U);
    g_mem.clear();
    init_test_rom();
    reset_00ad76_step_metrics();

    const unsigned sp = 0x00FFFE00U;
    const unsigned a6 = 0x00FF2000U;
    g_regs[kRegSP] = sp;
    g_regs[kRegA6] = a6;

    // Caller pushed return address 0x0000A1F8
    mock_write_mem(nullptr, sp, 4, 0x0000A1F8U);

    // Entity bit 1 of 54(a6) is 1 (hidden)
    mock_write_mem(nullptr, a6 + 54U, 1, 0x02U);

    auto api = make_test_api();

    auto res = step_m68k_00ad76(0x00AD76U, api, true);
    TEST_ASSERT(res == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(g_last_return_pc == 0x00AD7CU);

    res = step_m68k_00ad76(0x00AD7CU, api, true);
    TEST_ASSERT(res == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(g_last_return_pc == kExitRtsPc);

    res = step_m68k_00ad76(kExitRtsPc, api, true);
    TEST_ASSERT(res == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(g_last_return_pc == 0x0000A1F8U);
    TEST_ASSERT(g_regs[kRegSP] == sp + 4U);

    const auto& m = get_00ad76_step_metrics();
    TEST_ASSERT(m.target_entries == 1);
    TEST_ASSERT(m.fast_path_hidden_rts == 1);
    TEST_ASSERT(m.completed_calls == 1);
    std::cout << "[PASS] test_fast_path_hidden passed\n";
}

void test_weapon_hook_fallback() {
    g_regs.fill(0U);
    g_mem.clear();
    init_test_rom();
    reset_00ad76_step_metrics();

    const unsigned sp = 0x00FFFE00U;
    const unsigned a6 = 0x00FF2000U;
    g_regs[kRegSP] = sp;
    g_regs[kRegA6] = a6;
    mock_write_mem(nullptr, sp, 4, 0x0000A1F8U);

    // Entity 57(a6) != 0 (attached weapon part)
    mock_write_mem(nullptr, a6 + 57U, 1, 0x01U);

    auto api = make_test_api();
    auto res = step_m68k_00ad76(0x00AD76U, api, true);
    TEST_ASSERT(res == M68k9D70StepResult::NotHandled);

    const auto& m = get_00ad76_step_metrics();
    TEST_ASSERT(m.target_entries == 1);
    TEST_ASSERT(m.child_fallback_calls == 1);
    TEST_ASSERT(m.rom_calls == 1);
    std::cout << "[PASS] test_weapon_hook_fallback passed\n";
}

void test_full_render_piece_loop() {
    g_regs.fill(0U);
    g_mem.clear();
    init_test_rom();
    reset_00ad76_step_metrics();

    const unsigned sp = 0x00FFFE00U;
    const unsigned a6 = 0x00FF2000U;
    g_regs[kRegSP] = sp;
    g_regs[kRegA6] = a6;
    g_regs[kRegD7] = 0x12345678U; // test register save/restore

    mock_write_mem(nullptr, sp, 4, 0x0000A1F8U); // caller return address

    // Entity setup
    mock_write_mem(nullptr, a6 + 22U, 2, 0U); // direction/anim = 0
    mock_write_mem(nullptr, a6 + 50U, 2, 0U);
    mock_write_mem(nullptr, a6 + 52U, 2, 0U);
    mock_write_mem(nullptr, a6 + 6U, 2, 0U);  // 6(a6) = 0
    mock_write_mem(nullptr, a6 + 54U, 1, 0U); // visible, mode 0
    mock_write_mem(nullptr, a6 + 16U, 2, 0U); // height = 0
    mock_write_mem(nullptr, a6 + 12U, 2, 100U); // Y = 100
    mock_write_mem(nullptr, a6 + 8U, 2, 200U);  // X = 200
    mock_write_mem(nullptr, a6 + 23U, 1, 0U);
    mock_write_mem(nullptr, a6 + 57U, 1, 0U); // no weapon part
    mock_write_mem(nullptr, a6 + 62U, 2, 0x8000U); // negative -> BMI taken to 0x00AF02
    mock_write_mem(nullptr, a6 + 24U, 2, 0x1000U); // base tile attribute
    mock_write_mem(nullptr, a6 + 26U, 4, 0x00020000U); // piece table pointer

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

    // SAT buffer state
    mock_write_mem(nullptr, 0x00FF188AU, 2, 0U); // sprite count = 0
    mock_write_mem(nullptr, 0x00FF188CU, 2, 0U); // SAT offset = 0

    auto api = make_test_api();
    unsigned cur_pc = 0x00AD76U;
    unsigned step_count = 0U;

    while (step_count < 100U) {
        step_count++;
        auto res = step_m68k_00ad76(cur_pc, api, true);
        TEST_ASSERT(res == M68k9D70StepResult::InstructionStepped);
        if (cur_pc == kExitRtsPc) {
            break;
        }
        cur_pc = g_last_return_pc;
    }

    TEST_ASSERT(g_last_return_pc == 0x0000A1F8U);
    TEST_ASSERT(g_regs[kRegSP] == sp + 4U);
    TEST_ASSERT(g_regs[kRegD7] == 0x12345678U); // restored!

    // Verify SAT entry written at 0x00FF13CC:
    // Y: 100 - 50 + 128 - 0 + 10 = 188 (0x00BC)
    const unsigned sat_y = mock_read_mem(nullptr, 0x00FF13CCU, 2);
    TEST_ASSERT(sat_y == 188U);

    // Link: D2 (0) + 1 = 1
    const unsigned sat_link = mock_read_mem(nullptr, 0x00FF13CEU, 2);
    TEST_ASSERT(sat_link == 1U);

    // Tile: 0x0123 + 0x1000 = 0x1123
    const unsigned sat_tile = mock_read_mem(nullptr, 0x00FF13D0U, 2);
    TEST_ASSERT(sat_tile == 0x1123U);

    // X: 200 - 100 + 128 + 20 = 248 (0x00F8)
    const unsigned sat_x = mock_read_mem(nullptr, 0x00FF13D2U, 2);
    TEST_ASSERT(sat_x == 248U);

    // Verify updated SAT offset (8) and sprite count (1)
    const unsigned new_count = mock_read_mem(nullptr, 0x00FF188AU, 2);
    const unsigned new_offset = mock_read_mem(nullptr, 0x00FF188CU, 2);
    TEST_ASSERT(new_count == 1U);
    TEST_ASSERT(new_offset == 8U);

    const auto& m = get_00ad76_step_metrics();
    TEST_ASSERT(m.target_entries == 1);
    TEST_ASSERT(m.full_render_calls == 1);
    TEST_ASSERT(m.piece_loop_steps == 1);
    TEST_ASSERT(m.completed_calls == 1);
    std::cout << "[PASS] test_full_render_piece_loop passed (stepped " << m.stepped_instructions << " instructions)\n";
}

} // namespace

int main() {
    std::cout << "Running M68K 0x00AD76 Cluster Unit Tests...\n";
    test_fast_path_hidden();
    test_weapon_hook_fallback();
    test_full_render_piece_loop();
    std::cout << "All M68K 0x00AD76 Cluster Tests Passed Successfully.\n";
    return 0;
}
