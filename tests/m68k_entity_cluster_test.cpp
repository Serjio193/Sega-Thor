#include "tools/hybrid/m68k_entity_cluster_coordinator.hpp"
#include "tools/hybrid/m68k_entity_cluster_context.hpp"
#include "tools/hybrid/m68k_entity_cluster_dispatch.hpp"
#include "tools/hybrid/m68k_entity_cluster_metrics.hpp"
#include "tools/hybrid/m68k_entity_cluster_state0.hpp"
#include "tools/hybrid/m68k_entity_cluster_target10.hpp"
#include "tools/hybrid/m68k_entity_cluster_target11_12.hpp"
#include "tools/hybrid/m68k_02575c_instruction_step.hpp"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <map>

#define TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion failed at " << __FILE__ << ":" << __LINE__ << ": " #cond << "\n"; \
            std::exit(1); \
        } \
    } while (false)

namespace {
using namespace oasis::hybrid;
using namespace oasis::hybrid::entity_cluster;

std::array<std::uint32_t, 18> g_regs{};
std::map<std::uint32_t, std::uint8_t> g_mem{};
int g_cycle_adjustment{};
unsigned g_last_return_pc{};

std::uint32_t mock_read_reg(void*, unsigned reg) { return reg < g_regs.size() ? g_regs[reg] : 0U; }
void mock_write_reg(void*, unsigned reg, std::uint32_t val) { if (reg < g_regs.size()) g_regs[reg] = val; }
std::uint32_t mock_read_mem(void*, unsigned addr, int width) {
    addr &= 0xFFFFFFU; std::uint32_t val = 0U;
    for (int i = 0; i < width; ++i) val = (val << 8U) | g_mem[addr + i];
    return val;
}
void mock_write_mem(void*, unsigned addr, int width, std::uint32_t val) {
    addr &= 0xFFFFFFU;
    for (int i = width - 1; i >= 0; --i) { g_mem[addr + i] = static_cast<std::uint8_t>(val & 0xFFU); val >>= 8U; }
}
int mock_peek(void*, unsigned addr) { return g_mem[addr & 0xFFFFFFU]; }
void mock_set_return_state(void*, unsigned pc, unsigned, unsigned) { g_last_return_pc = pc; }
std::uint32_t mock_fetch16(void*) { return 0x4E71U; }
void mock_begin_inst(void*, unsigned) {}
void mock_finish_inst(void*, unsigned) {}
void mock_add_cycles(void*, int c) { g_cycle_adjustment += c; }
int mock_word_not_taken(void*) { return -2; }
void mock_observe(void*, unsigned, unsigned) {}

M68k9D70StepApi make_test_api() {
    M68k9D70StepApi api{};
    api.read_register = mock_read_reg; api.write_register = mock_write_reg;
    api.peek = mock_peek; api.read = mock_read_mem; api.write = mock_write_mem;
    api.fetch16 = mock_fetch16; api.set_return_state = mock_set_return_state;
    api.begin_instruction = mock_begin_inst; api.finish_instruction = mock_finish_inst;
    api.add_cycles = mock_add_cycles; api.word_not_taken_adjustment = mock_word_not_taken;
    api.observe_instruction = mock_observe;
    return api;
}

void step_sequence(const M68k9D70StepApi& api, std::initializer_list<unsigned> pcs) {
    for (unsigned pc : pcs) {
        TEST_ASSERT(step_m68k_02575c(pc, api, true) == M68k9D70StepResult::InstructionStepped);
    }
}

void reset_environment() {
    g_regs.fill(0U); g_mem.clear(); g_cycle_adjustment = 0; g_last_return_pc = 0U;
    reset_cluster_metrics(); reset_02575c_step_metrics();
    constexpr std::array<std::uint16_t, 6> kDisp{{0x302EU, 0x0004U, 0x41FBU, 0x0006U, 0xD0D0U, 0x4ED0U}};
    for (std::size_t i = 0; i < kDisp.size(); ++i) mock_write_mem(nullptr, 0x02575CU + static_cast<unsigned>(i*2), 2, kDisp[i]);
    constexpr std::array<std::int16_t, 13> kOffsets{{0x012E, 0x0136, 0x01D4, 0x01FA, 0x023C, 0x025C, 0x02FE, 0x03AC, 0x000A, 0x0058, 0x0072, 0x00C0, 0x0100}};
    for (std::size_t i = 0; i < kOffsets.size(); ++i) mock_write_mem(nullptr, kJumpTableBasePc + static_cast<unsigned>(i*2), 2, static_cast<std::uint16_t>(kOffsets[i]));
    mock_write_mem(nullptr, 0x00F0D2U, 2, 0x4E75U);
    mock_write_mem(nullptr, 0x00BCD8U, 2, 0x4E75U);
    mock_write_mem(nullptr, 0x00E502U, 2, 0x4E75U);
    mock_write_mem(nullptr, 0x00F476U, 2, 0x4E75U);
    mock_write_mem(nullptr, 0x00E66EU, 2, 0x4E75U);
    mock_write_mem(nullptr, 0x00E338U, 2, 0x4E75U);
    mock_write_mem(nullptr, 0x00B852U, 2, 0x4E75U);
    mock_write_mem(nullptr, 0x0255D8U, 2, 0x4E71U);
    mock_write_mem(nullptr, 0x00E7E8U, 2, 0x4E71U);
}

void test_jump_table_resolution() {
    reset_environment(); const auto api = make_test_api(); const GuestExecutionContext ctx(api);
    constexpr std::array<unsigned, 13> kExpected{{0x025896U, 0x0258A0U, 0x025940U, 0x025968U, 0x0259ACU, 0x0259CEU, 0x025A72U, 0x025B22U, 0x025782U, 0x0257D2U, 0x0257EEU, 0x02583EU, 0x025880U}};
    for (unsigned idx = 0; idx < 13; ++idx) {
        const auto res = calculate_jump_target(ctx, static_cast<std::uint16_t>(idx * 2U));
        TEST_ASSERT(res.is_valid && res.target_pc == kExpected[idx]);
    }
    TEST_ASSERT(!calculate_jump_target(ctx, 1U).is_valid && !calculate_jump_target(ctx, 26U).is_valid);
    std::cout << "[PASS] test_jump_table_resolution\n";
}

void test_runtime_bridge_state0_rts_success() {
    reset_environment(); const auto api = make_test_api();
    constexpr unsigned kA6 = 0xFF2000U, kSp = 0xFF7FFCU, kRet = 0x00DF1EU;
    g_regs[14] = kA6; g_regs[15] = kSp;
    mock_write_mem(nullptr, kSp, 4, kRet); mock_write_mem(nullptr, kA6 + 4U, 2, 0x0000U); mock_write_mem(nullptr, kA6 + 2U, 2, 5U);
    step_sequence(api, {0x02575CU, 0x025760U, 0x025764U, 0x025766U, 0x025896U, 0x02589AU, 0x02589EU});
    TEST_ASSERT(g_regs[15] == 0xFF8000U && g_last_return_pc == kRet && mock_read_mem(nullptr, kA6 + 2U, 2) == 4U);
    const auto& m = get_02575c_step_metrics();
    TEST_ASSERT(m.native_calls == 1 && m.stepped_instructions == 7 && m.state0_rts_exits == 1);
    std::cout << "[PASS] test_runtime_bridge_state0_rts_success\n";
}

void test_runtime_bridge_state0_loopback_success() {
    reset_environment(); const auto api = make_test_api();
    constexpr unsigned kA6 = 0xFF2000U, kSp = 0xFF7FFCU;
    g_regs[14] = kA6; g_regs[15] = kSp;
    mock_write_mem(nullptr, kA6 + 4U, 2, 0x0000U); mock_write_mem(nullptr, kA6 + 2U, 2, 1U);
    step_sequence(api, {0x02575CU, 0x025760U, 0x025764U, 0x025766U, 0x025896U, 0x02589AU});
    TEST_ASSERT(g_regs[15] == kSp && g_last_return_pc == 0x0255D8U && mock_read_mem(nullptr, kA6 + 2U, 2) == 0U);
    const auto& m = get_02575c_step_metrics();
    TEST_ASSERT(m.native_calls == 1 && m.stepped_instructions == 6 && m.state0_loopback_exits == 1);
    std::cout << "[PASS] test_runtime_bridge_state0_loopback_success\n";
}

void test_runtime_bridge_state1_child_and_rts() {
    reset_environment(); const auto api = make_test_api();
    constexpr unsigned kA6 = 0xFF2000U, kSp = 0xFF7FFCU, kRet = 0x00DF1EU;
    g_regs[14] = kA6; g_regs[15] = kSp;
    mock_write_mem(nullptr, kSp, 4, kRet); mock_write_mem(nullptr, kA6 + 4U, 2, 0x0002U);
    mock_write_mem(nullptr, kA6 + 48U, 2, 2U); // 48(a6) <= 4 -> exit RTS 1
    step_sequence(api, {0x02575CU, 0x025760U, 0x025764U, 0x025766U, 0x0258A0U, 0x0258A2U});
    TEST_ASSERT(g_last_return_pc == 0x00BCD8U && g_regs[15] == kSp - 4U);
    g_regs[15] = kSp; // child returns
    step_sequence(api, {0x0258A8U, 0x0258ACU, 0x0258B0U, 0x0258B6U, 0x0258BAU});
    TEST_ASSERT(g_regs[15] == 0xFF8000U && g_last_return_pc == kRet);
    const auto& m = get_02575c_step_metrics();
    TEST_ASSERT(m.state1_calls == 1 && m.state1_child_calls == 1 && m.state1_rts_exits == 1);
    std::cout << "[PASS] test_runtime_bridge_state1_child_and_rts\n";
}

void test_runtime_bridge_state2_rts() {
    reset_environment(); const auto api = make_test_api();
    constexpr unsigned kA6 = 0xFF2000U, kSp = 0xFF7FFCU, kRet = 0x00DF1EU;
    g_regs[14] = kA6; g_regs[15] = kSp;
    mock_write_mem(nullptr, kSp, 4, kRet); mock_write_mem(nullptr, kA6 + 4U, 2, 0x0004U);
    mock_write_mem(nullptr, kA6 + 131U, 1, 3U); // counter > 1 -> decrements to 2 != 0 -> RTS
    step_sequence(api, {0x02575CU, 0x025760U, 0x025764U, 0x025766U, 0x025940U, 0x025944U, 0x025946U, 0x02594CU, 0x025950U, 0x025954U, 0x025958U, 0x02595CU});
    TEST_ASSERT(g_regs[15] == 0xFF8000U && g_last_return_pc == kRet);
    const auto& m = get_02575c_step_metrics();
    TEST_ASSERT(m.state2_calls == 1 && m.state2_rts_exits == 1);
    std::cout << "[PASS] test_runtime_bridge_state2_rts\n";
}

void test_runtime_bridge_state7_rts() {
    reset_environment(); const auto api = make_test_api();
    constexpr unsigned kA6 = 0xFF2000U, kSp = 0xFF7FFCU, kRet = 0x00DF1EU;
    g_regs[14] = kA6; g_regs[15] = kSp;
    mock_write_mem(nullptr, kSp, 4, kRet); mock_write_mem(nullptr, kA6 + 4U, 2, 0x000EU);
    mock_write_mem(nullptr, kA6 + 55U, 1, 0x00U); // bit 1 == 0 -> RTS
    step_sequence(api, {0x02575CU, 0x025760U, 0x025764U, 0x025766U, 0x025B22U, 0x025B28U, 0x025B30U});
    TEST_ASSERT(g_regs[15] == 0xFF8000U && g_last_return_pc == kRet);
    const auto& m = get_02575c_step_metrics();
    TEST_ASSERT(m.state7_calls == 1 && m.state7_rts_exits == 1);
    std::cout << "[PASS] test_runtime_bridge_state7_rts\n";
}

void test_runtime_bridge_target10_child_and_rts() {
    reset_environment(); const auto api = make_test_api();
    constexpr unsigned kA6 = 0xFF2000U, kSp = 0xFF7FFCU, kRet = 0x00DF1EU;
    g_regs[14] = kA6; g_regs[15] = kSp;
    mock_write_mem(nullptr, kSp, 4, kRet); mock_write_mem(nullptr, kA6 + 4U, 2, 0x0014U);
    mock_write_mem(nullptr, 0x00FF1A20U, 1, 0x00U); mock_write_mem(nullptr, 0x00FF19FCU, 2, 10U);
    mock_write_mem(nullptr, kA6 + 20U, 2, 10U); mock_write_mem(nullptr, 0x00FF19F0U, 2, 100U);
    mock_write_mem(nullptr, 0x00FF19F4U, 2, 200U); mock_write_mem(nullptr, kA6 + 8U, 2, 100U);
    mock_write_mem(nullptr, kA6 + 12U, 2, 150U);
    step_sequence(api, {0x02575CU, 0x025760U, 0x025764U, 0x025766U, 0x0257EEU, 0x0257F6U, 0x0257FAU, 0x025800U, 0x025804U, 0x025808U, 0x02580EU, 0x025814U, 0x025818U, 0x02581CU});
    TEST_ASSERT(g_last_return_pc == 0x00F0D2U && g_regs[15] == kSp - 4U);
    g_regs[15] = kSp;
    g_regs[0] = 0x0040U;
    step_sequence(api, {0x025822U, 0x025826U, 0x02583CU});
    TEST_ASSERT(g_regs[15] == 0xFF8000U && g_last_return_pc == kRet);
    const auto& m = get_02575c_step_metrics();
    TEST_ASSERT(m.target10_calls == 1 && m.target10_child_calls == 1 && m.target10_rts_exits == 1);
    std::cout << "[PASS] test_runtime_bridge_target10_child_and_rts\n";
}

void test_runtime_bridge_target11_non_negative_rts() {
    reset_environment(); const auto api = make_test_api();
    constexpr unsigned kA6 = 0xFF2000U, kSp = 0xFF7FFCU, kRet = 0x00DF1EU;
    g_regs[14] = kA6; g_regs[15] = kSp;
    mock_write_mem(nullptr, kSp, 4, kRet); mock_write_mem(nullptr, kA6 + 4U, 2, 0x0016U);
    mock_write_mem(nullptr, kA6 + 86U, 4, 0x00001000U); mock_write_mem(nullptr, kA6 + 16U, 4, 0x00005000U);
    step_sequence(api, {0x02575CU, 0x025760U, 0x025764U, 0x025766U, 0x02583EU, 0x025842U, 0x025846U, 0x02584AU, 0x025850U, 0x025854U});
    TEST_ASSERT(mock_read_mem(nullptr, kA6 + 16U, 4) == 0x00004000U && mock_read_mem(nullptr, kA6 + 86U, 4) == 0x0000D000U);
    const auto& m = get_02575c_step_metrics();
    TEST_ASSERT(m.target11_calls == 1 && m.target11_rts_exits == 1);
    std::cout << "[PASS] test_runtime_bridge_target11_non_negative_rts\n";
}

void test_runtime_bridge_target11_negative_tail_jump() {
    reset_environment(); const auto api = make_test_api();
    constexpr unsigned kA6 = 0xFF2000U, kSp = 0xFF7FFCU;
    g_regs[14] = kA6; g_regs[15] = kSp;
    mock_write_mem(nullptr, kA6 + 4U, 2, 0x0016U);
    mock_write_mem(nullptr, kA6 + 86U, 4, 0x00006000U); mock_write_mem(nullptr, kA6 + 16U, 4, 0x00002000U);
    mock_write_mem(nullptr, kA6 + 57U, 1, 0x00U);
    step_sequence(api, {0x02575CU, 0x025760U, 0x025764U, 0x025766U, 0x02583EU, 0x025842U, 0x025846U, 0x025856U, 0x02585AU, 0x02585EU, 0x025864U, 0x02586AU, 0x025870U, 0x02587AU});
    TEST_ASSERT(mock_read_mem(nullptr, kA6 + 4U, 2) == 0x0018U && g_last_return_pc == 0x00E7E8U);
    const auto& m = get_02575c_step_metrics();
    TEST_ASSERT(m.target11_calls == 1 && m.target11_transfer_exits == 1);
    std::cout << "[PASS] test_runtime_bridge_target11_negative_tail_jump\n";
}

void test_runtime_bridge_target12_rts_and_loopback() {
    reset_environment(); const auto api = make_test_api();
    constexpr unsigned kA6 = 0xFF2000U, kSp = 0xFF7FFCU, kRet = 0x00DF1EU;
    g_regs[14] = kA6; g_regs[15] = kSp;
    mock_write_mem(nullptr, kSp, 4, kRet); mock_write_mem(nullptr, kA6 + 4U, 2, 0x0018U);
    mock_write_mem(nullptr, kA6 + 55U, 1, 0x00U);
    step_sequence(api, {0x02575CU, 0x025760U, 0x025764U, 0x025766U, 0x025880U, 0x025886U, 0x02588AU});
    TEST_ASSERT(g_last_return_pc == kRet);
    reset_environment();
    g_regs[14] = kA6; g_regs[15] = kSp;
    mock_write_mem(nullptr, kA6 + 4U, 2, 0x0018U);
    mock_write_mem(nullptr, kA6 + 55U, 1, 0x02U); mock_write_mem(nullptr, kA6 + 56U, 1, 0xFFU);
    step_sequence(api, {0x02575CU, 0x025760U, 0x025764U, 0x025766U, 0x025880U, 0x025886U, 0x02588CU, 0x025892U});
    TEST_ASSERT(g_last_return_pc == 0x0255D8U && mock_read_mem(nullptr, kA6 + 56U, 1) == 0xF3U);
    const auto& m = get_02575c_step_metrics();
    TEST_ASSERT(m.target12_calls == 1 && m.target12_loopback_exits == 1);
    std::cout << "[PASS] test_runtime_bridge_target12_rts_and_loopback\n";
}

void test_runtime_bridge_unsupported_and_disabled() {
    reset_environment(); const auto api = make_test_api();
    constexpr unsigned kA6 = 0xFF2000U; g_regs[14] = kA6;
    mock_write_mem(nullptr, kA6 + 4U, 2, 0x0020U); // State 0x20 (unsupported)
    TEST_ASSERT(step_m68k_02575c(0x02575CU, api, true) == M68k9D70StepResult::NotHandled);
    TEST_ASSERT(get_02575c_step_metrics().unsupported_state_entries == 1);
    reset_environment(); g_regs[14] = kA6;
    mock_write_mem(nullptr, kA6 + 4U, 2, 0x0000U);
    TEST_ASSERT(step_m68k_02575c(0x02575CU, api, false) == M68k9D70StepResult::NotHandled);
    TEST_ASSERT(get_02575c_step_metrics().rom_calls == 1);
    std::cout << "[PASS] test_runtime_bridge_unsupported_and_disabled\n";
}
} // namespace

int main() {
    std::cout << "Starting Entity Cluster Unit Tests...\n";
    test_jump_table_resolution();
    test_runtime_bridge_state0_rts_success();
    test_runtime_bridge_state0_loopback_success();
    test_runtime_bridge_state1_child_and_rts();
    test_runtime_bridge_state2_rts();
    test_runtime_bridge_state7_rts();
    test_runtime_bridge_target10_child_and_rts();
    test_runtime_bridge_target11_non_negative_rts();
    test_runtime_bridge_target11_negative_tail_jump();
    test_runtime_bridge_target12_rts_and_loopback();
    test_runtime_bridge_unsupported_and_disabled();
    std::cout << "ALL ENTITY CLUSTER UNIT TESTS PASSED SUCCESSFULLY!\n";
    return 0;
}
