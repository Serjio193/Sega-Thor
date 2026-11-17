# Standalone Handoff — M15.385: ROM Ali Combat Combo Chains, 6-Cartwheel Super Finisher & Ripple Stun Callback (`0x00780E..0x007A28`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x00780E..0x007850`: `ali_rapid_spin_facing_rotation` (computes Ali 360-degree facing rotation during rapid spin attack: starts `d1 = 3`; at frame `+0x30 = 4` facing is 3; at frame 8 facing is 2; at frame 12 facing is 1; at frame 16 facing is 0; saves facing to `+0x16(a6)`).
  - `0x007852..0x0078F6`: `ali_combo_chain_evaluator` (evaluates animation completion bit 1 of `+0x37(a6)` and input probe; verifies motion triggers `0x8814`, `0x86B4`, `0x8778`, `0x8910`, `0x8950` first; if none match, evaluates latched button bit 4 in `$FF165E`: if clear, dispatches Combo Hit 1 `state = 8, anim = 0x0A`, SFX `#$7004D3`, clears `$FF16EA`; if set, dispatches Combo Hit 2 `state = 0x0A, anim = 0x3A`).
  - `0x0078F8..0x007928`: `ali_combo_flurry_cleanup` (verifies target entity at `$FF1B60`: if entity state equals `0x3A`, installs stun handler `0xBEBA` into `$FF1940.l` and invokes cleanup).
  - `0x00792A..0x0079FA`: `ali_cartwheel_chain_finisher` (manages cartwheel evasion chain; checks motion `0x8910`: on match, increments counter `$FF1991`; if counter < 6, alternates anim between `0x7C` and `0x7E`, keeps `state = 0x72`, plays SFX `#$80025B`; on 6th cartwheel, triggers Super Finisher: zeroes horizontal sub-speeds `+0x4E` and `+0x52`, sets vertical leap impulse `+0x56 = 0x00040000` (+4.0 px/frame), sets elevation `+0x10 = 15` px, projects base speed `0x800` along facing angle via `0xE2D4` into `+0x72` and `+0x76`, sets `state = 0x26, anim = 0x32`, plays SFX `#$80025B`).
  - `0x0079FC..0x007A0E`: `ali_cartwheel_landing_callback` (ground friction callback installed at `$FF1940.l`: tests vertical velocity `+0x56(a0)`; when zero, clears horizontal sub-velocities `+0x4E(a0) = 0` and `+0x52(a0) = 0`).
  - `0x007A10..0x007A28`: `ali_air_dive_recovery` (checks recovery completion and ground impact frame 24, calling shockwave routine `0x846C`).
- **Evidence JSON:** `data/thor-evidence/rom_ali_combo_cartwheel_finisher_m15_385.json`
- **Python Extractor & Tests:** `tools/extract_rom_ali_combo_cartwheel_finisher.py`, `tests/rom_ali_combo_cartwheel_finisher_test.py`
