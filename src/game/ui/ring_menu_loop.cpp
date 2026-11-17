#include "game/ui/ring_menu_loop.hpp"

namespace oasis::game::ui {

bool can_open_ring_menu(
    std::uint8_t menu_block,
    std::uint8_t player_state) noexcept {
    return menu_block == 0U && player_state <= kMaxOpenPlayerState;
}

RingMenuAction decode_ring_menu_action(
    std::uint8_t joypad,
    bool button_swap) noexcept {
    const std::uint8_t close_mask = button_swap
        ? static_cast<std::uint8_t>(kJoypadB | kJoypadC)
        : kJoypadA;
    const std::uint8_t confirm_mask = button_swap
        ? kJoypadA
        : static_cast<std::uint8_t>(kJoypadB | kJoypadC);

    if ((joypad & close_mask) != 0U) {
        return RingMenuAction::Close;
    }
    if ((joypad & confirm_mask) != 0U) {
        return RingMenuAction::Confirm;
    }
    if ((joypad & kJoypadStart) != 0U) {
        return RingMenuAction::Start;
    }
    if ((joypad & kJoypadUp) != 0U) {
        return RingMenuAction::Up;
    }
    if ((joypad & kJoypadDown) != 0U) {
        return RingMenuAction::Down;
    }
    if ((joypad & kJoypadLeft) != 0U) {
        return RingMenuAction::Left;
    }
    if ((joypad & kJoypadRight) != 0U) {
        return RingMenuAction::Right;
    }
    return RingMenuAction::None;
}

} // namespace oasis::game::ui
