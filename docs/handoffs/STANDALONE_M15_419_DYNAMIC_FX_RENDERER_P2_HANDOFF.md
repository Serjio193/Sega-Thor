# Standalone Handoff M15.419: ROM Entity Special Dynamic FX Halo, Swirl & SAT Piece Compositor Subsystem (Part 2)

- **Milestone:** M15.419
- **ROM Source Range:** `0x00B56E..0x00B79A` (556 bytes)
- **SHA256:** `dd3b575d8d32eb02ea7aa50721a93ca72b8781fdf53bab5404d3ffa63ec73817`
- **Callers in ROM:**
  - `0x00B37E` (Master FX Dispatcher Bit 2 Halo)
  - `0x00B376` (Master FX Dispatcher Bit 1 Swirl)
  - `0x00B3CA` (Secondary Entity Attachment Pass)

---

## 1. Subsystem Responsibility

Part 2 of the dynamic FX rendering engine implements halo and energy swirl emitters along with the low-level SAT sprite piece blitters:
1. **Bit 2 Pulsing Halo Emitter (`0x00B56E..0x00B5CE`)**:
   - Decrements frame counter at `+0x9A(a6)` until 0, then clears bit 2 in `$39(a6)`.
   - Reads 21-entry offset table `0x00B5D0` (`dx`, `dy`).
   - Adds entity elevation `+0x10(a6)` to Y coordinate.
   - Invokes universal FX piece blitter `0x00B6A6`.
2. **Bit 1 Ground Energy Swirl Emitter (`0x00B610..0x00B65E`)**:
   - Only active when grounded (`elevation == 0`).
   - Advances counter at `+0x99(a6)` modulo 9.
   - Shifts Y position by `(preset >> 2) - 6` based on shadow preset `+0x3D(a6)`.
   - Reads 9-byte sequence table `0x00B660`.
3. **Universal FX Piece Blitter (`0x00B6A6..0x00B72E`)**:
   - Loads frame definitions from ROM `0x0003F326`.
   - Sets base VRAM tile `0x055E` with entity priority mask (`base_tile & 0x8000`).
   - Clips sprite pieces against viewport (`[0x60, 0x160]` Y, `[0x60, 0x1C0]` X).
   - Enforces 80-sprite link limit (`$50`).
4. **Static Raw Piece Blitter (`0x00B730..0x00B798`)**:
   - Direct piece blitter for pre-formatted raw sprite lists at `a0`.
   - Applies `+0x80` screen coordinate bias to X (`d1`) and Y (`d4`).

---

## 2. C++20 Interface Proposal

```cpp
#pragma once

#include <cstdint>
#include <array>

namespace thor::graphics {

class DynamicFxRendererP2 {
public:
    static constexpr uint16_t kFxVramBaseTile = 0x055E;
    static constexpr int16_t kViewportMinY = 0x60;
    static constexpr int16_t kViewportMaxY = 0x160;
    static constexpr int16_t kViewportMinX = 0x60;
    static constexpr int16_t kViewportMaxX = 0x1C0;

    static uint16_t ComputeFxTileAttr(uint16_t entity_tile_18) noexcept {
        return (entity_tile_18 & 0x8000) | kFxVramBaseTile;
    }

    static int16_t ComputeSwirlYOffset(uint8_t shadow_preset_3d) noexcept {
        return static_cast<int16_t>((shadow_preset_3d >> 2) - 6);
    }

    static bool IsClipped(int16_t screen_y, int16_t screen_x) noexcept {
        uint16_t y_rel = static_cast<uint16_t>(screen_y - kViewportMinY);
        uint16_t x_rel = static_cast<uint16_t>(screen_x - kViewportMinX);
        return (y_rel > 0x100) || (x_rel > 0x160);
    }
};

} // namespace thor::graphics
```
