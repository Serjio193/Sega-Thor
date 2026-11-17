# Standalone Handoff — M15.395: Master Gameplay Frame Sequencer, Screen Shake Dispatcher & Joypad Menu Hotkeys (`0x008AA6..0x008C5A`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x008AA6..0x008B8C`: `master_frame_sequencer` (executes 15-phase gameplay frame pipeline; checks demo flags `$FF1996`/`$FF1997`; increments global tick `$FF0D76` by 2 and anim tick `$FF1971` by 1; synchronizes with V-Int tick `$FF1654`; handles slow motion `$FF17B8` bit 0; coordinates Pause Menu `0xA4C8`, Input `0x8BAC`, Shake `0x8B8E`, Event VM `0xF788`, Dialogue `0x557A`, Mana `0xDEEC`, Weapon `0xE0BA`, Hazard `0xE0F4`, Player `0x8E22`, Entities `0x8E90`, GC `0x8DD4`, Camera `0xC998`, Plane Scroll `0xC980`, SAT `0xA196`)
  - `0x008B8E..0x008BAA`: `screen_shake_dispatcher` (reads `$FF1864`; clears bit 7; if bit 7 clear, branches to palette flash `0x530C`; if bit 7 set, branches to earthquake shake `0x7CC6`)
  - `0x008BAC..0x008C58`: `joypad_menu_hotkeys` (tests A/B/C release mask `0x70` in `$FF165E` to reset `$FF19A7 = 0xFF` and clear `$FF16EA = 0`; tests input locks `$FF1987`, `$FF17B8` bit 2, `$FF1983`, `$FF1982`; Start button `$FF165F` bit 7 triggers menu `0x3B3E` with `$FF185E = 0`; 6-button controller `$FF1660` hotkeys: bit 2 triggers Map/Status `$FF185E = 3`, bit 1 triggers Item ring `$FF185E = 1` [when Ali state <= 2 and spirit idle], bit 0 triggers Weapon ring `$FF185E = 2` [when Ali state <= 2 and spirit idle])
- **Evidence JSON:** `data/thor-evidence/rom_master_frame_sequencer_m15_395.json`
- **Python Extractor & Tests:** `tools/extract_rom_master_frame_sequencer.py`, `tests/rom_master_frame_sequencer_test.py`
