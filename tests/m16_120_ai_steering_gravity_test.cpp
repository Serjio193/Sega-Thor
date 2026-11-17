#include "game/entities/ai_steering_gravity.hpp"

#include <cassert>

namespace {

using namespace oasis::game::entities;

void test_facing_manhattan_and_steer() {
    const auto fd = facing_and_manhattan_to_player(100, 100, 130, 110);
    assert(fd.facing == 1U); // Right
    assert(fd.manhattan == 40);

    const auto steer = steer_4way_toward_player(100, 100, 105, 92, 0x0100U, 0x0100U);
    assert(steer.facing == 0U); // Up (|dy|=8 >= |dx|=5)
    assert(steer.vx == 0 && steer.vy < 0);
    assert(steer.chase_timer == 16); // clamped to min 16

    const auto steer_far = steer_4way_toward_player(100, 100, 200, 120, 0x0100U, 0x0100U);
    assert(steer_far.facing == 1U);
    assert(steer_far.vx > 0 && steer_far.vy == 0);
    assert(steer_far.chase_timer == 100);
}

void test_aim_vector_and_z_gravity() {
    // Major axis X=40, minor Y=20 -> vx=2.0 (0x20000), vy=1.0 (0x10000)
    const auto aim = aim_fixed_point_vector(10, 20, 50, 40);
    assert(aim.vx == 0x00020000);
    assert(aim.vy == 0x00010000);

    const auto arc = launch_knockback_arc(1U); // East (dir 1 in 0x00E2D4)
    assert(arc.vx == (0x0300 << 8));
    assert(arc.vy == 0);
    assert(arc.vz == 0x00060000);

    // Exact 0 vz skip: 0x0000C000 - 0x0000C000 -> -1
    const auto skip0 = step_z_gravity(0x00050000, kGravityZ16_16);
    assert(!skip0.landed);
    assert(skip0.z == 0x0005C000);
    assert(skip0.vz == -1);

    // Landing when z < 0
    const auto land = step_z_gravity(0x00004000, -0x00010000);
    assert(land.landed);
    assert(land.z == 0 && land.vz == 0);
}

} // namespace

int main() {
    test_facing_manhattan_and_steer();
    test_aim_vector_and_z_gravity();
    return 0;
}
