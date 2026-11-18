# Standalone Handoff: Milestone M15.467 — ROM Script Virtual Machine Opcodes $0E & $17: Fixed Position & Dual Cross-Linked Compound Entity Spawners

**ROM Range:** `0x00FD8C..0x00FEDE` (338 bytes)  
**Status:** Reverse-engineered, verified with Capstone M68K disassembly, JSON evidence generated, and unit tests passing.

---

## 1. System Overview

This subsystem implements specialized entity spawning routines within the Cutscene & Event Script Virtual Machine, notably including multi-part compound boss entities:
1. **Opcode `$0E` (`0x00FD8C..0x00FE0A`): Fixed Position Script Entity Spawner:**
   - Compares quest state (`0x0101EC`) with scenario progress threshold `$FF17B7`. If `>= threshold`, aborts script.
   - Allocates entity slot via `$DA2A` and verifies scene entity count limit via `0x0102BE`.
   - Reads:
     - `entity_type = (a0)+`: stores to `+$00(a6)`.
     - Runs constructor `$8D06`, sets priority `+$18 = d0`.
     - `visual_flag = (a0)+`: stores to `+$66(a6)`.
     - `facing = (a0)+`: stores to `+$16(a6)`.
     - `param_96 = (a0)+`: stores to `+$96(a6)`.
   - Positions entity via dedicated spatial calculation `0x0103B0`.
   - Configures initial/max HP `+$88/+$8A = d2`, drop item `+$8C`.
   - Runs animation setup `0x009D00`.
   - For entity types `>= 0x16` (22), activates AI handler from table `$DF54`.
2. **Opcode `$17` (`0x00FE0A..0x00FEDE`): Dual Cross-Linked Compound Entity Spawner (Types `$72` & `$70`):**
   - Spawns multi-part compound entities (e.g., rider/mount pairs or dual-segment bosses):
     - Primary Entity: Type `#$72` (slot `a5`).
     - Secondary Entity: Type `#$70` (slot `a6`).
   - Scenario gating check: tests quest state against `$FF17B7 - 1`. If `>= threshold`, aborts.
   - Allocates two entity slots sequentially via `$DA2A`.
   - Verifies compound entity limit via `0x010300`.
   - Sets visual flags: `+$66(a5) = 9`, `+$66(a6) = 5`.
   - Sets shared HP `d2` across both entities: `+$88(a5) = d2, +$8A(a5) = d2, +$88(a6) = d2, +$8A(a6) = d2`.
   - **Mutual Cross-Linking:** Stores cross-pointers at offset `+$A6`:
     - `+$A6(a5) = a6`
     - `+$A6(a6) = a5`
   - Clones spatial coordinates from `a6` to `a5` (`(x, y)` synced).
   - Activates coordinated dual AI controllers: calls `0x01DF1C` for `a6`, and `0x01D140` for `a5`.

---

## 2. Memory & Architecture Reference

| Symbol | Address | Description |
|---|---|---|
| `PRIMARY_COMPOUND_TYPE` | `$0072` | Primary compound entity type |
| `SECONDARY_COMPOUND_TYPE` | `$0070` | Secondary compound entity type |
| `CROSS_LINK_OFFSET` | `+$A6` | Mutual parent/child cross-pointer offset |
| `PRIMARY_AI_ROUTINE` | `0x01DF1C` | Compound secondary entity AI update |
| `SECONDARY_AI_ROUTINE` | `0x01D140` | Compound primary entity AI update |
| `SCRIPT_THRESHOLD` | `$FF17B7` | Scenario gating limit |

---

## 3. Cleanroom C++20 Interface

```cpp
namespace thor::script {

struct CompoundEntityPair {
    uint16_t primary_type{0x0072};
    uint16_t secondary_type{0x0070};
    uint8_t primary_visual_flag{9};
    uint8_t secondary_visual_flag{5};
    uint16_t shared_hp{0};
    uint16_t drop_id{0};
    uint16_t param_96{0};
    int16_t pos_x{0};
    int16_t pos_y{0};
};

bool execute_opcode_0e_spawn_fixed_pos(
    const uint8_t*& pc,
    uint8_t quest_var,
    uint8_t threshold);

bool execute_opcode_17_spawn_dual_compound(
    const uint8_t*& pc,
    uint8_t quest_var,
    uint8_t threshold,
    CompoundEntityPair& out_pair);

} // namespace thor::script
```
