# Standalone Handoff — M15.351: Ali Jump Trigger, Pickup Probe & 4-Dir Interaction Box (`0x00822C..0x008350`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x00822C..0x0082AC`: `ali_jump_trigger` (`vz = 0x00070000`, running jump state `22` with `+8` boost when `cur_state == 4 && dpad_held`, else standing jump state `16` with zeroed XY velocity)
  - `0x0082AE..0x0082F6`: `ali_pickup_probe` (`±6px` probe for entity `0x08` -> `$FF1976`)
  - `0x0082F8..0x008350`: `ali_interact_probe` (4 directional probe boxes `[x1, y1, x2, y2]` for Up/Right/Down/Left)
- **Evidence JSON:** `data/thor-evidence/rom_ali_jump_pickup_m15_351.json`
- **Python Extractor & Tests:** `tools/extract_rom_ali_jump_pickup.py`, `tests/rom_ali_jump_pickup_test.py`
