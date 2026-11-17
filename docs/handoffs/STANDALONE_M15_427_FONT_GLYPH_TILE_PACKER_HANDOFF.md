# STANDALONE HANDOFF: ROM Dynamic 1-Bit Font Glyph Expansion, 4-Neighbor Outline Filter & 4bpp Tile Packer (0x00C222..0x00C2EC)

## 1. Routine Overview
- **ROM Range:** `0x00C222..0x00C2EC` (202 bytes).
- **Function:** Real-time Font Glyph Rasterizer & Outline Generator. Expands 14-word 1-bit font glyphs from ROM `0x0016C1CA` into an 18x16 byte-per-pixel canvas at `$FF3028`, applies a 4-neighbor morphological outline/shadow filter with color `0x0E`, and packs the resulting 16x16 glyph into four 8x8 Sega Genesis 4bpp tiles (128 bytes) at `$FF2FA8` ready for direct VRAM DMA streaming.
- **Evidence JSON:** `data/thor-evidence/rom_font_glyph_tile_packer_m15_427.json`.

## 2. Ingress Entry Point
- Address: `0x00C222` (`render_font_glyph_to_vram_buffer`).
- Direct call site: `0x03B572` (in the dialogue / text string streamer subsystem).
- Input: `d0` (glyph index).

## 3. Algorithm & Logic
1. **Glyph Offset Resolution:**
   - Glyph descriptor table in ROM: `0x0016C1CA`.
   - Each glyph record is 28 bytes (14 words of 16-bit scanlines).
   - `offset = glyph_index * 28` (`mulu.w #$1C, d0`).
2. **1-Bit to Byte Expansion onto Padded Canvas (`$FF3028`):**
   - Canvas pitch: 18 bytes (1 leading 0-pixel border, 16 glyph pixels, 1 trailing 0-pixel border).
   - Writes top border row of 18 zeroes via `bsr.w $C2C8`.
   - For each of 14 words:
     * Writes leading zero byte `sf.b (a1)+`.
     * Loops 16 times shifting bits (`lsl.w #1, d1; subx.b d0, d0; move.b d0, (a1)+`).
       If bit is 1, emits `0xFF` (foreground pixel); if 0, emits `0x00` (background pixel).
     * Writes trailing zero byte `sf.b (a1)+`.
   - Writes bottom border row of 18 zeroes.
3. **4-Neighbor Cardinal Outline Filter (`0x00C248`):**
   - Iterates across canvas pixels starting at row 1, col 0 (`$FF303A`):
     * If pixel is foreground (`0xFF`): skip.
     * If pixel is background (`0x00`):
       Computes 4 cardinal neighbors with pitch 18:
       - Top neighbor: `curr - 18`
       - Bottom neighbor: `curr + 18`
       - Left neighbor: `curr - 1`
       - Right neighbor: `curr + 1`
       If any neighbor has sign bit set (`(top | bottom | left | right) & 0x80 != 0`):
       Writes outline color `#$0E` into current pixel!
4. **4bpp Sega Genesis Tile Packing (`0x00C276`):**
   - Extracts four 8x8 quadrants:
     * Tile 0 (Top-Left): canvas row 1, col 1 (`$FF303B`)
     * Tile 1 (Bottom-Left): canvas row 9, col 1 (`$FF30CB`)
     * Tile 2 (Top-Right): canvas row 1, col 9 (`$FF3043`)
     * Tile 3 (Bottom-Right): canvas row 9, col 9 (`$FF30D3`)
   - Packing loop `0x00C2A6`:
     * 8 rows per tile (`d3 = 7`).
     * 4 bytes per row (`d4 = 3`): reads 2 pixels, packs 2 nibbles into 1 byte (`(p0 << 4) | (p1 & 0x0F)`).
     * Skips to next canvas row: `lea $0A(a0), a0` (8 pixels read + 10 stride offset = 18 bytes pitch).
   - Output buffer: `$FF2FA8` (4 tiles * 32 bytes = 128 bytes total).

## 4. Recommended C++20 Interface
```cpp
namespace thor::font {

struct FontGlyphTileData {
    static constexpr size_t kTileBytes = 32;
    static constexpr size_t kTotalBytes = 128; // 4 tiles
    std::array<uint8_t, kTotalBytes> vram_tile_bytes{};
};

class FontRasterizer {
public:
    static constexpr uint8_t kOutlineColor = 0x0E;
    static constexpr uint8_t kForegroundNibble = 0x0F;

    FontGlyphTileData render_glyph(std::span<const uint16_t, 14> glyph_scanlines);
};

} // namespace thor::font
```
