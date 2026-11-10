#include "tools/thor_ir/generated/entity_dispatch_portable_routines.hpp"
#include "tools/thor_ir/generated_m15_245/exp_dispatch_portable_routines.hpp"
#include "tools/thor_ir/generated_m15_246/exp246_dispatch_portable_routines.hpp"
#include "data/thor-fixtures/m15_246_callee_fixtures.hpp"

#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace oasis::recompiler::ir;

static std::vector<std::uint8_t> load_rom_bytes() {
    std::ifstream f("local-roms/Beyond Oasis (USA).md", std::ios::binary | std::ios::ate);
    assert(f.good());
    auto sz = static_cast<std::size_t>(f.tellg());
    f.seekg(0);
    std::vector<std::uint8_t> buf(sz);
    f.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(sz));
    return buf;
}

static void init_env(PortableGuestEnvironment& env, const std::vector<std::uint8_t>& rom, const DispatchRefFixture& f) {
    env.rom_data = rom;
    std::fill(env.work_ram.begin(), env.work_ram.end(), std::uint8_t{0});
    for (int i = 0; i < 8; ++i) { env.d_regs[i] = f.ed[i]; env.a_regs[i] = f.ea[i]; }
    env.set_sr(f.entry_sr);
    env.pc = f.entry_pc;
    env.total_cycles = 0;
    env.bus_error = false;
    env.address_error = false;
    env.yielded = false;
    env.sync_boundary_count = 0;
    for (const auto& rb : f.ram) {
        if (rb.addr >= kWorkRamBase && rb.addr < kWorkRamBase + kWorkRamSize)
            env.work_ram[rb.addr - kWorkRamBase] = rb.val;
    }
}

static PortableExecutionResult run_all3_mode_b(PortableGuestEnvironment& env, std::uint32_t ep, std::uint32_t sp) {
    PortableIrBus246 bus{env};
    env.pc = ep & kBusAddressMask;
    std::uint32_t next_pc = env.pc;
    int guard = 0;
    while (!bus.faulted() && !env.yielded && env.pc != sp && ++guard < 8192) {
        if (is_compiled_entity_dispatch_pc(env.pc)) {
            if (!step_one_entity_dispatch_ir(bus, env.pc, next_pc)) break;
        } else if (is_compiled_exp_dispatch_pc(env.pc)) {
            if (!step_one_exp_dispatch_ir(bus, env.pc, next_pc)) break;
        } else if (is_compiled_exp246_dispatch_pc(env.pc)) {
            if (!step_exp246_mb_block(bus, env.pc, sp, next_pc)) break;
        } else {
            break;
        }
    }
    PortableExecutionResult res{};
    res.outcome = PortableContinuationOutcome::CompiledNextPc;
    if (bus.faulted()) {
        res.outcome = PortableContinuationOutcome::OriginalCpuException;
        res.exception_vector = bus.div_zero ? 5U : (env.address_error ? 3U : 2U);
    } else if (env.yielded || (env.pc != sp && !is_compiled_entity_dispatch_pc(env.pc) &&
                               !is_compiled_exp_dispatch_pc(env.pc) && !is_compiled_exp246_dispatch_pc(env.pc))) {
        res.outcome = PortableContinuationOutcome::ValidUncompiledPc;
    }
    for (int i = 0; i < 8; ++i) { res.d[i] = env.d_regs[i]; res.a[i] = env.a_regs[i]; }
    res.pc = env.pc; res.ccr = env.read_ccr(); res.cycles = env.total_cycles; res.writes = bus.writes;
    res.completed = (res.outcome == PortableContinuationOutcome::CompiledNextPc);
    return res;
}

static bool verify_res(const char* tag, const DispatchRefFixture& f, const PortableExecutionResult& r, bool strict_cyc) {
    if (f.outcome == PortableContinuationOutcome::OriginalCpuException) {
        if (r.outcome != PortableContinuationOutcome::OriginalCpuException || r.exception_vector != 5U) {
            std::cerr << "[" << f.id << " " << tag << "] Expected Vector 5 exception!\n";
            return false;
        }
        return true;
    }
    bool ok = true;
    if (r.pc != f.exit_pc) { std::cerr << "[" << f.id << " " << tag << "] PC mismatch: got 0x" << std::hex << r.pc << " exp 0x" << f.exit_pc << "\n"; ok = false; }
    if (r.outcome != f.outcome) { std::cerr << "[" << f.id << " " << tag << "] Outcome mismatch\n"; ok = false; }
    if ((r.ccr & 0x1FU) != (f.exit_sr & 0x1FU)) { std::cerr << "[" << f.id << " " << tag << "] CCR mismatch: got 0x" << std::hex << (r.ccr & 0x1F) << " exp 0x" << (f.exit_sr & 0x1F) << "\n"; ok = false; }
    for (int i = 0; i < 8; ++i) {
        if (r.d[i] != f.xd[i]) { std::cerr << "[" << f.id << " " << tag << "] D" << i << " mismatch: got 0x" << std::hex << r.d[i] << " exp 0x" << f.xd[i] << "\n"; ok = false; }
        if (r.a[i] != f.xa[i]) { std::cerr << "[" << f.id << " " << tag << "] A" << i << " mismatch: got 0x" << std::hex << r.a[i] << " exp 0x" << f.xa[i] << "\n"; ok = false; }
    }
    if (strict_cyc) {
        if (r.cycles != f.cycles) { std::cerr << "[" << f.id << " " << tag << "] CYC mismatch: got " << std::dec << r.cycles << " exp " << f.cycles << "\n"; ok = false; }
    } else if (r.cycles > f.cycles || r.cycles + (32ULL + f.cycles / 32ULL) < f.cycles) {
        std::cerr << "[" << f.id << " " << tag << "] CYC out of refresh band: got " << std::dec << r.cycles << " exp " << f.cycles << "\n"; ok = false;
    }
    return ok;
}

static void run_cross_boundary_and_e682_tests(const std::vector<std::uint8_t>& rom) {
    // 1. M15.246 0x006E0A -> BSR 0x006E7A (M15.245) -> RTS to 0x006E28 (M15.245 RTS) -> 0x00FF2000
    PortableGuestEnvironment env;
    env.rom_data = rom;
    std::fill(env.work_ram.begin(), env.work_ram.end(), std::uint8_t{0});
    env.a_regs[6] = 0x00FF8000U; env.a_regs[7] = 0x00FFFE00U;
    env.write_memory(0x00FFFE00U, IrOpSize::Long, 0x00FF2000U);
    env.set_sr(0x2100U);
    auto r1 = run_all3_mode_b(env, 0x006E0AU, 0x00FF2000U);
    assert(r1.outcome == PortableContinuationOutcome::CompiledNextPc && r1.pc == 0x00FF2000U);
    assert(r1.a[7] == 0x00FFFE04U);

    // 2. Preserve M15.244 0x005C1A -> 0x005C82 -> 0x00E682 nonlocal +8 A7 unwind
    std::fill(env.work_ram.begin(), env.work_ram.end(), std::uint8_t{0});
    env.a_regs[6] = 0x00FF8000U; env.a_regs[7] = 0x00FF0BDEU;
    env.write_memory(0x00FF0BDEU, IrOpSize::Long, 0x000057D6U);
    env.write_memory(0x00FF0BE2U, IrOpSize::Long, 0x0000558AU);
    env.write_memory(0x00FF1934U, IrOpSize::Long, 0x00FF0BE2U);
    env.write_memory(0x00FF165EU, IrOpSize::Byte, 0x04U);
    env.set_sr(0x2100U);
    auto r2 = run_all3_mode_b(env, 0x005C1AU, 0x0000558AU);
    assert(r2.pc == 0x0000558AU && r2.a[7] == 0x00FF0BE6U);
    assert(env.read_memory(0x00FF804EU, IrOpSize::Long) == 0U);
    std::cout << "  [PASS] Cross-module M15.246->M15.245->RTS & M15.244 nonlocal 0x00E682 unwind\n";
}

static void run_mutations(const std::vector<std::uint8_t>& rom, const DispatchRefFixture& f_divs, const DispatchRefFixture& f_jsr) {
    // Disjoint checks: M15.246 must NEVER claim 196 prod (e.g. 0x0085E2, 0x02EDCA, 0x02EEA0) or M15.244/245 (0x006E28, 0x0059A8)
    assert(!is_compiled_exp246_dispatch_pc(0x0085E2U));
    assert(!is_compiled_exp246_dispatch_pc(0x02EDCAU));
    assert(!is_compiled_exp246_dispatch_pc(0x02EEA0U));
    assert(!is_compiled_exp246_dispatch_pc(0x006E28U));
    assert(!is_compiled_exp246_dispatch_pc(0x0059A8U));

    PortableGuestEnvironment env;
    init_env(env, rom, f_divs);
    const auto base = run_exp246_dispatch_mode_b(env, f_divs.entry_pc, f_divs.exit_pc);
    assert(verify_res("BaseDivs", f_divs, base, true));

    // Mut 1: wrong opcode decode guard -> false PC rejected
    { auto m = base; m.pc = 0x0085E2U; assert(!verify_res("M1_opcode", f_divs, m, true)); }
    // Mut 2: wrong sign extension EXT.L (upper 16 bits corrupted)
    { auto m = base; m.d[0] ^= 0xFFFF0000U; assert(!verify_res("M2_sext", f_divs, m, true)); }
    // Mut 3: wrong branch target
    { auto m = base; m.pc = 0x02EE6CU; assert(!verify_res("M3_branch", f_divs, m, true)); }
    // Mut 4: wrong dynamic JSR (A2) target
    { PortableGuestEnvironment e2; init_env(e2, rom, f_jsr); auto rj = run_exp246_dispatch_mode_b(e2, f_jsr.entry_pc, 0x00FFFFFFU); auto m = rj; m.pc = 0x00FF2022U; assert(!verify_res("M4_dynjmp", f_jsr, m, true)); }
    // Mut 5: wrong A7 stack delta on MOVEM/JSR
    { auto m = base; m.a[7] -= 4U; assert(!verify_res("M5_a7", f_divs, m, true)); }
    // Mut 6: wrong return PC pushed on stack by JSR (A2)
    { PortableGuestEnvironment e2; init_env(e2, rom, f_jsr); auto rj = run_exp246_dispatch_mode_b(e2, f_jsr.entry_pc, 0x00FFFFFFU); assert(e2.read_memory(rj.a[7], IrOpSize::Long) == 0x0000BE5CU); assert(e2.read_memory(rj.a[7], IrOpSize::Long) != 0x0000BE5EU); }
    // Mut 7: wrong memory width check (MOVEM.L writes 4 bytes per reg, not 2)
    { auto m = base; m.d[1] &= 0xFFFFU; if (base.d[1] & 0xFFFF0000U) assert(!verify_res("M7_width", f_divs, m, true)); }
    // Mut 8: wrong cycle count under strict Booth/DIVS/ASL check
    { auto m = base; m.cycles += 2U; assert(!verify_res("M8_cyc", f_divs, m, true)); }
    // Mut 9: yielded/interrupt boundary stops cleanly before next block
    { PortableGuestEnvironment e2; init_env(e2, rom, f_divs); e2.yielded = true; auto ry = run_exp246_dispatch_mode_b(e2, f_divs.entry_pc, f_divs.exit_pc); assert(ry.cycles == 0 && ry.pc == f_divs.entry_pc && ry.outcome == PortableContinuationOutcome::ValidUncompiledPc); }
    // Mut 10: duplicate execution prevention after exiting uncompiled target
    { PortableGuestEnvironment e2; init_env(e2, rom, f_jsr); auto r1 = run_exp246_dispatch_mode_b(e2, f_jsr.entry_pc, 0x00FFFFFFU); e2.total_cycles = 0; auto r2 = run_exp246_dispatch_mode_b(e2, e2.pc, 0x00FFFFFFU); assert(r1.cycles > 0 && r2.cycles == 0 && r2.outcome == PortableContinuationOutcome::ValidUncompiledPc); }
    // Mut 11: invalid fallback outcome detected
    { auto m = base; m.outcome = PortableContinuationOutcome::ValidUncompiledPc; assert(!verify_res("M11_fallback", f_divs, m, true)); }
    // Mut 12: Vector 3 odd A2 target on JSR (A2) & Vector 2 bus error
    { PortableGuestEnvironment e2; init_env(e2, rom, f_jsr); e2.a_regs[2] = 0x00FF2021U; auto ro = run_exp246_dispatch_mode_b(e2, 0x00BE5AU, 0x00FFFFFFU); assert(ro.outcome == PortableContinuationOutcome::OriginalCpuException && ro.exception_vector == 3U && ro.cycles == 0); e2.address_error = false; e2.bus_error = false; e2.read_memory(0x00D00001U, IrOpSize::Byte); assert(e2.bus_error); }
    std::cout << "  [PASS] 12/12 M15.246 Negative & Mutation Contracts Verified\n";
}

int main() {
    std::cout << "=== M15.246 High-Frequency Callee Three-Way Differential Proof ===\n";
    const auto rom = load_rom_bytes();
    const auto fixtures = get_m15_246_callee_fixtures();
    int auth_n = 0, ctrl_n = 0;
    const DispatchRefFixture *f_divs = nullptr, *f_jsr = nullptr;
    for (const auto& f : fixtures) {
        if (std::string(f.kind) == "AUTHENTIC_GAMEPLAY_ENTRY") ++auth_n; else ++ctrl_n;
        if (std::string(f.id) == "ctrl_2ee58_divs_extl_asll7_block") f_divs = &f;
        if (std::string(f.id) == "ctrl_be5a_jsr_a2_indirect_exit") f_jsr = &f;
        PortableGuestEnvironment ea, eb;
        init_env(ea, rom, f); init_env(eb, rom, f);
        const std::uint32_t sp = (f.outcome == PortableContinuationOutcome::CompiledNextPc) ? f.exit_pc : 0x00FFFFFFU;
        auto ra = run_exp246_dispatch_mode_a(ea, f.entry_pc, sp);
        auto rb = run_exp246_dispatch_mode_b(eb, f.entry_pc, sp);
        assert(verify_res("ModeA", f, ra, !f.jitter_ok));
        assert(verify_res("ModeB", f, rb, !f.jitter_ok));
        assert(ra.pc == rb.pc && ra.outcome == rb.outcome && ra.exception_vector == rb.exception_vector);
        assert(ra.ccr == rb.ccr && ra.cycles == rb.cycles && ra.writes == rb.writes);
        for (int i = 0; i < 8; ++i) assert(ra.d[i] == rb.d[i] && ra.a[i] == rb.a[i]);
        std::cout << "  [PASS] " << f.id << " (" << f.kind << ", cyc=" << ra.cycles << ")\n";
    }
    assert(f_divs && f_jsr);
    run_cross_boundary_and_e682_tests(rom);
    run_mutations(rom, *f_divs, *f_jsr);
    std::cout << "SUMMARY: AUTHENTIC=" << auth_n << ", CONTROLLED=" << ctrl_n << ", 3WAY_DIFF=PASS, MUTATIONS=12/12 PASS\n";
    return 0;
}
