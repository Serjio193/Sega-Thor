# Standalone Handoff — M15.357: ROM 360-Spin & F-B-F Command-Motion Recognizers (`0x0086B4..0x008813`)

## ROM Source & Evidence
- ROM routines:
  - `0x0086B4..0x008777`: 360° Spin-Slash motion recognizer
  - `0x008778..0x008813`: Forward-Back-Forward (F-B-F) command recognizer
- Evidence JSON: `data/thor-evidence/rom_command_motions_m15_357.json`
- Extractor: `tools/extract_rom_command_motions.py`
- Python tests: `tests/rom_command_motions_test.py`

## Exact Behavioral Contract
1. **Shared Input Ring (`$FF1668`, 64 x 2B entries, mod `0x80`)**:
   - Reads `head = $FF16E8`, `count = $FF16EA`. On match, clears `$FF16EA = 0` and returns `true`.
2. **360° Spin-Slash (`0x0086B4..0x008777`)**:
   - Caps scan at `min(count, 30)` samples.
   - Maps low nibble via `pad_to_dir8_lut` (`0=Up, 2=Right, 4=Down, 6=Left`, ignores `-1` & diagonals).
   - Stage 0 locks initial cardinal `d2`; Stage 1 accepts `+2` (CW) or `-2` (CCW) mod 8; Stages 2 & 3 require the next two 90° cardinal steps in the same rotation direction.
3. **F-B-F Motion (`0x008778..0x008813`)**:
   - Rejects (returns `false`) if any sampled high-byte edge contains perpendicular D-pad bits (`4/8` for Up/Down facing, `1/2` for Left/Right facing).
   - Matches 3-step `sequence` masked by `masks` for `facing` (`0: 1->2->1`, `1: 8->4->8`, `2: 2->1->2`, `3: 4->8->4`).
