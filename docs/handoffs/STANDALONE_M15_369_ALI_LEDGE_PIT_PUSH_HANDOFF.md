# Standalone Handoff — M15.369: ROM Ali Ledge Climb, Pit Fall & Push Trigger (`0x005A30..0x005BA0`)

## ROM Source & Evidence
- ROM routine: `0x005A30..0x005BA0`
- Evidence JSON: `data/thor-evidence/rom_ali_ledge_pit_push_m15_369.json`
- Extractor: `tools/extract_rom_ali_ledge_pit_push.py`
- Python tests: `tests/rom_ali_ledge_pit_push_test.py`

## Exact Behavioral Contract
1. **Ledge Climb (`0x005A30..0x005A70`)**:
   - X-wall bit 5: `bit 1 == 0 -> state 0x2A`, `bit 1 == 1 -> state 0x2E`.
   - Y-wall bit 5: `bit 1 == 0 -> state 0x28`, `bit 1 == 1 -> state 0x2C`.
   - Clears `$FF1970 = 0`, sets `anim = 0x34`.
2. **Pit Fall vs. Moving Platform Carry (`0x005A72..0x005B2A`)**:
   - Checks `(tile_flags & 0x90) == 0x80` (`0x009C40`), then probes platforms (`0x009C7C`):
     - On platform (`carry == 1`): clears bit 3 of `+0x36`; if `Z == 0`, adds platform `(vx, vy)` to Ali's `(vx, vy)`.
     - On pit (`carry == 0`): sets bit 3 of `+0x36`; if `Z == 0`, sets `state = 0x3E`, `anim = 0x38`, `facing = 0`, zeroes velocities, sets `$FF1995 = 0xFF` and `$FF17B8` bit 2, and plays `0x00B00213` (or `0x00B00240` if `+0x39` bit 1 set).
3. **Pushable Block Trigger (`0x005B2C..0x005BA0`)**:
   - Requires contact entity `+0x68` in `[$FF19E8, $FF2954)` with `+0x38` bit 7 set and pure cardinal D-pad matching push direction; sets `facing`, `state = 0x30`, `anim = 0x36`.
