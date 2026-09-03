#pragma once

#include <cstdint>

namespace oasis::game::runtime {

struct InputSnapshot {
    bool up{};
    bool down{};
    bool left{};
    bool right{};
    bool a{};
    bool b{};
    bool c{};
    bool start{};
};

[[nodiscard]] constexpr std::uint16_t pack_input(const InputSnapshot& input) noexcept {
    return static_cast<std::uint16_t>(
        (input.up ? 1U << 0U : 0U) |
        (input.down ? 1U << 1U : 0U) |
        (input.left ? 1U << 2U : 0U) |
        (input.right ? 1U << 3U : 0U) |
        (input.a ? 1U << 4U : 0U) |
        (input.b ? 1U << 5U : 0U) |
        (input.c ? 1U << 6U : 0U) |
        (input.start ? 1U << 7U : 0U));
}

struct FrameState {
    std::uint64_t frame_index{};
    InputSnapshot input{};
    InputSnapshot previous_input{};
};

void step(FrameState& state, const InputSnapshot& next_input) noexcept;

[[nodiscard]] constexpr bool pressed(bool current, bool previous) noexcept {
    return current && !previous;
}

[[nodiscard]] constexpr bool released(bool current, bool previous) noexcept {
    return !current && previous;
}

} // namespace oasis::game::runtime
