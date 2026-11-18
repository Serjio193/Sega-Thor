#include "game/dialogue/dialogue_scroll_dma.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

using namespace oasis::game::dialogue;

static void test_constants_and_addresses() {
    static_assert(DialogueScrollDmaConfig::kEntryAdvanceLineAddr == 0x00C566U);
    static_assert(DialogueScrollDmaConfig::kEntryVdpClearLineAddr == 0x00C66AU);
    static_assert(DialogueScrollDmaConfig::kEntryRenderGlyphDmaAddr == 0x00C6BEU);
    static_assert(DialogueScrollDmaConfig::kEntryQueueBlankDmaAddr == 0x00C734U);
    static_assert(DialogueScrollDmaConfig::kRangeEndAddr == 0x00C756U);
    static_assert(DialogueScrollDmaConfig::kRoutineSizeBytes == 496U);

    static_assert(DialogueScrollDmaConfig::kInitRowOffsets[0] == 12);
    static_assert(DialogueScrollDmaConfig::kInitRowOffsets[1] == 24);
    static_assert(DialogueScrollDmaConfig::kInitRowOffsets[2] == 36);
    static_assert(DialogueScrollDmaConfig::kInitRowOffsets[3] == 48);

    static_assert(DialogueScrollDmaConfig::kRowRecycleValue == 48);
    static_assert(DialogueScrollDmaConfig::kScrollStepDecrement == 2);
    static_assert(DialogueScrollDmaConfig::kLineRowStrideBytes == 1024U);
    static_assert(DialogueScrollDmaConfig::kGlyphTileSizeBytes == 32U);
    static_assert(DialogueScrollDmaConfig::kDmaQueueEntrySizeBytes == 8U);

    static_assert(DialogueScrollDmaConfig::kBlankRomSourceWordAddr == 0x007F9DD4U);
    static_assert(DialogueScrollDmaConfig::kBlankDmaWordCount == 512U);
    static_assert(DialogueScrollDmaConfig::kGlyphDmaWordCount == 16U);

    static_assert(DialogueScrollDmaConfig::kPlaneTopBaseRam == 0x00FF1872U);
    static_assert(DialogueScrollDmaConfig::kPlaneBottomBaseRam == 0x00FF1874U);
    static_assert(DialogueScrollDmaConfig::kRowOffset0Ram == 0x00FF1876U);
    static_assert(DialogueScrollDmaConfig::kCursorXRam == 0x00FF187EU);
    static_assert(DialogueScrollDmaConfig::kLineCounterRam == 0x00FF1880U);
    static_assert(DialogueScrollDmaConfig::kDmaQueuePtrRam == 0x00FF1892U);
    static_assert(DialogueScrollDmaConfig::kVdpFlagsRam == 0x00FF164EU);
    static_assert(DialogueScrollDmaConfig::kGlyphBufferBaseRam == 0x00FF2FA8U);

    std::cout << "[PASS] test_constants_and_addresses\n";
}

static void test_line_advance_without_scroll() {
    WindowScrollState state{
        .plane_top_base = 0x2000U,
        .plane_bottom_base = 0x3000U,
        .row_offsets = {12, 24, 36, 48},
        .cursor_x = 15,
        .line_counter = 0
    };

    std::vector<DmaQueueEntry> dma_queue;

    // Line advance 1: counter becomes 1 (no scroll)
    const bool s1 = advance_line_and_scroll(state, dma_queue);
    assert(!s1);
    assert(state.cursor_x == 0);
    assert(state.line_counter == 1);
    assert(dma_queue.empty());
    assert(state.row_offsets[0] == 12 && state.row_offsets[1] == 24);

    // Line advance 2: counter becomes 2 (no scroll)
    const bool s2 = advance_line_and_scroll(state, dma_queue);
    assert(!s2);
    assert(state.line_counter == 2);
    assert(dma_queue.empty());

    // Line advance 3: counter becomes 3 (no scroll)
    const bool s3 = advance_line_and_scroll(state, dma_queue);
    assert(!s3);
    assert(state.line_counter == 3);
    assert(dma_queue.empty());

    std::cout << "[PASS] test_line_advance_without_scroll\n";
}

static void test_circular_scroll_and_recycling() {
    WindowScrollState state{
        .plane_top_base = 0x1000U,
        .plane_bottom_base = 0x2000U,
        .row_offsets = {12, 24, 36, 48},
        .cursor_x = 20,
        .line_counter = 3
    };

    std::vector<DmaQueueEntry> dma_queue;
    std::vector<VdpClearLineRecord> vdp_clears;

    // Line advance 4: counter becomes 4 -> triggers scroll!
    // Row 0 (12) steps by -2 six times -> hits 0 -> recycles to 48
    const bool s4 = advance_line_and_scroll(state, dma_queue, &vdp_clears);
    assert(s4);
    assert(state.line_counter == 4);
    assert(state.cursor_x == 0);
    assert(state.row_offsets[0] == 48);
    assert(state.row_offsets[1] == 12); // 24 - 12 = 12
    assert(state.row_offsets[2] == 24); // 36 - 12 = 24
    assert(state.row_offsets[3] == 36); // 48 - 12 = 36

    assert(dma_queue.size() == 1U);
    assert(dma_queue[0].src_word_addr == DialogueScrollDmaConfig::kBlankRomSourceWordAddr);
    assert(dma_queue[0].dest_vram == 0x1000U); // plane_top_base
    assert(dma_queue[0].word_count == 512U);

    // Line advance 5: counter becomes 5 -> triggers scroll!
    // Row 1 (now 12) steps to 0 -> recycles to 48
    dma_queue.clear();
    const bool s5 = advance_line_and_scroll(state, dma_queue);
    assert(s5);
    assert(state.line_counter == 5);
    assert(state.row_offsets[0] == 36);
    assert(state.row_offsets[1] == 48); // recycled
    assert(state.row_offsets[2] == 12);
    assert(state.row_offsets[3] == 24);

    assert(dma_queue.size() == 1U);
    assert(dma_queue[0].dest_vram == 0x1400U); // plane_top_base + 0x400

    // Line advance 6: counter becomes 6 -> Row 2 recycles to 48
    dma_queue.clear();
    const bool s6 = advance_line_and_scroll(state, dma_queue);
    assert(s6);
    assert(state.line_counter == 6);
    assert(state.row_offsets[0] == 24);
    assert(state.row_offsets[1] == 36);
    assert(state.row_offsets[2] == 48); // recycled
    assert(state.row_offsets[3] == 12);

    assert(dma_queue.size() == 1U);
    assert(dma_queue[0].dest_vram == 0x2000U); // plane_bottom_base

    // Line advance 7: counter becomes 7 -> Row 3 recycles to 48
    dma_queue.clear();
    const bool s7 = advance_line_and_scroll(state, dma_queue);
    assert(s7);
    assert(state.line_counter == 7);
    assert(state.row_offsets[0] == 12); // Back to original full circle!
    assert(state.row_offsets[1] == 24);
    assert(state.row_offsets[2] == 36);
    assert(state.row_offsets[3] == 48);

    assert(dma_queue.size() == 1U);
    assert(dma_queue[0].dest_vram == 0x2400U); // plane_bottom_base + 0x400

    std::cout << "[PASS] test_circular_scroll_and_recycling\n";
}

static void test_vdp_clear_line_records() {
    VdpClearLineRecord rec{};

    assert(calculate_vdp_clear_line(0x4000U, 0U, rec));
    assert(rec.dest_vram == 0x4000U);
    assert(rec.line_offset == 0U);
    assert(rec.row_count == 4U);
    assert(rec.words_per_row == 32U);

    assert(calculate_vdp_clear_line(0x4000U, 5U, rec));
    assert(rec.dest_vram == 0x4000U + 20U);
    assert(rec.line_offset == 5U);

    assert(calculate_vdp_clear_line(0x4000U, 7U, rec));
    assert(rec.dest_vram == 0x4000U + 28U);

    // Offsets >= 8 return false
    assert(!calculate_vdp_clear_line(0x4000U, 8U, rec));
    assert(!calculate_vdp_clear_line(0x4000U, 12U, rec));

    std::cout << "[PASS] test_vdp_clear_line_records\n";
}

static void test_glyph_dma_generation() {
    WindowScrollState state{
        .plane_top_base = 0x2000U,
        .plane_bottom_base = 0x3000U,
        .row_offsets = {12, 24, 36, 48},
        .cursor_x = 0,
        .line_counter = 0 // Line 0 -> plane_top_base
    };

    // ' ' (space, ASCII 0x20 -> glyph 0)
    const DmaQueueEntry e_space = make_glyph_dma_entry(state, ' ');
    assert(e_space.src_word_addr == (0x00FF2FA8U >> 1U));
    assert(e_space.dest_vram == 0x2000U);
    assert(e_space.word_count == 16U);
    assert(state.cursor_x == 1);

    // 'A' (ASCII 0x41 -> glyph index 33 = 0x21)
    // 0x00FF2FA8 + 33 * 32 = 0x00FF33C8 -> word addr 0x007F99E4
    const DmaQueueEntry e_a = make_glyph_dma_entry(state, 'A');
    assert(e_a.src_word_addr == (0x00FF33C8U >> 1U));
    assert(e_a.dest_vram == 0x2000U + 32U); // cursor was 1 -> offset 32
    assert(e_a.word_count == 16U);
    assert(state.cursor_x == 2);

    // Switch to Line 1 -> plane_top_base + 0x400 = 0x2400
    state.line_counter = 1;
    state.cursor_x = 0;
    const DmaQueueEntry e_l1 = make_glyph_dma_entry(state, ' ');
    assert(e_l1.dest_vram == 0x2400U);

    // Switch to Line 2 -> plane_bottom_base = 0x3000
    state.line_counter = 2;
    state.cursor_x = 0;
    const DmaQueueEntry e_l2 = make_glyph_dma_entry(state, ' ');
    assert(e_l2.dest_vram == 0x3000U);

    // Switch to Line 3 -> plane_bottom_base + 0x400 = 0x3400
    state.line_counter = 3;
    state.cursor_x = 0;
    const DmaQueueEntry e_l3 = make_glyph_dma_entry(state, ' ');
    assert(e_l3.dest_vram == 0x3400U);

    std::cout << "[PASS] test_glyph_dma_generation\n";
}

static void test_blank_dma_entry() {
    constexpr DmaQueueEntry e_blank = make_blank_dma_entry(0x5600U);
    static_assert(e_blank.src_word_addr == 0x007F9DD4U);
    static_assert(e_blank.dest_vram == 0x5600U);
    static_assert(e_blank.word_count == 512U);

    assert(e_blank.src_word_addr == 0x007F9DD4U);
    assert(e_blank.dest_vram == 0x5600U);
    assert(e_blank.word_count == 512U);

    std::cout << "[PASS] test_blank_dma_entry\n";
}

static void test_ram_integration() {
    std::vector<std::uint8_t> ram(0x10000U, 0x00U);

    const WindowScrollState state{
        .plane_top_base = 0x1234U,
        .plane_bottom_base = 0x5678U,
        .row_offsets = {10, 22, 34, 46},
        .cursor_x = 7,
        .line_counter = 2
    };

    sync_scroll_state_to_ram(state, ram);

    constexpr std::size_t top_off = DialogueScrollDmaConfig::kPlaneTopBaseRam & 0xFFFFU;
    const std::uint16_t top_val = static_cast<std::uint16_t>((ram[top_off] << 8U) | ram[top_off + 1U]);
    assert(top_val == 0x1234U);

    constexpr std::size_t bot_off = DialogueScrollDmaConfig::kPlaneBottomBaseRam & 0xFFFFU;
    const std::uint16_t bot_val = static_cast<std::uint16_t>((ram[bot_off] << 8U) | ram[bot_off + 1U]);
    assert(bot_val == 0x5678U);

    constexpr std::size_t r0_off = DialogueScrollDmaConfig::kRowOffset0Ram & 0xFFFFU;
    const std::int16_t r0_val = static_cast<std::int16_t>((ram[r0_off] << 8U) | ram[r0_off + 1U]);
    assert(r0_val == 10);

    // Setup DMA queue pointer at $FF1892 -> points to $FF18A0
    constexpr std::size_t qptr_off = DialogueScrollDmaConfig::kDmaQueuePtrRam & 0xFFFFU;
    ram[qptr_off + 0U] = 0x00U;
    ram[qptr_off + 1U] = 0xFFU;
    ram[qptr_off + 2U] = 0x18U;
    ram[qptr_off + 3U] = 0xA0U;

    const DmaQueueEntry test_entry{
        .src_word_addr = 0x007F1234U,
        .dest_vram = 0x4567U,
        .word_count = 0x0010U
    };

    append_dma_entry_to_ram(ram, test_entry);

    // Verify 8 bytes written at 0x18A0
    assert(ram[0x18A0U] == 0x00U);
    assert(ram[0x18A1U] == 0x7FU);
    assert(ram[0x18A2U] == 0x12U);
    assert(ram[0x18A3U] == 0x34U);
    assert(ram[0x18A4U] == 0x45U);
    assert(ram[0x18A5U] == 0x67U);
    assert(ram[0x18A6U] == 0x00U);
    assert(ram[0x18A7U] == 0x10U);

    // Verify pointer incremented to $FF18A8
    assert(ram[qptr_off + 3U] == 0xA8U);

    // Verify VDP sync flag bit 1 was set
    constexpr std::size_t vdp_off = DialogueScrollDmaConfig::kVdpFlagsRam & 0xFFFFU;
    assert((ram[vdp_off] & 0x02U) != 0U);

    std::cout << "[PASS] test_ram_integration\n";
}

int main() {
    std::cout << "Starting M16.178 Dialogue Scroll & DMA tests...\n";
    test_constants_and_addresses();
    test_line_advance_without_scroll();
    test_circular_scroll_and_recycling();
    test_vdp_clear_line_records();
    test_glyph_dma_generation();
    test_blank_dma_entry();
    test_ram_integration();
    std::cout << "All M16.178 Dialogue Scroll & DMA tests PASS!\n";
    return 0;
}
