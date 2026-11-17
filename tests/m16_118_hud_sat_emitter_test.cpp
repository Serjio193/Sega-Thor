#include "game/ui/hud_sat_emitter.hpp"

#include <cassert>

namespace {

using namespace oasis::game::ui;

void test_hud_slide_mirror_and_toast() {
    const auto plan = compute_hud_sat_and_toast_plan(0x0020U, true, 30U, 5U, 2U, 10U);
    assert(plan.primary_hud_sprites_emitted == 6U);
    assert(plan.enemy_gauge_sprites_emitted == 3U);
    assert(plan.primary_shift_x == 0x0020U);
    assert(plan.enemy_shift_x == (0x0158U - 0x0020U));

    assert(plan.toast_sprite.has_value());
    assert(plan.toast_sprite->y == 0x0118U);
    assert(plan.toast_sprite->size_hi == 0x0AU);
    assert(plan.toast_sprite->tile == 0x85C7U);
    assert(plan.toast_sprite->x == 0x0020U + 0x0068U);

    assert(plan.toast_dma.has_value());
    assert(plan.toast_dma->rom_byte_addr == 0x000AAEBBU + 5U * 0x0090U);
    assert(plan.toast_dma->vram_dst == 0xB8E0U);
    assert(plan.toast_dma->len_words == 0x0090U);
    assert(plan.total_sat_count == 20U);
}

void test_cached_toast_dma_skip_and_80_sat_cap() {
    // Same item_id == cached_toast_item_id -> no DMA queued
    const auto cached = compute_hud_sat_and_toast_plan(0x0138U, false, 10U, 5U, 5U, 0U);
    assert(cached.primary_hud_sprites_emitted == 6U);
    assert(cached.enemy_gauge_sprites_emitted == 0U);
    assert(cached.enemy_shift_x == 0x0020U);
    assert(cached.toast_sprite.has_value());
    assert(!cached.toast_dma.has_value());
    assert(cached.total_sat_count == 7U);

    // SAT cap at 80: 74 + 6 primary = 80 -> toast sprite & DMA skipped
    const auto capped = compute_hud_sat_and_toast_plan(0x0020U, true, 30U, 5U, 2U, 74U);
    assert(capped.primary_hud_sprites_emitted == 6U);
    assert(capped.enemy_gauge_sprites_emitted == 0U);
    assert(!capped.toast_sprite.has_value());
    assert(!capped.toast_dma.has_value());
    assert(capped.total_sat_count == 80U);
}

} // namespace

int main() {
    test_hud_slide_mirror_and_toast();
    test_cached_toast_dma_skip_and_80_sat_cap();
    return 0;
}
