#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

namespace oasis::game::ui {

inline constexpr std::uint32_t kHudSatEmitterAddr = 0x00A342U;
inline constexpr std::size_t kHudSatMaxSprites = 80U;
inline constexpr std::size_t kPrimaryHudSpriteCount = 6U;
inline constexpr std::size_t kEnemyGaugeSpriteCount = 3U;
inline constexpr std::uint16_t kEnemyMirrorBaseX = 0x0158U;

inline constexpr std::uint16_t kToastSpriteY = 0x0118U;
inline constexpr std::uint8_t kToastSpriteSizeHi = 0x0AU; // 3x3
inline constexpr std::uint16_t kToastSpriteTile = 0x85C7U;
inline constexpr std::uint16_t kToastSpriteXBias = 0x0068U;

inline constexpr std::uint32_t kToastIconRomBase = 0x000AAEBBU;
inline constexpr std::uint32_t kToastIconStride = 0x0090U;
inline constexpr std::uint16_t kToastIconVramDst = 0xB8E0U;
inline constexpr std::uint16_t kToastIconDmaWords = 0x0090U;

struct HudToastSatSprite {
    std::uint16_t y{kToastSpriteY};
    std::uint8_t size_hi{kToastSpriteSizeHi};
    std::uint16_t tile{kToastSpriteTile};
    std::uint16_t x{0U};
};

struct HudToastDmaDescriptor {
    std::uint32_t rom_byte_addr{0U};
    std::uint16_t vram_dst{kToastIconVramDst};
    std::uint16_t len_words{kToastIconDmaWords};
};

struct HudSatAndToastPlan {
    std::size_t primary_hud_sprites_emitted{0U};
    std::size_t enemy_gauge_sprites_emitted{0U};
    std::uint16_t primary_shift_x{0U};
    std::uint16_t enemy_shift_x{0U};
    std::optional<HudToastSatSprite> toast_sprite{};
    std::optional<HudToastDmaDescriptor> toast_dma{};
    std::size_t total_sat_count{0U};
};

/// Models 0x00A342..0x00A436: emits 6 primary HUD sprites shifted by slide_x,
/// 3 enemy-gauge sprites shifted by (0x0158 - slide_x) when active, and
/// 3x3 item-toast sprite (0x85C7 at (slide_x + 0x68, 0x0118)) + 0x90-word DMA
/// when toast_timer != 0 and SAT count < 80.
[[nodiscard]] HudSatAndToastPlan compute_hud_sat_and_toast_plan(
    std::uint16_t slide_x,
    bool enemy_gauge_active,
    std::uint8_t toast_timer,
    std::uint8_t toast_item_id,
    std::uint8_t cached_toast_item_id,
    std::size_t current_sat_count) noexcept;

} // namespace oasis::game::ui
