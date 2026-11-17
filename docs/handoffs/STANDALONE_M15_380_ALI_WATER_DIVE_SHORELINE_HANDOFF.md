# Standalone Handoff — M15.380: ROM Ali Water Dive, Air Combat, Splash & Shoreline Exit (`0x006D7A..0x007074`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x006D7A..0x006DD0`: `ali_water_paddle_coast` (advances stroke counter `$FF1980` by 1; triggers ripple effect mode 4/6 via `0x00BCBA`; on animation completion `bit 1 of +0x37 == 1`, transitions to `state = 0x10`, `anim = 0x24`).
  - `0x006DD2..0x006E78`: `ali_water_dive_jump_arc` (steers in air while diving; integrates horizontal deltas `+0x4E` and `+0x52`; applies `0xC000` gravity acceleration to vertical velocity `+0x56`; on descending to ground impact `height <= 0`, clears lock `$FF17BB = 0`, sets recovery timer `+0x30 = 0x0C`, and transitions to landing `0x005E78`).
  - `0x006E7A..0x006ED4`: `ali_air_attack_dispatch_from_water` (in-air attack on Button A: sword/unarmed sets `anim = 0x32`, `state = 0x26`, plays SFX `#$700209`; dagger sets `anim = 0x50` or `0x62` if diving in state `0x16`, `state = 0x48`, plays SFX `#$800223` / `#$800222`; clears lock `$FF16EA = 0`).
  - `0x006EDA..0x006F8A`: `ali_air_attack_steppers` (stepper logic for jump slash and dagger air dive, integrating velocities with gravity `0xC000`).
  - `0x006F8C..0x006FE2`: `ali_water_splash_landing` (surface splash reset: clears duration `+0x4A = 0`, clears height `+0x10 = 0`, clears vertical velocity `+0x56 = 0`, plays water landing SFX `#$600343`).
  - `0x006FE4..0x007074`: `ali_shoreline_boundary_exit_triggers` (probes water boundary via `0x007074` and terrain probe `0x007D8A`: primary exit triggers `anim = 0x72, state = 0x64`, vertical jump impulse `+0x56 = 0x40000`, splash exit SFX `#$500341`; secondary high jump triggers `anim = 0x74, state = 0x66`, vertical jump impulse `+0x56 = 0x50000`, splash exit SFX `#$500341`; shallow exit triggers `anim = 0x76, state = 0x68`).
- **Evidence JSON:** `data/thor-evidence/rom_ali_water_dive_shoreline_m15_380.json`
- **Python Extractor & Tests:** `tools/extract_rom_ali_water_dive_shoreline.py`, `tests/rom_ali_water_dive_shoreline_test.py`
