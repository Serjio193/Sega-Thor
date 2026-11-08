#include "tools/hybrid/m68k_00a9a0_instruction_step.hpp"
#include "tools/hybrid/m68k_00a9a0_step_ops.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>

namespace {

using namespace oasis::hybrid;
using namespace oasis::hybrid::step_00a9a0;

std::array<std::uint32_t, 18> g_regs{};
std::map<std::uint32_t, std::uint8_t> g_mem{};
unsigned g_pref_addr{};
unsigned g_pref_data{};
unsigned g_fetch_pc = 0U;

std::uint32_t rr(void*, unsigned idx) { return g_regs.at(idx); }
void wr(void*, unsigned idx, std::uint32_t val) { g_regs.at(idx) = val; }
int peek(void*, unsigned addr) {
    auto it = g_mem.find(addr & 0x00FFFFFFU);
    return it != g_mem.end() ? it->second : 0;
}
std::uint32_t read(void* ctx, unsigned addr, int width) {
    std::uint32_t v{};
    for (int i = 0; i < width; ++i) {
        v = (v << 8U) | static_cast<std::uint8_t>(peek(ctx, addr + static_cast<unsigned>(i)));
    }
    return v;
}
void write(void*, unsigned addr, int width, std::uint32_t val) {
    for (int i = width - 1; i >= 0; --i) {
        g_mem[addr + static_cast<unsigned>(i)] = static_cast<std::uint8_t>(val & 0xFFU);
        val >>= 8U;
    }
}
std::uint32_t fetch16(void* ctx) {
    const auto word = static_cast<std::uint16_t>(read(ctx, g_fetch_pc, 2));
    g_fetch_pc += 2U;
    return word;
}
void set_return(void*, unsigned, unsigned addr, unsigned data) {
    g_pref_addr = addr;
    g_pref_data = data;
}
void begin(void*, unsigned) {}
void finish(void*, unsigned) {}
void add_cycles(void*, int) {}
int branch_adj(void*) { return 0; }
int byte_branch_adj(void*) { return 0; }
void observe(void*, unsigned pc, unsigned) {
    g_fetch_pc = pc;
}
void skip_bus(void*) {}
int sched(void*, unsigned) { return 0; }
unsigned g_transfer_pc = 0x00A22CU;
unsigned transfer_pc(void*) { return g_transfer_pc; }

const M68k9D70StepApi kApi{
    nullptr, rr, wr, peek, read, write, fetch16, set_return,
    begin, finish, add_cycles, branch_adj, byte_branch_adj,
    observe, skip_bus, sched, transfer_pc,
};

void reset_state() {
    reset_00a9a0_state();
    reset_00a9a0_step_metrics();
    g_mem.clear();
    g_regs.fill(0U);
    g_fetch_pc = 0U;
    g_pref_addr = 0U;
    g_pref_data = 0U;
}

// Opcode writing helper
void write16(unsigned addr, std::uint16_t val) {
    write(nullptr, addr, 2, val);
}

void test_fast_path_hidden_exit() {
    reset_state();
    // Setup stack
    g_regs[15] = 0x00FF8000U;
    write(nullptr, 0x00FF8000U, 4, 0x00A230U); // return PC on stack

    // Entity a6 at 0x00FF1000
    const unsigned a6 = 0x00FF1000U;
    g_regs[14] = a6;
    // bit 1 of 54(a6) is set (0x02) -> fast exit to RTS at 0x00ACFC
    write(nullptr, a6 + 54U, 1, 0x02U);

    // 0x00A9A0: btst.b #1, 54(a6)
    write16(0x00A9A0U, 0x082EU);
    write16(0x00A9A2U, 0x0001U);
    write16(0x00A9A4U, 0x0036U);
    // 0x00A9A6: bne.w $00ACFC
    write16(0x00A9A6U, 0x6600U);
    write16(0x00A9A8U, 0x0354U);
    // 0x00ACFC: rts
    write16(0x00ACFCU, 0x4E75U);

    unsigned pc = 0x00A9A0U;
    auto res = step_m68k_00a9a0(pc, kApi, true);
    assert(res == M68k9D70StepResult::InstructionStepped);

    pc = 0x00A9A6U;
    res = step_m68k_00a9a0(pc, kApi, true);
    assert(res == M68k9D70StepResult::InstructionStepped);

    pc = 0x00ACFCU;
    res = step_m68k_00a9a0(pc, kApi, true);
    assert(res == M68k9D70StepResult::InstructionStepped);
    assert(g_regs[15] == 0x00FF8004U); // return PC popped from stack

    assert(get_00a9a0_step_metrics().target_entries == 1);
    assert(get_00a9a0_step_metrics().completed_calls == 1);
    assert(get_00a9a0_step_metrics().fast_path_hidden_rts == 1);
    std::cout << "[PASS] test_fast_path_hidden_exit\n";
}

void test_mode1_step_execution_boundary() {
    reset_state();
    // Test that step_m68k_00a9a0 rejects unhandled PCs cleanly
    unsigned unhandled_pc = 0x001234U;
    auto res = step_m68k_00a9a0(unhandled_pc, kApi, true);
    assert(res == M68k9D70StepResult::NotHandled);

    std::cout << "[PASS] test_mode1_step_execution_boundary\n";
}

} // namespace

namespace oasis::hybrid {
M68k9D70StepResult step_m68k_00b3b2(unsigned, const M68k9D70StepApi&, bool) noexcept {
    return M68k9D70StepResult::NotHandled;
}
M68k9D70StepResult step_m68k_00b34c(unsigned, const M68k9D70StepApi&, bool) noexcept {
    return M68k9D70StepResult::NotHandled;
}
} // namespace oasis::hybrid

int main() {
    test_fast_path_hidden_exit();
    test_mode1_step_execution_boundary();
    std::cout << "All 00A9A0 instruction step tests passed successfully!\n";
    return 0;
}
