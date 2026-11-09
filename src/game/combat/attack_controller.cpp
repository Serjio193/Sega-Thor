#include "game/combat/attack_controller.hpp"

#include <cmath>
#include <cstdlib>

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
    if (attack_pressed && player.movement_state != 0x30U) {
        player.movement_state = 0x30U; // ROM attack state ($FF19EC = 0x30, routine 0x005B90)
        is_attacking_ = true;
        attack_timer_ = 18U;           // 18-frame cooldown/duration
        sound_command_ = 0x0218U;      // Dagger slash sound request (routine 0x00632A: move.l #$1500218, d0)

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
                ent.hp -= 4; // Dagger base damage
                if (ent.hp <= 0) {
                    ent.state = 2; // Defeated state
                }
            }
        }
    } else if (player.movement_state == 0x30U) {
        if (attack_timer_ > 0U) {
            --attack_timer_;
        }
        if (attack_timer_ == 0U) {
            player.movement_state = 0U; // Return to idle
            is_attacking_ = false;
        }
    } else {
        is_attacking_ = false;
    }
}

} // namespace oasis::game
