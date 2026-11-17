# STANDALONE HANDOFF: M15.437 — ROM VDP Dual-Plane H/V Scroll Table Builder, Scanline/Strip Mode Dispatcher & V-Int DMA Sync Trigger

## 1. Routine Overview
- **Addresses:** `0x00D2B4..0x00D3B2` (254 bytes)
- **Primary Entrypoints:**
  - `0x00D2B4`: `vdp_update_hscroll_table` (Updates Plane A/B horizontal scroll registers or table entries depending on VDP mode)
  - `0x00D344`: `vdp_update_vscroll_table` (Updates Plane A/B vertical scroll registers or 2-cell table entries depending on VDP mode)
  - `0x00D398`: `vdp_commit_scroll_dma_flags` (Transfers pending scroll DMA flags from `$FF16F0` into V-Int sync register `$FF164E`)

## 2. Horizontal Scroll Table Generation (`0x00D2B4..0x00D342`)
- Clears dirty bit 2 on `+0x26(a0)` (`bclr.b #2, +0x26(a0)`). Returns if not dirty.
- Sets pending bit 3 on `$FF16F0` (`bset.b #3, $FF16F0`).
- Value: `d0 = +0x0E(a0)` (horizontal scroll word).
- Mode dispatch on `$FF16F0`:
  - **Full-Screen Mode (`bit 1 == 0`):**
    - Writes `d0` into single word at `$FF1344` (Plane A) or `$FF1346` (Plane B if `a0 != $FF1716`).
  - **16-Pixel Tile Strip Mode (`bit 1 == 1, bit 0 == 0`):**
    - Target: `$FFFB5C` (HScroll table in RAM, offset +0 for Plane A, +2 for Plane B).
    - Writes `d0` across 29 entries at stride 32 bytes (16 words): 28..0 loops.
  - **1-Line Scanline Parallax Mode (`bit 1 == 1, bit 0 == 1`):**
    - Target: `$FFFB5C`.
    - Writes `d0` across 232 scanlines (58 loops of 4 words, stride 4 bytes / 2 words).

## 3. Vertical Scroll Table Generation (`0x00D344..0x00D396`)
- Clears dirty bit 3 on `+0x26(a0)` (`bclr.b #3, +0x26(a0)`). Returns if not dirty.
- Sets pending bit 4 on `$FF16F0` (`bset.b #4, $FF16F0`).
- Value: `d0 = +0x10(a0)` (vertical scroll word).
- Mode dispatch on `$FF16F0`:
  - **Full-Screen Mode (`bit 2 == 0`):**
    - Writes `d0` into single word at `$FF1348` (Plane A) or `$FF134A` (Plane B if `a0 != $FF1716`).
  - **2-Cell Column Mode (`bit 2 == 1`):**
    - Target: `$FFFF5C` (VScroll table in RAM, offset +0 for Plane A, +2 for Plane B).
    - Writes `d0` across 20 column entries at stride 4 bytes (2 words).

## 4. V-Int DMA Sync Commit (`0x00D398..0x00D3B2`)
- Reads pending bits from `$FF16F0` (`bits 3 & 4`, bitmask `0x18`).
- Clears pending bits from `$FF16F0` (`$FF16F0 &= ~0x18`).
- Asserts flags into V-Int sync register: `$FF164E |= pending_flags`.

## 5. Cleanroom C++20 Interface
```cpp
namespace thor::render {

enum class HScrollMode : uint8_t {
    FullScreen = 0,
    TileStrip16px = 1,
    ScanlineParallax = 2,
};

enum class VScrollMode : uint8_t {
    FullScreen = 0,
    TwoCellColumn16px = 1,
};

struct VdpScrollConfig {
    HScrollMode h_mode{HScrollMode::FullScreen};
    VScrollMode v_mode{VScrollMode::FullScreen};
};

struct VdpScrollBuffers {
    uint16_t fullscreen_hscroll_plane_a{0};
    uint16_t fullscreen_hscroll_plane_b{0};
    uint16_t fullscreen_vscroll_plane_a{0};
    uint16_t fullscreen_vscroll_plane_b{0};
    std::array<uint16_t, 512> hscroll_table{};
    std::array<uint16_t, 40> vscroll_table{};
};

void update_vdp_scroll_tables(
    VdpScrollBuffers& buffers,
    const VdpScrollConfig& config,
    bool is_plane_a,
    uint16_t hscroll_val,
    uint16_t vscroll_val,
    bool h_dirty,
    bool v_dirty,
    uint8_t& vint_sync_flags);

} // namespace thor::render
```
