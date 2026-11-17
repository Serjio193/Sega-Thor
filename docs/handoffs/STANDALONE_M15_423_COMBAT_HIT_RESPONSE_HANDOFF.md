# STANDALONE HANDOFF: ROM Entity Combat Hit Response, Knockback & Trajectory Physics Calculator (0x00BE98..0x00C012)

## 1. Routine Overview
- **ROM Range:** `0x00BE98..0x00C012` (378 bytes).
- **Function:** Entity Combat Hit Response, Shield Blocking, Knockback Trajectory & Physics Calculator. Handles enemy HUD / boss health tracking, shield facing detection, guard break vs blocked attacks, damage and hitstun application, sound effect dispatch, 4-way knockback physics (grounded vs airborne air juggle), and shield clash resolution.
- **Evidence JSON:** `data/thor-evidence/rom_combat_hit_response_m15_423.json`.

## 2. Ingress Entry Points
1. `0x00BE98` (`combat_hit_response_tracked`):
   - Checks if target is enemy (`a0 != $FF19E8`) and status protect bit 0 of `$3A` is clear.
   - If so: writes `#$5A` into `$FF184F.l` and `a0` into `$FF1850.l` (HUD target / boss health display).
   - Falls through to `0x00BEBA`.
2. `0x00BEBA` (`combat_hit_response_direct`):
   - Direct hit evaluation without HUD tracking update.

## 3. Algorithm & Logic
1. **Shield Guard & Facing Evaluation:**
   - Tests guard flag `$6D(a0)`. If zero (no shield / idle), proceeds to unblocked hit (`0x00BED4`).
   - If guarding (`$6D(a0) != 0`):
     * Opponent facing check: `opp_facing = attacker_facing ^ 2`.
     * If `opp_facing == target_facing`: target is facing attacker with shield -> branch to blocked hit (`0x00BFAE`).
     * If not equal (flank / backstab): guard broken! Clears `$6D(a0) = 0`, proceeds to unblocked hit.
2. **Unblocked Hit State Application (`0x00BED4`):**
   - Sets attacker backlink: `+0x7E(a0) = a6`.
   - Sets hit cooldown / stun: `+0x82(a0) = stun_duration`, `+0x82(a6) = stun_duration`.
   - Sets damage: `+0x86(a0) = damage_word`.
   - Sets reaction mode: `+0x83(a0) = 6`, `+0x9D(a0) = 0`.
   - Sets facing backlink: `+0x84(a0) = attacker_facing`.
   - Calls hit spark effect: `bsr.w $C012`.
   - Attacker hit flag: `bset #3, $37(a6)`.
   - Plays attack hit sound from descriptor: `d0 = a3[4]`, `bsr.w $E7FC`.
3. **Knockback Physics Dispatcher (`0x00BF14`):**
   - Mode index from bits 14..15 of `damage_word`: `(rol.w #3, damage_word) & 6`.
   - Jump table at `0x00BF26`:
     * **Mode 0 (0x00BF2E - Light Knockback):**
       - If grounded (`$10(a0) == 0`): `h_speed = (0x0300, 0x02C0)`, `v_impulse = 0`.
       - If airborne (`$10(a0) != 0`): sets air juggle flag `bset #7, $86(a0)`, `h_speed = (0x0400, 0x03C0)`, `v_impulse = 0x00028000`.
     * **Mode 1 (0x00BF96 - None):** No knockback velocities applied.
     * **Mode 2 (0x00BF54 - Heavy Knockback):** `h_speed = (0x0400, 0x03C0)`, `v_impulse = 0x00028000`.
     * **Mode 3 (0x00BF72 - Severe Launch):** `h_speed = (0x0700, 0x0680)`, `v_impulse = 0x00030000`.
     * **Vertical Pop-up (0x00BF98):** `h_speed = (0, 0)`, `v_impulse = 0x00070000`.
   - Horizontal velocity vector projected via `bsr.w $E2D4(facing)` into `+0x4E(a0)` (X) and `+0x52(a0)` (Y).
   - Vertical impulse stored into `+0x56(a0)`.
4. **Blocked / Shield Hit Handling (`0x00BFAE`):**
   - Ali weapon durability decrement if attack type in `{6, 10}`.
   - Sets attacker backlink `+0x7E`, cooldown `+0x82`, damage `+0x86`, facing `+0x84`.
   - Calls hit spark effect `bsr.w $C012`.
   - Plays shield block clash sound: `d0 = 0x00900202`, `bsr.w $E7FC`.
   - Zero knockback velocity applied (target remains steady).

## 4. Recommended C++20 Interface
```cpp
namespace thor::combat {

enum class KnockbackMode : uint8_t {
    Light = 0,
    None = 1,
    Heavy = 2,
    Severe = 3,
    VerticalPopup = 4,
};

struct KnockbackPhysics {
    int32_t vel_x{0};
    int32_t vel_y{0};
    int32_t impulse_z{0};
    bool air_juggle{false};
};

bool is_shield_guard_active(int16_t attacker_facing, int16_t target_facing, bool has_guard);

KnockbackPhysics calculate_knockback(KnockbackMode mode, int16_t facing, bool is_airborne);

void apply_combat_hit_response(EntityState& target, EntityState& attacker,
                               uint32_t packed_damage_a4, uint32_t hit_sfx,
                               bool enable_hud_tracking);

} // namespace thor::combat
```
