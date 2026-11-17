# Standalone Handoff — M15.383: ROM Ali Water Combat Ripple Physics & Air Dive (`0x0074A4..0x007656`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x0074A4..0x00751E`: `ali_ripple_impulse_callback` (calculates directional ripple velocities: compares target coordinates `+0x08/+0x0C` against Ali coordinates, assigns orientation `+0x84`, applies horizontal speeds `(0x800, 0x780)` projected to `+0x4E/+0x52`, and assigns vertical jump impulse `+0x56 = 0x00020000`).
  - `0x007520..0x00754E`: `ali_weapon_durability_table` (indexes table `[0x0000, 0x0001, 0xFFFF, 0x2003]` by `element - 2`; on negative value `0xFFFF`, rolls random 1..6 via `0x0000E2A2(5) + 1` into `+0x90`).
  - `0x007550..0x007582`: `ali_water_dagger_strike_frame8` (at frame 8 and `+0x2E == 0`, plays strike SFX `#$800223`, emits ripple mode 6, and resets to idle `0x0062CC` on animation complete).
  - `0x007584..0x007630`: `ali_dagger_somersault_high_dive` (at keyframe 36, launches massive high dive vertical impulse `+0x56 = 0x00080000` (+8.0 px/frame) and plays water splash SFX `#$500341`; applies `0xC000` gravity acceleration; on water/ground impact `height <= 0`, sets landing timer `+0x30 = 0x48` (72 frames) and branches to landing recovery `0x005E78`).
  - `0x007632..0x007656`: `ali_target_hitstun_callback` (checks target vertical speed `+0x56`: if 0, zeroes horizontal velocities `+0x4E/+0x52` and applies 12-frame hitstun timer `+0x83 = 0x0C`).
- **Evidence JSON:** `data/thor-evidence/rom_ali_water_combat_ripple_dive_m15_383.json`
- **Python Extractor & Tests:** `tools/extract_rom_ali_water_combat_ripple_dive.py`, `tests/rom_ali_water_combat_ripple_dive_test.py`
