# Standalone Handoff: Milestone M15.473 — ROM Entity AI Trigonometric Tracking, Smooth Angle Turn-Rate Limiter & 2D Sin/Cos Velocity Projection Engine

**ROM Range:** `0x010594..0x01074A` (438 bytes)  
**Status:** Reverse-engineered, verified with Capstone M68K disassembly, JSON evidence generated, and unit tests passing.

---

## 1. System Overview

This subsystem implements trigonometric vector mathematics, arctangent angle calculation, smooth turn-rate limiting, cardinal facing quantization, and sin/cos 2D velocity vector projection for entity AI tracking:

1. **`0x010594..0x010660` (`Math_TargetAngleAndVelocityProjector_TurnRate`):**
   - Computes delta vector between entity position `+$08(a6)/+$0C(a6)` and target `(d0, d1)`.
   - Computes angle `0..255` (where 256 units = 360 degrees) via quadrant decomposition and octant arctangent lookup helper `0x010660`.
   - Compares target angle with current entity angle `d2`:
     * Computes clockwise difference `d5 = d2 - d0` and counterclockwise difference `d0 = d0 - d2`.
     * Chooses the shortest angular path.
     * Clamps angular change to max turn rate `8` (`moveq #8, d1`).
     * Accumulates smoothed angle back to `d2`.
   - Quantizes smoothed angle `d2` to 4 cardinal directions:
     * `[0xE0..0xFF]` and `[0x00..0x1F]`: Facing `1` (Right).
     * `[0x20..0x5F]`: Facing `0` (Up).
     * `[0x60..0x9F]`: Facing `3` (Left).
     * `[0xA0..0xDF]`: Facing `2` (Down).
     * Stores facing into `+$16(a6)`.
   - Scales 2D velocity components using speed parameters `(d3, d4)` and 256-entry sin/cos lookup table at ROM `$5D706`:
     * Sine: `sin = ROM_SIN_COS_TABLE[angle * 2]`.
     * Cosine: `cos = ROM_SIN_COS_TABLE[(angle + 0x40) * 2]`.
     * Velocity: `vx = (cos * speed_x) >> 7`, `vy = -(sin * speed_y) >> 7`.
     * Stores `vx` into `+$4E(a6)` and `vy` into `+$52(a6)`.

2. **`0x010660..0x010684` / `0x010726..0x01074A` (`Math_ArctangentLookup_Octant`):**
   - If `dx == 0`: returns `0x40` (90 degrees).
   - Computes fixed-point slope ratio: `ratio = (dy << 8) / dx`.
   - Linear scans 8-entry tangent threshold table at ROM `$5D906`:
     `[25, 78, 137, 210, 312, 479, 844, 2599]`.
   - Each step corresponds to an angle increment of `8` (11.25 degrees).

3. **`0x010684..0x010726` (`Math_TargetAngleAndVelocityProjector_Direct`):**
   - Direct angle snap variant (no turn rate clamp).
   - Computes delta vector, octant arctangent, 4-way facing quantization, and sin/cos velocity projection.

---

## 2. Memory & Architecture Reference

| Symbol | Address | Description |
|---|---|---|
| `ROM_SIN_COS_TABLE` | `0x05D706` | 256-entry sin/cos table with 2-byte stride (sine at even offset, cosine at +0x40) |
| `ROM_TANGENT_OCTANT_TABLE` | `0x05D906` | 8-entry 16-bit word tangent thresholds for octant angles |
| `SLOT_FACING_OFFSET` | `+$16` | Entity current facing direction (0=Up, 1=Right, 2=Down, 3=Left) |
| `SLOT_VEL_X_OFFSET` | `+$4E` | 32-bit fixed velocity X component |
| `SLOT_VEL_Y_OFFSET` | `+$52` | 32-bit fixed velocity Y component |

---

## 3. Cleanroom C++20 Interface

```cpp
namespace thor::math {

enum class CardinalFacing : uint16_t {
    Up = 0,
    Right = 1,
    Down = 2,
    Left = 3
};

struct TrackingVelocityResult {
    int32_t vx{0};
    int32_t vy{0};
    uint8_t smoothed_angle{0};
    CardinalFacing facing{CardinalFacing::Right};
};

uint8_t calculate_target_angle(int16_t dx, int16_t dy);
uint8_t smooth_turn_rate(uint8_t current_angle, uint8_t target_angle, uint8_t max_step = 8);
CardinalFacing quantize_cardinal_facing(uint8_t angle);

TrackingVelocityResult project_tracking_velocity_with_turn_rate(
    int16_t current_x, int16_t current_y,
    int16_t target_x, int16_t target_y,
    uint8_t current_angle,
    int16_t speed_x, int16_t speed_y,
    std::span<const int8_t> sincos_table,
    uint8_t max_turn = 8);

TrackingVelocityResult project_tracking_velocity_direct(
    int16_t current_x, int16_t current_y,
    int16_t target_x, int16_t target_y,
    int16_t speed_x, int16_t speed_y,
    std::span<const int8_t> sincos_table);

} // namespace thor::math
```
