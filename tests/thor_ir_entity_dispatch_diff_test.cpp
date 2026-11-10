#include "data/thor-fixtures/m15_244_entity_dispatch_fixtures.hpp"
#include "tools/thor_ir/generated/entity_dispatch_portable_routines.hpp"

#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <set>
#include <vector>

using namespace oasis::recompiler::ir;

namespace {

bool load_rom(std::vector<std::uint8_t>& out) {
    std::ifstream in("local-roms/Beyond Oasis (USA).md", std::ios::binary);
    if (!in) return false;
    out.assign(std::istreambuf_iterator<char>(in), {});
    return out.size() == 3145728U;
}

void init_env(PortableGuestEnvironment& env, const std::vector<std::uint8_t>& rom,
              const DispatchRefFixture& fx) {
    env = PortableGuestEnvironment{};
    env.set_rom(rom);
    for (int i = 0; i < 8; ++i) {
        env.d_regs[i] = fx.ed[i];
        env.a_regs[i] = fx.ea[i];
    }
    env.set_sr(fx.entry_sr);
    env.pc = fx.entry_pc;
    for (const auto& rb : fx.ram) {
        env.work_ram[rb.addr & 0xFFFFU] = rb.val;
    }
}

bool verify_vs_oracle(const char* mode, const DispatchRefFixture& fx,
                      const PortableGuestEnvironment& env,
                      const PortableExecutionResult& res) {
    if (res.outcome != fx.outcome) {
        std::cerr << "FAIL [" << mode << "] " << fx.id << " outcome mismatch: got "
                  << static_cast<int>(res.outcome) << " expected " << static_cast<int>(fx.outcome) << "\n";
        return false;
    }
    if (fx.outcome == PortableContinuationOutcome::OriginalCpuException) {
        if (res.exception_vector != 3U) {
            std::cerr << "FAIL [" << mode << "] " << fx.id << " expected vector 3\n";
            return false;
        }
        return true;
    }
    if ((res.pc & kBusAddressMask) != (fx.exit_pc & kBusAddressMask)) {
        std::cerr << "FAIL [" << mode << "] " << fx.id << " exit PC mismatch: got 0x"
                  << std::hex << res.pc << " expected 0x" << fx.exit_pc << std::dec << "\n";
        return false;
    }
    for (int i = 0; i < 8; ++i) {
        if (res.d[i] != fx.xd[i]) {
            std::cerr << "FAIL [" << mode << "] " << fx.id << " D" << i << " mismatch: got 0x"
                      << std::hex << res.d[i] << " expected 0x" << fx.xd[i] << std::dec << "\n";
            return false;
        }
        if (res.a[i] != fx.xa[i]) {
            std::cerr << "FAIL [" << mode << "] " << fx.id << " A" << i << " mismatch: got 0x"
                      << std::hex << res.a[i] << " expected 0x" << fx.xa[i] << std::dec << "\n";
            return false;
        }
    }
    if ((env.sr & 0x071FU) != (fx.exit_sr & 0x071FU)) {
        std::cerr << "FAIL [" << mode << "] " << fx.id << " SR/CCR/X mismatch: got 0x"
                  << std::hex << env.sr << " expected 0x" << fx.exit_sr << std::dec << "\n";
        return false;
    }
    if (!fx.jitter_ok) {
        if (res.cycles != fx.cycles) {
            std::cerr << "FAIL [" << mode << "] " << fx.id << " cycle mismatch: got "
                      << res.cycles << " expected " << fx.cycles << "\n";
            return false;
        }
    } else {
        if (res.cycles > fx.cycles || (fx.cycles - res.cycles) > 16U) {
            std::cerr << "FAIL [" << mode << "] " << fx.id << " unexpected replay cycle delta: portable="
                      << res.cycles << " replay=" << fx.cycles << "\n";
            return false;
        }
    }
    return true;
}

bool verify_mode_a_eq_mode_b(const DispatchRefFixture& fx,
                             const PortableGuestEnvironment& env_a, const PortableExecutionResult& ra,
                             const PortableGuestEnvironment& env_b, const PortableExecutionResult& rb) {
    if (ra.outcome != rb.outcome || ra.pc != rb.pc || ra.ccr != rb.ccr || ra.cycles != rb.cycles ||
        env_a.sr != env_b.sr || env_a.ssp != env_b.ssp || env_a.usp != env_b.usp ||
        ra.writes != rb.writes || env_a.work_ram != env_b.work_ram) {
        std::cerr << "FAIL Mode A vs Mode B mismatch on " << fx.id << "\n";
        return false;
    }
    return true;
}

} // namespace

int main() {
    std::cout << "=== M15.244 Expanded Three-Way Differential & Mutation Proof ===\n";
    std::vector<std::uint8_t> rom;
    if (!load_rom(rom)) {
        std::cerr << "FATAL: Canonical ROM missing\n";
        return 1;
    }
    const auto fixtures = get_m15_244_dispatch_fixtures();
    std::cout << "Loaded " << fixtures.size() << " real pinned GPGX fixtures.\n";

    std::size_t auth_pass = 0, ctrl_pass = 0;
    std::set<std::uint32_t> covered_entry_targets;

    for (const auto& fx : fixtures) {
        PortableGuestEnvironment env_a, env_b;
        init_env(env_a, rom, fx);
        init_env(env_b, rom, fx);
        const std::uint32_t stop_arg = (fx.outcome == PortableContinuationOutcome::CompiledNextPc) ? fx.exit_pc : 0xFFFFFFFFU;
        const auto ra = run_entity_dispatch_mode_a(env_a, fx.entry_pc, stop_arg);
        const auto rb = run_entity_dispatch_mode_b(env_b, fx.entry_pc, stop_arg);

        if (!verify_vs_oracle("ModeA", fx, env_a, ra)) return 2;
        if (!verify_vs_oracle("ModeB", fx, env_b, rb)) return 3;
        if (!verify_mode_a_eq_mode_b(fx, env_a, ra, env_b, rb)) return 4;

        covered_entry_targets.insert(env_a.a_regs[0] & kBusAddressMask);
        if (std::string(fx.kind) == "AUTHENTIC_GAMEPLAY_ENTRY") ++auth_pass;
        else ++ctrl_pass;
        std::cout << "  [PASS] " << fx.id << " (" << fx.kind << ", cyc=" << ra.cycles << ")\n";
    }

    // Phase 5 Continuation Safety: Split Yield->Resume mid-block + MMIO UnsupportedBusBehavior
    {
        PortableGuestEnvironment env_mid;
        init_env(env_mid, rom, fixtures[10]); // ctrl_idx12_branch_taken_rts
        const auto r1 = run_entity_dispatch_mode_a(env_mid, 0x0059A8U, 0x005CC8U);
        assert(r1.outcome == PortableContinuationOutcome::CompiledNextPc && r1.pc == 0x005CC8U && r1.cycles == 60U);
        const auto r2 = run_entity_dispatch_mode_a(env_mid, r1.pc, fixtures[10].exit_pc);
        assert(r2.outcome == PortableContinuationOutcome::CompiledNextPc && r2.pc == fixtures[10].exit_pc);
        assert(r2.cycles == fixtures[10].cycles && "No double-counted or skipped instructions on mid-handoff");

        PortableGuestEnvironment env_mmio;
        env_mmio.set_rom(rom);
        (void)env_mmio.read_memory(0x00C00004U, IrOpSize::Word);
        assert(env_mmio.bus_error && "Unmapped VDP MMIO read flagged bus_error");
        std::cout << "  [PASS] Continuation boundary yield/resume (0x0059A8 -> 0x005CC8 -> 0x00FF2000) & UnsupportedBusBehavior\n";
    }

    // Phase 9: 10 Explicit Mutation Tests
    const auto base_fx = fixtures[10];
    int mutations_detected = 0;
    auto expect_diff = [&](const char* name, PortableGuestEnvironment& em) {
        const auto rm = run_entity_dispatch_mode_b(em, base_fx.entry_pc, base_fx.exit_pc);
        const bool same = (rm.pc == base_fx.exit_pc && rm.cycles == base_fx.cycles &&
                           rm.d[0] == base_fx.xd[0] && rm.a[0] == base_fx.xa[0] &&
                           rm.a[7] == base_fx.xa[7] && (em.sr & 0x1F) == (base_fx.exit_sr & 0x1F));
        assert(!same && "Mutation test failed to diverge!");
        ++mutations_detected;
        std::cout << "  [PASS Mutation " << mutations_detected << "] " << name << "\n";
    };
    { PortableGuestEnvironment e; init_env(e, rom, base_fx); e.work_ram[0x8005] = 0x1A; e.a_regs[0] = 0x5CF8; expect_diff("1. wrong table target/index", e); }
    { PortableGuestEnvironment e; init_env(e, rom, base_fx); e.work_ram[0x8005] = 0x1C; expect_diff("2. wrong target index", e); }
    { PortableGuestEnvironment e; init_env(e, rom, base_fx); e.a_regs[7] += 4; expect_diff("3. wrong stack pop (A7 shifted)", e); }
    { PortableGuestEnvironment e; init_env(e, rom, base_fx); e.work_ram[0xFE03] ^= 2; expect_diff("4. wrong return PC", e); }
    { PortableGuestEnvironment e; init_env(e, rom, base_fx); e.set_sr(0x2104U); const auto rm = run_entity_dispatch_mode_b(e, 0x005CCCU, base_fx.exit_pc); assert(rm.pc == 0x0062CCU); ++mutations_detected; std::cout << "  [PASS Mutation " << mutations_detected << "] 5. wrong flags (Z-bit flipped at branch)\n"; }
    { PortableGuestEnvironment e; init_env(e, rom, base_fx); e.total_cycles += 14; expect_diff("6. wrong cycles", e); }
    { PortableGuestEnvironment e; init_env(e, rom, base_fx); e.work_ram[0x8051] ^= 1; (void)run_entity_dispatch_mode_b(e, base_fx.entry_pc, base_fx.exit_pc); assert(e.work_ram[0x8075] != 0); ++mutations_detected; std::cout << "  [PASS Mutation " << mutations_detected << "] 7. wrong RAM write detected\n"; }
    { PortableGuestEnvironment e; init_env(e, rom, base_fx); e.work_ram[0x8083] = 1; expect_diff("8. missing/flipped branch", e); }
    { PortableGuestEnvironment e; init_env(e, rom, base_fx); (void)run_entity_dispatch_mode_b(e, base_fx.entry_pc, base_fx.exit_pc); e.pc = 0x005CC8U; expect_diff("9. duplicate execution", e); }
    { PortableGuestEnvironment e; init_env(e, rom, base_fx); e.work_ram[0x8005] = 0x80; const auto r = run_entity_dispatch_mode_b(e, base_fx.entry_pc, 0xFFFFFFFFU); assert(r.outcome == PortableContinuationOutcome::ValidUncompiledPc && r.pc == 0x005A64U && !e.address_error); ++mutations_detected; std::cout << "  [PASS Mutation " << mutations_detected << "] 10. OOB even target stays ValidUncompiledPc (not false exception)\n"; }

    std::cout << "SUMMARY: AUTHENTIC=" << auth_pass << ", CONTROLLED=" << ctrl_pass
              << ", MUTATIONS=" << mutations_detected << "/10, UNIQUE_TARGETS_EXERCISED="
              << covered_entry_targets.size() << "\nALL M15.244 THREE-WAY DIFFERENTIAL & MUTATION TESTS PASSED!\n";
    return 0;
}
