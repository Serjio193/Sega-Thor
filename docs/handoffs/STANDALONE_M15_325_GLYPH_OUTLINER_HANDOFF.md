# Standalone Handoff — M15.325: 1-bpp Dialogue Glyph Outliner & 4-Tile VDP Rasterizer (`0x00C222..0x00C2EA`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x00C222..0x00C2A4`: `rasterize_outlined_glyph(glyph_id)`
  - `0x00C2A6..0x00C2C6`: `pack_8x8_4bpp_tile`
- **Evidence JSON:** `data/thor-evidence/rom_glyph_outliner_m15_325.json`
- **Python Extractor & Tests:** `tools/extract_rom_glyph_outliner.py`, `tests/rom_glyph_outliner_test.py`

## Behavior
1. Reads 14 `uint16_be` rows (`28` bytes) from `0x0016C1CA + glyph_id * 28`.
2. Unpacks into an `18x16` byte canvas (`1`px margin on all sides; `0xFF` for set bit, `0x00` for clear).
3. 4-neighbor outline pass: empty (`0x00`) cells adjacent orthogonally (`±1`, `±18`) to `0xFF` (`bit 7` set) become `0x0E` (color 14).
4. Packs the `16x16` interior into four `8x8` 4-bpp Mega Drive tiles (`128` bytes total) in column-major order: TL (`0x13`), BL (`0xA3`), TR (`0x1B`), BR (`0xAB`), where `0xFF & 0x0F = 15` (foreground) and `0x0E & 0x0F = 14` (outline).
