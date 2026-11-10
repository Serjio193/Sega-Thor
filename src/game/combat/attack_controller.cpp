#include "game/combat/attack_controller.hpp"

#include <algorithm>
#include <cmath>

namespace oasis::game {

void NativeAttackController::reset() noexcept {
    is_attacking_ = false;
    attack_timer_ = 0;
    sound_command_ = 0;
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
                                      player.movement_state == 0x46U);

    if (attack_pressed && !currently_in_attack) {
        is_attacking_ = true;
        sound_command_ = 0x0218U; // Authentic attack sound (routine 0x00632A)

        if (allow_experimental_combat_) {
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

            if (player.direction_code == 0U) { // Facing Up
                w_xmin = player_x - 16; w_xmax = player_x + 7;
                w_ymin = player_y - 32; w_ymax = player_y - 1;
            } else if (player.direction_code == 1U) { // Facing Right
                w_xmin = player_x + 0;  w_xmax = player_x + 32;
                w_ymin = player_y - 8;  w_ymax = player_y + 16;
            } else if (player.direction_code == 2U) { // Facing Down
                w_xmin = player_x - 16; w_xmax = player_x + 7;
                w_ymin = player_y + 1;  w_ymax = player_y + 32;
            } else { // Facing Left (routine 0x00BDB6: mirrored X)
                w_xmin = player_x - 32; w_xmax = player_x - 0;
                w_ymin = player_y - 8;  w_ymax = player_y + 16;
            }

            for (std::size_t i = 1; i < target_entities.size(); ++i) {
                auto& ent = target_entities[i];
                if (!ent.active) continue;
                if (ent.invulnerable) continue; // routine 0x00BDEE: btst #3, $38(a0)
                if (ent.type_id != 0x0030U) continue; // Only authentic damageable enemy types (0x0030 Guard)

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
                    // Routine 0x01F450: damage arithmetic clamped to minimum 1
                    constexpr int kBaseDmg = 4;
                    const int final_dmg = std::max(1, kBaseDmg - static_cast<int>(ent.defense));
                    ent.hp = static_cast<std::int16_t>(std::max(0, ent.hp - final_dmg));
                    ent.previous_state = ent.state;
                    ent.previous_anim = ent.anim_frame;
                    ent.invulnerable = true; // routine 0x01F466: bset #3, $38(a6)
                    if (ent.hp == 0) {
                        ent.state = 0x0CU; // Collapse / Death State (routine 0x01EF44)
                    } else {
                        ent.state = 0x0AU; // Hurt State (routine 0x01F478)
                    }
                    ent.anim_frame = 0x08U;  // Hurt Anim (routine 0x01F484)
                    sound_command_ = 0x04D9U; // Hit Sound (routine 0x01F43E)
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
