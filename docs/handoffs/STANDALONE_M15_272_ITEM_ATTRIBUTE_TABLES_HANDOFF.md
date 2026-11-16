# STANDALONE M15.272 ITEM ATTRIBUTE & PICKUP FANFARE TABLES HANDOFF

**Milestone:** SEGA-THOR HYBRID — M15.272  
**Repository:** `C:/Github/Sega-Thor`  
**Standalone Repository (`C:/Github/Sega-Thor-Standalone`):** STRICTLY READ-ONLY / UNTOUCHED  
**Machine-Readable Evidence Output:** `data/thor-evidence/rom_item_attribute_tables_m15_272.json`  
**Extraction Tool:** `tools/extract_rom_item_attribute_tables.py`  
**Deterministic Test Suite:** `tests/rom_item_attribute_tables_test.py`

---

## 1. 64-Byte Item Attribute Table (`0x05CE16`, IDs `0x00..0x3F`)

- Bit 5 (`0x20`): non-discardable / key item (`0x01` Knife = `0xA1`, `0x0E..0x11` = `0xA2`, `0x31..0x3F` = `0xA0`).
- `0x02..0x0C`: `0x01` (weapons). `0x1A..0x2A`: `0x00` (consumables/rings).

## 2. 64-Byte Pickup Fanfare Tier Table (`0x05CE56` at `0x007A6C`)

- Tier `0`: SFX `0xC0035F`
- Tier `1`: SFX `0xC00360`
- Tier `2`: SFX `0xC00361`
- Tier `3`: Fanfare `0x0508` + Player State `0x44` / Action `0x4A`
- Tier `4`: Fanfare `0x050D` + Player State `0x44` / Action `0x4A`
- Pickup persistent flag setter (`0x007AF0`): bank `$FF197A` (`1=$FF0DBC`, `2=$FF0E1C`, `3=$FF0E2C`), bit `$FF197C`.
