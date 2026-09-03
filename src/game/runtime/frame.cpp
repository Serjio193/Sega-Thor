#include "game/runtime/frame.hpp"

namespace oasis::game::runtime {

void FrameState::step(InputSnapshot input) noexcept {
    previous_ = current_;
    current_ = input;
    ++frame_index_;
}

bool FrameState::pressed(Button button) const noexcept {
    return current_.held(button) && !previous_.held(button);
}

bool FrameState::released(Button button) const noexcept {
    return !current_.held(button) && previous_.held(button);
}

} // namespace oasis::game::runtime
