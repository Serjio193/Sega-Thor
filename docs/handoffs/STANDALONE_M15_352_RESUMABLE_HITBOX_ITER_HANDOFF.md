# Standalone Handoff — M15.352: Resumable 21-Slot Masked 3D Hitbox Iterator (`0x00B852..0x00B920`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x00B852..0x00B910`: `find_first_3d_point_hit`
  - `0x00B912..0x00B920`: `find_next_3d_point_hit` (restores saved iterator state from `$FF194A..$FF1969`)
  - `0x00B922..0x00B99E`: `find_first_3d_box_hit` (Z-interval variant)
- **Evidence JSON:** `data/thor-evidence/rom_resumable_hitbox_iter_m15_352.json`
- **Python Extractor & Tests:** `tools/extract_rom_resumable_hitbox_iter.py`, `tests/rom_resumable_hitbox_iter_test.py`
