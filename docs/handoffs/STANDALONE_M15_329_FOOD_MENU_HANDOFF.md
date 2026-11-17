# Standalone Handoff — M15.329: 4x4 Food/Consumable Menu Use & Drop (`0x00474E..0x00494A`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x00474E..0x004828`: `food_menu_loop` (`$FF0E51` 4x4 cursor, `$FF1996` A/BC swap)
  - `0x00482A..0x004894`: `use_food_item` (`$FF0D90`, `0x005236` check, SFX `0x00A00256`, `90`-frame timer `$FF1856 = 0x5A`)
  - `0x004896..0x004904`: `drop_food_item` (checks `0x009C40` terrain bit 7 and `0x05CE16[id]` bit 5 undroppable)
  - `0x004906..0x00494A`: `step_cursor_4x4`
- **Evidence JSON:** `data/thor-evidence/rom_food_menu_m15_329.json`
- **Python Extractor & Tests:** `tools/extract_rom_food_menu.py`, `tests/rom_food_menu_test.py`
