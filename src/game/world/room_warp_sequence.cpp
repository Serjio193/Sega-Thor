#include "game/world/room_warp_sequence.hpp"

namespace oasis::game::world {

bool can_run_room_update_hooks(const RoomWarpLockState& state) noexcept {
    return (state.flags_ff17b8 & 0x02U) == 0U;
}

bool can_trigger_room_warp(const RoomWarpLockState& state) noexcept {
    return state.lock_ff17b9 == 0U &&
           state.lock_ff1987 == 0U &&
           state.lock_ff1985 == 0U &&
           (state.flags_ff17b8 & 0x04U) == 0U &&
           state.lock_ff1983 == 0U &&
           state.lock_ff1982 == 0U;
}

RoomWarpPlan execute_room_warp_sequence(
    RoomWarpLockState& state,
    const RoomWarpRequest& request) noexcept {
    RoomWarpPlan plan{};
    plan.resolved_room_id =
        (request.target_room_id == kWarpRedirectSrcRoom)
            ? kWarpRedirectDstRoom
            : request.target_room_id;
    plan.camera_x = static_cast<std::int16_t>(request.spawn_x - kWarpCameraHalfW);
    plan.camera_y = static_cast<std::int16_t>(request.spawn_y - kWarpCameraHalfH);
    plan.save_checkpoint =
        ((request.room_hdr_flag_19 & kWarpCheckpointBit6) != 0U) ||
        ((state.global_flag_16f1 & kWarpCheckpointBit6) != 0U);
    plan.sound_cmd = kWarpSoundStopCmd;
    plan.fade_speed = kWarpFadeSpeed;

    state.flag_ff17ba = 0xFFU;
    state.flags_ff17b8 = 0U;
    state.lock_ff17b9 = 0U;
    state.lock_ff1988 = 0U;
    state.lock_ff1858 = 0U;
    state.scroll_ff177e = 0U;
    state.scroll_ff1780 = 0U;
    state.fade_speed_ff1896 = kWarpFadeSpeed;
    return plan;
}

} // namespace oasis::game::world
