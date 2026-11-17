#include "game/video/sprite_dma_throttle.hpp"

#include <cassert>

namespace {

using namespace oasis::game::video;

void test_dma_budget_and_facing3_mirror() {
    // Uncached (cached_frame < 0) forces DMA even if dma_cost > dma_budget
    const auto uncached = select_entity_frame_with_dma_budget(3U, 10, -1, 100, 20);
    assert(uncached.hflip);
    assert(uncached.selected_frame == 11); // facing 3 -> 1 + 10
    assert(uncached.dma_queued);
    assert(!uncached.throttled);
    assert(uncached.next_cached_frame == 11);
    assert(uncached.backlog_flag == 0U);

    // Over budget with valid cached_frame >= 0 -> throttles back to cached_frame
    const auto throttled = select_entity_frame_with_dma_budget(0U, 12, 5, 50, 20);
    assert(!throttled.hflip);
    assert(throttled.selected_frame == 5);
    assert(!throttled.dma_queued);
    assert(throttled.throttled);
    assert(throttled.next_cached_frame == 5);
    assert(throttled.remaining_budget == 20);
    assert(throttled.backlog_flag == kDmaBacklogActiveFlag);

    // Within budget -> updates cached_frame and deducts budget
    const auto accepted = select_entity_frame_with_dma_budget(2U, 8, 5, 16, 20);
    assert(accepted.selected_frame == 10);
    assert(accepted.dma_queued);
    assert(!accepted.throttled);
    assert(accepted.remaining_budget == 4);

    // Cache hit -> no DMA
    const auto hit = select_entity_frame_with_dma_budget(3U, 10, 11, 16, 20);
    assert(hit.selected_frame == 11);
    assert(!hit.dma_queued);
    assert(!hit.throttled);
}

void test_shadow_visibility() {
    assert(should_draw_entity_shadow(5, 0x00U));    // z > 0
    assert(!should_draw_entity_shadow(5, 0x08U));   // bit 3 disables shadow
    assert(!should_draw_entity_shadow(0, 0x00U));   // z == 0 without bit 2
    assert(should_draw_entity_shadow(0, 0x04U));    // z == 0 with bit 2
    assert(!should_draw_entity_shadow(-1, 0x04U));  // z < 0
}

} // namespace

int main() {
    test_dma_budget_and_facing3_mirror();
    test_shadow_visibility();
    return 0;
}
