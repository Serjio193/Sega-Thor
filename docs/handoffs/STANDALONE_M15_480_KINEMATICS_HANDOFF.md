# STANDALONE HANDOFF: Milestone M15.480 — ROM Entity AI Target Angle Projector, Hover Bobbing Elevation Tracker & Multi-Segment Kinematics Engine

## 1. Subsystem Overview
- **ROM Address Range:** `0x01108C..0x011276` (490 bytes, 138 instructions).
- **Core Responsibilities:**
  1. `0x01108C` (`Math_TargetAngleAndVelocityProjector_Signed`): Evaluates target vector `dx = target_x - self_x`, `dy = self_y - target_y` (signed Y convention), determines quadrant, calls octant arctangent lookup `0x011102`, divides scale inputs `d2, d3` by 128 (`asr.l #7`), looks up sine/cosine from ROM table `$5D706`, and projects 2D velocity components into `(d0, d1)`.
  2. `0x011102` (`Math_ArctangentLookup_Octant_Helper`): Computes octant tangent slope `(dy << 8) / dx` and scans the 8-entry threshold table at ROM `$5D906` (`[25, 78, 137, 210, 312, 479, 844, 2599]`), returning angle in steps of 8 (or `0x40` on `dx == 0`).
  3. `0x011128` (`AI_UpdateEntityHoverBobbingAndElevationTracking`): Updates hover elevation bobbing. Advances oscillation phase counter `+$BB(a6) += 1`. When bit 4 is clear, adds `$8000` to `+$A2(a6)`; when bit 4 is set, subtracts `$8000`. Calculates target elevation `ali_elevation ($FF19FC) - +$14(a6) + (+A2(a6) >> 16)`, nudges entity elevation `+$10(a6)` towards target by +/- 1 unit per frame, and clamps at `>= 0`.
  4. `0x011176` (`AI_HookSurfaceContactHandler1`): Installs callback `0x01118A` at `$FF1940` and calls surface contact engine `0x00BCE2`. Callback preserves facing `+$16(a6)`, faces Player Ali via `0x010752`, runs animation handler `0x00BEBA`, and restores facing.
  5. `0x0111A8` (`AI_HookSurfaceContactHandler2`): Installs callback `0x0111BC` at `$FF1940` and calls surface contact engine `0x00BCE2`. Callback preserves facing, faces Ali, runs animation handler `0x00BEBA`, asserts bit 5 of `+$86(a0)` (special surface contact flag), and restores facing.
  6. `0x0111E0` (`AI_CountVacantSlotsPool2`): Iterates over the 17 slots of Pool 2 (`$FF1CD8`, stride 188 bytes) and counts vacant slots where `type +$0(a0) == 0`. Returns count in `d0.w`.
  7. `0x011206` (`AI_UpdateMultiSegmentEntityKinematics`): Updates multi-segment entity linkages. If `+$9E(a6)` is non-zero, applies displacement from table `+$A8(a6)` indexed by `+$A4(a6)*2` to entity coordinates and anchor positions. If parent link `+$A6(a6)` is non-null, shifts parent's 8-word history buffer and computes child-to-parent anchor deltas in `+$AA/+$AB(parent)`.

## 2. ROM Evidence Reference
- Evidence JSON: `data/thor-evidence/rom_entity_ai_kinematics_m15_480.json`
- Extractor: `tools/extract_rom_entity_ai_kinematics.py`
- Test: `tests/rom_entity_ai_kinematics_test.py`

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <cstdint>
#include <array>
#include <algorithm>

namespace Thor::AI {

constexpr std::array<uint16_t, 8> ROM_TANGENT_TABLE = {
    25, 78, 137, 210, 312, 479, 844, 2599
};

inline uint8_t LookupOctantArctangent(int16_t dx, int16_t dy) noexcept {
    if (dx == 0) return 0x40;
    uint32_t ratio = (static_cast<uint32_t>(dy) << 8) / static_cast<uint32_t>(dx);
    uint8_t angle = 0;
    for (uint16_t threshold : ROM_TANGENT_TABLE) {
        if (ratio <= threshold) break;
        angle += 8;
    }
    return angle;
}

inline uint8_t ComputeTargetAngleSigned(int16_t self_x, int16_t self_y,
                                       int16_t target_x, int16_t target_y) noexcept {
    int16_t dx = target_x - self_x;
    int16_t dy = self_y - target_y;

    if (dx >= 0) {
        if (dy >= 0) {
            return LookupOctantArctangent(dx, dy);
        } else {
            return static_cast<uint8_t>(-LookupOctantArctangent(dx, -dy));
        }
    } else {
        int16_t neg_dx = -dx;
        if (dy >= 0) {
            return static_cast<uint8_t>(-LookupOctantArctangent(neg_dx, dy) + 0x80);
        } else {
            return static_cast<uint8_t>(LookupOctantArctangent(neg_dx, -dy) + 0x80);
        }
    }
}

struct HoverElevationState {
    uint8_t phase_counter{0}; // +$BB
    int32_t hover_offset{0};   // +$A2 (fixed 16.16)
    int16_t base_offset{0};    // +$14
    int16_t elevation{0};      // +$10
};

inline void UpdateHoverElevation(HoverElevationState& state, int16_t ali_elevation) noexcept {
    state.phase_counter++;
    if ((state.phase_counter & 0x10) != 0) {
        state.hover_offset -= 0x8000;
    } else {
        state.hover_offset += 0x8000;
    }

    int16_t target = ali_elevation - state.base_offset + static_cast<int16_t>(state.hover_offset >> 16);
    if (target < state.elevation) {
        state.elevation = std::max<int16_t>(0, state.elevation - 1);
    } else if (target > state.elevation) {
        state.elevation += 1;
    }
}

template <size_t N>
inline uint16_t CountVacantPool2Slots(const std::array<uint16_t, N>& slot_types) noexcept {
    uint16_t count = 0;
    size_t limit = std::min<size_t>(N, 17);
    for (size_t i = 0; i < limit; ++i) {
        if (slot_types[i] == 0) {
            count++;
        }
    }
    return count;
}

} // namespace Thor::AI
```
