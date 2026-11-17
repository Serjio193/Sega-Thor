# Standalone Handoff — M15.368: ROM Ali HP Damage, Death Trigger & Hitstun/Knockdown States (`0x005850..0x0059B8`)

## ROM Source & Evidence
- ROM routine: `0x005850..0x0059B8`
- Evidence JSON: `data/thor-evidence/rom_ali_damage_death_m15_368.json`
- Extractor: `tools/extract_rom_ali_damage_death.py`
- Python tests: `tests/rom_ali_damage_death_test.py`

## Exact Behavioral Contract
1. **Mount Redirect Invuln (`0x005850..0x005862`)**:
   - Calls `0x0062CC`, sets `$FF198E = 7`, sets invuln bit 3 of `Ali+0x38`, and skips damage calc.
2. **Damage Calculation (`0x005866..0x005892`)**:
   - Clears `$FF1989`; `dmg = (damage_word & 0x0FFF) - Ali.defense (+0x92)`, clamped to `>= 1` and `<= Ali.HP (+0x8A)`.
   - Subtracts `dmg` from `Ali.HP`, adds `dmg` to `$FF0E3C`, and calls `0x0085C4`.
3. **Death Trigger (`0x005894..0x0058C0`)**:
   - When `Ali.HP == 0`: calls `0x00E4D0`, ORs `0xC000` into `damage_word` (forced knockdown), sets `$FF1987 = 0xFF`, zeroes `Ali.SP` (or sets `1` and `spirit.state = 0x1A` if `$FF1AA4 != 0`).
4. **Hitstun vs. Knockdown (`0x0058C6..0x005940`)**:
   - `(flags & 0xC000) == 0`: light hitstun (`state = 0x18`, `anim = 0x18 + 2 * dir`, with `facing == 3` odd-dir flip).
   - `(flags & 0xC000) != 0`: if `hit_facing != Ali.facing`, sets `Ali.facing = hit_facing ^ 2`, `state = 0x1A`, `anim = 0x26`; else `state = 0x20`, `anim = 0x2C`. Lifts `Ali.Z += 16`, clears `+0x83` and `$FF198E`, sets invuln bit 3 of `+0x38`.
