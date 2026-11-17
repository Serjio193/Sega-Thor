# Standalone Handoff — M15.316: Room & Area Transition Loader (`0x00C8AA..0x00C96B`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x00C8AA..0x00C90C`: `load_room_and_jump(uint16 room_id)` (`(area << 8) | room`)
  - `0x00C90E..0x00C92A`: `get_room_header(uint16 room_id)` -> ROM header address
  - `0x00C92C..0x00C96B`: 16-entry `uint32` Area Table (`Area 0..15`)
- **Evidence JSON:** `data/thor-evidence/rom_room_area_loader_m15_316.json`
- **Python Extractor & Tests:** `tools/extract_rom_room_area_loader.py`, `tests/rom_room_area_loader_test.py`

## Behavior
1. On room transition (`new_room_id != current_room_id` at `$FF0E40`):
   - Clear 16 bytes of per-room flags (`$FF0E2C..$FF0E3B`).
   - If `(new_room_id >> 8) != (old_room_id >> 8)`, also clear 16 bytes of per-area flags (`$FF0E1C..$FF0E2B`) and 3 bytes at `$FF0DB3..$FF0DB5`.
2. Store `new_room_id` into `$FF0E40`.
3. Resolve room header:
   - `table_base = area_table[area & 0x0F]`
   - `entry_addr = table_base + (room * 2)`
   - `header_addr = entry_addr + int16_be(rom[entry_addr])`
4. Store `header_addr` into `$FF16EC`, and enter room script at `header_addr + 0x1A`.
