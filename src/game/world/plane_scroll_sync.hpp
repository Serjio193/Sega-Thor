#pragma once

#include <array>
#include <cstdint>
#include <span>

namespace oasis::game::world {

inline constexpr std::uint32_t kUpdatePlaneScrollDeltasAddr = 0x00D188U;
inline constexpr std::uint32_t kWrapCameraToMapBoundsAddr = 0x00D266U;
inline constexpr std::uint32_t kFlushHScrollToVdpBufferAddr = 0x00D2B4U;

inline constexpr std::uint16_t kHScrollMask = 0x01FFU;
inline constexpr std::uint16_t kVScrollMask = 0x00FFU;
inline constexpr std::int32_t kOverflowWrapMin = -10000;
inline constexpr std::int32_t kOverflowWrapMax = 10000;

inline constexpr std::uint8_t kDirtyBitColStream = 0x01U; // bit 0
inline constexpr std::uint8_t kDirtyBitRowStream = 0x02U; // bit 1
inline constexpr std::uint8_t kDirtyBitHScroll = 0x04U;   // bit 2
inline constexpr std::uint8_t kDirtyBitVScroll = 0x08U;   // bit 3

struct PlaneScrollSyncState {
    std::int32_t cam_x{0};
    std::int32_t cam_y{0};
    std::uint16_t vdp_hscroll{0};      // 0..0x1FF
    std::uint16_t vdp_vscroll{0};      // 0..0x0FF
    std::int16_t pending_dx_tiles{0};  // +0x1C
    std::int16_t pending_dy_tiles{0};  // +0x1E
    std::uint16_t map_width_tiles{0};  // +0x20
    std::uint16_t map_height_tiles{0}; // +0x22
    std::uint8_t flags_27{0};          // +0x27 (bit 1 disables tile streaming)
    std::uint8_t dirty_flags{0};       // $FF16F0 bits 0..3
};

/// Models 0x00D188..0x00D264: updates 8px tile-crossing deltas, VDP H/V-scroll
/// registers, dirty flags, and guards cam_x/cam_y against [-10000, +10000].
void update_plane_scroll_deltas(
    PlaneScrollSyncState& state,
    std::int32_t target_x,
    std::int32_t target_y) noexcept;

/// Models 0x00D266..0x00D2B2: wraps cam_x/cam_y into [0, map_px_size).
void wrap_camera_to_map_bounds(PlaneScrollSyncState& state) noexcept;

/// Models 0x00D2B4..0x00D342: flushes vdp_hscroll to $FFFB5C (per-line/cell)
/// or $FF1344 (full-screen), offset +0 (Plane A) or +2 (Plane B).
void flush_hscroll_to_vdp_buffer(
    std::uint16_t vdp_hscroll,
    bool is_plane_b,
    bool per_line_mode,
    std::span<std::uint16_t> hscroll_line_buf,
    std::array<std::uint16_t, 2>& hscroll_full_buf) noexcept;

} // namespace oasis::game::world
