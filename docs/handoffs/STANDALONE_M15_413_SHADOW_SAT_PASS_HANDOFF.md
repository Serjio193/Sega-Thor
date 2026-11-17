# Standalone Handoff: M15.413 ROM Shadow & Special Effects SAT Composition Pass

## Summary
- **Source Address Range:** `0x00A6A0..0x00A7E6` (326 bytes total)
- **Target Area:** Shadow & Reflection Sprite Rendering (`shadow_sprites.cpp` / `sat_shadow_pass.hpp`)
- **Evidence File:** `data/thor-evidence/rom_shadow_sat_pass_m15_413.json`
- **Unit Test File:** `tests/rom_shadow_sat_pass_test.py`

## Routine Signatures & Memory Mappings

### 1. `0x00A6A0`: Shadow Composition Pipeline Entry
- **M68K Logic:**
  - `0x00A6A0`: calls pre-composition reset helper `bsr.w $a342`.
  - `0x00A6A4..0x00A6C2`: Group 1 (`$FF2954`, 4 slots of 90 bytes).
    * Tests `type > 0`.
    * Checks shadow/reflection flag: `btst.b #2, $3a(a6)`.
    * If bit 2 is set, dispatches to render `$AD76` or `$A9A0`.
    * If bit 2 is clear, skips the entity.
  - `0x00A6C6..0x00A738`: Group 2 Shadow Depth Sorter (`$FF19E8`, 21 slots of 188 bytes):
    * Builds inactive mask in `d7`: sets bit `i` if `type <= 0` or if `(flags_3a & 0x04) == 0`.
    * Runs descending-Y selection sort loop over active shadow slots.
    * Dispatches each shadow in top-down depth order to `$AD76` (normal SAT) or `$A9A0` (custom SAT).
  - `0x00A73C..0x00A750`: Group 4 (`$FF2ABC`, 8 slots of 90 bytes).
  - `0x00A754..0x00A768`: Group 3 (`$FF2D8C`, 6 slots of 90 bytes).
  - `0x00A76C..0x00A784`: SAT Link Terminator: writes link terminator `0` to `-5(a6, d7.w)` in SAT buffer `$FF13CC` and returns.

## C++20 Cleanroom Interface Guidance
```cpp
namespace oasis::rendering {

struct ShadowPassEntity {
    int16_t type{0};
    int16_t y{0};
    uint8_t flags_3a{0};
    uint8_t flags_36{0};

    [[nodiscard]] constexpr bool has_shadow() const noexcept {
        return (flags_3a & 0x04) != 0;
    }
};

inline void dispatch_shadow_pass_group_2(
    std::span<const ShadowPassEntity, 21> group2,
    auto&& render_callback) {
    uint32_t processed_mask = 0;
    for (size_t i = 0; i < 21; ++i) {
        if (group2[i].type <= 0 || !group2[i].has_shadow()) {
            processed_mask |= (1u << i);
        }
    }

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
