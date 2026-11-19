# STANDALONE HANDOFF: Milestone M15.505 — Boss AI Projectile States $0C/$14/$12, Hit Callbacks & State $0E Hit-Reaction Deceleration

## 1. Subsystem Overview
- **ROM Address Range:** `0x014D1A..0x014F60` (582 bytes, 152 instructions).
- **Core Responsibilities:**
  1. `0x014D1A..0x014DA0` (`AI_Boss_State0C_FeatherSporeProjectile`, 134 bytes, 36 instructions):
     - Feather/Spore projectile kinematics and life scaling.
     - Decrements launch/cooldown timer `+$2(a6)`. When zero, performs AABB intersection query against Ali via `0x00B922` (bounds `[-16..+16, -16..+16, -8..+16]`), invoking hit callback `0x014DA0` on contact.
     - Integrates XY position into `+$72/+$76` from velocities `+$4E/+$52`. Accelerates Z velocity by `+$6000` per frame, accumulating into `+$10`.
     - Scales collision/visual size `+$3D` according to lifetime timer `+$30`:
       - `<= 4 frames`: size 8
       - `5..16 frames`: size 16
       - `17..28 frames`: size 24
       - `> 28 frames`: size 24, asserts blink/shadow flag (bit 3 of `+$36`).
     - Despawns projectile slot `+$00=0` when animation finishes (bit 1 of `+$37`).
  2. `0x014DA0..0x014E0C` (`AI_Boss_HitCallback_FeatherSpore`, 108 bytes, 28 instructions):
     - Hit callback executed against Ali (`a0`): checks invulnerability bit 3 of `+$38(a0)`.
     - Sets attacker pointer `+$7E(a0) = a6`.
     - Damage application: ground target receives `$200C` (12 HP); airborne target (`+$10(a0) != 0`) receives `$A00C` with vertical launch impulse `+$56(a0) = $00028000` (2.5 fixed).
     - Sets hitstun timer `+$83(a0) = 6`, clears `+$9D(a0)`, sets knockback style `+$84(a0) = 2`.
     - Dispatches impact SFX `#$700345` via `0x00E7FC`.
     - Copies XY velocities from projectile to Ali, sets player state register `$FF198E = $0E`, asserts invulnerability bit 3 on Ali, and sets projectile cooldown `+$2(a6) = 10`.
  3. `0x014E0C..0x014E1A` (`AI_Boss_State14_ProjectileFadeout`, 14 bytes, 5 instructions):
     - Projectile impact / fadeout state: monitors animation loop flag bit 1 of `+$37(a6)`; clears `+$00(a6) = 0` to despawn.
  4. `0x014E1A..0x014E86` (`AI_Boss_State12_FireballProjectile`, 108 bytes, 27 instructions):
     - Fireball projectile motion and collision.
     - Cancels to State `$14` impact fadeout (`+$2A = $16, +$4 = $14`) if incoming damage `+$86` is received, if wall collision occurs (bit 6 of `+$40 | +$41`), or if floor contact occurs (`z < 0`, clamped at 0).
     - Performs AABB intersection query against Ali via `0x00B922` (bounds `[-6..+6, -6..+6, -4..+6]`), calling hit callback `0x014E86` on contact before entering fadeout.
     - Integrates 3D position `+$72, +$76, +$10` from velocities `+$4E, +$52, +$56`.
  5. `0x014E86..0x014EE6` (`AI_Boss_HitCallback_Fireball`, 96 bytes, 25 instructions):
     - Hit callback against Ali: sets attacker `+$7E(a0) = a6`, damage `$2008` (ground) or `$A008` (airborne, impulse `$28000`), hitstun `+$83(a0) = 8`.
     - Computes knockback facing angle from projectile velocity via `0x00E338` into `+$84(a0)`.
     - Dispatches impact SFX `#$700345` via `0x00E7FC`.
  6. `0x014EE6..0x014F60` (`AI_Boss_State0E_HitReactionDecel`, 122 bytes, 31 instructions):
     - Boss hit-reaction deceleration and counter-attack switch. Asserts flinch flag bit 6 of `+$36`.
     - Decelerates XY velocities by friction step `$8000` per frame toward 0, integrates position into `+$72/+$76`.
     - Decelerates vertical velocity `+$56` by `$A000` until `<= 0`.
     - While airborne or moving (`(d1 | d2) & $FFFFF000 != 0`), maintains invulnerability bit 3 of `+$38`.
     - When motion completely ceases, clears flinch flag `+$36:6` and checks hit counter `+$B6`:
       - `(+$B6 & 3) == 0` (every 4th hit): counter-attacks via State `$08` (`0x0149FE`, anim 0, timer 10, SFX `$800251`).
       - Otherwise: transitions to State `$10` (`0x014A22`, anim `$12`).

## 2. ROM Evidence Reference
- Evidence JSON: `data/thor-evidence/rom_boss_ai_projectiles_flinch_m15_505.json`
- Extractor: `tools/extract_rom_boss_ai_projectiles_flinch.py`
- Unit Tests: `tests/rom_boss_ai_projectiles_flinch_test.py`

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <cstdint>
#include <algorithm>

namespace Thor::AI::BossProjectiles {

struct FeatherProjectile {
    int16_t status{1};
    int16_t timer{0};
    int32_t x{0};
    int32_t y{0};
    int32_t z{0};
    int32_t x_vel{0};
    int32_t y_vel{0};
    int32_t z_vel{0};
    uint16_t life_timer{0};
    uint8_t size_3d{8};
    uint8_t flags_36{0};
    uint8_t flags_37{0};
};

struct FireballProjectile {
    int16_t status{1};
    uint16_t state{0x12};
    uint16_t anim_id{0x14};
    int32_t x{0};
    int32_t y{0};
    int32_t z{0};
    int32_t x_vel{0};
    int32_t y_vel{0};
    int32_t z_vel{0};
    uint16_t incoming_damage{0};
    uint8_t wall_collision{0};
};

struct PlayerTarget {
    int32_t z{0};
    int32_t x_vel{0};
    int32_t y_vel{0};
    int32_t z_vel{0};
    uint16_t damage_86{0};
    uint8_t hitstun_83{0};
    uint16_t knockback_mode_84{0};
    uint8_t flags_38{0};
    uint8_t player_state_198e{0};
    uint32_t sfx{0};
};

struct FlinchingBoss {
    int32_t x{0};
    int32_t y{0};
    int32_t x_vel{0};
    int32_t y_vel{0};
    int32_t z_vel{0};
    uint16_t hit_counter{0};
    uint8_t flags_36{0};
    uint8_t flags_38{0};
    uint16_t state{0x0E};
    uint16_t anim_id{0x10};
    uint16_t timer{0};
    uint32_t sfx{0};
};

/// Step Feather / Spore projectile kinematics and lifetime size (0x014D1A..0x014D9E).
inline void StepFeather(FeatherProjectile& p) noexcept {
    if (p.timer > 0) {
        --p.timer;
    }
    p.x += p.x_vel;
    p.y += p.y_vel;
    p.z_vel += 0x6000;
    p.z += p.z_vel;

    if (p.life_timer <= 4) {
        p.size_3d = 8;
    } else if (p.life_timer <= 16) {
        p.size_3d = 16;
    } else if (p.life_timer <= 28) {
        p.size_3d = 24;
    } else {
        p.size_3d = 24;
        p.flags_36 |= 0x08; // blink flag
    }

    if ((p.flags_37 & 0x02) != 0) {
        p.status = 0; // Despawn
    }
}

/// Execute Feather hit callback against player (0x014DA0..0x014E0A).
inline bool HitFeatherAli(PlayerTarget& ali, FeatherProjectile& proj) noexcept {
    if ((ali.flags_38 & 0x08) != 0) {
        return false;
    }
    bool airborne = (ali.z != 0);
    ali.damage_86 = airborne ? 0xA00C : 0x200C;
    ali.z_vel = airborne ? 0x28000 : 0;
    ali.hitstun_83 = 6;
    ali.knockback_mode_84 = 2;
    ali.sfx = 0x700345;
    ali.x_vel = proj.x_vel;
    ali.y_vel = proj.y_vel;
    ali.player_state_198e = 0x0E;
    ali.flags_38 |= 0x08;
    proj.timer = 10;
    return true;
}

/// Step Fireball projectile motion, obstacles, and floor contact (0x014E1A..0x014E84).
inline bool StepFireball(FireballProjectile& p, bool hit_ali) noexcept {
    if (p.incoming_damage != 0 || (p.wall_collision & 0x40) != 0 || hit_ali) {
        p.incoming_damage = 0;
        p.state = 0x14; // State $14 fadeout
        p.anim_id = 0x16;
        return false;
    }

    p.x += p.x_vel;
    p.y += p.y_vel;
    p.z += p.z_vel;

    if (p.z < 0) {
        p.z = 0;
        p.state = 0x14;
        p.anim_id = 0x16;
        return false;
    }
    return true;
}

/// Execute Fireball hit callback against player (0x014E86..0x014EE4).
inline bool HitFireballAli(PlayerTarget& ali, const FireballProjectile& proj, uint16_t facing) noexcept {
    if ((ali.flags_38 & 0x08) != 0) {
        return false;
    }
    bool airborne = (ali.z != 0);
    ali.damage_86 = airborne ? 0xA008 : 0x2008;
    ali.z_vel = airborne ? 0x28000 : 0;
    ali.hitstun_83 = 8;
    ali.knockback_mode_84 = facing;
    ali.sfx = 0x700345;
    ali.x_vel = proj.x_vel;
    ali.y_vel = proj.y_vel;
    return true;
}

/// Signed friction helper towards zero by fixed delta.
inline int32_t ApplyFriction(int32_t vel, int32_t step) noexcept {
    if (vel > 0) {
        vel -= step;
        if (vel < 0) vel = 0;
    } else if (vel < 0) {
        vel += step;
        if (vel > 0) vel = 0;
    }
    return vel;
}

/// Step State $0E hit-reaction deceleration and counter-attack switch (0x014EE6..0x014F60).
inline bool StepBossHitReaction(FlinchingBoss& b) noexcept {
    b.flags_36 |= 0x40; // bit 6: flinch
    b.x_vel = ApplyFriction(b.x_vel, 0x8000);
    b.y_vel = ApplyFriction(b.y_vel, 0x8000);
    b.x += b.x_vel;
    b.y += b.y_vel;

    b.z_vel -= 0xA000;
    if (b.z_vel >= 0) {
        b.flags_38 |= 0x08;
        return false; // Still in air
    }

    b.z_vel = 0;
    int32_t residual = (b.x_vel | b.y_vel) & static_cast<int32_t>(0xFFFFF000);
    if (residual != 0) {
        b.flags_38 |= 0x08;
        return false; // Still sliding on ground
    }

    // Completely stopped: clear flinch and trigger counter-attack
    b.flags_36 &= ~0x40;

    if ((b.hit_counter & 3) == 0) {
        b.state = 0x08; // State 8
        b.anim_id = 0x00;
        b.timer = 10;
        b.sfx = 0x800251;
    } else {
        b.state = 0x10; // State $10
        b.anim_id = 0x12;
    }
    return true; // Counter-attack triggered
}

} // namespace Thor::AI::BossProjectiles
```
