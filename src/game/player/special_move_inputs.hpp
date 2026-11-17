#pragma once

#include "game/video/vint_dma_and_input_ring.hpp"

#include <array>
#include <cstdint>

namespace oasis::game::player {

struct SpecialMoveInputConfig {
    static constexpr std::uint32_t kSpin360DetectorAddr = 0x000086B4U;
    static constexpr std::uint32_t kSpin360LutAddr = 0x00008768U;
    static constexpr std::uint32_t kFbfLungeDetectorAddr = 0x00008778U;
    static constexpr std::uint32_t kFbfLungeTableAddr = 0x000087F4U;
    static constexpr std::size_t kMaxHistoryFrames = 30U;

    // ROM 0x008768: 16-byte D-pad nibble (& 0x0F) LUT.
    // Even values (0,2,4,6) are cardinals; odd (1,3,5,7) are diagonals; 0xFF is neutral/invalid.
    static constexpr std::array<std::uint8_t, 16> kSpin360DpadLut = {
        255U, 0U, 4U, 255U, 6U, 7U, 5U, 255U,
        2U, 1U, 3U, 255U, 255U, 255U, 255U, 255U,
    };
};

// ROM 0x0086B4: Scans up to 30 frames backwards in `ring`, mapping D-pad nibble through
// 0x008768, matching 4 consecutive cardinal steps in CW (+2 mod 8) or CCW (-2 mod 8) order.
// Clears `ring.count()` ($FF16EA) on match.
[[nodiscard]] bool detect_spin_360(video::InputHistoryRing64& ring) noexcept;

// ROM 0x008778: Scans `ring` against `facing`'s 4-byte F-B-F sequence & mask (0x0087F4:
// Up 1,2,1,0; Right 8,4,8,0; Down 2,1,2,0; Left 4,8,4,0), aborting on perpendicular D-pad
// input and clearing `ring.count()` ($FF16EA) on match.
[[nodiscard]] bool detect_fbf_lunge(video::InputHistoryRing64& ring, std::uint8_t facing) noexcept;

} // namespace oasis::game::player
