# STANDALONE HANDOFF: M15.429 — ROM Dialogue Window 4-Line Circular Scroll Engine, VDP Direct Clearer & Glyph DMA Streamer

## 1. Routine Overview
- **Addresses:** `0x00C566..0x00C756` (496 bytes)
- **Subroutines:**
  - `0x00C566`: `dialogue_advance_line_and_scroll` (Resets X cursor, increments line counter; scrolls 4-line window if full)
  - `0x00C66A`: `dialogue_vdp_clear_line` (Direct VDP register/data write clearing 32 words with autoincrement 0x20)
  - `0x00C6BE`: `dialogue_render_glyph_dma` (Builds 16-word DMA transfer entry for 8x8 4bpp tile to destination VRAM)
  - `0x00C734`: `dialogue_queue_blank_dma` (Queues 512-word DMA transfer from ROM blank buffer `$007F9DD4` to VRAM)

## 2. Line Advance & 4-Line Circular Window Scroll (`0x00C566`)
- Resets character X cursor: `$FF187E = 0`.
- Increments line counter: `++$FF1880`.
- If `line_counter <= 3`: returns immediately (`rts`). No scrolling required.
- If `line_counter > 3`:
  - Enters scroll loop: decrements all 4 row offsets (`$FF1876`, `$FF1878`, `$FF187A`, `$FF187C`) by 2 each frame.
  - Detects row offset reaching 0:
    - If `$FF1876 == 0`: row 0 rolled off top. Queues blank transfer to `$FF1872`, recycles row offset to `0x30` (48).
    - If `$FF1878 == 0`: row 1 rolled off top. Queues blank transfer to `$FF1872 + 0x400`, recycles to `0x30`.
    - If `$FF187A == 0`: row 2 rolled off top. Queues blank transfer to `$FF1874`, recycles to `0x30`.
    - If `$FF187C == 0`: row 3 rolled off top. Queues blank transfer to `$FF1874 + 0x400`, recycles to `0x30`.

## 3. Glyph DMA Queue Construction (`0x00C6BE`)
- Glyph buffer index: `(char_code - 0x20) * 32`.
- Source bus word address: `(0xFF2FA8 + index) >> 1`.
- Target VRAM destination:
  - Active line index = `line_counter & 3`.
  - Line 0: `$FF1872 + (cursor_x << 5)`
  - Line 1: `$FF1872 + 0x400 + (cursor_x << 5)`
  - Line 2: `$FF1874 + (cursor_x << 5)`
  - Line 3: `$FF1874 + 0x400 + (cursor_x << 5)`
- Appends 8-byte entry to DMA table `$FF1892`:
  - 4 bytes: Source word address
  - 2 bytes: VRAM destination address
  - 2 bytes: Transfer word count (`0x0010` = 16 words = 32 bytes)
- Sets DMA request flag: `bset #1, $FF164E`.
- Increments cursor X: `++$FF187E`.

## 4. Cleanroom C++20 Interface
```cpp
namespace thor::dialogue {

struct DmaQueueEntry {
    uint32_t src_word_addr{0};
    uint16_t dest_vram{0};
    uint16_t word_count{0};
};

struct WindowScrollState {
    uint16_t plane_top_base{0};
    uint16_t plane_bottom_base{0};
    std::array<int16_t, 4> row_offsets{12, 24, 36, 48};
    int16_t cursor_x{0};
    int16_t line_counter{0};
};

bool advance_line_and_scroll(WindowScrollState& state, std::vector<uint16_t>& out_cleared_vram_bases);
DmaQueueEntry make_glyph_dma_entry(WindowScrollState& state, uint8_t char_code);
DmaQueueEntry make_blank_dma_entry(uint16_t dest_vram);

} // namespace thor::dialogue
```
