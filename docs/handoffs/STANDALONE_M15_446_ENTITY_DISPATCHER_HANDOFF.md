# STANDALONE HANDOFF: M15.446 — ROM Master Group 2 Entity Tick Dispatcher, Post-Tick Isometric Bounding Box Calculator & 89-Entry Action Function Pointer Table

- **Source Routine:** `master_group2_entity_tick_dispatcher` (`0x00DEEC..0x00E0BA`, 462 bytes).
- **Domain:** Entity Core Loop / Type Action Dispatcher / Isometric Footprint Projection.
- **Evidence Reference:** `data/thor-evidence/rom_entity_dispatcher_m15_446.json`.
- **Test Reference:** `tests/rom_entity_dispatcher_test.py`.

---

## 1. Architectural Role & Context

In Beyond Oasis / The Story of Thor, every active non-player dynamic entity (including companion spirits Bow, Ditte, Efreet, Shadow, general NPCs, interactive objects, and enemies) is managed within Group 2 of the master entity pool (`$FF1AA4`, 20 slots, stride 188 bytes / `$BC`).

Routine `0x00DEEC` is the central game tick dispatcher for all Group 2 entities. It executes once per game frame during active gameplay:
1. **Pause Gate:** Tests bit 1 on `$FF17B8.l` (`btst.b #1, $ff17b8.l`). If paused, the entire dispatch loop is skipped and returns immediately via `rts`.
2. **Pool Sweep:** Sweeps through all 20 entity slots (`moveq #$13, d7`). For each entity:
   - Evaluates type at `+$00(a6)`. If `<= 0` (inactive/dead) or `< 0x16` (types below 22 decimal), skips to the next entity slot.
   - Dispatches entity action via the 89-entry 32-bit function pointer table at `0x00DF54` (`movea.l $df28(pc, d6.w), a0; jsr (a0)`), where `d6 = 2 * entity_type`. Unmapped entity types jump to dummy handler `0x00E0B8` (`rts`).
3. **Post-Tick Isometric Ground Footprint Projection:** After each entity tick callback returns, the routine updates the entity's effective 2D isometric collision box `(+$42, +$44)` from its raw extent bytes `+$48(a6)` (raw X) and `+$49(a6)` (raw Y):
   - Tests orientation flag bit 0 at `+$17(a6)`. If set, swaps X and Y dimensions (`exg d1, d2`).
   - Stores effective collision half-width: `+$42(a6) = d1`.
   - Projects isometric height using a 3/4 compression ratio:
     - `d2 = (d2 + 1) >> 1`
     - `+$44(a6) = d2 + (d2 >> 1)`
   - Advances to the next entity slot (`lea $bc(a6), a6`) and loops via `dbra d7, ...`.

---

## 2. Register Conventions & Struct Offsets

| Register / Field | Semantic Meaning |
|---|---|
| `a6` | Pointer to current entity slot in Group 2 pool (`$FF1AA4 + i * 188`) |
| `d7` | Loop counter (initialized to `#$13` for 20 slots) |
| `d6` | Entity type word from `+$00(a6)`, doubled to index 4-byte jump table |
| `+$00(a6)` | Entity type word (active if `> 0`, must be `>= 0x16` for Group 2 dispatch) |
| `+$17(a6)` | Entity state bitmask (bit 0: swap X/Y bounding extents) |
| `+$42(a6)` | Word: effective collision half-width |
| `+$44(a6)` | Word: effective collision half-height (3/4 isometric projected) |
| `+$48(a6)` | Byte: raw collision X half-extent |
| `+$49(a6)` | Byte: raw collision Y half-extent |

---

## 3. 89-Entry Action Function Pointer Table (`0x00DF54..0x00E0B8`)

The jump table maps even entity types from `0x16` to `0xC6` directly to their ROM tick handlers.
Key mapped entities:
- `Type 0x16`: `0x0115C8` — Bow (Spirit of Plant)
- `Type 0x18`: `0x011B4E` — Ditte (Spirit of Water)
- `Type 0x1A`: `0x01254C` — Efreet (Spirit of Fire)
- `Type 0x1C`: `0x012F1E` — Shadow (Spirit of Darkness)
- `Type 0x1E`: `0x01387C` — Spirit interaction handler
- `Type 0x2E`: `0x00E25C` — General NPC / dynamic interactive entity
- Unmapped types point to `0x00E0B8` (dummy `rts` return).

---

## 4. C++20 Cleanroom Interface Design

```cpp
#pragma once

#include <cstdint>
#include <span>
#include <array>
#include <functional>

namespace thor::entity {

struct Group2Entity {
    int16_t type{0};            // +$00: Type ID
    uint8_t flags_17{0};        // +$17: Bit 0 = swap X/Y extents
    int16_t effective_half_w{0};// +$42: Effective collision half-width
    int16_t effective_half_h{0};// +$44: Effective collision half-height
    uint8_t raw_extent_x{0};    // +$48: Raw X half-extent
    uint8_t raw_extent_y{0};    // +$49: Raw Y half-extent
};

constexpr size_t kMinGroup2Type = 0x16;
constexpr size_t kMaxGroup2Type = 0xC6;
constexpr size_t kTableEntries = 89;

inline std::pair<int16_t, int16_t> project_isometric_footprint(
    uint8_t raw_x,
    uint8_t raw_y,
    bool swap_xy
) noexcept {
    uint16_t d1 = raw_x;
    uint16_t d2 = raw_y;
    if (swap_xy) {
        std::swap(d1, d2);
    }
    int16_t eff_w = static_cast<int16_t>(d1);
    uint16_t step1 = (d2 + 1) >> 1;
    uint16_t step2 = step1 >> 1;
    int16_t eff_h = static_cast<int16_t>(step1 + step2);
    return {eff_w, eff_h};
}

using EntityTickFn = void (*)(Group2Entity&);

inline void dispatch_group2_entities(
    std::span<Group2Entity, 20> pool,
    bool is_paused,
    const std::array<EntityTickFn, kTableEntries>& jump_table
) {
    if (is_paused) return;

    for (auto& entity : pool) {
        if (entity.type < static_cast<int16_t>(kMinGroup2Type)) continue;

        size_t index = static_cast<size_t>(entity.type - kMinGroup2Type) >> 1;
        if (index < jump_table.size() && jump_table[index] != nullptr) {
            jump_table[index](entity);
        }

        bool swap_xy = (entity.flags_17 & 0x01) != 0;
        auto [eff_w, eff_h] = project_isometric_footprint(
            entity.raw_extent_x,
            entity.raw_extent_y,
            swap_xy
        );
        entity.effective_half_w = eff_w;
        entity.effective_half_h = eff_h;
    }
}

} // namespace thor::entity
```
