# Standalone Handoff — M15.339: Entity Camera-Viewport Culler & Airborne Landing (`0x00E522..0x00E67E`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x00E522..0x00E5AC`: `airborne_move_and_land` (integrates 16.16 `(vx, vy)`, returns `2`=airborne, `1`=just landed with `15`-frame stun `0x9A=0x0F` & thud SFX `0x00800270`, `3`/`0`=grounded)
  - `0x00E5AE..0x00E60E`: `is_offscreen_with_z` (`(x, y - z)` vs `320x224` + `margin`)
  - `0x00E610..0x00E66C`: `is_offscreen_ground` (`(x, y)` vs `320x224` + `margin`)
  - `0x00E66E..0x00E67E`: `copy_entity_struct_0xbc` (`94` words)
- **Evidence JSON:** `data/thor-evidence/rom_viewport_culler_landing_m15_339.json`
- **Python Extractor & Tests:** `tools/extract_rom_viewport_culler_landing.py`, `tests/rom_viewport_culler_landing_test.py`
