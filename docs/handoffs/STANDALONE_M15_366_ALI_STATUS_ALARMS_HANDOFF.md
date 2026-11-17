# Standalone Handoff — M15.366: ROM Ali Status Tick, Low-HP/SP Alarms & Respawn Anchor (`0x00557A..0x0056D7`)

## ROM Source & Evidence
- ROM routine: `0x00557A..0x0056D7`
- Evidence JSON: `data/thor-evidence/rom_ali_status_alarms_m15_366.json`
- Extractor: `tools/extract_rom_ali_status_alarms.py`
- Python tests: `tests/rom_ali_status_alarms_test.py`

## Exact Behavioral Contract
1. **Invulnerability & Weapon Swap (`0x005670..0x0056D7`)**:
   - If `$FF1985 != 0`: zeroes Ali's `vx/vy` (`+0x72/+0x76`).
   - Else if `$FF198E > 0`: sets bit 3 of `Ali+0x38` (invulnerable), decrements `$FF198E`, clears bit 3 when `0`.
   - If `$FF198F != 0` and `$FF1B60 != 0x3A`: writes `$FF0D7E` to `Ali+0x06`, sets `Ali+0x5C = -1`, clears `$FF198F`.
2. **Ali Extent Sync (`0x00558A..0x0055AE`)**:
   - Copies `+0x48/+0x49` to `+0x42/+0x44` (swapping if `(+0x17 & 1) != 0`).
3. **Low-HP Heartbeat (`0x0055B0..0x0055EC`)**:
   - Counts down `$FF1993`. At `0`, if `0 < HP (+0x8A) <= 0x40`, reloads `(HP >> 1) + 10` and plays SFX `0x00A0038C`.
4. **Low-SP Alarm (`0x0055EE..0x005637`)**:
   - Counts down `$FF1994`. At `0`, if `$FF1AA4 != 0 && $FF1AA4 != 0x16 && SP (+0x8E) <= 0x18`, reloads `SP + 12` and plays SFX `0x00A004DD`.
5. **Safe Respawn Anchor (`0x005638..0x00566E`)**:
   - If bit 7 of `+0x6F` is clear and `Ali.state (+0x04) in {0, 2, 4}`, saves `(X, Y)` to `$FF1972..$FF1974`.
