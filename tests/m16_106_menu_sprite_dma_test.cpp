#include "game/ui/menu_sprite_dma.hpp"

#include <array>
#include <cassert>
#include <vector>

namespace {

using namespace oasis::game::ui;

void test_lut_and_ascii_remap() {
    constexpr std::array<std::uint8_t, 16> expected = {
        1U, 2U, 3U, 4U, 2U, 4U, 6U, 8U,
        3U, 6U, 9U, 12U, 4U, 8U, 12U, 16U
    };
    assert(kSpriteTileCountLut16 == expected);

    assert(remap_menu_ascii('+') == 0x5BU); // 0x2B + 0x30
    assert(remap_menu_ascii('/') == 0x5FU); // 0x2F + 0x30
    assert(remap_menu_ascii('A') == 'A');
    assert(remap_menu_ascii('0') == '0');
}

void test_font_strip_chunking() {
    std::vector<MenuSatSprite> sat;
    // 6-char string -> 4-tile chunk (size_code 0x0C) + 2-tile chunk (size_code 0x04)
    const auto plan = emit_font_strip_plan("HP+99!", 16, 24, 0x0100U, 0x8000U, false, sat);
    assert(plan.sprites.size() == 2U);
    assert(sat.size() == 2U);
    assert(plan.sprites[0].x == 16 + 0x80 && plan.sprites[0].y == 24 + 0x80);
    assert((plan.sprites[0].size_link >> 8U) == 0x0CU);
    assert(kSpriteTileCountLut16[plan.sprites[0].size_link >> 8U] == 4U);
    assert(plan.sprites[0].tile_attr == 0x8100U);

    assert(plan.sprites[1].x == 48 + 0x80);
    assert((plan.sprites[1].size_link >> 8U) == 0x04U);
    assert(kSpriteTileCountLut16[plan.sprites[1].size_link >> 8U] == 2U);
    assert(plan.sprites[1].tile_attr == 0x8104U);

    assert(plan.dma_chunks.size() == 6U);
    // '+' at index 2 remapped to 0x5B
    assert(plan.dma_chunks[2].rom_byte_addr == kFontPrimaryRom + 0x5BU * 32U);
}

void test_item_and_digit_pair_and_cap() {
    std::vector<MenuSatSprite> sat;
    assert(emit_item_and_digit_pair(sat, 40, 60, 15U)); // qty clamped to 9
    assert(sat.size() == 2U);
    assert(sat[0].tile_attr == 0x86DDU);
    assert(sat[0].y == 60 + 0x80);
    assert(sat[1].tile_attr == 0x85D0U + 9U);
    assert(sat[1].y == 76 + 0x80);

    // 80-sprite cap
    sat.resize(79U);
    assert(!emit_item_and_digit_pair(sat, 0, 0, 1U));
    assert(sat.size() == 80U);
}

} // namespace

int main() {
    test_lut_and_ascii_remap();
    test_font_strip_chunking();
    test_item_and_digit_pair_and_cap();
    return 0;
}
