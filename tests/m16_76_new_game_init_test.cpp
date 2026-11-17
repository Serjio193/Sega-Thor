#include "game/ui/new_game_init.hpp"

#include <cassert>

int main() {
    using oasis::game::ui::initialize_new_game;
    using oasis::game::ui::NewGameChordBranch;
    using oasis::game::ui::NewGameConfig;

    // 1. Normal New Game initialization (0x001300).
    const auto normal = initialize_new_game(0U);
    assert(normal.world_flags_ff0d80 == 0x01FFU);
    assert(normal.entity_id == 2U);
    assert(normal.facing == 2U);
    assert(normal.max_hp == 200U && normal.hp == 200U);
    assert(normal.max_sp == 0U && normal.sp == 0U);
    assert(normal.base_attack == 10U);
    assert(normal.active_spirit == 0U);
    assert(normal.start_room_id == 0x001BU);
    assert(normal.start_cam_x == 96 && normal.start_cam_y == 0);
    assert(normal.start_x == 256 && normal.start_y == 80);
    assert(normal.special_flag_ff0d7a == 0U);
    assert(normal.debug_flags == 0U);
    assert(normal.chord_branch == NewGameChordBranch::Normal);
    assert(normal.target_pc == NewGameConfig::kGameplayEntryAddr);

    // 2. Secret Menu chord (0xD6000100 -> 0x03C8DE).
    const auto secret = initialize_new_game(0xD6000100U);
    assert(secret.chord_branch == NewGameChordBranch::SecretMenu_0xD6000100);
    assert(secret.target_pc == 0x0003C8DEU);

    // 3. Debug Super Start chord (0xC9000200 -> 0x001450).
    const auto dbg = initialize_new_game(0xC9000200U);
    assert(dbg.chord_branch == NewGameChordBranch::DebugSuperStart_0xC9000200);
    assert(dbg.target_pc == 0x00001450U);
    assert(dbg.debug_flags != 0U);

    // 4. Special Flag chord (0xCA000100 -> 0x001394, $FF0D7A = 0x80).
    const auto spec = initialize_new_game(0xCA000100U);
    assert(spec.chord_branch == NewGameChordBranch::SpecialFlag_0xCA000100);
    assert(spec.target_pc == 0x00001394U);
    assert(spec.special_flag_ff0d7a == 0x80U);

    // 5. Alt Screen chord (0x90000000 -> 0x03E7F4).
    const auto alt = initialize_new_game(0x90000000U);
    assert(alt.chord_branch == NewGameChordBranch::AltScreen_0x90000000);
    assert(alt.target_pc == 0x0003E7F4U);

    return 0;
}
