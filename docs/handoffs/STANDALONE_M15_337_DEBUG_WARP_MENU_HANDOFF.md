# Standalone Handoff — M15.337: Debug Stage/Room Warp Menu & Spawn Table (`0x004C9C..0x004F20`, `0x005D918`)

## Summary
- **Source routine in Beyond Oasis (USA):** `0x004C9C..0x004F20`
- **Spawn Table:** `0x0005D918` (32 area slots -> room offset list -> `(tile_x, tile_y)` shifted `<< 3` to pixel coordinates)
- **Evidence JSON:** `data/thor-evidence/rom_debug_warp_menu_m15_337.json`
- **Python Extractor & Tests:** `tools/extract_rom_debug_warp_menu.py`, `tests/rom_debug_warp_menu_test.py`
