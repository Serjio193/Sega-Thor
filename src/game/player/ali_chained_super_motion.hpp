#pragma once

#include "game/player/ali_command_motions.hpp"
#include "game/video/vint_dma_and_input_ring.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace oasis::game::player {

struct ChainedSuperMotionConfig {
    static constexpr std::uint32_t kChainedSuperMotionAddr = 0x008814U;
    static constexpr std::uint32_t kPhase1FbfAddr = 0x008814U;
    static constexpr std::uint32_t kPhase2Spin360Addr = 0x00887EU;
    static constexpr std::uint32_t kChainedSuperMotionEndAddr = 0x00890EU;
};

/// Direct recognition of chained F-B-F + 360° spin over raw input words (ordered newest to oldest).
[[nodiscard]] bool recognize_chained_super_motion_direct(
    std::span<const std::uint16_t> ring_words,
    std::uint8_t facing) noexcept;

/// Direct recognition of chained F-B-F + 360° spin over raw D-pad bytes (ordered newest to oldest).
[[nodiscard]] bool recognize_chained_super_motion_dpad(
    std::span<const std::uint8_t> dpad_samples,
    std::uint8_t facing) noexcept;

/// ROM 0x008814..0x00890E: Chained F-B-F + 360° Spin Super-Motion Recognizer.
/// Scans full available ring.count() backwards from head.
/// Phase 1 matches F-B-F for `facing`, rejecting perpendicular inputs.
/// Phase 2 continues seamlessly from the 3rd matched F-B-F sample to detect 360° spin.
/// On completion, clears ring.count() ($FF16EA = 0) and returns true.
[[nodiscard]] bool detect_chained_super_motion(
    video::InputHistoryRing64& ring,
    std::uint8_t facing) noexcept;

} // namespace oasis::game::player
