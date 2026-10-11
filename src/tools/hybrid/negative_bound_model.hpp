#pragma once

#include <cstdint>

namespace oasis::hybrid {

struct NegativeBoundInput {
    std::uint16_t coordinate{};
    std::uint16_t axis_value{};
    std::uint16_t record_offset{};
    std::uint8_t flags{};
};

struct NegativeBoundResult {
    std::uint16_t coordinate{};
    std::uint16_t relative_value{};
    std::uint8_t flags{};
    bool flags_written{};
    bool extend_flag{};
};

[[nodiscard]] NegativeBoundResult apply_negative_bound(
    const NegativeBoundInput& input) noexcept;

} // namespace oasis::hybrid
