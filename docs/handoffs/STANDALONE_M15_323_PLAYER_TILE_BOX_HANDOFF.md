# Standalone Handoff — M15.323: Player Tile-Box Containment & Overlap Checks (`0x00C17A..0x00C220`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x00C17A..0x00C1CC`: `player_inside_tile_box(tx1, ty1, tx2, ty2)` -> `carry = 1` if Ali's AABB `[x-r, x+r] x [y-r, y+r]` is fully inside `[tx1*8, tx2*8+7] x [ty1*8, ty2*8+7]`.
  - `0x00C1CE..0x00C220`: `player_overlaps_tile_box(tx1, ty1, tx2, ty2)` -> `carry = 1` if Ali's AABB overlaps `[tx1*8, tx2*8+7] x [ty1*8, ty2*8+7]`.
- **Evidence JSON:** `data/thor-evidence/rom_player_tile_box_m15_323.json`
- **Python Extractor & Tests:** `tools/extract_rom_player_tile_box.py`, `tests/rom_player_tile_box_test.py`

## Entity Fields (`0x00FF19E8`)
- `0x08`: `x` (int16)
- `0x0C`: `y` (int16)
- `0x46`: `half_extent` `r` (int16)
