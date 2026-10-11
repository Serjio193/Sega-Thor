#pragma once

#include <cstdint>

namespace oasis::hybrid {

struct CoordinateBoundInput {
    std::uint16_t coordinate{};
    std::uint16_t input_origin{};
    std::uint16_t record_offset{};
    std::uint16_t row_units{};
    std::uint8_t flags{};
};

struct CoordinateBoundResult {
    std::uint16_t coordinate{};
    std::uint16_t coordinate_sum{};
    std::uint16_t maximum{};
    std::uint8_t flags{};
    bool flags_written{};
};

[[nodiscard]] CoordinateBoundResult apply_coordinate_bound(
    const CoordinateBoundInput& input) noexcept;

} // namespace oasis::hybrid
