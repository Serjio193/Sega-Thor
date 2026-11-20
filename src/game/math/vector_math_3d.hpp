#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "isqrt_atan2.hpp"

namespace thor::math {

// ROM entrypoints and boundaries (0x00F054..0x00F1B2, 350 bytes).
inline constexpr std::uint32_t kVectorMath3dStartAddr = 0x00F054U;
inline constexpr std::uint32_t kBoss3ARockLaunchSetupAddr = 0x00F054U;
inline constexpr std::uint32_t kBoss3AHudTrackerHookAddr = 0x00BE98U;
inline constexpr std::uint32_t kBoss3ARockVelocityTableAddr = 0x00F0C2U;
inline constexpr std::uint32_t kDistance2dAddr = 0x00F0D2U;
inline constexpr std::uint32_t kDistance3dAddr = 0x00F0ECU;
inline constexpr std::uint32_t kNormalizeVelocity3dAddr = 0x00F10CU;
inline constexpr std::uint32_t kClampToSphere3dAddr = 0x00F160U;
inline constexpr std::uint32_t kVectorMath3dEndAddr = 0x00F1B2U;
inline constexpr std::size_t kVectorMath3dSizeBytes = 350U;

// External subroutines invoked by Boss3A_RockProjectileLaunchSetup (0x00F054).
inline constexpr std::uint32_t kProjectVelocity8WayRefAddr = 0x00E2D4U;
inline constexpr std::uint32_t kResolveCardinalDirection4WayRefAddr = 0x00E338U;

// Work RAM and entity slot offsets observed in 0x00F054..0x00F0C2 and 0x00BE98.
inline constexpr std::uint32_t kWramBaseAddr = 0x00FF0000U;
inline constexpr std::size_t kWramSizeBytes = 0x10000U;
inline constexpr std::uint32_t kPlayerEntityRamAddr = 0x00FF19E8U;
inline constexpr std::uint32_t kEnemyHudTimerRamAddr = 0x00FF184FU;
inline constexpr std::size_t kEnemyHudTimerRamOffset = 0x184FU;
inline constexpr std::uint32_t kEnemyHudTargetPtrRamAddr = 0x00FF1850U;
inline constexpr std::size_t kEnemyHudTargetPtrRamOffset = 0x1850U;
inline constexpr std::uint8_t kEnemyHudLockDurationFrames = 0x5AU;

inline constexpr std::size_t kEntityPosXFpOffset = 0x08U;         // s32 16.16 (high word = s16)
inline constexpr std::size_t kEntityPosYFpOffset = 0x0CU;         // s32 16.16 (high word = s16)
inline constexpr std::size_t kEntityPosZFpOffset = 0x10U;         // s32 16.16 (high word = s16)
inline constexpr std::size_t kEntityHudFlagsOffset = 0x3AU;       // u8 (bit 0 suppresses HUD lock)
inline constexpr std::size_t kEntityVelXFpOffset = 0x4EU;         // s32 16.16 X velocity
inline constexpr std::size_t kEntityVelYFpOffset = 0x52U;         // s32 16.16 Y velocity
inline constexpr std::size_t kEntityVelZFpOffset = 0x56U;         // s32 16.16 Z velocity
inline constexpr std::size_t kEntityInheritedParamOffset = 0x7EU; // u32 (copied from spawner +$AA)
inline constexpr std::size_t kEntityFacingDir4WayOffset = 0x84U;  // u16 (0=N, 1=E, 2=S, 3=W)
inline constexpr std::size_t kSpawnerAttackPhaseOffset = 0xA6U;   // u16 attack phase index
inline constexpr std::size_t kSpawnerProjectileParamOffset = 0xAAU;

inline constexpr std::uint32_t kBoss3ACloseRangeThresholdFp = 0x00080000U; // 8.0 px in 16.16
inline constexpr std::uint16_t kBoss3ACloseRangeTableIndex = 3U;
inline constexpr std::uint32_t kVelocityNormalizeShift = 12U; // swap + clr.w + asr.l #4

struct Vec3i16 {
    std::int16_t x{0};
    std::int16_t y{0};
    std::int16_t z{0};

    [[nodiscard]] constexpr bool operator==(const Vec3i16&) const noexcept = default;
};

struct Vec3Fixed {
    std::int32_t vx{0};
    std::int32_t vy{0};
    std::int32_t vz{0};

    [[nodiscard]] constexpr bool operator==(const Vec3Fixed&) const noexcept = default;
};

struct SphereClampResult {
    Vec3i16 position{};
    std::uint16_t distance{0};
    bool clamped{false};

    [[nodiscard]] constexpr bool operator==(const SphereClampResult&) const noexcept = default;
};

struct Boss3ARockVelocityEntry {
    std::uint16_t speed_xy_8_8{0};
    std::int16_t launch_z_8_8{0};

    [[nodiscard]] constexpr bool operator==(const Boss3ARockVelocityEntry&) const noexcept = default;
};

// ROM table at 0x00F0C2..0x00F0D2 (4 entries x 4 bytes).
inline constexpr std::array<Boss3ARockVelocityEntry, 4> kBoss3ARockVelocityTable = {{
    {0x0300U, 0x0300},
    {0x0500U, 0x0400},
    {0x0700U, 0x0500},
    {0x0000U, 0x0800},
}};

struct Boss3ASpawnerState {
    std::int32_t x_fp{0};
    std::int32_t y_fp{0};
    std::uint16_t attack_phase_index{0};
    std::uint32_t param_aa{0};
};

struct Boss3AProjectileState {
    std::int32_t x_fp{0};
    std::int32_t y_fp{0};
    std::uint8_t flags_3a{0};
    std::uint32_t target_ram_addr{0};
    std::int32_t vel_x_fp{0};
    std::int32_t vel_y_fp{0};
    std::int32_t vel_z_fp{0};
    std::uint32_t param_7e{0};
    std::uint16_t facing_dir_4way{0};
};

struct Boss3ALaunchResult {
    std::uint16_t facing_dir_4way{0};
    std::uint16_t requested_table_index{0};
    std::uint16_t effective_table_index{0};
    std::uint32_t max_axis_abs_fp{0};
    bool close_range_override{false};
    bool table_index_clamped{false};
    bool hud_tracker_updated{false};
    std::int32_t vel_x_fp{0};
    std::int32_t vel_y_fp{0};
    std::int32_t vel_z_fp{0};

    [[nodiscard]] constexpr bool operator==(const Boss3ALaunchResult&) const noexcept = default;
};

/// 0x00F0D2: 2D integer Euclidean distance using 16-bit deltas and isqrt_u32.
[[nodiscard]] std::uint16_t distance_2d(
    std::int16_t x0,
    std::int16_t y0,
    std::int16_t x1,
    std::int16_t y1) noexcept;

/// 0x00F0EC: 3D integer Euclidean distance using 16-bit deltas and isqrt_u32.
[[nodiscard]] std::uint16_t distance_3d(Vec3i16 p0, Vec3i16 p1) noexcept;

/// 0x00F10C: Scale direction (p1 - p0) / dist by signed 16-bit speed and shift left by 12 bits.
/// Note: matches M68K muls.w/divs.w 16-bit signed arithmetic; returns zero if dist == 0.
[[nodiscard]] Vec3Fixed normalize_velocity_3d(
    Vec3i16 p0,
    Vec3i16 p1,
    std::int16_t speed) noexcept;

/// 0x00F160: Clamp target inside sphere around origin with unsigned radius comparison.
/// When unsigned dist <= radius (including dist == 0), returns target unchanged with clamped=false.
/// When dist > radius, rescales delta by muls.w(int16(radius)) / divs.w(int16(dist)) with clamped=true.
[[nodiscard]] SphereClampResult clamp_to_sphere_3d(
    Vec3i16 origin,
    Vec3i16 target,
    std::uint16_t radius) noexcept;

/// Select entry from 0x00F0C2 table, clamping OOB indices >= 4 safely to index 3.
[[nodiscard]] Boss3ARockVelocityEntry select_boss3a_rock_velocity_entry(
    std::uint16_t index) noexcept;

/// 0x00F054 (+ 0x00BE98): Configure Boss 3A rock projectile velocity, facing, and inherited param.
Boss3ALaunchResult setup_boss3a_rock_launch(
    const Boss3ASpawnerState& spawner,
    Boss3AProjectileState& projectile) noexcept;

/// Read entity integer position words (+$08.w, +$0C.w, +$10.w) from big-endian Work RAM.
[[nodiscard]] Vec3i16 read_entity_pos_i16_from_ram(
    std::span<const std::uint8_t> wram,
    std::size_t entity_offset) noexcept;

/// Write entity integer position words (+$08.w, +$0C.w, +$10.w) to big-endian Work RAM.
void write_entity_pos_i16_to_ram(
    std::span<std::uint8_t> wram,
    std::size_t entity_offset,
    Vec3i16 pos) noexcept;

/// 0x00F0D2 over two entity slots in big-endian Work RAM.
[[nodiscard]] std::uint16_t distance_2d_entities_from_ram(
    std::span<const std::uint8_t> wram,
    std::size_t entity0_offset,
    std::size_t entity1_offset) noexcept;

/// 0x00F0EC over two entity slots in big-endian Work RAM.
[[nodiscard]] std::uint16_t distance_3d_entities_from_ram(
    std::span<const std::uint8_t> wram,
    std::size_t entity0_offset,
    std::size_t entity1_offset) noexcept;

/// 0x00F10C over origin/target entity slots in big-endian Work RAM.
[[nodiscard]] Vec3Fixed normalize_entity_velocity_3d_from_ram(
    std::span<const std::uint8_t> wram,
    std::size_t origin_entity_offset,
    std::size_t target_entity_offset,
    std::int16_t speed) noexcept;

/// 0x00F160 over origin/target entity slots in big-endian Work RAM; updates target when clamped.
[[nodiscard]] SphereClampResult clamp_entity_pos_to_sphere_from_ram(
    std::span<std::uint8_t> wram,
    std::size_t origin_entity_offset,
    std::size_t target_entity_offset,
    std::uint16_t radius) noexcept;

/// 0x00F054 over spawner/projectile entity slots in big-endian Work RAM (+ $FF184F/$FF1850 HUD hook).
[[nodiscard]] Boss3ALaunchResult setup_boss3a_rock_launch_from_ram(
    std::span<std::uint8_t> wram,
    std::size_t spawner_offset,
    std::size_t projectile_offset) noexcept;

} // namespace thor::math

namespace oasis::game::math {
using namespace thor::math;
} // namespace oasis::game::math
