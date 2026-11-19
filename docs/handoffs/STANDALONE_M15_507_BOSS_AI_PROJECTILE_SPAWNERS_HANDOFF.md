# STANDALONE HANDOFF: Milestone M15.507 — Boss AI Feather/Spore & 3-Way Fireball Spread Projectile Spawners

## 1. Subsystem Overview
- **ROM Address Range:** `0x015136..0x015316` (480 bytes, 109 instructions + two 24-byte embedded offset tables).
- **Core Responsibilities:**
  1. `0x015136..0x0151FC` (`AI_Boss_SpawnFeatherSporeProjectile`, 198 bytes, 49 instructions) + Table `0x0151FC..0x015214` (24 bytes):
     - Allocates single entity slot in Pool 2 via `0x00DA2A`; early exits on allocation failure.
     - Configures Feather/Spore projectile (entity type `$24`): initializes core via `0x008D06`, VRAM tile base `$0200`, bounds extents 8px (`+$42..+$4A`), initial size 8 (`+$3D`), flying/invulnerability flags `+$38 |= $0C` (bits 2 and 3), layer priority `+$3C = 2`, state flags `+$B4 = $FF`, `+$A2 = $FF`.
     - Assigns animation ID 6, State `$0C` (Feather/Spore Projectile logic), and vertical upward launch impulse `+$56 = -$50000` (-5.0 fixed).
     - Indexes 3-entry 8-byte table at `0x0151FC` by `(parent_facing - 1) * 8` to determine spawn offsets:
       - Facing 1: `dx = +8, dy = +8, vx_hi = +4, vy_hi = +3`
       - Facing 2: `dx = 0, dy = +40, vx_hi = 0, vy_hi = +5`
       - Facing 3: `dx = -8, dy = +8, vx_hi = -4, vy_hi = +3`
     - Sets position `(parent_x + dx, parent_y + dy, parent_z + 16)`, adds table velocity to parent velocity, and initializes sprite via `0x009D00`.
  2. `0x015214..0x0152FE` (`AI_Boss_Spawn3WayFireballSpread`, 234 bytes, 60 instructions) + Table `0x0152FE..0x015316` (24 bytes):
     - Sequentially instantiates three spread fireballs with angle offsets `d6`:
       - Center: `d6 = 0` (`0x015214`)
       - Right: `d6 = +$1C` (+28 steps / ~39.4°) (`0x01521A`)
       - Left: `d6 = -$1C` (-28 steps / ~-39.4°) (`0x015222`)
     - Configures Fireball projectile (entity type `$24`): core init via `0x008D06`, VRAM tile base `$0200`, bounds extents 5px (`+$42..+$49`), height 8px (`+$4A`), size 8 (`+$3D`), flying flag `+$38 |= 4` (bit 2), layer priority `+$3C = 2`, flags `+$B4 = $FF`, `+$A2 = $FF`.
     - Assigns animation ID `$14`, State `$12` (Fireball Projectile logic), and vertical upward impulse `+$56 = -$C000` (-0.75 fixed).
     - Indexes 3-entry 8-byte table at `0x0152FE` for spawn displacement and base trajectory angle:
       - Facing 1: `dx = +48, dy = +8, base_angle = 234`
       - Facing 2: `dx = 0, dy = +40, base_angle = 192`
       - Facing 3: `dx = -48, dy = +8, base_angle = 170`
     - Evaluates `angle = (base_angle + d6) & 0xFF`, looks up 256-step signed trigonometry values from ROM `$05D706`:
       - `sin_val = ROM_SIN[angle]`, `cos_val = ROM_SIN[(angle + 64) & 0xFF]`
       - `x_vel = cos_val * 0x600`
       - `y_vel = -(sin_val * 0x600)`
     - Initializes sprite via `0x009D00`.

## 2. ROM Evidence Reference
- Evidence JSON: `data/thor-evidence/rom_boss_ai_projectile_spawners_m15_507.json`
- Extractor: `tools/extract_rom_boss_ai_projectile_spawners.py`
- Unit Tests: `tests/rom_boss_ai_projectile_spawners_test.py`

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <cstdint>
#include <array>
#include <optional>
#include <cmath>

namespace Thor::AI::BossSpawners {

struct FeatherOffsetEntry {
    int16_t dx{0};
    int16_t dy{0};
    int16_t vx_hi{0};
    int16_t vy_hi{0};
};

constexpr std::array<FeatherOffsetEntry, 3> FEATHER_SPAWN_TABLE = {{
    { 8,  8,  4, 3}, // Facing 1
    { 0, 40,  0, 5}, // Facing 2
    {-8,  8, -4, 3}  // Facing 3
}};

struct FireballOffsetEntry {
    int16_t dx{0};
    int16_t dy{0};
    uint8_t base_angle{0};
};

constexpr std::array<FireballOffsetEntry, 3> FIREBALL_SPAWN_TABLE = {{
    { 48,  8, 234}, // Facing 1
    {  0, 40, 192}, // Facing 2
    {-48,  8, 170}  // Facing 3
}};

struct ProjectileEntity {
    uint16_t type{0x24};
    uint16_t vram_tile{0x0200};
    uint8_t bounds_extent{8};
    uint8_t height{8};
    uint16_t facing{2};
    uint8_t size_3d{8};
    uint8_t flags_38{0};
    uint8_t layer_3c{2};
    uint8_t flag_b4{0xFF};
    uint8_t flag_a2{0xFF};
    uint16_t anim_id{0};
    uint16_t state{0};
    int32_t x{0};
    int32_t y{0};
    int32_t z{0};
    int32_t x_vel{0};
    int32_t y_vel{0};
    int32_t z_vel{0};
};

struct BossParent {
    int32_t x{0};
    int32_t y{0};
    int32_t z{0};
    int32_t x_vel{0};
    int32_t y_vel{0};
    uint16_t facing{2}; // 1..3
};

/// Spawns single Feather/Spore projectile from boss (0x015136..0x0151FA).
inline ProjectileEntity SpawnFeatherProjectile(const BossParent& boss) noexcept {
    size_t idx = (boss.facing >= 1 && boss.facing <= 3) ? (boss.facing - 1) : 1;
    const auto& tbl = FEATHER_SPAWN_TABLE[idx];

    ProjectileEntity p;
    p.type = 0x24;
    p.vram_tile = 0x0200;
    p.bounds_extent = 8;
    p.height = 8;
    p.facing = 2;
    p.size_3d = 8;
    p.flags_38 = 0x0C; // flying (bit 2) + invulnerable (bit 3)
    p.layer_3c = 2;
    p.flag_b4 = 0xFF;
    p.flag_a2 = 0xFF;
    p.anim_id = 6;
    p.state = 0x0C; // State $0C Feather
    p.x = boss.x + tbl.dx;
    p.y = boss.y + tbl.dy;
    p.z = boss.z + 16;
    p.x_vel = boss.x_vel + (static_cast<int32_t>(tbl.vx_hi) << 16);
    p.y_vel = boss.y_vel + (static_cast<int32_t>(tbl.vy_hi) << 16);
    p.z_vel = -0x50000;
    return p;
}

/// Spawns single Fireball projectile given angle spread delta (0x015226..0x0152FC).
inline ProjectileEntity SpawnSingleFireball(
    const BossParent& boss,
    int8_t spread_delta,
    const std::array<int8_t, 256>& sin_table
) noexcept {
    size_t idx = (boss.facing >= 1 && boss.facing <= 3) ? (boss.facing - 1) : 1;
    const auto& tbl = FIREBALL_SPAWN_TABLE[idx];

    uint8_t angle = static_cast<uint8_t>(tbl.base_angle + spread_delta);
    uint8_t cos_angle = static_cast<uint8_t>(angle + 0x40);

    int8_t sin_val = sin_table[angle];
    int8_t cos_val = sin_table[cos_angle];

    int32_t vx = static_cast<int32_t>(cos_val) * 0x600;
    int32_t vy = -(static_cast<int32_t>(sin_val) * 0x600);

    ProjectileEntity p;
    p.type = 0x24;
    p.vram_tile = 0x0200;
    p.bounds_extent = 5;
    p.height = 8;
    p.facing = 2;
    p.size_3d = 8;
    p.flags_38 = 0x04; // flying (bit 2)
    p.layer_3c = 2;
    p.flag_b4 = 0xFF;
    p.flag_a2 = 0xFF;
    p.anim_id = 0x14;
    p.state = 0x12; // State $12 Fireball
    p.x = boss.x + tbl.dx;
    p.y = boss.y + tbl.dy;
    p.z = boss.z + 16;
    p.x_vel = vx;
    p.y_vel = vy;
    p.z_vel = -0xC000;
    return p;
}

/// Spawns 3-way spread fireballs (Center, Right, Left) (0x015214..0x015226).
inline std::array<ProjectileEntity, 3> Spawn3WayFireballSpread(
    const BossParent& boss,
    const std::array<int8_t, 256>& sin_table
) noexcept {
    return {{
        SpawnSingleFireball(boss, 0, sin_table),      // Center: d6 = 0
        SpawnSingleFireball(boss, 0x1C, sin_table),   // Right:  d6 = +$1C (+28)
        SpawnSingleFireball(boss, -0x1C, sin_table)   // Left:   d6 = -$1C (-28)
    }};
}

} // namespace Thor::AI::BossSpawners
```
