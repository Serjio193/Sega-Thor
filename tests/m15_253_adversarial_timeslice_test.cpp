#include "tools/thor_ir/generated_m15_250/exp250_dispatch_portable_routines.hpp"
#include "src/tools/thor_ir/exp250_fused_hybrid_adapter.hpp"
#include "data/thor-fixtures/m15_250_callee_fixtures.hpp"

#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

using namespace oasis::recompiler::ir;

struct MockStepApi {
    std::vector<std::uint8_t> rom;
    std::vector<std::uint8_t> ram;
    std::uint32_t d_regs[8]{0};
    std::uint32_t a_regs[8]{0};
    std::uint16_t sr{0x2000};
    std::uint32_t cur_cycles{0};
    std::uint32_t end_cycles{1000};
    int int_level{0};
    int int_mask{0};
    std::uint32_t next_pc{0};
    std::uint32_t ret_pc{0};
    std::uint16_t next_word{0};
    int added_cycles{0};
    std::vector<std::pair<std::uint32_t, std::uint16_t>> observed_insns;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> memory_writes;

    static int mock_peek(void* ctx, unsigned addr) {
        auto* self = static_cast<MockStepApi*>(ctx);
        addr &= 0xFFFFFFU;
        if (addr < self->rom.size()) return self->rom[addr];
        if (addr >= 0xFF0000U && addr < 0x1000000U) return self->ram[addr - 0xFF0000U];
        return 0;
    }

    static std::uint32_t mock_read(void* ctx, unsigned addr, int size) {
        auto* self = static_cast<MockStepApi*>(ctx);
        addr &= 0xFFFFFFU;
        std::uint32_t val = 0;
        for (int i = 0; i < size; ++i) {
            unsigned a = (addr + i) & 0xFFFFFFU;
            std::uint8_t byte = 0;
            if (a < self->rom.size()) byte = self->rom[a];
            else if (a >= 0xFF0000U && a < 0x1000000U) byte = self->ram[a - 0xFF0000U];
            val = (val << 8) | byte;
        }
        return val;
    }

    static void mock_write(void* ctx, unsigned addr, int size, std::uint32_t val) {
        auto* self = static_cast<MockStepApi*>(ctx);
        self->memory_writes.push_back({addr, val});
        addr &= 0xFFFFFFU;
        if (addr >= 0xFF0000U && addr < 0x1000000U) {
            auto offset = addr - 0xFF0000U;
            if (size == 1) self->ram[offset] = val & 0xFF;
            else if (size == 2) {
                self->ram[offset] = (val >> 8) & 0xFF;
                self->ram[offset + 1] = val & 0xFF;
            } else if (size == 4) {
                self->ram[offset] = (val >> 24) & 0xFF;
                self->ram[offset + 1] = (val >> 16) & 0xFF;
                self->ram[offset + 2] = (val >> 8) & 0xFF;
                self->ram[offset + 3] = val & 0xFF;
            }
        }
    }

    static void mock_set_return_state(void* ctx, unsigned pc, unsigned ret_pc, unsigned next_word) {
        auto* self = static_cast<MockStepApi*>(ctx);
        self->next_pc = pc;
        self->ret_pc = ret_pc;
        self->next_word = static_cast<std::uint16_t>(next_word);
    }

    static int mock_read_scheduler_field(void* ctx, unsigned field) {
        auto* self = static_cast<MockStepApi*>(ctx);
        if (field == 0) return self->cur_cycles;
        if (field == 1) return self->end_cycles;
        if (field == 8) return self->int_level;
        if (field == 9) return self->int_mask;
        return 0;
    }

    static void mock_add_cycles(void* ctx, int cyc) {
        auto* self = static_cast<MockStepApi*>(ctx);
        self->added_cycles += cyc;
        self->cur_cycles += cyc;
    }

    static void mock_observe(void* ctx, unsigned pc, unsigned op) {
        auto* self = static_cast<MockStepApi*>(ctx);
        self->observed_insns.push_back({pc, static_cast<std::uint16_t>(op)});
    }

    static std::uint32_t mock_read_reg(void* ctx, unsigned reg) {
        auto* self = static_cast<MockStepApi*>(ctx);
        if (reg < 8) return self->d_regs[reg];
        if (reg >= 8 && reg < 16) return self->a_regs[reg - 8];
        if (reg == 16) return self->sr;
        return 0;
    }

    static void mock_write_reg(void* ctx, unsigned reg, std::uint32_t val) {
        auto* self = static_cast<MockStepApi*>(ctx);
        if (reg < 8) self->d_regs[reg] = val;
        else if (reg >= 8 && reg < 16) self->a_regs[reg - 8] = val;
        else if (reg == 16) self->sr = static_cast<std::uint16_t>(val);
    }

    oasis::hybrid::M68k9D70StepApi make_api() {
        oasis::hybrid::M68k9D70StepApi api{};
        api.context = this;
        api.peek = mock_peek;
        api.read = mock_read;
        api.write = mock_write;
        api.set_return_state = mock_set_return_state;
        api.read_scheduler_field = mock_read_scheduler_field;
        api.add_cycles = mock_add_cycles;
        api.observe_instruction = mock_observe;
        api.read_register = mock_read_reg;
        api.write_register = mock_write_reg;
        return api;
    }
};

static std::vector<std::uint8_t> load_rom() {
    std::ifstream f("local-roms/Beyond Oasis (USA).md", std::ios::binary | std::ios::ate);
    assert(f.good());
    auto sz = static_cast<std::size_t>(f.tellg());
    f.seekg(0);
    std::vector<std::uint8_t> buf(sz);
    f.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(sz));
    return buf;
}

int main() {
    std::cout << "=== Phase 5 & 6 Adversarial Timeslice and Interrupt Boundary Tests ===\n";
    auto rom = load_rom();

    // Test 1: Full basic block completes when cycles are ample
    {
        MockStepApi mock;
        mock.rom = rom;
        mock.ram.resize(0x10000, 0);
        mock.cur_cycles = 0;
        mock.end_cycles = 1000;
        mock.int_level = 0;
        mock.int_mask = 0;
        auto api = mock.make_api();
        
        // Root: 0x008778 (BB_00: 5 instructions)
        auto res = step_m68k_portable_m15250_fused(0x008778U, api, true);
        assert(res == oasis::hybrid::M68k9D70StepResult::InstructionStepped);
        assert(mock.observed_insns.size() == 5);
        assert(mock.next_pc != 0);
        std::cout << "  [PASS] Test 1: Full basic block executed under ample cycles (" << mock.observed_insns.size() << " insns)\n";
    }

    // Test 2: Slice yield exactly on 2nd instruction when cur_cycles >= end_cycles
    {
        MockStepApi mock;
        mock.rom = rom;
        mock.ram.resize(0x10000, 0);
        mock.cur_cycles = 100;
        mock.end_cycles = 100; // Zero remaining budget after 1st insn
        mock.int_level = 0;
        mock.int_mask = 0;
        auto api = mock.make_api();
        
        auto res = step_m68k_portable_m15250_fused(0x008778U, api, true);
        assert(res == oasis::hybrid::M68k9D70StepResult::InstructionStepped);
        // First instruction ran, 2nd yielded: total observed = 1
        assert(mock.observed_insns.size() == 1);
        // Target next PC must point exactly to 2nd instruction (0x00877E)
        assert(mock.next_pc == 0x00877EU);
        std::cout << "  [PASS] Test 2: Slice yield at budget expiration (stopped before insn 2, next_pc=0x" << std::hex << mock.next_pc << std::dec << ")\n";
    }

    // Test 3: Slice yield when budget is 1 cycle short
    {
        MockStepApi mock;
        mock.rom = rom;
        mock.ram.resize(0x10000, 0);
        mock.cur_cycles = 101;
        mock.end_cycles = 100; // 1 cycle short
        mock.int_level = 0;
        mock.int_mask = 0;
        auto api = mock.make_api();
        
        auto res = step_m68k_portable_m15250_fused(0x008778U, api, true);
        assert(res == oasis::hybrid::M68k9D70StepResult::InstructionStepped);
        assert(mock.observed_insns.size() == 1);
        assert(mock.next_pc == 0x00877EU);
        std::cout << "  [PASS] Test 3: Budget exceeded by previous insn yields immediately\n";
    }

    // Test 4: Pending interrupt before fused entry yields after first instruction
    {
        MockStepApi mock;
        mock.rom = rom;
        mock.ram.resize(0x10000, 0);
        mock.cur_cycles = 0;
        mock.end_cycles = 1000;
        mock.int_level = 6;
        mock.int_mask = 4; // int_level > int_mask (Interrupt pending!)
        auto api = mock.make_api();
        
        auto res = step_m68k_portable_m15250_fused(0x008778U, api, true);
        assert(res == oasis::hybrid::M68k9D70StepResult::InstructionStepped);
        assert(mock.observed_insns.size() == 1);
        assert(mock.next_pc == 0x00877EU);
        std::cout << "  [PASS] Test 4: Pending interrupt forces slice yield before insn 2\n";
    }

    // Test 5: Interrupt masked (int_level <= int_mask) does NOT yield
    {
        MockStepApi mock;
        mock.rom = rom;
        mock.ram.resize(0x10000, 0);
        mock.cur_cycles = 0;
        mock.end_cycles = 1000;
        mock.int_level = 4;
        mock.int_mask = 4; // int_level == int_mask (Masked)
        auto api = mock.make_api();
        
        auto res = step_m68k_portable_m15250_fused(0x008778U, api, true);
        assert(res == oasis::hybrid::M68k9D70StepResult::InstructionStepped);
        assert(mock.observed_insns.size() == 5);
        std::cout << "  [PASS] Test 5: Masked interrupt allows full block execution (" << mock.observed_insns.size() << " insns)\n";
    }

    // Test 6: Invariant Verification - Step after yield resumes cleanly at next_pc
    {
        MockStepApi mock;
        mock.rom = rom;
        mock.ram.resize(0x10000, 0);
        mock.cur_cycles = 100;
        mock.end_cycles = 100; // Trigger yield on insn 2
        mock.int_level = 0;
        mock.int_mask = 0;
        auto api = mock.make_api();
        
        step_m68k_portable_m15250_fused(0x008778U, api, true);
        auto resumed_pc = mock.next_pc;
        
        // Grant new timeslice budget
        mock.cur_cycles = 0;
        mock.end_cycles = 1000;
        auto res2 = step_m68k_portable_m15250_fused(resumed_pc, api, true);
        assert(res2 == oasis::hybrid::M68k9D70StepResult::InstructionStepped);
        assert(mock.observed_insns.size() >= 2);
        assert(mock.observed_insns[1].first == resumed_pc);
        std::cout << "  [PASS] Test 6: Resuming from yield point executes next instruction (0x" << std::hex << resumed_pc << std::dec << ") with zero double-execution\n";
    }

    std::cout << "ALL 6 ADVERSARIAL TIMESLICE & INTERRUPT TESTS PASSED!\n";
    return 0;
}
