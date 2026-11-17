# Standalone Handoff: M15.410 ROM Master Sprite Composition, Y-Depth Sorter & SAT Dispatcher

## Summary
- **Source Address Range:** `0x00A196..0x00A336` (416 bytes total)
- **Target Area:** Sprite Rendering Pipeline, SAT Composition & Depth Sorter (`sprite_composer.cpp` / `sat_dispatcher.hpp`)
- **Evidence File:** `data/thor-evidence/rom_sprite_sat_dispatcher_m15_410.json`
- **Unit Test File:** `tests/rom_sprite_sat_dispatcher_test.py`

## Routine Signatures & Memory Mappings

### 1. `0x00A196`: Master Sprite Composition Pipeline Entry
- **M68K Logic:**
  ```m68k
  0x00A196: sf.b $ff1651.l       ; clear composite flag
  0x00A19C: bsr.w $a342          ; pre-composition reset helper
  0x00A1A0: tst.b $ff1996.l      ; test demo mode active flag
  0x00A1A6: bne.w $a336          ; if demo active, branch to demo sprite hook $A7E6
  ```

### 2. Multi-Pool Render Sequence
1. **Group 1 (`0x00A1AA..0x00A1BE`)**:
   - Address: `$FF2954`, 4 slots, stride 90 (`$5A`).
   - If active (`type != 0`), pushes return vector `$A1BA` and dispatches to `$AD76` (normal SAT) or `$A9A0` (custom SAT, if `$36(a6)` bit 0 set).
2. **Special Pool (`0x00A1C2..0x00A1D2`)**:
   - Address: `$FF19C8`, 4 slots, stride 8.
   - If active (`type != 0`), dispatches through jump table at `$A33C`.
3. **Group 2 Y-Depth Sorter (`0x00A1D6..0x00A244`)**:
   - Address: `$FF19E8`, 21 slots, stride 188 (`$BC`).
   - Pass 1: builds active bitmask in `d7` and saves to `$FF188E.l`.
   - Pass 2: iterative selection sort finding the entity with the maximum Y coordinate (`+0x0C(a6)`) among unrendered slots.
   - Pushes return vector `$A1F8` and dispatches to `$AD76` or `$A9A0`.
   - Loops until all active slots have been dispatched in descending Y order (ensuring correct isometric foreground-over-background sprite layering).
4. **Group 4 (`0x00A248..0x00A25C`)**:
   - Address: `$FF2ABC`, 8 slots, stride 90.
5. **Group 3 (`0x00A260..0x00A274`)**:
   - Address: `$FF2D8C`, 6 slots, stride 90.
6. **Foreground Player Entity (`0x00A278..0x00A2A2`)**:
   - Tests flag `$FF1995.b`. If set, renders `$FF19E8[0]` in front of background groups via `$AD80` or `$A9AA`.
7. **SAT Link Terminator (`0x00A2A6..0x00A2C6`)**:
   - Pointer: `$FF13CC` (Mega Drive VDP SAT buffer).
   - Writes link terminator `link = 0` at `-5(a6, d7.w)` to terminate the VDP hardware sprite linked list.
   - Sets VDP dirty sync mask: `ori.b #3, $FF164E.l`.
   - Returns via `rts`.

## C++20 Cleanroom Interface Guidance
```cpp
namespace oasis::rendering {

enum class SatTarget : uint8_t {
    Normal = 0,
    Custom = 1,
};

struct SatEntity {
    int16_t type{0};
    int16_t y{0};
    uint8_t flags_36{0};
    uint32_t script_ptr{0};
};

inline void sort_and_dispatch_group_2(
    std::span<const SatEntity, 21> group2,
    auto&& render_callback) {
    uint32_t processed_mask = 0;
    for (size_t i = 0; i < 21; ++i) {
        if (group2[i].type == 0) {
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
