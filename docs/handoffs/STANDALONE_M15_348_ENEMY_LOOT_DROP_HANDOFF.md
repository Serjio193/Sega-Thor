# Standalone Handoff — M15.348: Enemy Loot / 2-Tier Drop-Table Roller (`0x00F476..0x00F53A`)

## Summary
- **Source routine in Beyond Oasis (USA):** `0x00F476..0x00F53A`
- **Evidence JSON:** `data/thor-evidence/rom_enemy_loot_drop_m15_348.json`
- **Python Extractor & Tests:** `tools/extract_rom_enemy_loot_drop.py`, `tests/rom_enemy_loot_drop_test.py`

## Behavior
1. If `0x8C(a6) != 0`: `0xFFFF` drops nothing; else drops fixed `0x8C(a6)`.
2. Otherwise (if `!(0x6F(a6) & 0x80)`): evaluates 2-tier table `(prob1, item1, prob2, item2)` against 16-bit PRNG (`roll <= prob`).
3. Spawns pickup entity `0x0008` with `vz = 0x00080000` (`+8.0 px/frame`), jittered timer `prng & 0x3F`, and flags from `0x0005CE16[item]`.
