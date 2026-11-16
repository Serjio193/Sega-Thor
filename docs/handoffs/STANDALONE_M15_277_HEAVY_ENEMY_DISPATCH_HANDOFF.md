# STANDALONE M15.277 HEAVY & BOSS ENEMY DISPATCH HANDOFF

**Milestone:** SEGA-THOR HYBRID — M15.277  
**Repository:** `C:/Github/Sega-Thor`  
**Standalone Repository (`C:/Github/Sega-Thor-Standalone`):** STRICTLY READ-ONLY / UNTOUCHED  
**Machine-Readable Evidence Output:** `data/thor-evidence/rom_heavy_enemy_dispatch_m15_277.json`  
**Extraction Tool:** `tools/extract_rom_heavy_enemy_dispatch.py`  
**Deterministic Test Suite:** `tests/rom_heavy_enemy_dispatch_test.py`

---

## 1. Heavy / Boss Enemy Families (`$66(A6)` Substate Dispatch)

| Type ID | Handler (`0x00DF54`) | Substates | Substate 0 Target | Notes |
|---|---|---|---|---|
| `0x0068` | `0x025B32` | `5` | `0x025D30` | Substate 1 -> `0x0263FA` |
| `0x0074` | `0x027C8A` | `5` | `0x027DDA` | Substate 1 -> `0x027CAA` |
| `0x0076` | `0x0283E6` | `4` (`& 0x7F`) | `0x0284E8` | Init timer `20` (`0x14`), Z `0x20`, `$90 = $96` |
| `0x0080` | `0x028810` | `3` | `0x0288C2` | Init timer `40` (`0x28`), State `6`, Action `4`, `$90 = $96` |
