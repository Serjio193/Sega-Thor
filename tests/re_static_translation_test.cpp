#include "tools/re_static_translation.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <vector>

namespace {

oasis::tools::M68kState state_fixture(std::uint16_t d1, std::uint16_t d2,
                                      std::uint16_t d3, std::uint16_t d4,
                                      std::uint16_t d5) {
    oasis::tools::M68kState state{};
    state.d[1] = d1;
    state.d[2] = d2;
    state.d[3] = d3;
    state.d[4] = d4;
    state.d[5] = d5;
    return state;
}

void test_a_format_a() {
    const std::array<std::uint8_t, 11> source{0x0A, 0x00, 0x03, 'A', 'B', 'C',
                                               0x80, 0x03, 0x62, 0x61, 0x00};
    std::array<std::uint8_t, 32> mechanical{};
    std::array<std::uint8_t, 32> native{};
    const auto generated = oasis::tools::mechanical_3820(source, mechanical);
    const auto reference = oasis::game::decompress_graphics(source, native);
    assert(generated.source_consumed == reference.source_consumed);
    assert(generated.output_size == reference.output_size);
    assert(std::equal(mechanical.begin(), mechanical.begin() + generated.output_size, native.begin()));
}

void test_a_format_b() {
    const std::array<std::uint8_t, 10> source{0x00, 0x00, 0x00, 0x03, 0x01, 0x30, 0x00, 'Q', 0x00, 0x00};
    std::array<std::uint8_t, 32> mechanical{};
    std::array<std::uint8_t, 32> native{};
    const auto generated = oasis::tools::mechanical_3820(source, mechanical);
    const auto reference = oasis::game::decompress_graphics(source, native);
    assert(generated.source_consumed == reference.source_consumed);
    assert(generated.output_size == reference.output_size);
    assert(std::equal(mechanical.begin(), mechanical.begin() + generated.output_size, native.begin()));
}

void test_b_leaf_and_first_divergence() {
    const auto fixture = state_fixture(0x1234, 0x0020, 0x0030, 0x0004, 0x0020);
    auto actual = fixture;
    const auto actual_run = oasis::tools::mechanical_A8DA(actual);
    auto expected = fixture;
    expected.d[0] = 0x0024;
    expected.d[5] = 0x1234;
    expected.ccr = 0;
    assert(actual_run.status == oasis::tools::TranslationStatus::verified);
    assert(actual_run.instructions_executed == 10);
    std::array<std::uint8_t, 1> expected_bytes{};
    std::array<std::uint8_t, 1> actual_bytes{};
    const oasis::tools::BoundedMemory expected_memory(0, expected_bytes);
    const oasis::tools::BoundedMemory actual_memory(0, actual_bytes);
    assert(oasis::tools::compare_state(expected, actual, expected_memory, actual_memory).equal);
    actual.d[0] ^= 1U;
    const auto diff = oasis::tools::compare_state(expected, actual, expected_memory, actual_memory);
    assert(!diff.equal && diff.first_divergence == "D0");

    actual = state_fixture(0x1234, 0x0020, 0x0030, 0x0004, 0x0050);
    const auto early = oasis::tools::mechanical_A8DA(actual);
    assert(early.instructions_executed == 3 && actual.d[0] == 0 && actual.d[5] == 0x0050);
    assert(actual.ccr == (1U << 2U));
}

void test_c_state_leaf_and_memory_divergence() {
    constexpr std::uint32_t base = 0x00FF1000U;
    std::array<std::uint8_t, 0x1000> expected_bytes{};
    std::array<std::uint8_t, 0x1000> actual_bytes{};
    oasis::tools::M68kState actual{};
    oasis::tools::M68kState expected{};
    expected.a[6] = actual.a[6] = base + 0x800U;
    oasis::tools::BoundedMemory expected_memory(base, expected_bytes);
    oasis::tools::BoundedMemory actual_memory(base, actual_bytes);
    expected.d[0] = 0;
    expected.ccr = 1U << 2U;
    expected_memory.write_u32(base + 0x84EU, 0);
    expected_memory.write_u32(base + 0x852U, 0);
    expected_memory.write_u16(base + 0x82AU, 0);
    expected_memory.write_u16(base + 0x804U, 0);
    auto actual_run = oasis::tools::mechanical_62CC(actual, actual_memory);
    assert(actual_run.instructions_executed == 6);
    assert(oasis::tools::compare_state(expected, actual, expected_memory, actual_memory).equal);
    actual_bytes[0x84E] = 1;
    const auto diff = oasis::tools::compare_state(expected, actual, expected_memory, actual_memory);
    assert(!diff.equal && diff.first_divergence == "memory byte 2126");
}

void test_explicit_stop() {
    const auto stopped = oasis::tools::unsupported_opcode(0x4EFA);
    assert(stopped.status == oasis::tools::TranslationStatus::unsupported);
    assert(stopped.instructions_executed == 0);
}

} // namespace

int main() {
    test_a_format_a();
    test_a_format_b();
    test_b_leaf_and_first_divergence();
    test_c_state_leaf_and_memory_divergence();
    test_explicit_stop();
    return 0;
}
