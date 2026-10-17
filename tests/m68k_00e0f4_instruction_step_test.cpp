#include "tools/hybrid/m68k_00e0f4_instruction_step.hpp"
#include "tools/hybrid/m68k_00e0f4_step_ops.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>

namespace {

using namespace oasis::hybrid;
using namespace oasis::hybrid::step_00e0f4;

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
unsigned transfer_pc(void*) { return 0x008B3AU; }

const M68k9D70StepApi kApi{
    nullptr, rr, wr, peek, read, write, fetch16, set_return,
    begin, finish, add_cycles, branch_adj, byte_branch_adj, observe,
    skip_bus, sched, transfer_pc
};

void setup_rom() {
    for (std::size_t i = 0; i < kExpectedWords_00e0f4.size(); ++i) {
        write(nullptr, kEntryPc_00e0f4 + static_cast<unsigned>(i) * 2U, 2, kExpectedWords_00e0f4[i]);
    }
    // Caller at 0x008B3A: bsr.w 0x00E0F4 (0x6100, 0x55B8)
    write(nullptr, 0x008B3AU, 2, 0x6100U);
    write(nullptr, 0x008B3CU, 2, 0x55B8U);
    // Return PC opcode at 0x008B3E
    write(nullptr, 0x008B3EU, 2, 0x4E71U);
}

void test_rejects_invalid_caller() {
    reset_m68k_00e0f4_state();
    g_mem.clear();
    g_regs.fill(0U);
    setup_rom();
    g_regs[15] = 0x00FF8000U;
    write(nullptr, 0x00FF8000U, 4, 0x00123456U); // invalid return PC
    auto res = step_m68k_00e0f4(kEntryPc_00e0f4, kApi, true);
    assert(res == M68k9D70StepResult::NotHandled);
    std::cout << "test_rejects_invalid_caller passed\n";
}

void test_rejects_odd_sp() {
    reset_m68k_00e0f4_state();
    g_mem.clear();
    g_regs.fill(0U);
    setup_rom();
    g_regs[15] = 0x00FF8001U; // odd SP
    auto res = step_m68k_00e0f4(kEntryPc_00e0f4, kApi, true);
    assert(res == M68k9D70StepResult::NotHandled);
    std::cout << "test_rejects_odd_sp passed\n";
}

void test_rejects_corrupt_rom() {
    reset_m68k_00e0f4_state();
    g_mem.clear();
    g_regs.fill(0U);
    setup_rom();
    write(nullptr, kEntryPc_00e0f4, 2, 0x0000U); // corrupt
    g_regs[15] = 0x00FF8000U;
    write(nullptr, 0x00FF8000U, 4, 0x008B3EU);
    auto res = step_m68k_00e0f4(kEntryPc_00e0f4, kApi, true);
    assert(res == M68k9D70StepResult::NotHandled);
    std::cout << "test_rejects_corrupt_rom passed\n";
}

void test_executes_timer_and_inhibit_exit_path() {
    reset_m68k_00e0f4_state();
    g_mem.clear();
    g_regs.fill(0U);
    setup_rom();
    write(nullptr, 0x00FF1856U, 2, 0x0005U); // timer = 5
    write(nullptr, 0x00FF17B8U, 1, 0x02U); // bit 1 set -> inhibit!
    g_regs[15] = 0x00FF8000U;
    write(nullptr, 0x00FF8000U, 4, 0x008B3EU);

    // 0x00E0F4: move.w $00FF1856, %d6
    unsigned pc = kEntryPc_00e0f4;
    auto res = step_m68k_00e0f4(pc, kApi, true);
    assert(res == M68k9D70StepResult::InstructionStepped);
    assert(g_regs[6] == 5U);

    // 0x00E0FA: beq.w 0x00E106 (not taken because d6 != 0)
    pc = 0x00E0FAU;
    res = step_m68k_00e0f4(pc, kApi, true);
    assert(res == M68k9D70StepResult::InstructionStepped);

    // 0x00E0FE: subq.w #1, %d6
    pc = 0x00E0FEU;
    res = step_m68k_00e0f4(pc, kApi, true);
    assert(res == M68k9D70StepResult::InstructionStepped);
    assert((g_regs[6] & 0xFFFFU) == 4U);

    // 0x00E100: move.w %d6, $00FF1856
    pc = 0x00E100U;
    res = step_m68k_00e0f4(pc, kApi, true);
    assert(res == M68k9D70StepResult::InstructionStepped);
    assert(read(nullptr, 0x00FF1856U, 2) == 4U);

    // 0x00E106: btst #1, $00FF17B8
    pc = 0x00E106U;
    res = step_m68k_00e0f4(pc, kApi, true);
    assert(res == M68k9D70StepResult::InstructionStepped);

    // 0x00E10E: bne.w 0x00E13E (taken to RTS)
    pc = 0x00E10EU;
    res = step_m68k_00e0f4(pc, kApi, true);
    assert(res == M68k9D70StepResult::InstructionStepped);

    // 0x00E13E: rts
    pc = 0x00E13EU;
    res = step_m68k_00e0f4(pc, kApi, true);
    assert(res == M68k9D70StepResult::InstructionStepped);
    assert(g_regs[15] == 0x00FF8004U); // popped return PC

    assert(m68k_00e0f4_metrics().target_entries == 1);
    assert(m68k_00e0f4_metrics().completed_calls == 1);
    assert(m68k_00e0f4_metrics().inhibit_exit_calls == 1);
    std::cout << "test_executes_timer_and_inhibit_exit_path passed\n";
}

void test_executes_empty_slots_path() {
    reset_m68k_00e0f4_state();
    g_mem.clear();
    g_regs.fill(0U);
    setup_rom();
    write(nullptr, 0x00FF1856U, 2, 0x0000U); // timer = 0 -> beq taken to 0x00E106
    write(nullptr, 0x00FF17B8U, 1, 0x00U); // inhibit clear
    g_regs[15] = 0x00FF8000U;
    write(nullptr, 0x00FF8000U, 4, 0x008B3EU);

    // All slots 0(%a6) <= 0 -> will branch from 0x00E11E (ble.w) to 0x00E136
    unsigned pc = kEntryPc_00e0f4;
    int steps = 0;
    while (steps < 50) {
        auto res = step_m68k_00e0f4(pc, kApi, true);
        if (res != M68k9D70StepResult::InstructionStepped) {
            std::cerr << "Step failed at pc=0x" << std::hex << pc << " res=" << static_cast<int>(res) << std::endl;
            std::exit(1);
        }
        steps++;
        if (pc == 0x00E13EU) break;
        pc = g_pref_addr - 2U;
    }
    if (pc != 0x00E13EU || steps != 39 || g_regs[15] != 0x00FF8004U ||
        m68k_00e0f4_metrics().target_entries != 1 ||
        m68k_00e0f4_metrics().completed_calls != 1 ||
        m68k_00e0f4_metrics().loop_completed_calls != 1 ||
        m68k_00e0f4_metrics().child_jsr_calls != 0) {
        std::cerr << "test_executes_empty_slots_path failed validation: steps=" << steps << " pc=0x" << std::hex << pc << std::endl;
        std::exit(1);
    }
    std::cout << "test_executes_empty_slots_path passed (steps=" << steps << ")\n";
}

void test_executes_active_slot_with_child() {
    reset_m68k_00e0f4_state();
    g_mem.clear();
    g_regs.fill(0U);
    setup_rom();
    write(nullptr, 0x00FF1856U, 2, 0x0000U);
    write(nullptr, 0x00FF17B8U, 1, 0x00U); // inhibit clear
    g_regs[15] = 0x00FF8000U;
    write(nullptr, 0x00FF8000U, 4, 0x008B3EU);

    // Slot 0 at 0x00FF2ABC: active = 1 (> 0), offset in jump table = 0 (word 4(%a6))
    write(nullptr, 0x00FF2ABCU, 2, 1U);
    write(nullptr, 0x00FF2ABCU + 4U, 2, 0U);
    // Jump table at 0x00E140: offset word = 0x0010 -> target = 0x00E140 + 0x0010 = 0x00E150
    write(nullptr, 0x00E140U, 2, 0x0010U);
    write(nullptr, 0x00E150U, 2, 0x4E75U); // rts opcode

    unsigned pc = kEntryPc_00e0f4;
    int steps = 0;
    bool child_invoked = false;
    while (steps < 100) {
        auto res = step_m68k_00e0f4(pc, kApi, true);
        if (res != M68k9D70StepResult::InstructionStepped) {
            std::cerr << "Active slot step failed at pc=0x" << std::hex << pc << " res=" << static_cast<int>(res) << std::endl;
            std::exit(1);
        }
        steps++;
        if (pc == 0x00E130U) {
            // JSR (A0) executed -> child target set
            child_invoked = true;
            if (g_regs[8] != 0x00E150U) {
                std::cerr << "Target A0 mismatch: 0x" << std::hex << g_regs[8] << std::endl;
                std::exit(1);
            }
            // Return from child: PC resumes at 0x00E132, SP restored to saved_child_sp
            pc = 0x00E132U;
            g_regs[15] = 0x00FF8000U - 8U; // child returned, popping return address
            continue;
        }
        if (pc == 0x00E13EU) break;
        pc = g_pref_addr - 2U;
    }
    if (!child_invoked || pc != 0x00E13EU || g_regs[15] != 0x00FF8004U ||
        m68k_00e0f4_metrics().target_entries != 1 ||
        m68k_00e0f4_metrics().completed_calls != 1 ||
        m68k_00e0f4_metrics().child_jsr_calls != 1) {
        std::cerr << "test_executes_active_slot_with_child failed validation: steps=" << steps << std::endl;
        std::exit(1);
    }
    std::cout << "test_executes_active_slot_with_child passed (steps=" << steps << ")\n";
}

} // namespace

int main() {
    test_rejects_invalid_caller();
    test_rejects_odd_sp();
    test_rejects_corrupt_rom();
    test_executes_timer_and_inhibit_exit_path();
    test_executes_empty_slots_path();
    test_executes_active_slot_with_child();
    std::cout << "All 00E0F4 tests passed cleanly!\n";
    return 0;
}
