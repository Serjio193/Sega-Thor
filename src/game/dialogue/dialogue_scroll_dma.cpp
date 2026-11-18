#include "game/dialogue/dialogue_scroll_dma.hpp"

#include <algorithm>
#include <cstdint>

namespace oasis::game::dialogue {

bool calculate_vdp_clear_line(
    std::uint16_t vram_base,
    std::uint16_t line_offset,
    VdpClearLineRecord& out_record) noexcept {
    if (line_offset >= 8U) {
        return false;
    }
    out_record.dest_vram = static_cast<std::uint16_t>(vram_base + (line_offset << 2U));
    out_record.line_offset = line_offset;
    out_record.row_count = 4U;
    out_record.words_per_row = 32U;
    return true;
}

bool advance_line_and_scroll(
    WindowScrollState& state,
    std::vector<DmaQueueEntry>& out_dma_queue,
    std::vector<VdpClearLineRecord>* out_vdp_clears) noexcept {
    state.cursor_x = 0;
    state.line_counter++;

    if (state.line_counter <= 3) {
        return false;
    }

    // Scroll loop: decrements all 4 row offsets by 2 until one reaches 0
    while (true) {
        for (std::size_t i = 0; i < 4U; ++i) {
            state.row_offsets[i] = static_cast<std::int16_t>(
                state.row_offsets[i] - DialogueScrollDmaConfig::kScrollStepDecrement);
        }

        // Check for line recycling when offset hits 0
        if (state.row_offsets[0] == 0) {
            out_dma_queue.push_back(make_blank_dma_entry(state.plane_top_base));
            state.row_offsets[0] = DialogueScrollDmaConfig::kRowRecycleValue;
            return true;
        }
        if (state.row_offsets[1] == 0) {
            out_dma_queue.push_back(make_blank_dma_entry(
                static_cast<std::uint16_t>(state.plane_top_base + DialogueScrollDmaConfig::kLineRowStrideBytes)));
            state.row_offsets[1] = DialogueScrollDmaConfig::kRowRecycleValue;
            return true;
        }
        if (state.row_offsets[2] == 0) {
            out_dma_queue.push_back(make_blank_dma_entry(state.plane_bottom_base));
            state.row_offsets[2] = DialogueScrollDmaConfig::kRowRecycleValue;
            return true;
        }
        if (state.row_offsets[3] == 0) {
            out_dma_queue.push_back(make_blank_dma_entry(
                static_cast<std::uint16_t>(state.plane_bottom_base + DialogueScrollDmaConfig::kLineRowStrideBytes)));
            state.row_offsets[3] = DialogueScrollDmaConfig::kRowRecycleValue;
            return true;
        }

        // Determine which line is rolling off the top (< 12)
        std::uint16_t vram_base = state.plane_top_base;
        std::int16_t active_offset = state.row_offsets[0];

        if (state.row_offsets[0] < 12) {
            active_offset = state.row_offsets[0];
        } else if (state.row_offsets[1] < 12) {
            vram_base = static_cast<std::uint16_t>(state.plane_top_base + DialogueScrollDmaConfig::kLineRowStrideBytes);
            active_offset = state.row_offsets[1];
        } else if (state.row_offsets[2] < 12) {
            vram_base = state.plane_bottom_base;
            active_offset = state.row_offsets[2];
        } else {
            vram_base = static_cast<std::uint16_t>(state.plane_bottom_base + DialogueScrollDmaConfig::kLineRowStrideBytes);
            active_offset = state.row_offsets[3];
        }

        const std::int16_t line_clear_idx = static_cast<std::int16_t>(10 - active_offset);
        if (line_clear_idx >= 0 && out_vdp_clears != nullptr) {
            VdpClearLineRecord rec{};
            if (calculate_vdp_clear_line(vram_base, static_cast<std::uint16_t>(line_clear_idx), rec)) {
                out_vdp_clears->push_back(rec);
            }
        }
    }
}

DmaQueueEntry make_glyph_dma_entry(
    WindowScrollState& state,
    std::uint8_t char_code) noexcept {
    const std::uint16_t glyph_idx = (char_code >= 0x20U)
        ? static_cast<std::uint16_t>(char_code - 0x20U)
        : 0U;

    const std::uint32_t src_byte_addr = DialogueScrollDmaConfig::kGlyphBufferBaseRam +
                                         (static_cast<std::uint32_t>(glyph_idx) << 5U);
    const std::uint32_t src_word_addr = src_byte_addr >> 1U;

    const std::uint16_t line_idx = static_cast<std::uint16_t>(state.line_counter & 3);
    std::uint16_t dest_base = (line_idx < 2U) ? state.plane_top_base : state.plane_bottom_base;
    if ((line_idx & 1U) != 0U) {
        dest_base = static_cast<std::uint16_t>(dest_base + DialogueScrollDmaConfig::kLineRowStrideBytes);
    }

    const std::uint16_t dest_vram = static_cast<std::uint16_t>(
        dest_base + (static_cast<std::uint16_t>(state.cursor_x) << 5U));

    state.cursor_x++;

    return DmaQueueEntry{
        .src_word_addr = src_word_addr,
        .dest_vram = dest_vram,
        .word_count = DialogueScrollDmaConfig::kGlyphDmaWordCount
    };
}

void sync_scroll_state_to_ram(
    const WindowScrollState& state,
    std::span<std::uint8_t> ram_window) {
    if (ram_window.size() < 0x10000U) {
        return;
    }
    constexpr auto wr_u16 = [](std::span<std::uint8_t> ram, std::uint32_t addr, std::uint16_t v) {
        const std::size_t off = addr & 0xFFFFU;
        ram[off] = static_cast<std::uint8_t>((v >> 8U) & 0xFFU);
        ram[off + 1U] = static_cast<std::uint8_t>(v & 0xFFU);
    };

    wr_u16(ram_window, DialogueScrollDmaConfig::kPlaneTopBaseRam, state.plane_top_base);
    wr_u16(ram_window, DialogueScrollDmaConfig::kPlaneBottomBaseRam, state.plane_bottom_base);
    wr_u16(ram_window, DialogueScrollDmaConfig::kRowOffset0Ram, static_cast<std::uint16_t>(state.row_offsets[0]));
    wr_u16(ram_window, DialogueScrollDmaConfig::kRowOffset1Ram, static_cast<std::uint16_t>(state.row_offsets[1]));
    wr_u16(ram_window, DialogueScrollDmaConfig::kRowOffset2Ram, static_cast<std::uint16_t>(state.row_offsets[2]));
    wr_u16(ram_window, DialogueScrollDmaConfig::kRowOffset3Ram, static_cast<std::uint16_t>(state.row_offsets[3]));
    wr_u16(ram_window, DialogueScrollDmaConfig::kCursorXRam, static_cast<std::uint16_t>(state.cursor_x));
    wr_u16(ram_window, DialogueScrollDmaConfig::kLineCounterRam, static_cast<std::uint16_t>(state.line_counter));
}

void append_dma_entry_to_ram(
    std::span<std::uint8_t> ram_window,
    const DmaQueueEntry& entry) {
    if (ram_window.size() < 0x10000U) {
        return;
    }
    constexpr std::size_t kQueuePtrAddr = DialogueScrollDmaConfig::kDmaQueuePtrRam & 0xFFFFU;
    const std::uint32_t queue_ptr = (static_cast<std::uint32_t>(ram_window[kQueuePtrAddr]) << 24U) |
                                    (static_cast<std::uint32_t>(ram_window[kQueuePtrAddr + 1U]) << 16U) |
                                    (static_cast<std::uint32_t>(ram_window[kQueuePtrAddr + 2U]) << 8U) |
                                    static_cast<std::uint32_t>(ram_window[kQueuePtrAddr + 3U]);

    const std::size_t target_off = queue_ptr & 0xFFFFU;
    if (target_off + DialogueScrollDmaConfig::kDmaQueueEntrySizeBytes > ram_window.size()) {
        return;
    }

    // [0..3]: Source word address
    ram_window[target_off + 0U] = static_cast<std::uint8_t>((entry.src_word_addr >> 24U) & 0xFFU);
    ram_window[target_off + 1U] = static_cast<std::uint8_t>((entry.src_word_addr >> 16U) & 0xFFU);
    ram_window[target_off + 2U] = static_cast<std::uint8_t>((entry.src_word_addr >> 8U) & 0xFFU);
    ram_window[target_off + 3U] = static_cast<std::uint8_t>(entry.src_word_addr & 0xFFU);

    // [4..5]: VRAM destination address
    ram_window[target_off + 4U] = static_cast<std::uint8_t>((entry.dest_vram >> 8U) & 0xFFU);
    ram_window[target_off + 5U] = static_cast<std::uint8_t>(entry.dest_vram & 0xFFU);

    // [6..7]: Transfer word count
    ram_window[target_off + 6U] = static_cast<std::uint8_t>((entry.word_count >> 8U) & 0xFFU);
    ram_window[target_off + 7U] = static_cast<std::uint8_t>(entry.word_count & 0xFFU);

    // Update queue pointer
    const std::uint32_t next_ptr = queue_ptr + static_cast<std::uint32_t>(DialogueScrollDmaConfig::kDmaQueueEntrySizeBytes);
    ram_window[kQueuePtrAddr + 0U] = static_cast<std::uint8_t>((next_ptr >> 24U) & 0xFFU);
    ram_window[kQueuePtrAddr + 1U] = static_cast<std::uint8_t>((next_ptr >> 16U) & 0xFFU);
    ram_window[kQueuePtrAddr + 2U] = static_cast<std::uint8_t>((next_ptr >> 8U) & 0xFFU);
    ram_window[kQueuePtrAddr + 3U] = static_cast<std::uint8_t>(next_ptr & 0xFFU);

    // Set bit 1 on $FF164E
    ram_window[DialogueScrollDmaConfig::kVdpFlagsRam & 0xFFFFU] |= 0x02U;
}

} // namespace oasis::game::dialogue
