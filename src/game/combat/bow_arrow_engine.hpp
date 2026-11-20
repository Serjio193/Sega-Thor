#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace thor::combat {

// ROM entrypoints, inline tables, and boundaries (0x00F396..0x00F476, 224 bytes).
inline constexpr std::uint32_t kBowArrowStartAddr = 0x00F396U;
inline constexpr std::uint32_t kBowArrowGroundEntryAddr = 0x00F396U;
inline constexpr std::size_t kBowArrowGroundEntrySizeBytes = 10U;
inline constexpr std::uint32_t kBowArrowGroundOffsetsAddr = 0x00F3A0U;
inline constexpr std::size_t kBowArrowGroundOffsetsSizeBytes = 8U;

inline constexpr std::uint32_t kBowArrowWaterEntryAddr = 0x00F3A8U;
inline constexpr std::size_t kBowArrowWaterEntrySizeBytes = 10U;
inline constexpr std::uint32_t kBowArrowWaterOffsetsAddr = 0x00F3B2U;
inline constexpr std::size_t kBowArrowWaterOffsetsSizeBytes = 8U;

inline constexpr std::uint32_t kBowArrowSharedInitAddr = 0x00F3BAU;
inline constexpr std::size_t kBowArrowSharedInitSizeBytes = 188U;
inline constexpr std::uint32_t kBowArrowEndAddr = 0x00F476U;
inline constexpr std::size_t kBowArrowSizeBytes = 224U;

// Observed USA ROM callers and helper callees.
inline constexpr std::uint32_t kBowArrowGroundCallerAddr = 0x0073C0U;
inline constexpr std::uint32_t kBowArrowWaterCallerAddr = 0x00741AU;
inline constexpr std::uint32_t kBowArrowDurabilityHelperAddr = 0x008530U;
inline constexpr std::uint32_t kBowArrowConstructorAddr = 0x008D06U;
inline constexpr std::uint32_t kBowArrowTerrainProbeAddr = 0x009D00U;
inline constexpr std::uint32_t kBowArrowVelocityProjectorAddr = 0x00E2D4U;

// Fixed projectile Work RAM slot ($FF1B60, 188 bytes) and slow-motion / double-anchor flag ($FF17B8:0).
inline constexpr std::uint32_t kBowArrowSlotRamAddr = 0x00FF1B60U;
inline constexpr std::size_t kBowArrowSlotRamOffset = 0x1B60U;
inline constexpr std::size_t kBowArrowSlotStrideBytes = 0xBCU;
inline constexpr std::uint32_t kBowArrowPauseSlowmoRamAddr = 0x00FF17B8U;
inline constexpr std::size_t kBowArrowPauseSlowmoRamOffset = 0x17B8U;
inline constexpr std::uint8_t kBowArrowSlowmoDoubleBitMask = 0x01U;

// Entity fields & parabolic arc constants (0x00F3C2..0x00F474).
inline constexpr std::uint16_t kBowArrowTypeId = 0x003AU;
inline constexpr std::uint16_t kBowArrowDamageFlags = 0x8005U;
inline constexpr std::uint16_t kBowArrowElevationThresholdPx = 0x0010U; // cmpi.w #$10, $10(a6); bcc.b
inline constexpr std::uint16_t kBowArrowLowArcSpeed = 0x0400U;
inline constexpr std::int32_t kBowArrowLowArcVelZFp = 0x00028000;
inline constexpr std::uint16_t kBowArrowHighArcSpeed = 0x0300U;
inline constexpr std::int32_t kBowArrowHighArcVelZFp = 0x00020000;
inline constexpr std::int16_t kBowArrowGroundCallerElevDefault = 0x0022; // 34 px at 0x0073B4
inline constexpr std::int16_t kBowArrowWaterCallerElevDefault = 0x0004;  // 4 px at 0x007412

/// Packed signed byte pair `(dx, dy)` from 0x00F3A0 (ground) or 0x00F3B2 (water).
struct BowArrowTipOffset {
    std::int8_t dx{0};
    std::int8_t dy{0};

    [[nodiscard]] constexpr bool operator==(const BowArrowTipOffset&) const noexcept = default;
};

// Raw 16-bit big-endian table words verified from ROM 0x00F3A0..0x00F3A8 and 0x00F3B2..0x00F3BA.
inline constexpr std::array<std::uint16_t, 4> kBowArrowGroundTipWords = {
    0x06C8U, 0x20DEU, 0xF6F8U, 0xE0DEU
};
inline constexpr std::array<std::uint16_t, 4> kBowArrowWaterTipWords = {
    0x0EEEU, 0x1204U, 0xFA14U, 0xEE04U
};

inline constexpr std::array<BowArrowTipOffset, 4> kBowArrowGroundTipOffsets = {{
    {.dx = 6, .dy = -56},   // 0: Up
    {.dx = 32, .dy = -34},  // 1: Right
    {.dx = -10, .dy = -8},  // 2: Down
    {.dx = -32, .dy = -34}, // 3: Left
}};

inline constexpr std::array<BowArrowTipOffset, 4> kBowArrowWaterTipOffsets = {{
    {.dx = 14, .dy = -18},  // 0: Up
    {.dx = 18, .dy = 4},    // 1: Right
    {.dx = -6, .dy = 20},   // 2: Down
    {.dx = -18, .dy = 4},   // 3: Left
}};

/// Arrow variant word passed in d0 and stored to +$A6(a6).
enum class BowArrowType : std::uint16_t {
    Normal = 0U,
    Fire = 1U,
    Bomb = 2U,
};

/// Surface entrypoint selecting ground (0x00F396 / 0x00F3A0) vs water (0x00F3A8 / 0x00F3B2).
enum class BowArrowSurface : std::uint8_t {
    Ground = 0U,
    Water = 1U,
};

/// Resolved parabolic speed/Z-velocity arc from `cmpi.w #$10, $10(a6); bcc.b`.
struct BowArrowLaunchParams {
    std::uint16_t subtype_a6{0U};
    std::int16_t initial_elevation{0};
    bool high_or_wrapped_arc{false}; // true when uint16_t(elevation) >= 16
    std::uint16_t speed_x{kBowArrowLowArcSpeed};
    std::uint16_t speed_y{kBowArrowLowArcSpeed};
    std::int32_t vel_z_fp{kBowArrowLowArcVelZFp};

    [[nodiscard]] constexpr bool operator==(const BowArrowLaunchParams&) const noexcept = default;
};

/// Initial 16.16 fixed-point launch anchor offsets written to +$72 / +$76.
struct BowArrowAnchor {
    std::int32_t anchor_dx_fp{0};
    std::int32_t anchor_dy_fp{0};

    [[nodiscard]] constexpr bool operator==(const BowArrowAnchor&) const noexcept = default;
};

/// Owner entity fields (Ali `a4`) read by 0x00F3BA..0x00F476.
struct BowArrowOwnerState {
    std::uint32_t owner_ram_addr{0x00FF19E8U};
    std::int32_t pos_x_fp{0};         // +$08.l
    std::int32_t pos_y_fp{0};         // +$0C.l
    std::uint16_t facing{0U};         // +$16.w (0..3 for table index; 0..7 for 0x00E2D4)
    std::uint16_t sprite_priority{0}; // +$5A.w -> copied to +$18(a6)

    [[nodiscard]] constexpr bool operator==(const BowArrowOwnerState&) const noexcept = default;
};

/// Projection of projectile slot $FF1B60 written by 0x00F3BA..0x00F476.
struct BowArrowSlotState {
    std::uint16_t type_id{kBowArrowTypeId};          // +$00.w (0x003A)
    std::uint16_t param_or_lifetime_02{0U};          // +$02.w (d2)
    std::uint16_t state_04{0U};                      // +$04.w (0)
    std::int32_t pos_x_fp{0};                        // +$08.l
    std::int32_t pos_y_fp{0};                        // +$0C.l
    std::int32_t pos_z_fp{0};                        // +$10.l (elevation << 16)
    std::uint16_t facing_16{0U};                     // +$16.w (cleared to 0!)
    std::uint16_t sprite_priority_18{0U};            // +$18.w (+$5A(a4))
    std::uint16_t anim_id_2a{0U};                    // +$2A.w (0)
    std::int32_t vel_x_fp{0};                        // +$4E.l
    std::int32_t vel_y_fp{0};                        // +$52.l
    std::int32_t vel_z_fp{kBowArrowLowArcVelZFp};    // +$56.l
    std::int32_t anchor_dx_fp{0};                    // +$72.l
    std::int32_t anchor_dy_fp{0};                    // +$76.l
    std::uint16_t subtype_a6{0U};                    // +$A6.w (d0)
    std::uint16_t damage_a8{kBowArrowDamageFlags};   // +$A8.w (0x8005)
    std::uint32_t owner_ptr_aa{0U};                  // +$AA.l (a4)

    [[nodiscard]] constexpr bool operator==(const BowArrowSlotState&) const noexcept = default;
};

/// Unpacks a big-endian arrow-tip word matching `move.b d0, d2; asr.w #8, d0; ext.w d2`.
[[nodiscard]] constexpr BowArrowTipOffset unpack_bow_arrow_tip_word(std::uint16_t packed_word) noexcept {
    return BowArrowTipOffset{
        .dx = static_cast<std::int8_t>((packed_word >> 8U) & 0xFFU),
        .dy = static_cast<std::int8_t>(packed_word & 0xFFU),
    };
}

/// Looks up ground (0x00F3A0) or water (0x00F3B2) tip `(dx, dy)` using `facing & 3`.
[[nodiscard]] constexpr BowArrowTipOffset get_bow_arrow_tip_offset(
    BowArrowSurface surface,
    std::uint16_t facing) noexcept {
    const std::size_t idx = static_cast<std::size_t>(facing & 3U);
    return (surface == BowArrowSurface::Water)
        ? kBowArrowWaterTipOffsets[idx]
        : kBowArrowGroundTipOffsets[idx];
}

/// Matches caller convention for `d2`: `(subtype << 4) + 1` on ground (0x0073BA) vs `(subtype << 3)` in water (0x007418).
[[nodiscard]] constexpr std::uint16_t compute_bow_arrow_caller_param_d2(
    BowArrowSurface surface,
    std::uint16_t subtype_d0) noexcept {
    return (surface == BowArrowSurface::Water)
        ? static_cast<std::uint16_t>(subtype_d0 << 3U)
        : static_cast<std::uint16_t>((subtype_d0 << 4U) + 1U);
}

/// Computes parabolic arc speeds and Z launch velocity via unsigned `cmpi.w #$10, d1; bcc.b`.
[[nodiscard]] BowArrowLaunchParams calculate_bow_arrow_launch_params(
    std::uint16_t subtype_d0,
    std::int16_t elevation_d1) noexcept;

/// Computes initial 16.16 anchor deltas (+$72, +$76) matching 0x00F3DE..0x00F40C.
[[nodiscard]] BowArrowAnchor compute_bow_arrow_anchor(
    BowArrowSurface surface,
    std::uint16_t owner_facing,
    std::int16_t elevation_d1,
    bool slowmo_double_bit0 = false) noexcept;

/// Executes full Bow Arrow spawner logic (0x00F396..0x00F476) in C++ struct space.
[[nodiscard]] BowArrowSlotState spawn_bow_arrow(
    BowArrowSurface surface,
    const BowArrowOwnerState& owner,
    std::uint16_t subtype_d0,
    std::int16_t elevation_d1,
    std::uint16_t param_d2,
    bool slowmo_double_bit0 = false) noexcept;

// ============================================================================
// 64KB Work RAM Window Helpers ($FF1B60 projectile slot & $FF17B8 bit 0)
// ============================================================================

[[nodiscard]] BowArrowSlotState load_bow_arrow_slot_from_ram(
    std::span<const std::uint8_t> ram,
    std::uint32_t slot_ram_addr = kBowArrowSlotRamAddr) noexcept;

void store_bow_arrow_slot_to_ram(
    std::span<std::uint8_t> ram,
    const BowArrowSlotState& state,
    std::uint32_t slot_ram_addr = kBowArrowSlotRamAddr) noexcept;

[[nodiscard]] BowArrowSlotState spawn_bow_arrow_from_ram(
    std::span<std::uint8_t> ram,
    BowArrowSurface surface,
    std::uint32_t owner_ram_addr,
    std::uint16_t subtype_d0,
    std::int16_t elevation_d1,
    std::uint16_t param_d2) noexcept;

[[nodiscard]] BowArrowSlotState spawn_bow_arrow_ground_from_ram(
    std::span<std::uint8_t> ram,
    std::uint32_t owner_ram_addr,
    std::uint16_t subtype_d0,
    std::int16_t elevation_d1,
    std::uint16_t param_d2) noexcept;

[[nodiscard]] BowArrowSlotState spawn_bow_arrow_water_from_ram(
    std::span<std::uint8_t> ram,
    std::uint32_t owner_ram_addr,
    std::uint16_t subtype_d0,
    std::int16_t elevation_d1,
    std::uint16_t param_d2) noexcept;

} // namespace thor::combat

namespace oasis::game::combat {
using namespace thor::combat;
} // namespace oasis::game::combat
