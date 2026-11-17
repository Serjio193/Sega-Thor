#include "game/player/ali_rapid_tripletap_demo.hpp"
#include "game/video/vint_dma_and_input_ring.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

using namespace oasis::game::player;
using oasis::game::video::InputHistoryRing64;

static void test_rom_constants() {
    assert(RapidMashTripleTapDemoConfig::kRapidMashAddr == 0x008910U);
    assert(RapidMashTripleTapDemoConfig::kRapidMashEndAddr == 0x00894EU);
    assert(RapidMashTripleTapDemoConfig::kTripleTapAddr == 0x008950U);
    assert(RapidMashTripleTapDemoConfig::kTripleTapEndAddr == 0x0089B0U);
    assert(RapidMashTripleTapDemoConfig::kDemoInitializerAddr == 0x0089B2U);
    assert(RapidMashTripleTapDemoConfig::kDemoInitializerEndAddr == 0x008A92U);
    assert(RapidMashTripleTapDemoConfig::kDemoAssetOffsetsTableAddr == 0x008A94U);
    assert(RapidMashTripleTapDemoConfig::kDemoAssetOffsetsTableEndAddr == 0x008AA6U);

    assert(RapidMashTripleTapDemoConfig::kDemoBankBaseRomAddr == 0x001698FEU);
    assert(RapidMashTripleTapDemoConfig::kDecompressRoutineAddr == 0x00003820U);
    assert(RapidMashTripleTapDemoConfig::kDecompressBufferRam == 0x00FFBFACU);
    assert(RapidMashTripleTapDemoConfig::kDeltaBufferRam == 0x00FFDFB2U);
    assert(RapidMashTripleTapDemoConfig::kGameplayCoordinatorAddr == 0x00CA24U);

    assert(RapidMashTripleTapDemoConfig::kMashWindowFrames == 13U);
    assert(RapidMashTripleTapDemoConfig::kMashRequiredPresses == 3U);
    assert(RapidMashTripleTapDemoConfig::kMashButtonBit == 4U);

    assert(RapidMashTripleTapDemoConfig::kTripleTapMaxFrames == 25U);
    assert(RapidMashTripleTapDemoConfig::kTripleTapRequiredMatches == 3U);

    assert(RapidMashTripleTapDemoConfig::kDemoEntryCount == 9U);
    assert(RapidMashTripleTapDemoConfig::kDemoMaxIndex == 8U);
    assert(RapidMashTripleTapDemoConfig::kDemoDeltaCountWords == 0x1FFEU);
    assert(RapidMashTripleTapDemoConfig::kDemoWaitTimerDefault == 0x7F28U);

    assert(target_dir_for_facing(0) == 0x01U); // Up
    assert(target_dir_for_facing(1) == 0x08U); // Right
    assert(target_dir_for_facing(2) == 0x02U); // Down
    assert(target_dir_for_facing(3) == 0x04U); // Left
}

static void test_rapid_mash_direct() {
    // Empty span
    assert(!recognize_rapid_mash_direct(std::span<const std::uint16_t>{}));

    // 2 presses in 13 frames -> false
    std::vector<std::uint16_t> seq1 = {0x0010U, 0x0000U, 0x0010U, 0x0000U};
    assert(!recognize_rapid_mash_direct(seq1));

    // 3 presses in 13 frames -> true
    std::vector<std::uint16_t> seq2 = {0x0010U, 0x0000U, 0x0010U, 0x0000U, 0x0010U};
    assert(recognize_rapid_mash_direct(seq2));

    // Boundary check: indices 0, 6, 12 in 13-frame window -> true
    std::vector<std::uint16_t> seq3(13, 0x0000U);
    seq3[0] = 0x0010U;
    seq3[6] = 0x0010U;
    seq3[12] = 0x0010U;
    assert(recognize_rapid_mash_direct(seq3));

    // Boundary check: index 13 is beyond 13-frame window (0..12) -> false
    std::vector<std::uint16_t> seq4(14, 0x0000U);
    seq4[0] = 0x0010U;
    seq4[1] = 0x0010U;
    seq4[13] = 0x0010U;
    assert(!recognize_rapid_mash_direct(seq4));

    // Upper-byte format (from Python test) 0x1000
    std::vector<std::uint16_t> seq5 = {0x1000U, 0x1000U, 0x1000U};
    assert(recognize_rapid_mash_direct(seq5));
}

static void test_rapid_mash_ring() {
    InputHistoryRing64 ring;

    // Fail case: 2 presses in 5 frames
    ring.record(0x0010U);
    ring.record(0x0000U);
    ring.record(0x0010U);
    ring.record(0x0000U);
    ring.record(0x0000U);
    assert(ring.count() == 5U);
    assert(!detect_rapid_mash_motion(ring));
    assert(ring.count() == 5U); // count retained on failure

    // Add 3rd press -> success
    ring.record(0x0010U);
    assert(ring.count() == 6U);
    assert(detect_rapid_mash_motion(ring));
    assert(ring.count() == 0U); // count cleared on success
}

static void test_triple_tap_direct() {
    // 4 cardinal directions
    for (std::uint8_t facing = 0U; facing < 4U; ++facing) {
        const std::uint8_t dir = target_dir_for_facing(facing);
        std::vector<std::uint8_t> dpad_seq = {dir, 0x00U, dir, 0x00U, dir};
        assert(recognize_triple_tap_dpad(dpad_seq, facing));

        std::vector<std::uint16_t> word_seq = {dir, 0x0000U, dir, 0x0000U, dir};
        assert(recognize_triple_tap_direct(word_seq, facing));
    }

    // Rejection on non-matching non-zero D-pad direction
    // Facing 1 (Right: 0x08), encountered Left (0x04)
    std::vector<std::uint8_t> fail_seq = {0x08U, 0x04U, 0x08U, 0x08U};
    assert(!recognize_triple_tap_dpad(fail_seq, 1U));

    // 25-frame max window limit
    std::vector<std::uint8_t> max_win(25, 0x00U);
    max_win[0] = 0x04U;  // Facing 3 Left
    max_win[12] = 0x04U;
    max_win[24] = 0x04U; // 25th frame (index 24)
    assert(recognize_triple_tap_dpad(max_win, 3U));

    // 26th frame is beyond limit
    std::vector<std::uint8_t> over_win(26, 0x00U);
    over_win[0] = 0x04U;
    over_win[1] = 0x04U;
    over_win[25] = 0x04U; // index 25 (26th frame)
    assert(!recognize_triple_tap_dpad(over_win, 3U));
}

static void test_triple_tap_ring() {
    InputHistoryRing64 ring;

    // Empty ring
    assert(!detect_triple_tap_motion(ring, 0U));

    // Facing 2 (Down: 0x02)
    ring.record(0x0002U);
    ring.record(0x0000U);
    ring.record(0x0002U);
    assert(ring.count() == 3U);
    assert(!detect_triple_tap_motion(ring, 2U));
    assert(ring.count() == 3U);

    // 3rd match -> success
    ring.record(0x0002U);
    assert(ring.count() == 4U);
    assert(detect_triple_tap_motion(ring, 2U));
    assert(ring.count() == 0U);

    // Mismatched direction test in ring
    ring.record(0x0002U);
    ring.record(0x0001U); // Up (mismatch for facing Down)
    ring.record(0x0002U);
    ring.record(0x0002U);
    assert(!detect_triple_tap_motion(ring, 2U));
    assert(ring.count() == 4U);
}

static void test_demo_stream_table() {
    constexpr std::array<std::uint16_t, 9> kExpectedOffsets = {
        0x0000U, 0x042CU, 0x0BC0U, 0x1202U, 0x144CU,
        0x16C8U, 0x18E0U, 0x1EE8U, 0x201EU
    };
    constexpr std::array<std::uint32_t, 9> kExpectedAddrs = {
        0x001698FEU, 0x00169D2AU, 0x0016A4BEU, 0x0016AB00U, 0x0016AD4AU,
        0x0016AFC6U, 0x0016B1DEU, 0x0016B7E6U, 0x0016B91CU
    };

    for (std::uint8_t i = 0U; i < 9U; ++i) {
        const auto off = lookup_demo_asset_offset(i);
        assert(off.has_value());
        assert(*off == kExpectedOffsets[i]);

        const auto addr = lookup_demo_stream_rom_addr(i);
        assert(addr.has_value());
        assert(*addr == kExpectedAddrs[i]);
        assert(*addr == RapidMashTripleTapDemoConfig::kDemoBankBaseRomAddr + kExpectedOffsets[i]);
    }

    assert(!lookup_demo_asset_offset(9U).has_value());
    assert(!lookup_demo_stream_rom_addr(9U).has_value());
}

static void test_demo_cycle_and_state() {
    // Cycle 0..8
    for (std::uint8_t i = 0U; i < 8U; ++i) {
        assert(cycle_demo_index(i) == static_cast<std::uint8_t>(i + 1U));
    }
    assert(cycle_demo_index(8U) == 0U);
    assert(cycle_demo_index(9U) == 0U);
    assert(cycle_demo_index(255U) == 0U);

    // DemoPlaybackState
    DemoPlaybackState state;
    state.reset();
    assert(state.demo_index == 0U);
    assert(!state.demo_active);
    assert(!state.demo_ready);
    assert(state.current_stream_rom_addr == 0x001698FEU);
    assert(state.demo_wait_timer == 0U);

    state.advance_cycle();
    assert(state.demo_index == 1U);
    assert(state.demo_active);
    assert(state.demo_ready);
    assert(state.current_stream_rom_addr == 0x00169D2AU);
    assert(state.demo_wait_timer == 0x7F28U);

    // Advance 8 more times to wrap back to 0
    for (int i = 0; i < 8; ++i) {
        state.advance_cycle();
    }
    assert(state.demo_index == 0U);
    assert(state.current_stream_rom_addr == 0x001698FEU);
}

static void test_delta_reconstruction() {
    std::vector<std::uint8_t> buffer = {0x10U, 0x05U, 0x02U, 0xFDU};
    delta_reconstruct_demo_stream(buffer);
    // 0: 0x10 (16)
    // 1: 0x10 + 0x05 = 0x15 (21)
    // 2: 0x15 + 0x02 = 0x17 (23)
    // 3: 0x17 + 0xFD (-3 mod 256) = 0x14 (20)
    assert(buffer[0] == 0x10U);
    assert(buffer[1] == 0x15U);
    assert(buffer[2] == 0x17U);
    assert(buffer[3] == 0x14U);

    // Empty and single-element buffers
    std::vector<std::uint8_t> empty;
    delta_reconstruct_demo_stream(empty);

    std::vector<std::uint8_t> single = {0x42U};
    delta_reconstruct_demo_stream(single);
    assert(single[0] == 0x42U);
}

int main() {
    std::cout << "[oasis_m16_143_ali_rapid_tripletap_demo_test] running tests..." << std::endl;
    test_rom_constants();
    test_rapid_mash_direct();
    test_rapid_mash_ring();
    test_triple_tap_direct();
    test_triple_tap_ring();
    test_demo_stream_table();
    test_demo_cycle_and_state();
    test_delta_reconstruction();
    std::cout << "[oasis_m16_143_ali_rapid_tripletap_demo_test] all tests passed!" << std::endl;
    return 0;
}
