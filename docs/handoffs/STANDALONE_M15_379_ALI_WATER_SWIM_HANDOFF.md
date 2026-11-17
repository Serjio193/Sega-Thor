# Standalone Handoff — M15.379: ROM Ali Pit Respawn Interpolation & Water Swimming Combat (`0x006B2A..0x006D7A`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x006B2A..0x006B46`: `ali_state58_recovery` (clears velocity deltas; upon animation completion `bit 1 of +0x37 == 1`, transitions to `state = 0x5A`, `anim = 0x10`).
  - `0x006B48..0x006C38`: `ali_pit_respawn_interpolation_step` (per-frame update for state 0x38: increments timer `+0x10` up to 8; steps coordinate interpolation 2px per axis towards anchor `$FF1972`; when target reached, resets facing to 2 (down), loads animation descriptors from `$03F306`, sets 63-frame invulnerability timer `+0x32 = 0x3F` and `$FF198E = 12`, notifies companion entity at `$FF1AA4` via `0x0001378A`, and clears pit lock flag `bit 2 of $FF17B8`).
  - `0x006C3A..0x006C44`: `ali_post_respawn_idle_gate` (waits for animation completion `bit 1 of +0x37 == 1` before dispatching to idle `0x0062CC`).
  - `0x006C46..0x006CCC`: `ali_water_swimming_locomotion` (sets duration `+0x4A = 20`; applies 25% speed scaling `d1 >>= 2, d2 >>= 2` while swimming; increments stroke counter `$FF1980`; sets anim `0x20` (stroke) or `0x24` (coast); when stopping without input: if `$FF1980 < 16`, enters tread water state 6 / anim 6 with timer 1 and splash SFX `#$500341`; if `$FF1980 >= 16`, enters state 0x12 / anim 0x12).
  - `0x006CCE..0x006D7A`: `ali_water_weapon_attacks` (Button A triggers water attacks based on equipped weapon `$FF0D7E`: unarmed -> anim 0x48 / state 0x42; dagger 0x0C -> anim 0x52 / state 0x4A; sword 0x0E -> checks lock `$FF1B60`, sets anim 0x56 or 0x5E for special sword `$FF0DB1 == 8`, state 0x4E, SFX `#$600377`; bow 0x10 -> checks lock `$FF1B60`, sets anim 0x5A / state 0x52; clears input lock `$FF16EA = 0`).
- **Evidence JSON:** `data/thor-evidence/rom_ali_water_swim_m15_379.json`
- **Python Extractor & Tests:** `tools/extract_rom_ali_water_swim.py`, `tests/rom_ali_water_swim_test.py`
