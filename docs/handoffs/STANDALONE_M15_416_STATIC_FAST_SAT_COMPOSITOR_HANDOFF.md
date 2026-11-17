# Standalone Handoff M15.416: ROM Static Entity & Shadowless Fast SAT Sprite Compositors

- **Milestone:** M15.416
- **ROM Source Range:** `0x00AD76..0x00B15E` (1000 bytes)
- **SHA256:** `f856c327cbb7bdb790f996ca9e6b2838913929bea6989ac536339b56069a771e`
- **Callers in ROM:**
  - `0x00A240`, `0x00A2DA`, `0x00A2F4`, `0x00A30E` (Pass A Master Sprite Dispatcher)
  - `0x00A59E`, `0x00A638`, `0x00A652`, `0x00A66C` (Pass B Master SAT Dispatcher)
  - `0x00A734`, `0x00A798`, `0x00A7B2`, `0x00A7CC` (Shadow & Special Effects SAT Pass)
  - `0x0135FA` (Direct caller to Shadowless Fast Compositor `0x00B022`)

---

## 1. Subsystem Architecture

The Beyond Oasis graphics pipeline partitions entity SAT composition into three tiers based on hardware overhead:
1. **Dynamic DMA Compositor (`0x00A9A0`)**: For entities requiring dynamic VRAM tile streaming (e.g. Ali, large bosses). Tracks remaining VRAM tile quota (`$FF1652`), queues DMA requests to `$FF1892`, and accumulates tile counts.
2. **Static Entity Compositor (`0x00AD76`)**: For standard entities with pre-loaded or fixed VRAM tile sets. Skips DMA queueing, checks elevation Z (`+0x10`), clips to screen viewport (`[0x60, 0x160]` Y, `[0x60, 0x1C0]` X), and generates dynamic shadows via table `0x00ACFE`.
3. **Shadowless Fast Compositor (`0x00B022`)**: Minimalist sprite emitter for background objects, pickups, and simple visual effects. Zero stack frame overhead, no shadow generation, fast viewport clipping.

### Dispatch Mode Selection
In the SAT passes, bit 0 of `$36(a6)` decides the engine tier:
```m68k
btst.b   #$0, $36(a6)
beq.w    $ad76        ; 0 = Static Entity Compositor (0x00AD76)
bra.w    $a9a0        ; 1 = Dynamic DMA Compositor (0x00A9A0)
```

---

## 2. C++20 Interface Proposal

```cpp
#pragma once

#include <cstdint>
#include <array>
#include <span>

namespace thor::graphics {

struct FastSpritePiece {
    int16_t y_offset{0};
    uint16_t size_flags{0};
    uint16_t tile_attr{0};
    int16_t x_offset{0};
};

class StaticSatCompositor {
public:
    static constexpr int16_t kViewportMinY = 0x60;
    static constexpr int16_t kViewportMaxY = 0x160;
    static constexpr int16_t kViewportMinX = 0x60;
    static constexpr int16_t kViewportMaxX = 0x1C0;

    static bool IsVisible(uint8_t flags_36) noexcept {
        return (flags_36 & 0x02) == 0;
    }

    static bool IsDynamicDma(uint8_t flags_36) noexcept {
        return (flags_36 & 0x01) != 0;
    }

    static bool ClipY(int16_t y) noexcept {
        uint16_t rel = static_cast<uint16_t>(y - kViewportMinY);
        return rel > 0x100;
    }

    static bool ClipX(int16_t x) noexcept {
        uint16_t rel = static_cast<uint16_t>(x - kViewportMinX);
        return rel > 0x160;
    }
};

} // namespace thor::graphics
```
