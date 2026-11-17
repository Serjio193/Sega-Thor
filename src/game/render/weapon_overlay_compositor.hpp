#pragma once

#include "game/render/master_sat_compositor.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace oasis::game::render {

/// Configuration and constants for Weapon & Auxiliary Overlay SAT Compositor (0x00B15E..0x00B34C).
struct WeaponOverlayCompositorConfig {
    static constexpr std::uint32_t kCompositorEntryAddr = 0x00B15EU;
    static constexpr std::uint32_t kDmaPathEntryAddr = 0x00B166U;
    static constexpr std::uint32_t kDescriptorTableAddr = 0x0003F2FAU;
    static constexpr std::uint32_t kDmaPathExitRtsAddr = 0x00B28CU;
    static constexpr std::uint32_t kNonDmaPathEntryAddr = 0x00B28EU;
    static constexpr std::uint32_t kNonDmaPathExitRtsAddr = 0x00B34AU;
    static constexpr std::uint32_t kCompositorRangeEnd = 0x00B34CU;

    // RAM variables
    static constexpr std::uint32_t kVdpSatBufferRam = 0x00FF13CCU;
    static constexpr std::uint32_t kSatLinkCountRam = 0x00FF188AU;
    static constexpr std::uint32_t kSatByteOffsetRam = 0x00FF188CU;
    static constexpr std::uint32_t kDmaQueueRam = 0x00FF1892U;
    static constexpr std::uint32_t kCameraPlane1Ram = 0x00FF1716U;
    static constexpr std::uint32_t kCameraPlane2Ram = 0x00FF173EU;

    // Entity Slot Field Offsets
    static constexpr std::size_t kTypeOffset = 0x00U;
    static constexpr std::size_t kWorldXOffset = 0x08U;
    static constexpr std::size_t kWorldYOffset = 0x0CU;
    static constexpr std::size_t kElevZOffset = 0x10U;
    static constexpr std::size_t kModeWordOffset = 0x3EU;
    static constexpr std::size_t kAuxVramBaseOffset = 0x5AU;
    static constexpr std::size_t kWeaponFrameCacheOffset = 0x5CU;

    // Mode bit 9 mask (0x0200): 0 = DMA path, 1 = Non-DMA path
    static constexpr std::uint16_t kNonDmaModeMask = 0x0200U;

    // Viewport Clipping Bounds
    static constexpr std::int16_t kViewportMinY = 0x60;  // 96
    static constexpr std::int16_t kViewportMaxY = 0x160; // 352
    static constexpr std::int16_t kViewportMinX = 0x60;  // 96
    static constexpr std::int16_t kViewportMaxX = 0x1C0; // 448
    static constexpr std::uint16_t kViewportClipHeight = 0x100U; // 256
    static constexpr std::uint16_t kViewportClipWidth = 0x160U;  // 352
    static constexpr std::int16_t kCoordBias = 128;      // 0x80

    static constexpr std::uint16_t kMaxSatSprites = 80U;
};

/// Overlay piece descriptor for equipped weapon or auxiliary attachment.
struct WeaponOverlayPiece {
    std::int16_t y_offset{0};
    std::uint8_t size{0U};
    std::uint16_t tile_attr{0U};
    std::int16_t x_offset{0};
};

/// Execution path selector for weapon overlay composition.
enum class WeaponOverlayPath : std::uint8_t {
    DmaStream = 0,
    NonDmaStatic = 1,
};

/// Result of evaluating weapon overlay composition.
struct WeaponOverlayResult {
    WeaponOverlayPath path{WeaponOverlayPath::DmaStream};
    bool dma_queued{false};
    std::int16_t updated_frame_cache{0};
    std::int16_t screen_x{0};
    std::int16_t screen_y{0};
    std::vector<CompositorSatEntry> emitted_pieces;
    std::uint16_t final_sprite_count{0U};
    std::uint16_t final_sat_offset{0U};
};

/// High-level entity weapon overlay state.
struct WeaponOverlayEntityState {
    std::int16_t type{0};
    std::int16_t world_x{0};
    std::int16_t world_y{0};
    std::int16_t elev_z{0};
    std::uint16_t mode_word{0U};
    std::uint16_t aux_vram_base{0U};
    std::int16_t weapon_frame{0};
    std::int16_t weapon_frame_cache{-1};
};

/// Returns whether overlay requires DMA streaming (bit 9 of mode word == 0).
[[nodiscard]] constexpr bool requires_weapon_dma(std::uint16_t mode_word) noexcept {
    return (mode_word & WeaponOverlayCompositorConfig::kNonDmaModeMask) == 0U;
}

/// Resolves entity descriptor table offset: 0x0003F2FA + (entity_type * 16) - 20 (0x00B1AA).
[[nodiscard]] constexpr std::uint32_t get_weapon_descriptor_offset(std::uint16_t entity_type) noexcept {
    return WeaponOverlayCompositorConfig::kDescriptorTableAddr +
           (static_cast<std::uint32_t>(entity_type) * 16U) - 20U;
}

/// Viewport clipping check for weapon overlay pieces.
[[nodiscard]] constexpr bool is_weapon_overlay_clipped(std::int16_t screen_y, std::int16_t screen_x) noexcept {
    const auto y_rel = static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(screen_y) - WeaponOverlayCompositorConfig::kViewportMinY);
    const auto x_rel = static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(screen_x) - WeaponOverlayCompositorConfig::kViewportMinX);
    return (y_rel > WeaponOverlayCompositorConfig::kViewportClipHeight) ||
           (x_rel > WeaponOverlayCompositorConfig::kViewportClipWidth);
}

/// Executes Weapon & Auxiliary Overlay SAT Composition (0x00B15E..0x00B34C).
[[nodiscard]] WeaponOverlayResult compose_weapon_overlay(
    WeaponOverlayEntityState& entity,
    std::span<const WeaponOverlayPiece> pieces,
    std::int16_t cam_x,
    std::int16_t cam_y,
    std::uint16_t current_sprites = 0U,
    std::uint16_t current_sat_offset = 0U);

/// Executes Weapon & Auxiliary Overlay SAT Composition against raw 64KB Mega Drive RAM.
[[nodiscard]] WeaponOverlayResult compose_weapon_overlay_raw(
    std::span<std::uint8_t> ram_buffer,
    std::uint32_t entity_addr,
    std::int16_t new_weapon_frame,
    std::span<const WeaponOverlayPiece> pieces,
    std::uint32_t ram_base_addr = 0x00FF0000U);

} // namespace oasis::game::render
