# Standalone Handoff: Milestone M15.466 — ROM Script Virtual Machine Opcodes $01, $06, $07, $08, $0A, $0B: Loop Engine, Frame Sleep & Dynamic Entity Spawner

**ROM Range:** `0x00FC6C..0x00FD8C` (288 bytes)  
**Status:** Reverse-engineered, verified with Capstone M68K disassembly, JSON evidence generated, and unit tests passing.

---

## 1. System Overview

This subsystem implements iteration control, timing delays, entity command messaging, scenario gating, and dynamic entity instantiation within the Cutscene & Event Script Virtual Machine:
1. **Opcode `$06` (`0x00FC6C..0x00FC94`): Loop Decrement and Branch (`DJNZ`):**
   - Reads 8-bit counter index from `+1(a0)`.
   - References 16-bit loop counter array at `$FF1826`.
   - Decrements counter: if non-zero, adds 16-bit relative displacement `+2(a0)` to script PC `a0`.
   - If counter reaches zero, advances past the 4-byte instruction (`addq.l #4, a0; rts`).
2. **Opcode `$07` (`0x00FC94..0x00FCA6`): Set Loop Counter (`SET_LOOP_COUNTER`):**
   - Sets loop counter `+1(a0)` at `$FF1826` to immediate 16-bit value `+2(a0)`.
3. **Opcode `$08` (`0x00FCA6..0x00FCB8`): Frame Delay / Sleep (`WAIT_FRAMES`):**
   - Reads 8-bit frame count from `(a0)+` into `$FF17B6`.
   - Stores script PC to `$FF17B2` and yields current frame execution (`bra.w $F91C`).
4. **Opcode `$0A` (`0x00FCB8..0x00FCC4`): Send Entity Event (`SEND_ENTITY_EVENT`):**
   - Reads 32-bit entity pointer `a6` and 16-bit event command `d6`, dispatching via `$D9A4`.
5. **Opcode `$0B` (`0x00FCC4..0x00FCCE`): Set Scenario Abort Threshold (`SET_THRESHOLD`):**
   - Sets scenario progress limit threshold `$FF17B7 = (a0)+`.
6. **Opcode `$01` (`0x00FCCE..0x00FD8C`): Dynamic Script Entity Spawner (`SPAWN_ENTITY`):**
   - Checks quest progress via `0x0101EC` against `$FF17B7`: if `>= threshold`, aborts script.
   - Allocates secondary entity slot via `0x00DA2A` and checks scene entity limit `0x01027C`.
   - Reads spawn mode `d7.b`, type ID `d1.w` (stored to `+$00(a6)`).
   - Executes entity constructor `0x008D06`.
   - Sets PRNG facing direction `0..3` (`0x00E268`).
   - Dispatches spatial coordinates through 3-entry jump table at `0x00FD86`.
   - Configures initial/max HP `+$88/+$8A = d2`, drop item `+$8C`, visual flag `+$66`, and animation setup `0x009D00`.
   - For entity types `>= 0x16` (22), activates autonomous AI from table `$DF54`.

---

## 2. Memory & Architecture Reference

| Symbol | Address | Description |
|---|---|---|
| `LOOP_COUNTER_BASE` | `$FF1826` | Array of 16-bit loop iteration counters |
| `SCRIPT_DELAY_TIMER` | `$FF17B6` | Frame sleep countdown timer |
| `SCRIPT_THRESHOLD` | `$FF17B7` | Scenario progression gating threshold |
| `SPAWN_POS_TABLE` | `0x00FD86` | 3-entry relative word jump table for spatial positioning |
| `ENTITY_AI_TABLE` | `0x00DF54` | AI controller jump table for enemy entity types `>= 0x16` |

---

## 3. Cleanroom C++20 Interface

```cpp
namespace thor::script {

struct SpawnEntityParams {
    uint8_t spawn_mode{0};
    uint16_t entity_type{0};
    uint16_t param_96{0};
    uint16_t hp{0};
    uint16_t drop_id{0};
    uint8_t visual_flag{0};
};

bool execute_opcode_06_loop_decrement(
    const uint8_t*& pc,
    uint8_t counter_idx,
    int16_t branch_displacement,
    std::span<uint16_t> loop_counters);

void execute_opcode_07_set_loop_counter(
    const uint8_t*& pc,
    uint8_t counter_idx,
    uint16_t count,
    std::span<uint16_t> loop_counters);

bool execute_opcode_08_wait_frames(
    const uint8_t*& pc,
    uint8_t frames,
    uint8_t& out_delay_timer);

void execute_opcode_0a_send_event(
    const uint8_t*& pc,
    uintptr_t target_entity,
    uint16_t event_cmd);

void execute_opcode_0b_set_threshold(
    const uint8_t*& pc,
    uint8_t threshold,
    uint8_t& out_scenario_threshold);

bool execute_opcode_01_spawn_entity(
    const uint8_t*& pc,
    const SpawnEntityParams& params,
    uint8_t current_quest_state,
    uint8_t scenario_threshold);

} // namespace thor::script
```
