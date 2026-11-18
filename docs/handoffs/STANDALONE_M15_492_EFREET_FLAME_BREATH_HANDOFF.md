# STANDALONE HANDOFF: Milestone M15.492 — ROM Efreet 7-Particle Flame Breath Ring Engine, Enemy Ignite Handler & Composite Vertex Exporter

## 1. Subsystem Overview
- **ROM Address Range:** `0x012C4E..0x012F1E` (720 bytes: 662 code bytes / 181 M68K instructions across 4 code segments + 58 inline data bytes across 3 embedded tables).
- **Executable / Data Partition:**
  - `0x012C4E..0x012C92` (68 bytes, 19 insns): `AI_Efreet_InitFlameBreathParticlesAndAngle` head
  - `0x012C92..0x012C96` (4 bytes, table): `EfreetFlameSweepStartAngleTable`
  - `0x012C96..0x012D80` (234 bytes, 63 insns): `AI_Efreet_InitFlameBreathParticlesAndAngle` D-pad branches (`0x012C96..0x012CDC`, 70 bytes, 16 insns) + `AI_Efreet_UpdateFlameBreathParticles` code (`0x012CDC..0x012D80`, 164 bytes, 47 insns)
  - `0x012D80..0x012DAE` (46 bytes, table): `EfreetFlameAnimTable`
  - `0x012DAE..0x012EA0` (242 bytes, 67 insns): `AI_Efreet_IgniteEnemyOnFlameHit` (`0x012DAE..0x012DFA`, 76 bytes, 19 insns) + `AI_Efreet_SpawnFlameParticle` code (`0x012DFA..0x012EA0`, 166 bytes, 48 insns)
  - `0x012EA0..0x012EA8` (8 bytes, table): `EfreetFlameSpawnOffsetTable`
  - `0x012EA8..0x012F1E` (118 bytes, 32 insns): `AI_Efreet_ExportFlameVertexBuffer`
- **Core Responsibilities:**
  1. `0x012C4E..0x012CDC` (`AI_Efreet_InitFlameBreathParticlesAndAngle`, 142 bytes):
     - Clears all 7 particle slots at `(a0)+$04` (stride `0x14`, Pool 1 slot `$FF1C1C`) via `st.b $0(a1)` (`0xFF` = free).
     - Tests latched directional byte `$FF19A3` in bit priority `0` (Up) -> `1` (Down) -> `3` (Right) -> `2` (Left). When pressed, sets `+$16(a6)` (`0/2/1/3`), clears `+$AA(a6) = 0x00` (fixed direction), and writes fixed angle `+$AB(a6)` (`0x40 / 0xC0 / 0x00 / 0x80`).
     - When no direction bit `0..3` is set, preserves `+$16(a6)`, sets `+$AA(a6) = 0xFF` (fan-sweep mode), and loads initial angle `EfreetFlameSweepStartAngleTable[+$16]` (`[0x20, 0xE0, 0xA0, 0x60]`).
  2. `0x012CDC..0x012DAE` (`AI_Efreet_UpdateFlameBreathParticles`, 210 bytes):
     - Iterates 7 slots at `+$A6(a6) + $04` (`stride 0x14`), skipping inactive (`+$00(a1) != 0`).
     - Decrements byte `+$12(a1)`. On negative wrap (`< 0`), increments `+$13(a1)` and reads `(tile_word, duration_word)` at `0x012D80 + idx * 4`. On negative `tile_word` (`0xFFFF`), frees slot (`st.b $0(a1)`) and skips physics/collision for that tick; otherwise stores `+$00(a1).w = tile_word` and `+$12(a1).b = duration`.
     - Integrates `+$02(a1).l += +$0A(a1).l`, updates `+$0E(a1).l -= 0x00004000` (upward flame curl), and integrates `+$06(a1).l += +$0E(a1).l`.
     - Scans `0x00B922` (`d0 = 0xF0`, box `[px - 8, px + 8] x [py - 8, py + 8] x [4, 20]` using integer words `+$02(a1)` and `+$06(a1)`). If hit and target `+$38(a0)` bit 3 is clear and `+$82(a0) == 0`, calls `0x012DAE`.
  3. `0x012DAE..0x012DFA` (`AI_Efreet_IgniteEnemyOnFlameHit`, 76 bytes):
     - Links `+$7E(a0) = a6`, clears `+$9D(a0) = 0`, plays SFX `#$00700345` via `0x00E7FC`, copies `+$16(a6)` to `+$84(a0)`, projects `(0x0300, 0x0300)` velocity via `0x00E2D4` into `+$4E/+$52(a0)`, sets `+$56(a0) = 0x00020000`, and writes fire damage `+$86(a0) = 0xA004 + (($FF0DB7 + 1) >> 2)`.
  4. `0x012DFA..0x012EA8` (`AI_Efreet_SpawnFlameParticle`, 174 bytes):
     - Finds first free slot (`+$00(a1).b != 0`) or returns Carry set if all 7 slots are active.
     - Initializes `+$00(a1).w = 0x0063`, `+$12(a1).b = 3`, `+$13(a1).b = 0`.
     - Reads `+$AB(a6)`; if `+$AA(a6) != 0`, increments `+$AB(a6)` by `+5` (mod 256) before computing `(vx, vy)` via `$05D706` at speed `0x0700`: `+$0A = cos * 0x0700`, `+$0E = (-sin * 0x0700) + 0x00020000`.
     - Reads `EfreetFlameSpawnOffsetTable[+$16]` signed `(dx, dy)` bytes and initializes `+$02 = dx << 16`, `+$06 = dy << 16`. Returns Carry clear.
  5. `0x012EA8..0x012F1E` (`AI_Efreet_ExportFlameVertexBuffer`, 118 bytes):
     - Scans all 7 particle slots, packing 3-byte tuples `(+$01 tile, dx, +$07 dy)` into `$FF19A9..` (`dx = -+$03` when `+$16(a6) == 3` Left, else `+$03`).
     - If `count == 0`, clears `+$06(a6) = 0`. Else writes header `(count - 1) | (facing == 2 ? 0x00 : 0x80)` to `$FF19A8`, sets `+$06(a6) = 0x001A`, `+$5E(a6) = 0`, `+$5A(a6) = 0x07A8`, and `+$62(a6) = 0x00FF19A8`.

### Inline Data Tables & Particle Slot Layout
- `EfreetFlameSweepStartAngleTable` (`0x012C92..0x012C96`, 4 bytes): `[0x20, 0xE0, 0xA0, 0x60]` for facings `[0=Up, 1=Right, 2=Down, 3=Left]` (`(center - 0x20) & 0xFF`).
- `EfreetFlameAnimTable` (`0x012D80..0x012DAE`, 46 bytes): 11 `(u16 tile_id, u16 duration)` pairs `[(0x63,1), (0x65,1), (0x67,0), (0x68,0), (0x69,0), (0x68,0), (0x67,0), (0x68,0), (0x69,0), (0x6A,0), (0x6B,0)]` + `0xFFFF` terminator (`14` active update ticks after spawn before termination on tick `15`).
- `EfreetFlameSpawnOffsetTable` (`0x012EA0..0x012EA8`, 8 bytes): `0: (0, -40)`, `1: (32, -12)`, `2: (0, 12)`, `3: (-32, -12)`.
- Particle Slot (`20 = 0x14` bytes, 7 slots at `+$04..+$8F`): `+$00` (`u8` status `0xFF` free / `0x00` active), `+$01` (`u8` tile ID), `+$02` (`s32` 16.16 X), `+$06` (`s32` 16.16 Y), `+$0A` (`s32` VX), `+$0E` (`s32` VY), `+$12` (`u8` timer), `+$13` (`u8` anim index).

## 2. ROM Evidence Reference
- Evidence JSON: `data/thor-evidence/rom_efreet_flame_breath_m15_492.json`
- Extractor: `tools/extract_rom_efreet_flame_breath.py`
- Test: `tests/rom_efreet_flame_breath_test.py`

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <array>
#include <cstdint>

namespace Thor::AI {

inline constexpr uint8_t  kFlameParticleCount         = 7;
inline constexpr uint8_t  kFlameSlotFreeSentinel      = 0xFF;
inline constexpr uint16_t kFlameInitialTileId         = 0x0063;
inline constexpr uint8_t  kFlameInitialDuration       = 3;
inline constexpr uint8_t  kFlameSweepAngleStep        = 5;
inline constexpr int16_t  kFlameParticleSpeed         = 0x0700;
inline constexpr int32_t  kFlameInitialVyBias         = 0x00020000;
inline constexpr int32_t  kFlameUpwardAccelStep       = 0x00004000;
inline constexpr uint8_t  kFlameAabbScanMask          = 0xF0;
inline constexpr uint32_t kFlameIgniteSfxId           = 0x00700345u;
inline constexpr uint16_t kFlameIgniteSpeedX          = 0x0300;
inline constexpr uint16_t kFlameIgniteSpeedY          = 0x0300;
inline constexpr int32_t  kFlameIgniteUpwardVz        = 0x00020000;
inline constexpr uint16_t kFlameBaseDamageWord        = 0xA004;
inline constexpr uint16_t kFlameCompositeAnimMode     = 0x001A;
inline constexpr uint16_t kFlameCompositePriority     = 0x07A8;

inline constexpr std::array<uint8_t, 4> kEfreetFlameSweepStartAngles = {
    0x20, 0xE0, 0xA0, 0x60
};

struct FlameAnimStep {
    int16_t tile_id;
    uint8_t duration;
};

inline constexpr std::array<FlameAnimStep, 12> kEfreetFlameAnimTable = {{
    {0x0063, 1}, {0x0065, 1}, {0x0067, 0}, {0x0068, 0},
    {0x0069, 0}, {0x0068, 0}, {0x0067, 0}, {0x0068, 0},
    {0x0069, 0}, {0x006A, 0}, {0x006B, 0}, {-1, 0}
}};

struct FlameSpawnOffset {
    int8_t dx;
    int8_t dy;
};

inline constexpr std::array<FlameSpawnOffset, 4> kEfreetFlameSpawnOffsets = {{
    {0, -40}, {32, -12}, {0, 12}, {-32, -12}
}};

struct FlameParticleSlot {
    uint8_t status_00{kFlameSlotFreeSentinel};
    uint8_t tile_01{0};
    int32_t pos_x_02{0};
    int32_t pos_y_06{0};
    int32_t vel_x_0a{0};
    int32_t vel_y_0e{0};
    uint8_t timer_12{0};
    uint8_t anim_idx_13{0};
};

struct FlameVertexTuple {
    uint8_t tile_id{0};
    int8_t  dx{0};
    int8_t  dy{0};
};

struct FlameVertexBuffer {
    uint8_t header{0};
    uint8_t count{0};
    std::array<FlameVertexTuple, kFlameParticleCount> tuples{};
};

inline uint16_t ComputeEfreetFlameDamageWord(uint8_t efreet_jewel_rank) noexcept {
    return static_cast<uint16_t>(
        kFlameBaseDamageWord + ((static_cast<uint16_t>(efreet_jewel_rank) + 1u) >> 2));
}

inline uint8_t ExportFlameVertices(
    const std::array<FlameParticleSlot, kFlameParticleCount>& slots,
    uint16_t efreet_facing,
    FlameVertexBuffer& out_buf) noexcept {
    out_buf.count = 0;
    for (const auto& s : slots) {
        if (s.status_00 != 0) continue;
        int8_t dx = static_cast<int8_t>((s.pos_x_02 >> 16) & 0xFF);
        if (efreet_facing == 3) {
            dx = static_cast<int8_t>(-dx);
        }
        int8_t dy = static_cast<int8_t>((s.pos_y_06 >> 16) & 0xFF);
        out_buf.tuples[out_buf.count++] = {s.tile_01, dx, dy};
    }
    if (out_buf.count == 0) {
        out_buf.header = 0;
        return 0;
    }
    uint8_t hdr = static_cast<uint8_t>(out_buf.count - 1u);
    if (efreet_facing != 2) {
        hdr |= 0x80u;
    }
    out_buf.header = hdr;
    return out_buf.count;
}

} // namespace Thor::AI
```
