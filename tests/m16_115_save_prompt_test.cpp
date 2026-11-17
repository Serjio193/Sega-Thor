#include "game/ui/save_prompt.hpp"

#include <cassert>

namespace {

using namespace oasis::game::ui;

void test_save_allowed_guard() {
    assert(can_open_save_prompt(0U, 0U, 0U, true, false));
    assert(can_open_save_prompt(0U, 0U, 2U, true, false));
    assert(can_open_save_prompt(0U, 0U, 1U, false, true)); // debug override

    // Blocked by room flag, player state, $FF1996, or $FF17C1 (SFX 0x00A00253)
    assert(!can_open_save_prompt(0U, 0U, 1U, false, false));
    assert(!can_open_save_prompt(0U, 0U, 3U, true, true));
    assert(!can_open_save_prompt(1U, 0U, 0U, true, true));
    assert(!can_open_save_prompt(0U, 1U, 0U, true, true));
}

void test_save_prompt_yes_no_state_machine() {
    std::uint8_t choice = kSaveDefaultChoice;
    assert(choice == 4U); // NO

    // Up: 4 -> 3 (YES) with toggle SFX 0x006003A5
    const auto up_res = step_save_prompt(choice, kSavePromptBtnUp);
    assert(up_res.next_choice == 3U);
    assert(up_res.outcome == SavePromptOutcome::Pending);
    assert(up_res.sound_cmd == kSavePromptToggleSfx);
    choice = up_res.next_choice;

    // A on YES -> ConfirmedSave + 0x00A003AA
    const auto confirm_res = step_save_prompt(choice, kSavePromptBtnA);
    assert(confirm_res.outcome == SavePromptOutcome::ConfirmedSave);
    assert(confirm_res.sound_cmd == kSavePromptConfirmSfx);

    // Down: 3 -> 4 (NO) with toggle SFX 0x006003A5
    const auto down_res = step_save_prompt(choice, kSavePromptBtnDown);
    assert(down_res.next_choice == 4U);
    assert(down_res.outcome == SavePromptOutcome::Pending);
    assert(down_res.sound_cmd == kSavePromptToggleSfx);
    choice = down_res.next_choice;

    // A on NO -> Cancelled + 0x00A00347
    const auto cancel_a = step_save_prompt(choice, kSavePromptBtnA);
    assert(cancel_a.outcome == SavePromptOutcome::Cancelled);
    assert(cancel_a.sound_cmd == kSavePromptCancelSfx);

    // B/C on YES -> Cancelled + 0x00A00347
    const auto cancel_bc = step_save_prompt(kSaveChoiceYes, kSavePromptBtnCancelMask);
    assert(cancel_bc.outcome == SavePromptOutcome::Cancelled);
    assert(cancel_bc.sound_cmd == kSavePromptCancelSfx);
}

} // namespace

int main() {
    test_save_allowed_guard();
    test_save_prompt_yes_no_state_machine();
    return 0;
}
