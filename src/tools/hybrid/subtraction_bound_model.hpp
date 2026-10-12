#pragma once

#include <cstdint>

namespace oasis::hybrid {

struct SubtractionBoundInput {
    std::uint32_t d0{};
    std::uint32_t d1{};
    std::uint32_t d3{};
    std::uint16_t limit{};
    std::uint8_t marker{};
    std::uint16_t sr{};
};

struct SubtractionBoundResult {
    std::uint32_t d0{};
    std::uint32_t d3{};
    std::uint8_t marker{};
    std::uint16_t sr{};
    bool marker_written{};
};

[[nodiscard]] SubtractionBoundResult apply_subtraction_bound(
    const SubtractionBoundInput& input) noexcept;

} // namespace oasis::hybrid
