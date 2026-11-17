#include "game/ui/dialogue_sat_builder.hpp"

#include <cassert>
#include <vector>

namespace {

using namespace oasis::game::ui;

void test_cursor_blinking_and_choice_rows() {
    DialogueSatInput in{};
    in.cursor_active = true;
    in.blink_visible = false;
    in.choice_row = 0;

    std::vector<DialogueSatEntry> sat;
    build_dialogue_sat(in, sat);
    assert(sat.empty()); // hidden when blink_visible == false

    in.blink_visible = true;
    in.cursor_alt_palette = false;
    in.choice_row = 0;
    build_dialogue_sat(in, sat);
    assert(sat.size() == 1U);
    assert(sat[0].x == 0x011CU);
    assert(sat[0].y == 0x014CU - 36U);
    assert(sat[0].tile == 0x86DCU);
    assert(sat[0].size_link == 0x0001U);

    // Choice 3 (and clamped > 3) -> Y = 0x014C, alt palette -> 0x96DC
    sat.clear();
    in.cursor_alt_palette = true;
    in.choice_row = 5;
    build_dialogue_sat(in, sat);
    assert(sat.size() == 1U);
    assert(sat[0].y == 0x014CU);
    assert(sat[0].tile == 0x96DCU);
}

void test_4x8_border_bars_and_80_sprite_cap() {
    DialogueSatInput in{};
    in.cursor_active = true;
    in.blink_visible = true;
    in.choice_row = 2;
    in.top_bars_active = true;
    in.top_bar1_y = 0x0010U;
    in.top_bar1_vram = 0x8000U;
    in.top_bar2_y = 0x0020U;
    in.top_bar2_vram = 0x8200U;
    in.bottom_bars_active = true;
    in.bottom_bar1_y = 0x0030U;
    in.bottom_bar1_vram = 0x8400U;
    in.bottom_bar2_y = 0x0040U;
    in.bottom_bar2_vram = 0x8600U;

    std::vector<DialogueSatEntry> sat;
    build_dialogue_sat(in, sat);
    // 1 cursor + 4 * 8 border sprites = 33
    assert(sat.size() == 33U);
    assert(sat[1].x == 0x00A0U);
    assert(sat[1].y == 0x0120U);
    assert(sat[1].size_link == 0x0C02U);
    assert(sat[1].tile == ((0x8000U >> 5U) | 0x8000U));
    assert(sat[8].x == 0x00A0U + 7U * 0x0020U);
    assert(sat[8].tile == ((0x8000U >> 5U) | 0x8000U) + 7U * 4U);

    // 80-sprite SAT cap enforcement
    std::vector<DialogueSatEntry> full_sat(78U);
    emit_8_sprite_bar(full_sat, 0U, 0x8000U);
    assert(full_sat.size() == 80U);
    assert(!append_dialogue_sat_entry(full_sat, 0U, 0U, 0U, 0U));
}

} // namespace

int main() {
    test_cursor_blinking_and_choice_rows();
    test_4x8_border_bars_and_80_sprite_cap();
    return 0;
}
