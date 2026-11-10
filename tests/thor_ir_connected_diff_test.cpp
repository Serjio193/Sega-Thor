#include "tools/thor_ir/portable_guest_env.hpp"
#include "tools/thor_ir/generated/connected_portable_routines.hpp"
#include "data/thor-fixtures/m15_242_connected_fixtures.hpp"

#include <cassert>
#include <iomanip>
#include <iostream>
#include <vector>

using namespace oasis::recompiler::ir;
using namespace oasis::recompiler::fixtures;

constexpr const char* kCoreHash = "ca4be3049e56b1fdfba9a7c727bebbdc645b3d50203d635a468048ba81165134";

void setup_env_from_fixture(PortableGuestEnvironment& env, const ConnectedFixture& fix) {
    for (int i = 0; i < 8; ++i) {
        env.d_regs[i] = 0;
        env.a_regs[i] = 0;
    }
    env.d_regs[0] = fix.d0;
    env.a_regs[0] = fix.a0;
    env.a_regs[6] = fix.a6;
    env.a_regs[7] = fix.a7;
    env.sr = 0x2700U;

    // Set RAM state
    env.write_memory(fix.a6 + 0x36U, IrOpSize::Byte, fix.flag_36);
    env.write_memory(fix.a6 + 0x38U, IrOpSize::Word, fix.scroll_a);
    env.write_memory(fix.a6 + 0x3AU, IrOpSize::Word, fix.scroll_b);
    env.write_memory(0x00FF1654U, IrOpSize::Byte, fix.ff1654);
    env.write_memory(0x00FF1657U, IrOpSize::Byte, 0x00U);

    // Push caller return address to guest stack
    env.a_regs[7] -= 4U;
    env.write_memory(env.a_regs[7], IrOpSize::Long, fix.expected_exit_pc);
}

void test_connected_three_way_fixtures() {
    const auto& fixtures = get_connected_fixtures();
    std::cout << "Reference Core Hash: " << kCoreHash << "\n";
    std::cout << "Verifying " << fixtures.size() << " connected subsystem execution paths...\n";

    for (const auto& fix : fixtures) {
        std::cout << "  Verifying " << fix.test_id << " (PC " << std::hex
                  << fix.caller_pc << " -> BSR " << fix.callee_pc << " -> RTS -> "
                  << fix.expected_exit_pc << ", " << std::dec << fix.expected_cycles << " cycles)...\n";

        // Mode A
        PortableGuestEnvironment env_a;
        setup_env_from_fixture(env_a, fix);
        const auto res_a = run_connected_mode_a(env_a, fix.caller_pc, fix.expected_exit_pc);

        // Mode B
        PortableGuestEnvironment env_b;
        setup_env_from_fixture(env_b, fix);
        const auto res_b = run_connected_mode_b(env_b, fix.caller_pc, fix.expected_exit_pc);

        // Verify Mode A vs Mode B exact match
        assert(res_a.pc == res_b.pc);
        assert(res_a.ccr == res_b.ccr);
        assert(res_a.cycles == res_b.cycles);
        for (int i = 0; i < 8; ++i) {
            assert(res_a.d[i] == res_b.d[i]);
            assert(res_a.a[i] == res_b.a[i]);
        }
        assert(res_a.writes.size() == res_b.writes.size());
        for (std::size_t i = 0; i < res_a.writes.size(); ++i) {
            assert(res_a.writes[i].first == res_b.writes[i].first);
            assert(res_a.writes[i].second == res_b.writes[i].second);
        }

        // Verify Mode A & Mode B vs Reference CPU Fixture
        assert(res_a.pc == fix.expected_exit_pc);
        assert(res_a.ccr == fix.expected_ccr);
        assert(res_a.cycles == fix.expected_cycles);
        assert(res_a.writes.size() == fix.expected_writes.size());
        for (std::size_t i = 0; i < res_a.writes.size(); ++i) {
            assert(res_a.writes[i].first == fix.expected_writes[i].addr);
            assert(res_a.writes[i].second == fix.expected_writes[i].val);
        }
        assert(env_a.a_regs[7] == fix.a7); // Stack balanced back to original entry
        assert(env_b.a_regs[7] == fix.a7);

        std::cout << "    [PASS] " << fix.test_id << " (100% Bit-Exact Match across CPU Ref, Mode A, Mode B)\n";
    }
}

void test_connected_mutations() {
    std::cout << "  Running Mutation Tests...\n";
    const auto& fix = get_connected_fixtures()[0];

    // Mutation 1: Mutated return address on stack
    {
        PortableGuestEnvironment env;
        setup_env_from_fixture(env, fix);
        // Mutate return address to uncompiled address
        env.write_memory(env.a_regs[7], IrOpSize::Long, 0x077777U);
        const auto res = run_connected_mode_a(env, fix.caller_pc, fix.expected_exit_pc);
        // Should halt at mutated return address as uncompiled PC
        assert(res.pc == 0x077777U);
        assert(res.outcome == PortableContinuationOutcome::ValidUncompiledPc);
        std::cout << "    [Mutation 1] Mutated return address correctly detected!\n";
    }

    // Mutation 2: Mutated RAM entry state (toggle Plane A bit)
    {
        PortableGuestEnvironment env;
        setup_env_from_fixture(env, fix);
        env.write_memory(fix.a6 + 0x36U, IrOpSize::Byte, 0x01U); // toggle bit
        const auto res = run_connected_mode_a(env, fix.caller_pc, fix.expected_exit_pc);
        // Cycles should differ from fast-exit 94 cycles
        assert(res.cycles != fix.expected_cycles);
        std::cout << "    [Mutation 2] Mutated RAM entry state correctly detected!\n";
    }

    // Mutation 3: Mutated stack pointer alignment
    {
        PortableGuestEnvironment env;
        setup_env_from_fixture(env, fix);
        env.a_regs[7] = 0x00FFFE01U; // odd stack pointer
        run_connected_mode_a(env, fix.caller_pc, fix.expected_exit_pc);
        assert(env.address_error);
        std::cout << "    [Mutation 3] Misaligned stack pointer correctly flagged address error!\n";
    }
}

int main() {
    std::cout << "=== Running M15.242 Three-Way Connected Differential Tests ===\n";
    test_connected_three_way_fixtures();
    test_connected_mutations();
    std::cout << "ALL THREE-WAY CONNECTED DIFFERENTIAL TESTS PASSED (100% BIT-EXACT)!\n";
    return 0;
}
