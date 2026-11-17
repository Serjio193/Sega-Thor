# Standalone Handoff — M15.354: ROM Combat Hitbox Dispatch & Ali Damage Scaling (`0x00BC8E..0x00BE96`)

## ROM Source & Evidence
- ROM routines: `0x00BC8E`, `0x00BCA4`, `0x00BCBA`, `0x00BCD8` -> shared body `0x00BCEA..0x00BE96`
- Evidence JSON: `data/thor-evidence/rom_combat_hit_dispatch_m15_354.json`
- Extractor: `tools/extract_rom_combat_hit_dispatch.py`
- Python tests: `tests/rom_combat_hit_dispatch_test.py`

## Exact Behavioral Contract
1. **Four Entrypoints**:
   - `0x00BC8E`: callback `0x00BEBA`, pool `$FF19E8` (21 slots, `d7=20`)
   - `0x00BCA4`: callback `0x00BE98`, pool `$FF1CD8` (17 slots, `d7=16`)
   - `0x00BCBA`: callback `0x00BE98`, sets `$FF198B = 2`, pool `$FF1CD8` (17 slots, `d7=16`)
   - `0x00BCD8`: callback `0x00BEBA`, pool `$FF19E8` (1 slot, `d7=0`)
2. **Frame Record Lookup (`0x00BCEA..0x00BD18`)**:
   - Early return if bit 3 of `entity+0x37` is set.
   - Saves `attack_index` (`d0`) to `$FF198C`.
   - Resolves `table = *(u32*)(entity+0x22) + attack_index + *(i16*)(table + attack_index)`.
   - Linear scan over `0x2C`-byte records for `record.frame_id == entity.frame_id (+0x32)`; stops on `< 0`.
3. **Direction & Damage Pack (`0x00BD1A..0x00BDA0`)**:
   - Facing `entity+0x16`: if `3`, `mirror_x = true`, `dir_idx = facing - 2`; `box_ptr = record + 8 + dir_idx * 12`.
   - `base_dmg = u8(record+2)`, `hit_flags = u8(record+3)`.
   - If `entity == $FF19E8` (Ali), add cumulative kill-count bonus from `entity+0x96` at thresholds `10(+1), 20(+1), 30(+1), 40(+1), 50(+1), 100(+2), 200(+4)`.
   - `hitstop = (hit_flags & 0x0F) + 1`; `damage_word = (((hit_flags & 0xF0) << 8) | dmg) + entity+0x90`.
4. **3D AABB Overlap & Callback (`0x00BDA2..0x00BE64`)**:
   - Transforms `12B` box (`z_min, z_max, x_min, y_min, x_max, y_max`) with `X=+0x08`, `Y=+0x0C + +0x14`, `Z=+0x10 + +0x14`.
   - Skips slots with `type <= 0`, `slot == attacker`, `bit 3 of +0x38` set, or `u8(slot+0x82) != 0` (already in hitstop).
   - Invokes callback on every overlapping target.
5. **Post-Loop Special Cancel (`0x00BE68..0x00BE96`)**:
   - If `attacker == $FF19E8`, bit 3 of `entity+0x37` became set, and `$FF198C` is `6` or `10`, triggers `0x008530`.
