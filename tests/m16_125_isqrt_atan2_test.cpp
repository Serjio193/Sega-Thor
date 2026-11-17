#include "game/math/isqrt_atan2.hpp"

#include <cassert>

namespace {

using namespace oasis::game::math;

void test_isqrt() {
    assert(isqrt_u32(0U) == 0U);
    assert(isqrt_u32(1U) == 1U);
    assert(isqrt_u32(2U) == 1U);
    assert(isqrt_u32(3U) == 1U);
    assert(isqrt_u32(4U) == 2U);
    assert(isqrt_u32(144U) == 12U);
    assert(isqrt_u32(1024U) == 32U);
    assert(isqrt_u32(10000U) == 100U);
    assert(isqrt_u32(65535U) == 255U);
    assert(isqrt_u32(1048576U) == 1024U);
}

void test_atan2_octants_and_quadrants() {
    // Principal axes
    assert(atan2_256(100, 0) == 0U);     // Right (0)
    assert(atan2_256(100, 100) == 32U);  // Up-Right (45 deg)
    assert(atan2_256(0, 100) == 64U);    // Up (90 deg)
    assert(atan2_256(-100, 100) == 96U); // Up-Left (135 deg)
    assert(atan2_256(-100, 0) == 128U);  // Left (180 deg)
    assert(atan2_256(-100, -100) == 160U); // Down-Left (225 deg)
    assert(atan2_256(0, -100) == 192U);  // Down (270 deg)
    assert(atan2_256(100, -100) == 224U); // Down-Right (315 deg)

    // Intermediate angle threshold checks in octant 1 (dx=100)
    // ratio = (dy << 8) / 100
    // dy=5: ratio=12 <= 0x19 -> angle 0
    assert(atan_q1(100, 5) == 0U);
    // dy=15: ratio=38 (0x26) <= 0x4E -> angle 8
    assert(atan_q1(100, 15) == 8U);
    // dy=30: ratio=76 (0x4C) <= 0x4E -> angle 8
    assert(atan_q1(100, 30) == 8U);
    // dy=40: ratio=102 (0x66) <= 0x89 -> angle 16
    assert(atan_q1(100, 40) == 16U);
}

void test_atan2_to_point_and_player() {
    // Entity at (100, 100) targeting points:
    // Right: (200, 100) -> dx=100, dy_up=0 -> angle 0
    assert(atan2_to_point(100, 100, 200, 100) == 0U);
    // Up: (100, 50) -> dx=0, dy_up=50 -> angle 64
    assert(atan2_to_point(100, 100, 100, 50) == 64U);
    // Left: (50, 100) -> dx=-50, dy_up=0 -> angle 128
    assert(atan2_to_point(100, 100, 50, 100) == 128U);
    // Down: (100, 150) -> dx=0, dy_up=-50 -> angle 192
    assert(atan2_to_point(100, 100, 100, 150) == 192U);

    // Player target helper matches to_point
    assert(atan2_to_player(100, 100, 200, 100) == 0U);
    assert(atan2_to_player(100, 100, 100, 50) == 64U);
}

void test_rom_evidence_lut_values() {
    assert(kTangentLut0x05D906[0] == 0x0019U);
    assert(kTangentLut0x05D906[1] == 0x004EU);
    assert(kTangentLut0x05D906[2] == 0x0089U);
    assert(kTangentLut0x05D906[3] == 0x00D2U);
    assert(kTangentLut0x05D906[4] == 0x0138U);
    assert(kTangentLut0x05D906[5] == 0x01DFU);
    assert(kTangentLut0x05D906[6] == 0x034CU);
    assert(kTangentLut0x05D906[7] == 0x0A27U);
    assert(kTangentLut0x05D906[8] == 0xFFFFU);
}

} // namespace

int main() {
    test_isqrt();
    test_atan2_octants_and_quadrants();
    test_atan2_to_point_and_player();
    test_rom_evidence_lut_values();
    return 0;
}
