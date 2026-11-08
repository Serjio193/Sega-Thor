#include "game/player/secondary_weapon.hpp"
#include "game/world/terrain_collision.hpp"

namespace oasis::game::player {

WeaponInventory default_weapon_inventory() noexcept {
    WeaponInventory inv{};
    inv[0] = { .id = kWeaponKnife, .ammo = 0xFF };
    inv[1] = { .id = kWeaponBow, .ammo = 10 };
    return inv;
}

bool cycle_secondary_weapon(WeaponInventory& inv, PlayerState& player) noexcept {
    const auto current = player.secondary_slot;
    for (std::uint8_t offset = 1; offset <= 8; ++offset) {
        const auto next = static_cast<std::uint8_t>((current + offset) % 8);
        if (inv[next].id != kWeaponNone) {
            player.secondary_slot = next;
            player.secondary_weapon = inv[next].id;
            player.secondary_ammo = inv[next].ammo;
            return true;
        }
    }
    return false;
}

bool try_start_secondary_attack(PlayerState& player, const WeaponInventory& inv) noexcept {
    if (player.is_hurt() || player.is_attacking()) return false;
    if (player.secondary_weapon != kWeaponBow) return false;
    if (player.secondary_ammo == 0) return false;
    player.secondary_attack_state = kStateSecondaryAttack;
    player.secondary_attack_timer = 0;
    player.secondary_attack_fired = false;
    player.secondary_attack_direction = static_cast<std::uint8_t>(player.direction_code & 0x03U);
    player.intent_x_fixed = 0;
    player.intent_y_fixed = 0;
    return true;
}

void update_secondary_attack(PlayerState& player, WeaponInventory& inv,
                             ArrowProjectile& arrow, std::span<const std::uint8_t> rom) {
    if (player.secondary_attack_state == 0) return;
    ++player.secondary_attack_timer;
    if (player.secondary_attack_timer == 12 && !player.secondary_attack_fired) {
        if (player.secondary_ammo != 0xFF) {
            --player.secondary_ammo;
            inv[player.secondary_slot].ammo = player.secondary_ammo;
            if (player.secondary_ammo == 0) {
                inv[player.secondary_slot] = { .id = kWeaponNone, .ammo = 0 };
                player.secondary_slot = 0;
                player.secondary_weapon = inv[0].id;
                player.secondary_ammo = inv[0].ammo;
            }
        }
        arrow.active = true;
        arrow.type_id = kArrowEntityType;
        arrow.timer = 0;
        arrow.direction = player.secondary_attack_direction;
        arrow.damage = kArrowDamage;
        arrow.target_entity_hit = 0;

        const auto px = static_cast<std::int32_t>(player.x_fixed / 0x10000);
        const auto py = static_cast<std::int32_t>(player.y_fixed / 0x10000);
        constexpr std::int32_t kSpeed = 8 * 0x10000;

        switch (arrow.direction) {
            case 0: // Up
                arrow.world_x = px;
                arrow.world_y = py - 8;
                arrow.velocity_x_fixed = 0;
                arrow.velocity_y_fixed = -kSpeed;
                break;
            case 1: // Right
                arrow.world_x = px + 12;
                arrow.world_y = py;
                arrow.velocity_x_fixed = kSpeed;
                arrow.velocity_y_fixed = 0;
                break;
            case 2: // Down
                arrow.world_x = px;
                arrow.world_y = py + 8;
                arrow.velocity_x_fixed = 0;
                arrow.velocity_y_fixed = kSpeed;
                break;
            case 3: // Left
            default:
                arrow.world_x = px - 12;
                arrow.world_y = py;
                arrow.velocity_x_fixed = -kSpeed;
                arrow.velocity_y_fixed = 0;
                break;
        }
        arrow.world_z = 0;

        arrow.pieces.clear();
        if (!rom.empty()) {
            const auto pcs = entities::decode_sprite_frame_from_rom(rom, 0x154C22);
            if (!pcs.empty()) {
                arrow.pieces = pcs;
            } else {
                arrow.pieces.push_back(entities::SpritePiece{
                    .x_offset = -4,
                    .y_offset = -16,
                    .width_cells = 1,
                    .height_cells = 4,
                    .tile_index = 239,
                    .palette_line = 0,
                });
            }
        }
        player.secondary_attack_fired = true;
    } else if (player.secondary_attack_timer >= 16) {
        player.secondary_attack_state = 0;
        player.secondary_attack_timer = 0;
        player.secondary_attack_fired = false;
    }
}

void update_arrow_projectile(ArrowProjectile& arrow,
                             const world::ByteGridView* terrain,
                             std::span<entities::ScreenEntity> entities,
                             PlayerState& player) {
    if (!arrow.active) return;
    arrow.world_x += arrow.velocity_x_fixed / 0x10000;
    arrow.world_y += arrow.velocity_y_fixed / 0x10000;
    ++arrow.timer;
    if (arrow.timer >= kArrowLifetime) {
        arrow.active = false;
        return;
    }

    if (terrain != nullptr) {
        const auto cell = terrain->sample_world(arrow.world_x, arrow.world_y);
        if (cell.has_value() && ((*cell & 0x0FU) == 0x05U || world::terrain_state_from_code(*cell) < 0)) {
            arrow.active = false;
            return;
        }
    }

    const auto hb = arrow.hitbox();
    for (auto& ent : entities) {
        if (!ent.active() || !ent.is_actor() || ent.actor().is_dead()) continue;
        if (hb.overlaps(ent.hitbox())) {
            ent.apply_damage(arrow.damage, arrow.direction);
            arrow.target_entity_hit = ent.type_id();
            player.last_weapon_hit = ent.type_id();
            arrow.active = false;
            break;
        }
    }
}

} // namespace oasis::game::player
