#pragma once

#include "core/runtime.hpp"
#include "game/entity/native_entity.hpp"
#include "game/player/player.hpp"

#include <cstdint>
#include <span>

namespace oasis::game {

// Native dagger attack subsystem (ROM routine 0x005B3C).
// Manages attack trigger on Button B, state 0x30 transition,
// 18-frame duration timer, forward hitbox collision, and sound requests.
//
// NOTE: State 0x30 is ROM disassembly confirmed (0x005B90: move.w #$0030, 4(a6)).
// Attack duration (18 frames), sound request (0xA0), forward 16x16 hitbox,
// and 4 HP target damage are native vertical slice approximations.
class NativeAttackController {
public:
    NativeAttackController() = default;

    void reset() noexcept;

    // Updates attack state machine, trigger, hitbox collision, and recovery.
    void update(player::PlayerState& player,
                std::int32_t player_x,
                std::int32_t player_y,
                const core::ControllerState& controller,
                std::span<NativeEntity> target_entities) noexcept;

    [[nodiscard]] bool is_attacking() const noexcept { return is_attacking_; }
    [[nodiscard]] std::uint16_t attack_timer() const noexcept { return attack_timer_; }
    [[nodiscard]] std::uint16_t sound_command() const noexcept { return sound_command_; }

    void clear_sound_command() noexcept { sound_command_ = 0; }

private:
    bool is_attacking_{false};
    std::uint16_t attack_timer_{0};
    std::uint16_t sound_command_{0};
};

} // namespace oasis::game
