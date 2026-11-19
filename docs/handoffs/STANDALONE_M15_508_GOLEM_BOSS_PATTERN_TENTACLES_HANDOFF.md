# STANDALONE HANDOFF: Milestone M15.508 — ROM Multi-Part Boss/Golem (`0x9A`) Master Tick, 4-State Dispatcher, 5x8 PRNG Pattern Sequencer & Tentacle/Child Initializer (`0x015316..0x0154E8`)

## 1. Subsystem Overview
- **ROM Address Range:** `0x015316..0x0154E8` (466 bytes, 102 M68K instructions across 5 code segments and 3 embedded tables; contiguous executable code implementing the multi-part Golem boss master router, body contact check, state jump table, PRNG pattern action sequencer, 7-part tentacle arm spawning, and retract cleanup).
- **Core Responsibilities:**
  1. `0x015316..0x015360` (`BossGolem_MasterTickAndRouter`, 74 bytes):
     - Dispatches on sub-mode `+$A2(a6)`:
       - `0` $\to$ `0x015758`
       - `1` $\to$ `0x015338` (continues in this range)
       - `2` $\to$ `0x0159E2`
       - `3` $\to$ `0x0159B4`
       - `4` $\to$ `0x01573C`
       - other $\to$ `rts` at `0x015336`
     - Evaluates body contact AABB via `0x00B922`: `dx in [-56, +56]`, `dy in [-120, +120]`. On contact, invokes callback `0x015648`.
     - Dispatches on `+$04(a6)` via 4-word relative jump table at `0x015360`:
       - State `0` $\to$ `0x015368` (Pattern Sequencer)
       - State `2` $\to$ `0x01552E`
       - State `4` $\to$ `0x0154E8`
       - State `6` $\to$ `0x0154A8` (Retract & Reset)
  2. `0x015368..0x0153E0` (`BossGolem_State00_PatternSequencer`, 68 bytes code + 12 bytes action jump table + 40 bytes 5x8 pattern table):
     - Decrements timer `+$2(a6)`; returns if not expired.
     - Checks 8-step cursor `+$AA(a6)`:
       - If `+$AA >= 8`: wraps `+$AA = 0`, calls PRNG `0x00E268`, selects new pattern index `d0 = (prng & 3) + 1` (1..4) into `+$A8(a6)`.
     - Advances step cursor `+$AA += 1`.
     - Reads Action ID from 5x8 table `0x0153B8`: `d2 = table[d0 * 8 + d1]`.
       - Row 0 (initial): `[5, 5, 5, 5, 0, 3, 2, 1]`
       - Row 1: `[4, 1, 2, 3, 0, 3, 1, 0]`
       - Row 2: `[0, 5, 3, 1, 2, 0, 2, 0]`
       - Row 3: `[4, 2, 2, 0, 3, 3, 2, 2]`
       - Row 4: `[1, 1, 1, 5, 1, 3, 2, 0]`
     - Dispatches via 6-entry Action Jump Table `0x0153AC`:
       - Action 0 $\to$ `0x0153F0`: `+$A6 = 12`, `State = 2`
       - Action 1 $\to$ `0x0153FE`: `+$A6 = 2`, `State = 2`
       - Action 2 $\to$ `0x01540C`: `timer = 90`, `State = 4`
       - Action 3 $\to$ `0x01541A`: Tentacle / Arm Activator
       - Action 4 $\to$ `0x0153E0`: `timer = 30`
       - Action 5 $\to$ `0x0153E8`: `timer = 50`
  3. `0x01541A..0x0154A8` (`BossGolem_TentacleArmActivator`, 142 bytes, 28 insns):
     - Activates primary arm slot `$FF1E50`: `state=1`, `timer=40`, `substate=8`, clears `$38:3` and `$36:1`, `offset_1e=-32`, `y=176`.
     - Sets boss parent state `+$04 = 6` (State `$06`).
     - Activates 7 child tentacle slots starting at `$FF1F0C` (stride `0xBC` via `dbra #6`):
       - Type `0x9A` via `0x008D06`
       - Tile attribute `0x0300`
       - Dynamic VRAM pointer: `$FF17A6 + 0x19A0` into `+$1A`
       - Sub-mode `+$A2 = 4`, substate `+$2A = 0x0012`
  4. `0x0154A8..0x0154E8` (`BossGolem_State06_RetractAndReset`, 64 bytes, 15 insns):
     - Checks primary arm `$FF1E50`: if type `0x9A` and `state == 0` (retract finished):
       - Clears all 7 child tentacle slots starting at `$FF1F0C` (`+$0(slot) = 0`).
       - Hides arm `$FF1E50`: `bset #3, $38`, `bset #1, $36`, `offset_1e = 0x80`.
       - Resets boss to idle via `0x01563C`.

## 2. ROM Evidence Reference
- Evidence JSON: `data/thor-evidence/rom_golem_boss_pattern_tentacles_m15_508.json`
- Extractor Tool: `tools/extract_rom_golem_boss_pattern_tentacles.py`
- Test Suite: `tests/rom_golem_boss_pattern_tentacles_test.py`

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <array>
#include <cstdint>
#include <optional>

namespace Thor::BossGolem {

inline constexpr uint16_t kGolemEntityType           = 0x009Au;
inline constexpr uint16_t kTentacleChildCount        = 7u;
inline constexpr uint16_t kTentacleTileAttrBase      = 0x0300u;
inline constexpr uint32_t kTentacleVramOffset        = 0x000019A0u;
inline constexpr uint8_t  kTentacleChildSubMode      = 4u;
inline constexpr uint16_t kTentacleChildSubstate     = 0x0012u;

inline constexpr int16_t  kBodyContactMinDx          = -56;
inline constexpr int16_t  kBodyContactMaxDx          = 56;
inline constexpr int16_t  kBodyContactMinDy          = -120;
inline constexpr int16_t  kBodyContactMaxDy          = 120;

inline constexpr std::array<std::array<uint8_t, 8>, 5> kGolemPatterns{{
    { 5, 5, 5, 5, 0, 3, 2, 1 }, // Pattern 0 (Initial)
    { 4, 1, 2, 3, 0, 3, 1, 0 }, // Pattern 1
    { 0, 5, 3, 1, 2, 0, 2, 0 }, // Pattern 2
    { 4, 2, 2, 0, 3, 3, 2, 2 }, // Pattern 3
    { 1, 1, 1, 5, 1, 3, 2, 0 }, // Pattern 4
}};

inline bool CheckGolemBodyContact(int16_t ali_dx, int16_t ali_dy) noexcept {
    return (ali_dx >= kBodyContactMinDx && ali_dx <= kBodyContactMaxDx &&
            ali_dy >= kBodyContactMinDy && ali_dy <= kBodyContactMaxDy);
}

struct PatternStepResult {
    uint8_t  action_id{0};
    uint8_t  pattern_index{0};
    uint8_t  step_cursor{0};
    bool     pattern_wrapped{false};
};

inline PatternStepResult StepGolemPattern(
    uint8_t current_pattern_index,
    uint8_t current_step_cursor,
    uint8_t prng_val) noexcept {
    uint8_t pattern = current_pattern_index;
    uint8_t cursor = current_step_cursor;
    bool wrapped = false;

    if (cursor >= 8u) {
        cursor = 0u;
        pattern = static_cast<uint8_t>((prng_val & 3u) + 1u); // 1..4
        wrapped = true;
    }

    const uint8_t action = kGolemPatterns[pattern][cursor];
    cursor = static_cast<uint8_t>(cursor + 1u);

    return PatternStepResult{action, pattern, cursor, wrapped};
}

} // namespace Thor::BossGolem
```
