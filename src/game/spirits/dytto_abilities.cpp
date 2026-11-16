#include "game/spirits/dytto_abilities.hpp"
#include "game/combat/elemental_damage.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace oasis::game::spirits {

namespace {
constexpr double kPi = 3.14159265358979323846;
} // namespace

void spawn_dytto_spread_projectiles(
    std::vector<DyttoProjectile>& out,
    std::int32_t origin_x,
    std::int32_t origin_y,
    std::int32_t origin_z,
    std::uint8_t base_angle) noexcept {
    // ROM 0x015214: spawns 3x entity 0x0024 at angle offsets 0, +28, -28 in State 0x12, Action 0x14.
    constexpr std::int32_t kSpeedFp = 0x00030000; // 3.0 px/frame
    for (const std::int16_t off : DyttoAbilityConfig::kSpreadAngleOffsets) {
        const std::uint8_t ang = static_cast<std::uint8_t>((static_cast<int>(base_angle) + off) & 0xFF);
        const double rad = static_cast<double>(ang) * ((2.0 * kPi) / 256.0);
        DyttoProjectile p{};
        p.active = true;
        p.type_id = DyttoAbilityConfig::kProjectileType;
        p.state = 0x0012U;
        p.action = 0x0014U;
        p.world_x = origin_x;
        p.world_y = origin_y;
        p.world_z = origin_z;
        p.x_fp = origin_x * 0x10000;
        p.y_fp = origin_y * 0x10000;
        p.z_fp = origin_z * 0x10000;
        p.vx_fp = static_cast<std::int32_t>(std::llround(std::cos(rad) * static_cast<double>(kSpeedFp)));
        p.vy_fp = static_cast<std::int32_t>(std::llround(std::sin(rad) * static_cast<double>(kSpeedFp)));
        p.vz_fp = 0;
        p.hitbox_radius = DyttoAbilityConfig::kSpreadHitboxRadius;
        p.lifetime = 45U;
        out.push_back(p);
    }
}

void spawn_dytto_magic_burst_projectile(
    std::vector<DyttoProjectile>& out,
    std::int32_t origin_x,
    std::int32_t origin_y,
    std::int32_t origin_z,
    std::uint8_t base_angle) noexcept {
    // ROM 0x015136: spawns entity 0x0024 in State 0x0C, Action 0x06, radius 8, vz = -0x00050000.
    constexpr std::int32_t kSpeedFp = 0x00020000; // 2.0 px/frame
    const double rad = static_cast<double>(base_angle) * ((2.0 * kPi) / 256.0);
    DyttoProjectile p{};
    p.active = true;
    p.type_id = DyttoAbilityConfig::kProjectileType;
    p.state = 0x000CU;
    p.action = 0x0006U;
    p.world_x = origin_x;
    p.world_y = origin_y;
    p.world_z = origin_z;
    p.x_fp = origin_x * 0x10000;
    p.y_fp = origin_y * 0x10000;
    p.z_fp = origin_z * 0x10000;
    p.vx_fp = static_cast<std::int32_t>(std::llround(std::cos(rad) * static_cast<double>(kSpeedFp)));
    p.vy_fp = static_cast<std::int32_t>(std::llround(std::sin(rad) * static_cast<double>(kSpeedFp)));
    p.vz_fp = DyttoAbilityConfig::kBurstInitialVzFp;
    p.hitbox_radius = DyttoAbilityConfig::kBurstHitboxRadius;
    p.lifetime = 45U;
    out.push_back(p);
}

std::uint32_t step_dytto_projectiles(
    std::vector<DyttoProjectile>& projectiles,
    std::span<NativeEntity> entities,
    bool apply_damage_immediately) noexcept {
    std::uint32_t hit_sfx = 0U;

    for (auto& p : projectiles) {
        if (!p.active) continue;

        if (p.state == 0x0014U) {
            // State 0x14 (0x014E0C): projectile pop/despawn
            p.active = false;
            continue;
        }

        if (p.lifetime == 0U) {
            p.state = 0x0014U;
            continue;
        }
        --p.lifetime;

        p.x_fp += p.vx_fp;
        p.y_fp += p.vy_fp;
        if (p.state == 0x000CU) {
            // State 0x0C (0x014D1A): arcing magic projectile flight (vz += 0x6000)
            p.vz_fp += DyttoAbilityConfig::kArcGravityVzFp;
            p.z_fp += p.vz_fp;
        }
        p.world_x = p.x_fp >> 16;
        p.world_y = p.y_fp >> 16;
        p.world_z = p.z_fp >> 16;

        // Collision via 0x00B922 -> 0x014E86 against active combat entities
        for (auto& ent : entities) {
            if (!ent.active || ent.invulnerable || ent.type_id == 0U || ent.type_id == 0x0060U) {
                continue;
            }
            const int dx = std::abs(p.world_x - ent.world_x);
            const int dy = std::abs(p.world_y - ent.world_y);
            const int dz = p.world_z - static_cast<int>(ent.world_z);
            const int xy_bound = static_cast<int>(p.hitbox_radius) + 6;
            if (dx <= xy_bound && dy <= xy_bound && dz >= -16 && dz <= 24) {
                // ROM 0x014E86: Water Elemental damage 0x2008 (ground) or 0xA008 (airborne)
                const std::uint16_t dmg_word = (ent.world_z != 0)
                    ? DyttoAbilityConfig::kWaterDamageAirborne
                    : DyttoAbilityConfig::kWaterDamageGround;
                ent.damage_mailbox = dmg_word;
                if (apply_damage_immediately) {
                    combat::apply_universal_elemental_damage(ent);
                }
                hit_sfx = DyttoAbilityConfig::kSfxWaterHit;
                p.state = 0x0014U; // Transition to State 0x14 pop/despawn
                break;
            }
        }
    }

    projectiles.erase(
        std::remove_if(projectiles.begin(), projectiles.end(),
                       [](const DyttoProjectile& p) { return !p.active; }),
        projectiles.end());

    return hit_sfx;
}

std::int32_t decelerate_toward_zero(std::int32_t val_fp, std::int32_t step_fp) noexcept {
    if (val_fp > 0) {
        return (val_fp > step_fp) ? (val_fp - step_fp) : 0;
    }
    if (val_fp < 0) {
        return (val_fp < -step_fp) ? (val_fp + step_fp) : 0;
    }
    return 0;
}

} // namespace oasis::game::spirits
