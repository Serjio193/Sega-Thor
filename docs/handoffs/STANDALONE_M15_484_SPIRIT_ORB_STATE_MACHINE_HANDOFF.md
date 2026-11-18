# STANDALONE HANDOFF: Milestone M15.484 — ROM Spirit Orb State 0 Scan, Catalyst Transmutation, Trail Particle Emitter & Tilemap Sampler

## 1. Subsystem Overview
- **ROM Address Range:** `0x01171C..0x011A4C` (816 bytes: 234 instructions across `0x01171C..0x011A3A` + 18-byte embedded table `0x011A3A..0x011A4C`).
- **Core Responsibilities:**
  1. `0x01171C..0x011810` (`AI_SpiritOrb_State0_ElementalMediumAndCatalystScan`, 244 bytes, 69 insns):
     - Checks cancel/force-summon flag `$FF1990`; if non-zero, clears `$FF1990` and enters `0x011818` using the existing `$FF19A6` spirit index.
     - Decays 32-bit fixed Z-elevation `+$10(a6)` by `0x00018000` per frame (clamped `>= 0`).
     - If `+$10(a6)` was non-zero before decay: tests `(+$40(a6) | +$41(a6))`. If bit 6 is clear, branches to entity AABB scan (`0x0117B6`); if bit 6 is set and bit 3 is set, despawns orb via `0x0119C8` (`+$0(a6) = 0`); if bit 6 is set and bit 3 is clear, falls through to medium probes.
     - Medium probes (`0x01175A..0x0117B6`): checks `+$39(a6)` bit 1 (`$FF19A6 = 0`), `$FF17C2 <= 3` with `+$6F(a6)` bit 5 (`$FF19A6 = $FF17C2`), and `$FF17C3 <= 3` with `+$6F(a6)` bit 6 (`$FF19A6 = $FF17C3`); otherwise falls directly to State 2 rebound (`0x011832`).
     - Entity AABB scan (`0x0117B6..0x011810`): calls `0x00B922` with `d0.b = 0xFC` (mask skipping slots 0..1, scanning slots 2..20) and box `x in [-12, +12]`, `y in [-10, +10]`, `z in [0, +24]`. On miss (Carry set), branches to `0x011866`. On hit: Torch (`$0064`) sets `+$02(a0) = 1`; Crystal (`$00B4`) sets `+$A6(a0) = 0xFF`; `+$39(a0)` bit 3 selects spirit `1`; `+$3C(a0)` bits `0..3` select spirits `0..3`; otherwise rebounds to State 2 (`0x011832`).
  2. `0x011810..0x011866` (`AI_SpiritOrb_TransitionToState1Or2`, 86 bytes, 20 insns):
     - Stores `d0.b` into `$FF19A6` (at `0x011810`), tests bit `$FF19A6` of spirit unlock mask `$FF0D7A` and bit 0 of inhibit flag `$FF16F1` (at `0x011818`). Falls back to State 2 (`0x011832`) if locked or inhibited; otherwise enters State 1 (`d0 = 1`).
     - Clears velocities `+$4E(a6)` / `+$52(a6)`, writes `+$04(a6) = state`, `+$2A(a6) = 2 * state`, adds `12` (`0x0C`) to integer elevation word `+$10(a6)`, clears timer `+$02(a6)`, sets bit 3 of `+$36(a6)`, plays SFX `#$8004D2` via `0x00E7FC`, and tail-jumps to `AI_SpiritOrb_Main` (`0x0115C8`).
  3. `0x011866..0x011938` (`AI_SpiritOrb_ScanItemCatalysts`, 210 bytes, 57 insns):
     - Calls FX/item AABB scanner `0x00B9EC` with box `x in [-4, +4]`, `y in [-4, +4]`, `z_offset = 0`. On miss or `+$00(a0) != 0x0008`, branches to `0x011938`.
     - If `$FF16F1` bit 0 is set, rebounds to State 2 (`0x011832`).
     - Checks unlocked spirits `0..3` in `$FF0D7A`:
       - Direct elemental catalysts: `$12` (spirit 0), `$13` (spirit 1), `$14` (spirit 2), `$15` (spirit 3) -> `0x011810`.
       - Item transmutations: `$1A -> $21` (spirit 0), `$1B -> $05` with `+$52(a0).b = $14` (spirit 1), `$1C -> $11` (spirit 2), `$1D -> $2A` and `$27 -> $26` (spirit 3). Transmutations write `+$32(a0) = new_subtype`, `+$22(a0) = $1E`, clear bit 1 of `+$36(a0)`, and branch to `0x011810`.
       - Unmatched Type `0x0008` items rebound to State 2 (`0x011832`).
  4. `0x011938..0x0119CE` (`AI_SpiritOrb_IntegrateAndUpdateTrail`, 150 bytes, 46 insns):
     - Calls `0x00E36E`, integrates `+$4E/+$52` into `+$72/+$76`, and checks viewport culling `0x00E5AE` (margin `0x10`; out-of-bounds despawns at `0x0119C8`).
     - Decrements signed cooldown `+$AC(a6)`; on `< 0`, scans 6-slot trail array `+$A6..+$AB(a6)` for a `0` byte, initializes slot to `1`, reloads `+$AC(a6) = 2` (3-frame cadence), toggles `+$AD(a6) ^= 1`, and sets bit 7 on the slot if `+$AD(a6)` is non-zero (`0x81`).
     - Iterates over the 6 slots calling `0x0119EC` to emit 3-byte vertices into `$FF19A9..` and advances active slots via `addq.b #1, (a0)`. Writes header `0x80 | (count - 1)` at `$FF19A8`, sets `+$06(a6) = $16`, `+$62(a6) = $00FF19A8` (or clears `+$06(a6) = 0` if `count == 0`).
  5. `0x0119CE..0x0119EC` (`Tilemap_SampleLowNibbleAttribute`, 30 bytes, 11 insns):
     - Computes `d4 = ((y >> 3) << *(a0 + 8)) + (x >> 3)` (signed `asr.w #3`), reads `*(uint8_t*)(*(a0) + d4)`, and returns `val & 0x0F`.
  6. `0x0119EC..0x011A4C` (`AI_SpiritOrb_EmitTrailParticleVertex` + `SpiritOrbTrailTileTable`, 96 bytes: 78 bytes code + 18-byte table at `0x011A3A..0x011A4C`):
     - Extracts bit 7 sign (`lat = +1` if clear, `lat = -2` if set) and 7-bit `age`. If `age >= 18`, writes `0xFF` to `(a0)` and decrements `d4` so caller's `addq.b #1` wraps `0xFF -> 0x00`.
     - Otherwise reads `tile_id = SpiritOrbTrailTileTable[age]`, computes radial distance `dist = 3 * age + 12`, rotates `(dist, lat)` by 4-way facing `+$66(a6)` (`0`=Up, `1`=Right, `2`=Down, `3`=Left), subtracts `12` from `dy`, and emits `(tile_id, dx, dy)`.

## 2. ROM Evidence Reference
- Evidence JSON: `data/thor-evidence/rom_spirit_orb_state_machine_m15_484.json`
- Extractor: `tools/extract_rom_spirit_orb_state_machine.py`
- Test: `tests/rom_spirit_orb_state_machine_test.py`

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace Thor::AI {

inline constexpr std::array<uint8_t, 18> SPIRIT_ORB_TRAIL_TILE_TABLE = {
    16, 8, 16, 9, 16, 10, 16, 11, 12, 13, 12, 11, 14, 15, 14, 17, 18, 19
};

enum class OrbFacing : uint8_t {
    Up = 0,
    Right = 1,
    Down = 2,
    Left = 3
};

struct TrailParticleVertex {
    uint8_t tile_id{0};
    int8_t dx{0};
    int8_t dy{0};
};

struct TrailVertexBuffer {
    uint8_t header{0};
    uint8_t count{0};
    std::array<TrailParticleVertex, 6> vertices{};
};

struct SpiritOrbState {
    uint16_t type_00{0};
    uint16_t timer_02{0};
    uint16_t state_04{0};
    uint16_t anim_06{0};
    int16_t x_08{0};
    int16_t y_0c{0};
    int32_t z_10_fixed{0};
    int16_t z_14{0};
    uint16_t field_2a{0};
    uint8_t flags_36{0};
    uint8_t flags_39{0};
    uint8_t flags_40{0};
    uint8_t flags_41{0};
    int32_t vel_x_4e{0};
    int32_t vel_y_52{0};
    OrbFacing facing_66{OrbFacing::Up};
    uint8_t flags_6f{0};
    int32_t pos_x_72{0};
    int32_t pos_y_76{0};
    std::array<uint8_t, 6> trail_slots_a6{};
    int8_t trail_timer_ac{0};
    uint8_t trail_sign_ad{0};
};

struct SpiritOrbGlobalContext {
    uint8_t spirit_unlock_mask_ff0d7a{0};
    uint8_t env_inhibit_flags_ff16f1{0};
    uint8_t medium_probe_a_ff17c2{0xFF};
    uint8_t medium_probe_b_ff17c3{0xFF};
    uint8_t cancel_flag_ff1990{0};
    uint8_t selected_spirit_ff19a6{0};
};

struct CatalystItemRecord {
    uint16_t type_00{0};
    uint16_t timer_22{0};
    uint16_t subtype_32{0};
    uint8_t flags_36{0};
    uint8_t field_52{0};
};

inline uint8_t SampleTilemapLowNibble(
    std::span<const uint8_t> tilemap_bytes,
    uint16_t stride_shift,
    int16_t x,
    int16_t y) noexcept {
    int16_t row = static_cast<int16_t>((y >> 3) << stride_shift);
    int16_t col = static_cast<int16_t>(x >> 3);
    int16_t offset = static_cast<int16_t>(row + col);
    return static_cast<uint8_t>(tilemap_bytes[static_cast<size_t>(offset)] & 0x0F);
}

inline std::optional<TrailParticleVertex> EmitTrailParticleVertex(
    uint8_t& slot_age,
    OrbFacing facing) noexcept {
    int8_t lat = (slot_age & 0x80u) ? int8_t{-2} : int8_t{1};
    uint8_t age = static_cast<uint8_t>(slot_age & 0x7Fu);
    if (age >= SPIRIT_ORB_TRAIL_TILE_TABLE.size()) {
        slot_age = 0xFF;
        return std::nullopt;
    }

    uint8_t tile_id = SPIRIT_ORB_TRAIL_TILE_TABLE[age];
    int8_t dist = static_cast<int8_t>(age * 3 + 12);
    int8_t d1 = dist;
    int8_t d2 = lat;

    switch (facing) {
        case OrbFacing::Up:
            d1 = lat;
            d2 = dist;
            break;
        case OrbFacing::Right:
            d1 = static_cast<int8_t>(-dist);
            break;
        case OrbFacing::Left:
            break;
        case OrbFacing::Down:
        default:
            d1 = lat;
            d2 = static_cast<int8_t>(-dist);
            break;
    }

    return TrailParticleVertex{
        tile_id,
        d1,
        static_cast<int8_t>(d2 - 12)
    };
}

inline TrailVertexBuffer UpdateOrbTrailParticles(SpiritOrbState& orb) noexcept {
    orb.trail_timer_ac = static_cast<int8_t>(orb.trail_timer_ac - 1);
    if (orb.trail_timer_ac < 0) {
        for (uint8_t& slot : orb.trail_slots_a6) {
            if (slot == 0) {
                orb.trail_timer_ac = 2;
                orb.trail_sign_ad ^= 1u;
                slot = static_cast<uint8_t>(1u | (orb.trail_sign_ad ? 0x80u : 0x00u));
                break;
            }
        }
    }

    TrailVertexBuffer out{};
    for (uint8_t& slot : orb.trail_slots_a6) {
        if (slot != 0) {
            if (auto vert = EmitTrailParticleVertex(slot, orb.facing_66)) {
                out.vertices[out.count++] = *vert;
            }
            slot = static_cast<uint8_t>(slot + 1u);
        }
    }

    if (out.count == 0) {
        orb.anim_06 = 0;
        return out;
    }

    out.header = static_cast<uint8_t>(0x80u | (out.count - 1u));
    orb.anim_06 = 0x16;
    return out;
}

inline bool TryMatchOrTransmuteItemCatalyst(
    CatalystItemRecord& item,
    SpiritOrbGlobalContext& ctx) noexcept {
    if (item.type_00 != 0x0008 || (ctx.env_inhibit_flags_ff16f1 & 0x01u) != 0) {
        return false;
    }

    const uint8_t sub = static_cast<uint8_t>(item.subtype_32 & 0xFFu);
    auto unlocked = [&](uint8_t spirit) noexcept -> bool {
        return ((ctx.spirit_unlock_mask_ff0d7a >> spirit) & 1u) != 0;
    };

    for (uint8_t spirit = 0; spirit < 4; ++spirit) {
        if (unlocked(spirit) && sub == static_cast<uint8_t>(0x12u + spirit)) {
            ctx.selected_spirit_ff19a6 = spirit;
            return true;
        }
    }

    struct TransmuteRule {
        uint8_t spirit;
        uint8_t from_sub;
        uint8_t to_sub;
        uint8_t extra_52;
    };
    constexpr std::array<TransmuteRule, 5> rules = {{
        {0, 0x1A, 0x21, 0x00},
        {1, 0x1B, 0x05, 0x14},
        {2, 0x1C, 0x11, 0x00},
        {3, 0x1D, 0x2A, 0x00},
        {3, 0x27, 0x26, 0x00},
    }};

    for (const auto& rule : rules) {
        if (unlocked(rule.spirit) && sub == rule.from_sub) {
            if (rule.extra_52 != 0) {
                item.field_52 = rule.extra_52;
            }
            item.subtype_32 = rule.to_sub;
            item.timer_22 = 0x1E;
            item.flags_36 &= static_cast<uint8_t>(~0x02u);
            ctx.selected_spirit_ff19a6 = rule.spirit;
            return true;
        }
    }

    return false;
}

inline void CommitOrbStateTransition(
    SpiritOrbState& orb,
    uint16_t target_state) noexcept {
    orb.vel_x_4e = 0;
    orb.vel_y_52 = 0;
    orb.state_04 = target_state;
    orb.field_2a = static_cast<uint16_t>(target_state * 2u);
    int32_t upper_z = (orb.z_10_fixed >> 16) + 12;
    orb.z_10_fixed = (upper_z << 16) | (orb.z_10_fixed & 0xFFFF);
    orb.timer_02 = 0;
    orb.flags_36 |= 0x08u;
}

} // namespace Thor::AI
```
