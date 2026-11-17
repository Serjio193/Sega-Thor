# Standalone Handoff — M15.341: Integer Square Root & 256-Step `atan2` Angle LUT (`0x00E6BA..0x00E78C`, `0x05D906`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x00E6BA..0x00E6EE`: `isqrt_u32(val)` (Heron's method, initial guess `min(val, 0x0400)`)
  - `0x00E6F0..0x00E766`: `atan2_to_point` / `atan2_to_player` (`0..255` in steps of `8`, `0`=Right, `64`=Up, `128`=Left, `192`=Down)
  - `0x00E768..0x00E78C`: `atan_q1` using 9-word tangent LUT at `0x0005D906`: `{0x0019, 0x004E, 0x0089, 0x00D2, 0x0138, 0x01DF, 0x034C, 0x0A27, 0xFFFF}`
- **Evidence JSON:** `data/thor-evidence/rom_isqrt_atan2_m15_341.json`
- **Python Extractor & Tests:** `tools/extract_rom_isqrt_atan2.py`, `tests/rom_isqrt_atan2_test.py`
