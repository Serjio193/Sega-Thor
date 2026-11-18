# STANDALONE HANDOFF: Milestone M15.453 — ROM Dynamic Effect Type $34 Spawner, Audio Rate Limiter & PRNG Shake-Jitter Engine

**ROM Range:** `0x00E90E..0x00EA76` (360 bytes, 95 instructions)  
**Evidence Artifact:** `data/thor-evidence/rom_effect_spawner_jitter_m15_453.json`  
**Reference Extractor:** `tools/extract_rom_effect_spawner_jitter.py`  
**Reference Unit Test:** `tests/rom_effect_spawner_jitter_test.py`

---

## 1. Architectural Summary & System Context

This milestone reverse-engineers the dynamic Type `$34` impact/smoke effect spawner, audio SFX rate-limiter, cutscene invisibility gate, and PRNG-driven position shake/jitter vibration engine in Beyond Oasis.

### Subroutines in this Range:
1. `0x00E90E..0x00E93A` (`spawn_effect_type34_normal` — 8 callers):
   - Allocates an entity slot from Pool 2 (`17` slots at `$FF1CD8`) via helper `0x00DA2A`.
   - Initializes 3D coordinates `(d1, d2, d3)`.
   - Calls common initializer `0x00E96C`.
2. `0x00E93A..0x00E96C` (`spawn_effect_type34_priority` — 6 callers):
   - Allocates slot from Pool 2, initializes coordinates, and sets high-priority bit on sprite priority word:
     `ori.w #$8000, +$18(a6)`.
3. `0x00E96C..0x00E9DE` (`init_effect_type34_and_sfx`):
   - Initializes entity type: `+$00(a6) = 0x34`.
   - Invokes entity constructor `0x008D06`.
   - Sets sprite priority word: `+$18(a6) = 0x055E` (or `0x855E` if priority flag set).
   - Clears facing: `+$16(a6) = 0`.
   - Anchors world position into both current position and base anchor fields:
     - `+$08(a6) = world_x`, `+$4E(a6) = world_x`
     - `+$0C(a6) = world_y`, `+$52(a6) = world_y`
     - `+$10(a6) = world_z`
   - Sets lifetime timer: `+$9E(a6) = d0`.
   - Audio rate limiter:
     - Checks frame interval between current frame `$FF199C` and last effect SFX frame `$FF1944`.
     - If elapsed `< 0` or `>= 7` frames:
       - Updates `$FF1944 = $FF199C`.
       - Triggers spatial SFX `#$90021B` via `0x00E7E8`.
       - Triggers spatial SFX `#$600382` via `0x00E7E8`.
4. `0x00E9DE..0x00EA04` (`cutscene_invisibility_gate`):
   - Checks entity slot 0 at `$FF1AA4`. If type is `#$1E` (cutscene actor):
     - Sets bit 1 on `+$36(a6)` (invisible).
     - Sets bit 2 on `+$38(a6)` (intangible).
   - Else: clears both bits.
5. `0x00EA04..0x00EA76` (`tick_effect_type34_prng_jitter`):
   - If timer `+$2A(a6) == 0` and active hit flag bit 1 on `+$37(a6)` is set:
     - Decrements lifetime counter: `+$9E(a6) -= 1`.
     - When `+$9E(a6) == 0`: despawns slot (`+$00(a6) = 0`).
     - While active:
       - Calls 16-bit PRNG `0x00E268` twice to obtain random offsets:
         `jitter_x = (rand1 & 0x1F) - 0x10` (range `[-16..+15]`).
         `jitter_y = (rand2 & 0x1F) - 0x10` (range `[-16..+15]`).
       - Updates position relative to anchor:
         `+$08(a6) = +$4E(a6) + jitter_x`
         `+$0C(a6) = +$52(a6) + jitter_y`
       - Triggers audio SFX `#$90021B` and `#$600382`.

---

## 2. C++20 Cleanroom API Specification

```cpp
namespace oasis::entity {

struct EffectType34 {
    uint16_t type{0x34};
    uint16_t priority{0x055E};
    int16_t x{0};
    int16_t y{0};
    int16_t z{0};
    int16_t anchor_x{0};
    int16_t anchor_y{0};
    uint16_t lifetime{0};
    bool invisible{false};
    bool intangible{false};
};

struct AudioRateLimiter {
    uint16_t last_sfx_frame{0};

    constexpr bool should_play(uint16_t current_frame) noexcept {
        int32_t elapsed = static_cast<int32_t>(current_frame) - static_cast<int32_t>(last_sfx_frame);
        if (elapsed < 0 || elapsed >= 7) {
            last_sfx_frame = current_frame;
            return true;
        }
        return false;
    }
};

constexpr bool tick_effect_jitter(EffectType34& effect, uint16_t rand_x, uint16_t rand_y) noexcept {
    if (effect.lifetime == 0) {
        effect.type = 0;
        return false;
    }

    --effect.lifetime;
    if (effect.lifetime == 0) {
        effect.type = 0;
        return false; // Despawned
    }

    int16_t jitter_x = static_cast<int16_t>((rand_x & 0x1FU) - 0x10U);
    int16_t jitter_y = static_cast<int16_t>((rand_y & 0x1FU) - 0x10U);

    effect.x = static_cast<int16_t>(effect.anchor_x + jitter_x);
    effect.y = static_cast<int16_t>(effect.anchor_y + jitter_y);
    return true;
}

} // namespace oasis::entity
```

---

## 3. Acceptance Criteria

1. **Effect Type & Priority:** Exact type code `0x34`, default priority `0x055E`, and high-priority bitmask `0x8000`.
2. **Audio Rate Limiting:** Exact 7-frame interval between spatial audio triggers `#$90021B` and `#$600382`.
3. **PRNG Shake-Jitter Parity:** `[-16..+15]` random offsets applied to anchor positions `(anchor_x, anchor_y)`.
4. **Despawn Parity:** Entity slot zeroed when lifetime counter `+$9E` hits 0.
5. All C++ source and test files must strictly stay `<= 500 lines`.
