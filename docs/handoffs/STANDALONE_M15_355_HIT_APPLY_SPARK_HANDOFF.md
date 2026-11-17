# Standalone Handoff — M15.355: ROM Hit-Apply, Guard Deflect, 4-Way Knockback & Hit-Spark (`0x00BE98..0x00C0BC`)

## ROM Source & Evidence
- ROM routines: `0x00BE98` (enemy gauge + apply), `0x00BEBA` (apply), `0x00BFAE` (guard deflect), `0x00C012` (hit-spark spawner)
- Evidence JSON: `data/thor-evidence/rom_hit_apply_spark_m15_355.json`
- Extractor: `tools/extract_rom_hit_apply_spark.py`
- Python tests: `tests/rom_hit_apply_spark_test.py`

## Exact Behavioral Contract
1. **Enemy Gauge Latch (`0x00BE98..0x00BEB8`)**:
   - If `target != $FF19E8` and bit 0 of `target+0x3A` is clear, sets `$FF184F = 0x5A` and `$FF1850 = target`.
2. **Guard / Shield Deflect (`0x00BEBA..0x00BED2`, `0x00BFAE..0x00C010`)**:
   - If `target+0x6D != 0` and `target.facing == (attacker.facing ^ 2)`:
     - Records attacker (`+0x7E`), hitstop on both (`+0x82`), damage word (`+0x86`), hit facing (`+0x84`), clears `+0x9D`.
     - If `attacker == $FF19E8` and `$FF198C in {6,10}`, calls `0x008530`.
     - Spawns hit-spark (`0x00C012`), sets bit 3 of `attacker+0x37` and plays block SFX `0x00900202` if newly set.
3. **4-Way Knockback Table (`0x00BF26..0x00BFAC`)**:
   - Clears `+0x6D`, sets `+0x83 = 6`, spawns hit-spark, plays hit SFX `*(u32*)(record+4)`.
   - `tier = (damage_word >> 14) & 3`:
     - `0`: if `target.z == 0`, `(0x300, 0x2C0, vz=0)`; else sets bit 7 of `target+0x86` and falls through to `2`.
     - `1`: `(0, 0, vz=0x70000)`.
     - `2`: `(0x400, 0x3C0, vz=0x28000)`.
     - `3`: `(0x700, 0x680, vz=0x30000)`.
   - Horizontal velocity signs computed by `0x00E2D4` from `(vx_mag, vy_mag, attacker.facing)`.
4. **4-Slot 3D Intersection Hit-Spark (`0x00C012..0x00C0BC`)**:
   - First free `8B` slot in `$FF19C8[0..3]` (`type == 0`): sets `type = 2`, `timer = 5`, `x = mid_x`, `y = mid_y - mid_z` from clamped AABB intersection midpoints.
