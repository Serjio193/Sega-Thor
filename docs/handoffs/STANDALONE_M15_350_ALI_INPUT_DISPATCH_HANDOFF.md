# Standalone Handoff — M15.350: Ali 5-Frame Input-Buffer Action & Spirit-Ball Dispatcher (`0x007D8A..0x007E9C`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x007D8A..0x007DC6`: `poll_buffered_input(max_frames)`
  - `0x007DC8..0x007E9C`: `dispatch_ali_buffered_buttons`
- **Evidence JSON:** `data/thor-evidence/rom_ali_input_dispatch_m15_350.json`
- **Python Extractor & Tests:** `tools/extract_rom_ali_input_dispatch.py`, `tests/rom_ali_input_dispatch_test.py`

## Behavior
1. Aborts if `$FF1983 != 0`.
2. ORs up to `5` recent frames from `$FF1668` ring.
3. Priority: `C` (`bit 6` -> summon if `state != 4 && active_spirit == 0 && sp > 0`, state `20`, SFX `0x00900359`), `A` (`bit 4` buffered or held -> attack), `B` (`bit 5` -> jump).
