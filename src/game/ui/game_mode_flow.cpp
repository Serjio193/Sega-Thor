#include "game/ui/game_mode_flow.hpp"

namespace oasis::game::ui {

std::optional<std::uint32_t> lookup_mode_handler_addr(
    std::span<const std::uint8_t> rom,
    TopLevelGameMode mode) noexcept {
    const std::uint16_t raw = static_cast<std::uint16_t>(mode);
    if (raw != 0x00U && raw != 0x04U && raw != 0x08U && raw != 0x0CU && raw != 0x10U) {
        return std::nullopt;
    }
    const std::size_t entry_addr =
        static_cast<std::size_t>(GameModeFlowConfig::kModeTableRomAddr) +
        static_cast<std::size_t>(raw & 0x7CU);
    if (entry_addr + 4U > rom.size()) {
        return std::nullopt;
    }
    return (static_cast<std::uint32_t>(rom[entry_addr]) << 24U) |
           (static_cast<std::uint32_t>(rom[entry_addr + 1U]) << 16U) |
           (static_cast<std::uint32_t>(rom[entry_addr + 2U]) << 8U) |
           static_cast<std::uint32_t>(rom[entry_addr + 3U]);
}

void GameModeStateMachine::request_mode(TopLevelGameMode next) noexcept {
    pending_mode = static_cast<std::uint16_t>(next);
}

bool GameModeStateMachine::latch_pending_mode() noexcept {
    if (pending_mode == GameModeFlowConfig::kNoPendingMode) {
        return false;
    }
    current_mode = static_cast<TopLevelGameMode>(pending_mode);
    pending_mode = GameModeFlowConfig::kNoPendingMode;
    return true;
}

void TitleScreenState::reset() noexcept {
    *this = TitleScreenState{};
}

bool TitleScreenState::press_start_visible() const noexcept {
    return (timer_frames & GameModeFlowConfig::kTitleBlinkMask) <
           GameModeFlowConfig::kTitleBlinkOnThreshold;
}

void TitleScreenState::step_frame(bool start_pressed, std::uint8_t pad_byte_ff165e) noexcept {
    if (start_pressed) {
        if (pad_byte_ff165e == 0x90U) {
            // 0x0034BE: A + Start -> Clear Records / Sound Test (0x03E4FA)
            entered_clear_records = true;
            last_sound_cmd = GameModeFlowConfig::kChord90Sfx;
            return;
        }
        if (pad_byte_ff165e == 0xD5U) {
            // 0x0034E4: Clear SRAM record magic (0x001E98) -> Clear Records (0x03E4FA)
            entered_clear_records = true;
            clear_sram_records_requested = true;
            last_sound_cmd = GameModeFlowConfig::kChordD5Sfx;
            return;
        }
        if (pad_byte_ff165e == 0x8AU) {
            // 0x003428: Immediate Demo / Attract Mode (0x08, $FF1996 = 0xFF)
            pending_mode = static_cast<std::uint16_t>(TopLevelGameMode::GameplayNew);
            demo_flag = true;
            return;
        }
        // 0x00347C: Normal Start -> File Select / Gameplay (0x0C)
        last_sound_cmd = GameModeFlowConfig::kNormalStartSfx;
        pending_mode = static_cast<std::uint16_t>(TopLevelGameMode::GameplayAlt);
        demo_flag = false;
        return;
    }

    if (timer_frames < GameModeFlowConfig::kTitleAttractTimeoutFrames) {
        ++timer_frames;
        if (timer_frames >= GameModeFlowConfig::kTitleAttractTimeoutFrames) {
            // 720-frame attract timeout -> GameplayNew (0x08) + demo_flag = true
            pending_mode = static_cast<std::uint16_t>(TopLevelGameMode::GameplayNew);
            demo_flag = true;
        }
    }
}

void ClearRecordsScreenState::reset() noexcept {
    *this = ClearRecordsScreenState{};
}

void ClearRecordsScreenState::clear_sram_record_magic() noexcept {
    sram_magic_2025cd = 0U;
    sram_magic_2026e1 = 0U;
    for (auto& slot : slots) {
        slot = ClearRecordSlot{};
    }
}

void ClearRecordsScreenState::step_frame(bool start_pressed) noexcept {
    if (start_pressed) {
        last_sound_cmd = GameModeFlowConfig::kClearRecordsExitSfx;
        soft_reset_requested = true;
        soft_reset_target = GameModeFlowConfig::kSoftResetTargetAddr;
    }
}

} // namespace oasis::game::ui
