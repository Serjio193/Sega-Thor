# Standalone Handoff — M15.327: Ring/Spirit Menu Entry Guard & Input Loop (`0x004416..0x00455C`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x004416..0x004438`: `try_open_ring_menu` (requires `$FF1A6E == 0` and `$FF19EC <= 2`, else plays SFX `0x00A00253`)
  - `0x004480..0x00455C`: `ring_menu_input_poll` (`$FF0E50` cursor `(row << 2) | col`, `$FF1996` A/BC swap, close SFX `0x00A00347`)
- **Evidence JSON:** `data/thor-evidence/rom_ring_menu_loop_m15_327.json`
- **Python Extractor & Tests:** `tools/extract_rom_ring_menu_loop.py`, `tests/rom_ring_menu_loop_test.py`
