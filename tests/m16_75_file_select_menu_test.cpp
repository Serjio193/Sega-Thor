#include "game/ui/file_select_menu.hpp"

#include <cassert>

int main() {
    using oasis::game::ui::FileSelectAction;
    using oasis::game::ui::FileSelectConfig;
    using oasis::game::ui::FileSelectInput;
    using oasis::game::ui::FileSelectMenuState;
    using oasis::game::ui::FileSelectSubmode;

    // 1. 2x3 grid navigation & NewGame / ContinueGame + $2025C9 persistence.
    FileSelectMenuState menu;
    menu.reset(0x02U, 0U); // Slot 1 occupied

    menu.step(FileSelectInput{.right = true});
    assert(menu.cursor == 1U);
    assert(menu.sound_commands.back() == FileSelectConfig::kCursorMoveSfx);

    menu.step(FileSelectInput{.confirm = true});
    assert(menu.last_action == FileSelectAction::ContinueGame);
    assert(menu.last_slot_sram_2025c9 == 1U);
    assert(menu.sound_commands.back() == FileSelectConfig::kConfirmSfx);

    menu.step(FileSelectInput{.left = true});
    assert(menu.cursor == 0U);
    menu.step(FileSelectInput{.confirm = true});
    assert(menu.last_action == FileSelectAction::NewGame);
    assert(menu.last_slot_sram_2025c9 == 0U);

    // 2. Copy/Erase abort when occupied_mask == 0.
    menu.reset(0x00U, 4U);
    menu.step(FileSelectInput{.confirm = true});
    assert(menu.submode == FileSelectSubmode::Main);

    menu.reset(0x00U, 5U);
    menu.step(FileSelectInput{.confirm = true});
    assert(menu.submode == FileSelectSubmode::Main);

    // 3. Copy flow: slot 4 -> CopySource -> CopyDest (toggles bit 0, guards dest != src).
    menu.reset(0x01U, 4U); // Slot 0 occupied
    menu.step(FileSelectInput{.confirm = true});
    assert(menu.submode == FileSelectSubmode::CopySource && menu.cursor == 0U);

    menu.step(FileSelectInput{.confirm = true});
    assert(menu.submode == FileSelectSubmode::CopyDest);
    assert(menu.copy_src_slot == 0U);
    assert(menu.cursor == 1U); // bit 0 toggled

    // Try moving Left onto excluded copy_src_slot (0) -> blocked.
    menu.step(FileSelectInput{.left = true});
    assert(menu.cursor == 1U);

    menu.step(FileSelectInput{.confirm = true});
    assert(menu.last_action == FileSelectAction::CopyCompleted);
    assert(menu.occupied_mask == 0x03U);
    assert(menu.submode == FileSelectSubmode::Main && menu.cursor == 4U);

    // 4. Erase flow: empty slot emits 0x00A00253, occupied slot erases.
    menu.reset(0x02U, 5U); // Slot 1 occupied, Slot 0 empty
    menu.step(FileSelectInput{.confirm = true});
    assert(menu.submode == FileSelectSubmode::EraseSelect && menu.cursor == 0U);

    menu.step(FileSelectInput{.confirm = true}); // Slot 0 empty -> error SFX
    assert(menu.sound_commands.back() == FileSelectConfig::kInvalidEraseSfx);
    assert(menu.submode == FileSelectSubmode::EraseSelect);

    menu.step(FileSelectInput{.right = true});
    assert(menu.cursor == 1U);
    menu.step(FileSelectInput{.confirm = true});
    assert(menu.last_action == FileSelectAction::EraseCompleted);
    assert(menu.occupied_mask == 0x00U);

    // 5. Cancel from Main -> ReturnToTitle (Mode 0x04).
    menu.step(FileSelectInput{.cancel = true});
    assert(menu.last_action == FileSelectAction::ReturnToTitle);

    return 0;
}
