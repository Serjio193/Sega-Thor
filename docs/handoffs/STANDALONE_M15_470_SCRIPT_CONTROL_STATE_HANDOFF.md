# Standalone Handoff: Milestone M15.470 — ROM Script Virtual Machine Opcodes $0F..$11, $13, $14 & State Query Subroutines: Persistent Modifiers, GOSUB & Ali Vital Metrics

**ROM Range:** `0x010140..0x01027C` (316 bytes)  
**Status:** Reverse-engineered, verified with Capstone M68K disassembly, JSON evidence generated, and unit tests passing.

---

## 1. System Overview

This subsystem implements persistent bitbank modifiers, subroutine call/return execution (`GOSUB`/`RETURN`), and query helpers for enemy counts, entity presence, and player Ali vitals in the Cutscene & Event Script Virtual Machine:

1. **Opcodes `$0F`, `$10`, `$11` (`0x010140..0x0101D0`): Persistent Bitbank Modifiers:**
   - Instruction format (4 bytes): `[Opcode:1] [Mode:1] [BitIndex:2]`.
   - Modifies specific bit within one of three persistent game state banks:
     * `$0F`: Bank 1 (`$FF0DBC`).
     * `$10`: Bank 2 (`$FF0E1C`).
     * `$11`: Bank 3 (`$FF0E2C`).
   - Bit addressing math:
     * Byte offset: `d0 = bit_idx >> 3`.
     * Bit index in byte: `d1 = bit_idx & 7`.
     * Target address: `bank_base + byte_offset`.
   - Action modes (`Mode = +1(a0)`):
     * `0`: Set bit (`bset.b d1, (a1)`).
     * `1`: Clear bit (`bclr.b d1, (a1)`).
     * `Other` (e.g. `2`): Invert / Toggle bit (`bchg.b d1, (a1)`).
   - Advances script PC by 4 (`addq.l #4, a0; rts`).

2. **Opcode `$13` (`0x0101D0..0x0101E4`): Script Subroutine Call (`GOSUB`):**
   - Reads 24-bit big-endian subroutine destination address: `+1(a0)..+3(a0)`.
   - Saves return address (current PC + 4) to script return register: `move.l a0, $FF1836.l`.
   - Sets script PC to subroutine address: `movea.l d0, a0; rts`.

3. **Opcode `$14` (`0x0101E4..0x0101EC`): Script Subroutine Return (`RETURN`):**
   - Restores saved script PC: `movea.l $FF1836.l, a0; rts`.

4. **Helper `0x0101EC` (`ScriptState_CountLivingEnemies`):**
   - Scans 17 Pool 2 entity slots starting at `$FF1CD8` (slot stride `0xBC` = 188 bytes).
   - Counts living enemies where `type (+$00) > 0` and bit 0 of status flags (`+$3A`) is 0.
   - Returns count in `d0.w`.

5. **Helper `0x010214` (`ScriptState_CountActiveEntities`):**
   - Scans 17 Pool 2 entity slots; counts all slots with `type (+$00) > 0`.
   - Returns count in `d0.w`.

6. **Helper `0x010232` (`ScriptState_FindEntityTypeInPool2`):**
   - Scans 17 Pool 2 slots searching for entity type `d1.w`.
   - Sets Carry flag (`ori.b #1, ccr`) if found; clears Carry flag (`andi.b #0xFE, ccr`) if not found.

7. **Helper `0x010254` (`ScriptState_CalcAliHpPercent`):**
   - Reads Ali current HP from `$FF1A72` and max HP from `$FF1A70`.
   - Computes integer percentage: `d0.w = (cur_hp * 100) / max_hp`.

8. **Helper `0x010268` (`ScriptState_CalcAliSpPercent`):**
   - Reads Ali current SP from `$FF1A76` and max SP from `$FF1A74`.
   - Computes integer percentage: `d0.w = (cur_sp * 100) / max_sp`.

---

## 2. Memory & Architecture Reference

| Symbol | Address | Description |
|---|---|---|
| `BANK1_FLAGS` | `$FF0DBC` | Persistent story flags bank 1 (32 bytes) |
| `BANK2_FLAGS` | `$FF0E1C` | Persistent story flags bank 2 (32 bytes) |
| `BANK3_FLAGS` | `$FF0E2C` | Persistent story flags bank 3 (32 bytes) |
| `SCRIPT_GOSUB_RETURN_PC` | `$FF1836` | Return address register for script subroutines |
| `ALI_MAX_HP` | `$FF1A70` | Ali maximum HP (16-bit word) |
| `ALI_CUR_HP` | `$FF1A72` | Ali current HP (16-bit word) |
| `ALI_MAX_SP` | `$FF1A74` | Ali maximum SP (16-bit word) |
| `ALI_CUR_SP` | `$FF1A76` | Ali current SP (16-bit word) |
| `POOL2_BASE` | `$FF1CD8` | Dynamic entity pool 2 (17 slots, stride 188 bytes) |

---

## 3. Cleanroom C++20 Interface

```cpp
namespace thor::script {

enum class BitOpMode : uint8_t {
    Set = 0,
    Clear = 1,
    Toggle = 2
};

void execute_opcode_bit_modify(
    std::span<uint8_t> bank,
    BitOpMode mode,
    uint16_t bit_idx,
    const uint8_t*& pc);

void execute_opcode_13_gosub(
    const uint8_t*& pc,
    uint32_t& saved_return_pc,
    const uint8_t* rom_base);

void execute_opcode_14_return(
    const uint8_t*& pc,
    uint32_t saved_return_pc,
    const uint8_t* rom_base);

uint16_t count_living_enemies(std::span<const EntitySlot> pool2_slots);
uint16_t count_active_entities(std::span<const EntitySlot> pool2_slots);
bool find_entity_type(std::span<const EntitySlot> pool2_slots, uint16_t target_type);

uint16_t calc_ali_hp_percent(uint16_t cur_hp, uint16_t max_hp);
uint16_t calc_ali_sp_percent(uint16_t cur_sp, uint16_t max_sp);

} // namespace thor::script
```
