# Standalone Handoff — M15.358: ROM Chained F-B-F + 360° Spin Super-Motion Recognizer (`0x008814..0x00890E`)

## ROM Source & Evidence
- ROM routine: `0x008814..0x00890E`
- Evidence JSON: `data/thor-evidence/rom_chained_super_motion_m15_358.json`
- Extractor: `tools/extract_rom_chained_super_motion.py`
- Python tests: `tests/rom_chained_super_motion_test.py`

## Exact Behavioral Contract
1. **Unbounded Ring Scan (`$FF16EA`)**: Unlike the standalone 360° recognizer (`0x0086B4`) which caps at 30 frames, `0x008814` scans the full `$FF16EA` sample count.
2. **Phase 1 — F-B-F (`0x008814..0x00887C`)**:
   - Aborts immediately (`false`) if any high-byte edge contains perpendicular D-pad bits (`1/2` for horizontal facing, `4/8` for vertical facing).
   - Matches the 3-step F-B-F sequence from `0x0087F4 + facing*8`.
3. **Phase 2 — Seamless 360° Spin (`0x00887E..0x00890E`)**:
   - Continues from the exact ring index `d5` and remaining count `d7` where Phase 1 finished (note: Phase 1's final matching sample is re-evaluated as the first sample of Phase 2!).
   - Runs the 4-stage CW/CCW 90° cardinal matcher using `0x008768`; on completion clears `$FF16EA = 0` and returns `true`.
