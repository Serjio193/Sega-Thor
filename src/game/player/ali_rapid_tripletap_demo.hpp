#pragma once

#include "game/video/vint_dma_and_input_ring.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace oasis::game::player {

struct RapidMashTripleTapDemoConfig {
    static constexpr std::uint32_t kRapidMashAddr = 0x008910U;
    static constexpr std::uint32_t kRapidMashEndAddr = 0x00894EU;
    static constexpr std::uint32_t kTripleTapAddr = 0x008950U;
    static constexpr std::uint32_t kTripleTapEndAddr = 0x0089B0U;
    static constexpr std::uint32_t kDemoInitializerAddr = 0x0089B2U;
    static constexpr std::uint32_t kDemoInitializerEndAddr = 0x008A92U;
    static constexpr std::uint32_t kDemoAssetOffsetsTableAddr = 0x008A94U;
    static constexpr std::uint32_t kDemoAssetOffsetsTableEndAddr = 0x008AA6U;

    static constexpr std::uint32_t kDemoBankBaseRomAddr = 0x001698FEU;
    static constexpr std::uint32_t kDecompressRoutineAddr = 0x00003820U;
    static constexpr std::uint32_t kDecompressBufferRam = 0x00FFBFACU;
    static constexpr std::uint32_t kDeltaBufferRam = 0x00FFDFB2U;
    static constexpr std::uint32_t kGameplayCoordinatorAddr = 0x00CA24U;

    static constexpr std::uint32_t kInputRingRam = 0x00FF1668U;
    static constexpr std::uint32_t kRingHeadRam = 0x00FF16E8U;
    static constexpr std::uint32_t kRingCountRam = 0x00FF16EAU;
    static constexpr std::uint32_t kDemoIndexRam = 0x00FF0BFCU;
    static constexpr std::uint32_t kDemoFlagRam = 0x00FF1996U;
    static constexpr std::uint32_t kDemoReadyRam = 0x00FF1997U;
    static constexpr std::uint32_t kDemoStreamPtrRam = 0x00FF1998U;
    static constexpr std::uint32_t kDemoTimerRam = 0x00FF1928U;

    static constexpr std::size_t kMashWindowFrames = 13U; // 13 iterations (dbra 12)
    static constexpr std::uint16_t kMashRequiredPresses = 3U; // cmpi.w #3, d1
    static constexpr std::uint8_t kMashButtonBit = 4U; // Button A

    static constexpr std::size_t kTripleTapMaxFrames = 25U; // cmpi.w #$18, d7 (25 frames)
    static constexpr std::uint16_t kTripleTapRequiredMatches = 3U; // move.w #3, d3

    static constexpr std::size_t kDemoEntryCount = 9U;
    static constexpr std::uint8_t kDemoMaxIndex = 8U;
    static constexpr std::uint16_t kDemoDeltaCountWords = 0x1FFEU; // 8190
    static constexpr std::uint16_t kDemoWaitTimerDefault = 0x7F28U;

    // Cardinal D-pad targets for facings 0..3 from 0x0087F4
    static constexpr std::array<std::uint8_t, 4> kFacingTargetDir = {
        0x01U, // 0: Up
        0x08U, // 1: Right
        0x02U, // 2: Down
        0x04U, // 3: Left
    };

    // 9 word offsets at 0x008A94
    static constexpr std::array<std::uint16_t, kDemoEntryCount> kDemoAssetOffsets = {
        0x0000U, 0x042CU, 0x0BC0U, 0x1202U, 0x144CU,
        0x16C8U, 0x18E0U, 0x1EE8U, 0x201EU
    };

    // 9 absolute ROM addresses relative to 0x1698FE
    static constexpr std::array<std::uint32_t, kDemoEntryCount> kDemoAbsoluteAddrs = {
        0x001698FEU, 0x00169D2AU, 0x0016A4BEU, 0x0016AB00U, 0x0016AD4AU,
        0x0016AFC6U, 0x0016B1DEU, 0x0016B7E6U, 0x0016B91CU
    };
};

// Target D-pad direction for Ali facing (0: Up, 1: Right, 2: Down, 3: Left).
[[nodiscard]] constexpr std::uint8_t target_dir_for_facing(std::uint8_t facing) noexcept {
    return RapidMashTripleTapDemoConfig::kFacingTargetDir[facing & 3U];
}

// 1. Rapid-Mash Detector (0x008910..0x00894E)
[[nodiscard]] bool recognize_rapid_mash_direct(std::span<const std::uint16_t> recent_words) noexcept;
[[nodiscard]] bool detect_rapid_mash_motion(video::InputHistoryRing64& ring) noexcept;

// 2. Triple-Tap Motion Detector (0x008950..0x0089B0)
[[nodiscard]] bool recognize_triple_tap_direct(
    std::span<const std::uint16_t> recent_words,
    std::uint8_t facing) noexcept;

[[nodiscard]] bool recognize_triple_tap_dpad(
    std::span<const std::uint8_t> recent_dpad_bytes,
    std::uint8_t facing) noexcept;

[[nodiscard]] bool detect_triple_tap_motion(
    video::InputHistoryRing64& ring,
    std::uint8_t facing) noexcept;

// 3. Demo Stream Asset Table & Initializer (0x0089B2..0x008AA6)
[[nodiscard]] std::optional<std::uint16_t> lookup_demo_asset_offset(std::uint8_t index) noexcept;
[[nodiscard]] std::optional<std::uint32_t> lookup_demo_stream_rom_addr(std::uint8_t index) noexcept;

// Cycles demo index (0..8) -> (idx + 1) <= 8 ? (idx + 1) : 0 matching 0x008A0A..0x008A14
[[nodiscard]] constexpr std::uint8_t cycle_demo_index(std::uint8_t current_index) noexcept {
    const std::uint8_t next = static_cast<std::uint8_t>(current_index + 1U);
    return (next <= RapidMashTripleTapDemoConfig::kDemoMaxIndex) ? next : 0U;
}

// Prefix sum mod 256 in-place delta reconstruction matching 0x008A36..0x008A46
void delta_reconstruct_demo_stream(std::span<std::uint8_t> buffer) noexcept;

struct DemoPlaybackState {
    std::uint8_t demo_index{0U};          // $FF0BFC
    bool demo_active{false};              // $FF1996
    bool demo_ready{false};               // $FF1997
    std::uint32_t current_stream_rom_addr{0U};
    std::uint16_t demo_wait_timer{0U};    // $FF1928

    void reset() noexcept;
    void advance_cycle() noexcept;
};

} // namespace oasis::game::player
