#include "game/entities/solid_entity_sweep.hpp"

#include <cassert>
#include <vector>

namespace {

using namespace oasis::game::entities;

void test_eligibility_flags() {
    assert(is_solid_sweep_eligible(0x00U, 0x00U));
    assert(!is_solid_sweep_eligible(0x02U, 0x00U)); // $3A bit 1 skips
    assert(!is_solid_sweep_eligible(0x00U, 0x04U)); // $38 bit 2 skips
}

void test_solid_x_sweep_and_push_contact() {
    std::vector<SolidSweepBlocker> slots(22U);
    for (auto& s : slots) {
        s.solid = false;
    }

    // Slot 0: mover at x=100, y=100, z=0, half_w=10, half_h=10, height_z=48
    slots[0] = {100, 100, 0, 0, 10, 10, 48, true, false};

    // Slot 1: non-solid blocker right in front -> skipped
    slots[1] = {115, 100, 0, 0, 10, 10, 48, false, false};

    // Slot 2: different Y lane -> no overlap
    slots[2] = {115, 130, 0, 0, 10, 10, 48, true, false};

    // Slot 3: above mover Z range -> no overlap
    slots[3] = {115, 100, 50, 0, 10, 10, 20, true, false};

    // Slot 4: solid blocker at x=124 (left edge = 114, mover_right = 110 -> clearance = 4)
    slots[4] = {124, 100, 0, 0, 10, 10, 48, true, true};

    // Slot 21 (beyond 21-slot limit) even closer -> must be ignored
    slots[21] = {121, 100, 0, 0, 10, 10, 48, true, false};

    // Move +6 px -> clamped to 4 px against slot 4
    const auto res = sweep_solid_x_pos(slots[0], 0U, 6, slots);
    assert(res.clamped_dx == 4);
    assert(res.blocker_index == 4U);
    assert(res.col_flag_bit7);
    assert(res.push_timer == 3U);
    assert(!res.nudge_blocker); // clearance is 4, not 0

    // Now mover is flush against pushable slot 4 (x = 104, mover_right = 114, clearance = 0)
    slots[0].x = 104;
    const auto flush = sweep_solid_x_pos(slots[0], 0U, 4, slots);
    assert(flush.clamped_dx == 0);
    assert(flush.blocker_index == 4U);
    assert(flush.col_flag_bit7);
    assert(flush.push_timer == 3U);
    assert(flush.nudge_blocker);

    // Non-pushable blocker at clearance == 0 does not set nudge_blocker
    slots[4].pushable = false;
    const auto flush_static = sweep_solid_x_pos(slots[0], 0U, 4, slots);
    assert(flush_static.clamped_dx == 0);
    assert(flush_static.col_flag_bit7);
    assert(!flush_static.nudge_blocker);
}

} // namespace

int main() {
    test_eligibility_flags();
    test_solid_x_sweep_and_push_contact();
    return 0;
}
