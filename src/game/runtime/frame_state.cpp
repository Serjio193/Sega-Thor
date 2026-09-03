#include "game/runtime/frame_state.hpp"

namespace oasis::game::runtime {

void step(FrameState& state, const InputSnapshot& next_input) noexcept {
    state.previous_input = state.input;
    state.input = next_input;
    ++state.frame_index;
}

} // namespace oasis::game::runtime
