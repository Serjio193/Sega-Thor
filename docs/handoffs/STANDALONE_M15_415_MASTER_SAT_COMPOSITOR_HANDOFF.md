# Standalone Handoff M15.415: ROM Master Entity SAT Sprite Compositor, Screen Clipping & Dynamic Shadow Blitter Subsystem

- **Milestone:** M15.415
- **ROM Source Range:** `0x00A9A0..0x00AD76` (982 bytes)
- **SHA256:** `6da5db62165ef796b301e97ba763db217d7796faa01f9010cff1a0d641b11b6b`
- **Callers in ROM:**
  - `0x00A244`, `0x00A2DE`, `0x00A2F8`, `0x00A312` (M15.410 Master Sprite Composition)
  - `0x00A5A2`, `0x00A63C`, `0x00A656`, `0x00A670` (M15.412 Pass B Master SAT Dispatcher)
  - `0x00A738`, `0x00A79C`, `0x00A7B6`, `0x00A7D0` (M15.413 Shadow & Special Effects SAT Pass)

---

## 1. Subsystem Responsibility

The master sprite attribute table (SAT) compositor is the central routine responsible for:
1. Validating entity visibility and animation frame cache (`$34(a6)`).
2. Deferring frame changes if VRAM DMA tile allocation quota (`$FF1652.w`) is exceeded.
3. Transforming 3D world coordinates (`X`, `Y`, elevation `Z`) to 2D screen coordinates with camera plane scrolling (`$FF1716` or `$FF173E`) and screen jitter (`$FF1971`).
4. Iterating over entity hardware sprite pieces, computing VRAM tile attributes, and applying Mega Drive screen viewport clipping (`[0x60, 0x160]` for Y, `[0x60, 0x1C0]` for X).
5. Accumulating sprite tile dimensions using the embedded 4x4 tile count table (`0x00AD56`).
6. Enqueuing DMA transfer requests when frames transition.
7. Generating dynamic shadow sprites using the embedded shadow parameter table (`0x00ACFE`).
8. Updating SAT buffer offsets (`$FF188C`), sprite link counters (`$FF188A`), and queue pointers (`$FF1892`).

---

## 2. Embedded Tables

### 2.1 Dynamic Shadow Parameter Table (`0x00ACFE..0x00AD1E`, 32 bytes)
Indexed by preset index (`d3 & 0x18`):
| Preset | Y Offset | Size Word | VRAM Tile | X Offset | Description |
|:------:|:--------:|:---------:|:---------:|:--------:|:------------|
| 0 | 3 | `0x0000` (1x1) | `0x0548` | 3 | Small entity shadow |
| 1 | 3 | `0x0400` (2x1) | `0x0549` | 8 | Medium entity shadow |
| 2 | 3 | `0x0800` (3x1) | `0x054B` | 11 | Large entity shadow |
| 3 | 8 | `0x0D00` (4x2) | `0x054E` | 16 | Giant entity/boss shadow |

### 2.2 4x4 Sprite Tile Count Table (`0x00AD56..0x00AD76`, 32 bytes)
Indexed by `(width - 1) * 4 + (height - 1)` (derived from SAT size field `(size & 0x0F00) >> 7`):
- `1x1` = 1 tile, `1x2` = 2, `1x3` = 3, `1x4` = 4
- `2x1` = 2 tiles, `2x2` = 4, `2x3` = 6, `2x4` = 8
- `3x1` = 3 tiles, `3x2` = 6, `3x3` = 9, `3x4` = 12
- `4x1` = 4 tiles, `4x2` = 8, `4x3` = 12, `4x4` = 16

---

## 3. C++20 Interface Proposal

```cpp
#pragma once

#include <cstdint>
#include <array>
#include <vector>

namespace thor::graphics {

struct SpritePiece {
    int16_t y_offset{0};
    uint16_t size_flags{0};
    uint16_t tile_attr{0};
    int16_t x_offset{0};
};

struct SatEntry {
    int16_t y{0};
    uint8_t link{0};
    uint8_t size{0};
    uint16_t tile_attr{0};
    int16_t x{0};
};

struct DmaQueueEntry {
    uint32_t src_addr{0};
    uint16_t vram_dst{0};
    uint16_t tile_count{0};
};

struct ShadowEntry {
    uint16_t y_offset{0};
    uint16_t size{0};
    uint16_t tile_index{0};
    uint16_t x_offset{0};
};

constexpr std::array<ShadowEntry, 4> kShadowTable = {{
    {3, 0x0000, 0x0548, 3},
    {3, 0x0400, 0x0549, 8},
    {3, 0x0800, 0x054B, 11},
    {8, 0x0D00, 0x054E, 16}
}};

constexpr std::array<uint16_t, 16> kSpriteTileCounts = {
    1, 2, 3, 4,
    2, 4, 6, 8,
    3, 6, 9, 12,
    4, 8, 12, 16
};

class MasterSatCompositor {
public:
    static bool IsClippedY(int16_t screen_y) noexcept {
        uint16_t rel = static_cast<uint16_t>(screen_y - 0x60);
        return rel > 0x100;
    }

    static bool IsClippedX(int16_t screen_x) noexcept {
        uint16_t rel = static_cast<uint16_t>(screen_x - 0x60);
        return rel > 0x160;
    }

    static uint16_t GetTileCount(uint8_t size_bits) noexcept {
        uint8_t idx = (size_bits >> 8) & 0x0F;
        return (idx < 16) ? kSpriteTileCounts[idx] : 1;
    }
};

} // namespace thor::graphics
```
