#include "game/player/ali_command_motions.hpp"

#include <algorithm>
#include <vector>

namespace oasis::game::player {

bool recognize_spin_360_dpad(std::span<const std::uint8_t> dpad_samples) noexcept {
    if (dpad_samples.empty()) {
        return false;
    }
    const std::size_t limit =
        std::min<std::size_t>(dpad_samples.size(), CommandMotionConfig::kMaxSamples);
    std::uint8_t stage = 0U;
    std::uint8_t d2 = 0U;
    std::int8_t d3 = 0;

    for (std::size_t i = 0U; i < limit; ++i) {
        const std::uint8_t nibble = static_cast<std::uint8_t>(dpad_samples[i] & 0x0FU);
        const std::int8_t dir = CommandMotionConfig::kPadToDir8Lut[nibble];
        if (dir < 0 || (dir & 1) != 0) {
            // Neutral (-1) and odd diagonals (1,3,5,7) are ignored
            continue;
        }
        const std::uint8_t d0 = static_cast<std::uint8_t>(dir);

        if (stage == 0U) {
            d2 = d0;
            stage = 1U;
        } else if (stage == 1U) {
            const std::uint8_t cw = static_cast<std::uint8_t>((d2 + 2U) & 7U);
            const std::uint8_t ccw = static_cast<std::uint8_t>((d2 + 6U) & 7U);
            if (d0 == cw) {
                d3 = 2;
                stage = 2U;
                d2 = static_cast<std::uint8_t>((cw + 2U) & 7U);
            } else if (d0 == ccw) {
                d3 = -2;
                stage = 2U;
                d2 = static_cast<std::uint8_t>((ccw + 6U) & 7U);
            }
        } else if (stage == 2U) {
            if (d0 == d2) {
                stage = 3U;
                d2 = static_cast<std::uint8_t>((d2 + (d3 > 0 ? 2U : 6U)) & 7U);
            }
        } else {
            // stage == 3U
            if (d0 == d2) {
                return true;
            }
        }
    }
    return false;
}

bool recognize_spin_360_direct(std::span<const std::uint16_t> ring_words) noexcept {
    if (ring_words.empty()) {
        return false;
    }
    const std::size_t limit =
        std::min<std::size_t>(ring_words.size(), CommandMotionConfig::kMaxSamples);
    std::uint8_t stage = 0U;
    std::uint8_t d2 = 0U;
    std::int8_t d3 = 0;

    for (std::size_t i = 0U; i < limit; ++i) {
        const std::uint8_t nibble = static_cast<std::uint8_t>(ring_words[i] & 0x0FU);
        const std::int8_t dir = CommandMotionConfig::kPadToDir8Lut[nibble];
        if (dir < 0 || (dir & 1) != 0) {
            continue;
        }
        const std::uint8_t d0 = static_cast<std::uint8_t>(dir);

        if (stage == 0U) {
            d2 = d0;
            stage = 1U;
        } else if (stage == 1U) {
            const std::uint8_t cw = static_cast<std::uint8_t>((d2 + 2U) & 7U);
            const std::uint8_t ccw = static_cast<std::uint8_t>((d2 + 6U) & 7U);
            if (d0 == cw) {
                d3 = 2;
                stage = 2U;
                d2 = static_cast<std::uint8_t>((cw + 2U) & 7U);
            } else if (d0 == ccw) {
                d3 = -2;
                stage = 2U;
                d2 = static_cast<std::uint8_t>((ccw + 6U) & 7U);
            }
        } else if (stage == 2U) {
            if (d0 == d2) {
                stage = 3U;
                d2 = static_cast<std::uint8_t>((d2 + (d3 > 0 ? 2U : 6U)) & 7U);
            }
        } else {
            if (d0 == d2) {
                return true;
            }
        }
    }
    return false;
}

bool recognize_fbf_dpad(
    std::span<const std::uint8_t> dpad_samples,
    std::uint8_t facing) noexcept {
    const auto& pattern = kFbfPatterns[facing & 3U];
    const std::size_t limit =
        std::min<std::size_t>(dpad_samples.size(), CommandMotionConfig::kMaxSamples);
    std::size_t stage = 0U;

    for (std::size_t i = 0U; i < limit; ++i) {
        const std::uint8_t dpad = static_cast<std::uint8_t>(dpad_samples[i] & 0x0FU);
        if ((dpad & pattern.perp_mask) != 0U) {
            return false; // Perpendicular input aborts F-B-F motion
        }
        if ((dpad & pattern.masks[stage]) == pattern.sequence[stage]) {
            ++stage;
            if (pattern.sequence[stage] == 0U) {
                return true;
            }
        }
    }
    return false;
}

bool recognize_fbf_direct(
    std::span<const std::uint16_t> ring_words,
    std::uint8_t facing) noexcept {
    const auto& pattern = kFbfPatterns[facing & 3U];
    const std::size_t limit =
        std::min<std::size_t>(ring_words.size(), CommandMotionConfig::kMaxSamples);
    std::size_t stage = 0U;

    for (std::size_t i = 0U; i < limit; ++i) {
        const std::uint8_t dpad = static_cast<std::uint8_t>(ring_words[i] & 0x0FU);
        if ((dpad & pattern.perp_mask) != 0U) {
            return false; // Perpendicular input aborts F-B-F motion
        }
        if ((dpad & pattern.masks[stage]) == pattern.sequence[stage]) {
            ++stage;
            if (pattern.sequence[stage] == 0U) {
                return true;
            }
        }
    }
    return false;
}

bool detect_spin_360_motion(video::InputHistoryRing64& ring) noexcept {
    if (ring.count() == 0U) {
        return false;
    }
    const std::size_t limit =
        std::min<std::size_t>(ring.count(), CommandMotionConfig::kMaxSamples);
    std::uint8_t stage = 0U;
    std::uint8_t d2 = 0U;
    std::int8_t d3 = 0;

    for (std::size_t ago = 0U; ago < limit; ++ago) {
        const auto word = ring.frames_ago(ago);
        if (!word.has_value()) {
            break;
        }
        const std::uint8_t nibble = static_cast<std::uint8_t>(*word & 0x0FU);
        const std::int8_t dir = CommandMotionConfig::kPadToDir8Lut[nibble];
        if (dir < 0 || (dir & 1) != 0) {
            continue;
        }
        const std::uint8_t d0 = static_cast<std::uint8_t>(dir);

        if (stage == 0U) {
            d2 = d0;
            stage = 1U;
        } else if (stage == 1U) {
            const std::uint8_t cw = static_cast<std::uint8_t>((d2 + 2U) & 7U);
            const std::uint8_t ccw = static_cast<std::uint8_t>((d2 + 6U) & 7U);
            if (d0 == cw) {
                d3 = 2;
                stage = 2U;
                d2 = static_cast<std::uint8_t>((cw + 2U) & 7U);
            } else if (d0 == ccw) {
                d3 = -2;
                stage = 2U;
                d2 = static_cast<std::uint8_t>((ccw + 6U) & 7U);
            }
        } else if (stage == 2U) {
            if (d0 == d2) {
                stage = 3U;
                d2 = static_cast<std::uint8_t>((d2 + (d3 > 0 ? 2U : 6U)) & 7U);
            }
        } else {
            if (d0 == d2) {
                ring.clear_count();
                return true;
            }
        }
    }
    return false;
}

bool detect_fbf_motion(video::InputHistoryRing64& ring, std::uint8_t facing) noexcept {
    if (ring.count() == 0U) {
        return false;
    }
    const auto& pattern = kFbfPatterns[facing & 3U];
    const std::size_t limit =
        std::min<std::size_t>(ring.count(), CommandMotionConfig::kMaxSamples);
    std::size_t stage = 0U;

    for (std::size_t ago = 0U; ago < limit; ++ago) {
        const auto word = ring.frames_ago(ago);
        if (!word.has_value()) {
            break;
        }
        const std::uint8_t dpad = static_cast<std::uint8_t>(*word & 0x0FU);
        if ((dpad & pattern.perp_mask) != 0U) {
            return false; // Perpendicular input aborts F-B-F motion
        }
        if ((dpad & pattern.masks[stage]) == pattern.sequence[stage]) {
            ++stage;
            if (pattern.sequence[stage] == 0U) {
                ring.clear_count();
                return true;
            }
        }
    }
    return false;
}

} // namespace oasis::game::player
