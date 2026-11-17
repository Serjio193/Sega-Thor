#include "game/player/master_frame_sequencer.hpp"

namespace oasis::game::player {

ScreenShakeDispatchResult dispatch_screen_shake(std::uint8_t& trigger_byte) noexcept {
    if (trigger_byte == 0U) {
        return {ScreenShakeAction::None, 0U};
    }
    const bool bit7_set = (trigger_byte & 0x80U) != 0U;
    trigger_byte = static_cast<std::uint8_t>(trigger_byte & 0x7FU);
    if (!bit7_set) {
        return {ScreenShakeAction::PaletteFlash, trigger_byte};
    }
    return {ScreenShakeAction::ScreenShake, trigger_byte};
}

MenuHotkeyResult poll_joypad_menu_hotkeys(
    std::uint8_t held_joypad,
    std::uint8_t edge_joypad,
    std::uint16_t extra_buttons,
    std::uint8_t lock_1987,
    std::uint8_t slow_motion_flags,
    std::uint8_t lock_1983,
    std::uint8_t lock_1982,
    std::uint16_t ali_substate,
    std::uint16_t spirit_active,
    std::uint8_t& menu_request,
    std::uint8_t& key_repeat,
    std::uint16_t& ring_count) noexcept {
    MenuHotkeyResult result{};

    // 0x008BAC..0x008BC6: Check release of A/B/C buttons (mask 0x70)
    if ((held_joypad & MasterFrameSequencerConfig::kButtonsReleaseMask) == 0U) {
        key_repeat = 0xFFU;
        ring_count = 0U;
        result.key_repeat_reset = true;
        result.ring_count_cleared = true;
    }

    // 0x008BC8..0x008BF0: Check input lock flags
    if (lock_1987 != 0U || (slow_motion_flags & 0x04U) != 0U || lock_1983 != 0U || lock_1982 != 0U) {
        return result;
    }

    // 0x008BF2: Clear menu request register by default
    menu_request = 0U;

    // 0x008BF8..0x008C02: Start button edge check (menu index 0)
    if ((edge_joypad & (1U << MasterFrameSequencerConfig::kStartButtonBit)) != 0U) {
        menu_request = 0U;
        result.menu_triggered = true;
        result.menu_index = 0U;
        return result;
    }

    // 0x008C04..0x008C10: Extra button 2 (Mode / Map menu index 3)
    if ((extra_buttons & 0x04U) != 0U) {
        menu_request = 3U;
        result.menu_triggered = true;
        result.menu_index = 3U;
        return result;
    }

    // 0x008C12..0x008C22: Ali state guard (<= 2) and spirit active guard (== 0)
    if (ali_substate <= 2U && spirit_active == 0U) {
        // Extra button 1: Item ring menu (menu index 1)
        if ((extra_buttons & 0x02U) != 0U) {
            menu_request = 1U;
            result.menu_triggered = true;
            result.menu_index = 1U;
            return result;
        }

        // Extra button 0: Weapon ring menu (menu index 2)
        if ((extra_buttons & 0x01U) != 0U) {
            menu_request = 2U;
            result.menu_triggered = true;
            result.menu_index = 2U;
            return result;
        }
    }

    return result;
}

MenuHotkeyResult poll_joypad_menu_hotkeys(GameplayFrameState& state) noexcept {
    return poll_joypad_menu_hotkeys(
        state.joypad_held,
        state.joypad_edge,
        state.joypad_extra,
        state.input_lock_1987,
        state.slow_motion_flags,
        state.input_lock_1983,
        state.input_lock_1982,
        state.ali_substate,
        state.spirit_active,
        state.menu_request,
        state.key_repeat_flags,
        state.input_ring_count);
}

FrameSequencerStatus step_gameplay_frame(
    GameplayFrameState& state,
    std::vector<PipelinePhase>* executed_phases) noexcept {
    // 0x008AA6..0x008ABA: Check demo active/ready flags
    if (state.demo_active && !state.demo_playing) {
        return FrameSequencerStatus::ExitToTitle;
    }

    // 0x008ABE..0x008AC8: Global tick & animation subtick increments
    state.global_tick += 2U;
    state.anim_subtick = static_cast<std::uint8_t>(state.anim_subtick + 1U);

    // 0x008ACA..0x008AE6: V-Int synchronization & frame count
    state.flags_ff164d |= 0x08U; // bset #3, $FF164D
    state.frame_count = static_cast<std::uint16_t>(state.frame_count + 1U);

    // 0x008AE8..0x008B0A: Slow-motion check ($FF17B8 bit 0)
    if ((state.slow_motion_flags & 0x01U) != 0U) {
        state.frame_countdown = 3U;
    } else {
        state.frame_countdown = 2U;
    }

    auto record_phase = [&](PipelinePhase phase) {
        if (executed_phases) {
            executed_phases->push_back(phase);
        }
    };

    // 0x008B0C..0x008B20: Pause menu check
    if (state.pause_flag != 0U && (state.pause_busy & 0x01U) == 0U) {
        record_phase(PipelinePhase::PauseMenu);
    }

    // 0x008B22: Joypad poll & menu hotkeys
    record_phase(PipelinePhase::JoypadPoll);
    (void)poll_joypad_menu_hotkeys(state);

    // 0x008B26: Screen shake / palette flash step
    record_phase(PipelinePhase::ScreenShakeStep);
    (void)dispatch_screen_shake(state.screen_shake_trigger);

    // 0x008B2A..0x008B48: Simulation sub-steps
    record_phase(PipelinePhase::EventVmTick);
    record_phase(PipelinePhase::DialogueHudStep);
    record_phase(PipelinePhase::SpiritManaStep);
    record_phase(PipelinePhase::SecondaryWeapon);
    record_phase(PipelinePhase::HazardBlast);
    record_phase(PipelinePhase::AliMachineStep);
    record_phase(PipelinePhase::EntitiesListStep);
    record_phase(PipelinePhase::EntityGcReclaim);

    // 0x008B4A..0x008B62: Camera clamp & Plane scroll sync
    state.flags_ff164d |= 0x20U; // bset #5, $FF164D
    record_phase(PipelinePhase::CameraClamp);
    record_phase(PipelinePhase::PlaneScrollSync);

    // 0x008B64..0x008B70: Clear enemy gauge timer in demo mode
    if (state.demo_active) {
        state.enemy_gauge_timer = 0U;
        record_phase(PipelinePhase::ClearEnemyGaugeInDemo);
    }

    // 0x008B72..0x008B88: SAT Compose
    record_phase(PipelinePhase::SatCompose);

    return FrameSequencerStatus::Continue;
}

} // namespace oasis::game::player
