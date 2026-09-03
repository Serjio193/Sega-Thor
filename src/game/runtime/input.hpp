#pragma once

#include <cstdint>

namespace oasis::game::runtime {

enum class Button : std::uint16_t {
    Up    = 1U << 0U,
    Down  = 1U << 1U,
    Left  = 1U << 2U,
    Right = 1U << 3U,
    A     = 1U << 4U,
    B     = 1U << 5U,
    C     = 1U << 6U,
    Start = 1U << 7U,
    X     = 1U << 8U,
    Y     = 1U << 9U,
    Z     = 1U << 10U,
    Mode  = 1U << 11U,
};

struct InputSnapshot {
    std::uint16_t buttons{};

    [[nodiscard]] constexpr bool held(Button button) const noexcept {
        return (buttons & static_cast<std::uint16_t>(button)) != 0U;
    }
};

[[nodiscard]] constexpr InputSnapshot operator|(Button lhs, Button rhs) noexcept {
    return {static_cast<std::uint16_t>(static_cast<std::uint16_t>(lhs) |
                                      static_cast<std::uint16_t>(rhs))};
}

[[nodiscard]] constexpr InputSnapshot operator|(InputSnapshot lhs, Button rhs) noexcept {
    lhs.buttons = static_cast<std::uint16_t>(lhs.buttons | static_cast<std::uint16_t>(rhs));
    return lhs;
}

} // namespace oasis::game::runtime
