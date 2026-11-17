#include "game/entities/velocity_vector_math.hpp"

#include <cassert>

namespace {

using namespace oasis::game::entities;

void test_8dir_velocity_vectors() {
    // speed = 0x0100 -> 16.16 fixed-point 0x00010000 (1.0 px/frame)
    // 0.75 scaled = (0x10000 >> 1) + (0x10000 >> 2) = 0xC000
    const auto n = compute_8dir_velocity(0x0100U, 0x0100U, 0U);
    assert(n.vx == 0 && n.vy == -0x00010000);

    const auto e = compute_8dir_velocity(0x0100U, 0x0100U, 1U);
    assert(e.vx == 0x00010000 && e.vy == 0);

    const auto s = compute_8dir_velocity(0x0100U, 0x0100U, 2U);
    assert(s.vx == 0 && s.vy == 0x00010000);

    const auto w = compute_8dir_velocity(0x0100U, 0x0100U, 3U);
    assert(w.vx == -0x00010000 && w.vy == 0);

    const auto ne = compute_8dir_velocity(0x0100U, 0x0100U, 4U);
    assert(ne.vx == 0x0000C000 && ne.vy == -0x0000C000);

    const auto se = compute_8dir_velocity(0x0100U, 0x0100U, 5U);
    assert(se.vx == 0x0000C000 && se.vy == 0x0000C000);

    const auto sw = compute_8dir_velocity(0x0100U, 0x0100U, 6U);
    assert(sw.vx == -0x0000C000 && sw.vy == 0x0000C000);

    const auto nw = compute_8dir_velocity(0x0100U, 0x0100U, 7U);
    assert(nw.vx == -0x0000C000 && nw.vy == -0x0000C000);

    // Direction wraps mod 8 and supports asymmetric sx/sy
    const auto wrapped = compute_8dir_velocity(0x0200U, 0x0100U, 12U); // 12 & 7 == 4 (NE)
    assert(wrapped.vx == 0x00018000 && wrapped.vy == -0x0000C000);
}

void test_cardinal_facing_resolution() {
    // Dominant vertical -> 0 (N) or 2 (S)
    assert(compute_cardinal_facing(0, -100) == 0U);
    assert(compute_cardinal_facing(50, -100) == 0U);
    assert(compute_cardinal_facing(0, 100) == 2U);
    assert(compute_cardinal_facing(-50, 100) == 2U);

    // Tie (|vx| == |vy|) resolves vertically (|vx| <= |vy|)
    assert(compute_cardinal_facing(100, -100) == 0U);
    assert(compute_cardinal_facing(-100, 100) == 2U);
    assert(compute_cardinal_facing(0, 0) == 2U);

    // Dominant horizontal -> 1 (E) or 3 (W)
    assert(compute_cardinal_facing(101, 100) == 1U);
    assert(compute_cardinal_facing(101, -100) == 1U);
    assert(compute_cardinal_facing(-101, 100) == 3U);
    assert(compute_cardinal_facing(-101, -100) == 3U);
}

} // namespace

int main() {
    test_8dir_velocity_vectors();
    test_cardinal_facing_resolution();
    return 0;
}
