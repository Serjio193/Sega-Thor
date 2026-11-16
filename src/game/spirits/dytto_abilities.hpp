#pragma once

#include "game/entity/native_entity.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace oasis::game::spirits {

// ROM 0x014914 (14 states 0x00..0x1A) & 0x014E86 Water Elemental damage constants.
struct DyttoAbilityConfig {
    // Autonomous dispatch (State 0x00 at 0x014932) PRNG thresholds (0x00E268)
    static constexpr std::uint16_t kPrngMagicBurstMax = 0x2000U;   // 12.5% -> State 0x06
    static constexpr std::uint16_t kPrngSpreadMax = 0x6000U;       // 25.0% -> State 0x10
    static constexpr std::uint16_t kPrngContactBuffMax = 0x9000U;  // 18.75% -> State 0x16

    // Projectile entity type & damage words (0x015214, 0x015136, 0x014E86)
    static constexpr std::uint16_t kProjectileType = 0x0024U;
    static constexpr std::uint16_t kWaterDamageGround = 0x2008U;   // bit 13 (Water) | 8 dmg
    static constexpr std::uint16_t kWaterDamageAirborne = 0xA008U; // bit 15 (Air) | bit 13 | 8 dmg
    static constexpr std::uint16_t kBaseDamage = 8U;

    // 3-way spread angle offsets in 0..255 binary angle space (0x015214)
    static constexpr std::array<std::int16_t, 3> kSpreadAngleOffsets = {0, 28, -28};
    static constexpr std::int16_t kSpreadHitboxRadius = 5;
    static constexpr std::int16_t kBurstHitboxRadius = 8;

    // Magic burst & knockback fixed-point constants (0x015136, 0x014D1A, 0x014EE6)
    static constexpr std::int32_t kBurstInitialVzFp = -0x00050000;
    static constexpr std::int32_t kArcGravityVzFp = 0x00006000;
    static constexpr std::int32_t kKnockbackXyDampFp = 0x00008000;
    static constexpr std::int32_t kKnockbackZDampFp = 0x0000A000;

    // Sound commands & callback addresses
    static constexpr std::uint32_t kSfxAbilityStart = 0x00800251U;
    static constexpr std::uint32_t kSfxProjectileSpawn = 0x0080034FU;
    static constexpr std::uint32_t kSfxWaterHit = 0x00700345U;
    static constexpr std::uint32_t kAliBuffCallbackAddr = 0x00014C04U;
};

struct DyttoProjectile {
    bool active{false};
    std::uint16_t type_id{DyttoAbilityConfig::kProjectileType}; // 0x0024
    std::uint16_t state{0x0012U};   // 0x12 = spread flight, 0x0C = arcing burst, 0x14 = pop
    std::uint16_t action{0x0014U};  // 0x14 = spread, 0x06 = burst
    std::int32_t world_x{0};
    std::int32_t world_y{0};
    std::int32_t world_z{0};
    std::int32_t x_fp{0};
    std::int32_t y_fp{0};
    std::int32_t z_fp{0};
    std::int32_t vx_fp{0};
    std::int32_t vy_fp{0};
    std::int32_t vz_fp{0};
    std::int16_t hitbox_radius{DyttoAbilityConfig::kSpreadHitboxRadius};
    std::uint16_t lifetime{45U};
};

// Spawns 3-way spread projectiles (0x015214: angles 0, +28, -28) from Dytto's position.
void spawn_dytto_spread_projectiles(
    std::vector<DyttoProjectile>& out,
    std::int32_t origin_x,
    std::int32_t origin_y,
    std::int32_t origin_z,
    std::uint8_t base_angle) noexcept;

// Spawns arcing magic burst projectile (0x015136: State 0x0C, Action 0x06, vz = -0x00050000).
void spawn_dytto_magic_burst_projectile(
    std::vector<DyttoProjectile>& out,
    std::int32_t origin_x,
    std::int32_t origin_y,
    std::int32_t origin_z,
    std::uint8_t base_angle) noexcept;

// Steps all active Dytto projectiles (State 0x0C / 0x12 / 0x14) and applies Water Elemental
// damage (0x2008 ground / 0xA008 airborne) to overlapping active enemy entities via 0x014E86.
// When `apply_damage_immediately` is true, also consumes `damage_mailbox` via ROM 0x017E30
// (`combat::apply_universal_elemental_damage`) so bit-13 elemental flags, defense, and HP update immediately.
// Returns the last hit SFX command if a hit occurred, or 0 otherwise.
std::uint32_t step_dytto_projectiles(
    std::vector<DyttoProjectile>& projectiles,
    std::span<NativeEntity> entities,
    bool apply_damage_immediately = false) noexcept;

// Decelerates knockback velocity toward 0 by step_fp (0x014EE6).
[[nodiscard]] std::int32_t decelerate_toward_zero(std::int32_t val_fp, std::int32_t step_fp) noexcept;

} // namespace oasis::game::spirits
