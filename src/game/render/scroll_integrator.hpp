#pragma once

#include <cstdint>
#include <span>

namespace thor::render {

inline constexpr std::uint32_t kAccumulateLongDeltaAndIntegrateAddr = 0x00D146U;
inline constexpr std::uint32_t kAccumulateWordDeltaAndIntegrateAddr = 0x00D168U;
inline constexpr std::uint32_t kCheckEnableAndIntegrateAddr = 0x00D17EU;
inline constexpr std::uint32_t kIntegrateScrollDeltasAddr = 0x00D188U;
inline constexpr std::uint32_t kNormalizeToroidalMapCoordsAddr = 0x00D266U;

inline constexpr std::uint32_t kPlaneARamOffset = 0x1716U;
inline constexpr std::uint32_t kPlaneBRamOffset = 0x173EU;

inline constexpr std::uint16_t kHScrollMask = 0x01FFU; // 512-pixel ring buffer
inline constexpr std::uint16_t kVScrollMask = 0x00FFU; // 256-pixel ring buffer

inline constexpr std::uint8_t kDirtyBitTileColStrip = 0x01U; // bit 0: horizontal tile col strip dirty
inline constexpr std::uint8_t kDirtyBitTileRowStrip = 0x02U; // bit 1: vertical tile row strip dirty
inline constexpr std::uint8_t kDirtyBitPlaneAHScroll = 0x04U; // bit 2: VDP Plane A HScroll dirty
inline constexpr std::uint8_t kDirtyBitPlaneAVScroll = 0x08U; // bit 3: VDP Plane A VScroll dirty

inline constexpr std::uint8_t kControlEnableIntegration = 0x01U;     // bit 0: enable integration
inline constexpr std::uint8_t kControlSuppressTileStripShift = 0x02U; // bit 1: suppress tile strip shift

/// Camera / Tilemap state matching authentic ROM layout at register a0 ($FF1716 / $FF173E).
struct TilemapCameraState {
    std::int16_t cam_x{0};          // +0x00: Camera World X coordinate (pixels)
    std::uint16_t cam_sub_x{0};     // +0x02: Camera Subpixel X (16.16 fractional)
    std::int16_t cam_y{0};          // +0x04: Camera World Y coordinate (pixels)
    std::uint16_t cam_sub_y{0};     // +0x06: Camera Subpixel Y (16.16 fractional)

    std::uint16_t vdp_hscroll{0};   // +0x0E: VDP Plane A HScroll accumulator (0..511)
    std::uint16_t vdp_vscroll{0};   // +0x10: VDP Plane A VScroll accumulator (0..255)

    std::int16_t delta_x{0};        // +0x12: Pixel Delta X for current frame (new_x - old_x)
    std::int16_t delta_y{0};        // +0x14: Pixel Delta Y for current frame (new_y - old_y)
    std::int16_t tile_col_step{0};   // +0x1C: Tile column shift count ((new_x >> 3) - (old_x >> 3))
    std::int16_t tile_row_step{0};   // +0x1E: Tile row shift count ((new_y >> 3) - (old_y >> 3))

    std::uint16_t map_width_tiles{0};  // +0x20: Map width in 8x8 tiles
    std::uint16_t map_height_tiles{0}; // +0x22: Map height in 8x8 tiles

    std::uint8_t dirty_flags{0};    // +0x26: Tilemap dirty status bitmask (bits 0..3)
    std::uint8_t control_flags{1};  // +0x27: Control flags (bit 0 = enable, bit 1 = suppress shift)
};

/// 0x00D146: Adds 32-bit fixed-point (16.16) deltas to camera X/Y, updates subpixels, and integrates scroll.
void accumulate_long_and_integrate(
    TilemapCameraState& state,
    std::int32_t delta_long_x,
    std::int32_t delta_long_y) noexcept;

/// 0x00D168: Adds 16-bit integer deltas to camera X/Y and integrates scroll.
void accumulate_word_and_integrate(
    TilemapCameraState& state,
    std::int16_t delta_word_x,
    std::int16_t delta_word_y) noexcept;

/// 0x00D17E: Checks control enable bit 0 (+0x27) before integrating scroll deltas.
bool check_enable_and_integrate(
    TilemapCameraState& state,
    std::int16_t new_x,
    std::int16_t new_y) noexcept;

/// 0x00D188: Calculates pixel deltas, VDP Plane A ring-buffer scroll accumulators, tile col/row shifts, and dirty flags.
void integrate_scroll_deltas(
    TilemapCameraState& state,
    std::int16_t new_x,
    std::int16_t new_y) noexcept;

/// 0x00D266: Normalizes camera world coordinates modulo map dimensions into [0..size_px).
void normalize_toroidal_coords(
    TilemapCameraState& state) noexcept;

/// Loads TilemapCameraState from a 64KB RAM window at the specified plane offset.
[[nodiscard]] TilemapCameraState load_state_from_ram(
    std::span<const std::uint8_t, 0x10000> ram,
    std::uint32_t plane_ram_offset) noexcept;

/// Stores TilemapCameraState back to a 64KB RAM window at the specified plane offset.
void store_state_to_ram(
    std::span<std::uint8_t, 0x10000> ram,
    std::uint32_t plane_ram_offset,
    const TilemapCameraState& state) noexcept;

/// RAM window integration helper for 0x00D146.
void accumulate_long_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    std::uint32_t plane_ram_offset,
    std::int32_t delta_long_x,
    std::int32_t delta_long_y) noexcept;

/// RAM window integration helper for 0x00D168.
void accumulate_word_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    std::uint32_t plane_ram_offset,
    std::int16_t delta_word_x,
    std::int16_t delta_word_y) noexcept;

/// RAM window integration helper for 0x00D188.
void integrate_scroll_deltas_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    std::uint32_t plane_ram_offset,
    std::int16_t new_x,
    std::int16_t new_y) noexcept;

/// RAM window integration helper for 0x00D266.
void normalize_toroidal_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    std::uint32_t plane_ram_offset) noexcept;

} // namespace thor::render

namespace oasis::game::render {
using namespace thor::render;
} // namespace oasis::game::render
