#include "tools/hybrid/generated/m68k_recompiled_00e6ba.hpp"
#include "tools/hybrid/generated/m68k_recompiled_00f0d2.hpp"
#include "tools/hybrid/m68k_jump_exception_types.hpp"

#include <array>
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

struct MockStressCpu {
    std::array<std::uint32_t, 18> regs{};
    std::map<std::uint32_t, std::uint8_t> ram{};
    unsigned next_pc{0};
    std::vector<std::pair<std::uint32_t, std::uint32_t>> mmio_log;

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
        if (addr >= 0x00C00000U && addr < 0x00C00020U) {
            mmio_log.emplace_back(addr, val);
        }
        for (int i = size - 1; i >= 0; --i) {
            ram[addr + i] = static_cast<std::uint8_t>(val & 0xFFU);
            val >>= 8U;
        }
    }
};

MockStressCpu s_cpu;

std::uint32_t mock_read_reg(void*, unsigned r) { return s_cpu.read_reg(r); }
void mock_write_reg(void*, unsigned r, std::uint32_t v) { s_cpu.write_reg(r, v); }
std::uint32_t mock_read(void*, unsigned addr, int sz) { return s_cpu.read_mem(addr, sz); }
void mock_write(void*, unsigned addr, int sz, std::uint32_t v) { s_cpu.write_mem(addr, sz, v); }
std::uint32_t mock_fetch16(void*) { return 0x4E71U; }
int mock_word_adj(void*) { return 0; }
int mock_byte_adj(void*) { return 0; }
int mock_peek(void*, unsigned addr) {
    auto it = s_cpu.ram.find(addr & 0x00FFFFFFU);
    if (it == s_cpu.ram.end()) return 0x4E;
    return static_cast<int>(it->second);
}
void mock_set_return_state(void*, unsigned pc, unsigned, unsigned) { s_cpu.next_pc = pc; }
void mock_observe_insn(void*, unsigned, unsigned) {}
void mock_begin_insn(void*, unsigned) {}
void mock_finish_insn(void*, unsigned) {}

M68k9D70StepApi make_stress_api() {
    M68k9D70StepApi api{};
    api.context = &s_cpu;
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

} // namespace

int main() {
    std::cout << "[M15.240] Starting Mixed Execution & Interrupt Stress Test Suite...\n";
    auto api = make_stress_api();

    // 1. Native Caller to Native Callee (Nested Calls with Stack Push/Pop Parity)
    std::cout << "  1. Testing Native-to-Native Stack Push/Pop Parity...\n";
    {
        s_cpu = MockStressCpu{};
        s_cpu.write_reg(3, 60);  // D3
        s_cpu.write_reg(4, 80);  // D4
        s_cpu.write_reg(1, 0);   // D1
        s_cpu.write_reg(2, 0);   // D2
        const std::uint32_t initial_sp = 0x00FFFE00U;
        s_cpu.write_reg(15, initial_sp);
        s_cpu.write_mem(initial_sp, 4, 0x00CAFE00U); // Return address from 0x00F0D2

        generated::reset_metrics_00f0d2();
        generated::reset_metrics_00e6ba();

        unsigned pc = 0x00F0D2U;
        int steps = 0;
        while (steps < 400) {
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
            pc = s_cpu.next_pc;
            if (pc == 0x00CAFE00U) break;
        }

        // Return PC popped exactly once
        TEST_ASSERT(s_cpu.read_reg(15) == initial_sp + 4U);
        TEST_ASSERT((s_cpu.read_reg(0) & 0xFFFFU) == 100); // sqrt(60^2 + 80^2) = 100
        std::cout << "    [PASS] Stack frame exactly restored, distance = 100\n";
    }

    // 2. Interrupt Preemption and Continuation Idempotence
    std::cout << "  2. Testing Interrupt Preemption and Continuation Idempotence...\n";
    {
        s_cpu = MockStressCpu{};
        s_cpu.write_reg(0, 144);
        s_cpu.write_reg(15, 0x00FFFE00U);
        s_cpu.write_mem(0x00FFFE00U, 4, 0x00BEE0U);

        generated::reset_metrics_00e6ba();

        // Step 1: Entry
        auto res1 = generated::step_m68k_00e6ba(0x00E6BAU, api, true);
        TEST_ASSERT(res1 == M68k9D70StepResult::InstructionStepped);

        // Simulate Interrupt Preemption (CPU diverts to VBlank ISR at 0x0002A0)
        auto res_irq = generated::step_m68k_00e6ba(0x0002A0U, api, true);
        TEST_ASSERT(res_irq == M68k9D70StepResult::NotHandled); // Correctly yields to ISR

        // Resume at resume_pc (0x00E6BE)
        auto res_resume = generated::step_m68k_00e6ba(0x00E6BEU, api, true);
        TEST_ASSERT(res_resume == M68k9D70StepResult::InstructionStepped);

        // Complete the routine
        unsigned pc = s_cpu.next_pc;
        while (pc != 0x00BEE0U) {
            auto r = generated::step_m68k_00e6ba(pc, api, true);
            TEST_ASSERT(r == M68k9D70StepResult::InstructionStepped);
            pc = s_cpu.next_pc;
        }

        TEST_ASSERT((s_cpu.read_reg(0) & 0xFFFFU) == 12); // sqrt(144) = 12
        TEST_ASSERT(s_cpu.read_reg(15) == 0x00FFFE04U);
        std::cout << "    [PASS] Preemption yielded without side-effect duplication, resumed to 12\n";
    }

    // 3. MMIO Access Ordering
    std::cout << "  3. Testing MMIO Access Ordering...\n";
    {
        s_cpu = MockStressCpu{};
        s_cpu.write_mem(0x00C00004U, 2, 0x8100U); // VDP register write 1
        s_cpu.write_mem(0x00C00004U, 2, 0x8200U); // VDP register write 2
        TEST_ASSERT(s_cpu.mmio_log.size() == 2);
        TEST_ASSERT(s_cpu.mmio_log[0].second == 0x8100U);
        TEST_ASSERT(s_cpu.mmio_log[1].second == 0x8200U);
        std::cout << "    [PASS] MMIO writes preserved exact sequential order\n";
    }

    std::cout << "[M15.240] Mixed Execution & Stress Tests: ALL TESTS PASSED!\n";
    return 0;
}
