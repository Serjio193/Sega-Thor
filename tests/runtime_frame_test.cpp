#include "game/runtime/frame.hpp"

#include <array>
#include <cassert>

using oasis::game::runtime::Button;
using oasis::game::runtime::FrameState;
using oasis::game::runtime::InputSnapshot;

namespace {

void run_sequence(FrameState& state, const std::array<InputSnapshot, 5>& sequence) {
    for (const auto input : sequence) {
        state.step(input);
    }
}

} // namespace

int main() {
    FrameState state;
    assert(state.frame_index() == 0U);
    assert(!state.held(Button::A));

    state.step(Button::Right | Button::A);
    assert(state.frame_index() == 1U);
    assert(state.held(Button::Right));
    assert(state.held(Button::A));
    assert(state.pressed(Button::Right));
    assert(state.pressed(Button::A));
    assert(!state.released(Button::A));

    state.step(Button::Right | Button::A);
    assert(state.frame_index() == 2U);
    assert(!state.pressed(Button::Right));
    assert(!state.pressed(Button::A));

    state.step(Button::Right);
    assert(state.frame_index() == 3U);
    assert(state.held(Button::Right));
    assert(!state.held(Button::A));
    assert(state.released(Button::A));
    assert(!state.released(Button::Right));

    const std::array<InputSnapshot, 5> sequence{
        InputSnapshot{},
        Button::Left | Button::B,
        Button::Left | Button::B,
        Button::B,
        InputSnapshot{},
    };

    FrameState first;
    FrameState second;
    run_sequence(first, sequence);
    run_sequence(second, sequence);

    assert(first.frame_index() == second.frame_index());
    assert(first.current_input().buttons == second.current_input().buttons);
    assert(first.previous_input().buttons == second.previous_input().buttons);
    assert(first.frame_index() == sequence.size());
    assert(first.released(Button::B));
    assert(!first.held(Button::Left));

    return 0;
}
