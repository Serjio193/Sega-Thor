#include "tools/hybrid/m68k_008b8e_instruction_step.hpp"
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

constexpr std::array<std::uint16_t, 15> kRomWords{{
    0x1039U, 0x00FFU, 0x1864U, // 0x008B8E: move.b $00FF1864, %d0
    0x6700U, 0x0014U,          // 0x008B94: beq.w  0x008BAA
    0x08B9U, 0x0007U, 0x00FFU, 0x1864U, // 0x008B98: bclr #7, $00FF1864
    0x6604U,                   // 0x008BA0: bne.s  0x008BA6
    0x6000U, 0xC768U,          // 0x008BA2: bra.w  0x00530C
    0x6000U, 0xF11EU,          // 0x008BA6: bra.w  0x007CC6
    0x4E75U                    // 0x008BAA: rts
}};

int mock_peek(void*, unsigned addr) {
    if (addr >= 0x008B8EU && addr < 0x008B8EU + kRomWords.size() * 2U) {
        const auto offset = (addr - 0x008B8EU) / 2U;
        const auto byte_idx = (addr - 0x008B8EU) % 2U;
        const auto word = kRomWords[offset];
        return byte_idx == 0 ? (word >> 8U) & 0xFFU : word & 0xFFU;
    }
    // Caller: 0x008B26 bsr.w 0x0066 -> 0x008B8E
    if (addr == 0x008B26U) return 0x61; if (addr == 0x008B27U) return 0x00;
    if (addr == 0x008B28U) return 0x00; if (addr == 0x008B29U) return 0x66;

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
int mock_word_not_taken(void*) { return 12; }
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
    reset_008b8e_step_metrics();
}

void test_rejects_invalid_caller() {
    reset_environment();
    auto api = make_test_api();

    g_regs[15] = 0x00FF8000U; // SP
    mock_write(nullptr, 0x00FF8000U, 4, 0x00123456U);

    auto res = step_m68k_008b8e(0x008B8EU, api, true);
    assert(res == M68k9D70StepResult::NotHandled);
    const auto& m = get_008b8e_step_metrics();
    assert(m.rom_fallback_calls == 1);
    assert(m.caller_other_calls == 1);
    assert(m.native_calls == 0);
    std::cout << "test_rejects_invalid_caller passed\n";
}

void test_rejects_odd_sp() {
    reset_environment();
    auto api = make_test_api();

    g_regs[15] = 0x00FF8001U; // Odd SP
    mock_write(nullptr, 0x00FF8000U, 4, 0x008B2AU);

    auto res = step_m68k_008b8e(0x008B8EU, api, true);
    assert(res == M68k9D70StepResult::NotHandled);
    const auto& m = get_008b8e_step_metrics();
    assert(m.native_calls == 0);
    std::cout << "test_rejects_odd_sp passed\n";
}

void test_rejects_corrupt_rom() {
    reset_environment();
    auto api = make_test_api();

    auto corrupt_peek = [](void*, unsigned addr) -> int {
        if (addr == 0x008B8EU) return 0xFF;
        return mock_peek(nullptr, addr);
    };
    api.peek = corrupt_peek;
    g_regs[15] = 0x00FF8000U;
    mock_write(nullptr, 0x00FF8000U, 4, 0x008B2AU);

    auto res = step_m68k_008b8e(0x008B8EU, api, true);
    assert(res == M68k9D70StepResult::NotHandled);
    const auto& m = get_008b8e_step_metrics();
    assert(m.native_calls == 0);
    std::cout << "test_rejects_corrupt_rom passed\n";
}

void test_executes_zero_exit_path() {
    reset_environment();
    auto api = make_test_api();

    // Caller 0x008B26, return PC 0x008B2A
    g_regs[15] = 0x00FF8000U;
    mock_write(nullptr, 0x00FF8000U, 4, 0x008B2AU);

    // Initial RAM state: $00FF1864 = 0
    mock_write(nullptr, 0x00FF1864U, 1, 0x00U);

    // 0x008B8E: move.b $00FF1864, %d0
    g_fetches = {0x1039U, 0x00FFU, 0x1864U};
    g_fetch_index = 0;
    auto res1 = step_m68k_008b8e(0x008B8EU, api, true);
    assert(res1 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008b8e_resume_pc() == 0x008B94U);
    assert((g_regs[0] & 0xFFU) == 0x00U);

    // 0x008B94: beq.w 0x008BAA (taken)
    g_fetches = {0x6700U, 0x0014U};
    g_fetch_index = 0;
    auto res2 = step_m68k_008b8e(0x008B94U, api, true);
    assert(res2 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008b8e_resume_pc() == 0x008BAAU);

    // 0x008BAA: rts
    g_fetches = {0x4E75U};
    g_fetch_index = 0;
    auto res3 = step_m68k_008b8e(0x008BAAU, api, true);
    assert(res3 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008b8e_resume_pc() == 0x008B2AU);
    assert(g_regs[15] == 0x00FF8004U);

    const auto& m = get_008b8e_step_metrics();
    assert(m.target_entries == 1);
    assert(m.native_calls == 1);
    assert(m.completed_calls == 1);
    assert(m.zero_exit_calls == 1);
    assert(m.stepped_instructions == 3);
    std::cout << "test_executes_zero_exit_path passed\n";
}

void test_executes_bra_530c_path() {
    reset_environment();
    auto api = make_test_api();

    g_regs[15] = 0x00FF8000U;
    mock_write(nullptr, 0x00FF8000U, 4, 0x008B2AU);

    // $00FF1864 = 0x1A (non-zero, bit 7 is 0)
    mock_write(nullptr, 0x00FF1864U, 1, 0x1AU);

    // 0x008B8E: move.b $00FF1864, %d0
    g_fetches = {0x1039U, 0x00FFU, 0x1864U};
    g_fetch_index = 0;
    auto res1 = step_m68k_008b8e(0x008B8EU, api, true);
    assert(res1 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008b8e_resume_pc() == 0x008B94U);
    assert((g_regs[0] & 0xFFU) == 0x1AU);

    // 0x008B94: beq.w 0x008BAA (not taken)
    g_fetches = {0x6700U, 0x0014U};
    g_fetch_index = 0;
    auto res2 = step_m68k_008b8e(0x008B94U, api, true);
    assert(res2 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008b8e_resume_pc() == 0x008B98U);

    // 0x008B98: bclr #7, $00FF1864
    g_fetches = {0x08B9U, 0x0007U, 0x00FFU, 0x1864U};
    g_fetch_index = 0;
    auto res3 = step_m68k_008b8e(0x008B98U, api, true);
    assert(res3 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008b8e_resume_pc() == 0x008BA0U);

    // 0x008BA0: bne.s 0x008BA6 (not taken because bit 7 was 0 -> Z=1)
    g_fetches = {0x6604U};
    g_fetch_index = 0;
    auto res4 = step_m68k_008b8e(0x008BA0U, api, true);
    assert(res4 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008b8e_resume_pc() == 0x008BA2U);

    // 0x008BA2: bra.w 0x00530C
    g_fetches = {0x6000U, 0xC768U};
    g_fetch_index = 0;
    auto res5 = step_m68k_008b8e(0x008BA2U, api, true);
    assert(res5 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008b8e_resume_pc() == 0x00530CU);

    const auto& m = get_008b8e_step_metrics();
    assert(m.target_entries == 1);
    assert(m.native_calls == 1);
    assert(m.completed_calls == 1);
    assert(m.tail_bra_530c_calls == 1);
    assert(m.stepped_instructions == 5);
    std::cout << "test_executes_bra_530c_path passed\n";
}

void test_executes_bra_7cc6_path() {
    reset_environment();
    auto api = make_test_api();

    g_regs[15] = 0x00FF8000U;
    mock_write(nullptr, 0x00FF8000U, 4, 0x008B2AU);

    // $00FF1864 = 0x85 (non-zero, bit 7 is 1)
    mock_write(nullptr, 0x00FF1864U, 1, 0x85U);

    // 0x008B8E: move.b $00FF1864, %d0
    g_fetches = {0x1039U, 0x00FFU, 0x1864U};
    g_fetch_index = 0;
    auto res1 = step_m68k_008b8e(0x008B8EU, api, true);
    assert(res1 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008b8e_resume_pc() == 0x008B94U);
    assert((g_regs[0] & 0xFFU) == 0x85U);

    // 0x008B94: beq.w 0x008BAA (not taken)
    g_fetches = {0x6700U, 0x0014U};
    g_fetch_index = 0;
    auto res2 = step_m68k_008b8e(0x008B94U, api, true);
    assert(res2 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008b8e_resume_pc() == 0x008B98U);

    // 0x008B98: bclr #7, $00FF1864
    g_fetches = {0x08B9U, 0x0007U, 0x00FFU, 0x1864U};
    g_fetch_index = 0;
    auto res3 = step_m68k_008b8e(0x008B98U, api, true);
    assert(res3 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008b8e_resume_pc() == 0x008BA0U);
    // Bit 7 should now be cleared in RAM: 0x85 & ~0x80 = 0x05
    assert(mock_peek(nullptr, 0x00FF1864U) == 0x05);

    // 0x008BA0: bne.s 0x008BA6 (taken because bit 7 was 1 -> Z=0)
    g_fetches = {0x6604U};
    g_fetch_index = 0;
    auto res4 = step_m68k_008b8e(0x008BA0U, api, true);
    assert(res4 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008b8e_resume_pc() == 0x008BA6U);

    // 0x008BA6: bra.w 0x007CC6
    g_fetches = {0x6000U, 0xF11EU};
    g_fetch_index = 0;
    auto res5 = step_m68k_008b8e(0x008BA6U, api, true);
    assert(res5 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008b8e_resume_pc() == 0x007CC6U);

    const auto& m = get_008b8e_step_metrics();
    assert(m.target_entries == 1);
    assert(m.native_calls == 1);
    assert(m.completed_calls == 1);
    assert(m.tail_bra_7cc6_calls == 1);
    assert(m.stepped_instructions == 5);
    std::cout << "test_executes_bra_7cc6_path passed\n";
}

void test_interruption_and_reentrancy() {
    reset_environment();
    auto api = make_test_api();

    g_regs[15] = 0x00FF8000U;
    mock_write(nullptr, 0x00FF8000U, 4, 0x008B2AU);
    mock_write(nullptr, 0x00FF1864U, 1, 0x00U);

    // 0x008B8E: move.b $00FF1864, %d0
    g_fetches = {0x1039U, 0x00FFU, 0x1864U};
    g_fetch_index = 0;
    auto res1 = step_m68k_008b8e(0x008B8EU, api, true);
    assert(res1 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008b8e_resume_pc() == 0x008B94U);

    // Interruption
    auto res_irq = step_m68k_008b8e(0x000400U, api, true);
    assert(res_irq == M68k9D70StepResult::NotHandled);

    // Resume at 0x008B94: beq.w 0x008BAA
    g_fetches = {0x6700U, 0x0014U};
    g_fetch_index = 0;
    auto res2 = step_m68k_008b8e(0x008B94U, api, true);
    assert(res2 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008b8e_resume_pc() == 0x008BAAU);

    // 0x008BAA: rts
    g_fetches = {0x4E75U};
    g_fetch_index = 0;
    auto res3 = step_m68k_008b8e(0x008BAAU, api, true);
    assert(res3 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008b8e_resume_pc() == 0x008B2AU);

    std::cout << "test_interruption_and_reentrancy passed\n";
}

} // namespace

int main() {
    test_rejects_invalid_caller();
    test_rejects_odd_sp();
    test_rejects_corrupt_rom();
    test_executes_zero_exit_path();
    test_executes_bra_530c_path();
    test_executes_bra_7cc6_path();
    test_interruption_and_reentrancy();
    std::cout << "All 008B8E tests passed cleanly!\n";
    return 0;
}
