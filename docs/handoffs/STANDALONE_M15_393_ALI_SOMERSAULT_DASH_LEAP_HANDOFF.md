# Standalone Handoff — M15.393: ROM Ali Somersault Flip (0x8778) & Compound Dash Leap (0x8814) Motion Recognizers (`0x008778..0x008910`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x008778..0x0087F2`: `ali_motion_somersault_0x8778` (Somersault flip motion recognizer: reads player facing `$16(a6)`; indexes 32-byte pattern table `0x0087F4`; checks 3-step sequence Forward -> Backward -> Forward; clears `$FF16EA = 0` and returns `ccr.C = 1` on match).
  - `0x0087F4..0x008814`: 32-byte 4-direction motion pattern table:
    * Facing 0 (Up): `[1, 2, 1, 0, 15, 2, 15, 0]` (Up -> Down -> Up)
    * Facing 1 (Right): `[8, 4, 8, 0, 15, 4, 15, 0]` (Right -> Left -> Right)
    * Facing 2 (Down): `[2, 1, 2, 0, 15, 1, 15, 0]` (Down -> Up -> Down)
    * Facing 3 (Left): `[4, 8, 4, 0, 15, 8, 15, 0]` (Left -> Right -> Left)
  - `0x008814..0x00890E`: `ali_motion_dash_leap_0x8814` (Compound Dash Leap motion recognizer: matches 3-step lead-in from table `0x0087F4`, then chains into 360-degree rotational slide sequence `0x00887E` across up to 30 frames; clears `$FF16EA = 0` and returns `ccr.C = 1` on match).
- **Evidence JSON:** `data/thor-evidence/rom_ali_somersault_dash_leap_m15_393.json`
- **Python Extractor & Tests:** `tools/extract_rom_ali_somersault_dash_leap.py`, `tests/rom_ali_somersault_dash_leap_test.py`
