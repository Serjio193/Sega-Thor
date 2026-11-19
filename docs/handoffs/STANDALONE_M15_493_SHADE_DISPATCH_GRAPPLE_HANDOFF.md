# STANDALONE HANDOFF: Milestone M15.493 — ROM Shade Spirit Master Tick, 40-Frame SP Drain, Damage-to-SP Absorption, 10-State Jump Table, Shadow Grapple & Anchor Warp Starters

## 1. Subsystem Overview
- **ROM Address Range:** `0x012F1E..0x01319E` (640 bytes: 144 instructions across `0x012F1E..0x013008` and `0x01301C..0x01319E` + 20-byte 10-entry relative jump table at `0x013008..0x01301C`).
- **Core Responsibilities:**
  1. `0x012F1E..0x01301E` (`AI_Shade_MasterTickAndSpAbsorption`, 256 bytes: 64 insns + 20-byte jump table):
     - Sets `a5 = $FF19E8`; returns immediately if `+$A4(a6) != 0`; jumps directly to dispatcher `0x012FFC` if `+$A2(a6) != 0`.
     - Clears `$FF1989`, calls `0x00E682` (hitstun early-return guard) and `0x011A4C` (`Spirit_PollButtonACommandInput`).
     - Ticks 40-frame (`0x28`) timer `+$8E(a6)` to drain `1` SP from `+$8E(a5)` (dismissing via `0x0136CC` if SP is `0` before or after decrement); toggles `+$36(a6)` bit 1 when `SP <= 1` and clears bit 1 when `SP > 1`.
     - **Damage-to-SP Absorption (`0x012F80..0x012FFC`):** When `+$86(a6) != 0`, clears `+$86`, doubles `(+$83 + +$82) * 2` clamped `>= 8` (clearing `+$82`), subtracts `+$86 & 0x0FFF` from `+$8E(a5)` clamped `>= 0`, loads `+$7E(a6) -> a0`, sets `+$39(a6)` bit 3 if bit 13 of `+$86` was set, enters State `$04`, copies `+$84(a6)` facing (flipping bit 1 via `bchg #1` if bit 0 is set), adds `+$0C(a6) += 1` if resulting facing `!= 0` and `+$10(a6) += 3`, sets anim `$0C` (`0x008EF8`), and calls `0x013764` (loading Shade sprite/palette data from `$03F4A6`).
     - Dispatches `+$04(a6)` (`$00..$12`) via the 10-entry signed word self-relative table at `0x013008..0x01301C` (State `$12` targets `rts` at `0x01301C`).
  2. `0x01301E..0x013134` (`AI_Shade_TryStartShadowGrapple`, 278 bytes, 60 insns; Command 1):
     - Guards Ali state `+$04(a5) == 0`, terrain probe `0x009C40` bit 7 clear, and `7` SP via `0x011B28` (else buzzer `0x01319E`).
     - Sets `$FF1982 = $FF`, `+$38(a5) |= 8`, `$FF17B8 |= 4`, State `$06`, `+$2A = 2`, calls `0x013764`.
     - Snaps Shade `(x, y)` and facing to Ali, zeroes Ali & Shade `+$72/+$76`, adds `(0x1000, 0x1000)` into `+$72/+$76` and writes `(0x0100, 0x0100)` into `+$4E/+$52` via `0x00E2D4`, sets `+$10 = 1`, `+$02 = $12` (18), clears `+$38` bit 2 & `+$AA = 0`.
     - Allocates Pool 1 tether slot (`0x00DA1A`): `+$00 = $FFFF`, `+$22 = 0x00013576`, inherits `+$16`, `+$32 = $60`, `+$34 = $FFFF`, `+$36 = 0`, `+$38 = $0D`, `+$0C = 0`, `+$10 = 0`, `+$18 = (parent_18 & $8000) | $07A8`, `+$1A = *($03F4A6)`, doubly links `+$A6`, plays SFX `#$80037B` via `$060004`.
  3. `0x013134..0x01319E` (`AI_Shade_TryStartShadowAnchorOrRescue`, 106 bytes, 20 insns; Command 3):
     - Requires `$FF1986 != 0` (else `0x01319E`), sets `+$38(a5) |= $4C`, clears `$FF198E`, sets `$FF17B8 |= 4`, clears `+$06(a5)`, snaps Shade to Ali and zeroes `+$72/+$76`, sets `$FF1985 = $FF`, clears `+$16`, enters State `$0C`, `+$2A = $0E`, calls `0x013764`, plays SFX `#$80037C`.

### Shade 10-Entry Relative Jump Table (`0x013008..0x01301C`)
| State | Slot Addr | Disp | Target | Role |
|---|---|---|---|---|
| `$00` | `0x013008` | `+0x0662` | `0x01366A` | Idle / command & follow |
| `$02` | `0x01300A` | `+0x070C` | `0x013716` | Summon / materialize |
| `$04` | `0x01300C` | `+0x06A0` | `0x0136AC` | Hitstun / damage reaction |
| `$06` | `0x01300E` | `+0x029C` | `0x0132AA` | Shadow Grapple launch |
| `$08` | `0x013010` | `+0x03C0` | `0x0133D0` | Grapple reel / return |
| `$0A` | `0x013012` | `+0x0408` | `0x01341A` | Grapple carry / swing |
| `$0C` | `0x013014` | `+0x019A` | `0x0131AE` | Dark Hole / Anchor active |
| `$0E` | `0x013016` | `+0x0248` | `0x01325E` | Anchor warp finish |
| `$10` | `0x013018` | `+0x06E0` | `0x0136F8` | Dismissal descent |
| `$12` | `0x01301A` | `+0x0002` | `0x01301C` | No-op (`rts`) |

## 2. ROM Evidence Reference
- Evidence JSON: `data/thor-evidence/rom_shade_dispatch_grapple_m15_493.json`
- Extractor: `tools/extract_rom_shade_dispatch_grapple.py`
- Test: `tests/rom_shade_dispatch_grapple_test.py`

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <algorithm>
#include <cstdint>

namespace Thor::AI {

inline constexpr uint16_t kShadeSpDrainIntervalFrames = 0x0028;      // 40 frames
inline constexpr uint16_t kShadeDamageSpMask          = 0x0FFFu;
inline constexpr uint8_t  kShadeMinHitstunFrames      = 8;
inline constexpr int16_t  kShadeGrappleSpCost         = 7;
inline constexpr uint16_t kShadeGrappleTimerFrames    = 0x0012;      // 18 frames
inline constexpr uint32_t kShadeTetherCallbackAddr    = 0x00013576u;
inline constexpr uint32_t kShadeGrappleSfxId          = 0x0080037Bu;
inline constexpr uint32_t kShadeAnchorSfxId           = 0x0080037Cu;
inline constexpr uint8_t  kShadeAnchorAliFlags38Mask  = 0x4Cu;

struct ShadeEntityState {
    uint16_t type_00{0x001C};
    uint16_t timer_02{0};
    uint16_t state_04{0};
    uint16_t sprite_ov_06{0};
    int16_t  x_08{0};
    int16_t  y_0c{0};
    int16_t  z_10{0};
    uint16_t facing_16{0};
    uint16_t priority_18{0};
    uint32_t sprite_1a{0};
    uint32_t callback_22{0};
    uint16_t action_2a{0};
    uint16_t subtype_32{0};
    uint16_t param_34{0};
    uint8_t  flags_36{0};
    uint8_t  flags_38{0};
    uint8_t  flags_39{0};
    int32_t  vel_x_4e{0};
    int32_t  vel_y_52{0};
    int32_t  pos_x_72{0};
    int32_t  pos_y_76{0};
    uint8_t  hit_flag_82{0};
    uint8_t  hitstun_83{0};
    uint16_t hit_dir_84{0};
    uint16_t damage_86{0};
    uint16_t drain_timer_8e{kShadeSpDrainIntervalFrames};
    uint8_t  sub_entity_a2{0};
    uint8_t  skip_flag_a4{0};
    uint32_t field_aa{0};
};

inline bool StepShadeSpDrain(ShadeEntityState& shade, int16_t& player_sp) noexcept {
    if (shade.sub_entity_a2 != 0) {
        return true;
    }
    if (shade.drain_timer_8e != 0) {
        --shade.drain_timer_8e;
    } else {
        shade.drain_timer_8e = kShadeSpDrainIntervalFrames;
        if (player_sp == 0 || --player_sp == 0) {
            return false;  // Dismiss via 0x0136CC
        }
    }
    if (player_sp <= 1) {
        shade.flags_36 ^= 0x02u;
    } else {
        shade.flags_36 &= static_cast<uint8_t>(~0x02u);
    }
    return true;
}

inline void ApplyShadeDamageToSpAbsorption(
    ShadeEntityState& shade,
    int16_t& player_sp) noexcept {
    if (shade.damage_86 == 0) {
        return;
    }
    const uint16_t dmg = shade.damage_86;
    shade.damage_86 = 0;

    uint8_t stun = static_cast<uint8_t>((shade.hitstun_83 + shade.hit_flag_82) * 2u);
    shade.hit_flag_82 = 0;
    if (stun < kShadeMinHitstunFrames) {
        stun = kShadeMinHitstunFrames;
    }
    shade.hitstun_83 = stun;

    const int16_t sp_cost = static_cast<int16_t>(dmg & kShadeDamageSpMask);
    player_sp = std::max<int16_t>(0, static_cast<int16_t>(player_sp - sp_cost));

    if ((dmg & (1u << 13)) != 0) {
        shade.flags_39 |= 0x08u;
    }
    shade.state_04 = 0x04;
    uint16_t facing = shade.hit_dir_84;
    if ((facing & 0x01u) != 0) {
        facing ^= 0x02u;
    }
    shade.facing_16 = facing;
    if (facing != 0) {
        ++shade.y_0c;
    }
    shade.z_10 = static_cast<int16_t>(shade.z_10 + 3);
    shade.action_2a = 0x0C;
}

inline void InitShadeGrappleTetherSlot(
    ShadeEntityState& tether,
    const ShadeEntityState& parent,
    uint32_t sprite_ptr) noexcept {
    tether.type_00 = 0xFFFFu;
    tether.callback_22 = kShadeTetherCallbackAddr;
    tether.facing_16 = parent.facing_16;
    tether.subtype_32 = 0x0060u;
    tether.param_34 = 0xFFFFu;
    tether.flags_36 = 0;
    tether.flags_38 = 0x0Du;
    tether.y_0c = 0;
    tether.z_10 = 0;
    tether.priority_18 = static_cast<uint16_t>((parent.priority_18 & 0x8000u) | 0x07A8u);
    tether.sprite_1a = sprite_ptr;
}

} // namespace Thor::AI
```
