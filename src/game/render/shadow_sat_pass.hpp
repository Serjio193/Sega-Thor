#pragma once

#include "game/render/master_sat_dispatcher.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace oasis::game::render {

/// Configuration and constants for Shadow SAT Composition Pass (0x00A6A0..0x00A786).
struct ShadowSatPassConfig {
    static constexpr std::uint32_t kShadowPassAddr = 0x00A6A0U;
    static constexpr std::uint32_t kShadowPassEnd = 0x00A786U;
    static constexpr std::uint32_t kPreCompositionHelperAddr = 0x00A342U;

    // RAM variables
    static constexpr std::uint32_t kVdpSatBufferRam = 0x00FF13CCU;
    static constexpr std::uint32_t kSatByteOffsetRam = 0x00FF188CU;
    static constexpr std::uint32_t kSatLinkCountRam = 0x00FF188AU;

    // Multi-pool definitions
    static constexpr std::uint32_t kGroup1PoolRam = 0x00FF2954U;
    static constexpr std::size_t kGroup1SlotCount = 4U;
    static constexpr std::size_t kGroup1SlotStride = 0x5AU;

    static constexpr std::uint32_t kGroup2PoolRam = 0x00FF19E8U;
    static constexpr std::size_t kGroup2SlotCount = 21U;
    static constexpr std::size_t kGroup2SlotStride = 0xBCU;

    static constexpr std::uint32_t kGroup4PoolRam = 0x00FF2ABCU;
    static constexpr std::size_t kGroup4SlotCount = 8U;
    static constexpr std::size_t kGroup4SlotStride = 0x5AU;

    static constexpr std::uint32_t kGroup3PoolRam = 0x00FF2D8CU;
    static constexpr std::size_t kGroup3SlotCount = 6U;
    static constexpr std::size_t kGroup3SlotStride = 0x5AU;

    // Field offsets within entity slot
    static constexpr std::size_t kTypeOffset = 0x00U;
    static constexpr std::size_t kXOffset = 0x08U;
    static constexpr std::size_t kYOffset = 0x0CU;
    static constexpr std::size_t kZOffset = 0x10U;
    static constexpr std::size_t kGroundHOffset = 0x14U;
    static constexpr std::size_t kFlags36Offset = 0x36U;
    static constexpr std::size_t kFlags3AOffset = 0x3AU;

    // Shadow enable bit: bit 2 of +0x3A (mask 0x04)
    static constexpr std::uint8_t kShadowFlagMask = 0x04U;

    // SAT target entrypoints
    static constexpr std::uint32_t kNormalSatTarget = 0x00AD76U;
    static constexpr std::uint32_t kCustomSatTarget = 0x00A9A0U;

    // Selection sort sentinel in M68K: moveq #-1, d5
    static constexpr std::int16_t kInitialMaxYSentinel = -1;
};

/// Shadow tier based on elevation stacking (air height z).
enum class ShadowScaleTier : std::uint8_t {
    Full = 0,    // z <= 8 (grounded / low hop)
    Medium = 1,  // 8 < z <= 24 (mid jump)
    Small = 2,   // 24 < z <= 48 (high jump / airborne)
    Tiny = 3,    // z > 48 (extreme height)
};

/// High-level entity slot participating in shadow pass and elevation stacking.
struct ShadowEntitySlot {
    std::int16_t type{0};
    std::int16_t x{0};
    std::int16_t y{0};
    std::int16_t z{0};
    std::int16_t ground_h{0};
    std::uint8_t flags_3a{0U};
    std::uint8_t flags_36{0U};

    [[nodiscard]] constexpr bool has_shadow() const noexcept {
        return (type > 0) && ((flags_3a & ShadowSatPassConfig::kShadowFlagMask) != 0U);
    }

    [[nodiscard]] constexpr std::int16_t ground_contact_y() const noexcept {
        return static_cast<std::int16_t>(y + ground_h);
    }

    [[nodiscard]] constexpr std::int16_t sprite_screen_y() const noexcept {
        return static_cast<std::int16_t>(y + ground_h - z);
    }

    [[nodiscard]] constexpr ShadowScaleTier shadow_tier() const noexcept {
        if (z <= 8) {
            return ShadowScaleTier::Full;
        }
        if (z <= 24) {
            return ShadowScaleTier::Medium;
        }
        if (z <= 48) {
            return ShadowScaleTier::Small;
        }
        return ShadowScaleTier::Tiny;
    }
};

/// Emitted shadow dispatch entry.
struct ShadowDispatchEntry {
    SatPoolKind pool{SatPoolKind::Group2Entities};
    std::size_t slot_index{0U};
    std::int16_t type{0};
    std::int16_t y{0};
    std::int16_t ground_y{0};
    ShadowScaleTier tier{ShadowScaleTier::Full};
    SatDispatchTarget target{SatDispatchTarget::NormalSat};
    std::uint32_t ram_addr{0U};
};

/// Result of shadow SAT composition pass.
struct ShadowSatPassResult {
    std::uint32_t shadow_done_mask{0U};
    std::vector<ShadowDispatchEntry> entries;
    bool sat_terminated{false};
    std::uint16_t terminated_sat_offset{0U};
};

/// Builds Group 2 inactive shadow bitmask (0x00A6C6..0x00A6E8).
/// Bit i is set if type <= 0 OR (flags_3a & 0x04) == 0.
[[nodiscard]] std::uint32_t build_shadow_inactive_mask(
    std::span<const ShadowEntitySlot> group2) noexcept;

/// Sorts active Group 2 shadows in descending Y order (0x00A6EA..0x00A738).
[[nodiscard]] std::vector<std::size_t> sort_group2_shadows_y_depth_order(
    std::span<const ShadowEntitySlot> group2,
    std::uint32_t* out_done_mask = nullptr);

/// Runs Shadow SAT Sprite Composition pass (0x00A6A0..0x00A786).
[[nodiscard]] ShadowSatPassResult dispatch_shadow_sat_pass(
    std::span<const ShadowEntitySlot> group1,
    std::span<const ShadowEntitySlot> group2,
    std::span<const ShadowEntitySlot> group4,
    std::span<const ShadowEntitySlot> group3,
    std::uint16_t current_sat_offset = 0U);

/// Runs Shadow SAT Sprite Composition against raw Mega Drive RAM byte buffer.
[[nodiscard]] ShadowSatPassResult dispatch_shadow_sat_raw(
    std::span<std::uint8_t> ram_buffer,
    std::uint32_t ram_base_addr = 0x00FF0000U);

} // namespace oasis::game::render
