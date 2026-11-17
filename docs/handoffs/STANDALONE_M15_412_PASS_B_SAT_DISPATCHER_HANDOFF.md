# Standalone Handoff: M15.412 ROM Pass B Master SAT Dispatcher & Cache Mask Reuse

## Summary
- **Source Address Range:** `0x00A524..0x00A69A` (374 bytes total)
- **Target Area:** Sprite Rendering Pipeline, SAT Composition & Depth Sorter (`sprite_composer.cpp` / `sat_dispatcher.hpp`)
- **Evidence File:** `data/thor-evidence/rom_pass_b_sat_dispatcher_m15_412.json`
- **Unit Test File:** `tests/rom_pass_b_sat_dispatcher_test.py`

## Routine Signatures & Memory Mappings

### 1. `0x00A524`: Pass B Multi-Pool Dispatcher
- **M68K Logic:**
  - `0x00A524..0x00A538`: Group 1 (`$FF2954`, 4 slots of 90 bytes). Dispatches to `$AD76` (normal SAT) or `$A9A0` (custom SAT, when `$36` bit 0 set).
  - `0x00A53C..0x00A54C`: Special pool (`$FF19C8`, 4 slots of 8 bytes). Dispatches through jump table at `$A69A`.
  - `0x00A550..0x00A5A2`: Group 2 Depth Sorter (`$FF19E8`, 21 slots of 188 bytes):
    * **Key Innovation**: Reuses cached active entity bitmask from `$FF188E.l` directly into `d7` without re-scanning all 21 slots.
    * Runs descending-Y selection sort loop: iteratively identifies the entity with the maximum Y coordinate (`+0x0C(a6)`) among unrendered slots, marks it in `d7`, and dispatches to `$AD76` or `$A9A0`.
    * Loops until all active entities are emitted in top-down isometric layering order.
  - `0x00A5A6..0x00A5BA`: Group 4 (`$FF2ABC`, 8 slots of 90 bytes).
  - `0x00A5BE..0x00A5D2`: Group 3 (`$FF2D8C`, 6 slots of 90 bytes).
  - `0x00A5D6..0x00A600`: Foreground Player Entity (`$FF1995` flag, `$FF19E8[0]`).
  - `0x00A604..0x00A624`: SAT link terminator: writes link terminator `0` to `-5(a6, d7.w)` in SAT buffer `$FF13CC` and sets VDP sync dirty flags `ori.b #3, $FF164E.l`.

## C++20 Cleanroom Interface Guidance
```cpp
namespace oasis::rendering {

inline void dispatch_pass_b_group_2(
    std::span<const SatEntity, 21> group2,
    uint32_t cached_mask,
    auto&& render_callback) {
    uint32_t processed_mask = cached_mask;

    while (true) {
        int16_t max_y = -128;
        int best_slot = -1;

        for (int i = 20; i >= 0; --i) {
            if (processed_mask & (1u << i)) {
                continue;
            }
            if (group2[i].y > max_y) {
                max_y = group2[i].y;
                best_slot = i;
            }
        }

        if (best_slot < 0) {
            break;
        }

        processed_mask |= (1u << best_slot);
        const auto& ent = group2[best_slot];
        const SatTarget target = (ent.flags_36 & 0x01) ? SatTarget::Custom : SatTarget::Normal;
        render_callback(best_slot, ent, target);
    }
}

} // namespace oasis::rendering
```
