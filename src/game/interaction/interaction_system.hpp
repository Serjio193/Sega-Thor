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

    // Sets whether chest interaction triggers on proximity contact alone (authentic original ROM
    // routine 0x0063DA, as observed at Frame 567 where current_input == 0).
    void set_trigger_on_proximity(bool enable) noexcept { trigger_on_proximity_ = enable; }
    [[nodiscard]] bool trigger_on_proximity() const noexcept { return trigger_on_proximity_; }

    // Sets whether experimental button-press trigger (Buttons A/C) is enabled for interactive free play.
    // In authentic original-parity replay mode, this is disabled.
    void set_experimental_button_trigger_enabled(bool enable) noexcept { allow_experimental_button_ = enable; }
    [[nodiscard]] bool experimental_button_trigger_enabled() const noexcept { return allow_experimental_button_; }

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
    bool trigger_on_proximity_{false};
    bool allow_experimental_button_{true};
};

} // namespace oasis::game
