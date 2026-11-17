# Standalone Handoff — M15.392: ROM Ali 8-Direction D-Pad Velocity Resolver & Slide Motion Recognizer (`0x0085E2..0x008778`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x0085E2..0x0086B2`: `ali_dpad_8way_velocity_resolver` (decodes 16-entry jump table `0x0085FA` using D-pad low nibble of `$FF165E`: computes fixed-point velocities: Up `(0, -0x30000)`, Down `(0, 0x30000)`, Left `(-0x36000, 0)`, Right `(0x36000, 0)`, Diagonals `(±0x2A000, ±0x25800)`; sets facing `d3` and returns `ccr.C = 1` if moving, `ccr.C = 0` if neutral).
  - `0x0086B4..0x008766`: `ali_motion_slide_recognizer_0x86b4` (official Slide command motion recognizer: scans up to 30 frames from ring `$FF1668`; converts D-pad nibble through 16-byte LUT `0x008768`; tracks 4-step quarter-turn rotational sequence; on full circle/arc match, clears `$FF16EA = 0` and returns `ccr.C = 1`).
  - `0x008768..0x008778`: 16-byte directional encoding LUT `[0xFF, 0, 4, 0xFF, 6, 7, 5, 0xFF, 2, 1, 3, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF]`.
- **Evidence JSON:** `data/thor-evidence/rom_ali_dpad_slide_recognizer_m15_392.json`
- **Python Extractor & Tests:** `tools/extract_rom_ali_dpad_slide_recognizer.py`, `tests/rom_ali_dpad_slide_recognizer_test.py`
