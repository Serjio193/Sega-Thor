#include "game/ui/menu_sprite_dma.hpp"

#include <algorithm>

namespace oasis::game::ui {

std::uint8_t remap_menu_ascii(std::uint8_t ch) noexcept {
    if (ch >= 0x2BU && ch <= 0x2FU) {
        return static_cast<std::uint8_t>(ch + 0x30U);
    }
    return ch;
}

bool append_menu_sat_sprite(
    std::vector<MenuSatSprite>& sat,
    std::int16_t screen_x,
    std::int16_t screen_y,
    std::uint8_t size_code,
    std::uint16_t tile_attr) {
    if (sat.size() >= kMenuSatMaxSprites) {
        return false;
    }
    const std::uint16_t link = static_cast<std::uint16_t>(sat.size() + 1U);
    const std::uint16_t size_link = static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(size_code & 0x0FU) << 8U) | link);
    sat.push_back(MenuSatSprite{
        static_cast<std::uint16_t>(screen_y + 0x80),
        size_link,
        tile_attr,
        static_cast<std::uint16_t>(screen_x + 0x80),
    });
    return true;
}

bool emit_item_and_digit_pair(
    std::vector<MenuSatSprite>& sat,
    std::int16_t screen_x,
    std::int16_t screen_y,
    std::uint8_t quantity,
    std::uint16_t icon_tile_attr) {
    if (!append_menu_sat_sprite(sat, screen_x, screen_y, 0x05U, icon_tile_attr)) {
        return false;
    }
    const std::uint16_t digit_tile = static_cast<std::uint16_t>(
        kDigitBaseTileAttr + std::min<std::uint8_t>(quantity, kMaxDigitValue));
    return append_menu_sat_sprite(
        sat,
        screen_x,
        static_cast<std::int16_t>(screen_y + 16),
        0x00U,
        digit_tile);
}

FontStripPlan emit_font_strip_plan(
    std::string_view text,
    std::int16_t screen_x,
    std::int16_t screen_y,
    std::uint16_t vram_tile_base,
    std::uint16_t palette_flags,
    bool use_alt_font,
    std::vector<MenuSatSprite>& sat) {
    FontStripPlan plan{};
    const std::uint32_t font_base = use_alt_font ? kFontAltRom : kFontPrimaryRom;

    std::size_t pos = 0U;
    std::int16_t cur_x = screen_x;
    std::uint16_t cur_vram_tile = vram_tile_base;
    while (pos < text.size()) {
        const std::size_t chunk_tiles = std::min<std::size_t>(4U, text.size() - pos);
        const std::uint8_t size_code =
            static_cast<std::uint8_t>((chunk_tiles - 1U) << 2U);
        const std::uint16_t tile_attr =
            static_cast<std::uint16_t>(palette_flags | cur_vram_tile);

        if (!append_menu_sat_sprite(sat, cur_x, screen_y, size_code, tile_attr)) {
            break;
        }
        plan.sprites.push_back(sat.back());

        for (std::size_t i = 0U; i < chunk_tiles; ++i) {
            const std::uint8_t glyph =
                remap_menu_ascii(static_cast<std::uint8_t>(text[pos + i]));
            plan.dma_chunks.push_back(FontDmaChunk{
                font_base + static_cast<std::uint32_t>(glyph) * 32U,
                static_cast<std::uint16_t>((cur_vram_tile + i) * 32U),
                16U,
            });
        }

        cur_x = static_cast<std::int16_t>(cur_x + static_cast<std::int16_t>(chunk_tiles * 8U));
        cur_vram_tile = static_cast<std::uint16_t>(cur_vram_tile + chunk_tiles);
        pos += chunk_tiles;
    }

    return plan;
}

} // namespace oasis::game::ui
