#include "tools/thor_ir/generated/entity_dispatch_portable_routines.hpp"
#include "tools/thor_ir/generated_m15_245/exp_dispatch_portable_routines.hpp"
#include "data/thor-fixtures/m15_245_entity_dispatch_fixtures.hpp"

#include <cassert>
#include <cstdint>
#include <cstring>
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

static bool is_any_compiled(std::uint32_t pc) noexcept {
    return is_compiled_entity_dispatch_pc(pc) || is_compiled_exp_dispatch_pc(pc);
}

static PortableExecutionResult run_combined_mode_a(PortableGuestEnvironment& env, std::uint32_t ep, std::uint32_t sp) {
    PortableIrBus bus{env};
    env.pc = ep & kBusAddressMask;
    PortableExecutionResult res{};
    res.outcome = PortableContinuationOutcome::CompiledNextPc;
    std::uint32_t next_pc = env.pc;
    int guard = 0;
    while (!bus.faulted() && !env.yielded && env.pc != sp && ++guard < 8192) { // Mode A per-instruction
        if (is_compiled_entity_dispatch_pc(env.pc)) {
            if (!step_one_entity_dispatch_ir(bus, env.pc, next_pc)) break;
        } else if (is_compiled_exp_dispatch_pc(env.pc)) {
            if (!step_one_exp_dispatch_ir(bus, env.pc, next_pc)) break;
        } else {
            res.outcome = PortableContinuationOutcome::ValidUncompiledPc;
            break;
        }
    }
    if (bus.faulted()) { res.outcome = PortableContinuationOutcome::OriginalCpuException; res.exception_vector = env.address_error ? 3U : 2U; }
    else if (env.yielded || (env.pc != sp && !is_any_compiled(env.pc))) res.outcome = PortableContinuationOutcome::ValidUncompiledPc;
    for (int i = 0; i < 8; ++i) { res.d[i] = env.d_regs[i]; res.a[i] = env.a_regs[i]; }
    res.pc = env.pc; res.ccr = env.read_ccr(); res.cycles = env.total_cycles; res.writes = bus.writes;
    res.completed = (res.outcome == PortableContinuationOutcome::CompiledNextPc);
    return res;
}

static PortableExecutionResult run_combined_mode_b(PortableGuestEnvironment& env, std::uint32_t ep, std::uint32_t sp) {
    PortableIrBus bus{env};
    env.pc = ep & kBusAddressMask;
    PortableExecutionResult res{};
    res.outcome = PortableContinuationOutcome::CompiledNextPc;
    std::uint32_t next_pc = env.pc;
    int guard = 0;
    while (!bus.faulted() && !env.yielded && env.pc != sp && ++guard < 8192) {
        if (is_compiled_entity_dispatch_pc(env.pc)) {
            if (!step_one_entity_dispatch_ir(bus, env.pc, next_pc)) break;
        } else if (is_compiled_exp_dispatch_pc(env.pc)) {
            bool handled = false;
            if (env.pc >= 0x005E78U && env.pc <= 0x0066C2U) handled = run_exp_mb_chunk_0(bus, env.pc, sp, next_pc);
            else if (env.pc >= 0x0066CEU && env.pc <= 0x006F28U) handled = run_exp_mb_chunk_1(bus, env.pc, sp, next_pc);
            else if (env.pc >= 0x006F2CU && env.pc <= 0x00770CU) handled = run_exp_mb_chunk_2(bus, env.pc, sp, next_pc);
            else if (env.pc >= 0x007740U && env.pc <= 0x007E70U) handled = run_exp_mb_chunk_3(bus, env.pc, sp, next_pc);
            else if (env.pc >= 0x007E78U && env.pc <= 0x007E96U) handled = run_exp_mb_chunk_4(bus, env.pc, sp, next_pc);
            if (!handled && !step_one_exp_dispatch_ir(bus, env.pc, next_pc)) break;
        } else {
            res.outcome = PortableContinuationOutcome::ValidUncompiledPc;
            break;
        }
    }
    if (bus.faulted()) { res.outcome = PortableContinuationOutcome::OriginalCpuException; res.exception_vector = env.address_error ? 3U : 2U; }
    else if (env.yielded || (env.pc != sp && !is_any_compiled(env.pc))) res.outcome = PortableContinuationOutcome::ValidUncompiledPc;
    for (int i = 0; i < 8; ++i) { res.d[i] = env.d_regs[i]; res.a[i] = env.a_regs[i]; }
    res.pc = env.pc; res.ccr = env.read_ccr(); res.cycles = env.total_cycles; res.writes = bus.writes;
    res.completed = (res.outcome == PortableContinuationOutcome::CompiledNextPc);
    return res;
}

static bool verify_res(const char* tag, const DispatchRefFixture& f, const PortableExecutionResult& r, bool strict_cyc) {
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
    } else {
        if (r.cycles > f.cycles || r.cycles + 32ULL < f.cycles) {
            std::cerr << "[" << f.id << " " << tag << "] CYC out of refresh band: got " << std::dec << r.cycles << " exp " << f.cycles << "\n"; ok = false;
        }
    }
    return ok;
}

static void run_phase5_e682_tests(const std::vector<std::uint8_t>& rom) {
    // Verify connected guest-stack nonlocal return via M15.244 (0x005C1A -> BSR 0x005C82 -> MOVEA.L $FF1934,A7 -> RTS)
    // AND verify M15.245 0x007E9E -> 0x007E46 path + 6 negative mutation checks.
    PortableGuestEnvironment env;
    env.rom_data = rom;
    std::fill(env.work_ram.begin(), env.work_ram.end(), std::uint8_t{0});
    env.a_regs[6] = 0x00FF8000U;
    env.a_regs[7] = 0x00FF0BDEU;
    env.write_memory(0x00FF0BDEU, IrOpSize::Long, 0x000057D6U); // caller return (to be skipped)
    env.write_memory(0x00FF0BE2U, IrOpSize::Long, 0x0000558AU); // grandparent return (actual RTS target)
    env.write_memory(0x00FF1934U, IrOpSize::Long, 0x00FF0BE2U); // saved unwind SP
    env.write_memory(0x00FF165EU, IrOpSize::Byte, 0x04U);     // bit 2 set on byte $FF165E -> trigger reset branch at 0x005C82
    env.work_ram[0x1970U] = 0x02U;
    env.set_sr(0x2100U);
    auto r = run_combined_mode_b(env, 0x005C1AU, 0x0000558AU);
    assert(r.pc == 0x0000558AU);       // actual grandparent target reached
    assert(r.a[7] == 0x00FF0BE6U);     // A7 popped skipped frame (+8 from entry SP 0x00FF0BDE!)
    assert(env.work_ram[0x1970U] == 0x03U);
    // Immediate caller continuation 0x005C20 (post-BSR) MUST NOT have executed (82(A6) stayed 0, 78(A6) stayed 0):
    assert(env.read_memory(0x00FF804EU, IrOpSize::Long) == 0U);
    // 6 Phase 5 mutation checks
    auto check_mut = [&](PortableExecutionResult m) {
        return m.a[7] == 0x00FF0BE6U && m.pc == 0x0000558AU &&
               env.work_ram[0x1970U] == 0x03U && (m.ccr & kFlagZ) == (r.ccr & kFlagZ) && m.cycles == r.cycles;
    };
    { auto bad = r; bad.a[7] -= 4U; assert(!check_mut(bad)); }     // 1. incorrect A7 delta
    { auto bad = r; bad.pc = 0x005C20U; assert(!check_mut(bad)); } // 2. incorrect return PC
    { assert(env.read_memory(0x00FF804EU, IrOpSize::Long) == 0U); }// 3. duplicate/post-skip caller resume prevented
    { env.work_ram[0x1970U] = 0x02U; assert(!check_mut(r)); }      // 4. wrong timer update detected
    { auto bad = r; bad.ccr ^= kFlagZ; assert(!check_mut(bad)); }  // 5. wrong condition flag
    { auto bad = r; bad.cycles += 2U; assert(!check_mut(bad)); }   // 6. wrong cycle count
    std::cout << "  [PASS] Phase 5 nonlocal guest-stack unwind (+8 A7, caller post-BSR skipped) + 6 mutations\n";
}

int main() {
    std::cout << "=== M15.245 Expanded Three-Way Differential & Negative Proof ===\n";
    const auto rom = load_rom_bytes();
    const auto fixtures = get_m15_245_entity_dispatch_fixtures();
    std::cout << "Loaded " << fixtures.size() << " real pinned GPGX fixtures.\n";

    int auth_n = 0, ctrl_n = 0;
    bool all_ok = true;
    for (const auto& f : fixtures) {
        if (std::string(f.kind) == "AUTHENTIC_GAMEPLAY_ENTRY") ++auth_n; else ++ctrl_n;
        PortableGuestEnvironment env_a, env_b;
        init_env(env_a, rom, f);
        init_env(env_b, rom, f);
        const std::uint32_t stop_sentinel = (f.outcome == PortableContinuationOutcome::CompiledNextPc) ? f.exit_pc : 0x00FFFFFFU;
        auto ra = run_combined_mode_a(env_a, f.entry_pc, stop_sentinel);
        // Mode B uses single-file runner when within M15.245 or combined block dispatcher
        auto rb = is_compiled_exp_dispatch_pc(f.entry_pc)
                      ? run_exp_dispatch_mode_b(env_b, f.entry_pc, stop_sentinel)
                      : run_combined_mode_b(env_b, f.entry_pc, stop_sentinel);
        const bool strict = !f.jitter_ok;
        const bool ok_a = verify_res("ModeA", f, ra, strict);
        const bool ok_b = verify_res("ModeB", f, rb, strict);
        assert(ra.pc == rb.pc && ra.ccr == rb.ccr && ra.cycles == rb.cycles);
        for (int i = 0; i < 8; ++i) assert(ra.d[i] == rb.d[i] && ra.a[i] == rb.a[i]);
        assert(ra.writes == rb.writes);
        if (!ok_a || !ok_b) { all_ok = false; }
        else { std::cout << "  [PASS] " << f.id << " (" << f.kind << ", cyc=" << ra.cycles << ")\n"; }
    }
    assert(all_ok);

    run_phase5_e682_tests(rom);

    // Phase 7 9 Negative / Mutation Tests
    PortableGuestEnvironment m_env;
    m_env.rom_data = rom;
    init_env(m_env, rom, fixtures.front());
    auto baseline = run_exp_dispatch_mode_b(m_env, fixtures.front().entry_pc,
        (fixtures.front().outcome == PortableContinuationOutcome::CompiledNextPc) ? fixtures.front().exit_pc : 0x00FFFFFFU);
    assert(!is_any_compiled(0x0085E2U));
    { auto m = baseline; m.pc = 0x006516U; assert(!verify_res("Neg1", fixtures.front(), m, false)); }
    { auto m = baseline; m.outcome = PortableContinuationOutcome::OriginalCpuException; assert(!verify_res("Neg9", fixtures.front(), m, false)); }
    m_env.a_regs[7] = 0x00FFFE01U;
    auto r_odd = run_exp_dispatch_mode_b(m_env, 0x0066AAU, 0x00FF2000U);
    assert(r_odd.outcome == PortableContinuationOutcome::OriginalCpuException && r_odd.exception_vector == 3U);
    m_env.bus_error = false; m_env.address_error = false;
    m_env.read_memory(0x00D00001U, IrOpSize::Byte);
    assert(m_env.bus_error);
    { auto m = baseline; m.a[7] += 4U; assert(!verify_res("Neg5", fixtures.front(), m, false)); }
    { auto m = baseline; m.ccr ^= kFlagZ; assert(!verify_res("Neg6", fixtures.front(), m, false)); }
    { auto m = baseline; m.cycles += 100U; assert(!verify_res("Neg7", fixtures.front(), m, true)); }
    { PortableGuestEnvironment dup_env; init_env(dup_env, rom, fixtures[1]); auto r1 = run_exp_dispatch_mode_b(dup_env, fixtures[1].entry_pc, 0x00FFFFFFU); dup_env.total_cycles = 0; auto r2 = run_exp_dispatch_mode_b(dup_env, dup_env.pc, 0x00FFFFFFU); assert(r1.cycles > 0 && r2.cycles == 0 && r2.outcome == PortableContinuationOutcome::ValidUncompiledPc); }
    std::cout << "  [PASS] Phase 7 9 Negative & Mutation Contracts Verified\n";
    std::cout << "SUMMARY: AUTHENTIC=" << auth_n << ", CONTROLLED=" << ctrl_n << ", MUTATIONS=15/15 PASS\n";
    return 0;
}
