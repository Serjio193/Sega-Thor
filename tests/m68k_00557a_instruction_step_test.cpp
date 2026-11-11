#include "tools/hybrid/m68k_00557a_instruction_step.hpp"
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

constexpr std::array<std::uint16_t, 123> kRomWords{{
    0x51F9U, 0x00FFU, 0x1986U, 0x4DF9U, 0x00FFU, 0x19E8U, 0x6100U, 0x00E8U,
    0x4DF9U, 0x00FFU, 0x19E8U, 0x4241U, 0x3401U, 0x122EU, 0x0048U, 0x142EU,
    0x0049U, 0x082EU, 0x0000U, 0x0017U, 0x6700U, 0x0004U, 0xC342U, 0x3D41U,
    0x0042U, 0x3D42U, 0x0044U, 0x1039U, 0x00FFU, 0x1993U, 0x6700U, 0x000EU,
    0x5300U, 0x13C0U, 0x00FFU, 0x1993U, 0x6000U, 0x002AU, 0x302EU, 0x008AU,
    0x6700U, 0x0022U, 0x0C40U, 0x0040U, 0x6200U, 0x001AU, 0xE248U, 0x0640U,
    0x000AU, 0x13C0U, 0x00FFU, 0x1993U, 0x203CU, 0x00A0U, 0x038CU, 0x4EB9U,
    0x0006U, 0x0004U, 0x1039U, 0x00FFU, 0x1994U, 0x6700U, 0x000EU, 0x5300U,
    0x13C0U, 0x00FFU, 0x1994U, 0x6000U, 0x0036U, 0x3039U, 0x00FFU, 0x1AA4U,
    0x6700U, 0x002CU, 0x0C40U, 0x0016U, 0x6700U, 0x0024U, 0x302EU, 0x008EU,
    0x0C40U, 0x0018U, 0x6200U, 0x0018U, 0x0640U, 0x000CU, 0x13C0U, 0x00FFU,
    0x1994U, 0x203CU, 0x00A0U, 0x04DDU, 0x4EB9U, 0x0006U, 0x0004U, 0x082EU,
    0x0007U, 0x006FU, 0x6600U, 0x001EU, 0x322EU, 0x0004U, 0x0C41U, 0x0000U,
    0x6700U, 0x0014U, 0x0C41U, 0x0002U, 0x6700U, 0x000CU, 0x0C41U, 0x0004U,
    0x6700U, 0x0004U, 0x4E75U, 0x41F9U, 0x00FFU, 0x1972U, 0x30EEU, 0x0008U,
    0x30AEU, 0x000CU, 0x4E75U
}};

int mock_peek(void*, unsigned addr) {
    if (addr >= 0x00557AU && addr < 0x00557AU + kRomWords.size() * 2U) {
        const auto offset = (addr - 0x00557AU) / 2U;
        const auto byte_idx = (addr - 0x00557AU) % 2U;
        const auto word = kRomWords[offset];
        return byte_idx == 0 ? (word >> 8U) & 0xFFU : word & 0xFFU;
    }
    // Caller: 0x008B2E bsr.w 0x00557A (opcode 0x6100, displacement 0xCA4A)
    if (addr == 0x008B2EU) return 0x61; if (addr == 0x008B2FU) return 0x00;
    if (addr == 0x008B30U) return 0xCA; if (addr == 0x008B31U) return 0x4A;

    // Subroutine 0x005670 entry: rts
    if (addr == 0x005670U) return 0x4E; if (addr == 0x005671U) return 0x75;

    // Subroutine 0x060004 entry: rts
    if (addr == 0x060004U) return 0x4E; if (addr == 0x060005U) return 0x75;

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
    reset_00557a_step_metrics();
}

void test_rejects_invalid_caller() {
    reset_environment();
    auto api = make_test_api();

    g_regs[15] = 0x00FF8000U;
    mock_write(nullptr, 0x00FF8000U, 4, 0x00123456U);

    auto res = step_m68k_00557a(0x00557AU, api, true);
    assert(res == M68k9D70StepResult::NotHandled);
    const auto& m = get_00557a_step_metrics();
    assert(m.rom_fallback_calls == 1);
    assert(m.caller_other_calls == 1);
    assert(m.native_calls == 0);
    std::cout << "test_rejects_invalid_caller passed\n";
}

void test_rejects_corrupt_rom() {
    reset_environment();
    auto api = make_test_api();

    auto corrupt_peek = [](void*, unsigned addr) -> int {
        if (addr == 0x00557AU) return 0xFF;
        return mock_peek(nullptr, addr);
    };
    api.peek = corrupt_peek;
    g_regs[15] = 0x00FF8000U;
    mock_write(nullptr, 0x00FF8000U, 4, 0x008B32U);

    auto res = step_m68k_00557a(0x00557AU, api, true);
    assert(res == M68k9D70StepResult::NotHandled);
    const auto& m = get_00557a_step_metrics();
    assert(m.native_calls == 0);
    std::cout << "test_rejects_corrupt_rom passed\n";
}

void test_executes_shadow_skip_path() {
    reset_environment();
    auto api = make_test_api();

    // Caller 0x008B2E, return PC 0x008B32
    g_regs[15] = 0x00FF8000U;
    mock_write(nullptr, 0x00FF8000U, 4, 0x008B32U);

    // Player entity slot at $00FF19E8
    // $FF1986 initially set
    mock_write(nullptr, 0x00FF1986U, 1, 0xFFU);
    // 72(a6) = 1, 73(a6) = 2
    mock_write(nullptr, 0x00FF19E8U + 72, 1, 0x01U);
    mock_write(nullptr, 0x00FF19E8U + 73, 1, 0x02U);
    // 23(a6) = 1 (facing flip set -> will exg d1, d2)
    mock_write(nullptr, 0x00FF19E8U + 23, 1, 0x01U);
    // $FF1993 = 0, 138(a6) = 0
    mock_write(nullptr, 0x00FF1993U, 1, 0x00U);
    mock_write(nullptr, 0x00FF19E8U + 138, 2, 0x0000U);
    // $FF1994 = 0, $FF1AA4 = 0
    mock_write(nullptr, 0x00FF1994U, 1, 0x00U);
    mock_write(nullptr, 0x00FF1AA4U, 2, 0x0000U);
    // 111(a6) = 0x80 (shadow disabled, bit 7 set)
    mock_write(nullptr, 0x00FF19E8U + 111, 1, 0x80U);

    // 0x00557A: sf $00FF1986
    g_fetches = {0x51F9U, 0x00FFU, 0x1986U}; g_fetch_index = 0;
    auto res1 = step_m68k_00557a(0x00557AU, api, true);
    assert(res1 == M68k9D70StepResult::InstructionStepped);
    assert(mock_read(nullptr, 0x00FF1986U, 1) == 0U);

    // 0x005580: lea $00FF19E8, %a6
    g_fetches = {0x4DF9U, 0x00FFU, 0x19E8U}; g_fetch_index = 0;
    auto res2 = step_m68k_00557a(0x005580U, api, true);
    assert(res2 == M68k9D70StepResult::InstructionStepped);
    assert(g_regs[14] == 0x00FF19E8U);

    // 0x005586: bsr.w 0x005670
    g_fetches = {0x6100U, 0x00E8U}; g_fetch_index = 0;
    auto res3 = step_m68k_00557a(0x005586U, api, true);
    assert(res3 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_00557a_resume_pc() == 0x00558AU);
    assert(g_last_return_pc == 0x005670U);
    assert(g_regs[15] == 0x00FF7FFCU); // SP decremented for child call

    // Child completes and returns to 0x00558A, restoring SP
    g_regs[15] = 0x00FF8000U;

    // 0x00558A: lea $00FF19E8, %a6
    g_fetches = {0x4DF9U, 0x00FFU, 0x19E8U}; g_fetch_index = 0;
    auto res4 = step_m68k_00557a(0x00558AU, api, true);
    assert(res4 == M68k9D70StepResult::InstructionStepped);

    // 0x005590: clr.w %d1
    g_fetches = {0x4241U}; g_fetch_index = 0;
    auto res5 = step_m68k_00557a(0x005590U, api, true);
    assert(res5 == M68k9D70StepResult::InstructionStepped);
    assert((g_regs[1] & 0xFFFFU) == 0U);

    // 0x005592: move.w %d1, %d2
    g_fetches = {0x3401U}; g_fetch_index = 0;
    auto res6 = step_m68k_00557a(0x005592U, api, true);
    assert(res6 == M68k9D70StepResult::InstructionStepped);

    // 0x005594: move.b 72(%a6), %d1
    g_fetches = {0x122EU, 0x0048U}; g_fetch_index = 0;
    auto res7 = step_m68k_00557a(0x005594U, api, true);
    assert(res7 == M68k9D70StepResult::InstructionStepped);
    assert((g_regs[1] & 0xFFU) == 0x01U);

    // 0x005598: move.b 73(%a6), %d2
    g_fetches = {0x142EU, 0x0049U}; g_fetch_index = 0;
    auto res8 = step_m68k_00557a(0x005598U, api, true);
    assert(res8 == M68k9D70StepResult::InstructionStepped);
    assert((g_regs[2] & 0xFFU) == 0x02U);

    // 0x00559C: btst #0, 23(%a6) (bit 0 is 1 -> Z=0)
    g_fetches = {0x082EU, 0x0000U, 0x0017U}; g_fetch_index = 0;
    auto res9 = step_m68k_00557a(0x00559CU, api, true);
    assert(res9 == M68k9D70StepResult::InstructionStepped);
    assert((g_regs[17] & 0x04U) == 0U); // Z == 0

    // 0x0055A2: beq.w 0x0055A8 (not taken)
    g_fetches = {0x6700U, 0x0004U}; g_fetch_index = 0;
    auto res10 = step_m68k_00557a(0x0055A2U, api, true);
    assert(res10 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_00557a_resume_pc() == 0x0055A6U);

    // 0x0055A6: exg %d1, %d2
    g_fetches = {0xC342U}; g_fetch_index = 0;
    auto res11 = step_m68k_00557a(0x0055A6U, api, true);
    assert(res11 == M68k9D70StepResult::InstructionStepped);
    assert((g_regs[1] & 0xFFU) == 0x02U);
    assert((g_regs[2] & 0xFFU) == 0x01U);

    // 0x0055A8: move.w %d1, 66(%a6)
    g_fetches = {0x3D41U, 0x0042U}; g_fetch_index = 0;
    auto res12 = step_m68k_00557a(0x0055A8U, api, true);
    assert(res12 == M68k9D70StepResult::InstructionStepped);
    assert(mock_read(nullptr, 0x00FF19E8U + 66, 2) == 0x0002U);

    // 0x0055AC: move.w %d2, 68(%a6)
    g_fetches = {0x3D42U, 0x0044U}; g_fetch_index = 0;
    auto res13 = step_m68k_00557a(0x0055ACU, api, true);
    assert(res13 == M68k9D70StepResult::InstructionStepped);
    assert(mock_read(nullptr, 0x00FF19E8U + 68, 2) == 0x0001U);

    // 0x0055B0: move.b $00FF1993, %d0 (is 0)
    g_fetches = {0x1039U, 0x00FFU, 0x1993U}; g_fetch_index = 0;
    auto res14 = step_m68k_00557a(0x0055B0U, api, true);
    assert(res14 == M68k9D70StepResult::InstructionStepped);

    // 0x0055B6: beq.w 0x0055C6 (taken)
    g_fetches = {0x6700U, 0x000EU}; g_fetch_index = 0;
    auto res15 = step_m68k_00557a(0x0055B6U, api, true);
    assert(res15 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_00557a_resume_pc() == 0x0055C6U);

    // 0x0055C6: move.w 138(%a6), %d0 (is 0)
    g_fetches = {0x302EU, 0x008AU}; g_fetch_index = 0;
    auto res16 = step_m68k_00557a(0x0055C6U, api, true);
    assert(res16 == M68k9D70StepResult::InstructionStepped);

    // 0x0055CA: beq.w 0x0055EE (taken)
    g_fetches = {0x6700U, 0x0022U}; g_fetch_index = 0;
    auto res17 = step_m68k_00557a(0x0055CAU, api, true);
    assert(res17 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_00557a_resume_pc() == 0x0055EEU);

    // 0x0055EE: move.b $00FF1994, %d0 (is 0)
    g_fetches = {0x1039U, 0x00FFU, 0x1994U}; g_fetch_index = 0;
    auto res18 = step_m68k_00557a(0x0055EEU, api, true);
    assert(res18 == M68k9D70StepResult::InstructionStepped);

    // 0x0055F4: beq.w 0x005604 (taken)
    g_fetches = {0x6700U, 0x000EU}; g_fetch_index = 0;
    auto res19 = step_m68k_00557a(0x0055F4U, api, true);
    assert(res19 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_00557a_resume_pc() == 0x005604U);

    // 0x005604: move.w $00FF1AA4, %d0 (is 0)
    g_fetches = {0x3039U, 0x00FFU, 0x1AA4U}; g_fetch_index = 0;
    auto res20 = step_m68k_00557a(0x005604U, api, true);
    assert(res20 == M68k9D70StepResult::InstructionStepped);

    // 0x00560A: beq.w 0x005638 (taken)
    g_fetches = {0x6700U, 0x002CU}; g_fetch_index = 0;
    auto res21 = step_m68k_00557a(0x00560AU, api, true);
    assert(res21 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_00557a_resume_pc() == 0x005638U);

    // 0x005638: btst #7, 111(%a6) (bit 7 is 1 -> Z=0)
    g_fetches = {0x082EU, 0x0007U, 0x006FU}; g_fetch_index = 0;
    auto res22 = step_m68k_00557a(0x005638U, api, true);
    assert(res22 == M68k9D70StepResult::InstructionStepped);

    // 0x00563E: bne.w 0x00565E (taken to RTS!)
    g_fetches = {0x6600U, 0x001EU}; g_fetch_index = 0;
    auto res23 = step_m68k_00557a(0x00563EU, api, true);
    assert(res23 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_00557a_resume_pc() == 0x00565EU);

    // 0x00565E: rts
    g_fetches = {0x4E75U}; g_fetch_index = 0;
    auto res24 = step_m68k_00557a(0x00565EU, api, true);
    assert(res24 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_00557a_resume_pc() == 0x008B32U);
    assert(g_regs[15] == 0x00FF8004U);

    const auto& m = get_00557a_step_metrics();
    assert(m.target_entries == 1);
    assert(m.native_calls == 1);
    assert(m.completed_calls == 1);
    assert(m.child_calls_005670 == 1);
    assert(m.shadow_skipped_calls == 1);
    assert(m.shadow_updated_calls == 0);
    std::cout << "test_executes_shadow_skip_path passed\n";
}

void test_interruption_and_reentrancy() {
    reset_environment();
    auto api = make_test_api();

    g_regs[15] = 0x00FF8000U;
    mock_write(nullptr, 0x00FF8000U, 4, 0x008B32U);

    g_fetches = {0x51F9U, 0x00FFU, 0x1986U}; g_fetch_index = 0;
    auto res1 = step_m68k_00557a(0x00557AU, api, true);
    assert(res1 == M68k9D70StepResult::InstructionStepped);

    // Interrupt
    auto res_irq = step_m68k_00557a(0x000400U, api, true);
    assert(res_irq == M68k9D70StepResult::NotHandled);

    // Re-entrance while active is rejected as NotHandled
    auto res_reenter = step_m68k_00557a(0x00557AU, api, true);
    assert(res_reenter == M68k9D70StepResult::NotHandled);
    const auto& m = get_00557a_step_metrics();
    assert(m.native_calls == 1);
    std::cout << "test_interruption_and_reentrancy passed\n";
}

} // namespace

int main() {
    test_rejects_invalid_caller();
    test_rejects_corrupt_rom();
    test_executes_shadow_skip_path();
    test_interruption_and_reentrancy();
    std::cout << "All 00557a unit tests passed successfully!\n";
    return 0;
}
