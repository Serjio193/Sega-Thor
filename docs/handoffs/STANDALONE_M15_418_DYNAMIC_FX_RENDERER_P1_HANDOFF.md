# Standalone Handoff M15.418: ROM Entity Special Dynamic FX, Aura, Wave & Attachment SAT Renderer (Part 1)

- **Milestone:** M15.418
- **ROM Source Range:** `0x00B34C..0x00B56E` (546 bytes)
- **SHA256:** `780c99c9f67ca0d5461fdf066131faa2c10307a715e0a64f19247feaf97bdcee`
- **Callers in ROM:**
  - `0x00AB66` (Master Entity SAT Compositor)
  - `0x00AC80` (Master Entity SAT Compositor Tail)
  - `0x00AF98` (Static Entity SAT Compositor)

---

## 1. Subsystem Responsibility

The dynamic FX rendering engine manages special visual effects, auras, energy attachments, and ground ripples rendered around entities:
1. **Master FX Dispatcher (`0x00B34C..0x00B382`)**:
   - Checks global shadow/FX suppression flag `$FF1927.l`.
   - Reads entity FX mode flags byte `$39(a6)`:
     * Bit 0: Aura / spark emitter (`0x00B3D2`)
     * Bit 3: Shadow / wave ring emitter (`0x00B45C`)
     * Bit 4: Ground ripple / flame attachment (`0x00B4FE`)
     * Bit 1: Energy swirl (`0x00B610`)
     * Bit 2: Pulsing halo (`0x00B56E`)
2. **Bit 0 Aura Emitter (`0x00B3D2..0x00B45C`)**:
   - Frame counter at `+0x9B(a6)` (or `+0x03(a6)`).
   - Wraps at count 80 (`0x50`); steps through 16-entry 3-byte offset table at `0x00B42C` (`dx`, `dy`, tile flags).
3. **Bit 3 Wave Ring Emitter (`0x00B45C..0x00B4FE`)**:
   - Frame counter at `+0x98(a6)` (or `+0x03(a6)`).
   - Checks shadow preset `+0x3D(a6)`:
     * If `<= 8`: wraps at count 8, uses 8-entry table `0x00B4C8`.
     * If `> 8`: wraps at count 10, uses 10-entry table `0x00B4E0`.
4. **Bit 4 Ground Ripple / Attachment Emitter (`0x00B4FE..0x00B56E`)**:
   - Frame counter at `+0x98(a6)` (or `+0x03(a6)`).
   - Wraps at count 8; on completion clears bit 4 of `$39(a6)` at `0x00B54E`.
   - Uses 8-entry offset table at `0x00B556`.

---

## 2. C++20 Interface Proposal

```cpp
#pragma once

#include <cstdint>
#include <array>

namespace thor::graphics {

struct FxOffsetEntry {
    uint8_t flag{0};
    int8_t dx{0};
    int8_t dy{0};
};

class DynamicFxRendererP1 {
public:
    static bool ShouldRender(bool suppress_fx, uint8_t mode_39) noexcept {
        return !suppress_fx && (mode_39 != 0);
    }

    static bool HasAura(uint8_t mode_39) noexcept { return (mode_39 & 0x01) != 0; }
    static bool HasWaveRing(uint8_t mode_39) noexcept { return (mode_39 & 0x08) != 0; }
    static bool HasRipple(uint8_t mode_39) noexcept { return (mode_39 & 0x10) != 0; }

    static size_t GetWaveRingStepCount(uint8_t shadow_preset) noexcept {
        return (shadow_preset <= 8) ? 8 : 10;
    }
};

} // namespace thor::graphics
```
