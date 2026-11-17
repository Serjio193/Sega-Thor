# Standalone Handoff — M15.373: ROM Game Over Death Sequence, Banner & Palette Line (`0x005F92..0x0060C8`)

## ROM Source & Evidence
- ROM routine: `0x005F92..0x0060C8`
- Evidence JSON: `data/thor-evidence/rom_game_over_banner_m15_373.json`
- Extractor: `tools/extract_rom_game_over_banner.py`
- Python tests: `tests/rom_game_over_banner_test.py`

## Exact Behavioral Contract
1. **Death Sequence & Post-Death Delay (`0x005F92..0x006008`)**:
   - Prone height `0x10`. When death anim finishes (`bit 1 of +0x37`):
     - Saves continue state via `0x00DAE6`, initializes 40-frame delay timer `$FF1992 = 0x28`.
     - (If continue disabled, triggers immediate Game Over chime `0x00D00355`).
   - Counts down `$FF1992`; upon reaching `0`, triggers Game Over banner transition.
2. **Game Over Graphic Decompression & VDP DMA (`0x00600C..0x006068`)**:
   - Clears plane/shadow buffers (`$FF188A`, `$FF13CC`, `$FF1344`, `$FF1348`, `0x03C956`).
   - Decompresses "GAME OVER" graphic at ROM `0x0541FE` via `0x003820` into RAM `$FF3FAC`.
   - DMAs tile data to VRAM `0x0000` (`command 0x40000003`) via `0x03C98A`.
   - Sets plane mapping table base `0xE000` via `0x00D950`.
3. **Palette Line 3 Load & Fade (`0x006074..0x0060A4`)**:
   - Copies 16 color words from ROM `0x0060CA` to CRAM line 3 `$FF13AC` (`0x0000, 0x08CE, 0x06AC, 0x048A, 0x0268, 0x0046, ...`).
   - Triggers CRAM fade-in via `0x00DAC2`.
4. **Interactive Continue Wait Loop (`0x0060A8..0x0060C6`)**:
   - Polls `$FF165E` waiting for `Start | C | B | A` (`pad & 0x00F0 != 0`).
   - On button press, branches to continue menu handler `0x0060EA`.
