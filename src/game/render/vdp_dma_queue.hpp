#pragma once

#include "genesis/vdp_state.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace oasis::game::render {

/// Reconstructed 8-byte Genesis DMA queue record matching ROM $FF10B4..$FF1892.
struct DmaTransferEvent {
    std::uint32_t guest_pc{0};
    std::uint32_t src_byte_addr{0};
    std::uint16_t dst_vram_addr{0};
    std::uint16_t length_words{0};
    std::uint8_t auto_inc{2};
};

/// Models the Beyond Oasis DMA queue producer ($FF1892) and ROM 0x002130 LIFO consumer.
class VdpDmaQueue {
public:
    static constexpr std::size_t kMaxQueuedTransfers = 128U;

    void clear() noexcept;
    bool enqueue(const DmaTransferEvent& event) noexcept;

    /// Drains queued transfers in ROM 0x002130 LIFO order (`subq.l #8, a0`) into VRAM.
    /// Leaves inactive VRAM outside each destination span untouched.
    std::size_t drain_lifo(std::span<const std::uint8_t> rom, genesis::VdpState& vdp) noexcept;

    [[nodiscard]] const std::vector<DmaTransferEvent>& queued() const noexcept { return queued_; }
    [[nodiscard]] const std::vector<DmaTransferEvent>& last_drained() const noexcept { return last_drained_; }

private:
    std::vector<DmaTransferEvent> queued_{};
    std::vector<DmaTransferEvent> last_drained_{};
};

} // namespace oasis::game::render
