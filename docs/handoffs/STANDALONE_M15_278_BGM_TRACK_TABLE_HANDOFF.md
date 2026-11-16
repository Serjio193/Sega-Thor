# STANDALONE M15.278 BGM TRACK TABLE & Z80 HANDSHAKE HANDOFF

**Milestone:** SEGA-THOR HYBRID — M15.278  
**Repository:** `C:/Github/Sega-Thor`  
**Standalone Repository (`C:/Github/Sega-Thor-Standalone`):** STRICTLY READ-ONLY / UNTOUCHED  
**Machine-Readable Evidence Output:** `data/thor-evidence/rom_bgm_track_table_m15_278.json`  
**Extraction Tool:** `tools/extract_rom_bgm_track_table.py`  
**Deterministic Test Suite:** `tests/rom_bgm_track_table_test.py`

---

## 1. BGM Sequence Offset Table (`0x0638D4`, Loader `0x060B50`)

- **Lookup:** `rel = u32_be(0x0638D4 + track_id * 4)`; if `rel != 0`, `seq_rom_addr = 0x0638D4 + rel`.
- **Track `0x00` (`0x0100`):** `rel = 0x000400`, `ROM = 0x063CD4` (`34+` valid tracks extracted).
- **Z80 Handshake (`0x060B68..0x060BC4`):** Requests Z80 bus `$00A11100 = 0x0100`, writes `0x83` to `$00A00003`, releases bus, and polls `$00A00003 == 0`.
