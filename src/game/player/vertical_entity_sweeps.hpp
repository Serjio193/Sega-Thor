#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace oasis::collision {

/// Configuration and ROM offsets for dynamic entity vertical sweeps (0x009FA0..0x00A196).
struct VerticalEntitySweepsConfig {
    static constexpr std::uint32_t kGroup2EntityPoolAddr = 0x00FF19E8U;
    static constexpr std::size_t kGroup2EntitySlots = 21U;
    static constexpr std::size_t kEntitySlotStride = 0xBCU; // 188 bytes

    // Field offsets in 188-byte entity slot
    static constexpr std::size_t kTypeOffset = 0x00U;
    static constexpr std::size_t kXOffset = 0x08U;
    static constexpr std::size_t kYOffset = 0x0CU;
    static constexpr std::size_t kZOffset = 0x10U;
    static constexpr std::size_t kGroundHOffset = 0x14U;
    static constexpr std::size_t kFlags37Offset = 0x37U;
    static constexpr std::size_t kFlags38Offset = 0x38U;
    static constexpr std::size_t kFlags3AOffset = 0x3AU;
    static constexpr std::size_t kCollisionFlagsYOffset = 0x41U;
    static constexpr std::size_t kRadXOffset = 0x42U;
    static constexpr std::size_t kRadYOffset = 0x44U;
    static constexpr std::size_t kHeightOffset = 0x4AU;
    static constexpr std::size_t kCollidedPtrOffset = 0x68U;
    static constexpr std::size_t kReactionCodeOffset = 0x6CU;
    static constexpr std::size_t kVelYOffset = 0x76U;

    // Bit flags
    static constexpr std::uint8_t kFlagCannotPushMask = 0x20U;       // +0x37 bit 5
    static constexpr std::uint8_t kFlagCannotBePushedMask = 0x40U;   // +0x37 bit 6
    static constexpr std::uint8_t kFlagIntangibleMask = 0x04U;       // +0x38 bit 2
    static constexpr std::uint8_t kFlagIgnoreGhostMask = 0x02U;      // +0x3A bit 1
    static constexpr std::uint8_t kCollisionFlagEntityHit = 0x80U;   // +0x41 bit 7
    static constexpr std::uint8_t kReactionCodeSolidEntity = 3U;     // +0x6C
};

/// High-level struct representing dynamic entity header fields participating in vertical sweep.
struct DynamicEntityHeaderY {
    std::uint32_t slot_id{0U};
    std::int16_t type{0};
    std::uint8_t flags_37{0U};
    std::uint8_t flags_38{0U};
    std::uint8_t flags_3a{0U};
    std::uint8_t collision_flags_y{0U};
    std::uint8_t reaction_code{0U};
    std::int16_t x{0};
    std::int16_t y{0};
    std::int16_t z{0};
    std::int16_t ground_h{0};
    std::int16_t rad_x{0};
    std::int16_t rad_y{0};
    std::int16_t height{0};
    std::int16_t vel_y{0};
    std::uint32_t collided_entity_id{0U};
};

/// Sweeps positive Y (+Y) against a dynamic entity pool (0x009FA0..0x00A098).
/// Returns clamped displacement step dy.
std::uint16_t sweep_entities_pos_y(
    std::uint16_t dy,
    DynamicEntityHeaderY& self,
    std::span<DynamicEntityHeaderY> pool) noexcept;

/// Sweeps negative Y (-Y) against a dynamic entity pool (0x00A09A..0x00A196).
/// Returns clamped displacement step dy.
std::uint16_t sweep_entities_neg_y(
    std::uint16_t dy,
    DynamicEntityHeaderY& self,
    std::span<DynamicEntityHeaderY> pool) noexcept;

/// Sweeps positive Y (+Y) using raw entity slot byte buffers (0x009FA0..0x00A098).
std::uint16_t sweep_slot_entities_pos_y(
    std::uint16_t dy,
    std::span<std::uint8_t> self_slot,
    std::span<std::uint8_t> pool_bytes,
    std::size_t slot_count = VerticalEntitySweepsConfig::kGroup2EntitySlots,
    std::size_t slot_stride = VerticalEntitySweepsConfig::kEntitySlotStride,
    std::uint32_t pool_base_addr = VerticalEntitySweepsConfig::kGroup2EntityPoolAddr) noexcept;

/// Sweeps negative Y (-Y) using raw entity slot byte buffers (0x00A09A..0x00A196).
std::uint16_t sweep_slot_entities_neg_y(
    std::uint16_t dy,
    std::span<std::uint8_t> self_slot,
    std::span<std::uint8_t> pool_bytes,
    std::size_t slot_count = VerticalEntitySweepsConfig::kGroup2EntitySlots,
    std::size_t slot_stride = VerticalEntitySweepsConfig::kEntitySlotStride,
    std::uint32_t pool_base_addr = VerticalEntitySweepsConfig::kGroup2EntityPoolAddr) noexcept;

} // namespace oasis::collision
