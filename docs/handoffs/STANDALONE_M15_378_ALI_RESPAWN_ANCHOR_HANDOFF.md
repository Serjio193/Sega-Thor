# Standalone Handoff — M15.378: ROM Ali Pit-Fall Damage, Safe Respawn Anchor & 3px Coordinate Interpolator (`0x00693C..0x006B28`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x00693C..0x006996`: `ali_pit_damage_and_fatal_check` (at recovery timer `10`, applies `-20` HP pit damage; if HP becomes `0`, sets death flag `$FF1987 = 0xFF` and scans for auto-revive item `0x2A`; if missing, triggers fatal death state `0x70`, stops BGM `0x00DAE6`, and starts 40-frame fade timer `$FF1992 = 0x28`).
  - `0x006998..0x006B10`: `ali_safe_anchor_respawn_loop` (wakes up at timer `0`: resets to idle state `0x00`, anim `0x00`, facing `2` (down), sets 63-frame invulnerability timer `$32 = 0x3F`, loads anchor coordinate from `$FF1972`, steps 3px interpolator, sets 12-frame post-respawn invulnerability timer `$FF198E = 0x0C`, and consumes auto-revive item `0x2A` if dead).
  - `0x006B12..0x006B28`: `step_1px_coordinate_interpolator` (1-pixel step towards target coordinate, increments step counter `d5` on change; called 3 times per axis for 3px/frame smooth anchor camera/position interpolation).
- **Evidence JSON:** `data/thor-evidence/rom_ali_respawn_anchor_m15_378.json`
- **Python Extractor & Tests:** `tools/extract_rom_ali_respawn_anchor.py`, `tests/rom_ali_respawn_anchor_test.py`
