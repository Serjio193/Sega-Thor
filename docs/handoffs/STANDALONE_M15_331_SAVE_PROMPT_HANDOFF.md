# Standalone Handoff — M15.331: In-Game Save Prompt & Save-Allowed Guard (`0x005472..0x005578`)

## Summary
- **Source routine in Beyond Oasis (USA):** `0x005472..0x005578`
- **Evidence JSON:** `data/thor-evidence/rom_save_prompt_m15_331.json`
- **Python Extractor & Tests:** `tools/extract_rom_save_prompt.py`, `tests/rom_save_prompt_test.py`

## Behavior
1. **Entry Guard:** Requires `$FF1996 == 0`, `$FF17C1 == 0`, `$FF19EC <= 2`, and `($FF16F1 & 0x40) != 0` (or debug override); else plays `0x00A00253`.
2. **YES/NO Prompt (`$FF1862`):** Starts at `4` (NO). Up (`bit 0`) switches `4 -> 3` (YES), Down (`bit 1`) switches `3 -> 4` (NO) with SFX `0x006003A5`.
3. **Confirm/Cancel:** `A` (`bit 4`) on `3` plays `0x00A003AA` and invokes SRAM save (`0x001726`); `B`/`C` or `A` on `4` plays `0x00A00347` and cancels.
