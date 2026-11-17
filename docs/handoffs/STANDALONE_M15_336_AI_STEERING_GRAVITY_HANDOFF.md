# Standalone Handoff — M15.336: Enemy AI Steering, Fixed-Point Aim & Z-Gravity Physics (`0x00E398..0x00E520`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x00E398..0x00E3E6`: `facing_and_manhattan_to_player` (`0=Up, 1=Right, 2=Down, 3=Left`, `|dx| + |dy|`)
  - `0x00E3E8..0x00E45C`: `steer_4way_toward_player` (sets `0x16` facing, `0x4E`/`0x52` velocity, `0x02` chase timer `max(16, max(|dx|, |dy|))`)
  - `0x00E45E..0x00E4CE`: `aim_fixed_point_vector` (16.16 normalized `2.0 px/frame` major-axis step)
  - `0x00E4D0..0x00E500`: `launch_knockback_arc` (`vx/vy` from `0x00E2D4` with `0x300`/`0x2E0`, `vz = 0x00060000`)
  - `0x00E502..0x00E520`: `step_z_gravity` (`vz -= 0x0000C000`, skips exact `0` via `-1`, lands at `z < 0`)
- **Evidence JSON:** `data/thor-evidence/rom_ai_steering_gravity_m15_336.json`
- **Python Extractor & Tests:** `tools/extract_rom_ai_steering_gravity.py`, `tests/rom_ai_steering_gravity_test.py`
