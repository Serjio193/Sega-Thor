#pragma once

#include "game/video/vint_dma_and_input_ring.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace oasis::game::input {

struct MashTripleTapConfig {
    static constexpr std::uint32_t kDetectRapidMashAddr = 0x00008910U;
    static constexpr std::uint32_t kDetectTripleTapAddr = 0x00008950U;
    static constexpr std::uint32_t kInputRingRam = 0x00FF1668U;
    static constexpr std::uint32_t kRingHeadRam = 0x00FF16E8U;
    static constexpr std::uint32_t kRingCountRam = 0x00FF16EAU;
    static constexpr std::uint8_t kRingByteMask = 0x7EU;

    static constexpr std::size_t kMashWindowFrames = 13U; // dbra #$0C (13 iterations)
    static constexpr std::uint16_t kMashMinPresses = 3U;   // cmpi.w #$3, d1
    static constexpr std::uint8_t kMashButtonBit = 4U;     // btst.b #4 (mask 0x0010)

    static constexpr std::size_t kTapMaxWindowFrames = 25U; // dbra #$18 (25 iterations max)
    static constexpr std::uint16_t kTapRequiredMatches = 3U; // move.w #$3, d3

    // Direction nibbles for cardinal facings (from 0x0087F4)
    // Facing 0: Up (0x01)
    // Facing 1: Right (0x08)
    // Facing 2: Down (0x02)
    // Facing 3: Left (0x04)
    static constexpr std::array<std::uint8_t, 4> kFacingTargetDir = {
        0x01U, // 0: Up
        0x08U, // 1: Right
        0x02U, // 2: Down
        0x04U, // 3: Left
    };
};

[[nodiscard]] constexpr std::uint8_t target_dir_for_facing(std::uint8_t facing) noexcept {
    return MashTripleTapConfig::kFacingTargetDir[facing & 3U];
}

// ROM 0x008910: Scans up to 13 recent words; returns true if at least 3 words have bit 4 set.
[[nodiscard]] bool detect_rapid_mash(std::span<const std::uint16_t> recent_words) noexcept;

// ROM 0x008910: Scans up to 13 frames backwards in `ring`. If at least 3 frames have button bit 4 set,
// clears `ring.count()` ($FF16EA = 0) and returns true; otherwise returns false without modifying ring.
[[nodiscard]] bool detect_rapid_mash(video::InputHistoryRing64& ring) noexcept;

// ROM 0x008950: Scans up to 25 recent words; returns true if target direction appears 3 times,
// ignoring neutral frames (0), aborting on any other direction input.
[[nodiscard]] bool detect_triple_tap(std::span<const std::uint16_t> recent_words, std::uint8_t target_dir_nibble) noexcept;

// ROM 0x008950: Convenience overload taking raw D-pad bytes/nibbles directly.
[[nodiscard]] bool detect_triple_tap_bytes(std::span<const std::uint8_t> recent_dpad_bytes, std::uint8_t target_dir_nibble) noexcept;

// ROM 0x008950: Scans up to 25 frames backwards in `ring` (clamped to `ring.count()`).
// Looks for target direction matching `facing` (0: Up, 1: Right, 2: Down, 3: Left) 3 times,
// ignoring neutral 0, aborting if any other direction is pressed.
// On match: clears `ring.count()` ($FF16EA = 0) and returns true; otherwise returns false.
[[nodiscard]] bool detect_triple_tap(video::InputHistoryRing64& ring, std::uint8_t facing) noexcept;

} // namespace oasis::game::input
