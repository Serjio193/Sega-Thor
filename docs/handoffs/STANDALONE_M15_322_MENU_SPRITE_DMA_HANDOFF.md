# Standalone Handoff — M15.322: Inventory/Menu Sprite & Font DMA Strip Emitter (`0x004000..0x00417A`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x004026..0x00405C`: `emit_item_and_digit_pair` (`0x86DD` icon + `0x85D0 + min(qty, 9)` digit at `Y + 16`)
  - `0x004066..0x0040A4`: `append_sat_sprite` (caps at `0x50` = 80 sprites, `+0x80` screen-coordinate bias)
  - `0x0040A6..0x0040CC`: `queue_sprite_tile_dma` (uses 16-byte LUT `0x0040CE`)
  - `0x0040E2..0x004156`: `emit_font_strip_sprites` (chunks into `<= 4`-tile horizontal sprites, `0x152F5A` / `0x15375A` font, remaps `0x2B..0x2F` by `+0x30`)
  - `0x004158..0x00417A`: `acquire_sat_and_dma_cursors`
- **Evidence JSON:** `data/thor-evidence/rom_menu_sprite_dma_m15_322.json`
- **Python Extractor & Tests:** `tools/extract_rom_menu_sprite_dma.py`, `tests/rom_menu_sprite_dma_test.py`

## LUT (`0x0040CE`)
`{1, 2, 3, 4, 2, 4, 6, 8, 3, 6, 9, 12, 4, 8, 12, 16}` (tile count for Mega Drive sprite size code `0..15`).
