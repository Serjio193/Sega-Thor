#pragma once

#include "game/render/master_sat_compositor.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace oasis::game::render {

/// Configuration and constants for Dynamic FX, Aura, Wave & Attachment SAT Renderer P1 (0x00B34C..0x00B56E).
struct DynamicFxP1Config {
    static constexpr std::uint32_t kMasterFxDispatcherAddr = 0x00B34CU;
    static constexpr std::uint32_t kDispatcherExitRtsAddr = 0x00B382U;
    static constexpr std::uint32_t kAuraEmitterAddr = 0x00B3D2U;
    static constexpr std::uint32_t kAuraExitRtsAddr = 0x00B42AU;
    static constexpr std::uint32_t kAuraTableAddr = 0x00B42CU;
    static constexpr std::uint32_t kWaveRingEmitterAddr = 0x00B45CU;
    static constexpr std::uint32_t kWaveRingExitRtsAddr = 0x00B4C6U;
    static constexpr std::uint32_t kWaveSmallTableAddr = 0x00B4C8U;
    static constexpr std::uint32_t kWaveLargeTableAddr = 0x00B4E0U;
    static constexpr std::uint32_t kRippleEmitterAddr = 0x00B4FEU;
    static constexpr std::uint32_t kRippleBitClearAddr = 0x00B54EU;
    static constexpr std::uint32_t kRippleExitRtsAddr = 0x00B554U;
    static constexpr std::uint32_t kRippleTableAddr = 0x00B556U;
    static constexpr std::uint32_t kDynamicFxP1RangeEnd = 0x00B56EU;

    // RAM variables
    static constexpr std::uint32_t kGlobalFxSuppressionRam = 0x00FF1927U;
    static constexpr std::uint32_t kVdpSatBufferRam = 0x00FF13CCU;
    static constexpr std::uint32_t kSatLinkCountRam = 0x00FF188AU;
    static constexpr std::uint32_t kSatByteOffsetRam = 0x00FF188CU;
    static constexpr std::uint32_t kCameraPlane1Ram = 0x00FF1716U;
    static constexpr std::uint32_t kCameraPlane2Ram = 0x00FF173EU;

    // Slot field offsets
    static constexpr std::size_t kTypeOffset = 0x00U;
    static constexpr std::size_t kWorldXOffset = 0x08U;
    static constexpr std::size_t kWorldYOffset = 0x0CU;
    static constexpr std::size_t kElevZOffset = 0x10U;
    static constexpr std::size_t kGroundHOffset = 0x14U;
    static constexpr std::size_t kFxModeFlagsOffset = 0x39U;
    static constexpr std::size_t kShadowPresetOffset = 0x3DU;
    static constexpr std::size_t kWaveRippleCounterOffset = 0x98U;
    static constexpr std::size_t kAuraCounterOffset = 0x9BU;

    // FX Mode Flags (+0x39)
    static constexpr std::uint8_t kAuraBitMask = 0x01U;         // Bit 0
    static constexpr std::uint8_t kWaveRingBitMask = 0x08U;     // Bit 3
    static constexpr std::uint8_t kGroundRippleBitMask = 0x10U; // Bit 4

    // Viewport Clipping Bounds
    static constexpr std::int16_t kViewportMinY = 0x60;
    static constexpr std::int16_t kViewportMaxY = 0x160;
    static constexpr std::int16_t kViewportMinX = 0x60;
    static constexpr std::int16_t kViewportMaxX = 0x1C0;
    static constexpr std::int16_t kCoordBias = 128;
    static constexpr std::uint16_t kMaxSatSprites = 80U;
};

/// 3-byte FX entry descriptor matching ROM tables.
struct FxTableEntry {
    std::uint8_t tile_or_flag{0U};
    std::int8_t dx{0};
    std::int8_t dy{0};

    [[nodiscard]] constexpr bool operator==(const FxTableEntry&) const noexcept = default;
};

/// 16-entry Aura Offset Table at 0x00B42C.
inline constexpr std::array<FxTableEntry, 16> kAuraTable = {{
    {255U, 0, 0},     {7U, 4, 4},       {8U, 4, 4},       {8U, 4, 4},
    {9U, 4, 4},       {9U, 4, 4},       {10U, 4, 4},      {255U, 0, 0},
    {255U, 0, 0},     {255U, 0, 0},     {7U, -4, -4},     {8U, -4, -4},
    {8U, -4, -4},     {9U, -4, -4},     {9U, -4, -4},     {10U, -4, -4},
}};

/// 8-entry Wave Ring Small Table at 0x00B4C8.
inline constexpr std::array<FxTableEntry, 8> kWaveSmallTable = {{
    {0U, 0, -8},  {0U, 0, -8},  {1U, 0, -8},  {1U, 0, -8},
    {2U, 0, -8},  {2U, 0, -8},  {3U, 0, -8},  {3U, 0, -8},
}};

/// 10-entry Wave Ring Large Table at 0x00B4E0.
inline constexpr std::array<FxTableEntry, 10> kWaveLargeTable = {{
    {30U, 0, -12}, {30U, 0, -12}, {31U, 0, -12}, {31U, 0, -12}, {32U, 0, -12},
    {32U, 0, -12}, {33U, 0, -12}, {33U, 0, -12}, {34U, 0, -12}, {34U, 0, -12},
}};

/// 8-entry Ground Ripple Table at 0x00B556.
inline constexpr std::array<FxTableEntry, 8> kRippleTable = {{
    {59U, 0, 0},  {59U, 0, 0},  {60U, 0, 0},  {60U, 0, 0},
    {61U, 0, 0},  {61U, 0, 0},  {62U, 0, 0},  {62U, 0, 0},
}};

/// High-level entity state participating in Dynamic FX Part 1 rendering.
struct DynamicFxEntityState {
    std::int16_t type{0};
    std::int16_t world_x{0};
    std::int16_t world_y{0};
    std::int16_t elev_z{0};
    std::int16_t ground_h{0};
    std::uint8_t fx_flags_39{0U};
    std::uint8_t shadow_preset_3d{0U};
    std::uint8_t aura_counter_9b{0U};
    std::uint8_t wave_counter_98{0U};
};

/// Combined result of dynamic FX pass part 1.
struct DynamicFxP1Result {
    bool suppressed{false};
    std::optional<CompositorSatEntry> emitted_aura;
    std::optional<CompositorSatEntry> emitted_wave;
    std::optional<CompositorSatEntry> emitted_ripple;
    bool ripple_completed{false};
    std::uint8_t updated_flags_39{0U};
    std::uint16_t final_sprite_count{0U};
    std::uint16_t final_sat_offset{0U};
};

/// Master FX should-render check (0x00B34C..0x00B356).
[[nodiscard]] constexpr bool should_render_dynamic_fx(
    std::uint8_t global_suppress_flag,
    std::uint8_t fx_flags_39) noexcept {
    return (global_suppress_flag == 0U) && (fx_flags_39 != 0U);
}

[[nodiscard]] constexpr bool has_aura_fx(std::uint8_t fx_flags_39) noexcept {
    return (fx_flags_39 & DynamicFxP1Config::kAuraBitMask) != 0U;
}

[[nodiscard]] constexpr bool has_wave_ring_fx(std::uint8_t fx_flags_39) noexcept {
    return (fx_flags_39 & DynamicFxP1Config::kWaveRingBitMask) != 0U;
}

[[nodiscard]] constexpr bool has_ground_ripple_fx(std::uint8_t fx_flags_39) noexcept {
    return (fx_flags_39 & DynamicFxP1Config::kGroundRippleBitMask) != 0U;
}

/// Viewport clipping check for FX sprites.
[[nodiscard]] constexpr bool is_dynamic_fx_clipped(std::int16_t screen_y, std::int16_t screen_x) noexcept {
    const auto y_rel = static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(screen_y) - DynamicFxP1Config::kViewportMinY);
    const auto x_rel = static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(screen_x) - DynamicFxP1Config::kViewportMinX);
    return (y_rel > 0x100U) || (x_rel > 0x160U);
}

/// Executes Bit 0 Aura / Spark Emitter (0x00B3D2..0x00B45C).
[[nodiscard]] std::optional<CompositorSatEntry> step_aura_fx(
    DynamicFxEntityState& entity,
    std::int16_t cam_x,
    std::int16_t cam_y,
    std::uint16_t current_sprites) noexcept;

/// Executes Bit 3 Wave Ring Emitter (0x00B45C..0x00B4FE).
[[nodiscard]] std::optional<CompositorSatEntry> step_wave_ring_fx(
    DynamicFxEntityState& entity,
    std::int16_t cam_x,
    std::int16_t cam_y,
    std::uint16_t current_sprites) noexcept;

/// Executes Bit 4 Ground Ripple Emitter (0x00B4FE..0x00B56E).
[[nodiscard]] std::optional<CompositorSatEntry> step_ground_ripple_fx(
    DynamicFxEntityState& entity,
    std::int16_t cam_x,
    std::int16_t cam_y,
    std::uint16_t current_sprites) noexcept;

/// Executes Dynamic FX Part 1 pass (0x00B34C..0x00B56E).
[[nodiscard]] DynamicFxP1Result dispatch_dynamic_fx_p1(
    DynamicFxEntityState& entity,
    std::uint8_t global_suppress_flag,
    std::int16_t cam_x,
    std::int16_t cam_y,
    std::uint16_t current_sprites = 0U,
    std::uint16_t current_sat_offset = 0U);

/// Executes Dynamic FX Part 1 pass against raw 64KB Mega Drive RAM byte buffer.
[[nodiscard]] DynamicFxP1Result dispatch_dynamic_fx_p1_raw(
    std::span<std::uint8_t> ram_buffer,
    std::uint32_t entity_addr,
    std::uint32_t ram_base_addr = 0x00FF0000U);

} // namespace oasis::game::render
