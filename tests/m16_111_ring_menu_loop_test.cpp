#include "game/ui/ring_menu_loop.hpp"

#include <cassert>

namespace {

using namespace oasis::game::ui;

void test_entry_guard() {
    assert(can_open_ring_menu(0U, 0U));
    assert(can_open_ring_menu(0U, 1U));
    assert(can_open_ring_menu(0U, 2U));

    // Blocked by player_state > 2 or menu_block != 0 (SFX 0x00A00253)
    assert(!can_open_ring_menu(0U, 3U));
    assert(!can_open_ring_menu(1U, 0U));
    assert(!can_open_ring_menu(1U, 2U));
}

void test_button_decode_normal_and_swapped() {
    // Normal ($FF1996 == 0): A closes (0x00A00347), B/C confirms
    assert(decode_ring_menu_action(kJoypadA, false) == RingMenuAction::Close);
    assert(decode_ring_menu_action(kJoypadB, false) == RingMenuAction::Confirm);
    assert(decode_ring_menu_action(kJoypadC, false) == RingMenuAction::Confirm);

    // Swapped ($FF1996 != 0): B/C closes, A confirms
    assert(decode_ring_menu_action(kJoypadA, true) == RingMenuAction::Confirm);
    assert(decode_ring_menu_action(kJoypadB, true) == RingMenuAction::Close);
    assert(decode_ring_menu_action(kJoypadC, true) == RingMenuAction::Close);

    // Start and D-pad directions
    assert(decode_ring_menu_action(kJoypadStart, false) == RingMenuAction::Start);
    assert(decode_ring_menu_action(kJoypadUp, false) == RingMenuAction::Up);
    assert(decode_ring_menu_action(kJoypadDown, false) == RingMenuAction::Down);
    assert(decode_ring_menu_action(kJoypadLeft, true) == RingMenuAction::Left);
    assert(decode_ring_menu_action(kJoypadRight, true) == RingMenuAction::Right);
    assert(decode_ring_menu_action(0U, false) == RingMenuAction::None);
}

} // namespace

int main() {
    test_entry_guard();
    test_button_decode_normal_and_swapped();
    return 0;
}
