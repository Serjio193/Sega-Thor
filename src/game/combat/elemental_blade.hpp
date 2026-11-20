#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace thor::combat {

// ROM entrypoints, inline tables, and range boundaries (0x00F258..0x00F396, 318 bytes).
inline constexpr std::uint32_t kElementalBladeStartAddr = 0x00F258U;
inline constexpr std::uint32_t kElementalBladeGroundEntryAddr = 0x00F258U;
inline constexpr std::size_t kElementalBladeGroundEntrySizeBytes = 10U;
inline constexpr std::uint32_t kElementalBladeGroundOffsetsAddr = 0x00F262U;
inline constexpr std::size_t kElementalBladeGroundOffsetsSizeBytes = 8U;

inline constexpr std::uint32_t kElementalBladeWaterEntryAddr = 0x00F26AU;
inline constexpr std::size_t kElementalBladeWaterEntrySizeBytes = 10U;
inline constexpr std::uint32_t kElementalBladeWaterOffsetsAddr = 0x00F274U;
inline constexpr std::size_t kElementalBladeWaterOffsetsSizeBytes = 8U;

inline constexpr std::uint32_t kElementalBladeSharedInitAddr = 0x00F27CU;
inline constexpr std::size_t kElementalBladeSharedInitSizeBytes = 282U;
inline constexpr std::uint32_t kElementalBladeEndAddr = 0x00F396U;
inline constexpr std::size_t kElementalBladeSizeBytes = 318U;

// Observed ROM callers and helper callees.
inline constexpr std::uint32_t kElementalBladeGroundCallerAddr = 0x0072C4U;
inline constexpr std::uint32_t kElementalBladeWaterCallerAddr = 0x007348U;
inline constexpr std::uint32_t kElementalBladeDurabilityHelperAddr = 0x008530U;
inline constexpr std::uint32_t kElementalBladeConstructorAddr = 0x008D06U;
inline constexpr std::uint32_t kElementalBladeTerrainProbeAddr = 0x009D00U;
inline constexpr std::uint32_t kElementalBladeVelocityProjectorAddr = 0x00E2D4U;

// Projectile slot & RAM flags.
inline constexpr std::uint32_t kSwordWaveSlotRamAddr = 0x00FF1B60U;
inline constexpr std::size_t kSwordWaveSlotRamOffset = 0x1B60U;
inline constexpr std::size_t kSwordWaveSlotStrideBytes = 0xBCU; // 188 bytes
inline constexpr std::uint32_t kSwordWavePauseSlowmoRamAddr = 0x00FF17B8U;
inline constexpr std::size_t kSwordWavePauseSlowmoRamOffset = 0x17B8U;
inline constexpr std::uint8_t kSwordWaveSlowmoDoubleBitMask = 0x01U;

// Projectile slot constants initialized by 0x00F27C..0x00F396.
inline constexpr std::uint16_t kSwordWaveTypeId = 0x0034U;
inline constexpr std::uint16_t kSwordWaveSpritePriority = 0x055EU;
inline constexpr std::uint16_t kSwordWaveHitboxRadius = 6U;
inline constexpr std::uint8_t kSwordWaveHitboxRx = 6U;
inline constexpr std::uint8_t kSwordWaveHitboxRy = 6U;
inline constexpr std::uint16_t kSwordWaveHitboxHeight = 8U;
inline constexpr std::uint8_t kSwordWaveFlags38 = 0x06U;
inline constexpr std::uint8_t kSwordWaveFlags3D = 0x00U;
inline constexpr std::uint8_t kSwordWaveFlag3CBit1Mask = 0x02U;
inline constexpr std::int32_t kSwordWaveFloatVelZFp = 0x0000C000;
inline constexpr std::uint8_t kSwordWaveLifetimeFrames = 0xF0U; // 240 frames
inline constexpr std::uint8_t kSwordWaveOmegaPierceByte = 0xFFU;
inline constexpr std::int16_t kSwordWaveGroundElevDefault = 28; // 0x001C from 0x0072A6..0x0072BE
inline constexpr std::int16_t kSwordWaveWaterElevDefault = 13;  // 0x000D from 0x00732A..0x007342

// Sword type parameter bits in d0 (bits 31, 30, 29).
inline constexpr std::uint32_t kSwordWaveMaskFire = 0x80000000U;
inline constexpr std::uint32_t kSwordWaveMaskOmega = 0x40000000U;
inline constexpr std::uint32_t kSwordWaveMaskHyper = 0x20000000U;

// Damage, animation ID, and 8.8 X/Y speed parameters per sword type.
inline constexpr std::uint16_t kSwordWaveDamageStandard = 0x0003U;
inline constexpr std::uint16_t kSwordWaveDamageFire = 0x2004U;
inline constexpr std::uint16_t kSwordWaveDamageOmega = 0x8005U;
inline constexpr std::uint16_t kSwordWaveDamageHyper = 0x0000U;

inline constexpr std::uint16_t kSwordWaveAnimStandard = 0x0002U;
inline constexpr std::uint16_t kSwordWaveAnimElemental = 0x0004U;

inline constexpr std::uint16_t kSwordWaveSpeedXStandard = 0x0C00U;
inline constexpr std::uint16_t kSwordWaveSpeedYStandard = 0x0BC0U;
inline constexpr std::uint16_t kSwordWaveSpeedXOmega = 0x1000U;
inline constexpr std::uint16_t kSwordWaveSpeedYOmega = 0x0F00U;
inline constexpr std::uint16_t kSwordWaveSpeedXHyper = 0x0E00U;
inline constexpr std::uint16_t kSwordWaveSpeedYHyper = 0x0D00U;

/// Packed signed byte pair `(dx, dy)` from 0x00F262 (ground) or 0x00F274 (water).
struct SwordTipOffset {
    std::int8_t dx{0};
    std::int8_t dy{0};

    [[nodiscard]] constexpr bool operator==(const SwordTipOffset&) const noexcept = default;
};

// Raw 16-bit big-endian table words verified from ROM 0x00F262..0x00F26A and 0x00F274..0x00F27C.
inline constexpr std::array<std::uint16_t, 4> kGroundSwordTipWords = {
    0x01D8U, 0x10E6U, 0xFD00U, 0xF0E6U
};
inline constexpr std::array<std::uint16_t, 4> kWaterSwordTipWords = {
    0x03E0U, 0x14EFU, 0xFD04U, 0xECEFU
};

inline constexpr std::array<SwordTipOffset, 4> kGroundSwordTipOffsets = {{
    {.dx = 1, .dy = -40},   // 0: Up
    {.dx = 16, .dy = -26},  // 1: Right
    {.dx = -3, .dy = 0},    // 2: Down
    {.dx = -16, .dy = -26}, // 3: Left
}};

inline constexpr std::array<SwordTipOffset, 4> kWaterSwordTipOffsets = {{
    {.dx = 3, .dy = -32},   // 0: Up
    {.dx = 20, .dy = -17},  // 1: Right
    {.dx = -3, .dy = 4},    // 2: Down
    {.dx = -20, .dy = -17}, // 3: Left
}};

/// Sword wave variant bits in high bits of d0 (bit 31 Fire, bit 30 Omega, bit 29 Hyper).
enum class SwordWaveType : std::uint32_t {
    Standard = 0U,
    Fire = kSwordWaveMaskFire,
    Omega = kSwordWaveMaskOmega,
    Hyper = kSwordWaveMaskHyper,
};

/// Entrypoint surface selecting ground (0x00F258 / 0x00F262) vs water (0x00F26A / 0x00F274) tip table.
enum class SwordWaveSurface : std::uint8_t {
    Ground = 0U,
    Water = 1U,
};

/// Decoded sword wave combat & speed configuration from 0x00F2C2..0x00F330.
struct SwordWaveConfig {
    SwordWaveType type{SwordWaveType::Standard};
    std::uint32_t param_bits{0U};
    std::int16_t elevation{0};
    std::uint16_t damage{kSwordWaveDamageStandard};
    std::uint16_t anim_id{kSwordWaveAnimStandard};
    bool piercing{false};
    bool flag_3c_bit1{false};
    std::uint16_t speed_x{kSwordWaveSpeedXStandard}; // 8.8 X speed passed to 0x00E2D4 (d1)
    std::uint16_t speed_y{kSwordWaveSpeedYStandard}; // 8.8 Y speed passed to 0x00E2D4 (d2)

    [[nodiscard]] constexpr bool operator==(const SwordWaveConfig&) const noexcept = default;
};

/// Initial 16.16 fixed-point sword-tip displacement deltas written to +$72 / +$76.
struct SwordWaveAnchor {
    std::int32_t anchor_dx_fp{0};
    std::int32_t anchor_dy_fp{0};

    [[nodiscard]] constexpr bool operator==(const SwordWaveAnchor&) const noexcept = default;
};

/// Owner entity fields copied from Ali (a4) into projectile slot $FF1B60 (a6).
struct ElementalBladeOwnerState {
    std::uint32_t owner_ram_addr{0x00FF19E8U};
    std::int32_t pos_x_fp{0}; // +$08.l
    std::int32_t pos_y_fp{0}; // +$0C.l
    std::uint16_t facing{0U}; // +$16.w (0..3)

    [[nodiscard]] constexpr bool operator==(const ElementalBladeOwnerState&) const noexcept = default;
};

/// Complete projection of projectile slot $FF1B60 initialized by 0x00F27C..0x00F396.
struct SwordWaveSlotState {
    std::uint16_t type_id{kSwordWaveTypeId};                 // +$00.w
    std::int32_t pos_x_fp{0};                                // +$08.l
    std::int32_t pos_y_fp{0};                                // +$0C.l
    std::int32_t pos_z_fp{0};                                // +$10.l (elevation << 16)
    std::uint16_t facing{0U};                                // +$16.w
    std::uint16_t sprite_priority_18{kSwordWaveSpritePriority}; // +$18.w
    std::uint16_t anim_id_2a{kSwordWaveAnimStandard};        // +$2A.w
    std::uint8_t flags_38{kSwordWaveFlags38};                // +$38.b
    std::uint8_t flags_3c{0U};                               // +$3C.b (bit 1 set for Fire/Hyper)
    std::uint8_t flags_3d{kSwordWaveFlags3D};                // +$3D.b
    std::uint16_t hitbox_radius_46{kSwordWaveHitboxRadius};  // +$46.w
    std::uint8_t hitbox_rx_48{kSwordWaveHitboxRx};           // +$48.b
    std::uint8_t hitbox_ry_49{kSwordWaveHitboxRy};           // +$49.b
    std::uint16_t hitbox_height_4a{kSwordWaveHitboxHeight};  // +$4A.w
    std::int32_t vel_x_fp{0};                                // +$4E.l
    std::int32_t vel_y_fp{0};                                // +$52.l
    std::int32_t vel_z_fp{kSwordWaveFloatVelZFp};            // +$56.l
    std::int32_t anchor_dx_fp{0};                            // +$72.l
    std::int32_t anchor_dy_fp{0};                            // +$76.l
    std::uint8_t lifetime_a6{kSwordWaveLifetimeFrames};      // +$A6.b
    std::uint8_t pierce_a7{0U};                              // +$A7.b (0xFF for Omega)
    std::uint16_t damage_a8{kSwordWaveDamageStandard};       // +$A8.w
    std::uint32_t owner_ptr_aa{0U};                          // +$AA.l

    [[nodiscard]] constexpr bool operator==(const SwordWaveSlotState&) const noexcept = default;
};

/// Unpacks a big-endian sword-tip word matching `asr.w #8, d1` and `move.b d1, d2; ext.w d2`.
[[nodiscard]] constexpr SwordTipOffset unpack_sword_tip_word(std::uint16_t packed_word) noexcept {
    return SwordTipOffset{
        .dx = static_cast<std::int8_t>((packed_word >> 8U) & 0xFFU),
        .dy = static_cast<std::int8_t>(packed_word & 0xFFU),
    };
}

/// Looks up ground or water sword-tip `(dx, dy)` offset for `facing & 3`.
[[nodiscard]] constexpr SwordTipOffset get_sword_tip_offset(
    SwordWaveSurface surface,
    std::uint16_t facing) noexcept {
    const std::size_t idx = static_cast<std::size_t>(facing & 3U);
    return (surface == SwordWaveSurface::Water)
        ? kWaterSwordTipOffsets[idx]
        : kGroundSwordTipOffsets[idx];
}

/// Decodes raw 32-bit M68K d0 parameter word matching 0x00F2C2 and 0x00F2CA..0x00F330.
[[nodiscard]] SwordWaveConfig decode_sword_wave_parameters(std::uint32_t param_d0) noexcept;

/// Convenience overload for single SwordWaveType enum values.
[[nodiscard]] SwordWaveConfig get_sword_wave_config(
    SwordWaveType type,
    std::int16_t elevation = 0) noexcept;

/// Computes initial 16.16 tip anchor deltas (+$72, +$76) matching 0x00F34E..0x00F37C.
[[nodiscard]] SwordWaveAnchor compute_sword_wave_anchor(
    SwordWaveSurface surface,
    std::uint16_t facing,
    std::int16_t elevation,
    bool slowmo_double_bit0 = false) noexcept;

/// Executes full Elemental Blade spawner logic (0x00F258..0x00F396) in C++ struct space.
[[nodiscard]] SwordWaveSlotState spawn_elemental_blade(
    SwordWaveSurface surface,
    const ElementalBladeOwnerState& owner,
    std::uint32_t param_d0,
    bool slowmo_double_bit0 = false,
    std::uint8_t base_flags_3c = 0U) noexcept;

// ============================================================================
// 64KB Work RAM Window Helpers ($FF1B60 projectile slot & $FF17B8 bit 0)
// ============================================================================

[[nodiscard]] SwordWaveSlotState load_sword_wave_slot_from_ram(
    std::span<const std::uint8_t> ram,
    std::uint32_t slot_ram_addr = kSwordWaveSlotRamAddr) noexcept;

void store_sword_wave_slot_to_ram(
    std::span<std::uint8_t> ram,
    const SwordWaveSlotState& state,
    std::uint32_t slot_ram_addr = kSwordWaveSlotRamAddr) noexcept;

[[nodiscard]] SwordWaveSlotState spawn_elemental_blade_from_ram(
    std::span<std::uint8_t> ram,
    SwordWaveSurface surface,
    std::uint32_t owner_ram_addr,
    std::uint32_t param_d0,
    std::uint8_t initial_flags_3c = 0U) noexcept;

[[nodiscard]] SwordWaveSlotState spawn_elemental_blade_ground_from_ram(
    std::span<std::uint8_t> ram,
    std::uint32_t owner_ram_addr,
    std::uint32_t param_d0,
    std::uint8_t initial_flags_3c = 0U) noexcept;

[[nodiscard]] SwordWaveSlotState spawn_elemental_blade_water_from_ram(
    std::span<std::uint8_t> ram,
    std::uint32_t owner_ram_addr,
    std::uint32_t param_d0,
    std::uint8_t initial_flags_3c = 0U) noexcept;

} // namespace thor::combat

namespace oasis::game::combat {
using namespace thor::combat;
} // namespace oasis::game::combat
