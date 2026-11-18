#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace oasis::game::world {

/// Companion spirit state representation for room warp synchronizer (0x00CC12..0x00CCA4).
struct SpiritState {
    bool active{false};
    std::uint16_t type{0U};
    std::int32_t x_fixed{0};          // 16.16 fixed-point X coordinate (+0x08)
    std::int32_t y_fixed{0};          // 16.16 fixed-point Y coordinate (+0x0C)
    std::int16_t elevation{0};        // Z / elevation (+0x14)
    std::uint16_t mode{0U};           // mode (+0x04)
    std::uint16_t facing{0U};         // facing direction (+0x2A)
    std::uint8_t mode_byte{0U};       // mode / state byte (+0x39)
    std::uint8_t counter_9e{0U};      // state counter (+0x9E)
    std::uint8_t counter_a2{0U};      // secondary counter (+0xA2)
    std::uint16_t subpixel_vel{0U};   // subpixel velocity / flags (+0x06)
    bool state_synced{false};         // helper 0x01378A executed (Dytto / Efreet)
    bool projected{false};            // helper 0x0147E0 executed (Shade)

    [[nodiscard]] std::int16_t x() const noexcept {
        return static_cast<std::int16_t>(x_fixed >> 16);
    }
    [[nodiscard]] std::int16_t y() const noexcept {
        return static_cast<std::int16_t>(y_fixed >> 16);
    }
    void set_pos(std::int16_t px, std::int16_t py) noexcept {
        x_fixed = static_cast<std::int32_t>(px) << 16;
        y_fixed = static_cast<std::int32_t>(py) << 16;
    }
};

/// BGM track synchronization result (0x00CD2C..0x00CD44).
struct BgmSyncResult {
    std::uint16_t sound_cmd{0U};
    std::uint16_t current_bgm{0U};
    bool track_changed{false};
};

/// Room collision table pointer resolution result (0x00CCA4..0x00CCD4).
struct CollisionTableResult {
    std::uint32_t collision_ptr{0U};
    std::uint8_t cadence_timer{10U};
    bool busy_flag_set{true};
};

/// Configuration and constants for ROM Companion Spirit Warp Repositioning,
/// Room Collision Table Loader & BGM Engine Synchronizer (0x00CC12..0x00CD64, 338 bytes).
struct SpiritWarpBgmConfig {
    static constexpr std::uint32_t kEntryRepositionSpiritAddr = 0x00CC12U;
    static constexpr std::uint32_t kEntryLoadCollisionTableAddr = 0x00CCA4U;
    static constexpr std::uint32_t kEntryInitRoomEnvSyncBgmAddr = 0x00CCE6U;
    static constexpr std::uint32_t kEntrySyncBgmTrackAddr = 0x00CD2CU;
    static constexpr std::uint32_t kEntryLatchAllGroundSurfacesAddr = 0x00CD4AU;
    static constexpr std::uint32_t kRangeEndAddr = 0x00CD64U;
    static constexpr std::size_t kRoutineSizeBytes = 338U;

    // ROM Table Locations
    static constexpr std::uint32_t kCollisionAreaTableAddr = 0x04371EU;
    static constexpr std::size_t kCollisionAreaCount = 32U;
    static constexpr std::uint8_t kCollisionTickCadence = 10U; // 0x0A frames

    // Memory Addresses
    static constexpr std::uint32_t kSpiritSlotAddr = 0x00FF1AA4U;
    static constexpr std::uint32_t kPlayerAliSlotAddr = 0x00FF19E8U;
    static constexpr std::uint32_t kCollisionTablePtrAddr = 0x00FF17B2U;
    static constexpr std::uint32_t kCollisionTickCounterAddr = 0x00FF17B6U;
    static constexpr std::uint32_t kCollisionTickCadenceAddr = 0x00FF17B7U;
    static constexpr std::uint32_t kRoomSpiritForbidAddr = 0x00FF16F1U;
    static constexpr std::uint32_t kTargetTrackAddr = 0x00FF16F4U;
    static constexpr std::uint32_t kCurrentTrackAddr = 0x00FF16F2U;
    static constexpr std::uint32_t kVdpBusyFlagAddr = 0x00FF164DU;
    static constexpr std::uint32_t kVIntBusyFlagAddr = 0x00FF164EU;
    static constexpr std::uint32_t kStaleEnemyHudTrackerAddr = 0x00FF184FU;
    static constexpr std::uint32_t kSatLinkCounterAddr = 0x00FF188AU;
    static constexpr std::uint32_t kSatBufferBaseAddr = 0x00FF13CCU;
    static constexpr std::uint32_t kSoundDriverPort = 0x00060004U;

    // Entity Slot Parameters
    static constexpr std::size_t kEntityGroup2Count = 21U;
    static constexpr std::size_t kEntitySlotStride = 0xBCU; // 188 bytes

    // Positioning and Sound Constants
    static constexpr std::int16_t kSpiritOffsetY = -8;
    static constexpr std::uint16_t kBgmCmdContinue = 3U;

    // Spirit Type Codes
    static constexpr std::uint16_t kTypeUnsummonBow = 0x0016U;
    static constexpr std::uint16_t kTypeResetState = 0x001AU;
    static constexpr std::uint16_t kTypeDyttoEfreet = 0x001CU;
    static constexpr std::uint16_t kTypeShade = 0x001EU;

    // 32-Area Collision Pointer Table at 0x04371E in ROM
    static constexpr std::array<std::uint32_t, kCollisionAreaCount> kCollisionAreaTable = {
        0x04379EU, 0x046332U, 0x046B9AU, 0x04734EU, 0x04735CU, 0x047E98U, 0x049046U, 0x049DB0U,
        0x04A8CEU, 0x04B6CEU, 0x04C520U, 0x04D5F4U, 0x04E5E2U, 0x0437F0U, 0x05016AU, 0x0437D8U,
        0x0437F0U, 0x0437F0U, 0x050646U, 0x050648U, 0x05064AU, 0x0437F0U, 0x0437F0U, 0x0437F0U,
        0x0437F0U, 0x0437F0U, 0x0437F0U, 0x0437F0U, 0x0437F0U, 0x0437F0U, 0x0437F0U, 0x0437DCU
    };
};

/// Repositions companion spirit on room warp entry with 16.16 fixed-point coordinates (0x00CC12..0x00CCA4).
void reposition_spirit_on_room_warp_fixed(
    SpiritState& spirit,
    std::int32_t ali_x_fixed,
    std::int32_t ali_y_fixed,
    std::int16_t ali_elevation,
    bool room_forbids_spirits = false) noexcept;

/// Repositions companion spirit on room warp entry with integer pixel coordinates.
inline void reposition_spirit_on_room_warp(
    SpiritState& spirit,
    std::int32_t ali_x_pixel,
    std::int32_t ali_y_pixel,
    std::int16_t ali_elevation = 0,
    bool room_forbids_spirits = false) noexcept {
    reposition_spirit_on_room_warp_fixed(
        spirit,
        ali_x_pixel << 16,
        ali_y_pixel << 16,
        ali_elevation,
        room_forbids_spirits);
}

/// Resolves collision table pointer for a packed room ID from ROM (0x00CCA4..0x00CCD4).
[[nodiscard]] std::uint32_t resolve_collision_table_pointer(
    std::span<const std::uint8_t> rom,
    std::uint16_t packed_id) noexcept;

/// Loads room collision table, sets cadence timer to 10 frames and sets bit 5 of VDP flag (0x00CCA4..0x00CCE2).
[[nodiscard]] CollisionTableResult load_room_collision_table(
    std::span<const std::uint8_t> rom,
    std::uint16_t packed_id) noexcept;

/// Synchronizes BGM track: sends command 3 (continue) or updates current track and sends new track ID (0x00CD2C..0x00CD44).
[[nodiscard]] BgmSyncResult synchronize_bgm_track(
    std::uint16_t current_bgm,
    std::uint16_t target_bgm) noexcept;

/// Sweeps 21 Group 2 entities, counting or latching ground surface for active slots (0x00CD4A..0x00CD64).
[[nodiscard]] std::size_t count_active_group2_entities(
    std::span<const std::uint8_t> ram) noexcept;

// Raw 64KB RAM window integration helpers
void reposition_spirit_raw(std::span<std::uint8_t> ram, bool room_forbids_spirits = false) noexcept;
std::uint32_t load_room_collision_table_raw(std::span<std::uint8_t> ram, std::span<const std::uint8_t> rom, std::uint16_t packed_id) noexcept;
BgmSyncResult synchronize_bgm_track_raw(std::span<std::uint8_t> ram) noexcept;
std::size_t latch_all_entity_ground_surfaces_raw(std::span<std::uint8_t> ram) noexcept;
BgmSyncResult init_room_environment_and_sync_bgm_raw(std::span<std::uint8_t> ram, std::span<const std::uint8_t> rom, std::uint16_t packed_id) noexcept;

} // namespace oasis::game::world

namespace thor::environment {
using SpiritState = oasis::game::world::SpiritState;
using BgmSyncResult = oasis::game::world::BgmSyncResult;
using CollisionTableResult = oasis::game::world::CollisionTableResult;
using SpiritWarpBgmConfig = oasis::game::world::SpiritWarpBgmConfig;

using oasis::game::world::reposition_spirit_on_room_warp;
using oasis::game::world::reposition_spirit_on_room_warp_fixed;
using oasis::game::world::resolve_collision_table_pointer;
using oasis::game::world::load_room_collision_table;
using oasis::game::world::synchronize_bgm_track;
using oasis::game::world::count_active_group2_entities;
} // namespace thor::environment

namespace thor::world {
using namespace thor::environment;
} // namespace thor::world
