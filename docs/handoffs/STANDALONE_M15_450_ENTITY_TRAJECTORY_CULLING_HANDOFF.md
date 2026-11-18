# STANDALONE HANDOFF: Milestone M15.450 — ROM Entity Trajectory Gravity Integration, Viewport AABB Culling & 188-Byte Slot Cloner

**ROM Range:** `0x00E502..0x00E6BA` (440 bytes, 122 instructions)  
**Evidence Artifact:** `data/thor-evidence/rom_entity_trajectory_culling_m15_450.json`  
**Reference Extractor:** `tools/extract_rom_entity_trajectory_culling.py`  
**Reference Unit Test:** `tests/rom_entity_trajectory_culling_test.py`

---

## 1. Architectural Summary & System Context

This milestone reverse-engineers the entity physics trajectory integrator (gravity fall, floor impact detection, shockwave trigger), screen and shadow viewport AABB visibility culling tests, the canonical 188-byte (`$BC`) entity slot cloner, and the hitstun/invulnerability cooldown timer.

### Subroutines in this Range:
1. `0x00E502..0x00E5AE` (`entity_integrate_trajectory_gravity_fall` — 22 callers):
   - Reads vertical velocity `d1 = +$56(a6)`.
   - If `d1 == 0`: branches to resting/grounded state handler `0x00E59C` (returns `d0 = 3` or `d0 = 0` if `+$37` bit 1 set).
   - Fixed-point gravity acceleration: `d1 -= 0x0000C000` (subtracted every frame).
   - If velocity reaches exactly 0: adjusts by `subq.w #1, d1`.
   - Stores back: `+$56(a6) = d1`.
   - Elevation integration: `+$10(a6) += d1`.
   - Ground collision test:
     - If `+$10(a6) < 0`:
       - Clears `+$10(a6) = 0`, `+$56(a6) = 0`, `+$2E(a6) = 0`, `+$83(a6) = 0`.
       - Sets landed flag: `bset.b #2, +$39(a6)`.
       - Sets recovery timer: `+$9A(a6) = 0x0F` (15 frames).
       - Tests shockwave trigger: `btst.b #1, +$39(a6)`. If set, clears bit 2 and calls `0x00E7E8` with ID `#$800270`!
       - Returns `d0 = 1` (landed state).
     - Else (`+$10(a6) >= 0`, airborne):
       - Adds `+$4E(a6)` to displacement accumulator `+$72(a6)`.
       - Adds `+$52(a6)` to displacement accumulator `+$76(a6)`.
       - If either velocity non-zero: calls collision iterator `0x00B79A`.
       - Else: if `$FF1971` bit 0 set, increments/wraps facing `+$16(a6) = (+$16 + 1) & 3`.
       - Returns `d0 = 2` (airborne state).

2. `0x00E5AE..0x00E60E` (`entity_viewport_culling_test_screen` — 35 callers):
   - Computes screen position: `screen_x = +$8(a6)`, `screen_y = +$C(a6) - +$10(a6)` (world Y minus elevation).
   - Camera bounds test: compares against `[cam_x - margin, cam_y - margin]` and `[cam_x + 320 + 2*margin, cam_y + 224 + 2*margin]`.
   - Returns Carry clear if visible, Carry set if culled.

3. `0x00E610..0x00E66C` (`entity_viewport_culling_test_shadow` — 18 callers):
   - Computes ground position: `ground_x = +$8(a6)`, `ground_y = +$C(a6)` (shadow position ignoring elevation).
   - Tests against same camera viewport `[320 x 224]` plus margin.
   - Returns Carry clear if visible, Carry set if culled.

4. `0x00E66E..0x00E680` (`entity_slot_copy_188b` — 12 callers):
   - Transfers 94 words (188 bytes / `$BC`) from `(a4)+` to `(a6)+` via `dbra d0, loop` with `d0 = 0x5D`.
   - Canonical entity record copy utility.

5. `0x00E682..0x00E6BA` (`entity_invulnerability_cooldown_tick` — 16 callers):
   - Clears invulnerability bit 6 of `+$36(a6)`.
   - If flash timer `+$9D(a6) > 0` and `< 20`, sets bit 6 to blink entity visibility/invulnerability.
   - If hitstun cooldown `+$82(a6) > 0`:
     - Increments animation frame `+$2E(a6)`.
     - Decrements `+$82(a6)`.
     - If still non-zero, pops return address (`addq.l #4, a7`) to prevent entity action during hitstun!

---

## 2. C++20 Cleanroom API Specification

```cpp
namespace oasis::entity {

enum class TrajectoryState : uint8_t {
    GroundedRest = 0,
    LandedImpact = 1,
    AirborneFlight = 2,
    RestingIdle = 3
};

struct TrajectoryResult {
    TrajectoryState state{TrajectoryState::RestingIdle};
    bool spawn_landing_shockwave{false};
};

constexpr TrajectoryResult integrate_gravity_trajectory(
    int32_t& elev_z, int32_t& vel_z,
    int32_t vel_x, int32_t vel_y,
    int32_t& disp_x, int32_t& disp_y,
    uint8_t& flags_39, uint8_t& recovery_timer_9a) noexcept
{
    if (vel_z == 0) {
        return {TrajectoryState::RestingIdle, false};
    }

    constexpr int32_t GRAVITY_ACCEL = 0x0000C000;
    vel_z -= GRAVITY_ACCEL;
    if (vel_z == 0) {
        vel_z = -1;
    }
    elev_z += vel_z;

    if (elev_z < 0) {
        elev_z = 0;
        vel_z = 0;
        flags_39 |= 0x04U; // bit 2: landed flag
        recovery_timer_9a = 15U;

        bool shockwave = false;
        if ((flags_39 & 0x02U) != 0) { // bit 1: shockwave enabled
            flags_39 &= ~0x04U;
            shockwave = true;
        }
        return {TrajectoryState::LandedImpact, shockwave};
    }

    disp_x += vel_x;
    disp_y += vel_y;
    return {TrajectoryState::AirborneFlight, false};
}

constexpr bool is_visible_in_viewport(int16_t x, int16_t y, int16_t elev_z,
                                     int16_t cam_x, int16_t cam_y, int16_t margin,
                                     bool account_for_elevation = true) noexcept
{
    int16_t screen_y = account_for_elevation ? static_cast<int16_t>(y - elev_z) : y;
    int16_t min_x = cam_x - margin;
    int16_t min_y = cam_y - margin;
    int16_t max_x = cam_x + 320 + 2 * margin;
    int16_t max_y = cam_y + 224 + 2 * margin;

    if (x < min_x || screen_y < min_y) return false;
    if (x > max_x || screen_y > max_y) return false;
    return true;
}

} // namespace oasis::entity
```

---

## 3. Acceptance Criteria

1. **Gravity Parity:** Exact `-0xC000` per frame gravity integration and negative elevation floor impact clamping.
2. **Landing Shockwave:** Correct evaluation of bit 1 on `flags_39` triggering shockwave `#$800270`.
3. **Viewport Culling Parity:** Exact 320x224 bounds checks with margin expansion for both screen and shadow modes.
4. **188-Byte Slot Copy:** Precise 94-word copy matching Beyond Oasis entity structure size `$BC`.
5. All C++ source and test files must strictly stay `<= 500 lines`.
