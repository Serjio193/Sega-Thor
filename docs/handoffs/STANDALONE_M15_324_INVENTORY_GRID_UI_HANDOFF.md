# Standalone Handoff — M15.324: Inventory/Ring 4x4 Grid UI Layout (`0x0042A6..0x004360`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x0042A6..0x0042D0`: `grid_slot_to_xy(slot)` -> `(0x70 + (slot & 3) * 24, 0x40 + ((slot & 0xC) >> 2) * 24)`
  - `0x0042E8..0x004360`: `render_inventory_grid` (8 slots from `$FF0D80`, 9 tiles/icon from `0x155D76`, plus spirit `$FF0E50` & special `$FF0DB0` in `$FF2FA8`)
- **Evidence JSON:** `data/thor-evidence/rom_inventory_grid_ui_m15_324.json`
- **Python Extractor & Tests:** `tools/extract_rom_inventory_grid_ui.py`, `tests/rom_inventory_grid_ui_test.py`
