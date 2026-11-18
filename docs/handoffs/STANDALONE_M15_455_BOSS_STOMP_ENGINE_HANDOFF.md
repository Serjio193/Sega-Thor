# STANDALONE HANDOFF: Milestone M15.455 — ROM Boss Entity Type $3A Spawner, Ground Stomp & Combat State Engine (Part 1)

**ROM Range:** `0x00ED04..0x00EE94` (400 bytes, 91 instructions)  
**Evidence Artifact:** `data/thor-evidence/rom_boss_stomp_engine_m15_455.json`  
**Reference Extractor:** `tools/extract_rom_boss_stomp_engine.py`  
**Reference Unit Test:** `tests/rom_boss_stomp_engine_test.py`

---

## 1. Architectural Summary & System Context

This milestone reverse-engineers the spawn initialization, phase transitions, and stomp/quake combat state logic for Boss Entity Type `$3A` (the giant rock colossus / minotaur boss).

### Subroutines in this Range:
1. `0x00ED04..0x00EDAA` (`spawn_init_boss_type3a` — 4 callers):
   - Sets boss fight active flag: `$FF198F = 0xFF`.
   - Clears Ali state word: `+$06(ali) = 0`.
   - Initializes entity type: `+$00(a6) = 0x3A`.
   - Invokes entity constructor `0x008D06`.
   - Adjusts elevation: `d5 -= 0x00040000` (clamped `>= 0`).
   - Copies sprite priority from Ali: `+$18(a6) = +$5A(ali)`.
   - Sets animation table index `0x009D00`.
   - Combat stats:
     - `+$A8(a6) = 0x8007` (damage / impact word).
     - `+$A6(a6) = 2` (initial phase).
     - `+$04(a6) = 8` (initial state).
     - `+$2A(a6) = 8` (state timer).
   - Audio & VFX:
     - Triggers boss roar SFX `#$B00225` via `0x00E7E8`.
     - Triggers ground shake SFX `#$600382` via `0x00E7E8`.
     - Sets camera screen shake timer: `$FF17C4 = 4` frames.
2. `0x00EDAA..0x00EDF2` (`boss_type3a_phase_and_damage_evaluator`):
   - Phase update: checks byte `+$66(a6)`.
     - If non-zero: sets `phase_a6 = +$66 - 1`, sets timer `+$02 = +$88`, elevation `+$10 = +$8C`, resets state `+$04 = 0`, clears `+$66 = 0`.
   - Incoming damage check `+$86(a6)`:
     - If heavy knockback bits set (`+$86 & 0xC000 != 0`): bumps elevation `+$10(a6) += 1`.
3. `0x00EDF2..0x00EE08` (`boss_type3a_state_jump_table`):
   - Dispatches state `+$04(a6)` via PC-relative word jump table at `0x00EDFE`.
4. `0x00EE08..0x00EE34` (`boss_type3a_state3_stomp_attack`):
   - Helper `0x00EF5E`.
   - Clears priority bit 11 on `+$18(a6)`.
   - Sets scan callback `0x00F054`.
   - Triggers combat scan `0x00BC98`.
   - If hit registered (bit 1 of `+$37` set): branches to recovery `0x00F038`.
5. `0x00EE34..0x00EE94` (`boss_type3a_state4_ground_quake`):
   - Bumps elevation `+$10 += 1`.
   - Triggers combat scan `0x00BC98`.
   - Quake impact frame check:
     - If `+$30(a6) == 8` and `+$2E(a6) == 0`:
       - Sets quake flag bit 4 on `+$39(a6)`.
       - Sets active quake byte `+$98(a6) = 0xFF`.
       - Sets camera screen shake timer: `$FF17C4 = 4`.
       - Triggers roar SFX `#$B00225`.

---

## 2. C++20 Cleanroom API Specification

```cpp
namespace oasis::boss {

struct BossType3A {
    uint16_t type{0x3A};
    int16_t x{0};
    int16_t y{0};
    int32_t z{0};
    uint16_t priority{0};
    uint16_t damage_a8{0x8007};
    uint16_t phase_a6{2};
    uint16_t state{8};
    uint16_t state_timer{8};
    uint8_t flags_39{0};
    bool active_quake{false};
};

struct BossSpawnResult {
    BossType3A boss{};
    bool boss_fight_active{true};
    uint16_t screen_shake_timer{4};
    uint32_t roar_sfx{0x00B00225};
    uint32_t shake_sfx{0x00600382};
};

constexpr BossSpawnResult spawn_boss_colossus(
    int16_t player_x, int16_t player_y, int32_t player_z,
    uint16_t player_priority) noexcept
{
    BossSpawnResult result{};
    result.boss.x = player_x;
    result.boss.y = player_y;
    int32_t elev = player_z - 0x00040000;
    result.boss.z = (elev < 0) ? 0 : elev;
    result.boss.priority = player_priority;
    return result;
}

constexpr bool check_ground_quake_trigger(
    uint16_t anim_frame_30, uint16_t tick_2e,
    BossType3A& boss, uint16_t& screen_shake) noexcept
{
    if (anim_frame_30 == 8U && tick_2e == 0U) {
        boss.flags_39 |= 0x10U; // bit 4: quake active
        boss.active_quake = true;
        screen_shake = 4U;
        return true;
    }
    return false;
}

} // namespace oasis::boss
```

---

## 3. Acceptance Criteria

1. **Boss Fight Activation:** Exact `$FF198F = 0xFF` flag and Ali word clearance.
2. **Colossus Elevation Offset:** Exact `-0x40000` clamp against player Z.
3. **Ground Quake Parity:** Matches frame `30 == 8` and `2e == 0` for screen shake `4` and roar SFX `#$B00225`.
4. **Heavy Knockback Elevation Bump:** Increments elevation `+$10` when damage `& 0xC000 != 0`.
5. All C++ source and test files must strictly stay `<= 500 lines`.
