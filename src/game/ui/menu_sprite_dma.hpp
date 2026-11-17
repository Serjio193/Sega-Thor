#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace oasis::game::ui {

inline constexpr std::uint32_t kEmitItemAndDigitPairAddr = 0x004026U;
inline constexpr std::uint32_t kAppendMenuSatSpriteAddr = 0x004066U;
inline constexpr std::uint32_t kQueueSpriteTileDmaAddr = 0x0040A6U;
inline constexpr std::uint32_t kSpriteTileCountLutAddr = 0x0040CEU;
inline constexpr std::uint32_t kEmitFontStripSpritesAddr = 0x0040E2U;
inline constexpr std::uint32_t kAcquireSatAndDmaCursorsAddr = 0x004158U;

inline constexpr std::uint16_t kIconDefaultTileAttr = 0x86DDU;
inline constexpr std::uint16_t kDigitBaseTileAttr = 0x85D0U;
inline constexpr std::uint8_t kMaxDigitValue = 9U;
inline constexpr std::size_t kMenuSatMaxSprites = 80U; // 0x50
inline constexpr std::uint32_t kFontPrimaryRom = 0x00152F5AU;
inline constexpr std::uint32_t kFontAltRom = 0x0015375AU;

/// 16-byte sprite tile-count LUT at ROM 0x0040CE for Mega Drive size codes 0..15.
inline constexpr std::array<std::uint8_t, 16> kSpriteTileCountLut16 = {
    1U, 2U, 3U, 4U,
    2U, 4U, 6U, 8U,
    3U, 6U, 9U, 12U,
    4U, 8U, 12U, 16U
};

struct MenuSatSprite {
    std::uint16_t y{0};
    std::uint16_t size_link{0};
    std::uint16_t tile_attr{0};
    std::uint16_t x{0};
};

struct FontDmaChunk {
    std::uint32_t rom_byte_addr{0};
    std::uint16_t vram_dst{0};
    std::uint16_t len_words{16U};
};

struct FontStripPlan {
    std::vector<MenuSatSprite> sprites{};
    std::vector<FontDmaChunk> dma_chunks{};
};

/// Models 0x0040E2 ASCII remap: maps 0x2B..0x2F by +0x30.
[[nodiscard]] std::uint8_t remap_menu_ascii(std::uint8_t ch) noexcept;

/// Models 0x004066..0x0040A4: appends a sprite with +0x80 coordinate bias,
/// capped at 80 sprites.
bool append_menu_sat_sprite(
    std::vector<MenuSatSprite>& sat,
    std::int16_t screen_x,
    std::int16_t screen_y,
    std::uint8_t size_code,
    std::uint16_t tile_attr);

/// Models 0x004026..0x00405C: emits 0x86DD icon + 0x85D0 + min(qty, 9) digit at Y + 16.
bool emit_item_and_digit_pair(
    std::vector<MenuSatSprite>& sat,
    std::int16_t screen_x,
    std::int16_t screen_y,
    std::uint8_t quantity,
    std::uint16_t icon_tile_attr = kIconDefaultTileAttr);

/// Models 0x0040E2..0x004156: chunks text into <= 4-tile horizontal SAT sprites
/// and emits corresponding 16-word glyph DMA chunks from 0x152F5A / 0x15375A.
[[nodiscard]] FontStripPlan emit_font_strip_plan(
    std::string_view text,
    std::int16_t screen_x,
    std::int16_t screen_y,
    std::uint16_t vram_tile_base,
    std::uint16_t palette_flags,
    bool use_alt_font,
    std::vector<MenuSatSprite>& sat);

} // namespace oasis::game::ui
