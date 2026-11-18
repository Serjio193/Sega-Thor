# Standalone Handoff: Milestone M15.472 — ROM Script Virtual Machine Spatial Positioning, Nearest Offscreen Target & Edge Spawner Subsystem

**ROM Range:** `0x0103B0..0x010594` (484 bytes)  
**Status:** Reverse-engineered, verified with Capstone M68K disassembly, JSON evidence generated, and unit tests passing.

---

## 1. System Overview

This subsystem implements spatial positioning strategies for spawned entities in the Cutscene & Event Script Virtual Machine:

1. **`0x0103B0..0x0103BE` (`ScriptSpawn_CopyFixedCoords`):**
   - Copies immediate 16-bit `(X, Y)` coordinates directly from instruction stream `(a0)+` into entity fields `+$08(a6)` and `+$0C(a6)`.
   - Clears Carry flag (`andi.b #$FE, ccr; rts`).

2. **`0x0103BE..0x01046A` (`ScriptSpawn_SelectNearestOffscreenTargetPoint`):**
   - Resolves camera/plane origin from `$FF1716` or `$FF173E` based on `$FF16F0` bit 7.
   - Adjusts viewport margins: `cam_x = plane_x - 0x40`, `cam_y = plane_y - 0x50` (`view_w = 0x1C0`, `view_h = 0x180`).
   - Fetches Player Ali coordinates `(ali_x, ali_y)` from `$FF19F0` / `$FF19F4`.
   - Iterates through 16 coordinate pairs in table `$FF17E6` (loaded by Opcode `$03`):
     * Ignores terminator/empty pairs `($FFFF, $FFFF)`.
     * Filters out on-screen points inside viewport `[cam_x, cam_x + view_w] x [cam_y, cam_y + view_h]`.
     * Computes Manhattan distance to Ali: `abs(pt_x - ali_x) + abs(pt_y - ali_y)`.
     * Tracks the point with minimum distance `d5`.
   - If a valid offscreen point is found:
     * Stores coordinates into `+$08(a6)` and `+$0C(a6)`.
     * Clears Carry flag (`andi.b #$FE, ccr; rts`).
   - If no valid offscreen points exist:
     * Sets Carry flag (`ori.b #1, ccr; rts`) to abort spawn.

3. **`0x01046A..0x010594` (`ScriptSpawn_SelectEdgeSpawnPoint`):**
   - Advances script PC by 4 (`addq.l #4, a0`).
   - Resolves plane origin from `$FF1716` or `$FF173E`.
   - Generates random edge index `0..3` via PRNG `0x00E2A2(4)`.
   - Dispatches via 4-entry relative jump table `0x01049E`:
     * Edge 0 (Top): `pos_x = plane_x + (prng * 8)`, `pos_y = plane_y - 0x50`, `facing = 2` (Down).
     * Edge 1 (Right): `pos_x = plane_x + 0x190`, `pos_y = plane_y + (prng * 8)`, `facing = 3` (Left).
     * Edge 2 (Bottom): `pos_x = plane_x + (prng * 8)`, `pos_y = plane_y + 0x180`, `facing = 0` (Up).
     * Edge 3 (Left): `pos_x = plane_x - 0x50`, `pos_y = plane_y + (prng * 8)`, `facing = 1` (Right).
   - Writes `pos_x` to `+$08(a6)`, `pos_y` to `+$0C(a6)`, `facing` to `+$16(a6)`.
   - Clears Carry flag (`andi.b #$FE, ccr; rts`).

---

## 2. Memory & Architecture Reference

| Symbol | Address | Description |
|---|---|---|
| `PLANE_A_BOUNDS` | `$FF1716` | Background plane / camera bounds structure A |
| `PLANE_B_BOUNDS` | `$FF173E` | Background plane / camera bounds structure B |
| `PLANE_SELECT_FLAG` | `$FF16F0` | Bit 7 selects plane structure ($FF1716 vs $FF173E) |
| `SCRIPT_TARGET_COORDS` | `$FF17E6` | 16 coordinate pair target points loaded by Opcode $03 |
| `PLAYER_ALI_COORDS` | `$FF19F0` | Player Ali world coordinates (X at +$00, Y at +$04) |

---

## 3. Cleanroom C++20 Interface

```cpp
namespace thor::script {

struct Viewport {
    int16_t cam_x{0};
    int16_t cam_y{0};
    int16_t width{0x1C0};
    int16_t height{0x180};
};

struct Point2D {
    int16_t x{0};
    int16_t y{0};
};

struct EdgeSpawnResult {
    int16_t x{0};
    int16_t y{0};
    uint16_t facing{0};
};

bool copy_fixed_coords(
    const uint8_t*& pc,
    Point2D& out_coords);

bool select_nearest_offscreen_point(
    const Viewport& vp,
    const Point2D& ali_pos,
    std::span<const Point2D> target_points,
    Point2D& out_selected_pt);

EdgeSpawnResult select_edge_spawn_point(
    uint8_t edge_idx,
    uint16_t prng_offset,
    int16_t plane_x,
    int16_t plane_y);

} // namespace thor::script
```
