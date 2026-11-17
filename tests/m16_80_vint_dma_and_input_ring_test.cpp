#include "game/video/vint_dma_and_input_ring.hpp"

#include <cassert>

int main() {
    using oasis::game::video::InputHistoryRing64;
    using oasis::game::video::VdpDmaEntry;
    using oasis::game::video::VdpDmaQueue;
    using oasis::game::video::VintFrameTimers;

    // 1. VdpDmaQueue: gated by $FF164E bit 1, flushes LIFO, clears queue and bit 1.
    VdpDmaQueue dma;
    dma.enqueue(VdpDmaEntry{0x010000U, 0x1000U, 16U});
    dma.enqueue(VdpDmaEntry{0x020000U, 0x2000U, 32U});
    assert(dma.flush_if_enabled().empty()); // Bit 1 not set -> not flushed
    assert(dma.size() == 2U);

    dma.set_flush_enabled(true);
    const auto flushed = dma.flush_if_enabled();
    assert(flushed.size() == 2U);
    assert(flushed[0].vram_dst == 0x2000U); // LIFO order
    assert(flushed[1].vram_dst == 0x1000U);
    assert(dma.size() == 0U);
    assert((dma.flags_ff164e() & 0x02U) == 0U);

    // 2. InputHistoryRing64: reverse-indexed (index - 2) & 0x7E, count clamped to 0x3F (63).
    InputHistoryRing64 ring;
    assert(!ring.frames_ago(0U).has_value());
    for (std::uint16_t i = 1U; i <= 70U; ++i) {
        ring.record(i);
    }
    assert(ring.count() == 0x3FU); // Clamped to 63
    assert(ring.frames_ago(0U) == 70U);
    assert(ring.frames_ago(1U) == 69U);
    assert(ring.frames_ago(62U) == 8U);
    assert(!ring.frames_ago(63U).has_value());

    // Soft reset combo 0xE0000202.
    assert(InputHistoryRing64::is_soft_reset_combo(0xE0000202U));
    assert(!InputHistoryRing64::is_soft_reset_combo(0x00000000U));

    // 3. VintFrameTimers: frame_counter += 1, countdown_1/2 decrement when > 0.
    VintFrameTimers timers;
    timers.countdown_1 = 2U;
    timers.countdown_2 = 1U;
    timers.step_frame();
    assert(timers.frame_counter == 1U && timers.countdown_1 == 1U && timers.countdown_2 == 0U);
    timers.step_frame();
    assert(timers.frame_counter == 2U && timers.countdown_1 == 0U && timers.countdown_2 == 0U);

    return 0;
}
