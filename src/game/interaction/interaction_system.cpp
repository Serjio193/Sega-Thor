#include "game/interaction/interaction_system.hpp"

#include <cmath>
#include <cstdlib>

namespace oasis::game {

void NativeInteractionSystem::reset() noexcept {
    interaction_timer_ = 0;
    event_flag_ = 0;
    sound_command_ = 0;
}

void NativeInteractionSystem::update(std::int32_t player_x,
                                     std::int32_t player_y,
                                     const core::ControllerState& controller,
                                     std::span<NativeEntity> entities) noexcept {
    sound_command_ = 0;

    // Countdown active interaction freeze timer ($FF1856)
    if (interaction_timer_ > 0U) {
        --interaction_timer_;
    }

    if (entities.empty() || !entities[0].active) {
        return;
    }

    auto& chest = entities[0];
    const int dist_x = std::abs(player_x - chest.world_x);
    const int dist_y = std::abs(player_y - chest.world_y);

    // Authentic ROM routine 0x0063DA: Action button proximity interaction
    if (chest.type_id == 0x0060U && chest.state != 0x6AU && dist_x <= 20 && dist_y <= 12) {
        const bool action = controller.pressed(core::Button::A) ||
                            controller.pressed(core::Button::C);
        if (action) {
            // Authentic state transition: 0x0063DA branch to 0x006430
            chest.state = 0x6AU;          // Opened state
            chest.tile_base = 0x04B0U;    // Opened chest sprite tile base
            interaction_timer_ = 60U;     // 60-frame event freeze timer ($FF1856)
            event_flag_ = 0x10U;          // Chest 0x10 opened event flag ($FF1854)
            sound_command_ = 0x36U;       // 0x0036 chest discovery sound
        }
    }
}

} // namespace oasis::game
