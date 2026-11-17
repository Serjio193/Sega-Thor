#include "game/ui/save_prompt.hpp"

namespace oasis::game::ui {

bool can_open_save_prompt(
    std::uint8_t button_swap_ff1996,
    std::uint8_t flag_ff17c1,
    std::uint8_t player_state,
    bool room_save_allowed,
    bool debug_override) noexcept {
    return button_swap_ff1996 == 0U &&
           flag_ff17c1 == 0U &&
           player_state <= kSaveMaxPlayerState &&
           (room_save_allowed || debug_override);
}

SavePromptStepResult step_save_prompt(
    std::uint8_t choice,
    std::uint8_t joypad) noexcept {
    if ((joypad & kSavePromptBtnUp) != 0U && choice == kSaveChoiceNo) {
        return SavePromptStepResult{
            kSaveChoiceYes,
            SavePromptOutcome::Pending,
            kSavePromptToggleSfx,
        };
    }
    if ((joypad & kSavePromptBtnDown) != 0U && choice == kSaveChoiceYes) {
        return SavePromptStepResult{
            kSaveChoiceNo,
            SavePromptOutcome::Pending,
            kSavePromptToggleSfx,
        };
    }
    if ((joypad & kSavePromptBtnA) != 0U) {
        if (choice == kSaveChoiceYes) {
            return SavePromptStepResult{
                choice,
                SavePromptOutcome::ConfirmedSave,
                kSavePromptConfirmSfx,
            };
        }
        return SavePromptStepResult{
            choice,
            SavePromptOutcome::Cancelled,
            kSavePromptCancelSfx,
        };
    }
    if ((joypad & kSavePromptBtnCancelMask) != 0U) {
        return SavePromptStepResult{
            choice,
            SavePromptOutcome::Cancelled,
            kSavePromptCancelSfx,
        };
    }
    return SavePromptStepResult{choice, SavePromptOutcome::Pending, 0U};
}

} // namespace oasis::game::ui
