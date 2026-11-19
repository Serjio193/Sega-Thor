# STANDALONE HANDOFF: Milestone M15.500 — ROM Bow Spirit (`0x1E`) Dismissal, States `$10`/`$02`, Neck/Vine Chain Interpolator, SAT Sub-Entity Renderer & Leash Velocity Clamper

## 1. Subsystem Overview
- **ROM Address Range:** `0x013F22..0x014242` (800 bytes, 206 M68K instructions; contiguous executable code immediately preceding Bow leash/range clamp helper `0x014242`).
- **Core Responsibilities:**
  1. `0x013F22..0x013F5A` (`AI_Bow_Dismissal`, 56 bytes, 12 insns):
     - Guards `+$04(a6) == 0`; if non-zero branches to `0x0138B8` (low-SP blink handler).
     - Plays dismissal SFX `#$00A0039E` via `0x00E7FC`, frees child sub-entity `+$00(+$A6(a6)) = 0`, enters State `$10` (`+$2A = $1E`), clears `+$16(a6) = 0`, sets priority `+$18(a6) = $07A8`, and clears `+$36(a6)` bit 0.
  2. `0x013F5A..0x013F6E` (`AI_Bow_State10_DescendDespawn`, 20 bytes, 5 insns):
     - Clears `+$36(a6)` bit 1; once `+$37(a6)` bit 1 (animation complete) is set, clears `+$00(a6) = 0` to despawn Bow.
  3. `0x013F6E..0x013FDC` (`AI_Bow_State02_ReturnAndSettle`, 110 bytes, 24 insns):
     - Waits for `+$37(a6)` bit 1, settles into State `0` (`+$2A = 0`, `+$9E = 0`, `+$16 = 1`, clears `+$38` bit 2).
     - Initializes `$FF1C1C` vine SAT sub-entity (`+$00 = $FFFF`, custom render callback `+$22 = 0x00014084`, `+$18 = (parent_18 & $8000) | $07A8`, anchor `+$4E/+$52/+$56` from parent `+$08/+$0C/+$10`, cross-links `+$A6` in both parent and child), then calls `0x013FFC`.
  4. `0x013FDC..0x014084` (`AI_Bow_InterpolateVineChainNodes`, 168 bytes, 55 insns):
     - `0x013FDC`: Computes 32-bit `(head - anchor)` X/Y delta into `0x00E338` and writes cardinal facing `+$16(a6)` pointing away from the root anchor.
     - `0x013FFC`: Sets `$FF1C1C` Y-depth sort key `+$0C(a4) = min(head_y - 2, anchor_y)`. Steps 4 nodes (`1/5..4/5` from anchor `+$4E/+$52/+$56` to head `+$08/+$0C/+$10`) into `$FF19A8..$FF19BF` (`4 x 6` bytes XYZ) using 12.4 fixed-point `divs.w #5` and per-step `asr.w #4 -> store -> asl.w #4` quantization (`0x01406A`).
  5. `0x014084..0x014200` (`AI_Bow_RenderVineSubEntitySAT`, 380 bytes, 92 insns):
     - Checks parent `+$00(+$A6(a6)) == $001E` (else frees self `+$00(a6) = 0`); skips rendering if parent `+$36` bit 1 blink flag is set.
     - Selects camera scroll `$FF1716` (Plane A) or `$FF173E` (Plane B via `$FF16F0` bit 7) and emits up to 6 SAT entries to `$FF13CC + $FF188C` (clip `d5 < 80`, `X in [$68, $1C0)`, `Y in [$70, $160)`):
       - 4 vine nodes walked backward `$FF19BA -> $FF19A8` (`tile = +$18`, `size = 0x0000`, `sx = x - cam_x + $7C`, `sy = y - cam_y + $7C - z`).
       - Bow ground shadow from `$FF1AA4` (`tile = 0x0549`, `size = 0x0400`, `sx = x - cam_x + $78`, `sy = y - cam_y + $7D`).
       - Root anchor from `$FF1C1C` (`tile = +$18 + 9`, `size = 0x0E00`, `sx = x - cam_x + $71`, `sy = y - cam_y + $74 - z`).
     - Commits updated SAT byte offset `$FF188C` and sprite count `$FF188A`.
  6. `0x014200..0x014242` (`AI_Bow_StepAndClampVelocityByLeash`, 66 bytes, 18 insns):
     - Saves 32-bit `+$08/+$0C(a6)` on stack, applies `+$72/+$76(a6)`, calls `0x014242` (leash range clamp), computes clamped step `new_pos - old_pos` into `+$72/+$76(a6)`, and restores original `+$08/+$0C(a6)`.

## 2. ROM Evidence Reference
- Evidence JSON: `data/thor-evidence/rom_bow_dismiss_vine_sat_m15_500.json`
- Extractor: `tools/extract_rom_bow_dismiss_vine_sat.py`
- Test: `tests/rom_bow_dismiss_vine_sat_test.py`

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <array>
#include <cstdint>

namespace Thor::AI {

inline constexpr uint16_t kBowEntityTypeId        = 0x001Eu;
inline constexpr uint32_t kBowDismissalSfxId      = 0x00A0039Eu;
inline constexpr uint16_t kBowDismissalAnim2A     = 0x001Eu;
inline constexpr uint16_t kBowVineSubSentinel     = 0xFFFFu;
inline constexpr uint16_t kBowVinePriorityBase    = 0x07A8u;
inline constexpr uint16_t kBowShadowTileAttr      = 0x0549u;
inline constexpr uint16_t kBowShadowSatSize       = 0x0400u;
inline constexpr uint16_t kBowRootTileOffset      = 9u;
inline constexpr uint16_t kBowRootSatSize         = 0x0E00u;
inline constexpr uint16_t kBowSatMaxSprites       = 80u;
inline constexpr int16_t  kBowSatMinX             = 0x0068;
inline constexpr int16_t  kBowSatMaxXExclusive    = 0x01C0;
inline constexpr int16_t  kBowSatMinY             = 0x0070;
inline constexpr int16_t  kBowSatMaxYExclusive    = 0x0160;

struct VineNode {
    int16_t x{0};
    int16_t y{0};
    int16_t z{0};
};

struct SatEntryWords {
    int16_t  y{0};
    uint16_t size_link{0};
    uint16_t tile_attr{0};
    int16_t  x{0};
};

inline std::array<VineNode, 4> InterpolateVineChainNodes(
    const VineNode& anchor,
    const VineNode& head) noexcept {
    const int16_t step_x = static_cast<int16_t>((static_cast<int32_t>(head.x - anchor.x) << 4) / 5);
    const int16_t step_y = static_cast<int16_t>((static_cast<int32_t>(head.y - anchor.y) << 4) / 5);
    const int16_t step_z = static_cast<int16_t>((static_cast<int32_t>(head.z - anchor.z) << 4) / 5);

    int16_t fx = static_cast<int16_t>(anchor.x << 4);
    int16_t fy = static_cast<int16_t>(anchor.y << 4);
    int16_t fz = static_cast<int16_t>(anchor.z << 4);
    std::array<VineNode, 4> nodes{};
    for (auto& node : nodes) {
        fx = static_cast<int16_t>(fx + step_x);
        fy = static_cast<int16_t>(fy + step_y);
        fz = static_cast<int16_t>(fz + step_z);
        node.x = static_cast<int16_t>(fx >> 4);
        node.y = static_cast<int16_t>(fy >> 4);
        node.z = static_cast<int16_t>(fz >> 4);
        fx = static_cast<int16_t>(node.x << 4);
        fy = static_cast<int16_t>(node.y << 4);
        fz = static_cast<int16_t>(node.z << 4);
    }
    return nodes;
}

inline bool TryAppendBowSatEntry(
    int16_t sx, int16_t sy, uint16_t tile_attr, uint16_t size_bits,
    uint16_t& sprite_count_d5, SatEntryWords& out_entry) noexcept {
    if (sprite_count_d5 >= kBowSatMaxSprites) return false;
    if (sx < kBowSatMinX || sx >= kBowSatMaxXExclusive) return false;
    if (sy < kBowSatMinY || sy >= kBowSatMaxYExclusive) return false;
    ++sprite_count_d5;
    out_entry = SatEntryWords{
        sy, static_cast<uint16_t>(size_bits + sprite_count_d5), tile_attr, sx
    };
    return true;
}

} // namespace Thor::AI
```
