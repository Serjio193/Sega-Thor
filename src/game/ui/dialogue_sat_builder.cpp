#include "game/ui/dialogue_sat_builder.hpp"

#include <algorithm>

namespace oasis::game::ui {

bool append_dialogue_sat_entry(
    std::vector<DialogueSatEntry>& sat,
    std::uint16_t y,
    std::uint16_t size_base,
    std::uint16_t tile,
    std::uint16_t x) {
    if (sat.size() >= kDialogueSatMaxSprites) {
        return false;
    }
    const std::uint16_t link = static_cast<std::uint16_t>(sat.size() + 1U);
    sat.push_back(DialogueSatEntry{
        y,
        static_cast<std::uint16_t>(size_base + link),
        tile,
        x,
    });
    return true;
}

void emit_8_sprite_bar(
    std::vector<DialogueSatEntry>& sat,
    std::uint16_t bar_y,
    std::uint16_t bar_vram) {
    const std::uint16_t y = static_cast<std::uint16_t>(bar_y + kBorderYBias);
    std::uint16_t tile = static_cast<std::uint16_t>((bar_vram >> 5U) | 0x8000U);
    std::uint16_t x = kBorderStartX;

    for (std::uint16_t i = 0U; i < kBorderSpritesPerBar; ++i) {
        if (!append_dialogue_sat_entry(sat, y, kBorderSizeLinkBase, tile, x)) {
            break;
        }
        x = static_cast<std::uint16_t>(x + kBorderXStep);
        tile = static_cast<std::uint16_t>(tile + kBorderTileStep);
    }
}

void build_dialogue_sat(
    const DialogueSatInput& input,
    std::vector<DialogueSatEntry>& sat) {
    if (input.cursor_active && input.blink_visible) {
        const std::uint16_t clamped_choice =
            static_cast<std::uint16_t>(std::min<std::uint8_t>(input.choice_row, 3U));
        const std::uint16_t y = static_cast<std::uint16_t>(
            kCursorYBase - kCursorYStep * (3U - clamped_choice));
        const std::uint16_t tile = static_cast<std::uint16_t>(
            kCursorTileBase | (input.cursor_alt_palette ? kCursorAltPaletteMask : 0U));
        append_dialogue_sat_entry(sat, y, 0x0000U, tile, kCursorX);
    }

    if (input.top_bars_active) {
        emit_8_sprite_bar(sat, input.top_bar1_y, input.top_bar1_vram);
        emit_8_sprite_bar(sat, input.top_bar2_y, input.top_bar2_vram);
    }
    if (input.bottom_bars_active) {
        emit_8_sprite_bar(sat, input.bottom_bar1_y, input.bottom_bar1_vram);
        emit_8_sprite_bar(sat, input.bottom_bar2_y, input.bottom_bar2_vram);
    }
}

} // namespace oasis::game::ui
