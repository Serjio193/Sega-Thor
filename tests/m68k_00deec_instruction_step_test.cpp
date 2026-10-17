#include "tools/hybrid/m68k_00deec_instruction_step.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>

namespace {

using namespace oasis::hybrid;

std::array<std::uint32_t, 18> g_regs{};
std::map<std::uint32_t, std::uint8_t> g_mem{};
unsigned g_pref_addr{};
unsigned g_pref_data{};

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
std::uint32_t fetch16(void*) { return 0U; }
void set_return(void*, unsigned, unsigned addr, unsigned data) {
    g_pref_addr = addr;
    g_pref_data = data;
}
void begin(void*, unsigned) {}
void finish(void*, unsigned) {}
void add_cycles(void*, int) {}
int branch_adj(void*) { return 0; }
int byte_branch_adj(void*) { return 0; }
void observe(void*, unsigned, unsigned) {}
void skip_bus(void*) {}
int sched(void*, unsigned) { return 0; }
unsigned transfer_pc(void*) { return 0x008B32U; }

const M68k9D70StepApi kApi{
    nullptr, rr, wr, peek, read, write, fetch16, set_return,
    begin, finish, add_cycles, branch_adj, byte_branch_adj, observe,
    skip_bus, sched, transfer_pc
};

constexpr std::array<std::uint16_t, 52> kRom = {
    0x0839U, 0x0001U, 0x00FFU, 0x17B8U, 0x6600U, 0x005CU, 0x4DF9U, 0x00FFU,
    0x1AA4U, 0x7E13U, 0x3C2EU, 0x0000U, 0x6F00U, 0x0044U, 0x0C46U, 0x0016U,
    0x6500U, 0x003CU, 0xDC46U, 0x207BU, 0x6014U, 0x48E7U, 0x0102U, 0x4E90U,
    0x4CDFU, 0x4080U, 0x4241U, 0x3401U, 0x122EU, 0x0048U, 0x142EU, 0x0049U,
    0x082EU, 0x0000U, 0x0017U, 0x6700U, 0x0004U, 0xC342U, 0x3D41U, 0x0042U,
    0x5242U, 0xE24AU, 0x3202U, 0xE24AU, 0xD441U, 0x3D42U, 0x0044U, 0x4DEEU,
    0x00BCU, 0x51CFU, 0xFFB0U, 0x4E75U,
};

void setup_rom() {
    for (std::size_t i = 0; i < kRom.size(); ++i) {
        write(nullptr, 0x00DEECU + static_cast<unsigned>(i) * 2U, 2, kRom[i]);
    }
    // Caller at 0x008B32: bsr.w 0x00DEEC (0x6100, 0x53B8)
    write(nullptr, 0x008B32U, 2, 0x6100U);
    write(nullptr, 0x008B34U, 2, 0x53B8U);
    // Return PC opcode at 0x008B36
    write(nullptr, 0x008B36U, 2, 0x4E71U);
    // Table entry at 0x00DF54 (d6 = 22 * 2 = 44 relative to 0x00DF28) -> 0x000115C8
    write(nullptr, 0x00DF54U, 4, 0x000115C8U);
    // Child target dummy opcode
    write(nullptr, 0x000115C8U, 2, 0x4E75U);
}

void test_rejects_invalid_caller() {
    reset_m68k_00deec_state();
    g_mem.clear();
    g_regs.fill(0U);
    setup_rom();
    g_regs[15] = 0x00FF8000U;
    write(nullptr, 0x00FF8000U, 4, 0x00123456U); // invalid return PC
    auto res = step_m68k_00deec(0x00DEECU, kApi, true);
    assert(res == M68k9D70StepResult::NotHandled);
    std::cout << "test_rejects_invalid_caller passed\n";
}

void test_rejects_odd_sp() {
    reset_m68k_00deec_state();
    g_mem.clear();
    g_regs.fill(0U);
    setup_rom();
    g_regs[15] = 0x00FF8001U; // odd SP
    auto res = step_m68k_00deec(0x00DEECU, kApi, true);
    assert(res == M68k9D70StepResult::NotHandled);
    std::cout << "test_rejects_odd_sp passed\n";
}

void test_rejects_corrupt_rom() {
    reset_m68k_00deec_state();
    g_mem.clear();
    g_regs.fill(0U);
    setup_rom();
    write(nullptr, 0x00DEECU, 2, 0x0000U); // corrupt
    g_regs[15] = 0x00FF8000U;
    write(nullptr, 0x00FF8000U, 4, 0x008B36U);
    auto res = step_m68k_00deec(0x00DEECU, kApi, true);
    assert(res == M68k9D70StepResult::NotHandled);
    std::cout << "test_rejects_corrupt_rom passed\n";
}

void test_executes_inhibit_exit_path() {
    reset_m68k_00deec_state();
    g_mem.clear();
    g_regs.fill(0U);
    setup_rom();
    write(nullptr, 0x00FF17B8U, 1, 0x02U); // bit 1 set -> inhibit!
    g_regs[15] = 0x00FF8000U;
    write(nullptr, 0x00FF8000U, 4, 0x008B36U);

    unsigned pc = 0x00DEECU;
    auto res = step_m68k_00deec(pc, kApi, true);
    assert(res == M68k9D70StepResult::InstructionStepped);

    pc = 0x00DEF4U;
    res = step_m68k_00deec(pc, kApi, true);
    assert(res == M68k9D70StepResult::InstructionStepped);

    pc = 0x00DF52U;
    res = step_m68k_00deec(pc, kApi, true);
    assert(res == M68k9D70StepResult::InstructionStepped);
    assert(g_regs[15] == 0x00FF8004U); // popped return PC

    assert(m68k_00deec_metrics().target_entries == 1);
    assert(m68k_00deec_metrics().completed_calls == 1);
    assert(m68k_00deec_metrics().inhibit_exit_calls == 1);
    std::cout << "test_executes_inhibit_exit_path passed\n";
}

void test_executes_empty_slots_path() {
    reset_m68k_00deec_state();
    g_mem.clear();
    g_regs.fill(0U);
    setup_rom();
    write(nullptr, 0x00FF17B8U, 1, 0x00U); // inhibit clear
    g_regs[15] = 0x00FF8000U;
    write(nullptr, 0x00FF8000U, 4, 0x008B36U);

    // All slots 0(A6) == 0 -> will branch from 0x00DF04 (ble.w) to 0x00DF4A
    unsigned pc = 0x00DEECU;
    int steps = 0;
    while (steps < 200) {
        auto res = step_m68k_00deec(pc, kApi, true);
        assert(res == M68k9D70StepResult::InstructionStepped);
        steps++;
        if (pc == 0x00DF52U) break;
        pc = g_pref_addr - 2U;
    }
    assert(pc == 0x00DF52U);
    assert(g_regs[15] == 0x00FF8004U); // popped return PC
    assert(m68k_00deec_metrics().target_entries == 1);
    assert(m68k_00deec_metrics().completed_calls == 1);
    assert(m68k_00deec_metrics().loop_completed_calls == 1);
    assert(m68k_00deec_metrics().child_jsr_calls == 0);
    std::cout << "test_executes_empty_slots_path passed (steps=" << steps << ")\n";
}

void test_executes_active_slot_with_child_and_projection() {
    reset_m68k_00deec_state();
    g_mem.clear();
    g_regs.fill(0U);
    setup_rom();
    write(nullptr, 0x00FF17B8U, 1, 0x00U); // inhibit clear
    g_regs[15] = 0x00FF8000U;
    write(nullptr, 0x00FF8000U, 4, 0x008B36U);

    // Slot 0 at 0x00FF1AA4: type = 22 (0x16)
    write(nullptr, 0x00FF1AA4U, 2, 22U);
    write(nullptr, 0x00FF1AA4U + 72U, 1, 10U); // displacement X = 10
    write(nullptr, 0x00FF1AA4U + 73U, 1, 20U); // displacement Y = 20
    write(nullptr, 0x00FF1AA4U + 23U, 1, 0U);  // facing bit 0 = 0

    unsigned pc = 0x00DEECU;
    int steps = 0;
    bool child_invoked = false;
    while (steps < 300) {
        auto res = step_m68k_00deec(pc, kApi, true);
        assert(res == M68k9D70StepResult::InstructionStepped);
        steps++;
        if (pc == 0x00DF1AU) {
            // JSR (A0) executed -> child target set
            child_invoked = true;
            assert(g_regs[8] == 0x000115C8U); // A0
            // Return from child: PC resumes at 0x00DF1CU, SP restored to saved_child_sp
            pc = 0x00DF1CU;
            g_regs[15] = 0x00FF8000U - 8U; // simulate child return popping return address
            continue;
        }
        if (pc == 0x00DF52U) break;
        pc = g_pref_addr - 2U;
    }
    assert(child_invoked);
    assert(pc == 0x00DF52U);
    assert(g_regs[15] == 0x00FF8004U);

    // Check isometric velocity projection on slot 0:
    // 66(A6) = 10
    // 68(A6) = ((20 + 1) / 2 = 10) + (10 / 2 = 5) = 15
    assert(read(nullptr, 0x00FF1AA4U + 66U, 2) == 10U);
    assert(read(nullptr, 0x00FF1AA4U + 68U, 2) == 15U);

    assert(m68k_00deec_metrics().target_entries == 1);
    assert(m68k_00deec_metrics().completed_calls == 1);
    assert(m68k_00deec_metrics().child_jsr_calls == 1);
    std::cout << "test_executes_active_slot_with_child_and_projection passed\n";
}

} // namespace

int main() {
    test_rejects_invalid_caller();
    test_rejects_odd_sp();
    test_rejects_corrupt_rom();
    test_executes_inhibit_exit_path();
    test_executes_empty_slots_path();
    test_executes_active_slot_with_child_and_projection();
    std::cout << "All 00DEEC tests passed cleanly!\n";
    return 0;
}
