# Standalone Handoff: M15.443 Tilemap Rectangle Fill & Block Copy Subsystem

## Routine Overview
- **Source ROM:** `local-roms/Beyond Oasis (USA).md`
- **ROM Address Range:** `0x00DB1A..0x00DC26` (268 bytes, 82 instructions)
- **Primary Subroutines:**
  - `0x00DB1A`: `tilemap_fill_rect_byte` (fills rectangular region in Plane A/B auxiliary byte map with byte value `d0`, returns via `rts` at `0x00DB62`).
  - `0x00DB64`: `tilemap_copy_rect_word_and_sync` (composite rectangular block transfer: copies auxiliary byte map rectangle, copies word tilemap rectangle in RAM, and triggers VRAM upload `0x00DC26`, returns via `rts` at `0x00DBC8`).
  - `0x00DBCA`: `tilemap_copy_rect_byte` (copies rectangular region in Plane A/B auxiliary byte map from source `(a0)` to destination `(a1)`, returns via `rts` at `0x00DC24`).
- **Callees:**
  - `0x00DBCA`: `tilemap_copy_rect_byte` (called by `0x00DB64`).
  - `0x00DC26`: `vdp_sync_dirty_plane_rect` (called by `0x00DB64` to stream modified tile cells to VRAM).

---

## Memory & Register Interface

| Address / Register | Type | Direction | Description |
|--------------------|------|-----------|-------------|
| `a3` (register)    | Struct ptr | In | Pointer to Plane context record (`$FF1716` Plane A, `$FF173E` Plane B) |
| `$FF1766`          | Struct ptr | In | Plane A auxiliary context record (`$0`: data ptr, `$4`: width, `$8`: shift) |
| `$FF1770`          | Struct ptr | In | Plane B auxiliary context record (`$0`: data ptr, `$4`: width, `$8`: shift) |
| `$FF16F0` bit 7    | Flag | In | Selects Plane A (`0`) vs Plane B (`1`) in `tilemap_fill_rect_byte` |
| `d0`               | `uint8_t`  | In | Fill value for `tilemap_fill_rect_byte` |
| `d1`               | `int16_t`  | In | Upper-left X coordinate |
| `d2`               | `int16_t`  | In | Upper-left Y coordinate |
| `d3`               | `int16_t`  | In | Lower-right X coordinate (becomes `width = x2 - x1`) |
| `d4`               | `int16_t`  | In | Lower-right Y coordinate (becomes `height = y2 - y1`) |
| `a0`               | `const void*` | In | Source coordinate stream or source data buffer |
| `a1`               | `void*`    | In/Out | Destination coordinate stream or destination data buffer |

---

## C++20 Interface Proposal

```cpp
#pragma once

#include <cstdint>
#include <span>

namespace thor::tilemap {

struct RectCoords {
    int16_t x1{0};
    int16_t y1{0};
    int16_t x2{0};
    int16_t y2{0};
};

class TilemapRectangularOps {
public:
    static void FillRectByte(
        std::span<uint8_t> aux_map,
        uint8_t row_shift,
        uint16_t row_stride,
        const RectCoords& rect,
        uint8_t fill_val
    ) {
        int16_t width = rect.x2 - rect.x1;
        int16_t height = rect.y2 - rect.y1;
        if (width < 0 || height < 0) return;

        std::size_t offset = (static_cast<std::size_t>(rect.y1) << row_shift) + rect.x1;
        for (int16_t row = 0; row <= height; ++row) {
            for (int16_t col = 0; col <= width; ++col) {
                aux_map[offset + col] = fill_val;
            }
            offset += row_stride;
        }
    }

    static void CopyRectWord(
        std::span<uint16_t> tilemap,
        uint8_t row_shift,
        uint16_t plane_width_tiles,
        const RectCoords& src_rect,
        int16_t dst_x,
        int16_t dst_y
    ) {
        int16_t width = src_rect.x2 - src_rect.x1;
        int16_t height = src_rect.y2 - src_rect.y1;
        if (width < 0 || height < 0) return;

        std::size_t src_offset = (static_cast<std::size_t>(src_rect.y1) << row_shift) + src_rect.x1;
        std::size_t dst_offset = (static_cast<std::size_t>(dst_y) << row_shift) + dst_x;

        for (int16_t row = 0; row <= height; ++row) {
            for (int16_t col = 0; col <= width; ++col) {
                tilemap[dst_offset + col] = tilemap[src_offset + col];
            }
            src_offset += plane_width_tiles;
            dst_offset += plane_width_tiles;
        }
    }
};

} // namespace thor::tilemap
```
