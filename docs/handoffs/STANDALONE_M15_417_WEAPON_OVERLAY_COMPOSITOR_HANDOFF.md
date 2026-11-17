# Standalone Handoff M15.417: ROM Entity Secondary Weapon & Auxiliary Overlay SAT Compositor

- **Milestone:** M15.417
- **ROM Source Range:** `0x00B15E..0x00B34C` (494 bytes)
- **SHA256:** `ef4ad599d3d45a5780ce78608267f9c3ecd01ea052a5b24b8bf7a400d327f675`
- **Callers in ROM:**
  - `0x00AB7A`, `0x00AC74` (M15.415 Master Entity SAT Compositor)
  - `0x00AEFA`, `0x00AF8C` (M15.416 Static Entity SAT Compositor)

---

## 1. Subsystem Responsibility

The weapon and auxiliary overlay compositor blits equipped weapons (swords, daggers, bows) and entity attachments (spirit bonds, shields, special overlays) onto the character's hardware sprite layout:
1. **Mode Routing (`0x00B15E`)**:
   - Tests bit 9 of flash/mode counter `d5` (`+0x3E(a6)`).
   - If bit 9 is clear (0) -> **DMA Overlay Path (`0x00B166..0x00B28C`)**: streams weapon graphics into VRAM when frames change, caching the active frame in `+0x5C(a6)`.
   - If bit 9 is set (1) -> **Non-DMA Overlay Path (`0x00B28E..0x00B34A`)**: for static / pre-allocated weapon tiles.
2. **Descriptor Lookup (`0x00B1AA`)**:
   - Resolves entity descriptor entry at `0x0003F2FA + (entity_type * 16) - 20`.
   - Reads auxiliary VRAM tile base from `+0x5A(a6)`.
3. **Screen Viewport Clipping & Linking**:
   - Clips each sprite piece against `[0x60, 0x160]` for Y and `[0x60, 0x1C0]` for X.
   - Enforces 80-sprite link maximum (`0x50`).
   - Looks up tile counts from 4x4 table `0x00AD56` and writes to DMA queue buffer `$FF1892`.

---

## 2. C++20 Interface Proposal

```cpp
#pragma once

#include <cstdint>
#include <array>

namespace thor::graphics {

struct WeaponOverlayPiece {
    int16_t y_offset{0};
    uint16_t size_flags{0};
    uint16_t tile_attr{0};
    int16_t x_offset{0};
};

class WeaponOverlayCompositor {
public:
    static constexpr int16_t kViewportMinY = 0x60;
    static constexpr int16_t kViewportMaxY = 0x160;
    static constexpr int16_t kViewportMinX = 0x60;
    static constexpr int16_t kViewportMaxX = 0x1C0;
    static constexpr uint8_t kMaxSprites = 80;

    static bool RequiresDma(uint16_t mode_word) noexcept {
        return (mode_word & 0x0200) == 0;
    }

    static uint32_t GetDescriptorOffset(uint16_t entity_type) noexcept {
        return 0x0003F2FA + (entity_type * 16) - 20;
    }

    static bool IsClipped(int16_t y, int16_t x) noexcept {
        uint16_t y_rel = static_cast<uint16_t>(y - kViewportMinY);
        uint16_t x_rel = static_cast<uint16_t>(x - kViewportMinX);
        return (y_rel > 0x100) || (x_rel > 0x160);
    }
};

} // namespace thor::graphics
```
