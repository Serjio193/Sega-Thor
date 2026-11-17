#include "game/video/vdp_clear_and_palette_cycle.hpp"

#include <array>
#include <cassert>

namespace {

using namespace oasis::game::video;

void test_vdp_clear_regions() {
    assert(kVdpClearRegions.size() == 6U);
    assert(kVdpClearRegions[0].addr == 0xDC00U && kVdpClearRegions[0].vdp_cmd == 0x5C000003U);
    assert(kVdpClearRegions[1].is_words && kVdpClearRegions[1].size_value == 40U);
    assert(kVdpClearRegions[2].size_value == 31U);
    assert(kVdpClearRegions[3].addr == 0xD000U && kVdpClearRegions[3].vdp_cmd == 0x50000003U);
    assert(kVdpClearRegions[4].addr == 0xC000U && kVdpClearRegions[4].size_value == 4095U);
    assert(kVdpClearRegions[5].addr == 0xE000U && kVdpClearRegions[5].size_value == 4095U);
}

void test_armlet_glow_ramp_pulse_decay() {
    // Active ramp 0 -> 1 -> 2 -> 3
    std::uint8_t glow = 0U;
    glow = step_armlet_glow(glow, true, 0U, 0U);
    assert(glow == 1U);
    glow = step_armlet_glow(glow, true, 0U, 0U);
    assert(glow == 2U);
    glow = step_armlet_glow(glow, true, 0U, 0U);
    assert(glow == 3U);

    // Below threshold: stays at 3
    assert(step_armlet_glow(3U, true, 19U, 0U) == 3U);

    // At/above 0x14 (20): pulses 4 on even pulse_ff199f, 3 on odd
    assert(step_armlet_glow(3U, true, 20U, 0U) == 4U);
    assert(step_armlet_glow(4U, true, 20U, 1U) == 3U);

    // Inactive decay: >=4 drops to 2, then 1 -> 0 -> 0
    assert(step_armlet_glow(4U, false, 20U, 0U) == 2U);
    assert(step_armlet_glow(3U, false, 0U, 0U) == 2U);
    assert(step_armlet_glow(2U, false, 0U, 0U) == 1U);
    assert(step_armlet_glow(1U, false, 0U, 0U) == 0U);
    assert(step_armlet_glow(0U, false, 0U, 0U) == 0U);
}

void test_palette_cycler() {
    std::array<std::uint16_t, 64> cram{};
    PaletteCycler cycler;

    PaletteCycleEntry entry;
    entry.cram_offset = 0x1EU; // index 15
    entry.keyframes = {
        {2U, 0x0246U},
        {3U, 0x0ACEU},
        {0U, 0x0000U}, // sentinel duration == 0 wraps back to index 0
    };
    entry.cursor = 0U;
    entry.timer = 2U;
    cycler.entries.push_back(entry);

    // Frame 1: timer 2 -> 1, no CRAM update yet
    assert(!cycler.step_frame(cram));
    assert(cycler.entries[0].timer == 1U);

    // Frame 2: timer == 1 -> advance to keyframe 1 (0x0ACE, duration 3)
    assert(cycler.step_frame(cram));
    assert(cycler.entries[0].cursor == 1U);
    assert(cycler.entries[0].timer == 3U);
    assert(cram[15] == 0x0ACEU);

    // Frames 3..4: timer 3 -> 2 -> 1
    assert(!cycler.step_frame(cram));
    assert(!cycler.step_frame(cram));
    assert(cycler.entries[0].timer == 1U);

    // Frame 5: timer == 1 -> hits sentinel duration 0, wraps to keyframe 0 (0x0246, duration 2)
    assert(cycler.step_frame(cram));
    assert(cycler.entries[0].cursor == 0U);
    assert(cycler.entries[0].timer == 2U);
    assert(cram[15] == 0x0246U);

    // Disabled cycler does nothing
    cycler.enabled = false;
    assert(!cycler.step_frame(cram));
}

} // namespace

int main() {
    test_vdp_clear_regions();
    test_armlet_glow_ramp_pulse_decay();
    test_palette_cycler();
    return 0;
}
