#include "tools/re_static_translation.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <algorithm>
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

void test_b_leaf_full_semantics() {
    constexpr std::uint32_t base = 0x1000U;
    constexpr std::uint32_t start = base + 0x08U;
    constexpr std::uint16_t negative = 1U << 3U;
    constexpr std::uint16_t zero = 1U << 2U;
    constexpr std::uint16_t x = oasis::tools::ccr::kExtend;
    std::array<std::uint8_t, 0x20> expected_bytes{};
    std::array<std::uint8_t, 0x20> actual_bytes{};
    expected_bytes.fill(0xCD);
    actual_bytes.fill(0xCD);
    oasis::tools::M68kState expected{};
    expected.d[0] = 0xAABB9999U;
    expected.d[1] = 0x1122F234U;
    expected.d[2] = 0x33443456U;
    expected.d[3] = 0x55669ABCU;
    expected.d[4] = 0x7788FFFFU;
    expected.d[5] = 0xCCDD0020U;
    expected.a[5] = start;
    expected.ccr = x;
    auto actual = expected;
    oasis::tools::BoundedMemory expected_memory(base, expected_bytes);
    oasis::tools::BoundedMemory actual_memory(base, actual_bytes);

    expected.d[0] = 0xAABB0020U;
    expected.d[5] = 0xCCDD0021U;
    expected.a[5] = start + 8U;
    expected.ccr = static_cast<std::uint16_t>(x | negative);
    expected_memory.write_u16(start, 0x3456U);
    expected_memory.write_u16(start + 2U, 0x0020U);
    expected_memory.write_u16(start + 4U, 0x9ABCU);
    expected_memory.write_u16(start + 6U, 0xF234U);

    const auto run = oasis::tools::mechanical_A8DA(actual, actual_memory);
    assert(run.status == oasis::tools::TranslationStatus::executed);
    assert(run.instructions_executed == 10U);
    assert(actual_memory.writes().size() == 4U);
    assert(actual_memory.writes()[0].address == start && actual_memory.writes()[0].width == 2U &&
           actual_memory.writes()[0].value == 0x3456U);
    assert(actual_memory.writes()[1].address == start + 2U && actual_memory.writes()[1].value == 0x0020U);
    assert(actual_memory.writes()[2].address == start + 4U && actual_memory.writes()[2].value == 0x9ABCU);
    assert(actual_memory.writes()[3].address == start + 6U && actual_memory.writes()[3].value == 0xF234U);
    assert(oasis::tools::compare_state(expected, actual, expected_memory, actual_memory).equal);

    actual = state_fixture(0x1234U, 0x0020U, 0x0030U, 0x0004U, 0x0050U);
    actual.d[0] = 0xAABB0000U;
    actual.d[5] = 0xCCDD0050U;
    actual.a[5] = start;
    actual.ccr = x;
    actual_bytes.fill(0xCD);
    oasis::tools::BoundedMemory early_memory(base, actual_bytes);
    const auto early = oasis::tools::mechanical_A8DA(actual, early_memory);
    assert(early.instructions_executed == 2U && actual.d[0] == 0xAABB0000U &&
           actual.d[5] == 0xCCDD0050U && actual.a[5] == start);
    assert(actual.ccr == static_cast<std::uint16_t>(x | zero));
    assert(early_memory.writes().empty());
}

void test_c_state_leaf_and_memory_divergence() {
    constexpr std::uint32_t base = 0x00FF1000U;
    std::array<std::uint8_t, 0x1000> expected_bytes{};
    std::array<std::uint8_t, 0x1000> actual_bytes{};
    oasis::tools::M68kState actual{};
    oasis::tools::M68kState expected{};
    actual.d[0] = expected.d[0] = 0xDEAD1234U;
    expected.a[6] = actual.a[6] = base + 0x800U;
    expected_bytes.fill(0xA5);
    actual_bytes.fill(0xA5);
    expected.ccr = actual.ccr = oasis::tools::ccr::kExtend | (1U << 3U) | 1U;
    oasis::tools::BoundedMemory expected_memory(base, expected_bytes);
    oasis::tools::BoundedMemory actual_memory(base, actual_bytes);
    expected.d[0] = 0;
    expected.ccr = oasis::tools::ccr::kExtend | (1U << 2U);
    expected_memory.write_u32(base + 0x84EU, 0);
    expected_memory.write_u32(base + 0x852U, 0);
    expected_memory.write_u16(base + 0x82AU, 0);
    expected_memory.write_u16(base + 0x804U, 0);
    auto actual_run = oasis::tools::mechanical_62CC(actual, actual_memory);
    assert(actual_run.status == oasis::tools::TranslationStatus::executed);
    assert(actual_run.instructions_executed == 6);
    assert(actual_memory.writes().size() == 4U);
    assert(oasis::tools::compare_state(expected, actual, expected_memory, actual_memory).equal);
    actual_bytes[0x84E] = 1;
    const auto diff = oasis::tools::compare_state(expected, actual, expected_memory, actual_memory);
    assert(!diff.equal && diff.first_divergence == "memory byte 2126");
}

void test_ccr_helpers() {
    constexpr std::uint16_t negative = 1U << 3U;
    constexpr std::uint16_t zero = 1U << 2U;
    constexpr std::uint16_t overflow = 1U << 1U;
    constexpr std::uint16_t carry = 1U;
    constexpr std::uint16_t x = oasis::tools::ccr::kExtend;
    assert(oasis::tools::ccr::move_word(x | negative | carry, 0) == (x | zero));
    assert(oasis::tools::ccr::move_word(0, 0x8000U) == negative);
    assert(oasis::tools::ccr::move_long(x | negative, 0) == (x | zero));
    assert(oasis::tools::ccr::compare_word(x, 0x0050U, 0x0050U) == (x | zero));
    assert(oasis::tools::ccr::compare_word(x, 0xFFFFU, 0x0050U) ==
           (x | negative));
    assert(oasis::tools::ccr::compare_word(0, 0x0000U, 1U) ==
           (negative | carry));
    assert(oasis::tools::ccr::compare_word(0, 0x7FFFU, 0xFFFFU) ==
           (negative | overflow | carry));
    assert(oasis::tools::ccr::add_word(0, 0x7FFFU, 1U) == (negative | overflow));
    assert(oasis::tools::ccr::add_word(0, 0xFFFFU, 1U) == (x | zero | carry));
    assert(oasis::tools::ccr::add_word(0, 0x8000U, 0x8000U) ==
           (x | zero | overflow | carry));
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
    test_b_leaf_full_semantics();
    test_c_state_leaf_and_memory_divergence();
    test_ccr_helpers();
    test_explicit_stop();
    return 0;
}
