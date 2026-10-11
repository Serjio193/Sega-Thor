#pragma once

#include <cstdint>

namespace oasis::hybrid {

struct PositiveBoundInput {
    std::uint16_t coordinate{};
    std::uint16_t axis_value{};
    std::uint16_t record_offset{};
    std::uint16_t dimension_units{};
    std::uint8_t flags{};
};

struct PositiveBoundResult {
    std::uint16_t coordinate{};
    std::uint16_t maximum{};
    std::uint16_t coordinate_sum{};
    std::uint8_t flags{};
    bool flags_written{};
    bool extend_flag{};
};

[[nodiscard]] PositiveBoundResult apply_positive_bound(
    const PositiveBoundInput& input) noexcept;

} // namespace oasis::hybrid
