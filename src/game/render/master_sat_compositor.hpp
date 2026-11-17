#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace oasis::game::render {

/// Configuration and constants for Master Entity SAT Sprite Compositor (0x00A9A0..0x00AD76).
struct MasterSatCompositorConfig {
    static constexpr std::uint32_t kCompositorEntryAddr = 0x00A9A0U;
    static constexpr std::uint32_t kEarlyExitRtsAddr = 0x00ACFCU;
    static constexpr std::uint32_t kShadowTableAddr = 0x00ACFEU;
    static constexpr std::uint32_t kQuotaExitRtsAddr = 0x00AD40U;
    static constexpr std::uint32_t kTileCountTableAddr = 0x00AD56U;
    static constexpr std::uint32_t kCompositorEndAddr = 0x00AD76U;

    // RAM variables
    static constexpr std::uint32_t kVdpSatBufferRam = 0x00FF13CCU;
    static constexpr std::uint32_t kDeferredFlagRam = 0x00FF1651U;
    static constexpr std::uint32_t kVramQuotaRam = 0x00FF1652U;
    static constexpr std::uint32_t kCameraPlane1Ram = 0x00FF1716U;
    static constexpr std::uint32_t kCameraPlane2Ram = 0x00FF173EU;
    static constexpr std::uint32_t kSatLinkCountRam = 0x00FF188AU;
    static constexpr std::uint32_t kSatByteOffsetRam = 0x00FF188CU;
    static constexpr std::uint32_t kDmaQueueRam = 0x00FF1892U;
    static constexpr std::uint32_t kScreenJitterRam = 0x00FF1971U;

    // Slot field offsets
    static constexpr std::size_t kTypeOffset = 0x00U;
    static constexpr std::size_t kWorldXOffset = 0x08U;
    static constexpr std::size_t kWorldYOffset = 0x0CU;
    static constexpr std::size_t kElevZOffset = 0x10U;
    static constexpr std::size_t kGroundHOffset = 0x14U;
    static constexpr std::size_t kAnimFrameOffset = 0x30U;
    static constexpr std::size_t kFrameCacheOffset = 0x34U;
    static constexpr std::size_t kFlags36Offset = 0x36U;
    static constexpr std::size_t kFlags3AOffset = 0x3AU;

    // Coordinate & Viewport bias constants
    static constexpr std::int16_t kCoordBias = 128; // 0x80
    static constexpr std::uint16_t kViewportClipOrigin = 0x60U;
    static constexpr std::uint16_t kViewportClipHeight = 0x100U;
    static constexpr std::uint16_t kViewportClipWidth = 0x160U;

    // Hardware limit
    static constexpr std::uint16_t kMaxSatSprites = 80U;
};

/// Preset entry in embedded Dynamic Shadow Parameter Table (0x00ACFE..0x00AD1E).
struct DynamicShadowPreset {
    std::uint16_t y_offset{0U};
    std::uint16_t size_word{0U};
    std::uint16_t tile_index{0U};
    std::uint16_t x_offset{0U};

    [[nodiscard]] constexpr bool operator==(const DynamicShadowPreset&) const noexcept = default;
};

/// 4 embedded dynamic shadow presets in ROM.
inline constexpr std::array<DynamicShadowPreset, 4> kShadowTable = {{
    {3U, 0x0000U, 0x0548U, 3U},   // Preset 0: 1x1, small
    {3U, 0x0400U, 0x0549U, 8U},   // Preset 1: 2x1, medium
    {3U, 0x0800U, 0x054BU, 11U},  // Preset 2: 3x1, large
    {8U, 0x0D00U, 0x054EU, 16U},  // Preset 3: 4x2, giant
}};

/// 4x4 Sprite Tile Count Table (0x00AD56..0x00AD76).
/// Indexed by (width - 1) * 4 + (height - 1).
inline constexpr std::array<std::uint16_t, 16> kSpriteTileCounts = {{
    1U, 2U, 3U, 4U,
    2U, 4U, 6U, 8U,
    3U, 6U, 9U, 12U,
    4U, 8U, 12U, 16U,
}};

/// Single hardware sprite piece within an entity animation frame.
struct FrameSpritePiece {
    std::int16_t y_offset{0};
    std::uint8_t size{0U};      // Mega Drive (h-1) | ((w-1) << 2)
    std::uint16_t tile_attr{0U};
    std::int16_t x_offset{0};
};

/// Emitted SAT entry.
struct CompositorSatEntry {
    std::int16_t y{0};
    std::uint8_t size{0U};
    std::uint8_t link{0U};
    std::uint16_t tile_attr{0U};
    std::int16_t x{0};
};

/// Result of evaluating frame cache and DMA quota.
struct FrameCacheEvaluation {
    std::int16_t selected_frame{0};
    bool deferred{false};
    bool queue_dma{false};
};

/// Result of master entity SAT composition pass.
struct MasterCompositorResult {
    bool early_exited{false};
    FrameCacheEvaluation cache_eval{};
    std::int16_t screen_x{0};
    std::int16_t screen_y{0};
    std::vector<CompositorSatEntry> emitted_pieces;
    std::optional<CompositorSatEntry> emitted_shadow;
    std::uint16_t final_sprite_count{0U};
    std::uint16_t final_sat_offset{0U};
};

/// Checks if screen Y is outside Mega Drive viewport range [0x60, 0x160].
[[nodiscard]] constexpr bool is_screen_clipped_y(std::int16_t screen_y) noexcept {
    const auto rel = static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(screen_y) - MasterSatCompositorConfig::kViewportClipOrigin);
    return rel > MasterSatCompositorConfig::kViewportClipHeight;
}

/// Checks if screen X is outside Mega Drive viewport range [0x60, 0x1C0].
[[nodiscard]] constexpr bool is_screen_clipped_x(std::int16_t screen_x) noexcept {
    const auto rel = static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(screen_x) - MasterSatCompositorConfig::kViewportClipOrigin);
    return rel > MasterSatCompositorConfig::kViewportClipWidth;
}

/// Returns tile count for a sprite size descriptor.
[[nodiscard]] constexpr std::uint16_t get_sprite_tile_count(std::uint8_t size_byte) noexcept {
    const std::size_t idx = static_cast<std::size_t>(size_byte & 0x0FU);
    return kSpriteTileCounts[idx];
}

/// Evaluates frame caching and VRAM DMA quota (0x00A9D6).
[[nodiscard]] constexpr FrameCacheEvaluation evaluate_frame_cache(
    std::int16_t cached_frame,
    std::int16_t new_frame,
    std::uint16_t required_tiles,
    std::uint16_t remaining_quota) noexcept {
    if (new_frame == cached_frame) {
        return { .selected_frame = cached_frame, .deferred = false, .queue_dma = false };
    }
    if ((cached_frame >= 0) && (required_tiles > remaining_quota)) {
        return { .selected_frame = cached_frame, .deferred = true, .queue_dma = false };
    }
    return { .selected_frame = new_frame, .deferred = false, .queue_dma = true };
}

/// Transforms 3D world coordinates to 2D screen coordinates (0x00AB00).
constexpr void project_world_to_screen(
    std::int16_t world_x,
    std::int16_t world_y,
    std::int16_t elev_z,
    std::int16_t cam_x,
    std::int16_t cam_y,
    std::int16_t& out_screen_x,
    std::int16_t& out_screen_y) noexcept {
    out_screen_x = static_cast<std::int16_t>(
        world_x - cam_x + MasterSatCompositorConfig::kCoordBias);
    out_screen_y = static_cast<std::int16_t>(
        (world_y - cam_y + MasterSatCompositorConfig::kCoordBias) - elev_z);
}

/// Blits dynamic shadow sprite at ground plane using preset parameters (0x00ACD0).
[[nodiscard]] std::optional<CompositorSatEntry> blit_dynamic_shadow(
    std::int16_t world_x,
    std::int16_t world_y,
    std::int16_t ground_h,
    std::int16_t cam_x,
    std::int16_t cam_y,
    std::size_t preset_index,
    std::uint16_t current_sprites) noexcept;

/// High-level entity state for master compositor.
struct MasterEntityRenderState {
    std::int16_t type{0};
    std::int16_t world_x{0};
    std::int16_t world_y{0};
    std::int16_t elev_z{0};
    std::int16_t ground_h{0};
    std::int16_t anim_frame{0};
    std::int16_t frame_cache{-1};
    std::uint8_t flags_36{0U};
    std::uint8_t flags_3a{0U};
};

/// Executes master entity SAT composition pass for a single entity (0x00A9A0..0x00AD76).
[[nodiscard]] MasterCompositorResult compose_master_entity_sat(
    MasterEntityRenderState& entity,
    std::span<const FrameSpritePiece> pieces,
    std::int16_t cam_x,
    std::int16_t cam_y,
    std::uint16_t remaining_quota,
    std::uint16_t current_sprites = 0U,
    std::uint16_t current_sat_offset = 0U);

/// Executes master entity SAT composition against raw 64KB Mega Drive RAM byte buffer.
[[nodiscard]] MasterCompositorResult compose_master_entity_sat_raw(
    std::span<std::uint8_t> ram_buffer,
    std::uint32_t entity_addr,
    std::span<const FrameSpritePiece> pieces,
    std::uint32_t ram_base_addr = 0x00FF0000U);

} // namespace oasis::game::render
