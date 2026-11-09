#pragma once

#include "core/runtime.hpp"
#include "game/entity/native_entity.hpp"
#include "game/player/player.hpp"

#include <cstdint>
#include <span>

namespace oasis::game {

// ============================================================================
// ATTACK & COMBAT SUBSYSTEM
//
// Classification of Subsystem Logic:
// 1. AUTHENTIC_ROM_BACKED (Disassembly & Trace Proven):
//    - Standalone Idle Attack: State 0x0A (unarmed) / 0x46 (dagger)
//    - Animation ID: 0x3A (unarmed) / 0x4E (dagger) / 0x36 (contextual counter)
//    - Contextual Counter State: State 0x30 (ROM 0x005B90: move.w #$30, 4(a6))
//    - Sound Mailbox: 0x01500218 (ROM 0x00632A: move.l #$1500218, d0 -> 0x0218)
//    - Attack Duration: 33 logic frames (initiation to idle return at frame 34)
//    - Movement Lock: position invariant during active attack
//    - Direction Latching: player facing locked on attack trigger frame
//
// 2. EXPERIMENTAL_APPROXIMATION (Gated Scaffold):
//    - Synthetic Cooldown: 18 frames
//    - Hitbox Geometry: 16x16 axis-aligned forward box
//    - Damage Value: 4 HP base reduction
//    - Defeat State: entity state = 2
//
// 3. NOT_IMPLEMENTED:
//    - Dynamic enemy hitstun reactions
//    - ROM damage calculation formula tables
// ============================================================================
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

    // Gate control for experimental combat approximations (hitbox & synthetic damage)
    void set_allow_experimental_combat(bool allow) noexcept {
        allow_experimental_combat_ = allow;
    }
    [[nodiscard]] bool allow_experimental_combat() const noexcept {
        return allow_experimental_combat_;
    }

private:
    bool is_attacking_{false};
    std::uint16_t attack_timer_{0};
    std::uint16_t sound_command_{0};
    bool allow_experimental_combat_{true};
};

} // namespace oasis::game
