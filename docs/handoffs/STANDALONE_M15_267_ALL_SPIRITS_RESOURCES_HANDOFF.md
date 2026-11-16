# STANDALONE M15.267 ALL-SPIRITS RESOURCES, DMA & SPAWN CONTRACTS HANDOFF

**Milestone:** SEGA-THOR HYBRID — M15.267  
**Repository:** `C:/Github/Sega-Thor`  
**Standalone Repository (`C:/Github/Sega-Thor-Standalone`):** STRICTLY READ-ONLY / UNTOUCHED  
**Machine-Readable Evidence Output:** `data/thor-evidence/rom_all_spirits_resources_m15_267.json`  
**Extraction Tool:** `tools/extract_rom_all_spirits_resources.py`  
**Deterministic Test Suite:** `tests/rom_all_spirits_resources_test.py`

---

## 1. All 4 Spirits Resource & Spawn Table (`0x014522`, `0x014668..0x014798`)

| Spirit | Type ID | `$FF0D7A` Bit | Spawn PC | Update PC (`0x00DF54`) | Nemesis ROM | VRAM Tile | VRAM Byte |
|---|---|---|---|---|---|---|---|
| **Dytto** (Water) | `0x0018` | `bit 0` | `0x014668` | `0x011B4E` | `0x00141580` | `0x04D0` | `0x9A00` |
| **Efreet** (Fire) | `0x001A` | `bit 1` | `0x0146D6` | `0x01254C` | `0x001425AE` | `0x0450` | `0x8A00` |
| **Shade** (Shadow) | `0x001C` | `bit 2` | `0x014742` | `0x012F1E` | `0x00143262` | `0x0280` | `0x5000` |
| **Bow** (Plant) | `0x001E` | `bit 3` | `0x014798` | `0x01387C` | `0x00143A42` | `0x0360` | `0x6C00` |

- Shared Nemesis decompression work buffer: `$00FF2FA8` (decompressed via `0x003820`).
- Shared DMA queue pointer: `$00FF1892`.
- Dytto, Efreet, and Shade all initialize `Max SP = 100` (`$88/$8A`) and `SP drain rate = 10` (`$8C/$8E`).
