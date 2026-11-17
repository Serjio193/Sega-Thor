# Standalone Handoff — M15.326: 5-Digit Decimal-to-ASCII Formatter (`0x00C0BE..0x00C178`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x00C0BE..0x00C112`: `format_u16_dec_zero_padded(uint16 val)` -> 5 ASCII digits at `$FF192A` (`"00000"`..`"65535"`).
  - `0x00C114..0x00C178`: `format_u16_dec_space_padded(uint16 val)` -> 5 space-padded ASCII chars at `$FF192A` (`"    0"`..`"65535"`).
- **Evidence JSON:** `data/thor-evidence/rom_decimal_ascii_m15_326.json`
- **Python Extractor & Tests:** `tools/extract_rom_decimal_ascii.py`, `tests/rom_decimal_ascii_test.py`
