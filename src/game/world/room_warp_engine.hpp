#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace oasis::game::world {

/// Camera and spawn coordinates calculated for Genesis 320x224 display.
struct CameraCoords {
    std::int16_t cam_x{0};
    std::int16_t cam_y{0};
    std::int16_t spawn_x{0};
    std::int16_t spawn_y{0};
};

/// Player Ali entity spawn state representation.
struct PlayerSpawnState {
    std::int16_t x{0};
    std::int16_t y{0};
    std::int16_t elevation{0};
    std::uint16_t mode{0U};
    std::uint16_t facing{0U};
    std::uint16_t idle_frame{0x003FU};
    bool visible{true};
    std::uint8_t layer{0x10U};
    std::uint32_t falling_anim_counter{0U};
};

/// 6-Flag cutscene and transition lock configuration (0x00C9B2).
struct CutsceneLockFlags {
    bool cutscene_active{false};   // $FF17B9 != 0
    bool transition_busy{false};   // $FF1987 != 0
    bool pause_menu{false};        // $FF1985 != 0
    bool cutscene_lock_bit{false}; // ($FF17B8 & 0x04) != 0
    bool secondary_lock{false};    // $FF1983 != 0
    bool tertiary_lock{false};     // $FF1982 != 0
};

/// Configuration and constants for ROM Room Warp Controller,
/// Camera Centering, Cutscene Locks & Player Ali Spawn Reset Engine (0x00C9B2..0x00CC12, 608 bytes).
struct RoomWarpEngineConfig {
    static constexpr std::uint32_t kEntryCheckCutsceneLockAddr = 0x00C9B2U;
    static constexpr std::uint32_t kEntryWarpImmediateAddr = 0x00C9ECU;
    static constexpr std::uint32_t kEntryWarpFadeAddr = 0x00CA10U;
    static constexpr std::uint32_t kEntryRoomTransitionExecAddr = 0x00CA3EU;
    static constexpr std::uint32_t kEntrySpawnPlayerAliAddr = 0x00CB64U;
    static constexpr std::uint32_t kRangeEndAddr = 0x00CC12U;
    static constexpr std::size_t kRoutineSizeBytes = 608U;

    // Camera centering offsets for Genesis 320x224 screen
    static constexpr std::int16_t kCameraCenterOffsetX = 160; // 320 / 2
    static constexpr std::int16_t kCameraCenterOffsetY = 112; // 224 / 2

    // Special room overrides
    static constexpr std::uint16_t kRoomOverrideSource = 0x0004U;
    static constexpr std::uint16_t kRoomOverrideTarget = 0x0611U;

    // Ali spawn defaults
    static constexpr std::uint16_t kAliIdleFrame = 0x003FU;
    static constexpr std::uint8_t kAliTangibleLayer = 0x10U;
    static constexpr std::uint16_t kAliCacheInvalidTag = 0xFFFFU;
    static constexpr std::int16_t kAliSkyDropElevation = 240; // 0x00F0
    static constexpr std::uint16_t kAliSkyDropMode = 6U;
    static constexpr std::uint16_t kAliSkyDropFacing = 6U;

    // Memory map addresses
    static constexpr std::uint32_t kPlayerAliSlotRam = 0x00FF19E8U;
    static constexpr std::uint32_t kCutsceneActiveRam = 0x00FF17B9U;
    static constexpr std::uint32_t kTransitionBusyRam = 0x00FF1987U;
    static constexpr std::uint32_t kPauseMenuRam = 0x00FF1985U;
    static constexpr std::uint32_t kScriptPauseFlagsRam = 0x00FF17B8U;
    static constexpr std::uint32_t kSecondaryLockRam = 0x00FF1983U;
    static constexpr std::uint32_t kTertiaryLockRam = 0x00FF1982U;
    static constexpr std::uint32_t kWarpTransitionModeRam = 0x00FF19FEU;
    static constexpr std::uint32_t kImmediateWarpFlagRam = 0x00FF17BEU;
    static constexpr std::uint32_t kFadeFlagRam = 0x00FF17BCU;
    static constexpr std::uint32_t kBlackoutFlagRam = 0x00FF17BBU;
    static constexpr std::uint32_t kRoomLoadedFlagRam = 0x00FF17BAU;
    static constexpr std::uint32_t kCameraSpawnCoordsRam = 0x00FF178EU;
    static constexpr std::uint32_t kPlayerSpawnCacheRam = 0x00FF1972U;
    static constexpr std::uint32_t kSpawnLockFlagRam = 0x00FF1984U;
};

/// Centers camera on target coordinate (target_x - 160, target_y - 112).
[[nodiscard]] CameraCoords calculate_camera_center(std::int16_t target_x, std::int16_t target_y) noexcept;

/// Checks if any of the 6 cutscene/transition lock conditions are active. Returns true if locked.
[[nodiscard]] bool check_cutscene_script_lock(const CutsceneLockFlags& flags) noexcept;

/// Applies special room ID overrides (maps room 4 -> 0x0611).
[[nodiscard]] std::uint16_t apply_room_id_overrides(std::uint16_t room_id) noexcept;

/// Resets Player Ali spawn state, applying sky drop falling pose if fade_transition is true.
[[nodiscard]] PlayerSpawnState spawn_player_ali(const CameraCoords& coords, bool fade_transition) noexcept;

/// Raw 64KB RAM window integration helper for checking cutscene script lock (0x00C9B2).
[[nodiscard]] bool check_cutscene_script_lock_raw(std::span<const std::uint8_t> ram) noexcept;

/// Raw 64KB RAM window integration helper for immediate warp entry (0x00C9EC).
CameraCoords warp_room_entry_immediate_raw(
    std::span<std::uint8_t> ram,
    std::uint16_t room_id,
    std::int16_t target_x,
    std::int16_t target_y,
    std::uint16_t mode = 0U);

/// Raw 64KB RAM window integration helper for fade warp entry (0x00CA10).
CameraCoords warp_room_entry_fade_raw(
    std::span<std::uint8_t> ram,
    std::uint16_t room_id,
    std::int16_t target_x,
    std::int16_t target_y);

/// Raw 64KB RAM window integration helper for resetting player Ali entity in room (0x00CB64).
PlayerSpawnState spawn_player_ali_in_room_raw(
    std::span<std::uint8_t> ram,
    std::int16_t spawn_x,
    std::int16_t spawn_y);

} // namespace oasis::game::world

namespace thor::warp {
using CameraCoords = oasis::game::world::CameraCoords;
using PlayerSpawnState = oasis::game::world::PlayerSpawnState;
using CutsceneLockFlags = oasis::game::world::CutsceneLockFlags;
using RoomWarpEngineConfig = oasis::game::world::RoomWarpEngineConfig;
using oasis::game::world::apply_room_id_overrides;
using oasis::game::world::calculate_camera_center;
using oasis::game::world::check_cutscene_script_lock;
using oasis::game::world::spawn_player_ali;
} // namespace thor::warp
