#include "tools/thor_ir/portable_guest_env.hpp"
#include "tools/thor_ir/generated/portable_routines.hpp"
#include "data/thor-fixtures/m15_241_cpu_reference_fixtures.hpp"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

#define TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion failed at " << __FILE__ << ":" << __LINE__ << ": " #cond << "\n"; \
            std::exit(1); \
        } \
    } while (false)

namespace {

using namespace oasis::recompiler::ir;
using namespace oasis::recompiler::fixtures;

void run_single_fixture_test(const CpuFixture& fix) {
    PortableGuestEnvironment env_a;
    PortableGuestEnvironment env_b;

    // Initialize environment from reference fixture inputs
    auto setup_env = [&](PortableGuestEnvironment& env) {
        env.d_regs[0] = fix.d0;
        env.d_regs[2] = fix.d2;
        env.a_regs[6] = fix.a6 & kBusAddressMask;
        env.a_regs[7] = fix.a7 & kBusAddressMask;
        env.pc = fix.entry_pc;
        for (const auto& ram : fix.initial_ram) {
            env.write_memory(ram.addr, IrOpSize::Byte, ram.val);
        }
    };

    setup_env(env_a);
    setup_env(env_b);

    PortableExecutionResult res_a{};
    PortableExecutionResult res_b{};

    if (fix.entry_pc == 0x00E682U) {
        res_a = run_mode_a_00e682(env_a);
        res_b = run_mode_b_00e682(env_b);
    } else if (fix.entry_pc == 0x016294U) {
        res_a = run_mode_a_016294(env_a);
        res_b = run_mode_b_016294(env_b);
    } else if (fix.entry_pc == 0x006E7AU) {
        res_a = run_mode_a_006e7a(env_a);
        res_b = run_mode_b_006e7a(env_b);
    } else {
        std::cerr << "Unknown fixture entry PC: " << std::hex << fix.entry_pc << "\n";
        std::exit(1);
    }

    // 1. Verify Mode A against Reference CPU Fixture
    TEST_ASSERT(res_a.pc == fix.expected_exit_pc);
    TEST_ASSERT(res_a.d[0] == fix.exp_d0);
    TEST_ASSERT(res_a.d[2] == fix.exp_d2);
    TEST_ASSERT(res_a.a[6] == fix.exp_a6);
    TEST_ASSERT(res_a.a[7] == fix.exp_a7);
    TEST_ASSERT((res_a.ccr & 0x1FU) == (fix.exp_ccr & 0x1FU));
    if (res_a.cycles != fix.expected_cycles) {
        std::cerr << "Cycle mismatch in " << fix.test_id << ": got " << res_a.cycles << ", expected " << fix.expected_cycles << "\n";
    }
    TEST_ASSERT(res_a.cycles == fix.expected_cycles);
    TEST_ASSERT(res_a.writes.size() == fix.expected_writes.size());
    for (std::size_t i = 0; i < fix.expected_writes.size(); ++i) {
        TEST_ASSERT(res_a.writes[i].first == fix.expected_writes[i].addr);
        TEST_ASSERT(res_a.writes[i].second == fix.expected_writes[i].val);
    }

    // 2. Verify Mode B against Reference CPU Fixture
    TEST_ASSERT(res_b.pc == fix.expected_exit_pc);
    TEST_ASSERT(res_b.d[0] == fix.exp_d0);
    TEST_ASSERT(res_b.d[2] == fix.exp_d2);
    TEST_ASSERT(res_b.a[6] == fix.exp_a6);
    TEST_ASSERT(res_b.a[7] == fix.exp_a7);
    TEST_ASSERT((res_b.ccr & 0x1FU) == (fix.exp_ccr & 0x1FU));
    if (res_b.cycles != fix.expected_cycles) {
        std::cerr << "Mode B Cycle mismatch in " << fix.test_id << ": got " << res_b.cycles << ", expected " << fix.expected_cycles << "\n";
    }
    TEST_ASSERT(res_b.cycles == fix.expected_cycles);
    TEST_ASSERT(res_b.writes.size() == fix.expected_writes.size());
    for (std::size_t i = 0; i < fix.expected_writes.size(); ++i) {
        TEST_ASSERT(res_b.writes[i].first == fix.expected_writes[i].addr);
        TEST_ASSERT(res_b.writes[i].second == fix.expected_writes[i].val);
    }

    // 3. Bit-Exact Equivalence between Mode A and Mode B
    TEST_ASSERT(res_a.pc == res_b.pc);
    TEST_ASSERT(res_a.d[0] == res_b.d[0]);
    TEST_ASSERT(res_a.ccr == res_b.ccr);
    TEST_ASSERT(res_a.cycles == res_b.cycles);
    TEST_ASSERT(res_a.writes == res_b.writes);

    std::cout << "  [PASS] Fixture " << fix.test_id << " (PC 0x" << std::hex << fix.entry_pc
              << " -> 0x" << fix.expected_exit_pc << ", " << std::dec << fix.expected_cycles << " cycles)\n";
}

void run_mutation_tests() {
    std::cout << "  Running Mutation Tests...\n";
    const auto& fixtures = get_cpu_fixtures();
    const auto& fix = fixtures[0]; // 00e682 normal unclamped

    // Mutation 1: Mutated entry register (D2 != 0 triggers early branch)
    {
        PortableGuestEnvironment env;
        env.d_regs[0] = fix.d0;
        env.d_regs[2] = 0x55U; // Mutate D2
        env.a_regs[6] = fix.a6;
        env.a_regs[7] = fix.a7;
        for (const auto& r : fix.initial_ram) env.write_memory(r.addr, IrOpSize::Byte, r.val);
        const auto res = run_mode_a_00e682(env);
        TEST_ASSERT(res.cycles != fix.expected_cycles || res.writes.size() != fix.expected_writes.size());
        std::cout << "    [Mutation 1] Mutated entry register correctly detected!\n";
    }

    // Mutation 2: Mutated Initial RAM value
    {
        PortableGuestEnvironment env;
        env.d_regs[0] = fix.d0;
        env.d_regs[2] = fix.d2;
        env.a_regs[6] = fix.a6;
        env.a_regs[7] = fix.a7;
        for (const auto& r : fix.initial_ram) env.write_memory(r.addr, IrOpSize::Byte, r.val ^ 0xFF);
        const auto res = run_mode_b_00e682(env);
        TEST_ASSERT(res.pc != fix.expected_exit_pc || res.cycles != fix.expected_cycles);
        std::cout << "    [Mutation 2] Mutated RAM entry state correctly detected!\n";
    }
}

} // namespace

int main() {
    std::cout << "=== Running M15.241 Three-Way Reference Independence Differential Tests ===\n";
    std::cout << "Reference Core Hash: " << kReferenceCoreHash << "\n";

    const auto& fixtures = get_cpu_fixtures();
    std::cout << "Total frozen fixtures to verify: " << fixtures.size() << "\n";

    for (const auto& fix : fixtures) {
        run_single_fixture_test(fix);
    }

    run_mutation_tests();

    std::cout << "ALL THREE-WAY DIFFERENTIAL TESTS PASSED (100% BIT-EXACT ACROSS CPU REF, MODE A, MODE B)!\n";
    return 0;
}
