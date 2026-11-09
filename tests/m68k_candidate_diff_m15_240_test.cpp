#include "tools/hybrid/generated/m68k_recompiled_00e6ba.hpp"
#include "tools/hybrid/generated/m68k_recompiled_00f0d2.hpp"
#include "tools/hybrid/generated/m68k_recompiled_016294.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>

#define TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion failed at " << __FILE__ << ":" << __LINE__ << ": " #cond << "\n"; \
            std::exit(1); \
        } \
    } while (false)

namespace {

using namespace oasis::hybrid;

struct MockCpu {
    std::array<std::uint32_t, 18> regs{};
    std::map<std::uint32_t, std::uint8_t> ram{};
    unsigned next_pc{0};

    std::uint32_t read_reg(unsigned r) const { return r < regs.size() ? regs[r] : 0U; }
    void write_reg(unsigned r, std::uint32_t v) { if (r < regs.size()) regs[r] = v; }

    std::uint32_t read_mem(std::uint32_t addr, int size) {
        addr &= 0x00FFFFFFU;
        std::uint32_t val = 0;
        for (int i = 0; i < size; ++i) {
            auto it = ram.find(addr + i);
            val = (val << 8U) | (it != ram.end() ? it->second : 0U);
        }
        return val;
    }

    void write_mem(std::uint32_t addr, int size, std::uint32_t val) {
        addr &= 0x00FFFFFFU;
        for (int i = size - 1; i >= 0; --i) {
            ram[addr + i] = static_cast<std::uint8_t>(val & 0xFFU);
            val >>= 8U;
        }
    }
};

MockCpu g_cpu;

std::uint32_t mock_read_reg(void*, unsigned r) { return g_cpu.read_reg(r); }
void mock_write_reg(void*, unsigned r, std::uint32_t v) { g_cpu.write_reg(r, v); }
std::uint32_t mock_read(void*, unsigned addr, int sz) { return g_cpu.read_mem(addr, sz); }
void mock_write(void*, unsigned addr, int sz, std::uint32_t v) { g_cpu.write_mem(addr, sz, v); }
std::uint32_t mock_fetch16(void*) { return 0x4E71U; }
int mock_word_adj(void*) { return 0; }
int mock_byte_adj(void*) { return 0; }
int mock_peek(void*, unsigned addr) {
    auto it = g_cpu.ram.find(addr & 0x00FFFFFFU);
    if (it == g_cpu.ram.end()) return 0x4E;
    return static_cast<int>(it->second);
}
void mock_set_return_state(void*, unsigned pc, unsigned, unsigned) { g_cpu.next_pc = pc; }
void mock_observe_insn(void*, unsigned, unsigned) {}
void mock_begin_insn(void*, unsigned) {}
void mock_finish_insn(void*, unsigned) {}

M68k9D70StepApi make_api() {
    M68k9D70StepApi api{};
    api.context = &g_cpu;
    api.read_register = mock_read_reg;
    api.write_register = mock_write_reg;
    api.read = mock_read;
    api.write = mock_write;
    api.fetch16 = mock_fetch16;
    api.word_not_taken_adjustment = mock_word_adj;
    api.byte_not_taken_adjustment = mock_byte_adj;
    api.peek = mock_peek;
    api.set_return_state = mock_set_return_state;
    api.observe_instruction = mock_observe_insn;
    api.begin_instruction = mock_begin_insn;
    api.finish_instruction = mock_finish_insn;
    return api;
}

// Pinned Reference Integer Square Root
std::uint32_t reference_isqrt(std::uint32_t n) {
    if (n == 0) return 0;
    std::uint32_t x0 = (n < 0x400) ? n : 0x400;
    while (true) {
        std::uint32_t x1 = (x0 + (n / x0)) >> 1;
        if (x1 >= x0) return x0;
        x0 = x1;
    }
}

} // namespace

int main() {
    std::cout << "[M15.240] Starting Independent Differential Qualification for Candidates 1..3..." << std::endl;
    auto api = make_api();

    // =========================================================================
    // Candidate 1: 0x00E6BA (Newton-Raphson Integer Square Root Engine)
    // =========================================================================
    std::cout << "  Testing Candidate 1 (Slot 242: 0x00E6BA Integer Sqrt)...\n";
    const std::vector<std::uint32_t> sqrt_inputs = {0, 1, 4, 25, 100, 1024, 2500, 65536, 1000000};
    for (auto val : sqrt_inputs) {
        g_cpu = MockCpu{};
        g_cpu.write_reg(0, val);
        g_cpu.write_reg(1, 0x11111111U); // D1 canary
        g_cpu.write_reg(2, 0x22222222U); // D2 canary
        g_cpu.write_reg(15, 0x00FFFE00U); // SP
        g_cpu.write_mem(0x00FFFE00U, 4, 0x00BEE0U); // Return address

        generated::reset_metrics_00e6ba();

        unsigned pc = 0x00E6BAU;
        int steps = 0;
        while (steps < 200) {
            auto res = generated::step_m68k_00e6ba(pc, api, true);
            if (res != M68k9D70StepResult::InstructionStepped) {
                std::cerr << "Step failed! pc=0x" << std::hex << pc << " res=" << static_cast<int>(res) << std::dec << "\n";
            }
            TEST_ASSERT(res == M68k9D70StepResult::InstructionStepped);
            steps++;
            pc = g_cpu.next_pc; // resume / next PC
            if (pc == 0x00BEE0U) break; // Return to caller
        }

        const auto expected_sqrt = reference_isqrt(val);
        TEST_ASSERT((g_cpu.read_reg(0) & 0xFFFFU) == expected_sqrt);
        TEST_ASSERT(g_cpu.read_reg(1) == 0x11111111U); // D1 restored
        TEST_ASSERT(g_cpu.read_reg(2) == 0x22222222U); // D2 restored
        TEST_ASSERT(g_cpu.read_reg(15) == 0x00FFFE04U); // SP popped return addr
        const auto& m = generated::get_metrics_00e6ba();
        TEST_ASSERT(m.target_entries == 1);
        TEST_ASSERT(m.completed_calls == 1);
    }
    std::cout << "    [PASS] Candidate 1 passed bit-exact sqrt across all inputs!\n";

    // =========================================================================
    // Candidate 2: 0x00F0D2 (2D Euclidean Distance Vector Math)
    // =========================================================================
    std::cout << "  Testing Candidate 2 (Slot 243: 0x00F0D2 2D Distance Math)...\n";
    {
        // (D3, D4) = (30, 40), (D1, D2) = (0, 0) -> distance = 50
        g_cpu = MockCpu{};
        g_cpu.write_reg(3, 30);
        g_cpu.write_reg(4, 40);
        g_cpu.write_reg(1, 0);
        g_cpu.write_reg(2, 0);
        g_cpu.write_reg(15, 0x00FFFE00U);
        g_cpu.write_mem(0x00FFFE00U, 4, 0x00BEE0U);

        generated::reset_metrics_00f0d2();
        generated::reset_metrics_00e6ba();

        unsigned pc = 0x00F0D2U;
        int steps = 0;
        while (steps < 300) {
            M68k9D70StepResult res;
            if (pc >= 0x00F0D2U && pc <= 0x00F0EAU) {
                res = generated::step_m68k_00f0d2(pc, api, true);
            } else if (pc >= 0x00E6BAU && pc <= 0x00E6EEU) {
                res = generated::step_m68k_00e6ba(pc, api, true);
            } else {
                break;
            }
            TEST_ASSERT(res == M68k9D70StepResult::InstructionStepped);
            steps++;
            pc = g_cpu.next_pc;
            if (pc == 0x00BEE0U) break;
        }

        TEST_ASSERT((g_cpu.read_reg(0) & 0xFFFFU) == 50); // sqrt(30^2 + 40^2) = 50
        TEST_ASSERT(g_cpu.read_reg(3) == 30); // D3 restored
        TEST_ASSERT(g_cpu.read_reg(4) == 40); // D4 restored
        TEST_ASSERT(g_cpu.read_reg(15) == 0x00FFFE04U);
        std::cout << "    [PASS] Candidate 2 nested call to Candidate 1 returned distance = 50!\n";
    }

    // =========================================================================
    // Candidate 3: 0x016294 (Entity AI Patrol State Timer & Selector)
    // =========================================================================
    std::cout << "  Testing Candidate 3 (Slot 244: 0x016294 AI Timer & Selector)...\n";
    {
        // Case A: Flag clear, timer = 0x10 -> timer becomes 0x11, branches to 0x015AFC (< 0x5A)
        g_cpu = MockCpu{};
        g_cpu.write_reg(14, 0x00FF1000U); // A6
        g_cpu.write_mem(0x00FF0E1DU, 1, 0x00); // BTST #0, $FF0E1D == 0
        g_cpu.write_mem(0x00FF10B0U, 2, 0x10); // Timer $B0(A6) = 0x10

        generated::reset_metrics_016294();

        unsigned pc = 0x016294U;
        int steps = 0;
        while (steps < 20) {
            auto res = generated::step_m68k_016294(pc, api, true);
            TEST_ASSERT(res == M68k9D70StepResult::InstructionStepped);
            steps++;
            pc = g_cpu.next_pc;
            if (pc == 0x015AFCU || pc == 0x016306U || pc == 0x0163A4U) break;
        }

        TEST_ASSERT(g_cpu.read_mem(0x00FF10B0U, 2) == 0x11); // Timer incremented
        TEST_ASSERT(pc == 0x015AFCU); // Correct patrol branch taken
        std::cout << "    [PASS] Candidate 3 timer increment & branch to 0x015AFC verified!\n";
    }

    // =========================================================================
    // Mutation Testing
    // =========================================================================
    std::cout << "  Running Candidate Mutation Injections...\n";
    {
        // Mutation 1: Inverted Z-flag during sqrt loop
        g_cpu = MockCpu{};
        g_cpu.write_reg(0, 100);
        g_cpu.write_reg(15, 0x00FFFE00U);
        g_cpu.write_mem(0x00FFFE00U, 4, 0x00BEE0U);
        g_cpu.regs[17] ^= 0x04U; // Invert Z-flag in SR
        auto res = generated::step_m68k_00e6ba(0x00E6BAU, api, true);
        TEST_ASSERT(res == M68k9D70StepResult::InstructionStepped);
        std::cout << "    [Mutation 1] Flag perturbation handled without contract fault!\n";
    }

    std::cout << "[M15.240] Candidate Differential Qualification: ALL CANDIDATES PASSED!\n";
    return 0;
}
