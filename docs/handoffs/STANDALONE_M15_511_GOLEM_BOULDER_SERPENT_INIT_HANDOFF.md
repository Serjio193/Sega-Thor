# STANDALONE HANDOFF: Milestone M15.511 — ROM Golem Boulder/Debris Projectile Ballistics & Composite Serpent/Multi-Segment Boss Constructor

## 1. Subsystem Overview
- **ROM Address Range:** `0x0159E2..0x015C4A` (616 bytes, 135 M68K instructions across 4 routines).
- **Core Responsibilities:**
  1. `0x0159E2..0x015A8C` (`AI_GolemBoulder_Tick`, 170 bytes, 41 instructions):
     - Boulder flight physics, gravity, and collision dispatcher.
     - State `+$4 != 0`: checks animation finished flag `+$37:1`; when set, despawns entity (`+$0 = 0`).
     - State `+$4 == 0`:
       - Integrates horizontal and vertical velocities (`+$4E`, `+$52`) into subpixel positions (`+$72`, `+$76`).
       - Gravity step: subtracts `0x0000C000` from vertical launch velocity `+$56`, then integrates into Z position `+$10`.
       - If `+$10 < 0`: clamps `+$10 = 0`, transitions to State 2 (`+$4 += 2`), animation ID `$0E` (shatter).
       - In flight (`+$10 >= 0`): executes AABB collision check vs Ali (`[-8..+8, -8..+8]`) via `0x00B922`, triggering Hit Callback `0x015A8C` on contact.
       - Terrain/environment collision evaluation (`+$40 | +$41`):
         - Bit 6 set (wall collision): if `(+$41 & 0x13) == 0x12`, performs Z-bounce (`+$10 -= +$52`); otherwise shatters (State 2, anim `$0E`).
         - Bit 7 set (entity collision): if colliding entity `+$68` is Ali (`$FF19E8`), shatters (State 2, anim `$0E`).
  2. `0x015A8C..0x015AE2` (`AI_GolemBoulder_HitCallback`, 86 bytes, 21 instructions):
     - Boulder collision hit callback. Enters State 2 (`+$4 += 2`), animation `$0E`.
     - Inspects Ali invulnerability flag `+$38:3` and guard state `+$82`:
       - If invulnerable or guarding: aborts without dealing damage or knockback.
       - If vulnerable: stores attacker back-pointer `+$7E(a0) = a6`, clears guard `+$82` and `+$9D`, inflicts 6 HP damage (`+$86 = 0x8006`), knockback mode 3 (`+$84 = 3`), horizontal knockback `+$4E = -0x00040000` (-4.0 px), vertical impulse `+$56 = 0x00030000` (+3.0 px), and triggers impact SFX `0x800207` via audio engine `0x00060004`.
  3. `0x015AE2..0x015AFE` (`AI_CompositeSerpent_SubModeRouter`, 28 bytes, 8 instructions):
     - Composite Serpent/Multi-segment boss sub-mode dispatcher. Evaluates `+$66(a6)`:
       - Sub-mode 0 $\to$ `0x016294`
       - Sub-mode 1 $\to$ `0x015AFE` (9-slot composite constructor)
       - Sub-mode 2 $\to$ `0x015CCA`
       - Sub-mode 3 $\to$ `0x015CB6`
  4. `0x015AFE..0x015C4A` (`AI_CompositeSerpent_InitCompositeSegments`, 332 bytes, 65 instructions):
     - Root preparation: sets `+$66 += 3`, `+$10 = 0`, facing `+$16 = 1`, sprite attributes `+$18 = 0x4200`. Stores root pointer in `a4`.
     - Allocates 9 sequential child entity slots in Pool 2 using stride `+$BC` (188 bytes) via `CopyParentEntityAttributes` (`0x00E66E`):
       - Slot 1: Body 1, anim `$3A`, facing `+$16 += 2` (facing 3).
       - Slot 2: Body 2, anim `$3A`.
       - Slot 3: Body 3, anim `$3A`, facing `+$16 += 2` (facing 5).
       - Slot 4: Body 4, anim `$3A`.
       - Slot 5: HEAD SEGMENT! Anim `$3C`, sub-mode `+$66 = 2`, size `+$3D = 16`, bounds width 8, depth 6, height 16, extent 4, position `(x - 32, y + 40)`, current HP 20, max HP 20, VRAM tile base `$0200`.
       - Slot 6: Tail/Orb 1, anim `$2C`, size `+$3D = 8`, tile base `$0200`.
       - Slot 7: Tail/Orb 2, anim `$2E`, size `+$3D = 0`, tile base `$0200`.
       - Slot 8: Tail/Orb 3, anim `$30`, size `+$3D = 0`, tile base `$0200`.
       - Slot 9: Tail/Orb 4, anim `$30`, size `+$3D = 0`, tile base `$0200`.
     - Restores root entity pointer (`a6 = a4`): clears sub-mode `+$66 = 0`, facing `+$16 = 0`, sets State `$18` (`+$4 = 0x18`), anim ID 4, HP 50, max HP 50, clears `+$B0`, and jumps to entity engine entry point `0x02ED20`.

## 2. ROM Evidence Reference
- Evidence JSON: `data/thor-evidence/rom_golem_boulder_serpent_init_m15_511.json`
- Extractor: `tools/extract_rom_golem_boulder_serpent_init.py`
- Unit Tests: `tests/rom_golem_boulder_serpent_init_test.py`

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <cstdint>
#include <array>
#include <optional>
#include <tuple>

namespace Thor::AI::GolemSerpent {

struct BoulderEntity {
    uint16_t active_0{0x0001};
    int16_t state{0};
    int16_t anim_id{0};
    uint8_t flags_37{0};
    int32_t x_accum{0};
    int32_t y_accum{0};
    int32_t x_vel{0};
    int32_t y_vel{0};
    int32_t z_vel{0};
    int32_t z{0};
};

struct PlayerTarget {
    uint8_t flags_38{0};
    bool guard_82{false};
    uint16_t damage_86{0};
    uint8_t knockback_mode_84{0};
    int32_t x_vel{0};
    int32_t y_vel{0};
    int32_t z_vel{0};
    uint32_t sfx{0};
    const BoulderEntity* attacker{nullptr};
};

enum class BoulderAction {
    InFlight,
    Despawned,
    GroundShatter,
    WallBounce,
    WallShatter,
    EntityShatter
};

enum class HitResult {
    None,
    DamageDealt,
    ImmuneBlocked
};

/// Emulates Hit Callback (0x015A8C..0x015AE2).
inline HitResult ApplyBoulderHit(BoulderEntity& boulder, PlayerTarget& ali) noexcept {
    boulder.state += 2;
    boulder.anim_id = 0x0E;

    // Check Ali invulnerability (+0x38:3) or active guard (+0x82)
    bool is_immune = ((ali.flags_38 & 0x08) != 0) || ali.guard_82;
    if (is_immune) {
        return HitResult::ImmuneBlocked;
    }

    ali.attacker = &boulder;
    ali.guard_82 = false;
    ali.damage_86 = 0x8006;  // 6 HP
    ali.knockback_mode_84 = 3;
    ali.x_vel = -0x00040000; // -4.0 px
    ali.y_vel = 0;
    ali.z_vel = 0x00030000;  // +3.0 px
    ali.sfx = 0x800207;
    return HitResult::DamageDealt;
}

/// Emulates Boulder Flight Physics and Collision Dispatcher (0x0159E2..0x015A8C).
template <typename AABBCheckFn>
inline std::pair<BoulderAction, HitResult> StepBoulderTick(
    BoulderEntity& boulder,
    PlayerTarget& ali,
    uint16_t terrain_flags,
    uint32_t colliding_entity_addr,
    AABBCheckFn&& aabb_fn
) noexcept {
    if (boulder.state != 0) {
        if ((boulder.flags_37 & 0x02) != 0) {
            boulder.active_0 = 0;
            return {BoulderAction::Despawned, HitResult::None};
        }
        return {BoulderAction::InFlight, HitResult::None};
    }

    // Accumulate velocities
    boulder.x_accum += boulder.x_vel;
    boulder.y_accum += boulder.y_vel;

    // Apply gravity -$C000 to Z velocity
    boulder.z_vel -= 0x0000C000;
    boulder.z += boulder.z_vel;

    if (boulder.z < 0) {
        boulder.z = 0;
        boulder.state += 2;
        boulder.anim_id = 0x0E;
        return {BoulderAction::GroundShatter, HitResult::None};
    }

    HitResult hit = HitResult::None;
    if (aabb_fn()) {
        hit = ApplyBoulderHit(boulder, ali);
    }

    uint8_t flag_40 = static_cast<uint8_t>(terrain_flags & 0xFF);
    uint8_t flag_41 = static_cast<uint8_t>((terrain_flags >> 8) & 0xFF);
    uint8_t merged = flag_40 | flag_41;

    if ((merged & 0x40) != 0) {
        // Wall collision (bit 6)
        if ((flag_41 & 0x13) == 0x12) {
            boulder.z -= boulder.y_vel; // Z-bounce
            return {BoulderAction::WallBounce, hit};
        } else {
            boulder.state += 2;
            boulder.anim_id = 0x0E;
            return {BoulderAction::WallShatter, hit};
        }
    }

    if ((merged & 0x80) != 0) {
        // Entity collision (bit 7)
        if (colliding_entity_addr == 0x00FF19E8) {
            boulder.state += 2;
            boulder.anim_id = 0x0E;
            return {BoulderAction::EntityShatter, hit};
        }
    }

    return {BoulderAction::InFlight, hit};
}

/// Emulates Sub-Mode Dispatcher (0x015AE2..0x015AFC).
inline std::optional<uint32_t> RouteSerpentSubMode(uint8_t sub_mode_66) noexcept {
    switch (sub_mode_66) {
        case 0: return 0x016294;
        case 1: return 0x015AFE;
        case 2: return 0x015CCA;
        case 3: return 0x015CB6;
        default: return std::nullopt;
    }
}

struct SerpentSegment {
    uint8_t slot_index{0};
    uint16_t anim_id{0};
    uint16_t facing{0};
    uint8_t sub_mode_66{0};
    uint8_t size_3d{0};
    uint8_t bounds_w{0};
    uint8_t bounds_d{0};
    uint16_t bounds_h{0};
    uint16_t bounds_extent{0};
    int16_t x{0};
    int16_t y{0};
    uint16_t hp{0};
    uint16_t max_hp{0};
    uint16_t vram_tile_18{0};
};

struct SerpentRootEntity {
    int16_t x{0};
    int16_t y{0};
    int32_t z{0};
    uint8_t sub_mode_66{0};
    uint16_t facing_16{0};
    uint16_t sprite_attr_18{0};
    uint16_t state{0};
    uint16_t anim_id{0};
    uint16_t hp{0};
    uint16_t max_hp{0};
    uint16_t flag_b0{0};
    uint32_t exit_target{0};
};

/// Emulates 9-Slot Composite Serpent Constructor (0x015AFE..0x015C4A).
inline std::array<SerpentSegment, 9> InitCompositeSerpent(SerpentRootEntity& root) noexcept {
    root.sub_mode_66 += 3;
    root.z = 0;
    root.facing_16 = 1;
    root.sprite_attr_18 = 0x4200;

    uint16_t current_facing = root.facing_16;
    std::array<SerpentSegment, 9> segments{};

    // Slot 1: Body 1
    current_facing += 2; // 3
    segments[0] = {1, 0x3A, current_facing, 0, 0, 0, 0, 0, 0, root.x, root.y, 0, 0, root.sprite_attr_18};

    // Slot 2: Body 2
    segments[1] = {2, 0x3A, current_facing, 0, 0, 0, 0, 0, 0, root.x, root.y, 0, 0, root.sprite_attr_18};

    // Slot 3: Body 3
    current_facing += 2; // 5
    segments[2] = {3, 0x3A, current_facing, 0, 0, 0, 0, 0, 0, root.x, root.y, 0, 0, root.sprite_attr_18};

    // Slot 4: Body 4
    segments[3] = {4, 0x3A, current_facing, 0, 0, 0, 0, 0, 0, root.x, root.y, 0, 0, root.sprite_attr_18};

    // Slot 5: Head
    segments[4] = {
        5, 0x3C, current_facing, 2, 0x10, 8, 6, 16, 4,
        static_cast<int16_t>(root.x - 0x20),
        static_cast<int16_t>(root.y + 0x28),
        20, 20, 0x0200
    };

    // Slot 6: Tail Orb 1
    segments[5] = {6, 0x2C, current_facing, 0, 8, 0, 0, 0, 0, root.x, root.y, 0, 0, 0x0200};

    // Slot 7: Tail Orb 2
    segments[6] = {7, 0x2E, current_facing, 0, 0, 0, 0, 0, 0, root.x, root.y, 0, 0, 0x0200};

    // Slot 8: Tail Orb 3
    segments[7] = {8, 0x30, current_facing, 0, 0, 0, 0, 0, 0, root.x, root.y, 0, 0, 0x0200};

    // Slot 9: Tail Orb 4
    segments[8] = {9, 0x30, current_facing, 0, 0, 0, 0, 0, 0, root.x, root.y, 0, 0, 0x0200};

    // Restore root
    root.sub_mode_66 = 0;
    root.facing_16 = 0;
    root.state = 0x18;
    root.anim_id = 4;
    root.hp = 50;
    root.max_hp = 50;
    root.flag_b0 = 0;
    root.exit_target = 0x02ED20;

    return segments;
}

} // namespace Thor::AI::GolemSerpent
```

## 4. Behavioral Parity & Invariants
1. **Vertical Parabolic Trajectory:** Gravity constant is fixed at `-0x0000C000` (-0.75 px/frame²) applied every active flight tick until Z drops below zero, at which point the entity transitions instantly to shatter State 2 (anim `$0E`).
2. **Selective Wall Bouncing:** Bit 6 wall collision does not uniformly shatter; if terrain slope flag matches `($41 & 0x13) == 0x12`, it performs an inverse velocity bounce step (`Z -= vy`), enabling projectiles to slide along angled cliff edges.
3. **Compound Boss Structure:** The serpent boss is dynamically instantiated across 10 contiguous memory blocks (1 root + 9 children, each spaced by stride `$BC = 188` bytes). The 5th slot serves as the vulnerable head with custom dimensions (8x6x16, HP 20), while the root maintains the primary boss HP bar (50 HP).
