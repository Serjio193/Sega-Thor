# Standalone Handoff — M15.314: Terrain Elevation & Ledge Probe (`0x00938E..0x0096F7`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x00938E..0x00945E`: `probe_terrain_x` (sets bit `5`/`6` of `0x40(a6)`)
  - `0x009460..0x0094D0`: `scan_col_with_ledge` & `scan_col_span` (`[y - r .. y + r] >> 3`)
  - `0x0094D2..0x0095A8`: `probe_terrain_y_up` (sets bit `5`/`6` of `0x41(a6)`)
  - `0x0095AA..0x009676`: `probe_terrain_y_down` (sets bit `5`/`6` of `0x41(a6)`)
  - `0x009678..0x0096E6`: `scan_row_with_ledge` & `scan_row_span` (`[x - r .. x + r] >> 3`)
  - `0x0096E8..0x0096F7`: 16-byte Gray-code elevation nibble LUT
- **Evidence JSON:** `data/thor-evidence/rom_terrain_elevation_probe_m15_314.json`
- **Python Extractor & Tests:** `tools/extract_rom_terrain_elevation_probe.py`, `tests/rom_terrain_elevation_probe_test.py`

## Elevation Nibble LUT (`0x0096E8`)
`{-1, 0, 2, 1, 4, -1, 3, -1, 6, 7, -1, -1, 5, -1, -1, -1}`

## Decision Core
1. Bypass if `entity_flags & 0x01` or `entity_elev < 0` -> passable (`carry = 0`).
2. Aggregate tile span (`OR` -> `or_bits`, `AND` -> `and_bits`).
3. `tile_elev = LUT[and_bits & 0x0F]`. If `< 0` -> blocked (`bit 6`, `carry = 1`).
4. `delta = int8(tile_elev - entity_elev)`:
   - `delta == 0`: blocked if `(or_bits & 0x80) && !(entity_flags & 0x40)`, or if `or_bits & 0x10`.
   - `delta >= +2`: blocked (`bit 6`).
   - `delta == +1`: blocked if `(entity_flags & 0x20)` or `(or_bits & 0x10)`.
   - `delta == -1`: blocked if `or_bits & 0x10`.
   - `delta <= -2`: ledge-drop check! Requires `(entity_flags & 0x10)`, `z_vel == 0`, `(entity_elev & 1) == 0`, and `height_table[entity_elev] - height_table[tile_elev] >= scan_depth` (`<= 0x0180` px in 8px steps). On pass, writes `drop_height` to `0xFF196E` and sets `bit 5` (`carry = 1`).
