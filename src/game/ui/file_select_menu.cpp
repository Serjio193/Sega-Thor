#include "game/ui/file_select_menu.hpp"

namespace oasis::game::ui {

void FileSelectMenuState::reset(
    std::uint8_t initial_occupied_mask,
    std::uint8_t initial_cursor) noexcept {
    cursor = (initial_cursor <= 5U) ? initial_cursor : 0U;
    occupied_mask = static_cast<std::uint8_t>(initial_occupied_mask & 0x0FU);
    copy_src_slot = 0U;
    last_slot_sram_2025c9 = (cursor < 4U) ? cursor : 0U;
    submode = FileSelectSubmode::Main;
    last_action = FileSelectAction::None;
    selected_slot = 0U;
    sound_commands.clear();
}

bool FileSelectMenuState::navigate(bool up, bool down, bool left, bool right) {
    const std::uint8_t max_row = (submode == FileSelectSubmode::Main) ? 4U : 2U;
    const std::uint8_t excluded_slot =
        (submode == FileSelectSubmode::CopyDest) ? copy_src_slot : 0xFFU;

    std::uint8_t cand = cursor;
    if (right) {
        cand = static_cast<std::uint8_t>(cand | 1U);
    } else if (left) {
        cand = static_cast<std::uint8_t>(cand & ~1U);
    }
    if (up && cand > 1U) {
        cand = static_cast<std::uint8_t>(cand - 2U);
    } else if (down && cand < max_row) {
        cand = static_cast<std::uint8_t>(cand + 2U);
    }

    if (cand == excluded_slot || cand == cursor) {
        return false;
    }

    cursor = cand;
    sound_commands.push_back(FileSelectConfig::kCursorMoveSfx);
    return true;
}

void FileSelectMenuState::step(const FileSelectInput& input) {
    last_action = FileSelectAction::None;

    if (input.up || input.down || input.left || input.right) {
        navigate(input.up, input.down, input.left, input.right);
    }

    if (input.cancel) {
        switch (submode) {
            case FileSelectSubmode::Main:
                last_action = FileSelectAction::ReturnToTitle;
                return;
            case FileSelectSubmode::CopySource:
                submode = FileSelectSubmode::Main;
                cursor = 4U;
                return;
            case FileSelectSubmode::CopyDest:
                submode = FileSelectSubmode::CopySource;
                cursor = copy_src_slot;
                return;
            case FileSelectSubmode::EraseSelect:
                submode = FileSelectSubmode::Main;
                cursor = 5U;
                return;
        }
    }

    if (!input.confirm) {
        return;
    }

    switch (submode) {
        case FileSelectSubmode::Main: {
            if (cursor < 4U) {
                selected_slot = cursor;
                last_slot_sram_2025c9 = cursor;
                sound_commands.push_back(FileSelectConfig::kConfirmSfx);
                last_action = ((occupied_mask & static_cast<std::uint8_t>(1U << cursor)) != 0U)
                    ? FileSelectAction::ContinueGame
                    : FileSelectAction::NewGame;
                return;
            }
            if (cursor == 4U) {
                if (occupied_mask == 0U) {
                    return; // Aborts Copy if $FF10AA == 0
                }
                submode = FileSelectSubmode::CopySource;
                cursor = 0U;
                sound_commands.push_back(FileSelectConfig::kConfirmSfx);
                return;
            }
            if (cursor == 5U) {
                if (occupied_mask == 0U) {
                    return; // Aborts Erase if $FF10AA == 0
                }
                submode = FileSelectSubmode::EraseSelect;
                cursor = 0U;
                sound_commands.push_back(FileSelectConfig::kConfirmSfx);
                return;
            }
            return;
        }
        case FileSelectSubmode::CopySource: {
            if ((occupied_mask & static_cast<std::uint8_t>(1U << cursor)) == 0U) {
                sound_commands.push_back(FileSelectConfig::kInvalidEraseSfx);
                return;
            }
            copy_src_slot = cursor;
            submode = FileSelectSubmode::CopyDest;
            cursor = static_cast<std::uint8_t>(cursor ^ 1U); // Toggle bit 0 for initial dest != src
            sound_commands.push_back(FileSelectConfig::kConfirmSfx);
            return;
        }
        case FileSelectSubmode::CopyDest: {
            occupied_mask = static_cast<std::uint8_t>(occupied_mask | (1U << cursor));
            selected_slot = cursor;
            last_slot_sram_2025c9 = cursor;
            submode = FileSelectSubmode::Main;
            cursor = 4U;
            last_action = FileSelectAction::CopyCompleted;
            sound_commands.push_back(FileSelectConfig::kConfirmSfx);
            return;
        }
        case FileSelectSubmode::EraseSelect: {
            if ((occupied_mask & static_cast<std::uint8_t>(1U << cursor)) == 0U) {
                sound_commands.push_back(FileSelectConfig::kInvalidEraseSfx);
                return;
            }
            occupied_mask = static_cast<std::uint8_t>(occupied_mask & ~(1U << cursor));
            selected_slot = cursor;
            submode = FileSelectSubmode::Main;
            cursor = 5U;
            last_action = FileSelectAction::EraseCompleted;
            sound_commands.push_back(FileSelectConfig::kConfirmSfx);
            return;
        }
    }
}

} // namespace oasis::game::ui
