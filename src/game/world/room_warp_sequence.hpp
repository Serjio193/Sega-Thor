#pragma once

#include <cstdint>

namespace oasis::game::world {

inline constexpr std::uint32_t kRoomUpdateEarlyAddr = 0x00C980U;
inline constexpr std::uint32_t kRoomUpdateLateAddr = 0x00C998U;
inline constexpr std::uint32_t kRoomWarpGuardAddr = 0x00C9B2U;
inline constexpr std::uint32_t kWarpDoorFadeAddr = 0x00C9ECU;
inline constexpr std::uint32_t kWarpInstantCutAddr = 0x00CA10U;

inline constexpr std::int16_t kWarpCameraHalfW = 160; // 0x00A0
inline constexpr std::int16_t kWarpCameraHalfH = 112; // 0x0070
inline constexpr std::uint16_t kWarpRedirectSrcRoom = 0x0004U;
inline constexpr std::uint16_t kWarpRedirectDstRoom = 0x0611U;
inline constexpr std::uint8_t kWarpCheckpointBit6 = 0x40U;
inline constexpr std::uint8_t kWarpSoundStopCmd = 2U;
inline constexpr std::uint8_t kWarpFadeSpeed = 4U;

struct RoomWarpLockState {
    std::uint8_t lock_ff17b9{0};
    std::uint8_t lock_ff1987{0};
    std::uint8_t lock_ff1985{0};
    std::uint8_t flags_ff17b8{0}; // bit 1 disables room hooks, bit 2 blocks warp
    std::uint8_t lock_ff1983{0};
    std::uint8_t lock_ff1982{0};
    std::uint8_t flag_ff17ba{0};
    std::uint8_t lock_ff1988{0};
    std::uint8_t lock_ff1858{0};
    std::uint16_t scroll_ff177e{0};
    std::uint16_t scroll_ff1780{0};
    std::uint8_t global_flag_16f1{0};
    std::uint8_t fade_speed_ff1896{0};
};

struct RoomWarpRequest {
    std::uint16_t target_room_id{0};
    std::int16_t spawn_x{0};
    std::int16_t spawn_y{0};
    std::uint8_t room_hdr_flag_19{0};
};

struct RoomWarpPlan {
    std::uint16_t resolved_room_id{0};
    std::int16_t camera_x{0};
    std::int16_t camera_y{0};
    bool save_checkpoint{false};
    std::uint8_t sound_cmd{kWarpSoundStopCmd};
    std::uint8_t fade_speed{kWarpFadeSpeed};
};

/// Models 0x00C980/0x00C998 guard: true when ($FF17B8 & 2) == 0.
[[nodiscard]] bool can_run_room_update_hooks(
    const RoomWarpLockState& state) noexcept;

/// Models 0x00C9B2..0x00C9EA: true when all 6 warp-lock conditions are clear.
[[nodiscard]] bool can_trigger_room_warp(
    const RoomWarpLockState& state) noexcept;

/// Models 0x00C9EC..0x00CB0A: redirects 0x0004 -> 0x0611, centers camera on
/// (spawn_x - 160, spawn_y - 112), evaluates checkpoint bit 6, and resets
/// transition/lock flags + sets fade_speed = 4.
RoomWarpPlan execute_room_warp_sequence(
    RoomWarpLockState& state,
    const RoomWarpRequest& request) noexcept;

} // namespace oasis::game::world
