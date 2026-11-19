# STANDALONE HANDOFF: Milestone M15.495 — ROM Shade Spirit Grapple Retraction, Entity/Item Reel-In Sync, Dynamic Tether Stretch & Tip Probe Offset Table

## 1. Subsystem Overview
- **ROM Address Range:** `0x01341A..0x01366A` (592 bytes total: 568 bytes executable M68K code across 159 instructions at `0x01341A..0x013652` + 24-byte inline `ShadeGrappleTipOffsetTable` at `0x013652..0x01366A`).
- **Core Responsibilities:**
  1. `0x01341A..0x0134DE` (`AI_Shade_State0A_GrappleRetractAndComplete`, 196 bytes, 51 insns):
     - Clears Ali velocities `+$72(a5) = 0` and `+$76(a5) = 0` every frame.
     - Decrements retract timer `+$02(a6)` (16-bit signed); if `<= 0`, branches to `0x01348E` (completion).
     - When `+$AA(a6) == 0` (empty grapple), calls `0x00E398`; if Manhattan distance `d5 <= $10` (16 px), branches to `0x01348E` early.
     - Accelerates `+$4E(a6), +$52(a6)` by `(0x60, 0x60)` along facing `+$16(a6)` via `0x00E2D4`:
       - `+$AA == 0`: adds `(+$4E, +$52)` into Shade `+$72(a6), +$76(a6)`.
       - `0 < +$AA < $00FF2954` (entity pool): subtracts `(+$4E, +$52)` from Ali anchor offset `+$AE(a6), +$B2(a6)` and calls `0x01352A`.
       - `+$AA >= $00FF2954` (item pool): adds `(+$4E, +$52)` into Shade `+$72(a6), +$76(a6)` and calls `0x013550`.
     - **Completion (`0x01348E`):**
       - If `0 < +$AA < $00FF2954`: runs `0x009D00` on the grappled entity (`exg.l d0, a6; jsr $9d00; exg.l d0, a6`).
       - If `+$AA >= $00FF2954`: snaps item 32-bit `+$08/+$0C` to Ali's `+$08/+$0C(a5)`.
       - Clears `$FF1982 = 0`, `+$38(a5)` bit 3, and `$FF17B8` bit 2.
       - Frees tether slot `+$A6(a6)` (`+$00 = 0`) if `+$00 == $FFFF`, and branches to `0x01378A` (Shade idle State `$00`).
  2. `0x0134DE..0x013576` (`AI_Shade_GrappleTetherAnchorAndDragHelpers`, 152 bytes, 41 insns):
     - **Anchor Lock (`0x0134DE`):** Returns on `+$AA == 0` or jumps to `0x013550` if `+$AA >= $00FF2954`. Otherwise tests `+$17(a5)` bit 0 (Ali horizontal vs vertical facing):
       - Horizontal (`Right=1` / `Left=3`): locks `+$AE = ali_x - tgt_x`, `+$B2 = 0`, and snaps both Shade and Ali `+$0C` onto `target_y`.
       - Vertical (`Up=0` / `Down=2`): locks `+$B2 = ali_y - tgt_y`, `+$AE = 0`, and snaps both Shade and Ali `+$08` onto `target_x`.
     - **Ali Sync (`0x01352A`):** Branches to `0x013550` if `+$AA >= $00FF2954`, else sets `Ali(+$08, +$0C) = Target(+$08, +$0C) + (+$AE, +$B2)`.
     - **Item Drag (`0x013550`):** Skips if `+$32(a6) < $15` (21); else calls `0x013622` and sets item word position `(+$08, +$0C) = (shade_x + dx, shade_y + dy - shade_z)`.
  3. `0x013576..0x013608` (`AI_Shade_UpdateTetherStretchEntity`, 146 bytes, 42 insns):
     - Checks tether parent `+$A6(a6)`: despawns (`+$00(a6) = 0`) if `+$00(parent) != $1C` (Shade); if `+$36(parent)` bit 1 is set (low-SP blink frame), returns (`0x013586`) without rendering and without despawning.
     - Computes Chebyshev `dist = max(|ali_x - shade_x|, |ali_y - (shade_y - shade_z)|)` from `$FF19E8` and `$FF1AA4`.
     - If `dist <= $14` (20 px), skips render. Otherwise sets `idx = min((dist - 20) >> 3, 11)`, writes `+$32(a6) = $60 + idx * 3`, copies Shade `(+$08.w, +$0C.w, +$10.l)`, calls `0x00B022`, and zeroes 32-bit `+$0C(a6)`.
  4. `0x013608..0x01366A` (`AI_Shade_ProbeItemsAtGrappleTip` + `ShadeGrappleTipOffsetTable`, 98 bytes: 74B code / 25 insns + 24B table):
     - `0x013608`: calls `0x013622`, expands `[dx - 4 .. dx + 4, dy - 4 .. dy + 4, z = 0]`, and tail-branches to `0x00B9EC`.
     - `0x013622`: selects signed `(dx, dy)` pair from `0x013652..0x01366A` (`3 x 4 x 2` signed bytes) by `+$32(a6)` tier and `+$16(a6)` facing `0..3`:
       - Tier 0 (`+$32 <= $17`): `[(0,-24), (32,0), (0,24), (-32,0)]`
       - Tier 1 (`$18 <= +$32 <= $1A`): `[(0,-14), (16,0), (0,10), (-16,0)]`
       - Tier 2 (`+$32 > $1A`): `[(0,-2), (8,0), (0,0), (-8,0)]`

## 2. ROM Evidence Reference
- Evidence JSON: `data/thor-evidence/rom_shade_retract_tether_m15_495.json`
- Extractor: `tools/extract_rom_shade_retract_tether.py`
- Test: `tests/rom_shade_retract_tether_test.py`

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>

namespace Thor::AI {

inline constexpr uint16_t kShadeRetractEarlyManhattanMaxPx   = 0x0010;      // 16 px
inline constexpr uint16_t kShadeRetractAccelStep             = 0x0060;
inline constexpr uint32_t kShadeItemPoolBaseAddress          = 0x00FF2954u;
inline constexpr uint16_t kShadeTetherFreeSentinel           = 0xFFFF;
inline constexpr uint16_t kShadeDragMinSpriteFrame           = 0x0015;      // 21
inline constexpr uint16_t kShadeEntityType                   = 0x001C;
inline constexpr uint16_t kShadeTetherMinStretchExclusivePx  = 0x0014;      // 20 px
inline constexpr uint16_t kShadeTetherMaxStretchIndex        = 0x000B;      // 11
inline constexpr uint16_t kShadeTetherSpriteBase             = 0x0060;
inline constexpr uint16_t kShadeTetherSpriteStride           = 3;
inline constexpr int16_t  kShadeTipProbeHalfExtentPx         = 4;
inline constexpr uint16_t kShadeTipTier0MaxFrame             = 0x0017;
inline constexpr uint16_t kShadeTipTier1MaxFrame             = 0x001A;

struct TipOffset2D {
    int8_t dx{0};
    int8_t dy{0};
};

inline constexpr std::array<std::array<TipOffset2D, 4>, 3> kShadeGrappleTipOffsetTable = {{
    {{{0, -24}, {32, 0}, {0, 24}, {-32, 0}}},  // Tier 0: sprite <= 0x17
    {{{0, -14}, {16, 0}, {0, 10}, {-16, 0}}},  // Tier 1: 0x18..0x1A
    {{{0, -2},  {8, 0},  {0, 0},  {-8, 0}}},   // Tier 2: > 0x1A
}};

struct ShadeGrappleRetractState {
    uint16_t type_00{kShadeEntityType};
    int16_t  timer_02{0};
    uint16_t state_04{0};
    int32_t  x_08_fixed{0};
    int32_t  y_0c_fixed{0};
    int32_t  z_10_fixed{0};
    uint16_t facing_16{0};
    uint16_t sprite_frame_32{0};
    uint8_t  flags_36{0};
    uint8_t  flags_38{0};
    int32_t  vel_x_4e{0};
    int32_t  vel_y_52{0};
    int32_t  vel_x_72{0};
    int32_t  vel_y_76{0};
    int32_t  anchor_dx_ae{0};
    int32_t  anchor_dy_b2{0};
};

inline TipOffset2D LookupShadeGrappleTipOffset(
    uint16_t sprite_frame_32,
    uint16_t facing_16) noexcept {
    const size_t tier = (sprite_frame_32 <= kShadeTipTier0MaxFrame)
        ? 0u
        : ((sprite_frame_32 <= kShadeTipTier1MaxFrame) ? 1u : 2u);
    return kShadeGrappleTipOffsetTable[tier][facing_16 & 0x03u];
}

inline void LockShadeTetherAnchor(
    ShadeGrappleRetractState& shade,
    ShadeGrappleRetractState& ali,
    int32_t target_x_fixed,
    int32_t target_y_fixed) noexcept {
    const int32_t dx = ali.x_08_fixed - target_x_fixed;
    const int32_t dy = ali.y_0c_fixed - target_y_fixed;
    if ((ali.facing_16 & 0x01u) != 0u) {
        shade.anchor_dx_ae = dx;
        shade.anchor_dy_b2 = 0;
        shade.y_0c_fixed -= dy;
        ali.y_0c_fixed -= dy;
    } else {
        shade.anchor_dy_b2 = dy;
        shade.anchor_dx_ae = 0;
        shade.x_08_fixed -= dx;
        ali.x_08_fixed -= dx;
    }
}

inline bool ComputeShadeTetherStretchSprite(
    const ShadeGrappleRetractState& shade,
    const ShadeGrappleRetractState& ali,
    uint16_t& out_sprite_frame_32) noexcept {
    const int16_t shade_x = static_cast<int16_t>(shade.x_08_fixed >> 16);
    const int16_t shade_y = static_cast<int16_t>(shade.y_0c_fixed >> 16);
    const int16_t shade_z = static_cast<int16_t>(shade.z_10_fixed >> 16);
    const int16_t ali_x = static_cast<int16_t>(ali.x_08_fixed >> 16);
    const int16_t ali_y = static_cast<int16_t>(ali.y_0c_fixed >> 16);
    const uint16_t dx = static_cast<uint16_t>(std::abs(ali_x - shade_x));
    const uint16_t dy = static_cast<uint16_t>(std::abs(ali_y - (shade_y - shade_z)));
    const uint16_t cheb = std::max(dx, dy);
    if (cheb <= kShadeTetherMinStretchExclusivePx) {
        return false;
    }
    uint16_t idx = static_cast<uint16_t>((cheb - kShadeTetherMinStretchExclusivePx) >> 3);
    idx = std::min(idx, kShadeTetherMaxStretchIndex);
    out_sprite_frame_32 = static_cast<uint16_t>(kShadeTetherSpriteBase + idx * kShadeTetherSpriteStride);
    return true;
}

} // namespace Thor::AI
```
