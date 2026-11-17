# Standalone Handoff — M15.328: Ring Menu 4x2 Cursor Wrap & Item Drop Handler (`0x00462E..0x0046E2`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x00462E..0x004670`: `step_cursor_4x2` (4 cols x 2 rows, `(row << 2) | col` -> `$FF0E50`, SFX `0x006003A6`)
  - `0x004674..0x0046E2`: `try_drop_inventory_item` (guards against `cursor == $FF0DB0`, `0x009C40` terrain bit 7, and `item_id <= 1`; clears slot ID and emits `(item_id | 0x80, qty)` to `$FF1864`/`$FF1865`)
- **Evidence JSON:** `data/thor-evidence/rom_ring_menu_drop_m15_328.json`
- **Python Extractor & Tests:** `tools/extract_rom_ring_menu_drop.py`, `tests/rom_ring_menu_drop_test.py`
