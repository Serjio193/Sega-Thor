# STANDALONE HANDOFF: Milestone M15.501 — Bow Spirit (0x1E) 4-Node Vine Tether Physics, Round-Robin Target Scanner & Nearest Manhattan Finder

## 1. Subsystem Overview
- **ROM Address Range:** `0x014242..0x014522` (736 bytes, 220 instructions).
- **Core Responsibilities:**
  1. `0x014242..0x0143C8` (`AI_BowSpirit_VineTetherPhysics`, 390 bytes, 108 instructions):
     - Decrements 4 vine node timers at `$FF19AC, $FF19B2, $FF19B8, $FF19BE` (if `> 0`).
     - Solves 4-node 3D distance constraints using a 2-pass FABRIK-style iterative relaxation algorithm via helper `0x00F160`:
       - **Forward Pass:** Anchors at Bow (`a6`), constraining Node 3 -> Node 2 -> Node 1 -> Node 0 with radius `d7`.
       - **Backward Pass:** Anchors at Ali (`a4`), constraining Node 0 (radius `d7 - 2`) -> Node 1 -> Node 2 -> Node 3 -> Bow (`a6`, radius `d7`).
       - **Early Exit:** Any link satisfying the distance constraint (`dist <= radius`) sets Carry Clear (`bcc`) and terminates solver early.
     - Enforces Bow vertical elevation relative to Ali base height (`+$52(a4)`).
  2. `0x0143C8..0x014494` (`AI_BowSpirit_RoundRobinTargetScanner`, 204 bytes, 62 instructions):
     - Advances round-robin cursor `+$aa(a6)` by `0xBC` (188 bytes) across 17 entity slots in Pool 2 (`$FF1CD8..$FF2954`), wrapping back to `0` upon reaching or exceeding `$FF2954`.
     - Filters candidate slots: `status > 0`, `+$38` bit 3 == 0 (non-immune), and `+$3a` bit 0 == 0 (targetable, bypassed if `status == 0x66`).
     - Checks 3D Euclidean distance from Ali (`$FF1C1C`) to enemy (incorporating `+$4a(a0) >> 2` elevation offset) against threshold `0x36` (54 px) via `0x00F0EC`.
     - On qualifying target: projects 3D velocity vector with speed `#$60` via `0x00F10C`, computes facing angle via `0x00E338`, sets animation `#$22`, timer `#12`, substate `#3` (Lunge Attack), and returns Carry Set (`ori.b #1, ccr`).
     - Returns Carry Clear (`andi.b #$fe, ccr`) if slot fails validation.
  3. `0x014494..0x014522` (`AI_BowSpirit_FindNearestPool2Enemy`, 142 bytes, 50 instructions):
     - Exhaustively scans all 17 slots in Pool 2 (`$FF1CD8..$FF2954`) relative to Ali position `+$4e/+$52/+$56(a4)`.
     - Evaluates active enemy filters identical to the round-robin scanner.
     - Computes exact integer 3D Manhattan metric: `|dx| + |dy| + |dz|`.
     - Tracks minimum distance (`d0`) and closest slot pointer (`a0`).
     - Returns Carry Clear (`andi.b #$fe, ccr`) on success with `a0` and `d0`, or Carry Set (`ori.b #1, ccr`) if no eligible enemy exists.

## 2. ROM Evidence Reference
- Evidence JSON: `data/thor-evidence/rom_bow_tether_target_m15_501.json`
- Extractor: `tools/extract_rom_bow_tether_target.py`
- Unit Tests: `tests/rom_bow_tether_target_test.py`

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <cstdint>
#include <array>
#include <optional>
#include <cmath>
#include <algorithm>

namespace Thor::AI::BowSpirit {

struct Vec3 {
    int16_t x{0};
    int16_t y{0};
    int16_t z{0};
};

struct TetherNode {
    Vec3 pos{};
    int16_t timer{0};
};

struct LungeParameters {
    uint8_t target_slot{0};
    uint16_t speed{0x60};
    uint16_t anim_id{0x22};
    uint16_t timer{12};
    uint8_t substate{3};
};

struct Pool2EntitySlot {
    int16_t status{0};
    uint8_t flags_38{0};
    uint8_t flags_3a{0};
    Vec3 pos{};
    int16_t height{0};
};

constexpr size_t POOL2_SLOT_COUNT = 17;
constexpr uint32_t POOL2_BASE_ADDR = 0xFF1CD8;
constexpr uint32_t POOL2_END_ADDR = 0xFF2954;
constexpr uint16_t POOL2_SLOT_SIZE = 0xBC; // 188 bytes

/// Step node timers: decrements positive timers (0x01425C..0x014270).
inline void StepNodeTimers(std::array<TetherNode, 4>& nodes) noexcept {
    for (auto& node : nodes) {
        if (node.timer > 0) {
            --node.timer;
        }
    }
}

/// 3D distance constraint solver helper (ROM 0x00F160).
/// Returns true if constraint violated and clamped (Carry Set), false if satisfied (Carry Clear).
inline bool ConstrainDistance3D(const Vec3& anchor, Vec3& candidate, int16_t radius) noexcept {
    int32_t dx = static_cast<int32_t>(candidate.x) - anchor.x;
    int32_t dy = static_cast<int32_t>(candidate.y) - anchor.y;
    int32_t dz = static_cast<int32_t>(candidate.z) - anchor.z;
    int32_t dist_sq = dx * dx + dy * dy + dz * dz;
    int32_t dist = static_cast<int32_t>(std::sqrt(dist_sq));

    if (dist <= radius) {
        return false; // Carry Clear
    }

    candidate.x = static_cast<int16_t>(anchor.x + (dx * radius) / dist);
    candidate.y = static_cast<int16_t>(anchor.y + (dy * radius) / dist);
    candidate.z = static_cast<int16_t>(anchor.z + (dz * radius) / dist);
    return true; // Carry Set
}

/// 2-Pass 4-Node Vine Tether Physics Solver (ROM 0x014272..0x0143C6).
inline bool SolveVineTetherPhysics(
    const Vec3& ali_pos,
    int16_t ali_base_y,
    Vec3& bow_pos,
    std::array<TetherNode, 4>& nodes,
    int16_t max_radius
) noexcept {
    // Forward pass: Bow -> Node 3 -> Node 2 -> Node 1 -> Node 0
    Vec3 anchor = bow_pos;
    for (int i = 3; i >= 0; --i) {
        if (!ConstrainDistance3D(anchor, nodes[i].pos, max_radius)) {
            return false;
        }
        anchor = nodes[i].pos;
    }

    // Backward pass: Ali -> Node 0 (radius - 2) -> Node 1 -> Node 2 -> Node 3 -> Bow
    anchor = ali_pos;
    if (!ConstrainDistance3D(anchor, nodes[0].pos, static_cast<int16_t>(max_radius - 2))) {
        return false;
    }
    anchor = nodes[0].pos;

    for (size_t i = 1; i < 4; ++i) {
        if (!ConstrainDistance3D(anchor, nodes[i].pos, max_radius)) {
            return false;
        }
        anchor = nodes[i].pos;
    }

    if (!ConstrainDistance3D(anchor, bow_pos, max_radius)) {
        return false;
    }

    // Elevation clamp relative to Ali base height
    int16_t clamped_y = static_cast<int16_t>(bow_pos.y - 2);
    if (clamped_y <= ali_base_y) {
        clamped_y = ali_base_y;
    }
    return true;
}

/// Round-robin Pool 2 enemy scanner and lunge trigger (ROM 0x0143C8..0x014494).
inline std::optional<LungeParameters> ScanRoundRobinTarget(
    uint16_t& cursor_offset,
    const std::array<Pool2EntitySlot, POOL2_SLOT_COUNT>& pool,
    const Vec3& ali_pos
) noexcept {
    cursor_offset += POOL2_SLOT_SIZE;
    uint32_t slot_addr = POOL2_BASE_ADDR + cursor_offset;
    if (slot_addr < POOL2_BASE_ADDR || slot_addr >= POOL2_END_ADDR) {
        cursor_offset = 0;
    }

    size_t slot_idx = cursor_offset / POOL2_SLOT_SIZE;
    const auto& ent = pool[slot_idx];

    if (ent.status <= 0 || (ent.flags_38 & 0x08) != 0) {
        return std::nullopt;
    }
    if (ent.status != 0x66 && (ent.flags_3a & 0x01) != 0) {
        return std::nullopt;
    }

    int32_t enemy_z_adj = ent.pos.z + (ent.height >> 2);
    int32_t dx = ent.pos.x - ali_pos.x;
    int32_t dy = ent.pos.y - ali_pos.y;
    int32_t dz = enemy_z_adj - ali_pos.z;
    int32_t dist = static_cast<int32_t>(std::sqrt(dx * dx + dy * dy + dz * dz));

    if (dist > 0x36) { // 54 px threshold
        return std::nullopt;
    }

    return LungeParameters{
        .target_slot = static_cast<uint8_t>(slot_idx),
        .speed = 0x60,
        .anim_id = 0x22,
        .timer = 12,
        .substate = 3
    };
}

/// Nearest Pool 2 enemy finder via 3D Manhattan metric (ROM 0x014494..0x014522).
inline std::optional<std::pair<size_t, uint16_t>> FindNearestPool2Enemy(
    const std::array<Pool2EntitySlot, POOL2_SLOT_COUNT>& pool,
    const Vec3& ali_pos
) noexcept {
    uint16_t best_dist = 0xFFFF;
    std::optional<size_t> best_idx;

    for (size_t i = 0; i < POOL2_SLOT_COUNT; ++i) {
        const auto& ent = pool[i];
        if (ent.status <= 0 || (ent.flags_38 & 0x08) != 0) {
            continue;
        }
        if (ent.status != 0x66 && (ent.flags_3a & 0x01) != 0) {
            continue;
        }

        uint16_t manhattan = static_cast<uint16_t>(
            std::abs(ent.pos.x - ali_pos.x) +
            std::abs(ent.pos.y - ali_pos.y) +
            std::abs(ent.pos.z - ali_pos.z)
        );

        if (manhattan < best_dist) {
            best_dist = manhattan;
            best_idx = i;
        }
    }

    if (!best_idx.has_value() || best_dist == 0xFFFF) {
        return std::nullopt; // Carry Set (failure)
    }

    return std::make_pair(*best_idx, best_dist); // Carry Clear (success)
}

} // namespace Thor::AI::BowSpirit
```
