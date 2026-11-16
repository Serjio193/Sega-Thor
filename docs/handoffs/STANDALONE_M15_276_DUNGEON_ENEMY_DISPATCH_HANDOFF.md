# STANDALONE M15.276 DUNGEON ENEMY DISPATCH HANDOFF

**Milestone:** SEGA-THOR HYBRID — M15.276  
**Repository:** `C:/Github/Sega-Thor`  
**Standalone Repository (`C:/Github/Sega-Thor-Standalone`):** STRICTLY READ-ONLY / UNTOUCHED  
**Machine-Readable Evidence Output:** `data/thor-evidence/rom_dungeon_enemy_dispatch_m15_276.json`  
**Extraction Tool:** `tools/extract_rom_dungeon_enemy_dispatch.py`  
**Deterministic Test Suite:** `tests/rom_dungeon_enemy_dispatch_test.py`

---

## 1. Dungeon Enemy Families (`$66(A6)` Substate Dispatch)

| Type ID | Handler (`0x00DF54`) | Substates | Notes |
|---|---|---|---|
| `0x006C` | `0x01F5A4` | `3` | 4-entry variant table `0x01F5C8`: `[3, 6, 9, 12]` (`$96 -> $90`) |
| `0x006E` | `0x01EB54` | `10` | 10-branch `$66(A6)` chain (`0..9`) |
| `0x0070` | `0x01DF1C` | `6` | 6-branch `$66(A6)` chain (`0..5`, shared helper `0x01DFA4`) |
| `0x0072` | `0x01D140` | `10` | Self-relative jump table `0x01D152` (`0..9`) |
