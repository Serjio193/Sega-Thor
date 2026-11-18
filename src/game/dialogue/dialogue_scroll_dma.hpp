#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace oasis::game::dialogue {

/// Sega Genesis VDP DMA Queue Entry (8 bytes).
struct DmaQueueEntry {
    std::uint32_t src_word_addr{0U}; // [0..3]: Source 68K word address (byte_addr >> 1)
    std::uint16_t dest_vram{0U};      // [4..5]: Target VRAM destination address
    std::uint16_t word_count{0U};     // [6..7]: Transfer word count
};

/// Record for direct VDP line clearing during smooth scroll step (0x00C66A).
struct VdpClearLineRecord {
    std::uint16_t dest_vram{0U};
    std::uint16_t line_offset{0U};
    std::uint16_t row_count{4U};
    std::uint16_t words_per_row{32U};
};

/// Configuration and constants for ROM Dialogue Window 4-Line Circular Scroll Engine,
/// VDP Direct Clearer & Glyph DMA Streamer (0x00C566..0x00C756, 496 bytes).
struct DialogueScrollDmaConfig {
    static constexpr std::uint32_t kEntryAdvanceLineAddr = 0x00C566U;
    static constexpr std::uint32_t kEntryVdpClearLineAddr = 0x00C66AU;
    static constexpr std::uint32_t kEntryRenderGlyphDmaAddr = 0x00C6BEU;
    static constexpr std::uint32_t kEntryQueueBlankDmaAddr = 0x00C734U;
    static constexpr std::uint32_t kRangeEndAddr = 0x00C756U;
    static constexpr std::size_t kRoutineSizeBytes = 496U;

    // Line offsets & scroll constants
    static constexpr std::array<std::int16_t, 4> kInitRowOffsets = {12, 24, 36, 48};
    static constexpr std::int16_t kRowRecycleValue = 48; // 0x30
    static constexpr std::int16_t kScrollStepDecrement = 2;
    static constexpr std::size_t kLineRowStrideBytes = 1024U; // 0x400
    static constexpr std::size_t kGlyphTileSizeBytes = 32U;
    static constexpr std::size_t kDmaQueueEntrySizeBytes = 8U;

    // Blank buffer DMA
    static constexpr std::uint32_t kBlankRomSourceWordAddr = 0x007F9DD4U;
    static constexpr std::uint16_t kBlankDmaWordCount = 512U; // 0x0200 words = 1024 bytes
    static constexpr std::uint16_t kGlyphDmaWordCount = 16U;   // 0x0010 words = 32 bytes

    // RAM Addresses
    static constexpr std::uint32_t kPlaneTopBaseRam = 0x00FF1872U;
    static constexpr std::uint32_t kPlaneBottomBaseRam = 0x00FF1874U;
    static constexpr std::uint32_t kRowOffset0Ram = 0x00FF1876U;
    static constexpr std::uint32_t kRowOffset1Ram = 0x00FF1878U;
    static constexpr std::uint32_t kRowOffset2Ram = 0x00FF187AU;
    static constexpr std::uint32_t kRowOffset3Ram = 0x00FF187CU;
    static constexpr std::uint32_t kCursorXRam = 0x00FF187EU;
    static constexpr std::uint32_t kLineCounterRam = 0x00FF1880U;
    static constexpr std::uint32_t kDmaQueuePtrRam = 0x00FF1892U;
    static constexpr std::uint32_t kVdpFlagsRam = 0x00FF164EU;
    static constexpr std::uint32_t kGlyphBufferBaseRam = 0x00FF2FA8U;
};

/// Models state of the 4-line circular scrolling message window.
struct WindowScrollState {
    std::uint16_t plane_top_base{0U};
    std::uint16_t plane_bottom_base{0U};
    std::array<std::int16_t, 4> row_offsets{DialogueScrollDmaConfig::kInitRowOffsets};
    std::int16_t cursor_x{0};
    std::int16_t line_counter{0};
};

/// Direct VDP line clearing calculation helper (0x00C66A).
[[nodiscard]] bool calculate_vdp_clear_line(
    std::uint16_t vram_base,
    std::uint16_t line_offset,
    VdpClearLineRecord& out_record) noexcept;

/// Line advance and 4-line circular scroll routine (0x00C566).
/// Resets cursor_x to 0, increments line_counter.
/// If line_counter <= 3, returns false (no scroll performed).
/// If line_counter > 3: performs smooth scroll decrements by 2 until a row reaches 0,
/// recycles that row to 48 (0x30), records intermediate VDP clear lines, and appends
/// blank DMA transfer to out_dma_queue. Returns true (scroll was performed).
bool advance_line_and_scroll(
    WindowScrollState& state,
    std::vector<DmaQueueEntry>& out_dma_queue,
    std::vector<VdpClearLineRecord>* out_vdp_clears = nullptr) noexcept;

/// Builds DMA transfer entry for 8x8 4bpp glyph character into active line VRAM buffer (0x00C6BE).
/// Increments cursor_x.
[[nodiscard]] DmaQueueEntry make_glyph_dma_entry(
    WindowScrollState& state,
    std::uint8_t char_code) noexcept;

/// Builds blank buffer DMA transfer entry to clear 1024-byte VRAM line buffer (0x00C734).
[[nodiscard]] constexpr DmaQueueEntry make_blank_dma_entry(std::uint16_t dest_vram) noexcept {
    return DmaQueueEntry{
        .src_word_addr = DialogueScrollDmaConfig::kBlankRomSourceWordAddr,
        .dest_vram = dest_vram,
        .word_count = DialogueScrollDmaConfig::kBlankDmaWordCount
    };
}

/// Synchronizes WindowScrollState to a 64KB Genesis RAM window.
void sync_scroll_state_to_ram(
    const WindowScrollState& state,
    std::span<std::uint8_t> ram_window);

/// Appends a DMA queue entry to RAM at $FF1892 and sets $FF164E bit 1.
void append_dma_entry_to_ram(
    std::span<std::uint8_t> ram_window,
    const DmaQueueEntry& entry);

} // namespace oasis::game::dialogue
