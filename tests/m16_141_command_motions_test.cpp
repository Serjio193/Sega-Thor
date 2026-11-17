#include "game/player/ali_command_motions.hpp"

#include <cassert>
#include <iostream>
#include <vector>

using namespace oasis::game::player;
using oasis::game::video::InputHistoryRing64;

static void test_rom_constants() {
    static_assert(CommandMotionConfig::kSpin360RecognizerAddr == 0x0086B4U);
    static_assert(CommandMotionConfig::kSpin360LutAddr == 0x008768U);
    static_assert(CommandMotionConfig::kFbfRecognizerAddr == 0x008778U);
    static_assert(CommandMotionConfig::kFbfPatternTableAddr == 0x0087F4U);

    static_assert(CommandMotionConfig::kInputRingRam == 0x00FF1668U);
    static_assert(CommandMotionConfig::kRingHeadRam == 0x00FF16E8U);
    static_assert(CommandMotionConfig::kRingCountRam == 0x00FF16EAU);
    static_assert(CommandMotionConfig::kRingByteMask == 0x7EU);
    static_assert(CommandMotionConfig::kRingEntryStride == 2U);
    static_assert(CommandMotionConfig::kMaxSamples == 30U);

    // Verify LUT values (from evidence JSON)
    constexpr std::array<std::int8_t, 16> expected_lut = {
        -1, 0, 4, -1, 6, 7, 5, -1, 2, 1, 3, -1, -1, -1, -1, -1
    };
    for (std::size_t i = 0; i < 16; ++i) {
        assert(CommandMotionConfig::kPadToDir8Lut[i] == expected_lut[i]);
    }

    // Verify FBF patterns for all 4 facings
    // Facing 0: Up (1 -> 2 -> 1)
    assert(kFbfPatterns[0].facing == 0U);
    assert(kFbfPatterns[0].sequence[0] == 1U && kFbfPatterns[0].sequence[1] == 2U && kFbfPatterns[0].sequence[2] == 1U);
    assert(kFbfPatterns[0].masks[0] == 15U && kFbfPatterns[0].masks[1] == 2U && kFbfPatterns[0].masks[2] == 15U);
    assert(kFbfPatterns[0].perp_mask == 0x0CU);

    // Facing 1: Right (8 -> 4 -> 8)
    assert(kFbfPatterns[1].facing == 1U);
    assert(kFbfPatterns[1].sequence[0] == 8U && kFbfPatterns[1].sequence[1] == 4U && kFbfPatterns[1].sequence[2] == 8U);
    assert(kFbfPatterns[1].masks[0] == 15U && kFbfPatterns[1].masks[1] == 4U && kFbfPatterns[1].masks[2] == 15U);
    assert(kFbfPatterns[1].perp_mask == 0x03U);

    // Facing 2: Down (2 -> 1 -> 2)
    assert(kFbfPatterns[2].facing == 2U);
    assert(kFbfPatterns[2].sequence[0] == 2U && kFbfPatterns[2].sequence[1] == 1U && kFbfPatterns[2].sequence[2] == 2U);
    assert(kFbfPatterns[2].masks[0] == 15U && kFbfPatterns[2].masks[1] == 1U && kFbfPatterns[2].masks[2] == 15U);
    assert(kFbfPatterns[2].perp_mask == 0x0CU);

    // Facing 3: Left (4 -> 8 -> 4)
    assert(kFbfPatterns[3].facing == 3U);
    assert(kFbfPatterns[3].sequence[0] == 4U && kFbfPatterns[3].sequence[1] == 8U && kFbfPatterns[3].sequence[2] == 4U);
    assert(kFbfPatterns[3].masks[0] == 15U && kFbfPatterns[3].masks[1] == 8U && kFbfPatterns[3].masks[2] == 15U);
    assert(kFbfPatterns[3].perp_mask == 0x03U);
}

static void test_spin_360_direct() {
    // Tests are ordered newest to oldest (as read from head backwards)

    // CW rotation (from head backwards): Left(4), Down(2), Right(8), Up(1)
    {
        const std::array<std::uint8_t, 4> cw_samples = {4U, 2U, 8U, 1U};
        assert(recognize_spin_360_dpad(cw_samples));
    }

    // CCW rotation (from head backwards): Right(8), Down(2), Left(4), Up(1)
    {
        const std::array<std::uint8_t, 4> ccw_samples = {8U, 2U, 4U, 1U};
        assert(recognize_spin_360_dpad(ccw_samples));
    }

    // With interleaved diagonals and neutral frames
    {
        // 4 (Left), 9 (diagonal ignored), 2 (Down), 0 (neutral ignored), 8 (Right), 5 (diagonal ignored), 1 (Up)
        const std::array<std::uint8_t, 7> noisy_samples = {4U, 9U, 2U, 0U, 8U, 5U, 1U};
        assert(recognize_spin_360_dpad(noisy_samples));
    }

    // Incomplete 3-step rotation fails
    {
        const std::array<std::uint8_t, 3> partial_samples = {4U, 2U, 8U};
        assert(!recognize_spin_360_dpad(partial_samples));
    }

    // Opposing direction jump fails: Up(1) -> Down(2)
    {
        const std::array<std::uint8_t, 4> bad_samples = {1U, 2U, 1U, 2U};
        assert(!recognize_spin_360_dpad(bad_samples));
    }

    // Empty samples fail
    {
        assert(!recognize_spin_360_dpad({}));
    }
}

static void test_fbf_direct() {
    // Samples ordered newest to oldest

    // Facing 0 (Up): sequence 1 -> 2 -> 1
    {
        const std::array<std::uint8_t, 3> fbf_up = {1U, 2U, 1U};
        assert(recognize_fbf_dpad(fbf_up, 0U));
        // Perpendicular Left (4) rejects
        const std::array<std::uint8_t, 4> fbf_up_perp_left = {1U, 4U, 2U, 1U};
        assert(!recognize_fbf_dpad(fbf_up_perp_left, 0U));
        // Perpendicular Right (8) rejects
        const std::array<std::uint8_t, 4> fbf_up_perp_right = {1U, 8U, 2U, 1U};
        assert(!recognize_fbf_dpad(fbf_up_perp_right, 0U));
    }

    // Facing 1 (Right): sequence 8 -> 4 -> 8
    {
        const std::array<std::uint8_t, 3> fbf_right = {8U, 4U, 8U};
        assert(recognize_fbf_dpad(fbf_right, 1U));
        // Perpendicular Up (1) rejects
        const std::array<std::uint8_t, 4> fbf_right_perp_up = {8U, 1U, 4U, 8U};
        assert(!recognize_fbf_dpad(fbf_right_perp_up, 1U));
        // Perpendicular Down (2) rejects
        const std::array<std::uint8_t, 4> fbf_right_perp_down = {8U, 2U, 4U, 8U};
        assert(!recognize_fbf_dpad(fbf_right_perp_down, 1U));
    }

    // Facing 2 (Down): sequence 2 -> 1 -> 2
    {
        const std::array<std::uint8_t, 3> fbf_down = {2U, 1U, 2U};
        assert(recognize_fbf_dpad(fbf_down, 2U));
        // Perpendicular Left (4) rejects
        const std::array<std::uint8_t, 4> fbf_down_perp = {2U, 4U, 1U, 2U};
        assert(!recognize_fbf_dpad(fbf_down_perp, 2U));
    }

    // Facing 3 (Left): sequence 4 -> 8 -> 4
    {
        const std::array<std::uint8_t, 3> fbf_left = {4U, 8U, 4U};
        assert(recognize_fbf_dpad(fbf_left, 3U));
        // Perpendicular Up (1) rejects
        const std::array<std::uint8_t, 4> fbf_left_perp = {4U, 1U, 8U, 4U};
        assert(!recognize_fbf_dpad(fbf_left_perp, 3U));
    }
}

static void test_ring_buffer_integration() {
    InputHistoryRing64 ring;

    // Empty ring returns false
    assert(!detect_spin_360_motion(ring));
    assert(!detect_fbf_motion(ring, 0U));

    // CW 360 Spin in chronological recording order:
    // Recorded: Up(1) -> Right(8) -> Down(2) -> Left(4)
    ring.record(1U);
    ring.record(9U); // Up-Right diagonal (ignored)
    ring.record(8U);
    ring.record(0U); // Neutral (ignored)
    ring.record(2U);
    ring.record(4U);
    assert(ring.count() == 6U);
    assert(detect_spin_360_motion(ring));
    assert(ring.count() == 0U); // Cleared on match!

    // CCW 360 Spin: Up(1) -> Left(4) -> Down(2) -> Right(8)
    ring.reset();
    ring.record(1U);
    ring.record(4U);
    ring.record(2U);
    ring.record(8U);
    assert(ring.count() == 4U);
    assert(detect_spin_360_motion(ring));
    assert(ring.count() == 0U);

    // Incomplete spin fails and preserves count
    ring.reset();
    ring.record(1U);
    ring.record(8U);
    ring.record(2U);
    assert(ring.count() == 3U);
    assert(!detect_spin_360_motion(ring));
    assert(ring.count() == 3U);

    // FBF in chronological recording order:
    // Facing 1 (Right): Right(8) -> Left(4) -> Right(8)
    ring.reset();
    ring.record(8U);
    ring.record(0U); // neutral frame
    ring.record(4U);
    ring.record(8U);
    assert(ring.count() == 4U);
    assert(detect_fbf_motion(ring, 1U));
    assert(ring.count() == 0U);

    // FBF with perpendicular input fails and preserves count
    ring.reset();
    ring.record(8U);
    ring.record(4U);
    ring.record(1U); // Up (perpendicular)
    ring.record(8U);
    assert(ring.count() == 4U);
    assert(!detect_fbf_motion(ring, 1U));
    assert(ring.count() == 4U);
}

int main() {
    std::cout << "[oasis_m16_141_command_motions_test] Starting..." << std::endl;
    test_rom_constants();
    test_spin_360_direct();
    test_fbf_direct();
    test_ring_buffer_integration();
    std::cout << "[oasis_m16_141_command_motions_test] All tests passed!" << std::endl;
    return 0;
}
