# Standalone Handoff — M15.367: ROM Ali Passive HP/SP Regen & Mount Damage Redirect (`0x0056D8..0x00584E`)

## ROM Source & Evidence
- ROM routine: `0x0056D8..0x00584E`
- Evidence JSON: `data/thor-evidence/rom_ali_regen_mount_redirect_m15_367.json`
- Extractor: `tools/extract_rom_ali_regen_mount_redirect.py`
- Python tests: `tests/rom_ali_regen_mount_redirect_test.py`

## Exact Behavioral Contract
1. **Passive HP Regen (`0x005710..0x005760`)**:
   - Active when `$FF1987 == 0`, `$FF1985 == 0`, `Ali.state == 0`, `$FF0DBA` bit 0 set, and `$FF16F0` bit 5 (water) set.
   - Period mask `0x07` if `$FF0DBA` bit 3 set, else `0x0F`. On `($FF1971 & mask) == 0`, increments `Ali.HP` if `0 < HP < MaxHP`.
2. **Passive SP Regen (`0x005762..0x0057B0`)**:
   - Active when `$FF1AA4 == 0` (no spirit summoned) and (`$FF16F0` bit 5 OR `$FF0DBA` bit 2 set).
   - Period mask `0x03` if `$FF0DBA` bit 3 set, else `0x07`. On `($FF1971 & mask) == 0`, increments `Ali.SP` if `SP < MaxSP`.
3. **Height Sync & Mount Damage Redirect (`0x0057D0..0x00584E`)**:
   - Decrements `$FF198B` and `Ali+0x6C` if nonzero; sets `Ali.height (+0x4A) = 0x28`.
   - When pending `Ali+0x86 != 0`: if `$FF1989 != 0`, `Ali.Z == 0`, and `slot[Ali+0xBC].type == 0x1C`, forwards `damage (+0x86)`, `attacker (+0x7E)`, `hitstop (+0x82/+0x83)`, and `hit_facing (+0x84)` to the mount slot and clears Ali's hit fields.
