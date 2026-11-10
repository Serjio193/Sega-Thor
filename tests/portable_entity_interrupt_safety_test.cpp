#include "data/thor-fixtures/m15_244_entity_dispatch_fixtures.hpp"
#include "tools/thor_ir/generated/entity_dispatch_portable_routines.hpp"

#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

using namespace oasis::recompiler::ir;

namespace {

bool load_rom(std::vector<std::uint8_t>& out) {
    std::ifstream in("local-roms/Beyond Oasis (USA).md", std::ios::binary);
    if (!in) return false;
    out.assign(std::istreambuf_iterator<char>(in), {});
    return out.size() == 3145728U;
}

} // namespace

int main() {
    std::cout << "=== M15.244 Phase 6: Interrupt Safety & Masking Verification ===\n";
    std::vector<std::uint8_t> rom;
    if (!load_rom(rom)) {
        std::cerr << "FATAL: Canonical ROM missing\n";
        return 1;
    }
    const auto fixtures = get_m15_244_dispatch_fixtures();
    const auto& fx12 = fixtures[10]; // ctrl_idx12_branch_taken_rts (202 cycles)

    auto init = [&](PortableGuestEnvironment& env) {
        env = PortableGuestEnvironment{};
        env.set_rom(rom);
        for (int i = 0; i < 8; ++i) {
            env.d_regs[i] = fx12.ed[i];
            env.a_regs[i] = fx12.ea[i];
        }
        env.set_sr(0x2100U); // IRQ mask = 1 (allows HBlank=4 and VBlank=6)
        env.pc = fx12.entry_pc;
        for (const auto& rb : fx12.ram) env.work_ram[rb.addr & 0xFFFFU] = rb.val;
    };

    // Baseline uninterrupted
    PortableGuestEnvironment env_base;
    init(env_base);
    const auto res_base = run_entity_dispatch_mode_a(env_base, fx12.entry_pc, fx12.exit_pc);
    assert(res_base.outcome == PortableContinuationOutcome::CompiledNextPc && res_base.cycles == 202U);

    // 1. Preempt with VBlank Level 6 right after 0x005CC8 (memory RMW write) before 0x005CCC (BNE)
    {
        PortableGuestEnvironment env_irq;
        init(env_irq);
        const auto pre = run_entity_dispatch_mode_a(env_irq, fx12.entry_pc, 0x005CCCU);
        assert(pre.pc == 0x005CCCU && pre.cycles == 76U); // 60 + 16
        const std::uint32_t ssp_before = env_irq.a_regs[7];
        const std::uint16_t sr_before = env_irq.sr; // Z=0, N=0, C=0, X=0, mask=1

        // Masked IRQ level 1 MUST be ignored (<= mask 1)
        const bool ignored = env_irq.try_deliver_interrupt(1, 0x000100U);
        assert(!ignored && env_irq.pc == 0x005CCCU && env_irq.a_regs[7] == ssp_before);

        // VBlank Level 6 (> mask 1) preempts cleanly
        const bool taken = env_irq.try_deliver_interrupt(6, 0x000100U);
        assert(taken && env_irq.pc == 0x000100U && (env_irq.sr & 0x0700U) == 0x0600U);
        assert(env_irq.a_regs[7] == ssp_before - 6U);

        // Simulated VBlank ISR preserves state and executes RTE
        env_irq.execute_rte();
        assert(env_irq.pc == 0x005CCCU && env_irq.sr == sr_before && env_irq.a_regs[7] == ssp_before);

        // Resume Mode A execution to exit
        const auto post = run_entity_dispatch_mode_a(env_irq, env_irq.pc, fx12.exit_pc);
        assert(post.outcome == PortableContinuationOutcome::CompiledNextPc);
        assert(post.pc == res_base.pc && env_irq.sr == env_base.sr);
        for (int i = 0; i < 8; ++i) assert(post.d[i] == res_base.d[i] && post.a[i] == res_base.a[i]);
        for (std::uint32_t addr = 0x8000U; addr < 0x80BCU; ++addr) assert(env_irq.work_ram[addr] == env_base.work_ram[addr]);
        // 44 (IRQ) + 20 (RTE) = 64 interrupt cycles + 202 entity cycles = 266
        assert(post.cycles == res_base.cycles + 64U);
        std::cout << "  [PASS] VBlank Level 6 preemption across RMW -> BNE boundary (exact SR/CCR/SSP/cycle parity)\n";
    }

    // 2. Preempt with HBlank Level 4 during nested BSR call 0x005C1E -> 0x005C82 (before nonlocal reset)
    {
        const auto& fx23 = fixtures[15]; // ctrl_idx23_nonlocal_stack_reset
        PortableGuestEnvironment env_nest;
        env_nest.set_rom(rom);
        for (int i = 0; i < 8; ++i) { env_nest.d_regs[i] = fx23.ed[i]; env_nest.a_regs[i] = fx23.ea[i]; }
        env_nest.set_sr(0x2100U);
        for (const auto& rb : fx23.ram) env_nest.work_ram[rb.addr & 0xFFFFU] = rb.val;

        const auto p1 = run_entity_dispatch_mode_a(env_nest, fx23.entry_pc, 0x005C82U);
        assert(p1.pc == 0x005C82U && env_nest.a_regs[7] == 0x00FFFDFCU); // return PC 0x005C20 on stack

        // Level 4 HBlank preempts before 0x005C82
        assert(env_nest.try_deliver_interrupt(4, 0x000200U));
        assert(env_nest.pc == 0x000200U && (env_nest.sr & 0x0700U) == 0x0400U);
        // Level 4 masked while inside Level 4 handler, Level 6 NMI-higher preempts
        assert(!env_nest.try_deliver_interrupt(4, 0x000200U));
        env_nest.execute_rte();
        assert(env_nest.pc == 0x005C82U && env_nest.a_regs[7] == 0x00FFFDFCU);

        const auto p2 = run_entity_dispatch_mode_a(env_nest, env_nest.pc, fx23.exit_pc);
        assert(p2.outcome == PortableContinuationOutcome::CompiledNextPc && p2.pc == 0x00FF2000U);
        assert(p2.a[7] == 0x00FFFE04U && p2.cycles == 180U + 64U);
        std::cout << "  [PASS] HBlank Level 4 preemption inside nested BSR before nonlocal stack reset\n";
    }

    std::cout << "ALL PHASE 6 INTERRUPT SAFETY TESTS PASSED!\n";
    return 0;
}
