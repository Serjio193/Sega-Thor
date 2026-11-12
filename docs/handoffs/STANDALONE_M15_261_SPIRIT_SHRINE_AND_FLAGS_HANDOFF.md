# STANDALONE M15.261 SPIRIT SHRINE EVENTS & GLOBAL FLAGS HANDOFF

**Milestone:** SEGA-THOR HYBRID — M15.261  
**Repository:** `C:/Github/Sega-Thor`  
**Branch:** `codex/helper10-99d6-bounded-scout`  
**Baseline Git Commit:** `a81dfaa0ed815467d7874e49aa05dc8623fe2dbe`  
**Standalone Repository (`C:/Github/Sega-Thor-Standalone`):** STRICTLY READ-ONLY / UNTOUCHED  
**Machine-Readable Evidence Output:** `data/thor-evidence/rom_spirit_shrine_events_m15_261.json`  
**Extraction Tool:** `tools/extract_rom_spirit_shrine_events.py`  
**Deterministic Test Suite:** `tests/rom_spirit_shrine_events_test.py`

---

## 1. Critical Architecture Correction (Standalone M16.50 Mislabeling)

> [!IMPORTANT]
> **Standalone M16.50 previously mislabeled ROM `0x018CAA`, `0x018CDE`, `0x018D38`, and Global Flag 3 as a "Princess Rescue" scene.**
> Static ROM disassembly and cross-reference analysis conclusively prove that:
> 1. `0x018CAA` is the **Spirit Awakening Dispatcher**, routing by Room ID (`$FF0E40.W`).
> 2. `0x018CDE` is the **Water Shrine Dytto Awakening Event** in Room `0x0106` (Area 1, Room 6).
> 3. `0x018D38` sets **Global Flag 3 (`$FF0DBC` bit 3)**, which denotes **Dytto Acquisition / Freedom**, NOT Princess Gwyn rescue.
> 4. `0x018D40` sets **Active Spirit Bit 0 (`$FF0D7A` bit 0)**, selecting Dytto as the active summoned spirit.
> 5. Altar Trigger Entity `0x0020` in Room `0x0106` (`0x04665E`) is gated on **Global Flag 3** via Opcode `0x15` (`SPAWN_GATED_PROP`).

---

## 2. ROM Spirit Awakening Architecture

### 2.1 Trigger & Dispatcher (`0x018CAA`)
Entity interaction logic (`0x008352..0x008392`) tests if the interacted entity is type `0x0020` (Altar / Prop). On match, it jumps to `0x018CAA`:

```m68k
0x018CAA: MOVE.W $00FF0E40.L, D0     ; Current Room ID (Area << 8 | Room)
0x018CB0: CMP.W #$0106, D0          ; Water Shrine (Room 0x0106) -> Dytto
0x018CB4: BEQ.W $018CDE
0x018CB8: CMP.W #$0206, D0          ; Fire Shrine (Room 0x0206) -> Efreet
0x018CBC: BEQ.W $018D4E
0x018CC0: CMP.W #$0805, D0          ; Shadow Shrine (Room 0x0805) -> Shade
0x018CC4: BEQ.W $018DAA
0x018CC8: CMP.W #$0A10, D0          ; Plant Shrine (Room 0x0A10) -> Bow
0x018CCC: BEQ.W $018E12
0x018CD0: CMP.W #$090B, D0          ; Seal Chamber (Room 0x090B) -> Agito
0x018CD4: BEQ.W $018E7A
0x018CD8: ANDI.B #$FE, CCR          ; Clear carry (unhandled room)
0x018CDC: RTS
```

---

## 3. Comprehensive Shrine Specifications

| Spirit | Element | Room ID | Handler | Spawn Helper | Message ID | Global Flag | Active Spirit Bit | Area Flag |
|---|---|---|---|---|---|---|---|---|
| **Dytto** | Water | `0x0106` | `0x018CDE` | `0x014668` | 0 | Flag 3 (`$FF0DBC` bit 3) | Bit 0 | `$FF0E1D` bit 2 |
| **Efreet** | Fire | `0x0206` | `0x018D4E` | `0x0146D6` | 9 | Flag 27 (`$FF0DBF` bit 3) | Bit 1 | None |
| **Shade** | Shadow | `0x0805` | `0x018DAA` | `0x014742` | 12 | Flag 63 (`$FF0DC3` bit 7) | Bit 2 | None |
| **Bow** | Plant | `0x0A10` | `0x018E12` | `0x014798` | 13 | Flag 20 (`$FF0DBE` bit 4) | Bit 3 | None |
| **Agito** | Dark/Seal | `0x090B` | `0x018E7A` | `0x00F53C` | 56 | Flag 54 (`$FF0DC2` bit 6) | None | None |

---

## 4. Dytto Awakening Event Flow (`0x018CDE..0x018D4E`)

1. **Trigger Entity Freeze:**
   - `MOVE.W #$003F, $32(A6)` freezes the interacting prop/altar entity.
2. **Dialogue Trigger:**
   - Prepares dialogue window at `(0x5000, 0x5800)`.
   - `MOVEQ #0, D0` -> `JSR $00C2EC` starts Message 0 ("I am Dytto, a water spirit...").
3. **Altar Coordinates & Teardown:**
   - Reads spawn location from `$FF1CD8` (prop slot): `X = $08(A4)`, `Y = $0C(A4)`, `Z = $10(A4) + 16px`.
   - `CLR.W (A4)` deactivates the altar prop entity so it no longer receives ticks.
4. **Spirit Entity Instantiation:**
   - Sets `A6` to Player base `$FF1AA4`.
   - Calls `BSR $014668` to allocate and spawn the spirit entity.
   - Sets Spirit State to `0x001A` (`APPEAR`) and Action/Anim to `0x0022`.
5. **State & Flag Commit:**
   - `BSET.B #2, $FF0E1D.L`: Sets Area Flag 2 (Water Shrine completed).
   - `BSET.B #3, $FF0DBC.L`: Sets Global Flag 3 (Dytto acquired).
   - `BSET.B #0, $FF0D7A.L`: Sets Active Spirit 0 (Dytto summoned).
   - `ORI.B #1, CCR; RTS`: Completes interaction with success.

---

## 5. Altar Entity 0x0020 Spawn Gating

In Room `0x0106` spawn script (`0x04664E`), Opcode `0x15` gates the altar entity:
```text
Offset: 0x04665E
Record: 15 01 00 20 00 03 00 00 00 00 00 A0 00 50 00 00 02 00
- Opcode: 0x15 (VM_OP_SPAWN_GATED_PROP)
- Bank: 1 ($FF0DBC Global Flags)
- Entity Type: 0x0020 (Altar Trigger)
- Flag Index: 3 (Global Flag 3)
- Coords: (160, 80, 0), VRAM tile base: 0x0200
```
When Global Flag 3 is set, this record is skipped, preventing the altar from re-spawning after Dytto is acquired.

---

## 6. Implementation Guidance for Standalone

```cpp
namespace oasis::game::spirit {

struct ShrineEventConfig {
    std::uint16_t room_id;
    std::uint16_t message_id;
    std::uint16_t global_flag_idx;
    std::uint8_t active_spirit_bit;
    std::int16_t elevation_delta_px;
};

inline constexpr ShrineEventConfig kShrines[] = {
    { 0x0106, 0,  3,  0, 16 }, // Dytto
    { 0x0206, 9,  27, 1, 8  }, // Efreet
    { 0x0805, 12, 63, 2, 16 }, // Shade
    { 0x0A10, 13, 20, 3, 0  }, // Bow
};

} // namespace oasis::game::spirit
```
