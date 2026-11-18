# STANDALONE HANDOFF: Milestone M15.461 — ROM Enemy Defeat Loot Drop & Chest Item Spawner Subsystem

## 1. Context & Range
- **Milestone:** M15.461
- **ROM Addresses:** `0x00F476..0x00F64C` (470 bytes)
- **Target Subsystem:** Master Enemy Death Drop, Scene Chest Loot Spawner & Persistent Collection State Subsystem:
  * Enemy death 2-tier drop table evaluator (`0x00F476..0x00F53C`, >30 callers across game!)
  * Fixed / Chest drop spawner and persistent collection bitmask tester (`0x00F53C..0x00F64C`)
  * 6-byte chest loot record format from ROM table `$43830`
  * Item entity spawn configuration (Type 8, launch velocity `+0x00080000`, ground spark)
- **Evidence JSON:** `data/thor-evidence/rom_item_drop_engine_m15_461.json`
- **Unit Tests:** `tests/rom_item_drop_engine_test.py`

---

## 2. Subroutines & ROM Architecture

### Subroutine 1: `ItemDrop_EnemyDefeatDropEvaluator` (`0x00F476..0x00F53C`, 198 bytes, 49 insns)
- Called whenever an enemy is defeated across Beyond Oasis (>30 callers).
- Inputs: `a6` = defeated enemy entity, `a1` = enemy drop table pointer.
- Checks `+$8C(a6)`: if non-zero, overrides with fixed drop via `0x00F53C`.
- Tests `btst.b #7, $6F(a6)`: if bit 7 is set (drop disabled / already dropped), restores `a6` and exits (`rts`).
- Allocates entity slot from Pool 2 (`$FF1CD8`) via `0x00D9BE`. If allocation fails (`carry set`), exits.
- Preserves slot index `d7 = d0`.
- Evaluates 2-tier drop table at `(a1)` (8 bytes, 4 words):
  * Primary drop:
    - If `(a1) == 0`: skips to secondary drop.
    - Generates PRNG word via `0x00E268`. If `rand <= (a1)`, succeeds! Loads item ID `d0 = 2(a1)` and proceeds to spawn.
  * Secondary drop:
    - If `4(a1) == 0`: exits without drop.
    - Generates PRNG word via `0x00E268`. If `rand <= 4(a1)`, succeeds! Loads item ID `d0 = 6(a1)`.
    - Else: exits without drop.
- Spawns World Item Entity (Type 8):
  * `+$00(a6) = 0x0008`.
  * Constructor `0x008D06`.
  * Item ID: `+$52(a6) = d0.b`, Category: `+$33(a6) = d0.w >> 8`.
  * Priority: `+$18(a6) = d7` (from pool allocation).
  * Coordinates: `+$14(a6) = +$14(a0)`, `+$08(a6) = +$08(a0)`, `+$0C(a6) = +$0C(a0)`.
  * Elevation: `+$10(a6) = +$10(a0) + 1` (spawns 1px above enemy).
  * Upward launch velocity: `+$56(a6) = 0x00080000` (+8.0 px/frame upward pop).
  * Sets flag: `bset.b #0, $39(a6)`.
  * Lifetime: PRNG `0x00E268 & 0x3F` (0..63 frames) -> `+$02(a6)`.
  * Attribute check at `$5CE16`: if `bit 7 of $5CE16(item_id)` is 0 (ground resting item), sets `+$04(a6) = 2`.
  * Spawns ground shadow / spark via `0x00F6E2`.
  * Restores `a6 = a0` and returns.

### Subroutine 2: `ItemDrop_FixedChestDropSpawner` (`0x00F53C..0x00F64C`, 272 bytes, 69 insns)
- Called when opening scene chests, fixed loot urns, and story reward entities.
- Inputs: `a0` = chest / container entity, `+$8C(a0)` = fixed item ID / chest record ID.
- If `+$8C(a0) == 0xFFFF`: exits with Carry set (`0x00F644`, no drop).
- Allocates slot via `0x00D9BE` (falls back to 8-slot probe at `0x00F64C` if full).
- Sets `+$00(a6) = 8`, constructor `0x008D06`, priority `+$18(a6) = d0`.
- Reads `d0 = +$8C(a0)`:
  * If `d0 >= 0x8000`: Scene chest record in table `$43830`:
    - Record index: `(d0 - 0x8000) * 6`.
    - 6-byte record layout:
      * `byte 0`: category (written to `+$33(a6)`)
      * `byte 1`: item ID (written to `+$52(a6)`)
      * `byte 2`: unused
      * `byte 3`: persistent bank index (1: `$FF0DBC`, 2: `$FF0E1C`, 3: `$FF0E2C`)
      * `bytes 4..5`: persistent bit index (written to `+$50(a6)`)
    - If `bank != 0`:
      * Computes byte offset: `byte_offset = bit_index >> 3`.
      * Computes bit mask: `bit_mask = 1 << (bit_index & 7)`.
      * Checks persistent collection RAM:
        - If bit is set (`already collected`):
          - Deallocates slot `+$00(a6) = 0`.
          - Exits with Carry set (`0x00F644`, no drop spawned).
  * If `d0 < 0x8000`: Direct item ID:
    - Sets `+$52(a6) = d0.b`, `+$33(a6) = d0.w >> 8`.
- Coordinates: `+$08(a6) = +$08(a0)`, `+$0C(a6) = +$0C(a0)`, `+$10(a6) = +$10(a0) + 1`.
- Upward launch velocity: `+$56(a6) = 0x00080000` (+8.0 px/frame).
- Sets flag: `bset.b #0, $39(a6)`.
- Lifetime: PRNG `0..63` -> `+$02(a6)`.
- Attribute check at `$5CE16`: if bit 7 is 0, sets State `+$04 = 2`.
- Spawns ground shadow / spark via `0x00F6E2`.
- Returns with Carry clear (`andi.b #$FE, ccr`), `exg.l a0, a6`, `rts`.

---

## 3. Recommended C++20 Interface
```cpp
namespace thor::items {

struct DropTableEntry {
    uint16_t primary_prob{0};
    uint16_t primary_item_id{0};
    uint16_t secondary_prob{0};
    uint16_t secondary_item_id{0};
};

struct ChestRecord {
    uint8_t category{0};
    uint8_t item_id{0};
    uint8_t bank_idx{0};
    uint16_t bit_index{0};
};

bool is_chest_collected(const ChestRecord& rec, std::span<const uint8_t> ram);
std::optional<uint16_t> evaluate_enemy_drop(const DropTableEntry& table, uint16_t rand1, uint16_t rand2);

} // namespace thor::items
```
