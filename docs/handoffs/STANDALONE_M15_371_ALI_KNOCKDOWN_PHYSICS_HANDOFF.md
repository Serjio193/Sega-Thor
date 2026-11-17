# Standalone Handoff — M15.371: ROM Ali Airborne Knockdown Physics, Wall Impact & Landing Recovery (`0x005CF8..0x005DE2`)

## ROM Source & Evidence
- ROM routine: `0x005CF8..0x005DE2`
- Evidence JSON: `data/thor-evidence/rom_ali_knockdown_physics_m15_371.json`
- Extractor: `tools/extract_rom_ali_knockdown_physics.py`
- Python tests: `tests/rom_ali_knockdown_physics_test.py`

## Exact Behavioral Contract
1. **Prone Height & Wall Impacts (`0x005CF8..0x005D2D`)**:
   - Sets `Ali.height (+0x4A) = 0x10`.
   - On X-wall impact (`bit 6 of +0x40`): clears `vx = 0`, plays impact SFX `0x0080035D`.
   - On Y-wall impact (`bit 6 of +0x41`): clears `vy = 0`, plays impact SFX `0x0080035D`.
2. **Airborne Gravity Integration (`0x005D2E..0x005D5A`)**:
   - `vz -= 0xC000`; if `vz == 0`, `vz -= 1`.
   - `z += vz`. If `z >= 0`: calls `0x00B79A` airborne probe, accumulates `(vx, vy)` into `+0x72/+0x76`, remains airborne.
3. **Ground Landing & Death vs. Wake-Up Recovery (`0x005D5C..0x005DE2`)**:
   - Sets `z = 0`, sets `bit 2 of +0x39` (landed), sets dust/shake timer `+0x9A = 0x0F`.
   - If dead (`$FF1987 != 0` or `HP == 0`): `HP = 0`, sets `state = 0x5E`, anim `0x68` (front) or `0x6C` (back).
   - If alive: advances `state += 2`, `anim += 2`; sets recovery timer `+0x02 = 10` (or `20` if `+0x39` bit 3 was set; adds `+10` if `anim != 0x28`).
   - `0x005DC8` ticks `+0x02` countdown; on 0, advances `state += 2`, `anim += 2` (standing up) and clears `+0x39` bit 3.
