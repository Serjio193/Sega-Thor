# STANDALONE HANDOFF: M15.444 — ROM Dynamic Partial Tilemap Plane VRAM Dirty Block Blitter, Viewport Bounds Clipper & Toroidal Stride Streamer

- **Source Routine:** `vdp_sync_dirty_plane_rect` (`0x00DC26..0x00DDA2`, 380 bytes).
- **Domain:** Tilemap VRAM Blitter / Viewport Bounds Clipping / Toroidal Plane Streamer.
- **Evidence Reference:** `data/thor-evidence/rom_dirty_plane_blitter_m15_444.json`.
- **Test Reference:** `tests/rom_dirty_plane_blitter_test.py`.

---

## 1. Architectural Role & Context

In Beyond Oasis / The Story of Thor, dynamic scenery changes (such as destructible walls, opening chests, water lock switches, interactive puzzles, and tilemap block updates performed by `0x00DBC0` `tilemap_copy_rect_word_and_sync`) require synchronizing modified rectangular regions of the system RAM tilemap into Mega Drive VDP VRAM.

Routine `0x00DC26` (`vdp_sync_dirty_plane_rect`) is the master blitter responsible for updating VRAM Plane A (`$C000`) or Plane B (`$E000`). It operates under two distinct modes depending on bit 1 of `+$27(a3)`:
1. **Viewport Clipped Mode (`bit 1 == 0`):**
   - Transforms map-relative tile coordinates `(d1, d2)` to camera-relative viewport coordinates using camera scroll offsets at `+$8(a3)` (X) and `+$A(a3)` (Y).
   - Performs two-sided horizontal clipping against the 41-tile viewport buffer (`move.w #$29, d0`). If the rectangle is fully to the left or right of the screen, the routine immediately exits with `rts`. If partially overlapping, it skips unrendered source columns (`a1 -= 2 * rel_x`) and clamps the transfer width `d3`.
   - Performs two-sided vertical clipping against the 29-tile viewport buffer (`move.w #$1d, d0`). If the rectangle is above or below the screen, it exits with `rts`. If partially overlapping, it skips unrendered source rows (`a1 -= rel_y * (1 << stride_shift)`) and clamps the transfer height `d4`.
   - Combines sub-tile scroll offset `+$C(a3)` with clipped screen coordinates to determine the destination VRAM toroidal cell `(d5, d6)`.
2. **Unclipped Toroidal Mode (`bit 1 == 1`, jump to `0x00DD14`):**
   - Bypasses screen boundary clipping entirely.
   - Directly wraps tile coordinates into 64x32 plane cells: `col = (x - cam_x) & 0x3F`, `row = (y - cam_y) & 0x1F`.

During transfer, for both modes:
- Interrupts are disabled (`ori.w #$700, sr`) to safeguard atomic VDP port streaming.
- For each row, the VDP VRAM write command is emitted to `$C00004` (`(row << 7) + (col << 1) + base_vram_cmd`).
- Tiles are streamed word-by-word to `$C00000` via `move.w (a1)+, (a2)`.
- When column `d5` reaches 64 (`cmpi.w #$40, d5`), horizontal toroidal wrap occurs: `d5 -= 64` and the VRAM write address is reissued to wrap seamlessly around the Mega Drive hardware plane boundary.
- When row `d6` reaches 32 (`cmpi.w #$20, d6`), vertical toroidal wrap occurs: `d6 -= 32`.
- After all rows finish, interrupts are restored (`andi.w #$f9ff, sr`) and the function returns.

---

## 2. Register Conventions

| Register | Direction | Semantic Meaning |
|---|---|---|
| `a1` | In / Modified | Pointer to source tile descriptor buffer (words: pattern index + palette + priority + flip) |
| `a2` | Scratch | VDP Data Port pointer (`$C00000`) |
| `a3` | In | Pointer to Plane Context struct (`$FF1716` Plane A, `$FF173E` Plane B) |
| `a4` | Scratch | VDP Control Port pointer (`$C00004`) |
| `d0` | Scratch | Temporary math register (clipping bounds, VRAM address calculation) |
| `d1` | In / Scratch | Target map tile X coordinate / VDP command low word |
| `d2` | In / Scratch | Target map tile Y coordinate / VDP command high word |
| `d3` | In / Scratch | Transfer width in tiles minus 1 (column count) |
| `d4` | In / Scratch | Transfer height in tiles minus 1 (row count) |
| `d5` | Scratch | VRAM toroidal column index (`0..63`) |
| `d6` | Scratch | VRAM toroidal row index (`0..31`) |
| `d7` | Scratch | Source buffer row stride in bytes (`2 * map_stride`) |

---

## 3. C++20 Cleanroom Interface Design

```cpp
#pragma once

#include <cstdint>
#include <span>
#include <algorithm>

namespace thor::vdp {

struct PlaneContext {
    int16_t cam_x{0};           // +$08(a3): Camera tile X
    int16_t cam_y{0};           // +$0A(a3): Camera tile Y
    uint16_t cam_sub_scroll{0}; // +$0C(a3): Sub-tile scroll
    uint16_t vram_base{0xC000}; // +$1A(a3): $C000 (Plane A) or $E000 (Plane B)
    uint16_t map_stride{64};    // +$20(a3): Stride in tiles
    uint16_t stride_shift{7};   // +$24(a3): 1 << shift == stride in bytes
    uint8_t flags{0};           // +$27(a3): bit 0: active, bit 1: unclipped
};

struct DirtyRectTransferResult {
    bool culled{false};
    int16_t clipped_rel_x{0};
    int16_t clipped_rel_y{0};
    int16_t clipped_width{0};
    int16_t clipped_height{0};
    size_t source_byte_offset{0};
};

inline DirtyRectTransferResult clip_dirty_rect(
    const PlaneContext& ctx,
    int16_t target_x,
    int16_t target_y,
    int16_t width,
    int16_t height
) {
    DirtyRectTransferResult res{};
    int16_t rel_x = target_x - ctx.cam_x;
    size_t byte_offset = 0;

    if (rel_x < 0) {
        width += rel_x;
        if (width < 0) {
            res.culled = true;
            return res;
        }
        byte_offset += static_cast<size_t>(-rel_x) * 2;
        rel_x = 0;
    }

    constexpr int16_t max_viewport_cols = 41; // #$29
    int16_t rem_w = max_viewport_cols - rel_x;
    if (rem_w < 0) {
        res.culled = true;
        return res;
    }
    width = std::min(width, rem_w);

    int16_t rel_y = target_y - ctx.cam_y;
    if (rel_y < 0) {
        height += rel_y;
        if (height < 0) {
            res.culled = true;
            return res;
        }
        byte_offset += static_cast<size_t>(-rel_y) * (1 << ctx.stride_shift);
        rel_y = 0;
    }

    constexpr int16_t max_viewport_rows = 29; // #$1D
    int16_t rem_h = max_viewport_rows - rel_y;
    if (rem_h < 0) {
        res.culled = true;
        return res;
    }
    height = std::min(height, rem_h);

    res.clipped_rel_x = rel_x;
    res.clipped_rel_y = rel_y;
    res.clipped_width = width;
    res.clipped_height = height;
    res.source_byte_offset = byte_offset;
    return res;
}

} // namespace thor::vdp
```
