#include "game/render/vdp_dma_queue.hpp"

namespace oasis::game::render {

void VdpDmaQueue::clear() noexcept {
    queued_.clear();
}

bool VdpDmaQueue::enqueue(const DmaTransferEvent& event) noexcept {
    if (queued_.size() >= kMaxQueuedTransfers || event.length_words == 0U || event.auto_inc == 0U) {
        return false;
    }
    queued_.push_back(event);
    return true;
}

std::size_t VdpDmaQueue::drain_lifo(std::span<const std::uint8_t> rom, genesis::VdpState& vdp) noexcept {
    last_drained_.clear();
    if (queued_.empty() || rom.empty()) {
        return 0U;
    }

    std::size_t bytes_transferred = 0U;
    // ROM 0x002130 iterates queue backward via `subq.l #8, a0` from ($FF1892) down to $FF10B4.
    for (auto it = queued_.rbegin(); it != queued_.rend(); ++it) {
        const auto& ev = *it;
        const std::size_t word_count = static_cast<std::size_t>(ev.length_words);
        std::size_t src = static_cast<std::size_t>(ev.src_byte_addr);
        std::size_t dst = static_cast<std::size_t>(ev.dst_vram_addr);

        for (std::size_t w = 0U; w < word_count; ++w) {
            if (src + 1U < rom.size() && dst + 1U < vdp.vram.size()) {
                vdp.vram[dst] = rom[src];
                vdp.vram[dst + 1U] = rom[src + 1U];
                bytes_transferred += 2U;
            }
            src += 2U;
            dst = (dst + ev.auto_inc) & 0xFFFFU;
        }

        // Update VDP registers 15, 19..23 post-DMA state (length reaches 0; src advances by words).
        const std::uint32_t end_word_addr = (ev.src_byte_addr >> 1U) + ev.length_words;
        vdp.registers[15] = ev.auto_inc;
        vdp.registers[19] = 0U;
        vdp.registers[20] = 0U;
        vdp.registers[21] = static_cast<std::uint8_t>(end_word_addr & 0xFFU);
        vdp.registers[22] = static_cast<std::uint8_t>((end_word_addr >> 8U) & 0xFFU);
        vdp.registers[23] = static_cast<std::uint8_t>((end_word_addr >> 16U) & 0x7FU);

        last_drained_.push_back(ev);
    }

    queued_.clear();
    return bytes_transferred;
}

} // namespace oasis::game::render
