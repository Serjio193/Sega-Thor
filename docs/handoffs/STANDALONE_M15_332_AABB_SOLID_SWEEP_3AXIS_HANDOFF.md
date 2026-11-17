# Standalone Handoff — M15.332: 3D AABB Solid Sweep (-X, +Y, -Y) (`0x009EBA..0x00A194`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x009EBA..0x009F9E`: `sweep_neg_x` (`0x40` bit 7, pushes `0x72(a5)` by `-1`)
  - `0x009FA0..0x00A098`: `sweep_pos_y` (`0x41` bit 7, pushes `0x76(a5)` by `+1`)
  - `0x00A09A..0x00A194`: `sweep_neg_y` (`0x41` bit 7, pushes `0x76(a5)` by `-1`)
- **Evidence JSON:** `data/thor-evidence/rom_aabb_solid_sweep_3axis_m15_332.json`
- **Python Extractor & Tests:** `tools/extract_rom_aabb_solid_sweep_3axis.py`, `tests/rom_aabb_solid_sweep_3axis_test.py`

## Notes
- Companion to M15.305 (`+X` sweep `0x009DD6..0x009EB8`).
- Scans 21 entity slots (`0x00FF19E8`, stride `0x00BC`), checks 3D AABB overlap + gap clamping, sets `0x68(a6) = a5` & `0x6C(a6) = 3`, and pushes `a5` when `gap == 0` and push flags (`0x37(a6) & 0x20 == 0`, `0x37(a5) & 0x40 == 0`) allow.
