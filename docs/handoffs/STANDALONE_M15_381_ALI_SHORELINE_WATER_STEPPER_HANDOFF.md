# Standalone Handoff — M15.381: ROM Ali Shoreline Stepper, Water 3/4 Scaler & Landing Gate (`0x007074..0x007262`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x007074..0x0070F6`: `ali_shoreline_exit_airborne_stepper` (airborne gravity arc applying `0xC000` gravity acceleration to vertical velocity `+0x56`, integrating horizontal deltas into `+0x72` and `+0x76`, and playing landing splash SFX `#$600343` when landing on water/ground `height <= 0`; checks animation finish `bit 1 of +0x37` when airborne vertical speed reaches 0).
  - `0x0070F8..0x007110`: `ali_ripple_custom_callback` (masks `$86(a0) &= 0x0FFF` when height is 0 and state != `0x66`).
  - `0x007112..0x007196`: `ali_water_jump_landing_audio_gate` (sets water contact bit 0 of `+0x67` while recovery timer `+0x30 <= 4`; when `+0x30 > 4`, clears bit 0 and triggers water splash SFX `#$500341`; applies `0xC000` gravity and transitions to landing recovery `0x005E78` with 20-frame landing timer `+0x30 = 0x14`).
  - `0x007198..0x0071D8`: `ali_water_velocity_3_4_scaler_and_clamp` (calculates exact 75% speed scaling `(v >> 1) + (v >> 2)` on D-pad velocity and zeroes input that opposes existing inertia for responsive water maneuvering).
  - `0x0071DA..0x007262`: `ali_jump_slash_descent_shadow` (applies `0xC000` gravity, updates shadow `0x009D00`, transitions to landing recovery `0x005E78` with 12-frame timer `+0x30 = 0x0C` on water/ground impact, and gates animation recovery `0x0062CC`).
- **Evidence JSON:** `data/thor-evidence/rom_ali_shoreline_water_stepper_m15_381.json`
- **Python Extractor & Tests:** `tools/extract_rom_ali_shoreline_water_stepper.py`, `tests/rom_ali_shoreline_water_stepper_test.py`
