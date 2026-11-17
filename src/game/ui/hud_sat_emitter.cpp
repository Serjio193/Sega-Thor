#include "game/ui/hud_sat_emitter.hpp"

#include <algorithm>

namespace oasis::game::ui {

HudSatAndToastPlan compute_hud_sat_and_toast_plan(
    std::uint16_t slide_x,
    bool enemy_gauge_active,
    std::uint8_t toast_timer,
    std::uint8_t toast_item_id,
    std::uint8_t cached_toast_item_id,
    std::size_t current_sat_count) noexcept {
    HudSatAndToastPlan plan{};
    plan.primary_shift_x = slide_x;
    plan.enemy_shift_x = static_cast<std::uint16_t>(kEnemyMirrorBaseX - slide_x);

    std::size_t sat_count = std::min(current_sat_count, kHudSatMaxSprites);
    const std::size_t primary_room = kHudSatMaxSprites - sat_count;
    plan.primary_hud_sprites_emitted = std::min(kPrimaryHudSpriteCount, primary_room);
    sat_count += plan.primary_hud_sprites_emitted;

    if (enemy_gauge_active && sat_count < kHudSatMaxSprites) {
        const std::size_t enemy_room = kHudSatMaxSprites - sat_count;
        plan.enemy_gauge_sprites_emitted = std::min(kEnemyGaugeSpriteCount, enemy_room);
        sat_count += plan.enemy_gauge_sprites_emitted;
    }

    if (toast_timer != 0U && sat_count < kHudSatMaxSprites) {
        plan.toast_sprite = HudToastSatSprite{
            kToastSpriteY,
            kToastSpriteSizeHi,
            kToastSpriteTile,
            static_cast<std::uint16_t>(slide_x + kToastSpriteXBias),
        };
        ++sat_count;

        if (toast_item_id != cached_toast_item_id) {
            plan.toast_dma = HudToastDmaDescriptor{
                kToastIconRomBase + static_cast<std::uint32_t>(toast_item_id) * kToastIconStride,
                kToastIconVramDst,
                kToastIconDmaWords,
            };
        }
    }

    plan.total_sat_count = sat_count;
    return plan;
}

} // namespace oasis::game::ui
