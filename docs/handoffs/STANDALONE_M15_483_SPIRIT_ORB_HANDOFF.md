# STANDALONE HANDOFF: Milestone M15.483 — ROM Spirit Entity Orb Kinematics, Trail Table & Diamond Vertex Generator

## 1. Subsystem Overview
- **ROM Address Range:** `0x0115C8..0x01171C` (340 bytes, 100 instructions, 18-byte scale table).
- **Core Responsibilities:**
  1. `0x0115D8` (`AI_SpiritOrb_State2_FrictionKinematics`): Spirit orb State 2 kinematics. Decelerates 32-bit fixed velocities `+$4E/+$52` by friction step `0x00018000` per frame toward zero, integrates position into `+$72/+$76`, increments timer `+$2(a6)`, looks up scale table `0x0116D2`, and calls diamond vertex generator `0x0116E4`.
  2. `0x011646` (`AI_SpiritOrb_State1_OrbAnimationAndTargetSnap`): Spirit orb State 1 animation. Increments timer `+$2(a6)`. For frames 0..8, forward indexes scale table `(timer - 1)`; for frames 9..26, reverse indexes scale table `(26 - timer)`. At frame 27+, checks `$FF19A6 == 3`, snaps to target `$FF19E8`, triggers warp SFX `$80022F` via `0x00E7FC`.
  3. `0x0116D2` (`SpiritOrbScaleTable`): 18-byte embedded scale table: `[3, 5, 7, 8, 9, 9, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11]`.
  4. `0x0116E4` (`AI_SpiritOrb_BuildDiamondVertexBuffer`): Generates 12-byte 4-point diamond vertex buffer at `(a2)` using dynamic radial offset `(timer % 3) + 11` and scale `d1`, writes 6 coordinate pairs, sets vertex count 3 at `(a1)`, sprite animation `$16` at `+$6(a6)`, and links buffer pointer to `+$62(a6)`.

## 2. ROM Evidence Reference
- Evidence JSON: `data/thor-evidence/rom_spirit_orb_kinematics_m15_483.json`
- Extractor: `tools/extract_rom_spirit_orb_kinematics.py`
- Test: `tests/rom_spirit_orb_kinematics_test.py`

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <cstdint>
#include <array>

namespace Thor::AI {

constexpr std::array<uint8_t, 18> SPIRIT_ORB_SCALE_TABLE = {
    3, 5, 7, 8, 9, 9, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11
};

inline int32_t StepFrictionVelocity(int32_t vel) noexcept {
    constexpr int32_t step = 0x18000;
    if (vel > 0) {
        vel -= step;
        if (vel < 0) vel = 0;
    } else if (vel < 0) {
        vel += step;
        if (vel > 0) vel = 0;
    }
    return vel;
}

inline uint8_t GetOrbScaleForTimer(uint16_t timer, uint16_t state) noexcept {
    if (state == 2) {
        uint16_t idx = timer - 1;
        return (idx < SPIRIT_ORB_SCALE_TABLE.size()) ? SPIRIT_ORB_SCALE_TABLE[idx] : 0;
    } else if (state == 1) {
        if (timer < 9) {
            return SPIRIT_ORB_SCALE_TABLE[timer - 1];
        } else if (timer < 27) {
            return SPIRIT_ORB_SCALE_TABLE[26 - timer];
        }
    }
    return 0;
}

struct DiamondVertexBuffer {
    std::array<int8_t, 12> vertices;
    uint8_t vertex_count{3};
    uint16_t anim_id{0x16};
};

inline DiamondVertexBuffer GenerateDiamondVertices(uint16_t timer, uint8_t scale_d1) noexcept {
    int8_t d0 = static_cast<int8_t>((timer % 3) + 11);
    int8_t d1 = static_cast<int8_t>(scale_d1);
    int8_t d2 = static_cast<int8_t>(-scale_d1);

    DiamondVertexBuffer buf;
    buf.vertices = {
        d0, d1,
        d1, d0,
        d1, d2,
        d0, d2,
        d1, d0,
        d2, d2
    };
    return buf;
}

} // namespace Thor::AI
```
