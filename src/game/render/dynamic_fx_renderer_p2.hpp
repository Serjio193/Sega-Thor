#pragma once

#include "game/render/master_sat_compositor.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace oasis::game::render {

/// Configuration and constants for Dynamic FX Halo, Swirl & SAT Piece Compositor Subsystem (0x00B56E..0x00B79A).
struct DynamicFxP2Config {
    static constexpr std::uint32_t kHaloEmitterAddr = 0x00B56EU;
    static constexpr std::uint32_t kHaloExitRtsAddr = 0x00B5C6U;
    static constexpr std::uint32_t kHaloBitClearAddr = 0x00B5C8U;
    static constexpr std::uint32_t kHaloTableAddr = 0x00B5D0U;
    static constexpr std::uint32_t kSwirlEmitterAddr = 0x00B610U;
    static constexpr std::uint32_t kSwirlExitRtsAddr = 0x00B65EU;
    static constexpr std::uint32_t kSwirlTableAddr = 0x00B660U;
    static constexpr std::uint32_t kSecondarySwirlAddr = 0x00B66AU;
    static constexpr std::uint32_t kFxBlitterAddr = 0x00B6A6U;
    static constexpr std::uint32_t kFxBlitterExitRtsAddr = 0x00B72EU;
    static constexpr std::uint32_t kRawPieceBlitterAddr = 0x00B730U;
    static constexpr std::uint32_t kRawPieceBlitterExitRtsAddr = 0x00B798U;
    static constexpr std::uint32_t kDynamicFxP2RangeEnd = 0x00B79AU;

    // RAM variables
    static constexpr std::uint32_t kGlobalPauseFlagRam = 0x00FF185DU;
    static constexpr std::uint32_t kGlobalFxSuppressionRam = 0x00FF1927U;
    static constexpr std::uint32_t kVdpSatBufferRam = 0x00FF13CCU;
    static constexpr std::uint32_t kSatLinkCountRam = 0x00FF188AU;
    static constexpr std::uint32_t kSatByteOffsetRam = 0x00FF188CU;
    static constexpr std::uint32_t kCameraPlane1Ram = 0x00FF1716U;
    static constexpr std::uint32_t kCameraPlane2Ram = 0x00FF173EU;
    static constexpr std::uint32_t kSecondaryPoolThreshold = 0x00FF2954U;

    // Slot field offsets
    static constexpr std::size_t kSecondaryCounterOffset = 0x03U;
    static constexpr std::size_t kWorldXOffset = 0x08U;
    static constexpr std::size_t kWorldYOffset = 0x0CU;
    static constexpr std::size_t kElevZOffset = 0x10U;
    static constexpr std::size_t kGroundHOffset = 0x14U;
    static constexpr std::size_t kBaseTileOffset = 0x18U;
    static constexpr std::size_t kFxModeFlagsOffset = 0x39U;
    static constexpr std::size_t kShadowPresetOffset = 0x3DU;
    static constexpr std::size_t kSwirlCounterOffset = 0x99U;
    static constexpr std::size_t kHaloCounterOffset = 0x9AU;

    // FX Mode Flags (+0x39)
    static constexpr std::uint8_t kSwirlBitMask = 0x02U; // Bit 1
    static constexpr std::uint8_t kHaloBitMask = 0x04U;  // Bit 2

    // Viewport Clipping Bounds
    static constexpr std::int16_t kViewportMinY = 0x60;
    static constexpr std::int16_t kViewportMaxY = 0x160;
    static constexpr std::int16_t kViewportMinX = 0x60;
    static constexpr std::int16_t kViewportMaxX = 0x1C0;
    static constexpr std::int16_t kScreenBias = 0x80;
    static constexpr std::uint16_t kMaxSatSprites = 80U;
    static constexpr std::uint16_t kFxVramBaseTile = 0x055EU;
};

/// 3-byte Halo table entry matching ROM table at 0x00B5D0.
struct HaloTableEntry {
    std::uint8_t frame_flag{0U};
    std::int8_t dx{0};
    std::int8_t dy{0};

    [[nodiscard]] constexpr bool operator==(const HaloTableEntry&) const noexcept = default;
};

/// 21-entry Halo Table at 0x00B5D0.
inline constexpr std::array<HaloTableEntry, 21> kHaloTable = {{
    {255U, 3, 3},  {6U, 6, 3},   {6U, 6, 3},   {6U, -4, 3},  {6U, -4, 3},
    {5U, -4, 3},   {5U, -4, 3},  {4U, -4, 3},  {4U, -4, 3},  {6U, 3, 1},
    {6U, 3, 1},    {5U, 3, 1},   {5U, 3, 1},   {4U, 3, 1},   {4U, 3, 1},
    {6U, -2, 4},   {6U, -2, 4},  {5U, -2, 4},  {5U, -2, 4},  {4U, -2, 4},
    {4U, -2, 4},
}};

/// 9-entry Swirl Table at 0x00B660.
inline constexpr std::array<std::uint8_t, 9> kSwirlTable = {{
    20U, 20U, 20U, 22U, 22U, 22U, 24U, 24U, 24U
}};

/// Single sprite piece in an FX frame.
struct FxSpritePiece {
    std::uint8_t size{0U};
    std::int8_t y_offset{0};
    std::uint16_t tile_attr{0U};
    std::int8_t x_flip{0};
    std::int8_t x_unflip{0};

    [[nodiscard]] constexpr bool operator==(const FxSpritePiece&) const noexcept = default;
};

/// Entity state participating in Dynamic FX Part 2 rendering.
struct DynamicFxEntityStateP2 {
    std::int16_t world_x{0};
    std::int16_t world_y{0};
    std::int16_t elev_z{0};
    std::int16_t ground_h{0};
    std::uint16_t base_tile_18{0U};
    std::uint8_t fx_flags_39{0U};
    std::uint8_t shadow_preset_3d{0U};
    std::uint8_t halo_counter{0U};  // +0x9A or +0x03
    std::uint8_t swirl_counter{0U}; // +0x99 or +0x03
};

/// Result of dynamic FX Part 2 pass.
struct DynamicFxP2Result {
    bool suppressed{false};
    std::vector<CompositorSatEntry> emitted_halo;
    std::vector<CompositorSatEntry> emitted_swirl;
    bool halo_completed{false};
    std::uint8_t updated_flags_39{0U};
    std::uint16_t final_sprite_count{0U};
    std::uint16_t final_sat_offset{0U};
};

/// Computes base FX tile attribute with entity priority mask (0x00B6B0..0x00B6B8).
[[nodiscard]] constexpr std::uint16_t compute_fx_tile_attr(std::uint16_t base_tile_18) noexcept {
    return static_cast<std::uint16_t>((base_tile_18 & 0x8000U) | DynamicFxP2Config::kFxVramBaseTile);
}

/// Computes primary swirl Y offset from shadow preset (0x00B646..0x00B64C): (preset >> 2) - 6.
[[nodiscard]] constexpr std::int16_t compute_swirl_y_offset(std::uint8_t shadow_preset_3d) noexcept {
    return static_cast<std::int16_t>(
        static_cast<std::int16_t>(shadow_preset_3d >> 2) - 6);
}

/// Computes secondary swirl Y offset (0x00B68A..0x00B692): -((preset >> 2) + 2).
[[nodiscard]] constexpr std::int16_t compute_secondary_swirl_y_offset(std::uint8_t shadow_preset_3d) noexcept {
    return static_cast<std::int16_t>(
        -static_cast<std::int16_t>((shadow_preset_3d >> 2) + 2));
}

/// Viewport clipping check for FX sprites.
[[nodiscard]] constexpr bool is_dynamic_fx_p2_clipped(std::int16_t screen_y, std::int16_t screen_x) noexcept {
    const auto y_rel = static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(screen_y) - DynamicFxP2Config::kViewportMinY);
    const auto x_rel = static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(screen_x) - DynamicFxP2Config::kViewportMinX);
    return (y_rel > 0x100U) || (x_rel > 0x160U);
}

/// Retrieves ROM frame pieces for given frame index (0..29).
[[nodiscard]] std::span<const FxSpritePiece> get_fx_frame_pieces(std::uint8_t frame_index) noexcept;

/// Executes Universal FX Piece Blitter (0x00B6A6..0x00B72E).
[[nodiscard]] std::vector<CompositorSatEntry> blit_fx_frame(
    std::uint8_t frame_index,
    std::int16_t screen_x,
    std::int16_t screen_y,
    std::uint16_t base_tile_attr,
    std::uint16_t current_sprites,
    bool flip_x = false) noexcept;

/// Executes Static Raw Piece Blitter (0x00B730..0x00B798).
[[nodiscard]] std::vector<CompositorSatEntry> blit_raw_pieces(
    std::span<const FxSpritePiece> pieces,
    std::int16_t screen_x,
    std::int16_t screen_y,
    std::uint16_t base_tile_attr,
    std::uint16_t current_sprites) noexcept;

/// Executes Bit 2 Pulsing Halo Emitter (0x00B56E..0x00B5CE).
[[nodiscard]] std::vector<CompositorSatEntry> step_halo_fx(
    DynamicFxEntityStateP2& entity,
    bool is_paused,
    std::int16_t cam_x,
    std::int16_t cam_y,
    std::uint16_t current_sprites,
    bool& out_completed) noexcept;

/// Executes Bit 1 Ground Energy Swirl Emitter (0x00B610..0x00B65E).
[[nodiscard]] std::vector<CompositorSatEntry> step_swirl_fx(
    DynamicFxEntityStateP2& entity,
    bool is_paused,
    std::int16_t cam_x,
    std::int16_t cam_y,
    std::uint16_t current_sprites,
    bool is_secondary = false) noexcept;

/// Executes combined Dynamic FX Part 2 pass.
[[nodiscard]] DynamicFxP2Result dispatch_dynamic_fx_p2(
    DynamicFxEntityStateP2& entity,
    bool is_paused,
    std::uint8_t global_suppress_flag,
    std::int16_t cam_x,
    std::int16_t cam_y,
    std::uint16_t current_sprites = 0U,
    std::uint16_t current_sat_offset = 0U);

/// Executes Dynamic FX Part 2 pass against raw 64KB Mega Drive RAM byte buffer.
[[nodiscard]] DynamicFxP2Result dispatch_dynamic_fx_p2_raw(
    std::span<std::uint8_t> ram_buffer,
    std::uint32_t entity_addr,
    std::uint32_t ram_base_addr = 0x00FF0000U);

} // namespace oasis::game::render
