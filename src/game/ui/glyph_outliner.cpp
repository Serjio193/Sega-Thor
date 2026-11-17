#include "game/ui/glyph_outliner.hpp"

namespace oasis::game::ui {

namespace {

void pack_8x8_4bpp_tile(
    const std::array<std::uint8_t, kCanvasTotalBytes>& canvas,
    std::size_t start_offset,
    std::span<std::uint8_t, 32> out_tile) noexcept {
    std::size_t dst = 0U;
    for (std::size_t row = 0U; row < 8U; ++row) {
        const std::size_t row_off = start_offset + row * kCanvasStrideBytes;
        for (std::size_t pair = 0U; pair < 4U; ++pair) {
            const std::uint8_t hi = canvas[row_off + pair * 2U] & 0x0FU;
            const std::uint8_t lo = canvas[row_off + pair * 2U + 1U] & 0x0FU;
            out_tile[dst++] = static_cast<std::uint8_t>((hi << 4U) | lo);
        }
    }
}

} // namespace

std::array<std::uint8_t, 128> rasterize_outlined_glyph(
    std::span<const std::uint16_t, kGlyphRows> rows) noexcept {
    std::array<std::uint8_t, kCanvasTotalBytes> canvas{};

    // Unpack 14 rows starting at (row=1, col=1) -> offset 0x13 (19)
    for (std::size_t r = 0U; r < kGlyphRows; ++r) {
        const std::uint16_t bits = rows[r];
        const std::size_t row_off = (r + 1U) * kCanvasStrideBytes + 1U;
        for (std::size_t c = 0U; c < 16U; ++c) {
            const bool set = ((bits >> (15U - c)) & 1U) != 0U;
            canvas[row_off + c] = set ? kGlyphForegroundByte : kGlyphBackgroundByte;
        }
    }

    // 4-neighbor outline pass: empty cells orthogonally adjacent to bit-7 set become 0x0E
    for (std::size_t r = 1U; r <= kGlyphRows + 1U; ++r) {
        for (std::size_t c = 1U; c <= 16U; ++c) {
            const std::size_t idx = r * kCanvasStrideBytes + c;
            if (canvas[idx] != kGlyphBackgroundByte) {
                continue;
            }
            const bool has_fg_neighbor =
                ((canvas[idx - 1U] & 0x80U) != 0U) ||
                ((canvas[idx + 1U] & 0x80U) != 0U) ||
                ((canvas[idx - kCanvasStrideBytes] & 0x80U) != 0U) ||
                ((canvas[idx + kCanvasStrideBytes] & 0x80U) != 0U);
            if (has_fg_neighbor) {
                canvas[idx] = kGlyphOutlineByte;
            }
        }
    }

    // Pack four 8x8 4-bpp tiles in column-major order: TL, BL, TR, BR
    std::array<std::uint8_t, 128> out{};
    for (std::size_t t = 0U; t < 4U; ++t) {
        pack_8x8_4bpp_tile(
            canvas,
            kTileOffsets4bpp[t],
            std::span<std::uint8_t, 32>(out.data() + t * 32U, 32U));
    }
    return out;
}

} // namespace oasis::game::ui
