# Standalone Handoff: M15.297 — Combat Hitbox Collision & Kill-Rank Scaling (`0x00BCBA`..`0x00BF26`)

## Summary
- **`0x00BCBA` / `0x00BCD8`:** Universal attack hitbox dispatcher checking 44-byte (`0x2C`) animation-frame hitbox records (12 bytes per facing, dir 3 mirrored from dir 1) against 17 enemy slots (`$FF1CD8`, callback `0x00BE98`) or Ali (`$FF19E8`, callback `0x00BEBA`).
- **Ali Kill-Rank Bonus (`0x00BD40`):** Adds `+1` at `10, 20, 30, 40, 50` kills, `+2` at `100`, `+4` at `200` (max **`+11`** cumulative bonus damage).
- **Contact & Shield Block (`0x00BE98`..`0x00BF26`):** Sets `90`-frame (`0x5A`) enemy HP HUD timer (`$FF184F`), blocks if defender guard `$6D != 0` and `(attacker_facing ^ 2) == defender_facing`, otherwise sets `6`-frame hit-flash (`$83`) and dispatches reaction (`0x00BF26`).

## C++20 Standalone Target
- Implement `compute_ali_kill_rank_bonus()`, shield opposite-facing block check, and `90`-frame enemy HUD / `6`-frame hit-flash timers in `src/game/combat/`.
- Evidence artifact: `data/thor-evidence/rom_combat_hitbox_kernel_m15_297.json`.
