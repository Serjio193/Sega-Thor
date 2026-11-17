#include "game/video/vint_dma_and_input_ring.hpp"

#include <algorithm>

namespace oasis::game::video {

void VdpDmaQueue::reset() noexcept {
    entries_.clear();
    flags_ff164e_ = 0U;
}

void VdpDmaQueue::enqueue(const VdpDmaEntry& entry) {
    entries_.push_back(entry);
}

void VdpDmaQueue::set_flush_enabled(bool enabled) noexcept {
    if (enabled) {
        flags_ff164e_ |= VintConfig::kDmaFlushFlagBit;
    } else {
        flags_ff164e_ &= static_cast<std::uint8_t>(~VintConfig::kDmaFlushFlagBit);
    }
}

std::vector<VdpDmaEntry> VdpDmaQueue::flush_if_enabled() {
    if ((flags_ff164e_ & VintConfig::kDmaFlushFlagBit) == 0U) {
        return {};
    }
    // ROM 0x002112: Flushes LIFO from tail ($FF1892) down to base ($FF10B4).
    std::vector<VdpDmaEntry> flushed(entries_.rbegin(), entries_.rend());
    entries_.clear();
    flags_ff164e_ &= static_cast<std::uint8_t>(~VintConfig::kDmaFlushFlagBit);
    return flushed;
}

void InputHistoryRing64::reset() noexcept {
    ring_.fill(0U);
    byte_index_ = 0U;
    count_ = 0U;
}

void InputHistoryRing64::record(std::uint16_t input_word) noexcept {
    byte_index_ = static_cast<std::uint8_t>((byte_index_ - 2U) & VintConfig::kInputRingByteMask);
    ring_[byte_index_ >> 1U] = input_word;
    if (count_ < VintConfig::kInputRingMaxCount) {
        ++count_;
    }
}

std::optional<std::uint16_t> InputHistoryRing64::frames_ago(std::size_t ago) const noexcept {
    if (ago >= count_ || ago >= VintConfig::kInputRingSize) {
        return std::nullopt;
    }
    const std::size_t slot = ((byte_index_ >> 1U) + ago) & (VintConfig::kInputRingSize - 1U);
    return ring_[slot];
}

void VintFrameTimers::reset() noexcept {
    frame_counter = 0U;
    countdown_1 = 0U;
    countdown_2 = 0U;
}

void VintFrameTimers::step_frame() noexcept {
    ++frame_counter;
    if (countdown_1 > 0U) {
        --countdown_1;
    }
    if (countdown_2 > 0U) {
        --countdown_2;
    }
}

} // namespace oasis::game::video
