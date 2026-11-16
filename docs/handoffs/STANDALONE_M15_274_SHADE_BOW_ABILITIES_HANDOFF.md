# STANDALONE M15.274 SHADE & BOW ABILITIES HANDOFF

**Milestone:** SEGA-THOR HYBRID — M15.274  
**Repository:** `C:/Github/Sega-Thor`  
**Standalone Repository (`C:/Github/Sega-Thor-Standalone`):** STRICTLY READ-ONLY / UNTOUCHED  
**Machine-Readable Evidence Output:** `data/thor-evidence/rom_shade_bow_abilities_m15_274.json`  
**Extraction Tool:** `tools/extract_rom_shade_bow_abilities.py`  
**Deterministic Test Suite:** `tests/rom_shade_bow_abilities_test.py`

---

## 1. Shade (`0x001C`, State Table `0x013008`)

- **Shadow Tether (`0x013036`):** SP cost `7` (`0x011B28`), State `0x06`, Action `0x02`, sets `$FF1982 = 0xFF`.
- **Shadow Surge (`0x013180`):** State `0x0C`, Action `0x0E`.

## 2. Bow (`0x001E`, State Table `0x0138E6`)

- **Bite (`0x013934`):** SP cost `4` (`0x011B28`).
- **Pollen / Vine (`0x0139BA`):** SP cost `9` (`0x011B28`), calls `0x013FDC`, State `0x04`, Action `0x00`.
- **Plant Storm (`0x013A18`):** SP cost `28` (`0x1C`, `0x011B28`), State `0x0A`, Action `0x10`.
