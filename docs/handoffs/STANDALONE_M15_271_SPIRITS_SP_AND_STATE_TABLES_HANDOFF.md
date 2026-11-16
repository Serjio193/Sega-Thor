# STANDALONE M15.271 ALL-SPIRITS SP DRAIN & STATE TABLES HANDOFF

**Milestone:** SEGA-THOR HYBRID — M15.271  
**Repository:** `C:/Github/Sega-Thor`  
**Standalone Repository (`C:/Github/Sega-Thor-Standalone`):** STRICTLY READ-ONLY / UNTOUCHED  
**Machine-Readable Evidence Output:** `data/thor-evidence/rom_spirits_sp_and_state_tables_m15_271.json`  
**Extraction Tool:** `tools/extract_rom_spirits_sp_and_state_tables.py`  
**Deterministic Test Suite:** `tests/rom_spirits_sp_and_state_tables_test.py`

---

## 1. Live SP Drain Intervals (`$8E(A6)` -> 1 SP from `$8E($FF19E8)`)

| Spirit | Type ID | Drain Interval (Frames / 1 SP) | Despawn PC | State Table PC |
|---|---|---|---|---|
| **Dytto** | `0x0018` | `45` (`0x2D`) | `0x012314` | `0x011BAC` |
| **Efreet** | `0x001A` | `35` (`0x23`) | `0x012BA8` | `0x0125AE` |
| **Shade** | `0x001C` | `40` (`0x28`) | `0x0136CC` | `0x013008` |
| **Bow** | `0x001E` | `30` (`0x1E`) | `0x013F22` | `0x0138E6` |

Each spirit decrements its `$8E(A6)` timer every tick; when it hits `0`, it resets `$8E(A6)` to the spirit's interval and decrements Ali's SP `$8E($FF19E8)` by `1`, despawning cleanly when SP reaches `0`.
