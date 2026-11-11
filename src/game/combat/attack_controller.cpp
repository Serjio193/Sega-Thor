#include "game/combat/attack_controller.hpp"
#include "game/combat/damage_arithmetic.hpp"

#include <algorithm>
#include <cmath>

namespace oasis::game {

namespace {

void weapon_aabb_for_facing(std::uint16_t dir, std::int32_t px, std::int32_t py,
                            int& xmin, int& xmax, int& ymin, int& ymax) noexcept {
    if (dir == 0U) {
        xmin = px - 16; xmax = px + 7; ymin = py - 32; ymax = py - 1;
    } else if (dir == 1U) {
        xmin = px + 0; xmax = px + 32; ymin = py - 8; ymax = py + 16;
    } else if (dir == 2U) {
        xmin = px - 16; xmax = px + 7; ymin = py + 1; ymax = py + 32;
    } else {
        xmin = px - 32; xmax = px - 0; ymin = py - 8; ymax = py + 16;
    }
}

void write_town_002c_hit(std::uint16_t facing, std::int32_t px, std::int32_t py,
                         std::span<NativeEntity> targets, std::uint16_t dmg_mailbox,
                         std::uint8_t act_timer, std::uint16_t& snd_out) noexcept {
    int wxmin = 0, wxmax = 0, wymin = 0, wymax = 0;
    weapon_aabb_for_facing(facing, px, py, wxmin, wxmax, wymin, wymax);
    for (std::size_t i = 0; i < targets.size(); ++i) {
        auto& ent = targets[i];
        if (!ent.active || ent.type_id != 0x002CU || ent.invulnerable) continue;
        if ((ent.flags38 & 0x08U) != 0U) continue;
        const bool ox = (wxmin <= ent.world_x + ent.half_width) && (wxmax >= ent.world_x - ent.half_width);
        const bool oy = (wymin <= ent.world_y + ent.half_depth) && (wymax >= ent.world_y - ent.half_depth);
        const bool oz = (0 <= ent.world_z + ent.height) && (16 >= ent.world_z);
        if (ox && oy && oz) {
            ent.action_timer = act_timer;
            ent.flash_timer = 6U;
            ent.damage_mailbox = dmg_mailbox;
            snd_out = 0x04D9U;
        }
    }
}

} // namespace

void NativeAttackController::reset() noexcept {
    is_attacking_ = false;
    attack_timer_ = 0;
    attack_elapsed_ = 0;
    sound_command_ = 0;
}

void NativeAttackController::update_town_30hz(player::PlayerState& player,
                                              std::int32_t player_x,
                                              std::int32_t player_y,
                                              const core::ControllerState& controller,
                                              std::span<NativeEntity> target_entities) noexcept {
    sound_command_ = 0;
    if (player.movement_state == 0x18U) {
        is_attacking_ = false;
        attack_timer_ = 0;
        attack_elapsed_ = 0;
        return;
    }

    const bool attack_pressed = controller.pressed(core::Button::B);
    const bool in_atk = (player.movement_state == 0x36U || player.movement_state == 0x0AU ||
                         player.movement_state == 0x08U || player.movement_state == 0x30U ||
                         player.movement_state == 0x46U);
    if (!in_atk) {
        is_attacking_ = false;
        if (!attack_pressed) return;
        is_attacking_ = true;
        attack_elapsed_ = 0U;
        player.attack_timer = 0U;
        sound_command_ = 0x0218U;
        if (player.movement_state == 2U || player.movement_state == 4U) {
            player.movement_state = 0x36U;
            attack_timer_ = 14U;
        } else {
            player.movement_state = 0x0AU;
            attack_timer_ = 12U;
        }
        return;
    }

    ++attack_elapsed_;
    player.attack_timer = static_cast<std::uint8_t>(attack_elapsed_);
    if (attack_timer_ > 0U) {
        --attack_timer_;
    }
    if (attack_timer_ == 0U) {
        player.movement_state = 0U;
        is_attacking_ = false;
        player.attack_timer = 0U;
        return;
    }
    if (player.movement_state == 0x0AU && attack_elapsed_ >= 2U) {
        player.movement_state = 0x08U;
    }

    if (player.movement_state == 0x36U) {
        if (attack_elapsed_ == 3U) {
            write_town_002c_hit(player.direction_code, player_x, player_y, target_entities, 0x2002U, 1U, sound_command_);
        } else if (attack_elapsed_ == 6U) {
            write_town_002c_hit(player.direction_code, player_x, player_y, target_entities, 0xC005U, 2U, sound_command_);
        }
    } else if (player.movement_state == 0x08U && attack_elapsed_ == 3U) {
        write_town_002c_hit(player.direction_code, player_x, player_y, target_entities, 0x2004U, 1U, sound_command_);
    }
}

void NativeAttackController::update(player::PlayerState& player,
                                    std::int32_t player_x,
                                    std::int32_t player_y,
                                    const core::ControllerState& controller,
                                    std::span<NativeEntity> target_entities) noexcept {
    sound_command_ = 0;

    const bool attack_pressed = controller.pressed(core::Button::B);
    const bool currently_in_attack = (player.movement_state == 0x30U ||
                                      player.movement_state == 0x0AU ||
                                      player.movement_state == 0x08U ||
                                      player.movement_state == 0x36U ||
                                      player.movement_state == 0x46U);

    if (attack_pressed && !currently_in_attack) {
        is_attacking_ = true;
        sound_command_ = 0x0218U; // Authentic attack sound (routine 0x00632A)

        if ((!allow_experimental_combat_ && player.movement_state == 4U) ||
            controller.pressed(core::Button::Up) || controller.pressed(core::Button::Left)) {
            player.movement_state = 0x36U; // Combo attack
            attack_timer_ = 32U;
        } else if (allow_experimental_combat_) {
            player.movement_state = 0x30U; // Contextual attack state
            attack_timer_ = 18U;           // 18-frame synthetic slice

            // Check forward hitbox collision with secondary entities
            for (std::size_t i = 1; i < target_entities.size(); ++i) {
                auto& ent = target_entities[i];
                if (!ent.active) continue;

                const int edx = ent.world_x - player_x;
                const int edy = ent.world_y - player_y;
                bool hit = false;

                if (player.direction_code == 0U && std::abs(edx) <= 16 && edy >= -28 && edy <= 0) {
                    hit = true;
                } else if (player.direction_code == 1U && edx >= 0 && edx <= 28 && std::abs(edy) <= 16) {
                    hit = true;
                } else if (player.direction_code == 2U && std::abs(edx) <= 16 && edy >= 0 && edy <= 28) {
                    hit = true;
                } else if (player.direction_code == 3U && edx >= -28 && edx <= 0 && std::abs(edy) <= 16) {
                    hit = true;
                }

                if (hit) {
                    ent.hp -= 4; // Synthetic damage
                    if (ent.hp <= 0) {
                        ent.state = 2; // Synthetic defeat state
                    }
                }
            }
        } else {
            // Authentic 33-frame original ROM attack (routine 0x0078AA - 0x0078F0)
            player.movement_state = 0x0AU; // Default sword attack state
            attack_timer_ = 33U;           // Exact 33-frame recovery duration

            // Authentic ROM 3D Axis-Aligned Bounding Box (AABB) collision (routine 0x00BDE0..0x00BE48)
            // Weapon bounding box relative to player origin (ROM table 0x058012 / routine 0x00BDBC..0x00BDDA)
            int w_xmin = 0, w_xmax = 0, w_ymin = 0, w_ymax = 0;
            constexpr int w_zmin = 0;
            constexpr int w_zmax = 16;
            weapon_aabb_for_facing(player.direction_code, player_x, player_y, w_xmin, w_xmax, w_ymin, w_ymax);

            for (std::size_t i = 1; i < target_entities.size(); ++i) {
                auto& ent = target_entities[i];
                if (!ent.active) continue;
                if (ent.invulnerable) continue; // routine 0x00BDEE: btst #3, $38(a0)
                if (ent.type_id != 0x002CU && ent.type_id != 0x006EU) continue;

                // Target bounding box in 3D world coordinates (routine 0x00BDF8..0x00BE48)
                const int t_xmin = ent.world_x - ent.half_width;
                const int t_xmax = ent.world_x + ent.half_width;
                const int t_ymin = ent.world_y - ent.half_depth;
                const int t_ymax = ent.world_y + ent.half_depth;
                const int t_zmin = ent.world_z;
                const int t_zmax = ent.world_z + ent.height;

                const bool overlap_x = (w_xmin <= t_xmax) && (w_xmax >= t_xmin);
                const bool overlap_y = (w_ymin <= t_ymax) && (w_ymax >= t_ymin);
                const bool overlap_z = (w_zmin <= t_zmax) && (w_zmax >= t_zmin);

                if (overlap_x && overlap_y && overlap_z) {
                    // Collision writer (ROM 0x00BEE8): sets damage mailbox + action/flash timers
                    ent.action_timer = (ent.type_id == 0x002CU) ? 1U : 0U;
                    ent.flash_timer = (ent.type_id == 0x002CU) ? 6U : 0U;
                    ent.damage_mailbox = 0x2004U; // 4 base damage + bit 13 hit sound flag
                    if (combat::step_entity_combat_tick(ent)) {
                        sound_command_ = 0x04D9U; // Hit Sound (routine 0x01F43E / 0x0256E8)
                    }
                }
            }
        }
    } else if (currently_in_attack) {
        if (attack_timer_ > 0U) {
            --attack_timer_;
        }
        if (attack_timer_ == 0U) {
            player.movement_state = 0U; // Return to idle (routine 0x0062CC)
            is_attacking_ = false;
        }
    } else {
        is_attacking_ = false;
    }
}

} // namespace oasis::game
