#include "tools/hybrid/m68k_00b3b2_instruction_step.hpp"
#include "tools/hybrid/m68k_00b3b2_step_ops.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>

namespace {

using namespace oasis::hybrid;
using namespace oasis::hybrid::step_00b3b2;

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
unsigned g_fetch_pc = 0U;
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
unsigned g_transfer_pc = 0x00AD4EU;
unsigned transfer_pc(void*) { return g_transfer_pc; }

const M68k9D70StepApi kApi{
    nullptr, rr, wr, peek, read, write, fetch16, set_return,
    begin, finish, add_cycles, branch_adj, byte_branch_adj, observe,
    skip_bus, sched, transfer_pc
};

void setup_rom() {
    for (std::size_t i = 0; i < kExpectedWords_00b3b2.size(); ++i) {
        write(nullptr, kEntryPc_00b3b2 + static_cast<unsigned>(i) * 2U, 2, kExpectedWords_00b3b2[i]);
    }
    write(nullptr, 0x00B382U, 2, kExpectedWord_00b382);
    // Caller at 0x00AD4E: bsr.w 0x00B3B2 (0x6100, 0x0662)
    write(nullptr, 0x00AD4EU, 2, 0x6100U);
    write(nullptr, 0x00AD50U, 2, 0x0662U);
    // Return PC opcode at 0x00AD52
    write(nullptr, 0x00AD52U, 2, 0x6000U);
    write(nullptr, 0x00AD54U, 2, 0xFF30U);
    // Caller at 0x00B01A: bsr.w 0x00B3B2 (0x6100, 0x0396)
    write(nullptr, 0x00B01AU, 2, 0x6100U);
    write(nullptr, 0x00B01CU, 2, 0x0396U);
    // Return PC opcode at 0x00B01E
    write(nullptr, 0x00B01EU, 2, 0x4E75U);
    // Child target dummy opcode at 0x00B66A
    write(nullptr, 0x00B66AU, 2, 0x4E75U);
}

void test_rejects_invalid_caller() {
    reset_m68k_00b3b2_state();
    g_mem.clear();
    g_regs.fill(0U);
    setup_rom();
    g_regs[15] = 0x00FF8000U;
    write(nullptr, 0x00FF8000U, 4, 0x00123456U); // invalid return PC
    auto res = step_m68k_00b3b2(kEntryPc_00b3b2, kApi, true);
    assert(res == M68k9D70StepResult::NotHandled);
    std::cout << "test_rejects_invalid_caller passed\n";
}

void test_rejects_odd_sp() {
    reset_m68k_00b3b2_state();
    g_mem.clear();
    g_regs.fill(0U);
    setup_rom();
    g_regs[15] = 0x00FF8001U; // odd SP
    auto res = step_m68k_00b3b2(kEntryPc_00b3b2, kApi, true);
    assert(res == M68k9D70StepResult::NotHandled);
    std::cout << "test_rejects_odd_sp passed\n";
}

void test_rejects_corrupt_rom() {
    reset_m68k_00b3b2_state();
    g_mem.clear();
    g_regs.fill(0U);
    setup_rom();
    write(nullptr, kEntryPc_00b3b2, 2, 0x0000U); // corrupt
    g_regs[15] = 0x00FF8000U;
    write(nullptr, 0x00FF8000U, 4, 0x00AD52U);
    auto res = step_m68k_00b3b2(kEntryPc_00b3b2, kApi, true);
    assert(res == M68k9D70StepResult::NotHandled);
    std::cout << "test_rejects_corrupt_rom passed\n";
}

void test_executes_pause_exit_path() {
    reset_m68k_00b3b2_state();
    g_mem.clear();
    g_regs.fill(0U);
    setup_rom();
    write(nullptr, 0x00FF1927U, 1, 0x01U); // pause non-zero
    g_regs[15] = 0x00FF8000U;
    write(nullptr, 0x00FF8000U, 4, 0x00AD52U);

    unsigned pc = kEntryPc_00b3b2;
    auto res = step_m68k_00b3b2(pc, kApi, true);
    assert(res == M68k9D70StepResult::InstructionStepped);

    pc = 0x00B3B8U;
    res = step_m68k_00b3b2(pc, kApi, true);
    assert(res == M68k9D70StepResult::InstructionStepped);

    pc = 0x00B382U;
    res = step_m68k_00b3b2(pc, kApi, true);
    assert(res == M68k9D70StepResult::InstructionStepped);
    assert(g_regs[15] == 0x00FF8004U); // popped return PC

    assert(m68k_00b3b2_metrics().target_entries == 1);
    assert(m68k_00b3b2_metrics().completed_calls == 1);
    assert(m68k_00b3b2_metrics().pause_exit_calls == 1);
    std::cout << "test_executes_pause_exit_path passed\n";
}

void test_executes_bit1_clear_exit_path() {
    reset_m68k_00b3b2_state();
    g_mem.clear();
    g_regs.fill(0U);
    setup_rom();
    write(nullptr, 0x00FF1927U, 1, 0x00U); // pause clear
    g_regs[14] = 0x00FF2000U; // A6
    write(nullptr, 0x00FF2000U + 57U, 1, 0x00U); // 57(A6) bit 1 is 0
    g_regs[15] = 0x00FF8000U;
    write(nullptr, 0x00FF8000U, 4, 0x00AD52U);

    unsigned pc = kEntryPc_00b3b2;
    int steps = 0;
    while (steps < 10) {
        auto res = step_m68k_00b3b2(pc, kApi, true);
        if (res != M68k9D70StepResult::InstructionStepped) {
            std::cerr << "Step failed at pc=0x" << std::hex << pc << std::endl;
            std::exit(1);
        }
        steps++;
        if (pc == 0x00B3C8U) break;
        pc = m68k_00b3b2_resume_pc();
    }
    if (pc != 0x00B3C8U || steps != 6 || g_regs[15] != 0x00FF8004U ||
        m68k_00b3b2_metrics().target_entries != 1 ||
        m68k_00b3b2_metrics().completed_calls != 1 ||
        m68k_00b3b2_metrics().normal_exit_calls != 1) {
        std::cerr << "test_executes_bit1_clear_exit_path failed validation: steps=" << steps << " pc=0x" << std::hex << pc << std::endl;
        std::exit(1);
    }
    std::cout << "test_executes_bit1_clear_exit_path passed (steps=" << steps << ")\n";
}

void test_executes_bit1_set_child_bsr_path() {
    reset_m68k_00b3b2_state();
    g_mem.clear();
    g_regs.fill(0U);
    setup_rom();
    write(nullptr, 0x00FF1927U, 1, 0x00U); // pause clear
    g_regs[14] = 0x00FF2000U; // A6
    write(nullptr, 0x00FF2000U + 57U, 1, 0x02U); // 57(A6) bit 1 is 1!
    g_regs[15] = 0x00FF8000U;
    write(nullptr, 0x00FF8000U, 4, 0x00AD52U);

    unsigned pc = kEntryPc_00b3b2;
    int steps = 0;
    bool child_invoked = false;
    while (steps < 20) {
        auto res = step_m68k_00b3b2(pc, kApi, true);
        if (res != M68k9D70StepResult::InstructionStepped) {
            std::cerr << "Child step failed at pc=0x" << std::hex << pc << std::endl;
            std::exit(1);
        }
        steps++;
        if (pc == 0x00B3CAU) {
            // BSR.W 0x00B66A executed
            child_invoked = true;
            assert(g_regs[15] == 0x00FF8000U - 4U); // pushed return PC 0x00B3CE
            assert(read(nullptr, g_regs[15], 4) == 0x00B3CEU);
            // Simulate child return: PC resumes at 0x00B3CE, SP restored to 0x00FF8000
            pc = 0x00B3CEU;
            g_regs[15] = 0x00FF8000U;
            continue;
        }
        if (pc == 0x00B3C8U) break;
        pc = m68k_00b3b2_resume_pc();
    }
    if (!child_invoked || pc != 0x00B3C8U || steps != 8 || g_regs[15] != 0x00FF8004U ||
        m68k_00b3b2_metrics().target_entries != 1 ||
        m68k_00b3b2_metrics().completed_calls != 1 ||
        m68k_00b3b2_metrics().child_bsr_calls != 1 ||
        m68k_00b3b2_metrics().normal_exit_calls != 1) {
        std::cerr << "test_executes_bit1_set_child_bsr_path failed validation: steps=" << steps << std::endl;
        std::exit(1);
    }
    std::cout << "test_executes_bit1_set_child_bsr_path passed (steps=" << steps << ")\n";
}

void test_executes_caller_00b01a_path() {
    reset_m68k_00b3b2_state();
    g_mem.clear();
    g_regs.fill(0U);
    setup_rom();
    g_transfer_pc = 0x00B01AU;
    write(nullptr, 0x00FF1927U, 1, 0x01U); // pause non-zero
    g_regs[15] = 0x00FF8000U;
    write(nullptr, 0x00FF8000U, 4, 0x00B01EU); // return PC to 0x00B01E

    unsigned pc = kEntryPc_00b3b2;
    auto res = step_m68k_00b3b2(pc, kApi, true);
    assert(res == M68k9D70StepResult::InstructionStepped);

    pc = 0x00B3B8U;
    res = step_m68k_00b3b2(pc, kApi, true);
    assert(res == M68k9D70StepResult::InstructionStepped);

    pc = 0x00B382U;
    res = step_m68k_00b3b2(pc, kApi, true);
    assert(res == M68k9D70StepResult::InstructionStepped);
    assert(g_regs[15] == 0x00FF8004U); // popped return PC

    assert(m68k_00b3b2_metrics().target_entries == 1);
    assert(m68k_00b3b2_metrics().completed_calls == 1);
    assert(m68k_00b3b2_metrics().pause_exit_calls == 1);
    std::cout << "test_executes_caller_00b01a_path passed\n";
    g_transfer_pc = 0x00AD4EU; // restore default
}

} // namespace

int main() {
    test_rejects_invalid_caller();
    test_rejects_odd_sp();
    test_rejects_corrupt_rom();
    test_executes_pause_exit_path();
    test_executes_bit1_clear_exit_path();
    test_executes_bit1_set_child_bsr_path();
    test_executes_caller_00b01a_path();
    std::cout << "All 00B3B2 tests passed cleanly!\n";
    return 0;
}
