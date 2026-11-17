# Standalone Handoff: M15.290 — Save/Load File Select Menu (`0x0004C6`..`0x000D30`)

## Summary
Normal gameplay entry (`0x0004C6`) opens the 2x3 File Select grid (`$FF10A8`, `0..5`):
- `0..3`: 4 Save Slots (`$FF10AA` occupied bitmask: `0` -> New Game `0x001300`, `1` -> Continue `0x00161C`).
- `4` (`0x000B04`): **Copy** (aborts if `$FF10AA == 0`; saves source slot in `$FF19A8`; passes `d3 = source_slot` to `0x0006F6` so `dest != src`).
- `5` (`0x000C46`): **Erase** (aborts if `$FF10AA == 0`; plays error SFX `0x00A00253` if target slot is empty).

## Grid Navigation (`0x0006F6`) & Persistence (`0x000682`)
- **D-Pad:** Right sets bit 0 (`cursor += 1` if even), Left clears bit 0 (`cursor -= 1` if odd), Up `cursor -= 2` if `> 1`, Down `cursor += 2` if `< d2` (`4` main, `2` in Copy/Erase). Skips if `cursor == d3` (Copy dest guard). Plays `0x006003A6` on move.
- **Buttons:** Start/A (`0x90`) confirms (SFX `0x00A00256`), B/C (`0x60`) cancels back to Title (`Mode 0x04`).
- **Last-Slot SRAM:** Persists `$FF10A8` to `$2025C9` (`$A130F1` write `1`, lock `2`).

## C++20 Standalone Target
- Implement `FileSelectMenuState` in `src/game/ui/` with 2x3 grid navigation, Copy/Erase guards, and `$2025C9` last-slot persistence.
- Evidence artifact: `data/thor-evidence/rom_file_select_menu_m15_290.json`.
