#include "game/player/ali_chained_super_motion.hpp"

#include <vector>

namespace oasis::game::player {

bool recognize_chained_super_motion_direct(
    std::span<const std::uint16_t> ring_words,
    std::uint8_t facing) noexcept {
    if (ring_words.empty()) {
        return false;
    }
    const auto& pattern = kFbfPatterns[facing & 3U];
    const std::size_t limit = ring_words.size();
    std::size_t pat_idx = 0U;
    std::size_t idx = 0U;

    // Phase 1: F-B-F (0x008814..0x00887C)
    for (; idx < limit; ++idx) {
        const std::uint16_t word = ring_words[idx];
        const std::uint8_t hi = static_cast<std::uint8_t>((word >> 8) & 0xFFU);
        const std::uint8_t lo = static_cast<std::uint8_t>(word & 0xFFU);

        if (hi == pattern.perp_d2 || hi == pattern.perp_d3 ||
            (hi & pattern.perp_mask) != 0U || (lo & pattern.perp_mask) != 0U) {
            return false;
        }

        if ((lo & pattern.masks[pat_idx]) == pattern.sequence[pat_idx]) {
            ++pat_idx;
            if (pattern.sequence[pat_idx] == 0U) {
                break; // Matched Phase 1! Note: idx is NOT incremented!
            }
        }
    }

    if (pat_idx < 3U || idx >= limit) {
        return false;
    }

    // Phase 2: Seamless 360° Spin (0x00887E..0x00890E) starting at `idx`
    std::uint8_t stage = 0U;
    std::uint8_t d2 = 0U;
    std::int8_t d3 = 0;

    for (; idx < limit; ++idx) {
        const std::uint8_t nibble = static_cast<std::uint8_t>(ring_words[idx] & 0x0FU);
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

bool recognize_chained_super_motion_dpad(
    std::span<const std::uint8_t> dpad_samples,
    std::uint8_t facing) noexcept {
    if (dpad_samples.empty()) {
        return false;
    }
    std::vector<std::uint16_t> words(dpad_samples.begin(), dpad_samples.end());
    return recognize_chained_super_motion_direct(words, facing);
}

bool detect_chained_super_motion(
    video::InputHistoryRing64& ring,
    std::uint8_t facing) noexcept {
    if (ring.count() == 0U) {
        return false;
    }
    const auto& pattern = kFbfPatterns[facing & 3U];
    const std::size_t limit = ring.count();
    std::size_t pat_idx = 0U;
    std::size_t ago = 0U;

    // Phase 1: F-B-F
    for (; ago < limit; ++ago) {
        const auto word = ring.frames_ago(ago);
        if (!word.has_value()) {
            return false;
        }
        const std::uint8_t hi = static_cast<std::uint8_t>((*word >> 8) & 0xFFU);
        const std::uint8_t lo = static_cast<std::uint8_t>(*word & 0xFFU);

        if (hi == pattern.perp_d2 || hi == pattern.perp_d3 ||
            (hi & pattern.perp_mask) != 0U || (lo & pattern.perp_mask) != 0U) {
            return false;
        }

        if ((lo & pattern.masks[pat_idx]) == pattern.sequence[pat_idx]) {
            ++pat_idx;
            if (pattern.sequence[pat_idx] == 0U) {
                break; // Matched Phase 1! Note: ago is NOT incremented!
            }
        }
    }

    if (pat_idx < 3U || ago >= limit) {
        return false;
    }

    // Phase 2: Seamless 360 Spin continuing from the same `ago`
    std::uint8_t stage = 0U;
    std::uint8_t d2 = 0U;
    std::int8_t d3 = 0;

    for (; ago < limit; ++ago) {
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

} // namespace oasis::game::player
