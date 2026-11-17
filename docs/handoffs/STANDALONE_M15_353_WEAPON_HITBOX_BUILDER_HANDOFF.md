# Standalone Handoff — M15.353: Facing-Mirrored 3D Weapon Hitbox Builder & FX-Pool Scanner (`0x00B9EC..0x00BB1E`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x00B9EC..0x00BAA8`: `find_fx_pool_hit` (8-slot `0x5A`-stride `$FF2ABC` pool with `$FF194A` resume state)
  - `0x00BAAC..0x00BB1E`: `build_weapon_hitbox_all` (`0x00FF19E8`) / `build_weapon_hitbox_enemies` (`0x00FF1CD8`, 17 slots)
- **Evidence JSON:** `data/thor-evidence/rom_weapon_hitbox_builder_m15_353.json`
- **Python Extractor & Tests:** `tools/extract_rom_weapon_hitbox_builder.py`, `tests/rom_weapon_hitbox_builder_test.py`

## Behavior
- Reads 12-byte local hitbox `(dz1, dz2, dx1, dy1, dx2, dy2)` indexed by `(facing == 3 ? 1 : facing) * 12`.
- When `facing == 3` (Left), mirrors X: `(dx1, dx2) = (-dx2, -dx1)`, then translates by `(x, y + y_off, z + y_off)`.
