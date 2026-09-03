#pragma once

#include "game/runtime/input.hpp"

#include <cstdint>

namespace oasis::game::runtime {

class FrameState {
public:
    void step(InputSnapshot input) noexcept;

    [[nodiscard]] std::uint64_t frame_index() const noexcept { return frame_index_; }
    [[nodiscard]] InputSnapshot current_input() const noexcept { return current_; }
    [[nodiscard]] InputSnapshot previous_input() const noexcept { return previous_; }

    [[nodiscard]] bool held(Button button) const noexcept { return current_.held(button); }
    [[nodiscard]] bool pressed(Button button) const noexcept;
    [[nodiscard]] bool released(Button button) const noexcept;

private:
    std::uint64_t frame_index_{};
    InputSnapshot current_{};
    InputSnapshot previous_{};
};

} // namespace oasis::game::runtime
