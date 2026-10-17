#include "tools/hybrid/m68k_008bac_instruction_step.hpp"
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

constexpr std::array<std::uint16_t, 87> kRomWords{{
    0x1039U, 0x00FFU, 0x165EU, 0x4600U, 0x0200U, 0x0070U, 0x6600U, 0x000EU,
    0x50F9U, 0x00FFU, 0x19A7U, 0x4279U, 0x00FFU, 0x16EAU, 0x4A39U, 0x00FFU,
    0x1987U, 0x6600U, 0x0064U, 0x0839U, 0x0002U, 0x00FFU, 0x17B8U, 0x6600U,
    0x0058U, 0x4A39U, 0x00FFU, 0x1983U, 0x6600U, 0x004EU, 0x4A39U, 0x00FFU,
    0x1982U, 0x6600U, 0x0044U, 0x51F9U, 0x00FFU, 0x185EU, 0x0839U, 0x0007U,
    0x00FFU, 0x165FU, 0x6600U, 0xAF3CU, 0x3039U, 0x00FFU, 0x1660U, 0x0800U,
    0x0002U, 0x6600U, 0x003EU, 0x0C79U, 0x0002U, 0x00FFU, 0x19ECU, 0x6218U,
    0x4A79U, 0x00FFU, 0x1A6EU, 0x6610U, 0x0800U, 0x0001U, 0x6600U, 0x000CU,
    0x0800U, 0x0000U, 0x6600U, 0x0010U, 0x4E75U, 0x13FCU, 0x0001U, 0x00FFU,
    0x185EU, 0x6000U, 0xAEFEU, 0x13FCU, 0x0002U, 0x00FFU, 0x185EU, 0x6000U,
    0xAEF2U, 0x13FCU, 0x0003U, 0x00FFU, 0x185EU, 0x6000U, 0xAEE6U
}};

int mock_peek(void*, unsigned addr) {
    if (addr >= 0x008BACU && addr < 0x008BACU + kRomWords.size() * 2U) {
        const auto offset = (addr - 0x008BACU) / 2U;
        const auto byte_idx = (addr - 0x008BACU) % 2U;
        const auto word = kRomWords[offset];
        return byte_idx == 0 ? (word >> 8U) & 0xFFU : word & 0xFFU;
    }
    // Caller: 0x008B22 bsr.w 0x0088 -> 0x008BAC
    if (addr == 0x008B22U) return 0x61; if (addr == 0x008B23U) return 0x00;
    if (addr == 0x008B24U) return 0x00; if (addr == 0x008B25U) return 0x88;

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
    reset_008bac_step_metrics();
}

void test_rejects_invalid_caller() {
    reset_environment();
    auto api = make_test_api();

    g_regs[15] = 0x00FF8000U;
    mock_write(nullptr, 0x00FF8000U, 4, 0x00123456U);

    auto res = step_m68k_008bac(0x008BACU, api, true);
    assert(res == M68k9D70StepResult::NotHandled);
    const auto& m = get_008bac_step_metrics();
    assert(m.rom_fallback_calls == 1);
    assert(m.caller_other_calls == 1);
    assert(m.native_calls == 0);
    std::cout << "test_rejects_invalid_caller passed\n";
}

void test_rejects_odd_sp() {
    reset_environment();
    auto api = make_test_api();

    g_regs[15] = 0x00FF8001U;
    mock_write(nullptr, 0x00FF8000U, 4, 0x008B26U);

    auto res = step_m68k_008bac(0x008BACU, api, true);
    assert(res == M68k9D70StepResult::NotHandled);
    const auto& m = get_008bac_step_metrics();
    assert(m.native_calls == 0);
    std::cout << "test_rejects_odd_sp passed\n";
}

void test_rejects_corrupt_rom() {
    reset_environment();
    auto api = make_test_api();

    auto corrupt_peek = [](void*, unsigned addr) -> int {
        if (addr == 0x008BACU) return 0xFF;
        return mock_peek(nullptr, addr);
    };
    api.peek = corrupt_peek;
    g_regs[15] = 0x00FF8000U;
    mock_write(nullptr, 0x00FF8000U, 4, 0x008B26U);

    auto res = step_m68k_008bac(0x008BACU, api, true);
    assert(res == M68k9D70StepResult::NotHandled);
    const auto& m = get_008bac_step_metrics();
    assert(m.native_calls == 0);
    std::cout << "test_rejects_corrupt_rom passed\n";
}

void test_executes_rts_path() {
    reset_environment();
    auto api = make_test_api();

    // Caller 0x008B22, return PC 0x008B26
    g_regs[15] = 0x00FF8000U;
    mock_write(nullptr, 0x00FF8000U, 4, 0x008B26U);

    // Initial RAM state: $FF165E = 0xFF (not -> 0, andi -> 0, bne not taken -> writes FF19A7 and clr FF16EA)
    mock_write(nullptr, 0x00FF165EU, 1, 0xFFU);

    // 0x008BAC: move.b $00FF165E, %d0
    g_fetches = {0x1039U, 0x00FFU, 0x165EU}; g_fetch_index = 0;
    auto res1 = step_m68k_008bac(0x008BACU, api, true);
    assert(res1 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008bac_resume_pc() == 0x008BB2U);

    // 0x008BB2: not.b %d0
    g_fetches = {0x4600U}; g_fetch_index = 0;
    auto res2 = step_m68k_008bac(0x008BB2U, api, true);
    assert(res2 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008bac_resume_pc() == 0x008BB4U);

    // 0x008BB4: andi.b #0x70, %d0
    g_fetches = {0x0200U, 0x0070U}; g_fetch_index = 0;
    auto res3 = step_m68k_008bac(0x008BB4U, api, true);
    assert(res3 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008bac_resume_pc() == 0x008BB8U);

    // 0x008BB8: bne.w 0x008BC8 (not taken)
    g_fetches = {0x6600U, 0x000EU}; g_fetch_index = 0;
    auto res4 = step_m68k_008bac(0x008BB8U, api, true);
    assert(res4 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008bac_resume_pc() == 0x008BBCU);

    // 0x008BBC: st $00FF19A7
    g_fetches = {0x50F9U, 0x00FFU, 0x19A7U}; g_fetch_index = 0;
    auto res5 = step_m68k_008bac(0x008BBCU, api, true);
    assert(res5 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008bac_resume_pc() == 0x008BC2U);
    assert(mock_peek(nullptr, 0x00FF19A7U) == 0xFF);

    // 0x008BC2: clr.w $00FF16EA
    g_fetches = {0x4279U, 0x00FFU, 0x16EAU}; g_fetch_index = 0;
    auto res6 = step_m68k_008bac(0x008BC2U, api, true);
    assert(res6 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008bac_resume_pc() == 0x008BC8U);

    // 0x008BC8: tst.b $00FF1987
    g_fetches = {0x4A39U, 0x00FFU, 0x1987U}; g_fetch_index = 0;
    auto res7 = step_m68k_008bac(0x008BC8U, api, true);
    assert(res7 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008bac_resume_pc() == 0x008BCEU);

    // 0x008BCE: bne.w 0x008C34 (not taken)
    g_fetches = {0x6600U, 0x0064U}; g_fetch_index = 0;
    auto res8 = step_m68k_008bac(0x008BCEU, api, true);
    assert(res8 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008bac_resume_pc() == 0x008BD2U);

    // 0x008BD2: btst #2, $00FF17B8
    g_fetches = {0x0839U, 0x0002U, 0x00FFU, 0x17B8U}; g_fetch_index = 0;
    auto res9 = step_m68k_008bac(0x008BD2U, api, true);
    assert(res9 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008bac_resume_pc() == 0x008BDAU);

    // 0x008BDA: bne.w 0x008C34 (not taken)
    g_fetches = {0x6600U, 0x0058U}; g_fetch_index = 0;
    auto res10 = step_m68k_008bac(0x008BDAU, api, true);
    assert(res10 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008bac_resume_pc() == 0x008BDEU);

    // 0x008BDE: tst.b $00FF1983
    g_fetches = {0x4A39U, 0x00FFU, 0x1983U}; g_fetch_index = 0;
    auto res11 = step_m68k_008bac(0x008BDEU, api, true);
    assert(res11 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008bac_resume_pc() == 0x008BE4U);

    // 0x008BE4: bne.w 0x008C34 (not taken)
    g_fetches = {0x6600U, 0x004EU}; g_fetch_index = 0;
    auto res12 = step_m68k_008bac(0x008BE4U, api, true);
    assert(res12 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008bac_resume_pc() == 0x008BE8U);

    // 0x008BE8: tst.b $00FF1982
    g_fetches = {0x4A39U, 0x00FFU, 0x1982U}; g_fetch_index = 0;
    auto res13 = step_m68k_008bac(0x008BE8U, api, true);
    assert(res13 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008bac_resume_pc() == 0x008BEEU);

    // 0x008BEE: bne.w 0x008C34 (not taken)
    g_fetches = {0x6600U, 0x0044U}; g_fetch_index = 0;
    auto res14 = step_m68k_008bac(0x008BEEU, api, true);
    assert(res14 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008bac_resume_pc() == 0x008BF2U);

    // 0x008BF2: sf $00FF185E
    g_fetches = {0x51F9U, 0x00FFU, 0x185EU}; g_fetch_index = 0;
    auto res15 = step_m68k_008bac(0x008BF2U, api, true);
    assert(res15 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008bac_resume_pc() == 0x008BF8U);

    // 0x008BF8: btst #7, $00FF165F
    g_fetches = {0x0839U, 0x0007U, 0x00FFU, 0x165FU}; g_fetch_index = 0;
    auto res16 = step_m68k_008bac(0x008BF8U, api, true);
    assert(res16 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008bac_resume_pc() == 0x008C00U);

    // 0x008C00: bne.w 0x003B3E (not taken)
    g_fetches = {0x6600U, 0xAF3CU}; g_fetch_index = 0;
    auto res17 = step_m68k_008bac(0x008C00U, api, true);
    assert(res17 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008bac_resume_pc() == 0x008C04U);

    // 0x008C04: move.w $00FF1660, %d0
    g_fetches = {0x3039U, 0x00FFU, 0x1660U}; g_fetch_index = 0;
    auto res18 = step_m68k_008bac(0x008C04U, api, true);
    assert(res18 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008bac_resume_pc() == 0x008C0AU);

    // 0x008C0A: btst #2, %d0
    g_fetches = {0x0800U, 0x0002U}; g_fetch_index = 0;
    auto res19 = step_m68k_008bac(0x008C0AU, api, true);
    assert(res19 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008bac_resume_pc() == 0x008C0EU);

    // 0x008C0E: bne.w 0x008C4E (not taken)
    g_fetches = {0x6600U, 0x003EU}; g_fetch_index = 0;
    auto res20 = step_m68k_008bac(0x008C0EU, api, true);
    assert(res20 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008bac_resume_pc() == 0x008C12U);

    // 0x008C12: cmpi.w #2, $00FF19EC
    g_fetches = {0x0C79U, 0x0002U, 0x00FFU, 0x19ECU}; g_fetch_index = 0;
    auto res21 = step_m68k_008bac(0x008C12U, api, true);
    assert(res21 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008bac_resume_pc() == 0x008C1AU);

    // 0x008C1A: bhi.s 0x008C34 (not taken)
    g_fetches = {0x6218U}; g_fetch_index = 0;
    auto res22 = step_m68k_008bac(0x008C1AU, api, true);
    assert(res22 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008bac_resume_pc() == 0x008C1CU);

    // 0x008C1C: tst.w $00FF1A6E
    g_fetches = {0x4A79U, 0x00FFU, 0x1A6EU}; g_fetch_index = 0;
    auto res23 = step_m68k_008bac(0x008C1CU, api, true);
    assert(res23 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008bac_resume_pc() == 0x008C22U);

    // 0x008C22: bne.s 0x008C34 (not taken)
    g_fetches = {0x6610U}; g_fetch_index = 0;
    auto res24 = step_m68k_008bac(0x008C22U, api, true);
    assert(res24 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008bac_resume_pc() == 0x008C24U);

    // 0x008C24: btst #1, %d0
    g_fetches = {0x0800U, 0x0001U}; g_fetch_index = 0;
    auto res25 = step_m68k_008bac(0x008C24U, api, true);
    assert(res25 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008bac_resume_pc() == 0x008C28U);

    // 0x008C28: bne.w 0x008C36 (not taken)
    g_fetches = {0x6600U, 0x000CU}; g_fetch_index = 0;
    auto res26 = step_m68k_008bac(0x008C28U, api, true);
    assert(res26 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008bac_resume_pc() == 0x008C2CU);

    // 0x008C2C: btst #0, %d0
    g_fetches = {0x0800U, 0x0000U}; g_fetch_index = 0;
    auto res27 = step_m68k_008bac(0x008C2CU, api, true);
    assert(res27 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008bac_resume_pc() == 0x008C30U);

    // 0x008C30: bne.w 0x008C42 (not taken)
    g_fetches = {0x6600U, 0x0010U}; g_fetch_index = 0;
    auto res28 = step_m68k_008bac(0x008C30U, api, true);
    assert(res28 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008bac_resume_pc() == 0x008C34U);

    // 0x008C34: rts
    g_fetches = {0x4E75U}; g_fetch_index = 0;
    auto res29 = step_m68k_008bac(0x008C34U, api, true);
    assert(res29 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008bac_resume_pc() == 0x008B26U);

    const auto& m = get_008bac_step_metrics();
    assert(m.target_entries == 1);
    assert(m.native_calls == 1);
    assert(m.completed_calls == 1);
    assert(m.rts_completed_calls == 1);
    assert(m.stepped_instructions == 29);
    std::cout << "test_executes_rts_path passed\n";
}

void test_executes_inhibit_rts_path() {
    reset_environment();
    auto api = make_test_api();

    g_regs[15] = 0x00FF8000U;
    mock_write(nullptr, 0x00FF8000U, 4, 0x008B26U);

    // FF165E = 0x00 (not -> 0xFF, andi #0x70 -> 0x70 -> Z=0, bne to 0x008BC8 taken)
    mock_write(nullptr, 0x00FF165EU, 1, 0x00U);
    // FF1987 = 0x01 (inhibit active -> bne to 0x008C34 taken!)
    mock_write(nullptr, 0x00FF1987U, 1, 0x01U);

    // 0x008BAC: move.b $00FF165E, %d0
    g_fetches = {0x1039U, 0x00FFU, 0x165EU}; g_fetch_index = 0;
    auto res1 = step_m68k_008bac(0x008BACU, api, true);
    assert(res1 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008bac_resume_pc() == 0x008BB2U);

    // 0x008BB2: not.b %d0
    g_fetches = {0x4600U}; g_fetch_index = 0;
    auto res2 = step_m68k_008bac(0x008BB2U, api, true);
    assert(res2 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008bac_resume_pc() == 0x008BB4U);

    // 0x008BB4: andi.b #0x70, %d0
    g_fetches = {0x0200U, 0x0070U}; g_fetch_index = 0;
    auto res3 = step_m68k_008bac(0x008BB4U, api, true);
    assert(res3 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008bac_resume_pc() == 0x008BB8U);

    // 0x008BB8: bne.w 0x008BC8 (taken!)
    g_fetches = {0x6600U, 0x000EU}; g_fetch_index = 0;
    auto res4 = step_m68k_008bac(0x008BB8U, api, true);
    assert(res4 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008bac_resume_pc() == 0x008BC8U);

    // 0x008BC8: tst.b $00FF1987
    g_fetches = {0x4A39U, 0x00FFU, 0x1987U}; g_fetch_index = 0;
    auto res5 = step_m68k_008bac(0x008BC8U, api, true);
    assert(res5 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008bac_resume_pc() == 0x008BCEU);

    // 0x008BCE: bne.w 0x008C34 (taken!)
    g_fetches = {0x6600U, 0x0064U}; g_fetch_index = 0;
    auto res6 = step_m68k_008bac(0x008BCEU, api, true);
    assert(res6 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008bac_resume_pc() == 0x008C34U);

    // 0x008C34: rts
    g_fetches = {0x4E75U}; g_fetch_index = 0;
    auto res7 = step_m68k_008bac(0x008C34U, api, true);
    assert(res7 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008bac_resume_pc() == 0x008B26U);

    const auto& m = get_008bac_step_metrics();
    assert(m.target_entries == 1);
    assert(m.native_calls == 1);
    assert(m.completed_calls == 1);
    assert(m.rts_completed_calls == 1);
    assert(m.stepped_instructions == 7);
    std::cout << "test_executes_inhibit_rts_path passed\n";
}

void test_executes_menu_branch_path() {
    reset_environment();
    auto api = make_test_api();

    g_regs[15] = 0x00FF8000U;
    mock_write(nullptr, 0x00FF8000U, 4, 0x008B26U);

    // FF165E = 0x00 (bne to 0x008BC8 taken)
    mock_write(nullptr, 0x00FF165EU, 1, 0x00U);
    // Pause pressed: FF165F bit 7 is set!
    mock_write(nullptr, 0x00FF165FU, 1, 0x80U);

    g_fetches = {0x1039U, 0x00FFU, 0x165EU}; g_fetch_index = 0;
    step_m68k_008bac(0x008BACU, api, true);
    g_fetches = {0x4600U}; g_fetch_index = 0;
    step_m68k_008bac(0x008BB2U, api, true);
    g_fetches = {0x0200U, 0x0070U}; g_fetch_index = 0;
    step_m68k_008bac(0x008BB4U, api, true);
    g_fetches = {0x6600U, 0x000EU}; g_fetch_index = 0;
    step_m68k_008bac(0x008BB8U, api, true);
    g_fetches = {0x4A39U, 0x00FFU, 0x1987U}; g_fetch_index = 0;
    step_m68k_008bac(0x008BC8U, api, true);
    g_fetches = {0x6600U, 0x0064U}; g_fetch_index = 0;
    step_m68k_008bac(0x008BCEU, api, true);
    g_fetches = {0x0839U, 0x0002U, 0x00FFU, 0x17B8U}; g_fetch_index = 0;
    step_m68k_008bac(0x008BD2U, api, true);
    g_fetches = {0x6600U, 0x0058U}; g_fetch_index = 0;
    step_m68k_008bac(0x008BDAU, api, true);
    g_fetches = {0x4A39U, 0x00FFU, 0x1983U}; g_fetch_index = 0;
    step_m68k_008bac(0x008BDEU, api, true);
    g_fetches = {0x6600U, 0x004EU}; g_fetch_index = 0;
    step_m68k_008bac(0x008BE4U, api, true);
    g_fetches = {0x4A39U, 0x00FFU, 0x1982U}; g_fetch_index = 0;
    step_m68k_008bac(0x008BE8U, api, true);
    g_fetches = {0x6600U, 0x0044U}; g_fetch_index = 0;
    step_m68k_008bac(0x008BEEU, api, true);
    g_fetches = {0x51F9U, 0x00FFU, 0x185EU}; g_fetch_index = 0;
    step_m68k_008bac(0x008BF2U, api, true);
    g_fetches = {0x0839U, 0x0007U, 0x00FFU, 0x165FU}; g_fetch_index = 0;
    step_m68k_008bac(0x008BF8U, api, true);

    // 0x008C00: bne.w 0x003B3E (taken!)
    g_fetches = {0x6600U, 0xAF3CU}; g_fetch_index = 0;
    auto res = step_m68k_008bac(0x008C00U, api, true);
    assert(res == M68k9D70StepResult::InstructionStepped);
    assert(m68k_008bac_resume_pc() == 0x003B3EU);

    const auto& m = get_008bac_step_metrics();
    assert(m.target_entries == 1);
    assert(m.native_calls == 1);
    assert(m.completed_calls == 1);
    assert(m.menu_branch_calls == 1);
    std::cout << "test_executes_menu_branch_path passed\n";
}

} // namespace

int main() {
    test_rejects_invalid_caller();
    test_rejects_odd_sp();
    test_rejects_corrupt_rom();
    test_executes_rts_path();
    test_executes_inhibit_rts_path();
    test_executes_menu_branch_path();
    std::cout << "All 008BAC tests passed cleanly!\n";
    return 0;
}
