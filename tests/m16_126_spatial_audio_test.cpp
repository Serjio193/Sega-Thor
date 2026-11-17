#include "game/audio/spatial_audio.hpp"

#include <cassert>

namespace {

using namespace oasis::game::audio;

void test_unattenuated_low_sound_id() {
    // Sound ID < 0x200 (system/bgm): unattenuated, unpanned, never culled
    const auto res = compute_spatial_sfx(0x0100U, -200, 0);
    assert(res.should_play);
    assert(res.attenuation_steps == 0U);
    assert(res.pan_flag == 0U);

    const auto cmd_res = apply_spatial_audio(0x00800100U, -200, 0);
    assert(cmd_res.should_play);
    assert(cmd_res.command == 0x00800100U);
}

void test_fm_psg_spatial_sfx() {
    // 0x0200 <= sound_id < 0x0300
    // Pan left at dx = -100 (|dx| >= 96), atten 0 (dist < 176)
    auto res = compute_spatial_sfx(0x0250U, -100, 0);
    assert(res.should_play);
    assert(res.attenuation_steps == 0U);
    assert(res.pan_flag == kPanLeftFlag);

    // Pan right at dx = 230, dist >= 224 (step 2 attenuation)
    res = compute_spatial_sfx(0x0250U, 230, 0);
    assert(res.should_play);
    assert(res.attenuation_steps == 2U);
    assert(res.pan_flag == kPanRightFlag);

    // Culled at dist >= 272 (0x110)
    res = compute_spatial_sfx(0x0250U, 272, 0);
    assert(!res.should_play);

    // Dist Y scaling (ady + (ady >> 2)) = 1.25 * dy
    // dy = 160 -> dist_y = 160 + 40 = 200 >= 176 (step 1 atten), center pan
    res = compute_spatial_sfx(0x0250U, 0, 160);
    assert(res.should_play);
    assert(res.attenuation_steps == 1U);
    assert(res.pan_flag == 0U);
}

void test_hires_spatial_sfx() {
    // sound_id >= 0x0300
    // dist = 80 <= 88 -> eff_dist = 0, drop = (0 >> 5) + 1 = 1, pan right (|dx| >= 64)
    auto res = compute_spatial_sfx(0x0350U, 80, 0);
    assert(res.should_play);
    assert(res.attenuation_steps == 1U);
    assert(res.pan_flag == kPanRightFlag);

    // dist = 400 -> eff_dist = 312 -> drop = (312 >> 5) + 1 = 10 > 7 -> culled
    res = compute_spatial_sfx(0x0350U, 400, 0);
    assert(!res.should_play);
}

void test_spatial_at_xy_and_entity() {
    // Entity at (300, 100), Player at (200, 100) -> dx = 100, dy = 0
    // sound_id = 0x0270 (thud), dist = 100 < 176, dx = 100 >= 96 -> pan right (0x80)
    const auto xy_res = play_spatial_at_xy(0x00800270U, 300, 100, 200, 100);
    assert(xy_res.should_play);
    assert(((xy_res.command >> 16U) & 0xFFU) == kPanRightFlag);

    const auto ent_res = play_spatial_at_entity(0x00800270U, 300, 100, 200, 100);
    assert(ent_res.should_play);
    assert(ent_res.command == xy_res.command);
}

} // namespace

int main() {
    test_unattenuated_low_sound_id();
    test_fm_psg_spatial_sfx();
    test_hires_spatial_sfx();
    test_spatial_at_xy_and_entity();
    return 0;
}
