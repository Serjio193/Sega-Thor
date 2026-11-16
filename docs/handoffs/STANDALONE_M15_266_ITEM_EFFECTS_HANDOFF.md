# STANDALONE M15.266 CONSUMABLE ITEM EFFECTS & SPIRIT SUMMON DISPATCH HANDOFF

**Milestone:** SEGA-THOR HYBRID — M15.266  
**Repository:** `C:/Github/Sega-Thor`  
**Standalone Repository (`C:/Github/Sega-Thor-Standalone`):** STRICTLY READ-ONLY / UNTOUCHED  
**Machine-Readable Evidence Output:** `data/thor-evidence/rom_item_effects_m15_266.json`  
**Extraction Tool:** `tools/extract_rom_item_effects.py`  
**Deterministic Test Suite:** `tests/rom_item_effects_test.py`

---

## 1. Eligibility Check (`0x005236`)

Given `item_id` (`$FF0D90 + $FF0E51 * 2`):
- Reject if current area (`$FF0E40`) == `0x10`.
- **`0x1A..0x1D` (Spirit Summon Rings):** requires `$FF16F1:0 == 0`, `$FF1AA4 == 0` (no active spirit), `$FF1A76 != 0` (SP > 0), and `($FF0D7A & (1 << (item_id - 0x1A))) != 0`.
- **`0x1E..0x21` (HP Food):** requires `hp < max_hp` (`$FF1A72 != $FF1A70`).
- **`0x22..0x25` (SP Potions):** requires `sp < max_sp` (`$FF1A76 != $FF1A74`).
- **`0x26..0x2A` (Dual Elixirs):** requires `hp < max_hp || sp < max_sp`.

---

## 2. Effect Dispatcher (`0x00530C`, Jump Table `0x00532C`)

- **`0x1A..0x1D`:** Restores SP to Max (`$FF1A76 = $FF1A74`), sets `$FF198E = 0x14`, `$FF19A6 = item_id - 0x1A` (0=Dytto, 1=Efreet, 2=Shade, 3=Bow), and calls `0x007E9E`.
- **`0x1E..0x21` (HP Food):** Adds `25%` (`MaxHP >> 2`), `50%` (`MaxHP >> 1`), `75%` (`(MaxHP >> 1) + (MaxHP >> 2)`), or `100%` (`MaxHP`) clamped to `MaxHP`, plays SFX `0xA00234`.
- **`0x22..0x25` (SP Potions):** Adds `25%`, `50%`, `75%`, or `100%` of `MaxSP` clamped to `MaxSP`, plays SFX `0xA00234`.
- **`0x26..0x2A` (Dual Elixirs):** Applies both HP and SP restoration (`25%`, `50%`, `75%`, `100%`, `100%`).
