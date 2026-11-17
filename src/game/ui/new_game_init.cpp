#include "game/ui/new_game_init.hpp"

namespace oasis::game::ui {

NewGameStartState initialize_new_game(std::uint32_t input_ff165e_l) noexcept {
    NewGameStartState state{};

    if (input_ff165e_l == NewGameConfig::kMaskSecretMenu) {
        state.chord_branch = NewGameChordBranch::SecretMenu_0xD6000100;
        state.target_pc = NewGameConfig::kTargetSecretMenu;
    } else if (input_ff165e_l == NewGameConfig::kMaskDebugSuperStart) {
        state.chord_branch = NewGameChordBranch::DebugSuperStart_0xC9000200;
        state.target_pc = NewGameConfig::kTargetDebugSuperStart;
        state.debug_flags = 0x01U;
    } else if (input_ff165e_l == NewGameConfig::kMaskSpecialFlag) {
        state.chord_branch = NewGameChordBranch::SpecialFlag_0xCA000100;
        state.target_pc = NewGameConfig::kTargetSpecialFlag;
        state.special_flag_ff0d7a = 0x80U;
    } else if (input_ff165e_l == NewGameConfig::kMaskAltScreen) {
        state.chord_branch = NewGameChordBranch::AltScreen_0x90000000;
        state.target_pc = NewGameConfig::kTargetAltScreen;
    }

    return state;
}

} // namespace oasis::game::ui
