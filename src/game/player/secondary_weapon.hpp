#pragma once

#include "core/runtime.hpp"
#include "game/entities/screen_entity.hpp"
#include "game/player/player.hpp"
#include "game/world/byte_grid.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace oasis::game::player {

inline constexpr std::uint8_t kWeaponNone = 0x00;
inline constexpr std::uint8_t kWeaponKnife = 0x01;
inline constexpr std::uint8_t kWeaponSword = 0x02;
inline constexpr std::uint8_t kWeaponBroadsword = 0x03;
inline constexpr std::uint8_t kWeaponDeathSword = 0x04;
inline constexpr std::uint8_t kWeaponOmegaSword = 0x05;
inline constexpr std::uint8_t kWeaponBow = 0x06;
inline constexpr std::uint8_t kWeaponFireBow = 0x07;
inline constexpr std::uint8_t kWeaponMetalBow = 0x08;
inline constexpr std::uint8_t kWeaponBomb = 0x09;
inline constexpr std::uint8_t kWeaponPowerBomb = 0x0A;
inline constexpr std::uint8_t kWeaponHyperBomb = 0x0B;
inline constexpr std::uint8_t kWeaponAtomicBow = 0x0C;

inline constexpr std::uint16_t kArrowEntityType = 0x0034;
inline constexpr std::uint8_t kArrowDamage = 6;
inline constexpr std::uint16_t kArrowLifetime = 30;
inline constexpr std::uint8_t kStateSecondaryAttack = 14;

struct WeaponInventorySlot {
    std::uint8_t id{kWeaponNone};
    std::uint8_t ammo{0};
};

using WeaponInventory = std::array<WeaponInventorySlot, 8>;

struct ArrowProjectile {
    bool active{false};
    std::uint16_t type_id{kArrowEntityType};
    std::int32_t world_x{0};
    std::int32_t world_y{0};
    std::int32_t world_z{0};
    std::int32_t velocity_x_fixed{0};
    std::int32_t velocity_y_fixed{0};
    std::uint8_t direction{1};
    std::uint16_t timer{0};
    std::uint16_t damage{kArrowDamage};
    std::uint16_t target_entity_hit{0};
    std::vector<entities::SpritePiece> pieces{};

    [[nodiscard]] entities::EntityHitbox hitbox() const noexcept {
        return entities::EntityHitbox{
            .left = world_x - 6,
            .top = (world_y + world_z) - 6,
            .right = world_x + 6,
            .bottom = (world_y + world_z) + 6,
            .z_min = world_z - 8,
            .z_max = world_z + 8,
        };
    }
};

[[nodiscard]] WeaponInventory default_weapon_inventory() noexcept;

bool cycle_secondary_weapon(WeaponInventory& inv, PlayerState& player) noexcept;

bool try_start_secondary_attack(PlayerState& player, const WeaponInventory& inv) noexcept;

void update_secondary_attack(PlayerState& player, WeaponInventory& inv,
                             ArrowProjectile& arrow, std::span<const std::uint8_t> rom);

void update_arrow_projectile(ArrowProjectile& arrow,
                             const world::ByteGridView* terrain,
                             std::span<entities::ScreenEntity> entities,
                             PlayerState& player);

} // namespace oasis::game::player
