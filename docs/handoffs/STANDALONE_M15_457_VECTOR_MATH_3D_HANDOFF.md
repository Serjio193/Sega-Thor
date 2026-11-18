# STANDALONE HANDOFF: Milestone M15.457 — ROM 3D Vector Mathematics Engine: Euclidean Distance (2D/3D), Direction Normalization & Sphere Clamp

## 1. Context & Range
- **Milestone:** M15.457
- **ROM Addresses:** `0x00F054..0x00F1B2` (350 bytes, 135 instructions)
- **Target Subsystem:** Core 3D Vector Math Library & Projectile Launch Velocity Selector:
  * Boss 3A rock projectile launch trajectory and velocity selector (`0x00F054..0x00F0D2`)
  * 4-entry launch velocity table `(speed_xy, launch_z)` at `0x00F0C2`
  * 2D Integer Euclidean Distance Calculator `0x00F0D2` (`sqrt(dx^2 + dy^2)`, 17 callers)
  * 3D Integer Euclidean Distance Calculator `0x00F0EC` (`sqrt(dx^2 + dy^2 + dz^2)`)
  * 3D Vector Direction Normalization & Scaled Velocity Projector `0x00F10C` (`(speed * delta) / dist` with 12 fractional bits)
  * 3D Sphere Ray-Interception Clamp & Surface Projection `0x00F160` (12 callers, returns CCR Carry)
- **Evidence JSON:** `data/thor-evidence/rom_vector_math_3d_m15_457.json`
- **Unit Tests:** `tests/rom_vector_math_3d_test.py`

---

## 2. Subroutines & ROM Architecture

### Subroutine 1: `Boss3A_RockProjectileLaunchSetup` (`0x00F054..0x00F0C2`, 110 bytes)
- Called as callback during Boss 3A stomp/rock attacks.
- Inputs: `a6` = spawner (Boss 3A), `a0` = projectile entity slot.
- Stores spawner pointer: `+$7E(a0) = +$AA(a6)`.
- Calculates delta vector to target: `d1 = +$08(a0) - +$08(a6)`, `d2 = +$0C(a0) - +$0C(a6)`.
- Calls `0x00E338` (`vector_to_cardinal_direction_4way`) to resolve cardinal angle into `d3`, storing to `+$84(a0)`.
- Computes dominant Manhattan distance `d1 = max(|dx|, |dy|)`.
- Checks close-range threshold: `cmpi.l #$80000, d1` (8 pixels in 16.16).
  * If `d1 <= 8px`: selects Entry 3 (`d2 = 3` -> vertical pop / high elevation arch).
  * Else: selects Entry by boss phase `d2 = +$A6(a6)`.
- Reads launch parameters from table `0x00F0C2(pc, d2 * 4)`:
  * Horizontal base speed: `d1.w = speed_xy`.
  * Vertical launch speed: `d2.w = launch_z_word`.
  * Elevation velocity: `ext.l d2; asl.l #8, d2; +$56(a0) = d2`.
- Calls `0x00E2D4` (`vector_directional_projection_8way`) with horizontal speed `d1` and angle `d3`.
- Stores resulting velocities: `+$4E(a0) = vel_x`, `+$52(a0) = vel_y`.

### Subroutine 2: `Boss3A_RockVelocityTable` (`0x00F0C2..0x00F0D2`, 16 bytes)
4 entries of 4 bytes `(uint16_t speed_xy, int16_t launch_z_word)`:
| Index | `speed_xy` | `launch_z_word` | Elevation Velocity `+$56` | Description |
|---|---|---|---|---|
| 0 | `0x0300` | `0x0300` | `+0x00030000` (+3.0 px/frame) | Phase 0: Low speed, low arc |
| 1 | `0x0500` | `0x0400` | `+0x00040000` (+4.0 px/frame) | Phase 1: Medium speed, medium arc |
| 2 | `0x0700` | `0x0500` | `+0x00050000` (+5.0 px/frame) | Phase 2: High speed, high arc |
| 3 | `0x0000` | `0x0800` | `+0x00080000` (+8.0 px/frame) | Close range (`<= 8px`): Pure vertical pop |

### Subroutine 3: `Math_EuclideanDistance2D` (`0x00F0D2..0x00F0EC`, 26 bytes, 17 callers)
- Inputs: `(d1.w, d2.w)` = Point 0 `(x0, y0)`, `(d3.w, d4.w)` = Point 1 `(x1, y1)`.
- Preserves: `d3, d4`.
- Computes:
  `dx = d3 - d1; dy = d4 - d2`
  `sum_sq = dx*dx + dy*dy`
  Calls `0x00E6BA` (`math_isqrt_newton_raphson`).
- Returns: `d0.w = sqrt(dx^2 + dy^2)`.

### Subroutine 4: `Math_EuclideanDistance3D` (`0x00F0EC..0x00F10C`, 32 bytes)
- Inputs: `(d1.w, d2.w, d3.w)` = Point 0, `(d4.w, d5.w, d6.w)` = Point 1.
- Preserves: `d4, d5, d6`.
- Computes:
  `dx = d4 - d1; dy = d5 - d2; dz = d6 - d3`
  `sum_sq = dx*dx + dy*dy + dz*dz`
  Calls `0x00E6BA`.
- Returns: `d0.w = sqrt(dx^2 + dy^2 + dz^2)`.

### Subroutine 5: `Math_VectorNormalize3DVelocity` (`0x00F10C..0x00F160`, 84 bytes)
- Inputs: `(d1.w, d2.w, d3.w)` = P0, `(d4.w, d5.w, d6.w)` = P1, `d0.w` = speed.
- Computes 3D distance `dist`.
- If `dist == 0`: returns `d1 = 0, d2 = 0, d3 = 0`.
- Else:
  `d1 = (speed * dx) / dist`
  `d2 = (speed * dy) / dist`
  `d3 = (speed * dz) / dist`
  Converts to 16.16 fixed point with 1/16 scaling (`(val << 16) >> 4 = val << 12`):
  `swap d1; clr.w d1; asr.l #4, d1`
- Returns: `(d1.l, d2.l, d3.l)` 32-bit fixed-point velocities.

### Subroutine 6: `Math_SphereInterceptionClamp3D` (`0x00F160..0x00F1B2`, 82 bytes, 12 callers)
- Inputs: `(d1.w, d2.w, d3.w)` = Origin P0, `(d4.w, d5.w, d6.w)` = Target P1, `d0.w` = Sphere Radius.
- Computes 3D distance `dist`.
- If `dist <= radius`:
  * Target already within sphere boundary.
  * Preserves `(d4, d5, d6)`.
  * Clears Carry flag: `andi.b #$FE, ccr`.
  * Returns.
- If `dist > radius`:
  * Clamps target to sphere surface along ray `P0 -> P1`:
    `d4 = d1 + (dx * radius) / dist`
    `d5 = d2 + (dy * radius) / dist`
    `d6 = d3 + (dz * radius) / dist`
  * Sets Carry flag: `ori.b #$01, ccr`.
  * Returns clamped `(d4, d5, d6)`.

---

## 3. Recommended C++20 Interface
```cpp
namespace thor::math {

struct Vec3i16 {
    int16_t x{0};
    int16_t y{0};
    int16_t z{0};
};

struct Vec3Fixed {
    int32_t vx{0};
    int32_t vy{0};
    int32_t vz{0};
};

uint16_t distance_2d(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
uint16_t distance_3d(const Vec3i16& p0, const Vec3i16& p1);

Vec3Fixed normalize_velocity_3d(const Vec3i16& p0, const Vec3i16& p1, uint16_t speed);

struct SphereClampResult {
    Vec3i16 point;
    bool clamped{false};
};

SphereClampResult clamp_to_sphere_3d(const Vec3i16& origin, const Vec3i16& target, uint16_t radius);

} // namespace thor::math
```
