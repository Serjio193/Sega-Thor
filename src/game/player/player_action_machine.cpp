#include "game/player/player_action_machine.hpp"

#include <algorithm>

namespace oasis::game::player {

std::optional<std::uint32_t> lookup_player_state_handler_addr(
    std::span<const std::uint8_t> rom,
    std::uint16_t state_id) noexcept {
    if ((state_id & 1U) != 0U || state_id > PlayerActionMachineConfig::kMaxStateId) {
        return std::nullopt;
    }
    const std::size_t entry_addr =
        static_cast<std::size_t>(PlayerActionMachineConfig::kStateTableAddr) +
        static_cast<std::size_t>(state_id);
    if (entry_addr + 2U > rom.size()) {
        return std::nullopt;
    }
    const auto rel = static_cast<std::int16_t>(
        (static_cast<std::uint16_t>(rom[entry_addr]) << 8U) |
        static_cast<std::uint16_t>(rom[entry_addr + 1U]));
    const std::int64_t target = static_cast<std::int64_t>(entry_addr) + rel;
    if (target < 0 || static_cast<std::size_t>(target) >= rom.size()) {
        return std::nullopt;
    }
    return static_cast<std::uint32_t>(target);
}

void PlayerPreTickState::step_pre_dispatch_tick() {
    // 1. Low HP Warning (0x0055B0): 0 < HP <= 0x40 -> SFX 0xA0038C every (HP >> 1) + 10 frames.
    if (hp > 0U && hp <= PlayerActionMachineConfig::kLowHpThreshold) {
        if (low_hp_timer == 0U) {
            sound_commands.push_back(PlayerActionMachineConfig::kLowHpSfx);
            low_hp_timer = static_cast<std::uint8_t>((hp >> 1U) + 10U);
        } else {
            --low_hp_timer;
        }
    } else {
        low_hp_timer = 0U;
    }

    // 2. Low SP Warning (0x0055EE): active_spirit != 0 && != 0x16 && SP <= 0x18 -> SFX 0xA004DD every SP + 12.
    if (active_spirit != 0U && active_spirit != 0x0016U &&
        sp <= PlayerActionMachineConfig::kLowSpThreshold) {
        if (low_sp_timer == 0U) {
            sound_commands.push_back(PlayerActionMachineConfig::kLowSpSfx);
            low_sp_timer = static_cast<std::uint8_t>(sp + 12U);
        } else {
            --low_sp_timer;
        }
    } else {
        low_sp_timer = 0U;
    }

    // 3. Safe Respawn Coord Capture (0x005638): state in {0x00, 0x02, 0x04} && !airborne.
    if (!airborne && (state == 0x00U || state == 0x02U || state == 0x04U)) {
        safe_respawn_x = x;
        safe_respawn_y = y;
    }

    // 4. Hazard Damage (0x005718): state == 0 && hazard_ring && tile_hazard_or_water -> 16 or 8 frames.
    if (state == 0x00U && hazard_ring && tile_hazard_or_water) {
        const std::uint8_t period = regen_speed_boost ? 8U : 16U;
        ++hazard_timer;
        if (hazard_timer >= period) {
            hazard_timer = 0U;
            if (hp > 0U) {
                --hp;
            }
        }
    } else {
        hazard_timer = 0U;
    }

    // 5. Passive SP Regen (0x005762): active_spirit == 0 && (water || sp_ring) -> +1 every 8 or 4 frames.
    if (active_spirit == 0U && (tile_hazard_or_water || sp_ring)) {
        const std::uint8_t period = regen_speed_boost ? 4U : 8U;
        ++sp_regen_timer;
        if (sp_regen_timer >= period) {
            sp_regen_timer = 0U;
            if (sp < max_sp) {
                ++sp;
            }
        }
    } else {
        sp_regen_timer = 0U;
    }
}

void PlayerPreTickState::step_death_and_revive() {
    if (state == static_cast<std::uint16_t>(PlayerActionState::Death)) {
        if (!death_anim_finished) {
            return;
        }
        if (auto_revive_slot_present) {
            // 0x005F92 -> 0x0061CC: consume auto-revive, set item 0x2A, SFX 0x00D00355 -> 0x6C
            auto_revive_slot_present = false;
            active_item_id = PlayerActionMachineConfig::kReviveItemId;
            sound_commands.push_back(PlayerActionMachineConfig::kReviveSfx);
            state = static_cast<std::uint16_t>(PlayerActionState::AutoRevive);
        } else {
            // 0x005FEA: Game Over fade (40 frames = 0x28) -> 0x60
            game_over_fade_timer = PlayerActionMachineConfig::kGameOverDelayFrames;
            state = static_cast<std::uint16_t>(PlayerActionState::GameOver);
        }
        return;
    }

    if (state == static_cast<std::uint16_t>(PlayerActionState::AutoRevive)) {
        // 0x00611E: +3 HP on even frames, +3 SP on odd frames until full
        if ((frame_counter & 1U) == 0U) {
            hp = std::min<std::uint16_t>(max_hp, static_cast<std::uint16_t>(hp + PlayerActionMachineConfig::kReviveStepPerFrame));
        } else {
            sp = std::min<std::uint16_t>(max_sp, static_cast<std::uint16_t>(sp + PlayerActionMachineConfig::kReviveStepPerFrame));
        }
        ++frame_counter;
        if (hp >= max_hp && sp >= max_sp) {
            state = static_cast<std::uint16_t>(PlayerActionState::IdleStanding);
        }
        return;
    }

    if (state == static_cast<std::uint16_t>(PlayerActionState::GameOver)) {
        if (game_over_fade_timer > 0U) {
            --game_over_fade_timer;
        }
    }
}

} // namespace oasis::game::player
