#include "game/render/tilemap_rect_ops.hpp"

#include <algorithm>
#include <optional>

namespace thor::render {

namespace {

[[nodiscard]] std::uint16_t read_be16(
    std::span<const std::uint8_t> data,
    std::size_t offset) noexcept {
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(data[offset]) << 8U) |
        static_cast<std::uint16_t>(data[offset + 1U]));
}

[[nodiscard]] std::uint32_t read_be32(
    std::span<const std::uint8_t> data,
    std::size_t offset) noexcept {
    return (static_cast<std::uint32_t>(data[offset]) << 24U) |
           (static_cast<std::uint32_t>(data[offset + 1U]) << 16U) |
           (static_cast<std::uint32_t>(data[offset + 2U]) << 8U) |
           static_cast<std::uint32_t>(data[offset + 3U]);
}

[[nodiscard]] std::optional<std::size_t> compute_rect_start(
    std::int16_t x,
    std::int16_t y,
    std::uint16_t shift,
    std::size_t elem_bytes,
    std::uint16_t width_cells,
    std::uint16_t height_cells,
    std::size_t row_stride_bytes,
    std::size_t buffer_bytes) noexcept {
    if (x < 0 || y < 0 || shift >= 16U || width_cells == 0U || height_cells == 0U ||
        row_stride_bytes == 0U) {
        return std::nullopt;
    }

    const std::uint64_t row_bytes = static_cast<std::uint64_t>(width_cells) * elem_bytes;
    const std::uint64_t start =
        (static_cast<std::uint64_t>(y) << shift) + static_cast<std::uint64_t>(x) * elem_bytes;
    const std::uint64_t last_row =
        start + static_cast<std::uint64_t>(height_cells - 1U) * row_stride_bytes;
    const std::uint64_t end = last_row + row_bytes;
    if (end > buffer_bytes) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(start);
}

[[nodiscard]] std::optional<std::size_t> ram_addr_to_offset(std::uint32_t addr) noexcept {
    if ((addr & 0xFFFF0000U) != TilemapRectOpsConfig::kMegaDriveRamBase) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(addr & 0xFFFFU);
}

} // namespace

bool parse_tilemap_rect_copy_args(
    std::span<const std::uint8_t> src_rect_be16,
    std::span<const std::uint8_t> dst_point_be16,
    TilemapRect& out_src_rect,
    TilemapPoint& out_dst_point) noexcept {
    if (src_rect_be16.size() < 8U || dst_point_be16.size() < 4U) {
        return false;
    }
    out_src_rect.x1 = static_cast<std::int16_t>(read_be16(src_rect_be16, 0U));
    out_src_rect.y1 = static_cast<std::int16_t>(read_be16(src_rect_be16, 2U));
    out_src_rect.x2 = static_cast<std::int16_t>(read_be16(src_rect_be16, 4U));
    out_src_rect.y2 = static_cast<std::int16_t>(read_be16(src_rect_be16, 6U));
    out_dst_point.x = static_cast<std::int16_t>(read_be16(dst_point_be16, 0U));
    out_dst_point.y = static_cast<std::int16_t>(read_be16(dst_point_be16, 2U));
    return true;
}

std::size_t tilemap_fill_rect_byte(
    std::span<std::uint8_t> aux_map,
    std::uint16_t word_stride_shift,
    std::uint16_t row_stride_bytes,
    const TilemapRect& rect,
    std::uint8_t fill_val) noexcept {
    if (!rect.is_valid()) {
        return 0U;
    }

    const std::uint16_t width = rect.width();
    const std::uint16_t height = rect.height();
    const auto start = compute_rect_start(
        rect.x1, rect.y1, word_stride_shift, 1U, width, height, row_stride_bytes, aux_map.size());
    if (!start.has_value()) {
        return 0U;
    }

    // 0x00DB4A..0x00DB5C: fill (width_minus_1 + 1) cols across (height_minus_1 + 1) rows
    std::size_t row_off = *start;
    for (std::uint16_t row = 0U; row < height; ++row) {
        std::fill_n(aux_map.begin() + static_cast<std::ptrdiff_t>(row_off), width, fill_val);
        row_off += row_stride_bytes;
    }
    return static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
}

std::size_t tilemap_copy_rect_byte(
    std::span<const std::uint8_t> src_aux_map,
    std::span<std::uint8_t> dst_aux_map,
    std::uint16_t word_stride_shift,
    std::uint16_t row_stride_bytes,
    const TilemapRect& src_rect,
    TilemapPoint dst) noexcept {
    if (!src_rect.is_valid() || !dst.is_non_negative()) {
        return 0U;
    }

    const std::uint16_t width = src_rect.width();
    const std::uint16_t height = src_rect.height();
    const auto src_start = compute_rect_start(
        src_rect.x1, src_rect.y1, word_stride_shift, 1U, width, height, row_stride_bytes, src_aux_map.size());
    const auto dst_start = compute_rect_start(
        dst.x, dst.y, word_stride_shift, 1U, width, height, row_stride_bytes, dst_aux_map.size());
    if (!src_start.has_value() || !dst_start.has_value()) {
        return 0U;
    }

    // 0x00DC0A..0x00DC1E: explicit forward byte loop matching `move.b (a0)+, (a1)+`
    std::size_t src_row = *src_start;
    std::size_t dst_row = *dst_start;
    for (std::uint16_t row = 0U; row < height; ++row) {
        for (std::uint16_t col = 0U; col < width; ++col) {
            dst_aux_map[dst_row + col] = src_aux_map[src_row + col];
        }
        src_row += row_stride_bytes;
        dst_row += row_stride_bytes;
    }
    return static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
}

std::size_t tilemap_copy_rect_byte(
    std::span<std::uint8_t> aux_map,
    std::uint16_t word_stride_shift,
    std::uint16_t row_stride_bytes,
    const TilemapRect& src_rect,
    TilemapPoint dst) noexcept {
    return tilemap_copy_rect_byte(
        std::span<const std::uint8_t>(aux_map), aux_map, word_stride_shift, row_stride_bytes, src_rect, dst);
}

std::size_t tilemap_copy_rect_word(
    std::span<std::uint16_t> tilemap_words,
    std::uint16_t word_stride_shift,
    std::uint16_t width_tiles,
    const TilemapRect& src_rect,
    TilemapPoint dst) noexcept {
    if (!src_rect.is_valid() || !dst.is_non_negative()) {
        return 0U;
    }

    const std::uint16_t width = src_rect.width();
    const std::uint16_t height = src_rect.height();
    const auto src_start = compute_rect_start(
        src_rect.x1, src_rect.y1, word_stride_shift, 1U, width, height, width_tiles, tilemap_words.size());
    const auto dst_start = compute_rect_start(
        dst.x, dst.y, word_stride_shift, 1U, width, height, width_tiles, tilemap_words.size());
    if (!src_start.has_value() || !dst_start.has_value()) {
        return 0U;
    }

    std::size_t src_row = *src_start;
    std::size_t dst_row = *dst_start;
    for (std::uint16_t row = 0U; row < height; ++row) {
        for (std::uint16_t col = 0U; col < width; ++col) {
            tilemap_words[dst_row + col] = tilemap_words[src_row + col];
        }
        src_row += width_tiles;
        dst_row += width_tiles;
    }
    return static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
}

std::size_t tilemap_copy_rect_word_only_and_sync(
    std::span<std::uint8_t> tilemap_bytes,
    std::uint16_t byte_stride_shift,
    std::uint16_t width_tiles,
    const TilemapRect& src_rect,
    TilemapPoint dst,
    bool is_plane_b,
    const DirtyRectSyncCallback& sync_callback) {
    if (!src_rect.is_valid() || !dst.is_non_negative()) {
        return 0U;
    }

    const std::uint16_t width = src_rect.width();
    const std::uint16_t height = src_rect.height();
    // 0x00DB70..0x00DB74: row byte stride = width_tiles * 2
    const std::size_t row_stride_bytes = static_cast<std::size_t>(width_tiles) * 2U;
    const auto src_start = compute_rect_start(
        src_rect.x1, src_rect.y1, byte_stride_shift, 2U, width, height, row_stride_bytes, tilemap_bytes.size());
    const auto dst_start = compute_rect_start(
        dst.x, dst.y, byte_stride_shift, 2U, width, height, row_stride_bytes, tilemap_bytes.size());
    if (!src_start.has_value() || !dst_start.has_value()) {
        return 0U;
    }

    // 0x00DBA2..0x00DBB6: copy (width_minus_1 + 1) words across (height_minus_1 + 1) rows
    const std::size_t row_copy_bytes = static_cast<std::size_t>(width) * 2U;
    std::size_t src_row = *src_start;
    std::size_t dst_row = *dst_start;
    for (std::uint16_t row = 0U; row < height; ++row) {
        for (std::size_t b = 0U; b < row_copy_bytes; b += 2U) {
            const std::uint8_t hi = tilemap_bytes[src_row + b];
            const std::uint8_t lo = tilemap_bytes[src_row + b + 1U];
            tilemap_bytes[dst_row + b] = hi;
            tilemap_bytes[dst_row + b + 1U] = lo;
        }
        src_row += row_stride_bytes;
        dst_row += row_stride_bytes;
    }

    // 0x00DBB8..0x00DBC0: invoke 0x00DC26 (vdp_sync_dirty_plane_rect) with dst rect metadata
    if (sync_callback) {
        TilemapRectSyncRequest req{};
        req.is_plane_b = is_plane_b;
        req.plane_ram_offset = is_plane_b
            ? TilemapRectOpsConfig::kPlaneBContextRamOffset
            : TilemapRectOpsConfig::kPlaneAContextRamOffset;
        req.dst_x = dst.x;
        req.dst_y = dst.y;
        req.width_minus_1 = src_rect.width_minus_1();
        req.height_minus_1 = src_rect.height_minus_1();
        req.dst_byte_offset = *dst_start;
        req.dst_tilemap_tail = std::span<const std::uint8_t>(tilemap_bytes).subspan(*dst_start);
        sync_callback(req);
    }

    return static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
}

std::size_t tilemap_copy_rect_word_and_sync(
    std::span<std::uint8_t> tilemap_bytes,
    std::span<std::uint8_t> aux_map,
    std::uint16_t byte_stride_shift,
    std::uint16_t width_tiles,
    std::uint16_t word_stride_shift,
    const TilemapRect& src_rect,
    TilemapPoint dst,
    bool is_plane_b,
    const DirtyRectSyncCallback& sync_callback) {
    // 0x00DB64: bsr.w $dbca (aux byte rect copy) before word tilemap rect copy + sync
    const std::size_t aux_copied = tilemap_copy_rect_byte(
        aux_map, word_stride_shift, width_tiles, src_rect, dst);
    if (aux_copied == 0U) {
        return 0U;
    }

    return tilemap_copy_rect_word_only_and_sync(
        tilemap_bytes, byte_stride_shift, width_tiles, src_rect, dst, is_plane_b, sync_callback);
}

std::size_t tilemap_fill_rect_byte_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    const TilemapRect& rect,
    std::uint8_t fill_val) noexcept {
    // 0x00DB1E..0x00DB32: $FF16F0 bit 7 selects $FF1766 (Plane A aux) vs $FF1770 (Plane B aux)
    const bool use_plane_b =
        (ram[TilemapRectOpsConfig::kScrollModeRamOffset] &
         TilemapRectOpsConfig::kScrollModeSelectPlaneBBit) != 0U;
    const std::size_t aux_ctx_off = use_plane_b
        ? TilemapRectOpsConfig::kPlaneBAuxRamOffset
        : TilemapRectOpsConfig::kPlaneAAuxRamOffset;

    const auto aux_data_off = ram_addr_to_offset(read_be32(ram, aux_ctx_off + 0x00U));
    if (!aux_data_off.has_value()) {
        return 0U;
    }
    const std::uint16_t width_tiles = read_be16(ram, aux_ctx_off + 0x04U);
    const std::uint16_t word_stride_shift = read_be16(ram, aux_ctx_off + 0x08U);

    return tilemap_fill_rect_byte(
        ram.subspan(*aux_data_off), word_stride_shift, width_tiles, rect, fill_val);
}

std::size_t tilemap_copy_rect_byte_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    bool is_plane_b,
    const TilemapRect& src_rect,
    TilemapPoint dst) noexcept {
    // 0x00DBCE..0x00DBE0: a3 == $FF1716 selects $FF1766, else $FF1770
    const std::size_t aux_ctx_off = is_plane_b
        ? TilemapRectOpsConfig::kPlaneBAuxRamOffset
        : TilemapRectOpsConfig::kPlaneAAuxRamOffset;

    const auto aux_data_off = ram_addr_to_offset(read_be32(ram, aux_ctx_off + 0x00U));
    if (!aux_data_off.has_value()) {
        return 0U;
    }
    const std::uint16_t width_tiles = read_be16(ram, aux_ctx_off + 0x04U);
    const std::uint16_t word_stride_shift = read_be16(ram, aux_ctx_off + 0x08U);

    return tilemap_copy_rect_byte(
        ram.subspan(*aux_data_off), word_stride_shift, width_tiles, src_rect, dst);
}

std::size_t tilemap_copy_rect_word_only_and_sync_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    bool is_plane_b,
    const TilemapRect& src_rect,
    TilemapPoint dst,
    const DirtyRectSyncCallback& sync_callback) {
    const std::size_t plane_ctx_off = is_plane_b
        ? TilemapRectOpsConfig::kPlaneBContextRamOffset
        : TilemapRectOpsConfig::kPlaneAContextRamOffset;

    const auto tile_data_off = ram_addr_to_offset(read_be32(ram, plane_ctx_off + 0x16U));
    if (!tile_data_off.has_value()) {
        return 0U;
    }
    const std::uint16_t width_tiles = read_be16(ram, plane_ctx_off + 0x20U);
    const std::uint16_t byte_stride_shift = read_be16(ram, plane_ctx_off + 0x24U);

    return tilemap_copy_rect_word_only_and_sync(
        ram.subspan(*tile_data_off),
        byte_stride_shift,
        width_tiles,
        src_rect,
        dst,
        is_plane_b,
        sync_callback);
}

std::size_t tilemap_copy_rect_word_and_sync_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    bool is_plane_b,
    const TilemapRect& src_rect,
    TilemapPoint dst,
    const DirtyRectSyncCallback& sync_callback) {
    if (tilemap_copy_rect_byte_from_ram(ram, is_plane_b, src_rect, dst) == 0U) {
        return 0U;
    }
    return tilemap_copy_rect_word_only_and_sync_from_ram(
        ram, is_plane_b, src_rect, dst, sync_callback);
}

} // namespace thor::render
