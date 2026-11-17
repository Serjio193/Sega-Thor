#include "game/player/ali_rapid_tripletap_demo.hpp"

#include <algorithm>

namespace oasis::game::player {

namespace {

[[nodiscard]] constexpr bool has_button_a(std::uint16_t word) noexcept {
    return (word & (1U << RapidMashTripleTapDemoConfig::kMashButtonBit)) != 0U ||
           ((word >> 8U) & (1U << RapidMashTripleTapDemoConfig::kMashButtonBit)) != 0U;
}

[[nodiscard]] constexpr std::uint8_t extract_dpad_nibble(std::uint16_t word) noexcept {
    const std::uint8_t low = static_cast<std::uint8_t>(word & 0x0FU);
    if (low != 0U) {
        return low;
    }
    return static_cast<std::uint8_t>((word >> 8U) & 0x0FU);
}

} // namespace

bool recognize_rapid_mash_direct(std::span<const std::uint16_t> recent_words) noexcept {
    const std::size_t limit =
        std::min<std::size_t>(recent_words.size(), RapidMashTripleTapDemoConfig::kMashWindowFrames);
    std::uint16_t hits = 0U;
    for (std::size_t i = 0U; i < limit; ++i) {
        if (has_button_a(recent_words[i])) {
            ++hits;
        }
    }
    return hits >= RapidMashTripleTapDemoConfig::kMashRequiredPresses;
}

bool detect_rapid_mash_motion(video::InputHistoryRing64& ring) noexcept {
    std::uint16_t hits = 0U;
    for (std::size_t ago = 0U; ago < RapidMashTripleTapDemoConfig::kMashWindowFrames; ++ago) {
        const auto word = ring.frames_ago(ago);
        if (!word.has_value()) {
            break;
        }
        if (has_button_a(*word)) {
            ++hits;
        }
    }
    if (hits >= RapidMashTripleTapDemoConfig::kMashRequiredPresses) {
        ring.clear_count();
        return true;
    }
    return false;
}

bool recognize_triple_tap_direct(
    std::span<const std::uint16_t> recent_words,
    std::uint8_t facing) noexcept {
    const std::size_t limit =
        std::min<std::size_t>(recent_words.size(), RapidMashTripleTapDemoConfig::kTripleTapMaxFrames);
    const std::uint8_t target = target_dir_for_facing(facing);
    std::uint16_t rem = RapidMashTripleTapDemoConfig::kTripleTapRequiredMatches;

    for (std::size_t i = 0U; i < limit; ++i) {
        const std::uint8_t dpad = extract_dpad_nibble(recent_words[i]);
        if (dpad == 0U) {
            continue;
        }
        if (dpad != target) {
            return false;
        }
        --rem;
        if (rem == 0U) {
            return true;
        }
    }
    return false;
}

bool recognize_triple_tap_dpad(
    std::span<const std::uint8_t> recent_dpad_bytes,
    std::uint8_t facing) noexcept {
    const std::size_t limit =
        std::min<std::size_t>(recent_dpad_bytes.size(), RapidMashTripleTapDemoConfig::kTripleTapMaxFrames);
    const std::uint8_t target = target_dir_for_facing(facing);
    std::uint16_t rem = RapidMashTripleTapDemoConfig::kTripleTapRequiredMatches;

    for (std::size_t i = 0U; i < limit; ++i) {
        const std::uint8_t dpad = static_cast<std::uint8_t>(recent_dpad_bytes[i] & 0x0FU);
        if (dpad == 0U) {
            continue;
        }
        if (dpad != target) {
            return false;
        }
        --rem;
        if (rem == 0U) {
            return true;
        }
    }
    return false;
}

bool detect_triple_tap_motion(
    video::InputHistoryRing64& ring,
    std::uint8_t facing) noexcept {
    if (ring.count() == 0U) {
        return false;
    }
    const std::size_t limit =
        std::min<std::size_t>(ring.count(), RapidMashTripleTapDemoConfig::kTripleTapMaxFrames);
    const std::uint8_t target = target_dir_for_facing(facing);
    std::uint16_t rem = RapidMashTripleTapDemoConfig::kTripleTapRequiredMatches;

    for (std::size_t ago = 0U; ago < limit; ++ago) {
        const auto word = ring.frames_ago(ago);
        if (!word.has_value()) {
            break;
        }
        const std::uint8_t dpad = extract_dpad_nibble(*word);
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

std::optional<std::uint16_t> lookup_demo_asset_offset(std::uint8_t index) noexcept {
    if (index >= RapidMashTripleTapDemoConfig::kDemoEntryCount) {
        return std::nullopt;
    }
    return RapidMashTripleTapDemoConfig::kDemoAssetOffsets[index];
}

std::optional<std::uint32_t> lookup_demo_stream_rom_addr(std::uint8_t index) noexcept {
    if (index >= RapidMashTripleTapDemoConfig::kDemoEntryCount) {
        return std::nullopt;
    }
    return RapidMashTripleTapDemoConfig::kDemoAbsoluteAddrs[index];
}

void delta_reconstruct_demo_stream(std::span<std::uint8_t> buffer) noexcept {
    if (buffer.size() < 2U) {
        return;
    }
    std::uint8_t acc = buffer[0];
    for (std::size_t i = 1U; i < buffer.size(); ++i) {
        acc = static_cast<std::uint8_t>(acc + buffer[i]);
        buffer[i] = acc;
    }
}

void DemoPlaybackState::reset() noexcept {
    demo_index = 0U;
    demo_active = false;
    demo_ready = false;
    current_stream_rom_addr = RapidMashTripleTapDemoConfig::kDemoAbsoluteAddrs[0];
    demo_wait_timer = 0U;
}

void DemoPlaybackState::advance_cycle() noexcept {
    demo_index = cycle_demo_index(demo_index);
    current_stream_rom_addr = RapidMashTripleTapDemoConfig::kDemoAbsoluteAddrs[demo_index];
    demo_active = true;
    demo_ready = true;
    demo_wait_timer = RapidMashTripleTapDemoConfig::kDemoWaitTimerDefault;
}

} // namespace oasis::game::player
