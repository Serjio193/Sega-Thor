#include "game/input/mash_tripletap_detectors.hpp"

#include <algorithm>

namespace oasis::game::input {

bool detect_rapid_mash(std::span<const std::uint16_t> recent_words) noexcept {
    const std::size_t limit =
        std::min<std::size_t>(recent_words.size(), MashTripleTapConfig::kMashWindowFrames);
    std::uint16_t hits = 0U;
    for (std::size_t i = 0U; i < limit; ++i) {
        if ((recent_words[i] & (1U << MashTripleTapConfig::kMashButtonBit)) != 0U) {
            ++hits;
        }
    }
    return hits >= MashTripleTapConfig::kMashMinPresses;
}

bool detect_rapid_mash(video::InputHistoryRing64& ring) noexcept {
    std::uint16_t hits = 0U;
    for (std::size_t ago = 0U; ago < MashTripleTapConfig::kMashWindowFrames; ++ago) {
        const auto word = ring.frames_ago(ago);
        if (!word.has_value()) {
            break;
        }
        if ((*word & (1U << MashTripleTapConfig::kMashButtonBit)) != 0U) {
            ++hits;
        }
    }
    if (hits >= MashTripleTapConfig::kMashMinPresses) {
        ring.clear_count();
        return true;
    }
    return false;
}

bool detect_triple_tap(std::span<const std::uint16_t> recent_words, std::uint8_t target_dir_nibble) noexcept {
    const std::size_t limit =
        std::min<std::size_t>(recent_words.size(), MashTripleTapConfig::kTapMaxWindowFrames);
    std::uint16_t rem = MashTripleTapConfig::kTapRequiredMatches;

    for (std::size_t i = 0U; i < limit; ++i) {
        const std::uint8_t dpad = static_cast<std::uint8_t>(recent_words[i] & 0x0FU);
        if (dpad == 0U) {
            continue;
        }
        if (dpad != target_dir_nibble) {
            return false;
        }
        --rem;
        if (rem == 0U) {
            return true;
        }
    }
    return false;
}

bool detect_triple_tap_bytes(std::span<const std::uint8_t> recent_dpad_bytes, std::uint8_t target_dir_nibble) noexcept {
    const std::size_t limit =
        std::min<std::size_t>(recent_dpad_bytes.size(), MashTripleTapConfig::kTapMaxWindowFrames);
    std::uint16_t rem = MashTripleTapConfig::kTapRequiredMatches;

    for (std::size_t i = 0U; i < limit; ++i) {
        const std::uint8_t dpad = static_cast<std::uint8_t>(recent_dpad_bytes[i] & 0x0FU);
        if (dpad == 0U) {
            continue;
        }
        if (dpad != target_dir_nibble) {
            return false;
        }
        --rem;
        if (rem == 0U) {
            return true;
        }
    }
    return false;
}

bool detect_triple_tap(video::InputHistoryRing64& ring, std::uint8_t facing) noexcept {
    if (ring.count() == 0U) {
        return false;
    }
    const std::size_t limit =
        std::min<std::size_t>(ring.count(), MashTripleTapConfig::kTapMaxWindowFrames);
    const std::uint8_t target = target_dir_for_facing(facing);
    std::uint16_t rem = MashTripleTapConfig::kTapRequiredMatches;

    for (std::size_t ago = 0U; ago < limit; ++ago) {
        const auto word = ring.frames_ago(ago);
        if (!word.has_value()) {
            break;
        }
        const std::uint8_t dpad = static_cast<std::uint8_t>(*word & 0x0FU);
        if (dpad == 0U) {
            continue;
        }
        if (dpad != target) {
            return false;
        }
        --rem;
        if (rem == 0U) {
            ring.clear_count();
            return true;
        }
    }
    return false;
}

} // namespace oasis::game::input
