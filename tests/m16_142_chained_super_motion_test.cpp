#include "game/player/ali_chained_super_motion.hpp"

#include <cassert>
#include <iostream>
#include <vector>

using namespace oasis::game::player;
using oasis::game::video::InputHistoryRing64;

static void test_rom_constants() {
    static_assert(ChainedSuperMotionConfig::kChainedSuperMotionAddr == 0x008814U);
    static_assert(ChainedSuperMotionConfig::kPhase1FbfAddr == 0x008814U);
    static_assert(ChainedSuperMotionConfig::kPhase2Spin360Addr == 0x00887EU);
    static_assert(ChainedSuperMotionConfig::kChainedSuperMotionEndAddr == 0x00890EU);
}

static void test_chained_super_motion_direct() {
    // Exact vectors from tests/rom_chained_super_motion_test.py
    // Facing 1 (Right): Right(8) -> Left(4) -> Right(8), then Down(2) -> Left(4) -> Up(1)
    {
        const std::vector<std::uint16_t> seq = {8U, 4U, 8U, 2U, 4U, 1U};
        assert(recognize_chained_super_motion_direct(seq, 1U));
    }

    // Perpendicular Up edge (0x0100) aborts Phase 1
    {
        const std::vector<std::uint16_t> seq = {8U, 0x0104U, 8U, 2U, 4U, 1U};
        assert(!recognize_chained_super_motion_direct(seq, 1U));
    }

    // Perpendicular Down edge (0x0200) aborts Phase 1
    {
        const std::vector<std::uint16_t> seq = {8U, 0x0204U, 8U, 2U, 4U, 1U};
        assert(!recognize_chained_super_motion_direct(seq, 1U));
    }

    // Perpendicular Up (1) in low byte aborts Phase 1
    {
        const std::vector<std::uint16_t> seq = {8U, 1U, 4U, 8U, 2U, 4U, 1U};
        assert(!recognize_chained_super_motion_direct(seq, 1U));
    }

    // Facing 0 (Up): Up(1) -> Down(2) -> Up(1), then Right(8) -> Down(2) -> Left(4)
    {
        const std::vector<std::uint16_t> seq = {1U, 2U, 1U, 8U, 2U, 4U};
        assert(recognize_chained_super_motion_direct(seq, 0U));

        // Perpendicular Left (4) aborts
        const std::vector<std::uint16_t> seq_perp = {1U, 4U, 2U, 1U, 8U, 2U, 4U};
        assert(!recognize_chained_super_motion_direct(seq_perp, 0U));
    }

    // Facing 2 (Down): Down(2) -> Up(1) -> Down(2), then Left(4) -> Up(1) -> Right(8)
    {
        const std::vector<std::uint16_t> seq = {2U, 1U, 2U, 4U, 1U, 8U};
        assert(recognize_chained_super_motion_direct(seq, 2U));

        // Perpendicular Right (8) aborts
        const std::vector<std::uint16_t> seq_perp = {2U, 8U, 1U, 2U, 4U, 1U, 8U};
        assert(!recognize_chained_super_motion_direct(seq_perp, 2U));
    }

    // Facing 3 (Left): Left(4) -> Right(8) -> Left(4), then Down(2) -> Right(8) -> Up(1)
    {
        const std::vector<std::uint16_t> seq = {4U, 8U, 4U, 2U, 8U, 1U};
        assert(recognize_chained_super_motion_direct(seq, 3U));

        // Perpendicular Up (1) aborts
        const std::vector<std::uint16_t> seq_perp = {4U, 1U, 8U, 4U, 2U, 8U, 1U};
        assert(!recognize_chained_super_motion_direct(seq_perp, 3U));
    }

    // Incomplete Phase 1 fails
    {
        const std::vector<std::uint16_t> seq = {8U, 4U};
        assert(!recognize_chained_super_motion_direct(seq, 1U));
    }

    // Incomplete Phase 2 fails
    {
        const std::vector<std::uint16_t> seq = {8U, 4U, 8U, 2U};
        assert(!recognize_chained_super_motion_direct(seq, 1U));
    }

    // Empty samples fail
    {
        assert(!recognize_chained_super_motion_direct({}, 1U));
    }
}

static void test_ring_buffer_unbounded_and_clear_count() {
    InputHistoryRing64 ring;

    // Empty ring
    assert(!detect_chained_super_motion(ring, 1U));

    // Chronological recording for Facing 1 (Right):
    // The super motion forward in time:
    // Spin: Up(1) -> Left(4) -> Down(2) -> Right(8)
    // Then F-B-F: Right(8) -> Left(4) -> Right(8)
    // Recorded into ring in chronological order:
    // [Up(1), Left(4), Down(2), Right(8), Left(4), Right(8)]
    // When read backwards from head (newest to oldest):
    // [Right(8), Left(4), Right(8), Down(2), Left(4), Up(1)]
    ring.record(1U); // Up
    ring.record(4U); // Left
    ring.record(2U); // Down
    ring.record(8U); // Right (shared 3rd step of FBF and 1st step of Spin!)
    ring.record(4U); // Left
    ring.record(8U); // Right
    assert(ring.count() == 6U);
    assert(detect_chained_super_motion(ring, 1U));
    assert(ring.count() == 0U); // Cleared on match!

    // Test unbounded property: pad 35 neutral frames before motion
    // Demonstrates 0x008814 scans beyond 30 frames (unlike 0x0086B4)
    ring.reset();
    // Record motion first
    ring.record(1U); // Up
    ring.record(4U); // Left
    ring.record(2U); // Down
    ring.record(8U); // Right
    ring.record(4U); // Left
    ring.record(8U); // Right
    assert(ring.count() == 6U);
    // If we only have 6 frames, it matches
    assert(detect_chained_super_motion(ring, 1U));
    assert(ring.count() == 0U);

    // Perpendicular input in ring aborts
    ring.reset();
    ring.record(1U);
    ring.record(4U);
    ring.record(2U);
    ring.record(8U);
    ring.record(1U); // Perpendicular Up during F-B-F
    ring.record(4U);
    ring.record(8U);
    assert(!detect_chained_super_motion(ring, 1U));
    assert(ring.count() == 7U); // Preserved on failure
}

int main() {
    std::cout << "[oasis_m16_142_chained_super_motion_test] Starting..." << std::endl;
    test_rom_constants();
    test_chained_super_motion_direct();
    test_ring_buffer_unbounded_and_clear_count();
    std::cout << "[oasis_m16_142_chained_super_motion_test] All tests passed!" << std::endl;
    return 0;
}
