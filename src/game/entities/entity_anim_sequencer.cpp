#include "game/entities/entity_anim_sequencer.hpp"

#include <algorithm>

namespace oasis::game::entities {

bool step_anim_keyframe(
    std::span<const AnimKeyframe4> keyframes,
    EntityAnimState& state) noexcept {
    if (keyframes.empty()) {
        return false;
    }

    state.combat_flags_37 = static_cast<std::uint8_t>(
        state.combat_flags_37 & ~kAnimFlagSwingHitOnce);

    if ((state.combat_flags_37 & kAnimFlagLastFrame) != 0U) {
        state.cursor_bytes = 0U;
        state.combat_flags_37 = static_cast<std::uint8_t>(
            state.combat_flags_37 & ~kAnimFlagLastFrame);
    }

    std::size_t idx = static_cast<std::size_t>(state.cursor_bytes / 4U);
    if (idx >= keyframes.size()) {
        idx = 0U;
        state.cursor_bytes = 0U;
    }

    const AnimKeyframe4& kf = keyframes[idx];
    state.frame_id_32 = kf.frame_id;
    state.cursor_bytes = static_cast<std::uint16_t>((idx + 1U) * 4U);

    const std::int32_t raw_dur =
        static_cast<std::int32_t>((kf.flags_dur & 0x7FFFU) >> 1U) - 1;
    state.timer_2e = static_cast<std::uint16_t>(std::max<std::int32_t>(0, raw_dur));

    if (kf.is_last_frame()) {
        state.combat_flags_37 |= kAnimFlagLastFrame;
        if (state.timer_2e == 0U) {
            state.combat_flags_37 |= kAnimFlagAnimCompleted;
        }
    }

    return true;
}

bool step_anim_tick(
    std::span<const AnimKeyframe4> keyframes,
    EntityAnimState& state) noexcept {
    if (keyframes.empty()) {
        return false;
    }

    if (state.timer_2e > 0U) {
        --state.timer_2e;
        if (state.timer_2e == 0U && (state.combat_flags_37 & kAnimFlagLastFrame) != 0U) {
            state.combat_flags_37 |= kAnimFlagAnimCompleted;
        }
        return false;
    }

    return step_anim_keyframe(keyframes, state);
}

} // namespace oasis::game::entities
