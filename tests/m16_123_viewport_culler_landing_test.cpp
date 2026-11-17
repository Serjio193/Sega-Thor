#include "game/entities/viewport_culler_landing.hpp"

#include <array>
#include <cassert>

namespace {

using namespace oasis::game::entities;

void test_airborne_move_and_land() {
    // 1. Airborne step integrates XY and spins facing when idle + ff1971 bit 0
    AirborneEntityState s{};
    s.x_16_16 = 0x00100000;
    s.y_16_16 = 0x00200000;
    s.z_16_16 = 0x00020000;
    s.vz_16_16 = 0x0000C000;
    s.facing = 3U;

    auto res = step_airborne_move_and_land(s, true);
    assert(res.status == AirborneLandStatus::Airborne);
    assert(res.next_state.facing == 0U); // (3 + 1) & 3
    assert(res.sound_cmd == 0U);

    // Non-zero velocity integrates XY without spinning
    s.vx_16_16 = 0x00010000;
    s.vy_16_16 = -0x00010000;
    res = step_airborne_move_and_land(s, true);
    assert(res.status == AirborneLandStatus::Airborne);
    assert(res.next_state.x_16_16 == 0x00110000);
    assert(res.next_state.y_16_16 == 0x001F0000);
    assert(res.next_state.facing == 3U);

    // 2. Landing (z transitions < 0): 15f stun + thud SFX when flags_39 & 2
    AirborneEntityState land{};
    land.z_16_16 = 0x00001000;
    land.vz_16_16 = -0x00010000;
    land.air_timer_2e = 42U;
    land.byte_83 = 7U;
    land.flags_39 = 0x02U;

    res = step_airborne_move_and_land(land);
    assert(res.status == AirborneLandStatus::JustLanded);
    assert(res.next_state.z_16_16 == 0);
    assert(res.next_state.vz_16_16 == 0);
    assert(res.next_state.air_timer_2e == 0U);
    assert(res.next_state.byte_83 == 0U);
    assert(res.next_state.landing_stun_9a == kLandingStunFrames);
    assert((res.next_state.flags_39 & 0x04U) == 0U); // cleared when bit 1 set
    assert(res.sound_cmd == kLandingThudSfx);

    // Landing without flags_39 & 2 sets bit 2 and emits no thud SFX
    land.flags_39 = 0x00U;
    res = step_airborne_move_and_land(land);
    assert(res.status == AirborneLandStatus::JustLanded);
    assert((res.next_state.flags_39 & 0x04U) != 0U);
    assert(res.sound_cmd == 0U);

    // 3. Grounded return codes (3 vs 0)
    AirborneEntityState ground{};
    res = step_airborne_move_and_land(ground);
    assert(res.status == AirborneLandStatus::Grounded);
    ground.flags_37 = 0x02U;
    res = step_airborne_move_and_land(ground);
    assert(res.status == AirborneLandStatus::GroundedClear);
}

void test_viewport_cullers_and_copy() {
    // 320x224 viewport at cam=(100, 200), margin=16 -> [84..436] x [184..440]
    assert(!is_offscreen_ground(84, 184, 100, 200, 16));
    assert(!is_offscreen_ground(436, 440, 100, 200, 16));
    assert(is_offscreen_ground(83, 200, 100, 200, 16));
    assert(is_offscreen_ground(437, 200, 100, 200, 16));
    assert(is_offscreen_ground(200, 183, 100, 200, 16));
    assert(is_offscreen_ground(200, 441, 100, 200, 16));

    // is_offscreen_with_z subtracts z from y: y=200, z=20 -> py=180 < 184 -> offscreen
    assert(!is_offscreen_ground(200, 200, 100, 200, 16));
    assert(is_offscreen_with_z(200, 200, 20, 100, 200, 16));

    // Direct ROM test cases from rom_viewport_culler_landing_test.py
    assert(is_offscreen_ground(100, 260, 0, 0, 16));
    assert(!is_offscreen_with_z(100, 260, 40, 0, 0, 16));
    assert(!is_offscreen_ground(336, 240, 0, 0, 16));
    assert(is_offscreen_ground(337, 240, 0, 0, 16));

    // copy_entity_struct_0xbc copies all 188 bytes (94 words)
    std::array<std::uint8_t, kEntityStructBytes> src{};
    std::array<std::uint8_t, kEntityStructBytes> dst{};
    for (std::size_t i = 0U; i < src.size(); ++i) {
        src[i] = static_cast<std::uint8_t>((i * 7U + 3U) & 0xFFU);
    }
    copy_entity_struct_0xbc(src, dst);
    assert(src == dst);
}

} // namespace

int main() {
    test_airborne_move_and_land();
    test_viewport_cullers_and_copy();
    return 0;
}
