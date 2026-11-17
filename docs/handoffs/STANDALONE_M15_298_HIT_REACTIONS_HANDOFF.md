# Standalone Handoff: M15.298 — Hit-Reaction Physics, Shield Parry & Hit-Spark Pool (`0x00BF26`..`0x00C060`)

## Summary
- **4 Hit-Reaction Profiles (`0x00BF26`):**
  - `0` (`0x00BF2E`): `(0x300, 0x2C0)`, `vz = 0` when grounded; if airborne (`$10 != 0`), sets `$86` bit 7 and falls through to `4`.
  - `2` (`0x00BF98`): `(0, 0)`, **`vz = 0x70000`** (vertical launcher).
  - `4` (`0x00BF58`): `(0x400, 0x3C0)`, **`vz = 0x28000`** (medium knockback).
  - `6` (`0x00BF78`): `(0x700, 0x680)`, **`vz = 0x30000`** (heavy launch).
- **Shield Parry (`0x00BFAE`):** Plays `0x00900202`, triggers Ali recoil (`0x008530`) when weapon `$FF198C` is `6` or `10`, and spawns hit-spark.
- **Hit-Spark Pool (`0x00C012`):** 4 slots at `$FF19C8` (stride `8`, `type = 2`, `timer = 5` frames).

## C++20 Standalone Target
- Extend `src/game/combat/` with `apply_hit_reaction_physics()`, shield parry recoil, and 4-slot hit-spark pool.
- Evidence artifact: `data/thor-evidence/rom_hit_reactions_m15_298.json`.
