#include "tools/hybrid/m68k_005670_instruction_step.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>
#include <map>

namespace {
using namespace oasis::hybrid;

std::vector<std::uint16_t> g_fetches;
std::size_t g_fetch_index = 0;
std::map<unsigned, std::uint32_t> g_mem;
std::uint32_t g_regs[16]{};
std::uint16_t g_ccr = 0;
unsigned g_last_return_pc = 0;
unsigned g_last_pref_addr = 0;
unsigned g_last_pref_data = 0;

std::uint32_t mock_read_reg(void*, unsigned idx) {
    if (idx < 16) return g_regs[idx];
    if (idx == 17) return g_ccr;
    return 0;
}

void mock_write_reg(void*, unsigned idx, std::uint32_t val) {
    if (idx < 16) g_regs[idx] = val;
    if (idx == 17) g_ccr = static_cast<std::uint16_t>(val);
}

int (*g_custom_peek)(void*, unsigned) = nullptr;

int mock_peek(void* ctx, unsigned addr) {
    if (g_custom_peek) return g_custom_peek(ctx, addr);
    return 0;
}

std::uint32_t mock_read(void*, unsigned addr, int width) {
    std::uint32_t res = 0;
    for (int i = 0; i < width; ++i) {
        auto it = g_mem.find(addr + static_cast<unsigned>(i));
        const std::uint8_t b = (it != g_mem.end()) ? static_cast<std::uint8_t>(it->second) : 0U;
        res = (res << 8U) | b;
    }
    return res;
}

void mock_write(void*, unsigned addr, int width, std::uint32_t val) {
    for (int i = width - 1; i >= 0; --i) {
        g_mem[addr + static_cast<unsigned>(i)] = val & 0xFFU;
        val >>= 8U;
    }
}

std::uint32_t mock_fetch16(void*) {
    if (g_fetch_index < g_fetches.size()) {
        return g_fetches[g_fetch_index++];
    }
    return 0;
}

void mock_set_return(void*, unsigned pc, unsigned pa, unsigned pd) {
    g_last_return_pc = pc;
    g_last_pref_addr = pa;
    g_last_pref_data = pd;
}

void mock_begin(void*, unsigned) {}
void mock_finish(void*, unsigned) {}
void mock_add_cycles(void*, int) {}
int mock_branch_adjust(void*) { return 0; }
int mock_byte_branch_adjust(void*) { return 0; }
void mock_observe(void*, unsigned, unsigned) {}
void mock_skip_bus_refresh(void*) {}
int mock_scheduler_field(void*, unsigned) { return 0; }
unsigned mock_transfer_source_pc(void*) { return 0; }

const M68k9D70StepApi api{
    nullptr,
    mock_read_reg,
    mock_write_reg,
    mock_peek,
    mock_read,
    mock_write,
    mock_fetch16,
    mock_set_return,
    mock_begin,
    mock_finish,
    mock_add_cycles,
    mock_branch_adjust,
    mock_byte_branch_adjust,
    mock_observe,
    mock_skip_bus_refresh,
    mock_scheduler_field,
    mock_transfer_source_pc
};

void test_rejects_invalid_caller() {
    reset_005670_step_metrics();
    g_mem.clear();
    g_regs[15] = 0x00FF2000U;
    mock_write(nullptr, 0x00FF2000U, 4, 0x00001234U); // invalid return PC

    auto res = step_m68k_005670(0x005670U, api, true);
    assert(res == M68k9D70StepResult::NotHandled);
    const auto& m = get_005670_step_metrics();
    assert(m.caller_other_calls == 1);
    assert(m.rom_fallback_calls == 1);
    assert(m.native_calls == 0);
    std::cout << "test_rejects_invalid_caller passed\n";
}

void test_rejects_odd_sp() {
    reset_005670_step_metrics();
    g_mem.clear();
    g_regs[15] = 0x00FF2001U; // odd SP

    auto res = step_m68k_005670(0x005670U, api, true);
    assert(res == M68k9D70StepResult::NotHandled);
    const auto& m = get_005670_step_metrics();
    assert(m.rom_fallback_calls == 1);
    std::cout << "test_rejects_odd_sp passed\n";
}

void test_rejects_corrupt_rom() {
    reset_005670_step_metrics();
    g_mem.clear();
    g_regs[15] = 0x00FF2000U;
    mock_write(nullptr, 0x00FF2000U, 4, 0x000558AU); // valid return PC from 0x005586
    g_fetches = {0x0000U}; // corrupt opcode
    g_fetch_index = 0;
    g_custom_peek = [](void*, unsigned) -> int { return -1; };

    auto res = step_m68k_005670(0x005670U, api, true);
    g_custom_peek = nullptr;
    assert(res == M68k9D70StepResult::ContractFault);
    std::cout << "test_rejects_corrupt_rom passed\n";
}

void test_executes_action_dispatch_path() {
    reset_005670_step_metrics();
    g_mem.clear();
    g_regs[15] = 0x00FF2000U;
    g_regs[14] = 0x00FF19E8U; // A6
    mock_write(nullptr, 0x00FF2000U, 4, 0x000558AU);

    // 0x005670: move.l %sp, $FF1934
    g_fetches = {0x23CFU, 0x00FFU, 0x1934U}; g_fetch_index = 0;
    auto res1 = step_m68k_005670(0x005670U, api, true);
    assert(res1 == M68k9D70StepResult::InstructionStepped);
    assert(mock_read(nullptr, 0x00FF1934U, 4) == 0x00FF2000U);

    // 0x005676: move.b $FF1985, %d0 (is 0)
    mock_write(nullptr, 0x00FF1985U, 1, 0U);
    g_fetches = {0x1039U, 0x00FFU, 0x1985U}; g_fetch_index = 0;
    auto res2 = step_m68k_005670(0x005676U, api, true);
    assert(res2 == M68k9D70StepResult::InstructionStepped);

    // 0x00567C: beq.s 0x005686 (taken)
    g_fetches = {0x6708U}; g_fetch_index = 0;
    auto res3 = step_m68k_005670(0x00567CU, api, true);
    assert(res3 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_005670_resume_pc() == 0x005686U);

    // 0x005686: tst.b %d0 (is 0)
    g_fetches = {0x4A00U}; g_fetch_index = 0;
    auto res4 = step_m68k_005670(0x005686U, api, true);
    assert(res4 == M68k9D70StepResult::InstructionStepped);

    // 0x005688: bne.w 0x0056AE (not taken)
    g_fetches = {0x6600U, 0x0024U}; g_fetch_index = 0;
    auto res5 = step_m68k_005670(0x005688U, api, true);
    assert(res5 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_005670_resume_pc() == 0x00568CU);

    // Jump to 0x0059B2: lea 6(%pc, %d0.w), %a0 with %d0 = 0
    g_regs[0] = 0;
    g_fetches = {0x41FBU, 0x0006U}; g_fetch_index = 0;
    auto res6 = step_m68k_005670(0x0059B2U, api, true);
    assert(res6 == M68k9D70StepResult::InstructionStepped);
    assert(g_regs[8] == 0x0059BAU);

    // 0x0059B6: adda.w (%a0), %a0 where (%a0) = 0x083C (2108) -> 0x0061F6
    mock_write(nullptr, 0x0059BAU, 2, 0x083CU);
    g_fetches = {0xD0D0U}; g_fetch_index = 0;
    auto res7 = step_m68k_005670(0x0059B6U, api, true);
    assert(res7 == M68k9D70StepResult::InstructionStepped);
    assert(g_regs[8] == 0x0061F6U);

    // 0x0059B8: jmp (%a0) -> jumps to 0x0061F6
    g_fetches = {0x4ED0U}; g_fetch_index = 0;
    auto res8 = step_m68k_005670(0x0059B8U, api, true);
    assert(res8 == M68k9D70StepResult::InstructionStepped);
    assert(m68k_005670_resume_pc() == 0x0061F6U);

    const auto& m = get_005670_step_metrics();
    assert(m.target_entries == 1);
    assert(m.native_calls == 1);
    assert(m.completed_calls == 1);
    assert(m.dispatch_jmp_calls == 1);
    std::cout << "test_executes_action_dispatch_path passed\n";
}

void test_interruption_and_reentrancy() {
    reset_005670_step_metrics();
    g_mem.clear();
    g_regs[15] = 0x00FF2000U;
    mock_write(nullptr, 0x00FF2000U, 4, 0x000558AU);

    g_fetches = {0x23CFU, 0x00FFU, 0x1934U}; g_fetch_index = 0;
    auto res = step_m68k_005670(0x005670U, api, true);
    assert(res == M68k9D70StepResult::InstructionStepped);

    // Simulate interrupt pushing state (SP lower than entry_sp)
    g_regs[15] = 0x00FF1FE0U;
    auto res_irq = step_m68k_005670(0x00000400U, api, true);
    assert(res_irq == M68k9D70StepResult::NotHandled);

    // Return from interrupt
    g_regs[15] = 0x00FF2000U;
    // Next instruction at 0x005676
    g_fetches = {0x1039U, 0x00FFU, 0x1985U}; g_fetch_index = 0;
    auto res_resume = step_m68k_005670(0x005676U, api, true);
    assert(res_resume == M68k9D70StepResult::InstructionStepped);

    std::cout << "test_interruption_and_reentrancy passed\n";
}

} // namespace

int main() {
    test_rejects_invalid_caller();
    test_rejects_odd_sp();
    test_rejects_corrupt_rom();
    test_executes_action_dispatch_path();
    test_interruption_and_reentrancy();
    std::cout << "All 005670 unit tests passed successfully!\n";
    return 0;
}
