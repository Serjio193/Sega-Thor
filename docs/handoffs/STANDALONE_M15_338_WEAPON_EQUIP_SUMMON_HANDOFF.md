# Standalone Handoff — M15.338: Spirit Summon Trigger & Weapon Quick-Equip Class Mapper (`0x007E9E..0x007F80`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x007E9E..0x007EC2`: `start_spirit_summon` (sets Ali state `0x14` and plays `0x00900359`)
  - `0x007EC4..0x007F80`: `quick_equip_weapon_slot` (toggles back to slot `0` if re-selecting equipped slot, maps item ID to weapon class `0x0A`/`0x0C`/`0x0E`/`0x10`, sets 60-frame HUD toast, plays `0x00C0025D`)
- **Evidence JSON:** `data/thor-evidence/rom_weapon_equip_summon_m15_338.json`
- **Python Extractor & Tests:** `tools/extract_rom_weapon_equip_summon.py`, `tests/rom_weapon_equip_summon_test.py`
