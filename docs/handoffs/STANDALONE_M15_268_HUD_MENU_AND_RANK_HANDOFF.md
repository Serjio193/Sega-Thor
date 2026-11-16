# STANDALONE M15.268 PAUSE RING MENU & PLAYER RANK SCALING HANDOFF

**Milestone:** SEGA-THOR HYBRID — M15.268  
**Repository:** `C:/Github/Sega-Thor`  
**Standalone Repository (`C:/Github/Sega-Thor-Standalone`):** STRICTLY READ-ONLY / UNTOUCHED  
**Machine-Readable Evidence Output:** `data/thor-evidence/rom_hud_menu_and_rank_m15_268.json`  
**Extraction Tool:** `tools/extract_rom_hud_menu_and_rank.py`  
**Deterministic Test Suite:** `tests/rom_hud_menu_and_rank_test.py`

---

## 1. Pause Ring Menu Rendering (`0x0042E8..0x0043AC`)

- **Icon Tileset:** ROM `0x00155D76` (`3x3 = 9` tiles per icon).
- **Weapon Ring (`0x0042E8`):** 8 slots from `$FF0D80`, 24px spacing, cursor `$FF0E50`.
- **Item Ring (`0x004388`):** 16 slots from `$FF0D90`, 24px spacing, cursor `$FF0E51`.

## 2. Player Rank & MaxHP Scaling (`0x007BF6..0x007C1C`)

- **Rank (`$FF1A7E`):** Capped at `200` (`0xC8`).
- **MaxHP (`$FF1A70`):** `198 + 2 * Rank` (`0xC6 + 2 * Rank`). On rank-up, Current HP (`$FF1A72`) also increases by `+2`.
- **SP Pickup (`0x007BCA`):** Adds `+5` to Current SP (`$FF1A76`).
- **Gem / Upgrade Counters (`0x007BD4`):** `$FF0DB2 + (item_id - 0x0D)`.
