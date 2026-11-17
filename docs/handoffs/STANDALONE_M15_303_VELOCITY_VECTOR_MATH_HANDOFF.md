# Standalone Handoff: M15.303 — 8-Direction Fixed-Point Velocity Vector (`0x00E2D4`..`0x00E36E`)

## Summary
- **`0x00E2D4` (`compute_8dir_velocity`):** Converts `u16 (sx, sy)` to `16.16` fixed-point `(sx << 8, sy << 8)` and dispatches `dir & 7` via `0x00E2F2`. Cardinal directions (`0=N, 1=E, 2=S, 3=W`) use full speed; diagonals (`4=NE, 5=SE, 6=SW, 7=NW`) apply **`0.75` scaling** via `(v >> 1) + (v >> 2)`.
- **`0x00E338` (`compute_cardinal_facing`):** Maps `(vx, vy)` to cardinal facing `0..3` (`|vx| <= |vy|`: `vy < 0 ? 0 : 2`; else `vx < 0 ? 3 : 1`).

## C++20 Standalone Target
- Implement `compute_8dir_velocity()` and `compute_cardinal_facing()` in `src/game/physics/` (or `src/game/entities/`).
- Evidence artifact: `data/thor-evidence/rom_velocity_vector_math_m15_303.json`.
