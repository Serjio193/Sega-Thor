#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace thor::physics {

// ROM entrypoints and boundaries (0x00DDA2..0x00DEEC, 330 bytes).
inline constexpr std::uint32_t kAabbForceFieldsStartAddr = 0x00DDA2U;
inline constexpr std::uint32_t kApplyBoxForceVectorAddr = 0x00DDA2U;
inline constexpr std::uint32_t kApplyBoxForceVectorEndAddr = 0x00DE00U;
inline constexpr std::size_t kApplyBoxForceVectorSizeBytes = 94U;

inline constexpr std::uint32_t kApplyBoxForceVectorElevationBoostAddr = 0x00DE00U;
inline constexpr std::uint32_t kApplyBoxForceVectorElevationBoostEndAddr = 0x00DE6CU;
inline constexpr std::size_t kApplyBoxForceVectorElevationBoostSizeBytes = 108U;

inline constexpr std::uint32_t kApplyConveyorCurrentForceAddr = 0x00DE6CU;
inline constexpr std::uint32_t kApplyConveyorCurrentForceEndAddr = 0x00DEECU;
inline constexpr std::size_t kApplyConveyorCurrentForceSizeBytes = 128U;

inline constexpr std::uint32_t kAabbForceFieldsEndAddr = 0x00DEECU;
inline constexpr std::size_t kAabbForceFieldsSizeBytes = 330U;

// Master entity pool geometry in 64KB Work RAM ($FF19E8, moveq #$14 + dbra -> 21 slots, stride $BC).
inline constexpr std::uint32_t kEntityPoolBaseRamAddr = 0x00FF19E8U;
inline constexpr std::size_t kEntityPoolBaseRamOffset = 0x19E8U;
inline constexpr std::size_t kEntityPoolSlotCount = 21U;
inline constexpr std::size_t kEntitySlotStrideBytes = 0xBCU; // 188 bytes
inline constexpr std::size_t kEntityPoolByteSize = kEntityPoolSlotCount * kEntitySlotStrideBytes;

// Field offsets within each 0xBC-byte entity slot.
// +$08, +$0C, +$10 are 16.16 fixed-point coordinates; M68K word reads access the high integer word.
inline constexpr std::size_t kEntityStatusOffset = 0x00U;    // s16 (> 0 active)
inline constexpr std::size_t kEntityPosXOffset = 0x08U;      // s16 integer X (high word of +$08.l)
inline constexpr std::size_t kEntityPosYOffset = 0x0CU;      // s16 integer Y (high word of +$0C.l)
inline constexpr std::size_t kEntityPosZFpOffset = 0x10U;    // s32 16.16 Z (+$10.w integer Z, +$10.l != 0 boost)
inline constexpr std::size_t kEntityFlags3AOffset = 0x3AU;   // u8 trait / immunity flags
inline constexpr std::size_t kEntityAccumDxFpOffset = 0x72U; // s32 16.16 X force accumulator
inline constexpr std::size_t kEntityAccumDyFpOffset = 0x76U; // s32 16.16 Y force accumulator

// Immunity bitmasks checked at +$3A(a0) by 0x00DE6C.
inline constexpr std::uint8_t kEntityImmuneBit0Mask = 0x01U; // btst.b #0, $3a(a0)
inline constexpr std::uint8_t kEntityImmuneBit3Mask = 0x08U; // btst.b #3, $3a(a0)
inline constexpr std::uint8_t kEntityConveyorImmuneMask =
    static_cast<std::uint8_t>(kEntityImmuneBit0Mask | kEntityImmuneBit3Mask);

inline constexpr std::int32_t kFixedPointOne = 0x00010000;

/// Selects which of the three 0x00DDA2..0x00DEEC entrypoints to execute.
enum class ForceFieldMode : std::uint8_t {
    Standard = 0U,        // 0x00DDA2: 1x force inside 3D AABB
    ElevationBoost = 1U,  // 0x00DE00: 2x force when 32-bit pos_z_fp != 0
    ConveyorCurrent = 2U, // 0x00DE6C: +$3A bit0/bit3 immunity + 2x force when pos_z_fp != 0
};

/// M68K packed register representation for the 3D AABB bounds (d3, d4, d5).
struct PackedBoxRegisters {
    std::uint32_t d3{0U}; // (min_x << 16) | min_y
    std::uint32_t d4{0U}; // (max_x << 16) | max_y
    std::uint32_t d5{0U}; // (min_z << 16) | max_z

    [[nodiscard]] constexpr bool operator==(const PackedBoxRegisters&) const noexcept = default;
};

/// Signed 16-bit inclusive 3D Axis-Aligned Bounding Box (all bounds [min, max]).
struct BoundingBox3D {
    std::int16_t min_x{0};
    std::int16_t max_x{0};
    std::int16_t min_y{0};
    std::int16_t max_y{0};
    std::int16_t min_z{0};
    std::int16_t max_z{0};

    /// Evaluates inclusive signed 16-bit containment in M68K test order: Y, X, Z.
    [[nodiscard]] constexpr bool contains(
        std::int16_t x,
        std::int16_t y,
        std::int16_t z) const noexcept {
        return y >= min_y && y <= max_y &&
               x >= min_x && x <= max_x &&
               z <= max_z && z >= min_z;
    }

    /// Unpacks M68K d3/d4/d5 register bounds:
    /// d3 = (min_x << 16) | min_y, d4 = (max_x << 16) | max_y, d5 = (min_z << 16) | max_z.
    [[nodiscard]] static constexpr BoundingBox3D from_registers(
        std::uint32_t reg_d3,
        std::uint32_t reg_d4,
        std::uint32_t reg_d5) noexcept {
        return BoundingBox3D{
            .min_x = static_cast<std::int16_t>((reg_d3 >> 16U) & 0xFFFFU),
            .max_x = static_cast<std::int16_t>((reg_d4 >> 16U) & 0xFFFFU),
            .min_y = static_cast<std::int16_t>(reg_d3 & 0xFFFFU),
            .max_y = static_cast<std::int16_t>(reg_d4 & 0xFFFFU),
            .min_z = static_cast<std::int16_t>((reg_d5 >> 16U) & 0xFFFFU),
            .max_z = static_cast<std::int16_t>(reg_d5 & 0xFFFFU),
        };
    }

    /// Packs bounds back into M68K d3/d4/d5 register layout.
    [[nodiscard]] constexpr PackedBoxRegisters to_registers() const noexcept {
        return PackedBoxRegisters{
            .d3 = (static_cast<std::uint32_t>(static_cast<std::uint16_t>(min_x)) << 16U) |
                  static_cast<std::uint32_t>(static_cast<std::uint16_t>(min_y)),
            .d4 = (static_cast<std::uint32_t>(static_cast<std::uint16_t>(max_x)) << 16U) |
                  static_cast<std::uint32_t>(static_cast<std::uint16_t>(max_y)),
            .d5 = (static_cast<std::uint32_t>(static_cast<std::uint16_t>(min_z)) << 16U) |
                  static_cast<std::uint32_t>(static_cast<std::uint16_t>(max_z)),
        };
    }

    [[nodiscard]] constexpr bool operator==(const BoundingBox3D&) const noexcept = default;
};

/// 2D 16.16 fixed-point force/displacement delta passed in M68K d1/d2 and accumulated into +$72/+$76.
struct ForceVector2D {
    std::int32_t force_x_fp{0}; // d1 -> +$72(a0)
    std::int32_t force_y_fp{0}; // d2 -> +$76(a0)

    [[nodiscard]] constexpr bool operator==(const ForceVector2D&) const noexcept = default;
};

/// Extracts signed 16-bit integer component (big-endian M68K high word) from a 16.16 fixed-point value.
[[nodiscard]] constexpr std::int16_t unpack_fixed_integer(std::int32_t value_fp) noexcept {
    return static_cast<std::int16_t>(static_cast<std::uint32_t>(value_fp) >> 16U);
}

/// Packs a signed 16-bit integer component and optional 16-bit subpixel fraction into 16.16 fixed-point.
[[nodiscard]] constexpr std::int32_t pack_fixed_16_16(
    std::int16_t integer_part,
    std::uint16_t frac_part = 0U) noexcept {
    return static_cast<std::int32_t>(
        (static_cast<std::uint32_t>(static_cast<std::uint16_t>(integer_part)) << 16U) |
        static_cast<std::uint32_t>(frac_part));
}

/// Entity slot projection for the 0x00DDA2..0x00DEEC 3D AABB force field subsystem.
struct ForceEntitySlot {
    std::int16_t status{0};       // +0x00.w (> 0 active; <= 0 skipped)
    std::int16_t pos_x{0};        // +0x08.w (integer world X = high word of 16.16 x_fp)
    std::int16_t pos_y{0};        // +0x0C.w (integer world Y = high word of 16.16 y_fp)
    std::int32_t pos_z_fp{0};     // +0x10.l (16.16 fixed-point Z; high word for AABB, != 0 for 2x boost)
    std::uint8_t flags_3a{0U};    // +0x3A.b (bits 0 and 3 exempt entity in 0x00DE6C)
    std::int32_t accum_dx_fp{0};  // +0x72.l (16.16 fixed-point X displacement accumulator)
    std::int32_t accum_dy_fp{0};  // +0x76.l (16.16 fixed-point Y displacement accumulator)

    /// Returns integer elevation word (+0x10.w) used by the Z AABB bounds check.
    [[nodiscard]] constexpr std::int16_t integer_z() const noexcept {
        return unpack_fixed_integer(pos_z_fp);
    }

    [[nodiscard]] constexpr bool operator==(const ForceEntitySlot&) const noexcept = default;
};

/// Active entity guard (`tst.w $0(a0); ble`).
[[nodiscard]] constexpr bool is_entity_active_for_force_field(std::int16_t status) noexcept {
    return status > 0;
}

/// Conveyor/current immunity predicate (`btst #0, $3a(a0)` or `btst #3, $3a(a0)`).
[[nodiscard]] constexpr bool is_conveyor_current_immune(std::uint8_t flags_3a) noexcept {
    return (flags_3a & kEntityConveyorImmuneMask) != 0U;
}

/// Airborne elevation boost predicate (`tst.l $10(a0); beq`): true when any of the 32 bits of pos_z_fp != 0.
[[nodiscard]] constexpr bool has_elevation_boost(std::int32_t pos_z_fp) noexcept {
    return pos_z_fp != 0;
}

/// Unified dispatcher across up to 21 entity slots matching 0x00DDA2 / 0x00DE00 / 0x00DE6C.
/// Returns the number of entity slots modified.
std::size_t apply_force_field(
    std::span<ForceEntitySlot> entities,
    const BoundingBox3D& box,
    std::int32_t force_x_fp,
    std::int32_t force_y_fp,
    ForceFieldMode mode) noexcept;

/// 0x00DDA2..0x00DE00: Standard 3D AABB force vector application (water flumes, ramps, push fields).
std::size_t apply_box_force_vector(
    std::span<ForceEntitySlot> entities,
    const BoundingBox3D& box,
    std::int32_t force_x_fp,
    std::int32_t force_y_fp) noexcept;

/// 0x00DE00..0x00DE6C: 3D AABB force vector with 2x boost when 32-bit pos_z_fp != 0.
std::size_t apply_box_force_vector_with_elevation_boost(
    std::span<ForceEntitySlot> entities,
    const BoundingBox3D& box,
    std::int32_t force_x_fp,
    std::int32_t force_y_fp) noexcept;

/// 0x00DE6C..0x00DEEC: Conveyor/whirlpool force with +$3A bit0/bit3 immunity and 2x pos_z_fp != 0 boost.
std::size_t apply_conveyor_current_force(
    std::span<ForceEntitySlot> entities,
    const BoundingBox3D& box,
    std::int32_t force_x_fp,
    std::int32_t force_y_fp) noexcept;

/// ForceVector2D overloads forwarding to the primary entrypoints.
inline std::size_t apply_box_force_vector(
    std::span<ForceEntitySlot> entities,
    const BoundingBox3D& box,
    ForceVector2D force) noexcept {
    return apply_box_force_vector(entities, box, force.force_x_fp, force.force_y_fp);
}

inline std::size_t apply_box_force_vector_with_elevation_boost(
    std::span<ForceEntitySlot> entities,
    const BoundingBox3D& box,
    ForceVector2D force) noexcept {
    return apply_box_force_vector_with_elevation_boost(entities, box, force.force_x_fp, force.force_y_fp);
}

inline std::size_t apply_conveyor_current_force(
    std::span<ForceEntitySlot> entities,
    const BoundingBox3D& box,
    ForceVector2D force) noexcept {
    return apply_conveyor_current_force(entities, box, force.force_x_fp, force.force_y_fp);
}

// ============================================================================
// 64KB Work RAM Window Helpers ($FF19E8, 21 slots x 0xBC bytes)
// ============================================================================

/// Loads ForceEntitySlot fields from the 64KB Work RAM window at slot_index (0..20).
[[nodiscard]] ForceEntitySlot load_force_entity_slot_from_ram(
    std::span<const std::uint8_t> ram,
    std::size_t slot_index) noexcept;

/// Writes ForceEntitySlot fields into the 64KB Work RAM window at slot_index (0..20).
void store_force_entity_slot_to_ram(
    std::span<std::uint8_t> ram,
    std::size_t slot_index,
    const ForceEntitySlot& slot) noexcept;

/// Executes a force field pass directly over $FF19E8..$FF2953 in the 64KB Work RAM window,
/// mutating only +$72 and +$76 on matching slots.
std::size_t apply_force_field_from_ram(
    std::span<std::uint8_t> ram,
    const BoundingBox3D& box,
    std::int32_t force_x_fp,
    std::int32_t force_y_fp,
    ForceFieldMode mode) noexcept;

std::size_t apply_box_force_vector_from_ram(
    std::span<std::uint8_t> ram,
    const BoundingBox3D& box,
    std::int32_t force_x_fp,
    std::int32_t force_y_fp) noexcept;

std::size_t apply_box_force_vector_with_elevation_boost_from_ram(
    std::span<std::uint8_t> ram,
    const BoundingBox3D& box,
    std::int32_t force_x_fp,
    std::int32_t force_y_fp) noexcept;

std::size_t apply_conveyor_current_force_from_ram(
    std::span<std::uint8_t> ram,
    const BoundingBox3D& box,
    std::int32_t force_x_fp,
    std::int32_t force_y_fp) noexcept;

/// Executes a force field pass directly from M68K register inputs (d1..d5) over the 64KB RAM window.
std::size_t apply_force_field_packed_from_ram(
    std::span<std::uint8_t> ram,
    std::int32_t d1_force_x_fp,
    std::int32_t d2_force_y_fp,
    std::uint32_t d3_min_xy,
    std::uint32_t d4_max_xy,
    std::uint32_t d5_min_max_z,
    ForceFieldMode mode) noexcept;

} // namespace thor::physics

namespace oasis::game::physics {
using namespace thor::physics;
} // namespace oasis::game::physics
