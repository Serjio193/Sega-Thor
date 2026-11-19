#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace thor::physics {

// ROM entrypoints and boundaries (0x00E398..0x00E502, 362 bytes, 120 instructions).
inline constexpr std::uint32_t kPlayerTargetingStartAddr = 0x00E398U;

inline constexpr std::uint32_t kTargetPlayerAliQuadrantAngleAddr = 0x00E398U;
inline constexpr std::uint32_t kTargetPlayerAliQuadrantAngleEndAddr = 0x00E3E8U;
inline constexpr std::size_t kTargetPlayerAliQuadrantAngleSizeBytes = 80U;

inline constexpr std::uint32_t kTargetPlayerAliLoadMotionTableAddr = 0x00E3E8U;
inline constexpr std::uint32_t kTargetPlayerAliLoadMotionTableEndAddr = 0x00E45EU;
inline constexpr std::size_t kTargetPlayerAliLoadMotionTableSizeBytes = 118U;

inline constexpr std::uint32_t kNormalizeTargetAimVectorDivsAddr = 0x00E45EU;
inline constexpr std::uint32_t kNormalizeTargetAimVectorDivsEndAddr = 0x00E4D0U;
inline constexpr std::size_t kNormalizeTargetAimVectorDivsSizeBytes = 114U;

inline constexpr std::uint32_t kLaunchDirectionalProjectilePhysicsAddr = 0x00E4D0U;
inline constexpr std::uint32_t kLaunchDirectionalProjectilePhysicsEndAddr = 0x00E502U;
inline constexpr std::size_t kLaunchDirectionalProjectilePhysicsSizeBytes = 50U;

inline constexpr std::uint32_t kPlayerTargetingEndAddr = 0x00E502U;
inline constexpr std::size_t kPlayerTargetingSizeBytes = 362U;

// Player Ali world coordinates in 64KB Work RAM ($FF19E8 + $08 / +$0C integer words).
inline constexpr std::uint32_t kPlayerAliPosXRamAddr = 0x00FF19F0U;
inline constexpr std::uint32_t kPlayerAliPosYRamAddr = 0x00FF19F4U;
inline constexpr std::size_t kPlayerAliPosXRamOffset = 0x19F0U;
inline constexpr std::size_t kPlayerAliPosYRamOffset = 0x19F4U;

// Entity slot offsets accessed by 0x00E398..0x00E502 (relative to entity base a6).
inline constexpr std::size_t kTargetEntityTimerOffset = 0x02U;     // u16 motion timer
inline constexpr std::size_t kTargetEntityPosXOffset = 0x08U;      // s16 integer X (high word of +$08.l)
inline constexpr std::size_t kTargetEntityPosYOffset = 0x0CU;      // s16 integer Y (high word of +$0C.l)
inline constexpr std::size_t kTargetEntityFacingOffset = 0x16U;    // u16 cardinal facing 0..3
inline constexpr std::size_t kTargetEntityVelXFpOffset = 0x4EU;    // s32 16.16 X velocity
inline constexpr std::size_t kTargetEntityVelYFpOffset = 0x52U;    // s32 16.16 Y velocity
inline constexpr std::size_t kTargetEntityVelZFpOffset = 0x56U;    // s32 16.16 Z velocity
inline constexpr std::size_t kTargetEntityLaunchDirOffset = 0x84U; // u16 8-way direction (0..7)
inline constexpr std::size_t kTargetEntityMinStructBytes = 0x86U;

// Cardinal direction codes (d0 / d3 / +$16).
inline constexpr std::uint8_t kTargetDirUp = 0U;
inline constexpr std::uint8_t kTargetDirRight = 1U;
inline constexpr std::uint8_t kTargetDirDown = 2U;
inline constexpr std::uint8_t kTargetDirLeft = 3U;

// Motion table & clamp constants (0x00E3E8..0x00E45E).
inline constexpr std::uint16_t kTargetEntityMinMotionTimer = 16U; // cmpi.w #$10, d5
inline constexpr std::size_t kTargetMotionTableEntryBytes = 8U;   // d0 << 3
inline constexpr std::size_t kTargetMotionTableEntries = 4U;

// Launch / knockback projectile constants (0x00E4D0..0x00E502).
inline constexpr std::uint16_t kLaunchSuppressMask = 0xC000U;      // d7 & 0xC000 != 0 -> early rts
inline constexpr std::uint16_t kLaunchBaseSpeedX = 0x0300U;        // 8.8 speed X (3.0 px/frame)
inline constexpr std::uint16_t kLaunchBaseSpeedY = 0x02E0U;        // 8.8 speed Y (2.875 px/frame)
inline constexpr std::int32_t kLaunchUpwardVelZFp = 0x00060000;    // 16.16 +6.0 px/frame

/// Output of 0x00E398..0x00E3E8 (`target_player_ali_quadrant_angle`).
struct TargetResult {
    std::uint8_t direction{0U};  // d0: 0=Up, 1=Right, 2=Down, 3=Left
    std::uint16_t distance{0U};  // d5: 16-bit wrapping Manhattan distance |dx| + |dy|
    std::int16_t dx{0};          // d1: (int16_t)(player_x - entity_x)
    std::int16_t dy{0};          // d2: (int16_t)(player_y - entity_y)
    std::uint16_t abs_x{0U};     // d3: 16-bit |dx|
    std::uint16_t abs_y{0U};     // d4: 16-bit |dy|

    [[nodiscard]] constexpr bool operator==(const TargetResult&) const noexcept = default;
};

/// Single 8-byte (vx, vy) 16.16 fixed-point entry in a cardinal motion table passed in a0 to 0x00E3E8.
struct CardinalMotionEntry {
    std::int32_t vel_x_fp{0};
    std::int32_t vel_y_fp{0};

    [[nodiscard]] constexpr bool operator==(const CardinalMotionEntry&) const noexcept = default;
};

using CardinalMotionTable = std::array<CardinalMotionEntry, kTargetMotionTableEntries>;

/// Entity slot projection for 0x00E398..0x00E502.
struct TargetingEntityState {
    std::uint16_t timer_02{0U};      // +$02.w motion countdown
    std::int16_t pos_x{0};           // +$08.w integer X
    std::int16_t pos_y{0};           // +$0C.w integer Y
    std::uint16_t facing_16{0U};     // +$16.w cardinal facing (0..3)
    std::int32_t vel_x_fp{0};        // +$4E.l 16.16 X velocity
    std::int32_t vel_y_fp{0};        // +$52.l 16.16 Y velocity
    std::int32_t vel_z_fp{0};        // +$56.l 16.16 Z velocity
    std::uint16_t launch_dir_84{0U}; // +$84.w 8-way launch/knockback direction

    [[nodiscard]] constexpr bool operator==(const TargetingEntityState&) const noexcept = default;
};

/// Result summary from 0x00E3E8..0x00E45E (`target_player_ali_and_load_motion_table`).
struct MotionTableLoadResult {
    std::uint8_t direction{0U};          // +$16(a6) unscaled direction (0..3)
    std::uint16_t table_byte_offset{0U}; // d0 on return: direction * 8
    std::uint16_t motion_timer{0U};      // d5 / +$02(a6): max(dominant_dist, 16)
    std::int32_t vel_x_fp{0};            // +$4E(a6)
    std::int32_t vel_y_fp{0};            // +$52(a6)

    [[nodiscard]] constexpr bool operator==(const MotionTableLoadResult&) const noexcept = default;
};

/// Output of 0x00E45E..0x00E4D0 (`normalize_target_aim_vector_divs`).
struct NormalizedAimVector {
    std::int32_t vel_x_fp{0};     // d1: 16.16 fixed-point X velocity
    std::int32_t vel_y_fp{0};     // d2: 16.16 fixed-point Y velocity
    std::uint16_t quadrant{0U};   // d3: 0..3, or preserved current_facing on zero fallback
    std::uint16_t half_span{0U};  // d4: max(|dx|, |dy|) >> 1, or 0 on zero fallback

    [[nodiscard]] constexpr bool operator==(const NormalizedAimVector&) const noexcept = default;
};

/// Result summary from 0x00E4D0..0x00E502 (`launch_directional_projectile_physics`).
struct ProjectileLaunchResult {
    bool launched{false};         // false when (d7 & 0xC000) != 0
    std::int32_t vel_x_fp{0};     // +$4E(a6)
    std::int32_t vel_y_fp{0};     // +$52(a6)
    std::int32_t vel_z_fp{0};     // +$56(a6) = 0x00060000 when launched

    [[nodiscard]] constexpr bool operator==(const ProjectileLaunchResult&) const noexcept = default;
};

/// 0x00E398..0x00E3E8: Computes delta to Player Ali, Manhattan distance, and dominant cardinal direction.
/// Note: ties (`|dx| == |dy|`) resolve to X-dominant (`Right` / `Left`) per `cmp.w d3, d4; bhi`.
[[nodiscard]] constexpr TargetResult target_player_ali_quadrant_angle(
    std::int16_t player_x,
    std::int16_t player_y,
    std::int16_t entity_x,
    std::int16_t entity_y) noexcept {
    const auto dx = static_cast<std::int16_t>(
        static_cast<std::uint16_t>(player_x) - static_cast<std::uint16_t>(entity_x));
    const auto dy = static_cast<std::int16_t>(
        static_cast<std::uint16_t>(player_y) - static_cast<std::uint16_t>(entity_y));
    const auto abs_x = (dx < 0)
        ? static_cast<std::uint16_t>(0U - static_cast<std::uint16_t>(dx))
        : static_cast<std::uint16_t>(dx);
    const auto abs_y = (dy < 0)
        ? static_cast<std::uint16_t>(0U - static_cast<std::uint16_t>(dy))
        : static_cast<std::uint16_t>(dy);
    const auto manhattan = static_cast<std::uint16_t>(abs_x + abs_y);

    std::uint8_t dir = kTargetDirUp;
    if (abs_y > abs_x) {
        dir = (dy < 0) ? kTargetDirUp : kTargetDirDown;
    } else {
        dir = (dx < 0) ? kTargetDirLeft : kTargetDirRight;
    }

    return TargetResult{
        .direction = dir,
        .distance = manhattan,
        .dx = dx,
        .dy = dy,
        .abs_x = abs_x,
        .abs_y = abs_y,
    };
}

/// 0x00E45E..0x00E4D0: Normalizes aim vector via signed M68K `divs.w` (truncation toward zero).
/// Note: ties (`|dx| == |dy|`) resolve to Y-dominant (`cmp.w d5, d4; bhi`), and `d4` returns
/// `half_span` (`max_span >> 1`) or `0` on the early/zero fallback path.
[[nodiscard]] constexpr NormalizedAimVector normalize_target_aim_vector_divs(
    std::int16_t target_x,
    std::int16_t target_y,
    std::int16_t entity_x,
    std::int16_t entity_y,
    std::uint16_t current_facing = 0U) noexcept {
    const auto dx = static_cast<std::int16_t>(
        static_cast<std::uint16_t>(target_x) - static_cast<std::uint16_t>(entity_x));
    const auto dy = static_cast<std::int16_t>(
        static_cast<std::uint16_t>(target_y) - static_cast<std::uint16_t>(entity_y));
    const auto abs_x = (dx < 0)
        ? static_cast<std::uint16_t>(0U - static_cast<std::uint16_t>(dx))
        : static_cast<std::uint16_t>(dx);
    const auto abs_y = (dy < 0)
        ? static_cast<std::uint16_t>(0U - static_cast<std::uint16_t>(dy))
        : static_cast<std::uint16_t>(dy);

    std::uint16_t dom_axis = 1U;
    std::uint16_t max_span = abs_x;
    if (abs_x <= abs_y) {
        max_span = abs_y;
        dom_axis = 0U;
    }

    const auto half_span_s16 = static_cast<std::int16_t>(
        static_cast<std::int16_t>(max_span) >> 1);
    if (half_span_s16 == 0) {
        return NormalizedAimVector{
            .vel_x_fp = 0,
            .vel_y_fp = 0,
            .quadrant = current_facing,
            .half_span = 0U,
        };
    }

    const auto num_x = static_cast<std::int32_t>(
        static_cast<std::uint32_t>(static_cast<std::int32_t>(dx)) << 8U);
    const auto num_y = static_cast<std::int32_t>(
        static_cast<std::uint32_t>(static_cast<std::int32_t>(dy)) << 8U);

    const auto qx = static_cast<std::int16_t>(num_x / static_cast<std::int32_t>(half_span_s16));
    const auto qy = static_cast<std::int16_t>(num_y / static_cast<std::int32_t>(half_span_s16));
    const auto vel_x = static_cast<std::int32_t>(
        static_cast<std::uint32_t>(static_cast<std::int32_t>(qx)) << 8U);
    const auto vel_y = static_cast<std::int32_t>(
        static_cast<std::uint32_t>(static_cast<std::int32_t>(qy)) << 8U);

    if ((vel_x | vel_y) == 0) {
        return NormalizedAimVector{
            .vel_x_fp = 0,
            .vel_y_fp = 0,
            .quadrant = current_facing,
            .half_span = 0U,
        };
    }

    std::uint16_t quad = dom_axis;
    if (dom_axis != 0U) {
        if (vel_x < 0) {
            quad = static_cast<std::uint16_t>(quad + 2U);
        }
    } else {
        if (vel_y >= 0) {
            quad = static_cast<std::uint16_t>(quad + 2U);
        }
    }

    return NormalizedAimVector{
        .vel_x_fp = vel_x,
        .vel_y_fp = vel_y,
        .quadrant = quad,
        .half_span = static_cast<std::uint16_t>(half_span_s16),
    };
}

/// Decodes a 32-byte big-endian 4-direction (vx, vy) motion table (0x00E3E8 a0 table format).
[[nodiscard]] CardinalMotionTable decode_cardinal_motion_table(
    std::span<const std::uint8_t> table_bytes) noexcept;

/// 0x00E3E8..0x00E45E: Resolves cardinal direction to Ali (ties favor Y per `cmp.w d4, d3; bls`),
/// stores +$16, loads 8-byte (vx, vy) from motion_table into +$4E/+$52, and stores max(dom_dist, 16) in +$02.
MotionTableLoadResult target_player_ali_and_load_motion_table(
    std::int16_t player_x,
    std::int16_t player_y,
    TargetingEntityState& entity,
    const CardinalMotionTable& motion_table) noexcept;

/// 0x00E4D0..0x00E502: If (status_mask_d7 & 0xC000) == 0, projects (0x0300, 0x02E0) along
/// entity.launch_dir_84 & 7 into +$4E/+$52 and sets upward velocity +$56 = 0x00060000.
ProjectileLaunchResult launch_directional_projectile_physics(
    TargetingEntityState& entity,
    std::uint16_t status_mask_d7) noexcept;

// ============================================================================
// 64KB Work RAM Window Integration Helpers
// ============================================================================

[[nodiscard]] TargetingEntityState load_targeting_entity_state_from_ram(
    std::span<const std::uint8_t> ram,
    std::uint32_t entity_ram_addr) noexcept;

void store_targeting_entity_state_to_ram(
    std::span<std::uint8_t> ram,
    std::uint32_t entity_ram_addr,
    const TargetingEntityState& state) noexcept;

[[nodiscard]] TargetResult target_player_ali_quadrant_angle_from_ram(
    std::span<const std::uint8_t> ram,
    std::uint32_t entity_ram_addr) noexcept;

MotionTableLoadResult target_player_ali_and_load_motion_table_from_ram(
    std::span<std::uint8_t> ram,
    std::uint32_t entity_ram_addr,
    const CardinalMotionTable& motion_table) noexcept;

[[nodiscard]] NormalizedAimVector normalize_target_aim_vector_from_ram(
    std::span<const std::uint8_t> ram,
    std::uint32_t entity_ram_addr,
    std::int16_t target_x,
    std::int16_t target_y) noexcept;

ProjectileLaunchResult launch_directional_projectile_physics_from_ram(
    std::span<std::uint8_t> ram,
    std::uint32_t entity_ram_addr,
    std::uint16_t status_mask_d7) noexcept;

} // namespace thor::physics

namespace oasis::game::physics {
using namespace thor::physics;
} // namespace oasis::game::physics
