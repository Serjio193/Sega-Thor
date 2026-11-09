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
        sound_command_ = 0x0218U; // Authentic sound request (routine 0x00632A)

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

            // Authentic ROM combat contact & damage calculation (routine 0x00E6BA / 0x01F450)
            for (std::size_t i = 1; i < target_entities.size(); ++i) {
                auto& ent = target_entities[i];
                if (!ent.active) continue;

                const int edx = ent.world_x - player_x;
                const int edy = ent.world_y - player_y;
                // Authentic distance check (routine 0x00E6F0)
                if (std::abs(edx) <= 32 && std::abs(edy) <= 32) {
                    // Routine 0x01F450: damage = max(1, incoming_dmg - defense)
                    constexpr int kBaseDmg = 4;
                    constexpr int kDef = 0;
                    const int final_dmg = std::max(1, kBaseDmg - kDef);
                    ent.hp = static_cast<std::int16_t>(std::max(0, ent.hp - final_dmg));
                    if (ent.hp == 0) {
                        ent.state = 0x0EU; // Defeat state
                    } else {
                        ent.state = 0x0AU; // Hurt reaction state 0x0A (routine 0x01F478)
                    }
                    ent.anim_frame = 0x08U; // Hurt animation 0x08 (routine 0x01F484)
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
