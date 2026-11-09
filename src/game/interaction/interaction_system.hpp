#pragma once

#include "core/runtime.hpp"
#include "game/entity/native_entity.hpp"

#include <cstdint>
#include <span>

namespace oasis::game {

// Authentic chest interaction logic (ROM routine 0x0063DA -> 0x006430).
// Handles proximity detection, action button trigger, state transition,
// event flags ($FF1854), and interaction freeze timer countdown ($FF1856).
class NativeInteractionSystem {
public:
    NativeInteractionSystem() = default;

    void reset() noexcept;

    // Checks proximity and controller input, triggers chest opening if qualified.
    // Handles authentic interaction timer countdown.
    void update(std::int32_t player_x,
                std::int32_t player_y,
                const core::ControllerState& controller,
                std::span<NativeEntity> entities) noexcept;

    [[nodiscard]] std::uint16_t interaction_timer() const noexcept { return interaction_timer_; }
    [[nodiscard]] std::uint16_t event_flag() const noexcept { return event_flag_; }
    [[nodiscard]] std::uint16_t sound_command() const noexcept { return sound_command_; }

    void set_event_flag(std::uint16_t flag) noexcept { event_flag_ = flag; }
    void set_interaction_timer(std::uint16_t timer) noexcept { interaction_timer_ = timer; }
    void clear_sound_command() noexcept { sound_command_ = 0; }

private:
    std::uint16_t interaction_timer_{0};
    std::uint16_t event_flag_{0};
    std::uint16_t sound_command_{0};
};

} // namespace oasis::game
