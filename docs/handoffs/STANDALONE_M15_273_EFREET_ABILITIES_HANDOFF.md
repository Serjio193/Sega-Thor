# STANDALONE M15.273 EFREET ABILITIES & SHARED SP GUARD HANDOFF

**Milestone:** SEGA-THOR HYBRID — M15.273  
**Repository:** `C:/Github/Sega-Thor`  
**Standalone Repository (`C:/Github/Sega-Thor-Standalone`):** STRICTLY READ-ONLY / UNTOUCHED  
**Machine-Readable Evidence Output:** `data/thor-evidence/rom_efreet_abilities_m15_273.json`  
**Extraction Tool:** `tools/extract_rom_efreet_abilities.py`  
**Deterministic Test Suite:** `tests/rom_efreet_abilities_test.py`

---

## 1. Shared Spirit SP-Cost Guard (`0x011B28`)

- Checks `player_sp - cost >= 1` (`cmpi.w #$1, d1; blt`). A spirit ability cannot be cast if it would drop Ali's SP below `1`.

## 2. Efreet (`0x001A`) Abilities (`0x0125CA..0x01267E`)

| Ability | PC | SP Cost | Next State | Action | Notes |
|---|---|---|---|---|---|
| **Fire Dash** | `0x0125CA` | `8` | `0x08` (`0x0126AC`) | `0x0C` | Speed `0x0100`, SFX `0x800258` |
| **Flame Pillar** | `0x012608` | `6` | `0x0A` (`0x0127A4`) | `0x02` | Reserves `$FF1C1C`, calls `0x012C4E` |
| **Meltdown** | `0x01263C` | `25` | `0x0C` (`0x012802`) | `0x08` | Base duration `96` (`0x60`) + `$FF0DB7` level |
| **Dismiss** | `0x012672` | `0` | `0x10` (`0x012988`) | `0x0A` | Dismisses Efreet |
