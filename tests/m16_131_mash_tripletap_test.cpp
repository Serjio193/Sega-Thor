#include "game/input/mash_tripletap_detectors.hpp"
#include "game/video/vint_dma_and_input_ring.hpp"

#include <array>
#include <cassert>
#include <iostream>
#include <vector>

using namespace oasis::game::input;
using namespace oasis::game::video;

static void test_constants_and_rom_addresses() {
    assert(MashTripleTapConfig::kDetectRapidMashAddr == 0x00008910U);
    assert(MashTripleTapConfig::kDetectTripleTapAddr == 0x00008950U);
    assert(MashTripleTapConfig::kInputRingRam == 0x00FF1668U);
    assert(MashTripleTapConfig::kRingHeadRam == 0x00FF16E8U);
    assert(MashTripleTapConfig::kRingCountRam == 0x00FF16EAU);
    assert(MashTripleTapConfig::kRingByteMask == 0x7EU);

    assert(MashTripleTapConfig::kMashWindowFrames == 13U);
    assert(MashTripleTapConfig::kMashMinPresses == 3U);
    assert(MashTripleTapConfig::kMashButtonBit == 4U);

    assert(MashTripleTapConfig::kTapMaxWindowFrames == 25U);
    assert(MashTripleTapConfig::kTapRequiredMatches == 3U);
}

static void test_facing_direction_mapping() {
    assert(target_dir_for_facing(0) == 0x01U); // Up
    assert(target_dir_for_facing(1) == 0x08U); // Right
    assert(target_dir_for_facing(2) == 0x02U); // Down
    assert(target_dir_for_facing(3) == 0x04U); // Left

    // Modulo 4 wrapping
    assert(target_dir_for_facing(4) == 0x01U);
    assert(target_dir_for_facing(7) == 0x04U);
}

static void test_rapid_mash_detection_span() {
    // Empty span
    assert(!detect_rapid_mash(std::span<const std::uint16_t>{}));

    // Exactly 2 hits in 13 frames -> false
    std::vector<std::uint16_t> seq1 = {0x0010U, 0x0000U, 0x0010U, 0x0000U};
    assert(!detect_rapid_mash(seq1));

    // Exactly 3 hits in 13 frames -> true
    std::vector<std::uint16_t> seq2 = {0x0010U, 0x0000U, 0x0010U, 0x0000U, 0x0010U};
    assert(detect_rapid_mash(seq2));

    // Hits at boundary (indices 0, 6, 12) -> true (within 13 frames: 0..12)
    std::vector<std::uint16_t> seq3(13, 0x0000U);
    seq3[0] = 0x0010U;
    seq3[6] = 0x0010U;
    seq3[12] = 0x0010U;
    assert(detect_rapid_mash(seq3));

    // Hits outside 13-frame window (indices 0, 1, 13) -> false (index 13 ignored)
    std::vector<std::uint16_t> seq4(14, 0x0000U);
    seq4[0] = 0x0010U;
    seq4[1] = 0x0010U;
    seq4[13] = 0x0010U;
    assert(!detect_rapid_mash(seq4));

    // Non-bit-4 inputs do not count
    std::vector<std::uint16_t> seq5 = {0x000FU, 0xFFE0U, 0x0020U, 0x0040U};
    assert(!detect_rapid_mash(seq5));
}

static void test_triple_tap_detection_bytes_and_words() {
    // Matches Python reference tests
    const std::vector<std::uint8_t> tap_pass = {0x08U, 0x00U, 0x08U, 0x08U};
    assert(detect_triple_tap_bytes(tap_pass, 0x08U));

    const std::vector<std::uint8_t> tap_fail = {0x08U, 0x04U, 0x08U, 0x08U};
    assert(!detect_triple_tap_bytes(tap_fail, 0x08U));

    // Test all cardinal directions
    const std::vector<std::uint8_t> dir_up = {0x01U, 0x01U, 0x01U};
    const std::vector<std::uint8_t> dir_down = {0x02U, 0x02U, 0x02U};
    const std::vector<std::uint8_t> dir_left = {0x04U, 0x04U, 0x04U};
    const std::vector<std::uint8_t> dir_right = {0x08U, 0x08U, 0x08U};
    assert(detect_triple_tap_bytes(dir_up, 0x01U));       // Up
    assert(detect_triple_tap_bytes(dir_down, 0x02U));     // Down
    assert(detect_triple_tap_bytes(dir_left, 0x04U));     // Left
    assert(detect_triple_tap_bytes(dir_right, 0x08U));    // Right

    // Words overload testing D-pad nibble masking (& 0x0F)
    const std::vector<std::uint16_t> words_pass = {0x0018U, 0x0000U, 0x0038U, 0x0008U};
    assert(detect_triple_tap(words_pass, 0x08U));

    const std::vector<std::uint16_t> words_mismatch = {0x0018U, 0x0002U, 0x0038U, 0x0008U};
    assert(!detect_triple_tap(words_mismatch, 0x08U));

    // 25-frame max window: 3rd hit at index 24 (25th frame) -> true
    std::vector<std::uint8_t> long_pass(25, 0x00U);
    long_pass[0] = 0x04U;
    long_pass[10] = 0x04U;
    long_pass[24] = 0x04U;
    assert(detect_triple_tap_bytes(long_pass, 0x04U));

    // 3rd hit at index 25 (26th frame) -> false (outside 25-frame window)
    std::vector<std::uint8_t> long_fail(26, 0x00U);
    long_fail[0] = 0x04U;
    long_fail[10] = 0x04U;
    long_fail[25] = 0x04U;
    assert(!detect_triple_tap_bytes(long_fail, 0x04U));
}

static void test_rapid_mash_input_ring_integration() {
    InputHistoryRing64 ring;
    ring.reset();

    // Empty ring
    assert(!detect_rapid_mash(ring));

    // Record 2 presses (0x0010) and 2 neutral (0x0000)
    ring.record(0x0010U);
    ring.record(0x0000U);
    ring.record(0x0010U);
    ring.record(0x0000U);
    assert(ring.count() == 4U);

    // Only 2 hits -> false, count preserved
    assert(!detect_rapid_mash(ring));
    assert(ring.count() == 4U);

    // Record 3rd press
    ring.record(0x0010U);
    assert(ring.count() == 5U);

    // 3 hits -> true, count cleared to 0
    assert(detect_rapid_mash(ring));
    assert(ring.count() == 0U);
}

static void test_triple_tap_input_ring_integration() {
    InputHistoryRing64 ring;
    ring.reset();

    // Facing 1 is Right (0x08)
    // Record Right, Neutral, Right
    ring.record(0x0008U);
    ring.record(0x0000U);
    ring.record(0x0008U);
    assert(ring.count() == 3U);

    // Only 2 matches -> false, count preserved
    assert(!detect_triple_tap(ring, 1U));
    assert(ring.count() == 3U);

    // Record wrong direction (Left = 0x0004)
    ring.record(0x0004U);
    assert(ring.count() == 4U);

    // Aborts due to mismatch -> false, count preserved
    assert(!detect_triple_tap(ring, 1U));
    assert(ring.count() == 4U);

    // Reset and build a valid 3-tap sequence for Facing 2 (Down = 0x02)
    ring.reset();
    ring.record(0x0002U);
    ring.record(0x0000U);
    ring.record(0x0002U);
    ring.record(0x0000U);
    ring.record(0x0002U);
    assert(ring.count() == 5U);

    // Detect for Facing 2 -> true, count cleared to 0
    assert(detect_triple_tap(ring, 2U));
    assert(ring.count() == 0U);
}

int main() {
    test_constants_and_rom_addresses();
    test_facing_direction_mapping();
    test_rapid_mash_detection_span();
    test_triple_tap_detection_bytes_and_words();
    test_rapid_mash_input_ring_integration();
    test_triple_tap_input_ring_integration();

    std::cout << "oasis_m16_131_mash_tripletap_test: ALL TESTS PASSED\n";
    return 0;
}
