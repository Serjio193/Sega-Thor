# Standalone Handoff — M15.342: Spatial 2D Audio Attenuation & Stereo Panning (`0x00E78E..0x00E90C`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x00E78E..0x00E7E6`: `play_spatial_at_xy`
  - `0x00E7E8..0x00E90C`: `play_spatial_at_entity`
- **Evidence JSON:** `data/thor-evidence/rom_spatial_audio_pan_m15_342.json`
- **Python Extractor & Tests:** `tools/extract_rom_spatial_audio_pan.py`, `tests/rom_spatial_audio_pan_test.py`

## Behavior
- `dist = max(|dx|, |dy| * 1.25)`.
- `sound_id < 0x200`: unattenuated center.
- `0x200 <= sound_id < 0x300`: 2-step attenuation at `176px` (`0xB0`) / `224px` (`0xE0`), culled at `>= 272px` (`0x110`), stereo pan (`0x40` Left / `0x80` Right) at `|dx| >= 96px` (`0x60`).
- `sound_id >= 0x300`: `drop = ((max(0, dist - 88) >> 5) + 1)` (culled if `> 7`), stereo pan at `|dx| >= 64px` (`0x40`).
