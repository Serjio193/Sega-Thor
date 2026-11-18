# Standalone Handoff: Milestone M15.471 — ROM Script Virtual Machine Scene Entity Limit Checker, Parameter Buffer Indexer & ID Allocator Subsystem

**ROM Range:** `0x01027C..0x0103B0` (308 bytes)  
**Status:** Reverse-engineered, verified with Capstone M68K disassembly, JSON evidence generated, and unit tests passing.

---

## 1. System Overview

This subsystem implements entity spawn limit validation, dynamic ID candidate scanning from the script parameter word buffer (`$FF17C6`), and duplicate prevention across the 17 entity slots in Pool 2 (`$FF1CD8`):

1. **`0x01027C..0x0102BE` (`ScriptState_CheckEntitySlotLimit1`):**
   - Invoked by dynamic entity spawner Opcode `$01`.
   - Reads index range word `d0 = +16(a0)` (`start = d0 >> 8`, `end = d0 & $FF`, count = `end - start`) and flag byte `d1 = +15(a0)`.
   - If `d1 == 0`: immediately allows spawn (`andi.b #$FE, ccr; rts`, Carry clear).
   - Iterates through parameter buffer slice `param_buffer[start..end]` calling `0x010382`.
   - If an available candidate `cand = *a1 & 0x7FF` is found:
     * Packs flag into high bits: `d0 = cand | ((d1 << 8) & 0xF800)`.
     * Clears Carry flag (`andi.b #$FE, ccr; rts`) and returns allocated ID in `d0.w`.
   - If all candidates are active in Pool 2:
     * Sets Carry flag (`ori.b #1, ccr; rts`) signaling limit exhaustion (spawn aborted).

2. **`0x0102BE..0x010300` (`ScriptState_CheckEntitySlotLimit2`):**
   - Invoked by fixed position entity spawner Opcode `$0E`.
   - Identical logic to Limit 1, but reads parameter range from `+2(a0)` and flag byte from `+1(a0)`.

3. **`0x010300..0x010382` (`ScriptState_CheckCompoundEntitySlotLimit`):**
   - Invoked by dual-compound entity spawner Opcode `$17`.
   - Evaluates two candidate ranges for primary (Type `$72`) and secondary (Type `$70`) compound entities:
     * Primary range word at `+2(a0)`, flag byte at `+6(a0)`.
     * Secondary range word at `+4(a0)`, flag byte at `+7(a0)`.
   - Scans parameter buffer for primary ID; aborts with Carry set if exhausted.
   - Scans parameter buffer for secondary ID; aborts with Carry set if exhausted.
   - Packs primary and secondary IDs into `d0.l`:
     * High word: `primary_id | ((flag1 << 8) & 0xF800)`.
     * Low word: `secondary_id | ((flag2 << 8) & 0xF800)`.
   - Returns Carry clear on success.

4. **`0x010382..0x0103B0` (`ScriptState_FindEntityIdInPool2`):**
   - Scans 17 entity slots in Pool 2 (`$FF1CD8`, stride 188 bytes / `0xBC`).
   - If slot is active (`type (+$00) > 0`):
     * Compares `+$18(slot) & 0x7FF` against candidate ID `d0 & 0x7FF`.
     * If matched (entity already exists on screen): returns Carry clear (`andi.b #$FE, ccr; rts`).
   - If loop finishes with no match: returns Carry set (`ori.b #1, ccr; rts`, indicating candidate is available).

---

## 2. Memory & Architecture Reference

| Symbol | Address | Description |
|---|---|---|
| `SCRIPT_PARAM_BUFFER` | `$FF17C6` | 16-word parameter buffer loaded by Opcode $02 |
| `POOL2_BASE` | `$FF1CD8` | Dynamic entity pool 2 (17 slots, stride 188 bytes) |
| `SLOT_ID_OFFSET` | `+$18` | Entity unique ID / visual ID field in entity slot |
| `SLOT_TYPE_OFFSET` | `+$00` | Entity active type field (0 = vacant, >0 = active) |

---

## 3. Cleanroom C++20 Interface

```cpp
namespace thor::script {

bool check_entity_slot_limit(
    uint16_t range_word,
    uint8_t flag_byte,
    std::span<const uint16_t> param_buffer,
    std::span<const EntitySlot> pool2_slots,
    uint16_t& out_allocated_id);

bool check_compound_entity_slot_limit(
    uint16_t primary_range,
    uint16_t secondary_range,
    uint8_t primary_flag,
    uint8_t secondary_flag,
    std::span<const uint16_t> param_buffer,
    std::span<const EntitySlot> pool2_slots,
    uint32_t& out_dual_allocated_ids);

bool is_entity_id_available(
    std::span<const EntitySlot> pool2_slots,
    uint16_t candidate_id);

} // namespace thor::script
```
