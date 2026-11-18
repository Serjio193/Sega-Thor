#pragma once

#include "game/render/scroll_integrator.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace thor::render {

inline constexpr std::uint32_t kTilemapFillRectByteAddr = 0x00DB1AU;
inline constexpr std::uint32_t kTilemapFillRectByteEndAddr = 0x00DB64U;
inline constexpr std::uint32_t kTilemapCopyRectWordAndSyncAddr = 0x00DB64U;
inline constexpr std::uint32_t kTilemapCopyRectWordOnlyAddr = 0x00DB68U;
inline constexpr std::uint32_t kTilemapCopyRectWordEndAddr = 0x00DBCAU;
inline constexpr std::uint32_t kTilemapCopyRectByteAddr = 0x00DBCAU;
inline constexpr std::uint32_t kTilemapCopyRectByteEndAddr = 0x00DC26U;
inline constexpr std::uint32_t kVdpSyncDirtyPlaneRectAddr = 0x00DC26U;
inline constexpr std::uint32_t kTilemapRectOpsEndAddr = 0x00DC26U;

/// RAM offsets and flags matching 0x00DB1A..0x00DC26 without colliding across thor::render.
struct TilemapRectOpsConfig {
    static constexpr std::uint32_t kMegaDriveRamBase = 0x00FF0000U;
    static constexpr std::uint32_t kPlaneAContextRamOffset = kPlaneARamOffset; // 0x1716
    static constexpr std::uint32_t kPlaneBContextRamOffset = kPlaneBRamOffset; // 0x173E
    static constexpr std::uint32_t kPlaneAAuxRamOffset = 0x1766U;
    static constexpr std::uint32_t kPlaneBAuxRamOffset = 0x1770U;
    static constexpr std::uint32_t kScrollModeRamOffset = 0x16F0U;
    static constexpr std::uint8_t kScrollModeSelectPlaneBBit = 0x80U; // $FF16F0 bit 7
};

/// Inclusive rectangular tile bounds (x1..x2, y1..y2) matching M68K d1..d4.
struct TilemapRect {
    std::int16_t x1{0};
    std::int16_t y1{0};
    std::int16_t x2{0};
    std::int16_t y2{0};

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return x1 >= 0 && y1 >= 0 && x2 >= x1 && y2 >= y1;
    }

    [[nodiscard]] constexpr std::int16_t width_minus_1() const noexcept {
        return static_cast<std::int16_t>(x2 - x1);
    }

    [[nodiscard]] constexpr std::int16_t height_minus_1() const noexcept {
        return static_cast<std::int16_t>(y2 - y1);
    }

    [[nodiscard]] constexpr std::uint16_t width() const noexcept {
        return is_valid() ? static_cast<std::uint16_t>(x2 - x1 + 1) : 0U;
    }

    [[nodiscard]] constexpr std::uint16_t height() const noexcept {
        return is_valid() ? static_cast<std::uint16_t>(y2 - y1 + 1) : 0U;
    }

    [[nodiscard]] constexpr bool operator==(const TilemapRect&) const noexcept = default;
};

/// Destination upper-left tile coordinate (dst_x, dst_y).
struct TilemapPoint {
    std::int16_t x{0};
    std::int16_t y{0};

    [[nodiscard]] constexpr bool is_non_negative() const noexcept {
        return x >= 0 && y >= 0;
    }

    [[nodiscard]] constexpr bool operator==(const TilemapPoint&) const noexcept = default;
};

/// Register contract forwarded to 0x00DC26 (vdp_sync_dirty_plane_rect) at 0x00DBC0:
/// a3 = plane context ($FF1716/$FF173E), a1 = destination top-left cell byte pointer,
/// d1 = dst_x, d2 = dst_y, d3 = width_minus_1, d4 = height_minus_1.
struct TilemapRectSyncRequest {
    bool is_plane_b{false};
    std::uint32_t plane_ram_offset{kPlaneARamOffset};
    std::int16_t dst_x{0};
    std::int16_t dst_y{0};
    std::int16_t width_minus_1{0};
    std::int16_t height_minus_1{0};
    std::size_t dst_byte_offset{0U};
    std::span<const std::uint8_t> dst_tilemap_tail{};

    [[nodiscard]] constexpr std::uint16_t width() const noexcept {
        return (width_minus_1 >= 0) ? static_cast<std::uint16_t>(width_minus_1 + 1) : 0U;
    }

    [[nodiscard]] constexpr std::uint16_t height() const noexcept {
        return (height_minus_1 >= 0) ? static_cast<std::uint16_t>(height_minus_1 + 1) : 0U;
    }
};

/// Optional callback modeling `bsr.w $dc26` (vdp_sync_dirty_plane_rect) at 0x00DBC0.
using DirtyRectSyncCallback = std::function<void(const TilemapRectSyncRequest&)>;

/// Parses BE16 4-word source rect (x1, y1, x2, y2) and 2-word destination point (dst_x, dst_y)
/// matching `movem.w (a0)+, d1-d4` and `movem.w (a1)+, d1-d2` in 0x00DB64 / 0x00DBCA.
[[nodiscard]] bool parse_tilemap_rect_copy_args(
    std::span<const std::uint8_t> src_rect_be16,
    std::span<const std::uint8_t> dst_point_be16,
    TilemapRect& out_src_rect,
    TilemapPoint& out_dst_point) noexcept;

/// 0x00DB1A..0x00DB62: Fills inclusive rect (x1..x2, y1..y2) in a 1B/tile auxiliary map
/// using `(y1 << word_stride_shift) + x1` start offset and `row_stride_bytes` row step.
/// Returns cells filled (0 if rect is invalid or out of bounds).
std::size_t tilemap_fill_rect_byte(
    std::span<std::uint8_t> aux_map,
    std::uint16_t word_stride_shift,
    std::uint16_t row_stride_bytes,
    const TilemapRect& rect,
    std::uint8_t fill_val) noexcept;

/// 0x00DBCA..0x00DC24: Copies inclusive rect (x1..x2, y1..y2) to `dst` within a 1B/tile
/// auxiliary map (forward row-major order matching `move.b (a0)+, (a1)+`).
/// Returns cells copied (0 if rect/dst is invalid or out of bounds).
std::size_t tilemap_copy_rect_byte(
    std::span<std::uint8_t> aux_map,
    std::uint16_t word_stride_shift,
    std::uint16_t row_stride_bytes,
    const TilemapRect& src_rect,
    TilemapPoint dst) noexcept;

/// Separate source/destination span variant of 0x00DBCA..0x00DC24.
std::size_t tilemap_copy_rect_byte(
    std::span<const std::uint8_t> src_aux_map,
    std::span<std::uint8_t> dst_aux_map,
    std::uint16_t word_stride_shift,
    std::uint16_t row_stride_bytes,
    const TilemapRect& src_rect,
    TilemapPoint dst) noexcept;

/// Cleanroom 16-bit word tilemap rectangular copy helper (native `uint16_t` cells).
std::size_t tilemap_copy_rect_word(
    std::span<std::uint16_t> tilemap_words,
    std::uint16_t word_stride_shift,
    std::uint16_t width_tiles,
    const TilemapRect& src_rect,
    TilemapPoint dst) noexcept;

/// 0x00DB68..0x00DBC8: Copies inclusive word tilemap rect inside BE16 byte-addressed
/// buffer (`(y << byte_stride_shift) + 2 * x`, stride `width_tiles * 2` bytes) and invokes
/// optional `sync_callback` (`0x00DC26`). Returns word cells copied (0 if invalid or OOB).
std::size_t tilemap_copy_rect_word_only_and_sync(
    std::span<std::uint8_t> tilemap_bytes,
    std::uint16_t byte_stride_shift,
    std::uint16_t width_tiles,
    const TilemapRect& src_rect,
    TilemapPoint dst,
    bool is_plane_b = false,
    const DirtyRectSyncCallback& sync_callback = {});

/// 0x00DB64..0x00DBC8: Composite transfer — first copies auxiliary byte rect (0x00DBCA),
/// then copies BE16 word tilemap rect (0x00DB68), then invokes optional `sync_callback` (0x00DC26).
/// Returns word cells copied (0 if either aux or word transfer is invalid or OOB).
std::size_t tilemap_copy_rect_word_and_sync(
    std::span<std::uint8_t> tilemap_bytes,
    std::span<std::uint8_t> aux_map,
    std::uint16_t byte_stride_shift,
    std::uint16_t width_tiles,
    std::uint16_t word_stride_shift,
    const TilemapRect& src_rect,
    TilemapPoint dst,
    bool is_plane_b = false,
    const DirtyRectSyncCallback& sync_callback = {});

/// 64KB RAM window helper for 0x00DB1A: selects $FF1766 / $FF1770 via $FF16F0 bit 7.
std::size_t tilemap_fill_rect_byte_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    const TilemapRect& rect,
    std::uint8_t fill_val) noexcept;

/// 64KB RAM window helper for 0x00DBCA: selects $FF1766 (`!is_plane_b`) or $FF1770 (`is_plane_b`).
std::size_t tilemap_copy_rect_byte_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    bool is_plane_b,
    const TilemapRect& src_rect,
    TilemapPoint dst) noexcept;

/// 64KB RAM window helper for 0x00DB68: word tilemap copy + optional 0x00DC26 sync hook.
std::size_t tilemap_copy_rect_word_only_and_sync_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    bool is_plane_b,
    const TilemapRect& src_rect,
    TilemapPoint dst,
    const DirtyRectSyncCallback& sync_callback = {});

/// 64KB RAM window helper for 0x00DB64: aux byte rect copy + word rect copy + 0x00DC26 sync hook.
std::size_t tilemap_copy_rect_word_and_sync_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    bool is_plane_b,
    const TilemapRect& src_rect,
    TilemapPoint dst,
    const DirtyRectSyncCallback& sync_callback = {});

} // namespace thor::render

namespace oasis::game::render {
using namespace thor::render;
} // namespace oasis::game::render
