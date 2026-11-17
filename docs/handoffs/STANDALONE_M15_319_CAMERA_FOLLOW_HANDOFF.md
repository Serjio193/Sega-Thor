# Standalone Handoff — M15.319: Camera Deadzone Follow, Screen Shake & HUD Slide Trigger (`0x00CEEA..0x00D114`)

## Summary
- **Source routine in Beyond Oasis (USA):** `0x00CEEA..0x00D114`
- **Evidence JSON:** `data/thor-evidence/rom_camera_follow_m15_319.json`
- **Python Extractor & Tests:** `tools/extract_rom_camera_follow.py`, `tests/rom_camera_follow_test.py`

## Behavior
1. Target player pos: `(player_x, player_y - ($FF17BB ? 0 : player_z))`.
2. **Deadzone follow (`$FF16F1 & 8 == 0`):**
   - HUD slide threshold check: `rel_x <= 0x50` sets `$FF1859 = 0xFF` (if instant snap, `$FF185A = 0x138`); `rel_x >= 0xF0` clears `$FF1859 = 0` (if instant snap, `$FF185A = 0x20`).
   - X deadzone `[0x78, 0xC8]`, Y deadzone `[0x68, 0x80]`.
   - Smooth ramp when `instant_snap == 0`: `|delta| <= 0x20` clamps to `±3`, else `delta ∓ 0x1E` (upward Y catch-up clamped `>= -6`).
   - Clamped `[0, cam_max_x]` / `[0, cam_max_y]`.
   - Horizontal camera motion sets/clears `$FF1859`.
3. **Locked center mode (`$FF16F1 & 8 != 0`):** snaps to `(player_x + off_x - 0xA0, player_y + off_y - 0x78)`.
4. **Screen shake (`$FF17C4`):** decrements timer and adds `t` (odd) or `-t` (even) to `cam_y`, clamped `[0, cam_max_y]`.
