# Standalone Handoff — M15.389: ROM Ali Weapon Combat Attack Initializers (`0x00803C..0x00822C`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x00803C..0x00808E`: `ali_sword_attack_dispatch` (Sword attack variations: evaluates Dash Leap motion `0x8814` to trigger leap slash `state = 0x5C, anim = 0x66`; evaluates moving D-pad bits `0x0F` and running bit 4 in `$FF165F` to trigger running slash `state = 0x54, anim = 0x60` vs stationary slash `state = 0x46, anim = 0x4E`).
  - `0x008090..0x0080D6`: `ali_bow_bomb_attack_dispatch` (Bow draw: verifies `$FF1B60 == 0`, plays bow SFX `#$600377`, triggers standard bow `state = 0x4C, anim = 0x54` or Fire Bow `anim = 0x5C` when `$FF0DB1 == 8`; Bomb placement: triggers `state = 0x50, anim = 0x58`).
  - `0x0080D8..0x00814C`: `ali_sprint_dash_attacks` (running attacks from state 4: Sword `0x0C` redirects to `0x803C`; Dagger `0x0A` checks special motions `0x8814`, `0x8778`, `0x8950`; default fallback initializes sprint tackle `state = 0x38, anim = 0x40`, sets bit 2 of `+0x39`, timer `+0x9A = 0x15`).
  - `0x00814E..0x00820C`: `ali_mount_grab_attacks` (mount/grab entity combat strikes: initializes `anim = 0x42, state = 0x3A` or `anim = 0x3E, state = 0x36`).
  - `0x00820E..0x00822C`: `ali_attack_commit_finalize` (sets recovery timer `$FF198B = 4`, commits `anim = d0` and `state = d1`, clears input ring count `$FF16EA = 0`, unrolls stack via `$FF1934.l`).
- **Evidence JSON:** `data/thor-evidence/rom_ali_weapon_attacks_m15_389.json`
- **Python Extractor & Tests:** `tools/extract_rom_ali_weapon_attacks.py`, `tests/rom_ali_weapon_attacks_test.py`
