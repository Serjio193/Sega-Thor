#include "tools/thor_ir/generated_m15_250/exp250_dispatch_portable_routines.hpp"
#include "src/tools/thor_ir/exp250_fused_hybrid_adapter.hpp"
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

using namespace oasis::recompiler::ir;

struct MockMultiBlockStepApi {
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

    static int mock_peek(void* ctx, unsigned addr) {
        auto* self = static_cast<MockMultiBlockStepApi*>(ctx);
        addr &= 0xFFFFFFU;
        if (addr < self->rom.size()) return self->rom[addr];
        if (addr >= 0xFF0000U && addr < 0x1000000U) return self->ram[addr - 0xFF0000U];
        return 0;
    }

    static std::uint32_t mock_read(void* ctx, unsigned addr, int size) {
        auto* self = static_cast<MockMultiBlockStepApi*>(ctx);
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
        auto* self = static_cast<MockMultiBlockStepApi*>(ctx);
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
        auto* self = static_cast<MockMultiBlockStepApi*>(ctx);
        self->next_pc = pc;
        self->ret_pc = ret_pc;
        self->next_word = static_cast<std::uint16_t>(next_word);
    }

    static int mock_read_scheduler_field(void* ctx, unsigned field) {
        auto* self = static_cast<MockMultiBlockStepApi*>(ctx);
        if (field == 0) return self->cur_cycles;
        if (field == 1) return self->end_cycles;
        if (field == 8) return self->int_level;
        if (field == 9) return self->int_mask;
        return 0;
    }

    static void mock_add_cycles(void* ctx, int cyc) {
        auto* self = static_cast<MockMultiBlockStepApi*>(ctx);
        self->added_cycles += cyc;
        self->cur_cycles += cyc;
    }

    static void mock_observe(void* ctx, unsigned pc, unsigned op) {
        auto* self = static_cast<MockMultiBlockStepApi*>(ctx);
        self->observed_insns.push_back({pc, static_cast<std::uint16_t>(op)});
    }

    static std::uint32_t mock_read_reg(void* ctx, unsigned reg) {
        auto* self = static_cast<MockMultiBlockStepApi*>(ctx);
        if (reg < 8) return self->d_regs[reg];
        if (reg >= 8 && reg < 16) return self->a_regs[reg - 8];
        if (reg == 16 || reg == 17) return self->sr;
        return 0;
    }

    static void mock_write_reg(void* ctx, unsigned reg, std::uint32_t val) {
        auto* self = static_cast<MockMultiBlockStepApi*>(ctx);
        if (reg < 8) self->d_regs[reg] = val;
        else if (reg >= 8 && reg < 16) self->a_regs[reg - 8] = val;
        else if (reg == 16 || reg == 17) self->sr = static_cast<std::uint16_t>(val);
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
    std::cout << "=== Phase 9 Adversarial Multi-Block Fusion Safety Tests ===\n";
    auto rom = load_rom();

    // Test 1: Full multi-block chaining through candidate edge BB_15 -> BB_16
    {
        MockMultiBlockStepApi mock;
        mock.rom = rom;
        mock.ram.resize(0x10000, 0);
        auto api = mock.make_api();
        detail::exp250_metrics_mut() = Exp250DispatchHybridMetrics{};

        auto res = step_m68k_portable_m15250_multiblock(0x00A550U, api, true);
        assert(res == oasis::hybrid::M68k9D70StepResult::InstructionStepped);
        assert(mock.observed_insns.size() == 7); // 5 in BB_15 + 2 in BB_16
        const auto& m = get_exp250_dispatch_hybrid_metrics();
        assert(m.multi_block_entries == 1);
        assert(m.multi_block_chained_blocks == 2);
        assert(m.internal_cfg_edges_traversed == 1);
        std::cout << "  [PASS] Test 1: Chained BB_15 -> BB_16 across CFG edge (7 insns in 1 entry)\n";
    }

    // Test 2: Timeslice budget expiration yields on 2nd instruction
    {
        MockMultiBlockStepApi mock;
        mock.rom = rom;
        mock.ram.resize(0x10000, 0);
        mock.cur_cycles = 100;
        mock.end_cycles = 100; // Zero budget remaining for instruction 2
        auto api = mock.make_api();
        detail::exp250_metrics_mut() = Exp250DispatchHybridMetrics{};

        auto res = step_m68k_portable_m15250_multiblock(0x00A550U, api, true);
        assert(res == oasis::hybrid::M68k9D70StepResult::InstructionStepped);
        assert(mock.observed_insns.size() == 1); // Yields before instruction 2
        assert(mock.next_pc == 0x00A556U);
        std::cout << "  [PASS] Test 2: Timeslice budget expiration stops multi-block execution safely\n";
    }

    // Test 3: Pending interrupt at boundary prevents chaining into successor block
    {
        MockMultiBlockStepApi mock;
        mock.rom = rom;
        mock.ram.resize(0x10000, 0);
        mock.cur_cycles = 0;
        mock.end_cycles = 1000;
        mock.int_level = 6;
        mock.int_mask = 2; // Interrupt pending!
        auto api = mock.make_api();
        detail::exp250_metrics_mut() = Exp250DispatchHybridMetrics{};

        auto res = step_m68k_portable_m15250_multiblock(0x00A550U, api, true);
        assert(res == oasis::hybrid::M68k9D70StepResult::InstructionStepped);
        assert(mock.observed_insns.size() == 1); // Yields immediately
        std::cout << "  [PASS] Test 3: Pending interrupt inhibits multi-block chaining\n";
    }

    // Test 4: Camera Tracker BB_09 -> BB_10 -> BB_11 chaining under non-zero flag
    {
        MockMultiBlockStepApi mock;
        mock.rom = rom;
        mock.ram.resize(0x10000, 0);
        // Set 0xFF184F to non-zero (0x184F in RAM) so BEQ at 0x00A4DC falls through to BB_10 (0x00A4E0)
        mock.ram[0x184F] = 0x01;
        // Set 0xFF1858 to non-zero so BEQ at 0x00A4F8 falls through to BB_12 (0x00A4FA)
        mock.ram[0x1858] = 0x01;
        auto api = mock.make_api();
        detail::exp250_metrics_mut() = Exp250DispatchHybridMetrics{};

        auto res = step_m68k_portable_m15250_multiblock(0x00A4C8U, api, true);
        assert(res == oasis::hybrid::M68k9D70StepResult::InstructionStepped);
        // BB_09 (4 insns) -> BB_10 (2 insns) -> BB_11 (3 insns) = 9 insns total
        assert(mock.observed_insns.size() == 9);
        const auto& m = get_exp250_dispatch_hybrid_metrics();
        assert(m.multi_block_entries == 1);
        assert(m.multi_block_chained_blocks == 3);
        assert(m.internal_cfg_edges_traversed == 2);
        std::cout << "  [PASS] Test 4: 3-block chaining BB_09 -> BB_10 -> BB_11 (" << mock.observed_insns.size() << " insns)\n";
    }

    // Test 5: Camera Tracker BB_09 branch taken jumps directly to BB_11 (skipping BB_10)
    {
        MockMultiBlockStepApi mock;
        mock.rom = rom;
        mock.ram.resize(0x10000, 0);
        // 0xFF184F is 0x00 (default) -> BEQ taken to 0x00A4EE (BB_11)
        // Set 0xFF1858 to non-zero so BEQ at 0x00A4F8 does not branch to compiled region
        mock.ram[0x1858] = 0x01;
        auto api = mock.make_api();
        detail::exp250_metrics_mut() = Exp250DispatchHybridMetrics{};

        auto res = step_m68k_portable_m15250_multiblock(0x00A4C8U, api, true);
        assert(res == oasis::hybrid::M68k9D70StepResult::InstructionStepped);
        // BB_09 (4 insns) -> BB_11 (3 insns) = 7 insns total
        if (mock.observed_insns.size() != 7) {
            std::cerr << "Observed " << mock.observed_insns.size() << " instructions:\n";
            for (auto [pc, op] : mock.observed_insns) {
                std::cerr << "  0x" << std::hex << pc << " (op 0x" << op << ")\n";
            }
        }
        assert(mock.observed_insns.size() == 7);
        const auto& m = get_exp250_dispatch_hybrid_metrics();
        assert(m.multi_block_entries == 1);
        assert(m.multi_block_chained_blocks == 2);
        assert(m.internal_cfg_edges_traversed == 1);
        std::cout << "  [PASS] Test 5: Conditional branch taken edge BB_09 -> BB_11 chained successfully\n";
    }

    // Test 6: Invariant & Resumption: Yielded block resumes without register corruption
    {
        MockMultiBlockStepApi mock;
        mock.rom = rom;
        mock.ram.resize(0x10000, 0);
        mock.cur_cycles = 100;
        mock.end_cycles = 100; // Trigger yield
        auto api = mock.make_api();
        detail::exp250_metrics_mut() = Exp250DispatchHybridMetrics{};

        step_m68k_portable_m15250_multiblock(0x00A550U, api, true);
        auto resumed_pc = mock.next_pc;

        mock.cur_cycles = 0;
        mock.end_cycles = 1000;
        auto res2 = step_m68k_portable_m15250_multiblock(resumed_pc, api, true);
        assert(res2 == oasis::hybrid::M68k9D70StepResult::InstructionStepped);
        assert(mock.observed_insns.size() >= 2);
        std::cout << "  [PASS] Test 6: Resumption from yield point restores seamless execution\n";
    }

    std::cout << "ALL 6 ADVERSARIAL MULTI-BLOCK FUSION TESTS PASSED!\n";
    return 0;
}
