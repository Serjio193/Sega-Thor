# Standalone Handoff — M15.360: ROM Master Gameplay Frame Loop & Menu Dispatcher (`0x008AA6..0x008C58`)

## ROM Source & Evidence
- ROM routines:
  - `0x008AA6..0x008B8C`: Master gameplay per-frame loop
  - `0x008B8E..0x008BAA`: Queued item / spirit trigger (`$FF1864`)
  - `0x008BAC..0x008C58`: A+B+C hold detector & 4-mode menu dispatcher (`0x003B3E`)
- Evidence JSON: `data/thor-evidence/rom_main_game_loop_m15_360.json`
- Extractor: `tools/extract_rom_main_game_loop.py`
- Python tests: `tests/rom_main_game_loop_test.py`

## Exact Behavioral Contract
1. **Per-Frame Loop Order (`0x008AA6..0x008B8C`)**:
   - Aborts to `0x002934` if demo active (`$FF1996 != 0`) and `$FF1997 == 0`.
   - Advances `$FF0D76 += 2`, `$FF1971 += 1`, `$FF199C += 1`, waits for V-Int sync (`$FF1654`), optionally ticks an extra frame if `$FF17B8` bit 0 is set.
   - Runs conditional HUD update (`0x00A4C8`), then ordered ticks: `0x008BAC -> 0x008B8E -> 0x00F788 -> 0x00557A -> 0x00DEEC -> 0x00E0BA -> 0x00E0F4 -> 0x008E22 -> 0x008E90 -> 0x008DD4 -> 0x00C998 -> (wait V-Int) -> 0x00C980 -> (wait V-Int) -> 0x00A196`.
2. **Queued Trigger (`0x008B8E`)**:
   - If `$FF1864 != 0`: bit 7 -> clears bit 7 and calls `0x007CC6`; else calls `0x00530C`.
3. **Menu Dispatcher (`0x008BAC`)**:
   - A+B+C held (`(pad & 0x70) == 0x70`): sets `$FF19A7 = 0xFF`, clears `$FF16EA = 0`.
   - Guards (`$FF1987`, `$FF17B8` bit 2, `$FF1983`, `$FF1982`): if any set, returns.
   - Priority: Start -> mode `0`; edge bit 2 -> mode `3`; if `$FF19EC <= 2 && $FF1A6E == 0`: edge bit 1 -> mode `1`, edge bit 0 -> mode `2`. Invokes `0x003B3E`.
