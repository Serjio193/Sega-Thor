#include "game/render/sat_shadow_records.hpp"

#include <algorithm>
#include <limits>

namespace oasis::game::render {
namespace {
constexpr std::size_t header_size = 2U;
constexpr std::size_t record_size = 6U;
constexpr std::size_t sat_record_size = 8U;
constexpr std::uint16_t coordinate_bias = 0x0080U;
constexpr std::uint16_t clip_origin = 0x0060U;

std::uint16_t read_be_word(std::span<const std::uint8_t> bytes,
                           std::size_t offset) noexcept {
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(bytes[offset]) << 8U) | bytes[offset + 1U]);
}

void write_be_word(std::span<std::uint8_t> bytes, std::size_t offset,
                   std::uint16_t value) noexcept {
    bytes[offset] = static_cast<std::uint8_t>(value >> 8U);
    bytes[offset + 1U] = static_cast<std::uint8_t>(value);
}

std::uint16_t add_signed_byte(std::uint16_t value,
                              std::uint16_t base) noexcept {
    const auto low_byte = static_cast<std::int32_t>(value & 0x00FFU);
    const auto signed_value = low_byte < 0x80 ? low_byte : low_byte - 0x100;
    return static_cast<std::uint16_t>(signed_value + static_cast<std::int32_t>(base));
}

bool inside_vertical_window(std::uint16_t coordinate,
                             std::uint16_t maximum) noexcept {
    const auto relative = static_cast<std::uint16_t>(coordinate - clip_origin);
    return relative <= maximum;
}

} // namespace

std::optional<SatShadowRecordResult> build_sat_shadow_records(
    std::span<const std::uint8_t> input,
    std::span<std::uint8_t> sat_shadow,
    std::size_t a1_offset,
    std::uint16_t d1,
    std::uint16_t d2,
    std::uint16_t d3,
    std::uint16_t d4) noexcept {
    if (input.size() < header_size || a1_offset > sat_shadow.size()) {
        return std::nullopt;
    }

    const auto count_word = read_be_word(input, 0U);
    std::size_t declared_records = 0U;
    if ((count_word & 0x8000U) == 0U) {
        declared_records = static_cast<std::size_t>(count_word) + 1U;
        if (declared_records > (std::numeric_limits<std::size_t>::max() - header_size) /
                                   record_size) {
            return std::nullopt;
        }
        const auto required = header_size + declared_records * record_size;
        if (input.size() != required) return std::nullopt;
    } else if (input.size() != header_size) {
        return std::nullopt;
    }

    const auto adjusted_d1 = static_cast<std::uint16_t>(d1 + coordinate_bias);
    const auto adjusted_d4 = static_cast<std::uint16_t>(d4 + coordinate_bias);
    const auto publish_limit = std::min(
        declared_records, static_cast<std::size_t>(0x0050U - d2));
    if (publish_limit > (std::numeric_limits<std::size_t>::max() - a1_offset) /
                            sat_record_size) {
        return std::nullopt;
    }
    const auto required_end = a1_offset + publish_limit * sat_record_size;
    const bool has_iterations = declared_records != 0U && d2 < 0x0050U;
    const auto minimum_write_end = has_iterations
        ? a1_offset + sat_record_size : a1_offset;
    if (required_end > sat_shadow.size() || minimum_write_end > sat_shadow.size()) {
        return std::nullopt;
    }

    auto cursor = a1_offset;
    auto current_d2 = d2;
    std::size_t consumed = 0U;
    std::size_t published = 0U;
    for (std::size_t index = 0U; index < declared_records; ++index) {
        if (current_d2 >= 0x0050U) break;
        const auto record_offset = header_size + index * record_size;
        const auto raw_a = read_be_word(input, record_offset);
        const auto vertical_a = add_signed_byte(raw_a, adjusted_d4);
        write_be_word(sat_shadow, cursor, vertical_a);
        ++consumed;
        if (!inside_vertical_window(vertical_a, 0x0100U)) continue;

        const auto horizontal = static_cast<std::uint16_t>(
            read_be_word(input, record_offset + 2U) + d3);
        const auto attribute = static_cast<std::uint16_t>(
            (raw_a & 0xFF00U) | static_cast<std::uint8_t>(current_d2 + 1U));
        write_be_word(sat_shadow, cursor + 2U, attribute);
        write_be_word(sat_shadow, cursor + 4U, horizontal);

        const auto raw_b = read_be_word(input, record_offset + 4U);
        const auto vertical_b = add_signed_byte(raw_b, adjusted_d1);
        write_be_word(sat_shadow, cursor + 6U, vertical_b);
        if (!inside_vertical_window(vertical_b, 0x0160U)) continue;

        cursor += sat_record_size;
        ++current_d2;
        ++published;
    }

    return SatShadowRecordResult{cursor, current_d2, consumed, published};
}

} // namespace oasis::game::render
