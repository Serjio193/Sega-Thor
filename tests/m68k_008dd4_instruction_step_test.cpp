#include "tools/hybrid/m68k_008dd4_instruction_step.hpp"
#include "tools/hybrid/m68k_9d70_instruction_step.hpp"

#include <array>
#include <cassert>
#include <iostream>
#include <vector>

namespace {
using namespace oasis::hybrid;

std::array<std::uint8_t, 0x10000> g_ram{};
std::array<std::uint32_t, 18> g_regs{};
std::vector<std::uint16_t> g_fetches{};
std::size_t g_fetch_index{};
unsigned g_last_return_pc{};
int g_cycles_added{};

constexpr std::array<std::uint16_t, 39> kRomWords{{
    0x1039U, 0x00FFU, 0x183EU, 0x4A79U, 0x00FFU, 0x1AA4U, 0x6726U, 0x0C00U,
    0x0003U, 0x6404U, 0x5200U, 0x602EU, 0x103CU, 0x0003U, 0x0C79U, 0x0014U,
    0x00FFU, 0x199EU, 0x6520U, 0x0839U, 0x0000U, 0x00FFU, 0x199FU, 0x6616U,
    0x5200U, 0x6012U, 0x4A00U, 0x6714U, 0x0C00U, 0x0004U, 0x6404U, 0x5300U,
    0x6004U, 0x103CU, 0x0002U, 0x13C0U, 0x00FFU, 0x183EU, 0x4E75U
}};

int mock_peek(void*, unsigned addr) {
    if (addr >= 0x008DD4U && addr < 0x008DD4U + kRomWords.size() * 2U) {
        const auto offset = (addr - 0x008DD4U) / 2U;
        const auto byte_idx = (addr - 0x008DD4U) % 2U;
        const auto word = kRomWords[offset];
        return byte_idx == 0 ? (word >> 8U) & 0xFFU : word & 0xFFU;
    }
    // Caller: 0x008B46 bsr.w 0x028C -> 0x008DD4
    if (addr == 0x008B46U) return 0x61; if (addr == 0x008B47U) return 0x00;
    if (addr == 0x008B48U) return 0x02; if (addr == 0x008B49U) return 0x8C;

    if (addr >= 0xFF0000U && addr < 0x1000000U) return g_ram[addr - 0xFF0000U];
    return 0;
}

std::uint32_t mock_read(void* ctx, unsigned addr, int width) {
    std::uint32_t val = 0;
    for (int i = 0; i < width; ++i) {
        val = (val << 8U) | static_cast<std::uint32_t>(mock_peek(ctx, addr + static_cast<unsigned>(i)));
    }
    return val;
}

void mock_write(void*, unsigned addr, int width, std::uint32_t val) {
    if (addr >= 0xFF0000U && addr + static_cast<unsigned>(width) <= 0x1000000U) {
        for (int i = 0; i < width; ++i) {
            const int shift = (width - 1 - i) * 8;
            g_ram[addr - 0xFF0000U + static_cast<unsigned>(i)] =
                static_cast<std::uint8_t>((val >> shift) & 0xFFU);
        }
    }
}

std::uint32_t mock_read_reg(void*, unsigned reg) {
    return reg < g_regs.size() ? g_regs[reg] : 0U;
}

void mock_write_reg(void*, unsigned reg, std::uint32_t val) {
    if (reg < g_regs.size()) g_regs[reg] = val;
}

std::uint32_t mock_fetch16(void*) {
    if (g_fetch_index < g_fetches.size()) return g_fetches[g_fetch_index++];
    return 0U;
}

void mock_set_return_state(void*, unsigned pc, unsigned, unsigned) {
    g_last_return_pc = pc;
}

void mock_begin_instruction(void*, unsigned) {}
void mock_finish_instruction(void*, unsigned) {}
void mock_add_cycles(void*, int c) { g_cycles_added += c; }
int mock_word_not_taken(void*) { return 14; }
int mock_byte_not_taken(void*) { return 8; }
void mock_observe(void*, unsigned, unsigned) {}
void mock_skip_bus_refresh(void*) {}
int mock_scheduler_field(void*, unsigned) { return 0; }
unsigned mock_transfer_source_pc(void*) { return 0U; }

M68k9D70StepApi make_test_api() {
    return M68k9D70StepApi{
        nullptr, mock_read_reg, mock_write_reg, mock_peek, mock_read, mock_write,
        mock_fetch16, mock_set_return_state, mock_begin_instruction, mock_finish_instruction,
        mock_add_cycles, mock_word_not_taken, mock_byte_not_taken, mock_observe,
        mock_skip_bus_refresh, mock_scheduler_field, mock_transfer_source_pc
    };
}

void reset_environment() {
    g_ram.fill(0);
    g_regs.fill(0);
    g_fetches.clear();
    g_fetch_index = 0;
    g_last_return_pc = 0;
    g_cycles_added = 0;
    reset_008dd4_step_metrics();
}

void test_rejects_invalid_caller() {
    reset_environment();
    auto api = make_test_api();

    g_regs[15] = 0x00FF8000U; // SP
    mock_write(nullptr, 0x00FF8000U, 4, 0x00123456U);

    auto res = step_m68k_008dd4(0x008DD4U, api, true);
    assert(res == M68k9D70StepResult::NotHandled);
    const auto& m = get_008dd4_step_metrics();
    assert(m.rom_fallback_calls == 1);
    assert(m.caller_other_calls == 1);
    assert(m.native_calls == 0);
    std::cout << "test_rejects_invalid_caller passed\n";
}

void test_rejects_corrupt_rom() {
    reset_environment();
    auto api = make_test_api();

    auto corrupt_peek = [](void*, unsigned addr) -> int {
        if (addr == 0x008DD4U) return 0xFF;
        return mock_peek(nullptr, addr);
    };
    api.peek = corrupt_peek;
    g_regs[15] = 0x00FF8000U;
    mock_write(nullptr, 0x00FF8000U, 4, 0x008B4AU);

    auto res = step_m68k_008dd4(0x008DD4U, api, true);
    assert(res == M68k9D70StepResult::NotHandled);
    const auto& m = get_008dd4_step_metrics();
    assert(m.native_calls == 0);
    std::cout << "test_rejects_corrupt_rom passed\n";
}

void test_active_increment_path() {
    reset_environment();
    auto api = make_test_api();

    // Caller 0x008B46, return PC 0x008B4A
    g_regs[15] = 0x00FF8000U;
    mock_write(nullptr, 0x00FF8000U, 4, 0x008B4AU);

    // Initial RAM state:
    // $00FF183E = 1 (counter < 3)
    // $00FF1AA4 = 1 (spirit/mode active != 0)
    mock_write(nullptr, 0x00FF183EU, 1, 0x01U);
    mock_write(nullptr, 0x00FF1AA4U, 2, 0x0001U);

    // 0x008DD4: move.b $00FF183E, %d0
    g_fetches = {0x1039U, 0x00FFU, 0x183EU};
    g_fetch_index = 0;
    auto res1 = step_m68k_008dd4(0x008DD4U, api, true);
    assert(res1 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008dd4_resume_pc() == 0x008DDAU);
    assert((g_regs[0] & 0xFFU) == 0x01U);

    // 0x008DDA: tst.w $00FF1AA4
    g_fetches = {0x4A79U, 0x00FFU, 0x1AA4U};
    g_fetch_index = 0;
    auto res2 = step_m68k_008dd4(0x008DDAU, api, true);
    assert(res2 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008dd4_resume_pc() == 0x008DE0U);

    // 0x008DE0: beq.s 0x008E08 (not taken)
    g_fetches = {0x6726U};
    g_fetch_index = 0;
    auto res3 = step_m68k_008dd4(0x008DE0U, api, true);
    assert(res3 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008dd4_resume_pc() == 0x008DE2U);

    // 0x008DE2: cmpi.b #3, %d0 (1 < 3)
    g_fetches = {0x0C00U, 0x0003U};
    g_fetch_index = 0;
    auto res4 = step_m68k_008dd4(0x008DE2U, api, true);
    assert(res4 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008dd4_resume_pc() == 0x008DE6U);

    // 0x008DE6: bcc.s 0x008DEC (not taken)
    g_fetches = {0x6404U};
    g_fetch_index = 0;
    auto res5 = step_m68k_008dd4(0x008DE6U, api, true);
    assert(res5 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008dd4_resume_pc() == 0x008DE8U);

    // 0x008DE8: addq.b #1, %d0 -> d0 becomes 2
    g_fetches = {0x5200U};
    g_fetch_index = 0;
    auto res6 = step_m68k_008dd4(0x008DE8U, api, true);
    assert(res6 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008dd4_resume_pc() == 0x008DEAU);
    assert((g_regs[0] & 0xFFU) == 0x02U);

    // 0x008DEA: bra.s 0x008E1A
    g_fetches = {0x602EU};
    g_fetch_index = 0;
    auto res7 = step_m68k_008dd4(0x008DEAU, api, true);
    assert(res7 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008dd4_resume_pc() == 0x008E1AU);

    // 0x008E1A: move.b %d0, $00FF183E
    g_fetches = {0x13C0U, 0x00FFU, 0x183EU};
    g_fetch_index = 0;
    auto res8 = step_m68k_008dd4(0x008E1AU, api, true);
    assert(res8 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008dd4_resume_pc() == 0x008E20U);
    assert(mock_read(nullptr, 0x00FF183EU, 1) == 0x02U);

    // 0x008E20: rts
    g_fetches = {0x4E75U};
    g_fetch_index = 0;
    auto res9 = step_m68k_008dd4(0x008E20U, api, true);
    assert(res9 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008dd4_resume_pc() == 0x008B4AU);
    assert(g_regs[15] == 0x00FF8004U);

    const auto& m = get_008dd4_step_metrics();
    assert(m.target_entries == 1);
    assert(m.native_calls == 1);
    assert(m.completed_calls == 1);
    assert(m.stepped_instructions == 9);
    assert(m.update_exit_calls == 1);
    assert(m.zero_exit_calls == 0);
    std::cout << "test_active_increment_path passed\n";
}

void test_inactive_zero_exit_path() {
    reset_environment();
    auto api = make_test_api();

    g_regs[15] = 0x00FF8000U;
    mock_write(nullptr, 0x00FF8000U, 4, 0x008B4AU);

    // $00FF183E = 0
    // $00FF1AA4 = 0 (inactive)
    mock_write(nullptr, 0x00FF183EU, 1, 0x00U);
    mock_write(nullptr, 0x00FF1AA4U, 2, 0x0000U);

    // 0x008DD4: move.b $00FF183E, %d0
    g_fetches = {0x1039U, 0x00FFU, 0x183EU};
    g_fetch_index = 0;
    auto res1 = step_m68k_008dd4(0x008DD4U, api, true);
    assert(res1 == M68k9D70StepResult::InstructionStepped);

    // 0x008DDA: tst.w $00FF1AA4 (is 0)
    g_fetches = {0x4A79U, 0x00FFU, 0x1AA4U};
    g_fetch_index = 0;
    auto res2 = step_m68k_008dd4(0x008DDAU, api, true);
    assert(res2 == M68k9D70StepResult::InstructionStepped);

    // 0x008DE0: beq.s 0x008E08 (taken!)
    g_fetches = {0x6726U};
    g_fetch_index = 0;
    auto res3 = step_m68k_008dd4(0x008DE0U, api, true);
    assert(res3 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008dd4_resume_pc() == 0x008E08U);

    // 0x008E08: tst.b %d0 (is 0)
    g_fetches = {0x4A00U};
    g_fetch_index = 0;
    auto res4 = step_m68k_008dd4(0x008E08U, api, true);
    assert(res4 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008dd4_resume_pc() == 0x008E0AU);

    // 0x008E0A: beq.s 0x008E20 (taken to RTS!)
    g_fetches = {0x6714U};
    g_fetch_index = 0;
    auto res5 = step_m68k_008dd4(0x008E0AU, api, true);
    assert(res5 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008dd4_resume_pc() == 0x008E20U);

    // 0x008E20: rts
    g_fetches = {0x4E75U};
    g_fetch_index = 0;
    auto res6 = step_m68k_008dd4(0x008E20U, api, true);
    assert(res6 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008dd4_resume_pc() == 0x008B4AU);
    assert(g_regs[15] == 0x00FF8004U);

    const auto& m = get_008dd4_step_metrics();
    assert(m.target_entries == 1);
    assert(m.native_calls == 1);
    assert(m.completed_calls == 1);
    assert(m.stepped_instructions == 6);
    assert(m.zero_exit_calls == 1);
    assert(m.update_exit_calls == 0);
    std::cout << "test_inactive_zero_exit_path passed\n";
}

void test_interruption_and_reentrancy() {
    reset_environment();
    auto api = make_test_api();

    g_regs[15] = 0x00FF8000U;
    mock_write(nullptr, 0x00FF8000U, 4, 0x008B4AU);
    mock_write(nullptr, 0x00FF183EU, 1, 0x01U);
    mock_write(nullptr, 0x00FF1AA4U, 2, 0x0001U);

    g_fetches = {0x1039U, 0x00FFU, 0x183EU};
    g_fetch_index = 0;
    auto res1 = step_m68k_008dd4(0x008DD4U, api, true);
    assert(res1 == M68k9D70StepResult::InstructionStepped);

    // Interrupt
    auto res_irq = step_m68k_008dd4(0x000400U, api, true);
    assert(res_irq == M68k9D70StepResult::NotHandled);

    // Re-entrance while active is rejected as NotHandled
    auto res_reenter = step_m68k_008dd4(0x008DD4U, api, true);
    assert(res_reenter == M68k9D70StepResult::NotHandled);
    const auto& m = get_008dd4_step_metrics();
    assert(m.native_calls == 1);
    std::cout << "test_interruption_and_reentrancy passed\n";
}

} // namespace

int main() {
    test_rejects_invalid_caller();
    test_rejects_corrupt_rom();
    test_active_increment_path();
    test_inactive_zero_exit_path();
    test_interruption_and_reentrancy();
    std::cout << "All 008dd4 unit tests passed successfully!\n";
    return 0;
}
