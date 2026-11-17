#include "game/world/aabb_solid_sweep_3axis.hpp"

#include <array>
#include <cassert>

namespace {

using namespace oasis::game::entities;
using namespace oasis::game::world;

void test_neg_x_clamp_and_zero_gap_push() {
    SolidSweepBlocker mover{100, 100, 0, 0, 8, 8, 16, true, false};
    std::array<SolidSweepBlocker, 3> slots{};
    slots[0] = mover;
    // Blocker at x=80, half_w=8 -> right edge 88; mover left edge 92 -> gap=4
    slots[1] = {80, 100, 0, 0, 8, 8, 16, true, true};
    // Z-non-overlapping blocker closer (gap=1, z=32..48) must be skipped
    slots[2] = {83, 100, 32, 0, 8, 8, 16, true, false};

    const auto clamped = sweep_aabb_solid_3axis(SolidSweepAxis::NegX, mover, 0U, -10, slots);
    assert(clamped.clamped_delta == -4);
    assert(clamped.blocker_index == 1U);
    assert(clamped.contact_type == kSolidContactType);
    assert(clamped.hit_flag_bit7);
    assert(clamped.push_delta == 0);

    // Zero gap -> push_delta == -1 when pushable and mover_can_push
    mover.x = 96; // left=88 == blocker right=88 -> gap=0
    const auto pushed = sweep_aabb_solid_3axis(SolidSweepAxis::NegX, mover, 0U, -4, slots, true);
    assert(pushed.clamped_delta == 0);
    assert(pushed.push_delta == -1);

    // mover_can_push=false blocks push
    const auto no_push = sweep_aabb_solid_3axis(SolidSweepAxis::NegX, mover, 0U, -4, slots, false);
    assert(no_push.clamped_delta == 0);
    assert(no_push.push_delta == 0);
}

void test_pos_y_and_neg_y_clamp_and_push() {
    SolidSweepBlocker mover{100, 100, 0, 0, 8, 8, 16, true, false};
    std::array<SolidSweepBlocker, 3> slots{};
    slots[0] = mover;
    // +Y blocker at y=119, half_h=8 -> top=111; mover bottom=108 -> gap=3
    slots[1] = {100, 119, 0, 0, 8, 8, 16, true, true};
    // -Y blocker at y=84, half_h=8 -> bottom=92; mover top=92 -> gap=0
    slots[2] = {100, 84, 0, 0, 8, 8, 16, true, true};

    const auto pos_y = sweep_aabb_solid_3axis(SolidSweepAxis::PosY, mover, 0U, 8, slots);
    assert(pos_y.clamped_delta == 3);
    assert(pos_y.blocker_index == 1U);
    assert(pos_y.push_delta == 0);

    mover.y = 103; // bottom=111 -> gap=0 with slots[1]
    const auto pos_y_push = sweep_aabb_solid_3axis(SolidSweepAxis::PosY, mover, 0U, 4, slots);
    assert(pos_y_push.clamped_delta == 0);
    assert(pos_y_push.push_delta == 1);

    mover.y = 100;
    const auto neg_y = sweep_aabb_solid_3axis(SolidSweepAxis::NegY, mover, 0U, -5, slots);
    assert(neg_y.clamped_delta == 0);
    assert(neg_y.blocker_index == 2U);
    assert(neg_y.push_delta == -1);
}

} // namespace

int main() {
    test_neg_x_clamp_and_zero_gap_push();
    test_pos_y_and_neg_y_clamp_and_push();
    return 0;
}
