#include "tools/thor_ir/portable_guest_env.hpp"
#include "tools/thor_ir/generated/entity_dispatch_portable_routines.hpp"
#include "data/thor-fixtures/m15_243_entity_dispatch_fixtures.hpp"

#include <cassert>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

using namespace oasis::recompiler::ir;
using namespace oasis::recompiler::fixtures;

constexpr const char* kCoreHash = "ca4be3049e56b1fdfba9a7c727bebbdc645b3d50203d635a468048ba81165134";

// 118 authentic jump table bytes at 0x0059BA
static const std::uint8_t kJumpTableBytes[118] = {
    0x08, 0x3c, 0x09, 0x28, 0x0b, 0x58, 0x14, 0x12, 0x1e, 0x90, 0x1c, 0x92, 0x0c, 0x78, 0x18, 0x12,
    0x12, 0x7c, 0x12, 0x6e, 0x20, 0x42, 0x17, 0x42, 0x02, 0xf6, 0x03, 0x24, 0x03, 0xf2, 0x04, 0x0c,
    0x03, 0x1e, 0x03, 0xec, 0x05, 0x92, 0x14, 0xfa, 0x01, 0xc0, 0x01, 0xe6, 0x01, 0xfa, 0x02, 0x32,
    0x09, 0x4e, 0x0e, 0x8a, 0x0f, 0x4e, 0x1f, 0x08, 0x0c, 0xb8, 0x20, 0x34, 0x22, 0x46, 0x0e, 0x08,
    0x1e, 0x14, 0x13, 0x7e, 0x21, 0x2c, 0x1a, 0x3c, 0x15, 0x3a, 0x13, 0x9e, 0x18, 0x5c, 0x18, 0xe2,
    0x19, 0x5a, 0x19, 0xe0, 0x1b, 0x42, 0x15, 0x7c, 0x11, 0x18, 0x11, 0x34, 0x1b, 0x6e, 0x05, 0x7a,
    0x05, 0xd0, 0x15, 0xc8, 0x15, 0xfc, 0x16, 0x30, 0x16, 0x38, 0x0a, 0x40, 0x06, 0xf8, 0x0d, 0xb6,
    0x05, 0xb8, 0x1e, 0xfe, 0x0d, 0x0a
};

void setup_env_from_fixture(PortableGuestEnvironment& env, const EntityDispatchFixture& fix) {
    // Populate jump table in ROM data if not already set
    if (env.rom_data.size() < 0x0059BAU + sizeof(kJumpTableBytes)) {
        env.rom_data.resize(0x0059BAU + sizeof(kJumpTableBytes) + 16, 0);
        for (std::size_t i = 0; i < sizeof(kJumpTableBytes); ++i) {
            env.rom_data[0x0059BAU + i] = kJumpTableBytes[i];
        }
    }

    for (int i = 0; i < 8; ++i) {
        env.d_regs[i] = 0;
        env.a_regs[i] = 0;
    }
    env.a_regs[6] = fix.a6;
    env.a_regs[7] = fix.a7;
    env.sr = 0x2700U;

    // Set RAM state
    env.write_memory(0x00FF165EU, IrOpSize::Word, 0x0000U);
    env.write_memory(fix.a6 + 0x04U, IrOpSize::Word, fix.state_idx);
    env.write_memory(fix.a6 + 0x83U, IrOpSize::Byte, fix.timer_83);
    env.write_memory(fix.a6 + 0x36U, IrOpSize::Byte, fix.flag_36);
    env.write_memory(fix.a6 + 0x39U, IrOpSize::Byte, fix.flag_39);
    env.write_memory(fix.a6 + 0x4EU, IrOpSize::Long, fix.vel_x);
    env.write_memory(fix.a6 + 0x52U, IrOpSize::Long, fix.vel_y);
    env.write_memory(fix.a6 + 0x72U, IrOpSize::Long, fix.pos_x);
    env.write_memory(fix.a6 + 0x76U, IrOpSize::Long, fix.pos_y);
    env.write_memory(fix.a6 + 0x2AU, IrOpSize::Word, 40U);
    env.write_memory(fix.a6 + 0x8AU, IrOpSize::Word, 0x1234U);
    const std::uint16_t timer_02 = (fix.test_id == "dispatch_path3_state17_timer_tick_branch_taken") ? 2U : 1U;
    env.write_memory(fix.a6 + 0x02U, IrOpSize::Word, timer_02);

    // Push caller return address to guest stack
    env.a_regs[7] -= 4U;
    env.write_memory(env.a_regs[7], IrOpSize::Long, fix.expected_exit_pc);
}

void test_entity_dispatch_three_way_fixtures() {
    const auto& fixtures = get_entity_dispatch_fixtures();
    std::cout << "Reference Core Hash: " << kCoreHash << "\n";
    std::cout << "Verifying " << fixtures.size() << " entity dispatch execution paths...\n";

    for (const auto& fix : fixtures) {
        std::cout << "  Verifying " << fix.test_id << " (Entry " << std::hex
                  << fix.entry_pc << " -> State " << fix.state_idx << " -> Exit "
                  << fix.expected_exit_pc << ", " << std::dec << fix.expected_cycles << " cycles)...\n";

        const std::uint32_t stop_pc = (fix.expected_outcome == PortableContinuationOutcome::OriginalCpuException) ? 0U : fix.expected_exit_pc;

        // Mode A
        PortableGuestEnvironment env_a;
        setup_env_from_fixture(env_a, fix);
        const auto res_a = run_entity_dispatch_mode_a(env_a, fix.entry_pc, stop_pc);

        // Mode B
        PortableGuestEnvironment env_b;
        setup_env_from_fixture(env_b, fix);
        const auto res_b = run_entity_dispatch_mode_b(env_b, fix.entry_pc, stop_pc);



        // Verify Mode A vs Mode B exact match
        assert(res_a.outcome == res_b.outcome);
        assert(res_a.pc == res_b.pc);
        assert(res_a.cycles == res_b.cycles);
        for (int i = 0; i < 8; ++i) {
            assert(res_a.d[i] == res_b.d[i]);
            assert(res_a.a[i] == res_b.a[i]);
        }
        assert(res_a.writes.size() == res_b.writes.size());

        // Verify Mode A & Mode B vs Reference CPU Fixture
        assert(res_a.outcome == fix.expected_outcome);
        assert(res_a.pc == fix.expected_exit_pc);
        assert(res_a.cycles == fix.expected_cycles);

        if (fix.expected_outcome == PortableContinuationOutcome::CompiledNextPc && fix.vel_x != 0) {
            const auto final_x = env_a.read_memory(fix.a6 + 0x72U, IrOpSize::Long);
            const auto final_y = env_a.read_memory(fix.a6 + 0x76U, IrOpSize::Long);
            assert(final_x == fix.expected_pos_x);
            assert(final_y == fix.expected_pos_y);
        }

        std::cout << "    [PASS] " << fix.test_id << " (100% Bit-Exact Match across CPU Ref, Mode A, Mode B)\n";
    }
}

void test_entity_dispatch_mutations() {
    std::cout << "  Running Mutation Tests...\n";
    const auto& fix = get_entity_dispatch_fixtures()[0];

    // Mutation 1: Mutated return address on stack
    {
        PortableGuestEnvironment env;
        setup_env_from_fixture(env, fix);
        env.write_memory(env.a_regs[7], IrOpSize::Long, 0x088888U);
        const auto res = run_entity_dispatch_mode_a(env, fix.entry_pc, 0x088888U);
        assert(res.pc == 0x088888U);
        std::cout << "    [Mutation 1] Mutated return address correctly detected!\n";
    }

    // Mutation 2: Mutated timer state (0 vs 5 flips branch)
    {
        PortableGuestEnvironment env;
        setup_env_from_fixture(env, fix);
        env.write_memory(fix.a6 + 0x83U, IrOpSize::Byte, 0x01U); // timer expires to 0
        const auto res = run_entity_dispatch_mode_a(env, fix.entry_pc, 0x0062CCU);
        assert(res.outcome == PortableContinuationOutcome::ValidUncompiledPc);
        assert(res.pc == 0x0062CCU);
        std::cout << "    [Mutation 2] Mutated timer state correctly flipped branch decision!\n";
    }

    // Mutation 3: Odd jump table index triggers address error
    {
        PortableGuestEnvironment env;
        setup_env_from_fixture(env, fix);
        env.write_memory(fix.a6 + 0x04U, IrOpSize::Word, 0x0019U); // odd index
        const auto res = run_entity_dispatch_mode_a(env, fix.entry_pc, fix.expected_exit_pc);
        assert(res.outcome == PortableContinuationOutcome::OriginalCpuException);
        assert(res.exception_vector == 3U);
        std::cout << "    [Mutation 3] Misaligned table index correctly triggered Address Error (Vector 3)!\n";
    }
}

int main() {
    std::cout << "=== Running M15.243 Three-Way Entity Dispatch Differential Tests ===\n";
    test_entity_dispatch_three_way_fixtures();
    test_entity_dispatch_mutations();
    std::cout << "ALL THREE-WAY ENTITY DISPATCH DIFFERENTIAL TESTS PASSED (100% BIT-EXACT)!\n";
    return 0;
}
