#pragma once

#include "game/render/master_sat_compositor.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace oasis::game::render {

/// Configuration and constants for Static Entity & Shadowless Fast SAT Compositors (0x00AD76..0x00B15E).
struct StaticFastSatCompositorConfig {
    static constexpr std::uint32_t kStaticCompositorAddr = 0x00AD76U;
    static constexpr std::uint32_t kStaticExitRtsAddr = 0x00B010U;
    static constexpr std::uint32_t kDynamicTailBsrAddr = 0x00B012U;
    static constexpr std::uint32_t kFastCompositorAddr = 0x00B022U;
    static constexpr std::uint32_t kBufferBaseSubtractAddr = 0x00B14AU;
    static constexpr std::uint32_t kFastExitRtsAddr = 0x00B15CU;
    static constexpr std::uint32_t kCompositorRangeEnd = 0x00B15EU;

    // RAM variables
    static constexpr std::uint32_t kVdpSatBufferRam = 0x00FF13CCU;
    static constexpr std::uint32_t kSatLinkCountRam = 0x00FF188AU;
    static constexpr std::uint32_t kSatByteOffsetRam = 0x00FF188CU;
    static constexpr std::uint32_t kCameraPlane1Ram = 0x00FF1716U;
    static constexpr std::uint32_t kCameraPlane2Ram = 0x00FF173EU;

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

/// Result of evaluating static entity SAT composition (0x00AD76..0x00B022).
struct StaticCompositorResult {
    bool early_exited{false};
    std::int16_t screen_x{0};
    std::int16_t screen_y{0};
    std::vector<CompositorSatEntry> emitted_pieces;
    std::optional<CompositorSatEntry> emitted_shadow;
    std::uint16_t final_sprite_count{0U};
    std::uint16_t final_sat_offset{0U};
};

/// Result of evaluating shadowless fast SAT composition (0x00B022..0x00B15E).
struct FastCompositorResult {
    std::int16_t screen_x{0};
    std::int16_t screen_y{0};
    std::vector<CompositorSatEntry> emitted_pieces;
    std::uint16_t final_sprite_count{0U};
    std::uint16_t final_sat_offset{0U};
};

/// Mode selection logic from SAT dispatch passes: selects 0x00AD76 (Static) or 0x00A9A0 (Dynamic).
[[nodiscard]] constexpr std::uint32_t select_sat_compositor_tier(std::uint8_t flags_36) noexcept {
    if ((flags_36 & 0x01U) == 0U) {
        return StaticFastSatCompositorConfig::kStaticCompositorAddr;
    }
    return MasterSatCompositorConfig::kCompositorEntryAddr;
}

/// Checks visibility flag: bit 1 of flags_36 must be clear for entity to render.
[[nodiscard]] constexpr bool is_static_entity_visible(std::uint8_t flags_36) noexcept {
    return (flags_36 & 0x02U) == 0U;
}

/// Viewport Y clipping check for static & fast compositors: rel > 0x100.
[[nodiscard]] constexpr bool is_static_viewport_clipped_y(std::int16_t screen_y) noexcept {
    const auto rel = static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(screen_y) - StaticFastSatCompositorConfig::kViewportMinY);
    return rel > StaticFastSatCompositorConfig::kViewportClipHeight;
}

/// Viewport X clipping check for static & fast compositors: rel > 0x160.
[[nodiscard]] constexpr bool is_static_viewport_clipped_x(std::int16_t screen_x) noexcept {
    const auto rel = static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(screen_x) - StaticFastSatCompositorConfig::kViewportMinX);
    return rel > StaticFastSatCompositorConfig::kViewportClipWidth;
}

/// Combined viewport clipping check for static & fast compositors.
[[nodiscard]] constexpr bool is_static_viewport_clipped(std::int16_t screen_y, std::int16_t screen_x) noexcept {
    return is_static_viewport_clipped_y(screen_y) || is_static_viewport_clipped_x(screen_x);
}

/// Executes Static Entity SAT Compositor (0x00AD76..0x00B022).
/// Skips dynamic DMA queueing, checks elevation Z, clips to viewport, and blits shadows.
[[nodiscard]] StaticCompositorResult compose_static_entity_sat(
    const MasterEntityRenderState& entity,
    std::span<const FrameSpritePiece> pieces,
    std::int16_t cam_x,
    std::int16_t cam_y,
    std::uint16_t current_sprites = 0U,
    std::uint16_t current_sat_offset = 0U);

/// Executes Shadowless Fast SAT Compositor (0x00B022..0x00B15E).
/// Minimalist sprite emitter for background objects, pickups, and VFX; no shadow generation.
[[nodiscard]] FastCompositorResult compose_fast_shadowless_sat(
    std::int16_t world_x,
    std::int16_t world_y,
    std::int16_t elev_z,
    std::span<const FrameSpritePiece> pieces,
    std::int16_t cam_x,
    std::int16_t cam_y,
    std::uint16_t current_sprites = 0U,
    std::uint16_t current_sat_offset = 0U);

/// Executes Static Entity SAT Compositor against raw Mega Drive 64KB RAM byte buffer.
[[nodiscard]] StaticCompositorResult compose_static_entity_sat_raw(
    std::span<std::uint8_t> ram_buffer,
    std::uint32_t entity_addr,
    std::span<const FrameSpritePiece> pieces,
    std::uint32_t ram_base_addr = 0x00FF0000U);

/// Executes Shadowless Fast SAT Compositor against raw Mega Drive 64KB RAM byte buffer.
[[nodiscard]] FastCompositorResult compose_fast_shadowless_sat_raw(
    std::span<std::uint8_t> ram_buffer,
    std::int16_t world_x,
    std::int16_t world_y,
    std::int16_t elev_z,
    std::span<const FrameSpritePiece> pieces,
    std::uint32_t ram_base_addr = 0x00FF0000U);

} // namespace oasis::game::render
