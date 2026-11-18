# Standalone Handoff: Milestone M15.462 — ROM Drop Item Slot Allocation Eviction Probe, Ground Shadow Spawner & Ali Kill Counter Engine

**ROM Range:** `0x00F64C..0x00F788` (316 bytes)  
**Status:** Reverse-engineered, verified with Capstone M68K disassembly, JSON evidence generated, and unit tests passing.

---

## 1. System Overview

This subsystem implements three critical components of the game's drop and combat bookkeeping architecture:
1. **8-Slot Drop Item Allocation & Distance-Based Eviction Probe (`0x00F64C..0x00F6E2`):**
   - The drop item pool resides at `$FF2ABC` with 8 slots of stride 90 bytes (`$5A`).
   - When an item cannot be allocated normally via `$DA2A`, this probe scans the pool to evict an existing drop.
   - **Pass 1:** Scans slots 0..7. For any slot whose item attribute byte at `$5CE16[item_id]` has bit 7 clear (unprotected item), calls `0x00E5AE` with threshold 16 (`#$10`). If Carry is set (item is offscreen or far from Ali), this slot is evicted immediately and recycled at `$F550`.
   - **Pass 2:** If all eligible slots are nearby, rescans slots 0..7 and evicts the first slot with bit 7 clear regardless of distance.
   - **Critical Item Fallback:** If all 8 slots are protected (bit 7 set), tests bit 5 of the candidate item (`+$8C(a0)`). If bit 5 is clear, drop fails gracefully (`$F644`). If bit 5 is set (critical quest/story item), triggers ROM panic trap `jmp $206.l`.
2. **Ground Shadow / Spark Particle Entity Spawner (`0x00F6E2..0x00F740`):**
   - Spawns secondary particle entity (Type `$6A`) attached to parent item entity.
   - Calls particle allocator `$DA2A`. On allocation failure, exits cleanly.
   - Zeroes 94 words (188 bytes) in child entity slot.
   - Clones 45 words (90 bytes) from parent item `a4` to child `a6`.
   - In parent: sets flag `+$36` bit 1 and visual priority `+$0C = $7D00`.
   - In child: entity type = `#$6A`, flags `+$37 |= 0x60`, `+$38 &= ~0x04`, and parent back-pointer `+$A6 = a4`.
3. **Player Ali Kill Counter Credit & Special Drop Flag Evaluator (`0x00F740..0x00F788`):**
   - Called by enemy defeat routines (e.g. `0x00F476`, `0x016510`, `0x0201D6`, etc.).
   - If enemy drop field `+$8C` is 0 and global flag `$FF0DBB` is set, clears `$FF0DBB` and forces special drop `+$8C = $1101`.
   - If attacker pointer `+$7E` matches Player Ali (`$FF19E8`) and `+$3A` bit 0 is 0:
     - Increments Ali's lifetime kill counter at `$FF1A7C`.
     - Uses saturating clamp: wraps back to `$FFFF` on overflow.

---

## 2. Memory & Structural Layout

| Symbol | Address / Value | Description |
|---|---|---|
| `ITEM_POOL_BASE` | `$FF2ABC` | 8-slot drop item allocation pool |
| `ITEM_SLOT_STRIDE` | `90` (`$5A`) | Stride per drop item entity slot |
| `ITEM_SLOT_COUNT` | `8` | Total drop item slots |
| `ITEM_BASE_TILE` | `$0694` | VRAM pattern base index for drop items |
| `ITEM_TILE_STRIDE` | `9` | Pattern stride per item slot |
| `ITEM_ATTRIB_TABLE` | `$05CE16` | ROM item attributes table |
| `SHADOW_ENTITY_TYPE` | `$006A` | Ground shadow / spark particle entity ID |
| `ALI_ENTITY_ADDR` | `$FF19E8` | Canonical Player Ali entity RAM structure |
| `ALI_KILL_COUNTER` | `$FF1A7C` | Player Ali total kill counter (16-bit saturating) |
| `SPECIAL_DROP_FLAG` | `$FF0DBB` | Global flag forcing special drop `$1101` |

---

## 3. C++20 Interface Reference

```cpp
namespace thor::entity {

struct DropItemSlot {
    uint16_t item_id{0};
    uint8_t attrib_byte{0};
    uint16_t tile_pattern_base{0};
    int16_t pos_x{0};
    int16_t pos_y{0};
    int16_t pos_z{0};
    uint8_t flag_36{0};
    uint16_t elevation_priority{0};
};

enum class EvictionResult {
    Evicted,
    FailedDrop,
    PanicTrap
};

struct EvictionProbeResult {
    EvictionResult result{EvictionResult::FailedDrop};
    size_t slot_index{0};
    uint16_t tile_offset{0};
};

EvictionProbeResult evaluate_drop_eviction_probe(
    std::span<const DropItemSlot, 8> slots,
    uint8_t candidate_item_attrib,
    std::function<bool(size_t slot_idx)> is_out_of_range);

bool spawn_ground_shadow_particle(
    DropItemSlot& parent,
    DropItemSlot& child);

void credit_ali_kill_and_special_drop(
    uint16_t& enemy_drop_field_8c,
    uintptr_t attacker_ptr,
    uint8_t flag_3a,
    bool& global_special_drop_flag,
    uint16_t& ali_kill_counter);

} // namespace thor::entity
```
