#pragma once

#include "game/render/master_sat_dispatcher.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace oasis::game::render {

/// Configuration and constants for Pass B Master SAT Dispatcher (0x00A524..0x00A69A).
struct PassBSatDispatcherConfig {
    static constexpr std::uint32_t kPassBDispatcherAddr = 0x00A524U;
    static constexpr std::uint32_t kPassBDispatcherEnd = 0x00A69AU;
    static constexpr std::uint32_t kSpecialPoolJumpTableTarget = 0x00A69AU;

    // RAM variables
    static constexpr std::uint32_t kCachedMaskRam = 0x00FF188EU;
    static constexpr std::uint32_t kForegroundPlayerFlagRam = 0x00FF1995U;
    static constexpr std::uint32_t kVdpSatBufferRam = 0x00FF13CCU;
    static constexpr std::uint32_t kSatByteOffsetRam = 0x00FF188CU;
    static constexpr std::uint32_t kSatLinkCountRam = 0x00FF188AU;
    static constexpr std::uint32_t kVdpSyncFlagsRam = 0x00FF164EU;
    static constexpr std::uint8_t kVdpSyncMask = 0x03U;

    // Multi-pool definitions
    static constexpr std::uint32_t kGroup1PoolRam = 0x00FF2954U;
    static constexpr std::size_t kGroup1SlotCount = 4U;
    static constexpr std::size_t kGroup1SlotStride = 0x5AU;

    static constexpr std::uint32_t kSpecialPoolRam = 0x00FF19C8U;
    static constexpr std::size_t kSpecialSlotCount = 4U;
    static constexpr std::size_t kSpecialSlotStride = 0x08U;

    static constexpr std::uint32_t kGroup2PoolRam = 0x00FF19E8U;
    static constexpr std::size_t kGroup2SlotCount = 21U;
    static constexpr std::size_t kGroup2SlotStride = 0xBCU;

    static constexpr std::uint32_t kGroup4PoolRam = 0x00FF2ABCU;
    static constexpr std::size_t kGroup4SlotCount = 8U;
    static constexpr std::size_t kGroup4SlotStride = 0x5AU;

    static constexpr std::uint32_t kGroup3PoolRam = 0x00FF2D8CU;
    static constexpr std::size_t kGroup3SlotCount = 6U;
    static constexpr std::size_t kGroup3SlotStride = 0x5AU;

    // Standard camera window viewport bounds
    static constexpr std::int16_t kScreenWidth = 320;
    static constexpr std::int16_t kScreenHeight = 224;
    static constexpr std::int16_t kDefaultCullMargin = 64;
};

/// Camera window bounds filter for frustum/culling optimization.
struct CameraWindowFilter {
    std::int16_t camera_x{0};
    std::int16_t camera_y{0};
    std::int16_t width{PassBSatDispatcherConfig::kScreenWidth};
    std::int16_t height{PassBSatDispatcherConfig::kScreenHeight};
    std::int16_t margin{PassBSatDispatcherConfig::kDefaultCullMargin};

    [[nodiscard]] constexpr bool is_in_window(
        std::int16_t x, std::int16_t y,
        std::int16_t radius_x = 0, std::int16_t radius_y = 0) const noexcept {
        const std::int16_t min_x = static_cast<std::int16_t>(camera_x - margin);
        const std::int16_t max_x = static_cast<std::int16_t>(camera_x + width + margin);
        const std::int16_t min_y = static_cast<std::int16_t>(camera_y - margin);
        const std::int16_t max_y = static_cast<std::int16_t>(camera_y + height + margin);

        if ((x + radius_x) < min_x || (x - radius_x) > max_x) {
            return false;
        }
        if ((y + radius_y) < min_y || (y - radius_y) > max_y) {
            return false;
        }
        return true;
    }
};

/// Updates cached mask to mark off-screen/culled entities as done/inactive.
[[nodiscard]] std::uint32_t filter_cached_mask_by_camera_window(
    std::span<const SatEntitySlot> group2,
    std::uint32_t cached_mask,
    const CameraWindowFilter& camera,
    std::span<const std::int16_t> entity_x_coords = {}) noexcept;

/// Sorts Group 2 entities in descending Y order using a precomputed/cached mask (0x00A550..0x00A5A2).
[[nodiscard]] std::vector<std::size_t> sort_group2_pass_b_order(
    std::span<const SatEntitySlot> group2,
    std::uint32_t cached_mask,
    std::uint32_t* out_done_mask = nullptr);

/// Runs Pass B Master SAT Sprite Dispatcher (0x00A524..0x00A630).
[[nodiscard]] MasterSatDispatchResult dispatch_pass_b_sat_pass(
    std::uint32_t cached_mask,
    bool foreground_player_override,
    std::span<const SatEntitySlot> group1,
    std::span<const SatEntitySlot> special,
    std::span<const SatEntitySlot> group2,
    std::span<const SatEntitySlot> group4,
    std::span<const SatEntitySlot> group3,
    std::uint16_t current_sat_offset = 0U,
    std::uint8_t initial_vdp_sync_flags = 0U);

/// Runs Pass B Master SAT Sprite Dispatcher against raw Mega Drive RAM byte buffer.
[[nodiscard]] MasterSatDispatchResult dispatch_pass_b_sat_raw(
    std::span<std::uint8_t> ram_buffer,
    std::uint32_t ram_base_addr = 0x00FF0000U);

} // namespace oasis::game::render
