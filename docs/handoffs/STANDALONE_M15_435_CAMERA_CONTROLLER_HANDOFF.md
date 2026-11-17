# STANDALONE HANDOFF: M15.435 — ROM Camera Deadzone Tracking, Boundary Clamp & Screen Shake Controller

## 1. Routine Overview
- **Addresses:** `0x00CEEA..0x00D146` (604 bytes)
- **Primary Entrypoints:**
  - `0x00CEEA`: `camera_tracker_update` (Invoked by over 100 map scene scripts; reads Player Ali position, adjusts for elevation, evaluates deadzone panning, screen shake, and motion bias)
  - `0x00CF06`: `camera_tracker_start_panning` (Entrypoint with camera coordinates in registers `d1`, `d2`)
  - `0x00D0C4`: `camera_fixed_axis_direct_follow` (Invoked when room flag `$FF16F1` bit 3 is set; locks camera directly onto player with fixed offsets `(target_x - 160, target_y - 120)`)
  - `0x00D118`: `clamp_camera_viewport_bounds` (Clamps camera viewport within tile map dimensions `[0..(width - 40) * 8]` and `[0..(height - 28) * 8]`)

## 2. Horizontal Deadzone & Panning Logic (`0x00CF26..0x00CFE8`)
- Relative coordinate: `rel_x = player_x - cam_x` (on 320x224 screen).
- **Deadzone bias direction flag (`$FF1859`):**
  - If `rel_x <= 0x50` (80 px): sets `$FF1859 = 0xFF` (facing left); if instant snap `$FF17BA != 0`, sets scroll target `$FF185A = 0x0138` (312).
  - If `rel_x >= 0xF0` (240 px): clears `$FF1859 = 0x00` (facing right); if instant snap `$FF17BA != 0`, sets scroll target `$FF185A = 0x20` (32).
- **Left edge panning:**
  - Active when `rel_x < 0x78` (120 px from left edge).
  - Speed `d0 = rel_x - 120` (negative offset).
  - Smooth speed clamping (when instant snap `$FF17BA == 0`):
    - If `d0 >= -3`: speed = `d0`.
    - Else if `d0 >= -32`: speed = `-3`.
    - Else (`d0 < -32`): fast catch-up `speed = d0 + 30`.
  - Left boundary clamp: `cam_x = max(0, cam_x + speed)`.
- **Right edge panning:**
  - Active when `rel_x > 0xC8` (200 px from left edge).
  - Speed `d0 = rel_x - 200` (positive offset).
  - Smooth speed clamping (when instant snap `$FF17BA == 0`):
    - If `d0 <= 3`: speed = `d0`.
    - Else if `d0 <= 32`: speed = `+3`.
    - Else (`d0 > 32`): fast catch-up `speed = d0 - 30`.
  - Right boundary clamp: `cam_x = min(cam_x + speed, max_cam_x)` from `$FF177A`.

## 3. Vertical Deadzone & Panning Logic (`0x00CFE8..0x00D068`)
- Relative coordinate: `rel_y = (player_y - player_elev_z) - cam_y`.
- **Top edge panning:**
  - Active when `rel_y < 0x68` (104 px from top edge).
  - Speed `d0 = rel_y - 104`.
  - Smooth speed clamping:
    - If `d0 >= -3`: speed = `d0`.
    - Else if `d0 >= -32`: speed = `-3`.
    - Else: fast catch-up `speed = max(-6, d0 + 30)`.
  - Top boundary clamp: `cam_y = max(0, cam_y + speed)`.
- **Bottom edge panning:**
  - Active when `rel_y > 0x80` (128 px from top edge).
  - Speed `d0 = rel_y - 128`.
  - Smooth speed clamping:
    - If `d0 <= 3`: speed = `d0`.
    - Else if `d0 <= 32`: speed = `+3`.
    - Else: fast catch-up `speed = d0 - 30`.
  - Bottom boundary clamp: `cam_y = min(cam_y + speed, max_cam_y)` from `$FF177C`.

## 4. Screen Shake Oscillation Engine (`0x00D068..0x00D098`)
- Uses timer register `$FF17C4`:
  - If `$FF17C4 != 0`:
    - Decrement timer: `timer -= 1`.
    - If `timer` is odd: `offset = +timer`.
    - If `timer` is even: `offset = -timer`.
    - Applies `cam_y += offset`.
    - Clamps `cam_y` to `[0..max_cam_y]`.

## 5. Viewport Tile Map Boundary Clamp Subroutine (`0x00D118..0x00D146`)
- Calculates maximum camera bounds based on map dimensions in 8x8 tiles:
  ```cpp
  int16_t max_x = (map_width_tiles - 40) << 3;
  int16_t max_y = (map_height_tiles - 28) << 3;
  cam_x = std::clamp<int16_t>(cam_x, 0, std::max<int16_t>(0, max_x));
  cam_y = std::clamp<int16_t>(cam_y, 0, std::max<int16_t>(0, max_y));
  ```

## 6. Cleanroom C++20 Interface
```cpp
namespace thor::camera {

struct CameraDeadzoneConfig {
    int16_t left_deadzone{120};
    int16_t right_deadzone{200};
    int16_t top_deadzone{104};
    int16_t bottom_deadzone{128};
    int16_t left_bias_edge{80};
    int16_t right_bias_edge{240};
};

struct CameraState {
    int16_t x{0};
    int16_t y{0};
    int16_t max_x{0};
    int16_t max_y{0};
    int16_t offset_x{0};
    int16_t offset_y{0};
    uint8_t shake_timer{0};
    uint8_t scroll_bias_dir{0};
    uint16_t scroll_bias_target{0};
};

void update_camera_deadzone(
    CameraState& cam,
    int16_t player_x,
    int16_t player_y,
    int16_t player_elev_z,
    bool warp_active,
    bool instant_snap,
    bool direct_follow_mode);

void clamp_camera_viewport(
    CameraState& cam,
    uint16_t map_width_tiles,
    uint16_t map_height_tiles);

} // namespace thor::camera
```
