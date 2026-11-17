#pragma once

#include "game/video/vint_dma_and_input_ring.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace oasis::game::player {

struct CommandMotionConfig {
    static constexpr std::uint32_t kSpin360RecognizerAddr = 0x0086B4U;
    static constexpr std::uint32_t kSpin360LutAddr = 0x008768U;
    static constexpr std::uint32_t kFbfRecognizerAddr = 0x008778U;
    static constexpr std::uint32_t kFbfPatternTableAddr = 0x0087F4U;

    static constexpr std::uint32_t kInputRingRam = 0x00FF1668U;
    static constexpr std::uint32_t kRingHeadRam = 0x00FF16E8U;
    static constexpr std::uint32_t kRingCountRam = 0x00FF16EAU;
    static constexpr std::uint8_t kRingByteMask = 0x7EU;
    static constexpr std::size_t kRingEntryStride = 2U;
    static constexpr std::size_t kMaxSamples = 30U; // cmpi.w #$1d, d7 (30 samples max)

    // ROM 0x008768: 16-entry LUT mapping low nibble of D-pad to 8-direction cardinal/diagonal.
    // -1 (0xFF) = neutral/invalid, 0 = Up, 2 = Right, 4 = Down, 6 = Left.
    // Odd values (1, 3, 5, 7) are diagonals and are ignored/skipped during 360 spin detection.
    static constexpr std::array<std::int8_t, 16> kPadToDir8Lut = {
        -1, 0, 4, -1, 6, 7, 5, -1, 2, 1, 3, -1, -1, -1, -1, -1,
    };
};

struct FbfPattern {
    std::uint8_t facing{0U};
    std::array<std::uint8_t, 4> sequence{};
    std::array<std::uint8_t, 4> masks{};
    std::uint8_t perp_d2{0U};
    std::uint8_t perp_d3{0U};
    std::uint8_t perp_mask{0U};
};

// ROM 0x0087F4: 4 facing patterns (Up, Right, Down, Left).
inline constexpr std::array<FbfPattern, 4> kFbfPatterns = {{
    {0U, {1U, 2U, 1U, 0U}, {15U, 2U, 15U, 0U}, 4U, 8U, 0x0CU}, // Facing 0 (Up): 1->2->1, perp Left(4)/Right(8)
    {1U, {8U, 4U, 8U, 0U}, {15U, 4U, 15U, 0U}, 1U, 2U, 0x03U}, // Facing 1 (Right): 8->4->8, perp Up(1)/Down(2)
    {2U, {2U, 1U, 2U, 0U}, {15U, 1U, 15U, 0U}, 4U, 8U, 0x0CU}, // Facing 2 (Down): 2->1->2, perp Left(4)/Right(8)
    {3U, {4U, 8U, 4U, 0U}, {15U, 8U, 15U, 0U}, 1U, 2U, 0x03U}, // Facing 3 (Left): 4->8->4, perp Up(1)/Down(2)
}};

/// Direct 360-degree spin recognizer on a sequence of D-pad nibbles (ordered newest to oldest).
[[nodiscard]] bool recognize_spin_360_dpad(std::span<const std::uint8_t> dpad_samples) noexcept;

/// Direct 360-degree spin recognizer on raw 16-bit input ring words (ordered newest to oldest).
[[nodiscard]] bool recognize_spin_360_direct(std::span<const std::uint16_t> ring_words) noexcept;

/// Direct F-B-F command recognizer on a sequence of D-pad nibbles (ordered newest to oldest).
[[nodiscard]] bool recognize_fbf_dpad(
    std::span<const std::uint8_t> dpad_samples,
    std::uint8_t facing) noexcept;

/// Direct F-B-F command recognizer on raw 16-bit input ring words (ordered newest to oldest).
[[nodiscard]] bool recognize_fbf_direct(
    std::span<const std::uint16_t> ring_words,
    std::uint8_t facing) noexcept;

/// ROM 0x0086B4..0x008777: 360° Spin-Slash recognizer.
/// Scans up to min(ring.count(), 30) frames backwards from head.
/// On successful motion match, clears ring.count() ($FF16EA = 0) and returns true.
[[nodiscard]] bool detect_spin_360_motion(video::InputHistoryRing64& ring) noexcept;

/// ROM 0x008778..0x008813: Forward-Back-Forward command recognizer.
/// Rejects if perpendicular input appears in the sample window.
/// Matches facing 3-step sequence. On match, clears ring.count() ($FF16EA = 0) and returns true.
[[nodiscard]] bool detect_fbf_motion(video::InputHistoryRing64& ring, std::uint8_t facing) noexcept;

} // namespace oasis::game::player
