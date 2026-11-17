#include "game/video/ysort_sat_pipeline.hpp"

#include <array>
#include <cassert>

namespace {

using namespace oasis::game::video;

void test_five_render_passes_and_ysort() {
    const std::array<bool, 4> hi_fx = {false, true, false, false};
    const std::array<bool, 4> sparks = {true, false, false, true};
    const std::array<SatRenderSlotInput, 5> entities = {{
        {true, 50},
        {false, 200}, // inactive -> skipped
        {true, 120},
        {true, -1},   // y <= -1 sentinel -> skipped
        {true, 80},
    }};
    const std::array<bool, 8> ground_fx = {false, false, true, false, false, false, false, false};
    const std::array<bool, 6> floor_fx = {true, false, false, false, false, false};

    const auto order = build_ysort_sat_render_order(hi_fx, sparks, entities, ground_fx, floor_fx);
    assert(order.size() == 8U);

    // Pass 1: High-priority FX (slot 1)
    assert(order[0].pass == SatRenderPassKind::HighPriorityFx && order[0].slot_index == 1U);
    // Pass 2: Hit-sparks (slots 0, 3)
    assert(order[1].pass == SatRenderPassKind::HitSparks && order[1].slot_index == 0U);
    assert(order[2].pass == SatRenderPassKind::HitSparks && order[2].slot_index == 3U);
    // Pass 3: Entities sorted by descending Y (120 -> slot 2, 80 -> slot 4, 50 -> slot 0)
    assert(order[3].pass == SatRenderPassKind::YSortedEntity && order[3].slot_index == 2U);
    assert(order[4].pass == SatRenderPassKind::YSortedEntity && order[4].slot_index == 4U);
    assert(order[5].pass == SatRenderPassKind::YSortedEntity && order[5].slot_index == 0U);
    // Pass 4: Ground/shadow FX (slot 2)
    assert(order[6].pass == SatRenderPassKind::GroundShadowFx && order[6].slot_index == 2U);
    // Pass 5: Floor FX (slot 0)
    assert(order[7].pass == SatRenderPassKind::FloorFx && order[7].slot_index == 0U);
}

void test_sentinel_y_lt_minus_1_skipped() {
    const std::array<bool, 4> empty4{};
    const std::array<bool, 8> empty8{};
    const std::array<bool, 6> empty6{};
    const std::array<SatRenderSlotInput, 3> entities = {{
        {true, -5},
        {true, -1},
        {true, 0},
    }};

    const auto order = build_ysort_sat_render_order(empty4, empty4, entities, empty8, empty6);
    assert(order.size() == 1U);
    assert(order[0].pass == SatRenderPassKind::YSortedEntity && order[0].slot_index == 2U);
}

} // namespace

int main() {
    test_five_render_passes_and_ysort();
    test_sentinel_y_lt_minus_1_skipped();
    return 0;
}
