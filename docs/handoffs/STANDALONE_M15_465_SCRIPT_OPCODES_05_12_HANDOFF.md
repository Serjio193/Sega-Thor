# Standalone Handoff: Milestone M15.465 — ROM Script Virtual Machine Opcodes $05 & $12: Unconditional Jump & Conditional Branch Engine

**ROM Range:** `0x00FAAC..0x00FC6C` (448 bytes)  
**Status:** Reverse-engineered, verified with Capstone M68K disassembly, JSON evidence generated, and unit tests passing.

---

## 1. System Overview

This subsystem implements unconditional control-flow transfers and conditional branching within the Cutscene and Event Script Bytecode Virtual Machine:
1. **Opcode `$05` (`0x00FAAC..0x00FAB2`): Unconditional Relative Jump (`BRA`):**
   - Directly adds the 16-bit word displacement at `+2(a0)` to the current script PC `a0` (`adda.w $2(a0), a0; rts`).
   - Supports forward and backward jumps across the script stream.
2. **Opcode `$12` (`0x00FAB2..0x00FC6C`): Conditional Branch / If-Goto (`BRANCH_IF`):**
   - Evaluates a condition. If the condition is met, branches by displacement `+6(a0)` (`adda.w $6(a0), a0; rts`).
   - If the condition fails, skips the 8-byte instruction and proceeds immediately to the next sequential instruction (`lea.l $8(a0), a0; rts`).
   - Dispatches source variables through a 12-entry mode jump table at `0x00FAC2`:
     - **Mode 0 (`0x00FADA`):** Calls subroutine `0x0101EC` to retrieve game/quest variable, evaluates against relational jump table at `0x00FB64`.
     - **Mode 1 (`0x00FBAC`):** Tests persistent bitmask in RAM Bank 1 (`$FF0DBC`). Polarity on `+3(a0)` (`0` = branch if bit set, `!= 0` = branch if bit clear).
     - **Mode 2 (`0x00FBDC`):** Tests persistent bitmask in RAM Bank 2 (`$FF0E1C`).
     - **Mode 3 (`0x00FBFC`):** Tests persistent bitmask in RAM Bank 3 (`$FF0E2C`).
     - **Mode 4 (`0x00FC1C`):** Tests item inventory possession via subroutine `0x010232`.
     - **Mode 5 (`0x00FAE8`):** Calls subroutine `0x010254`.
     - **Mode 6 (`0x00FAF6`):** Calls subroutine `0x010268`.
     - **Mode 7 (`0x00FC42`):** Compares companion spirit status `$FF1AA4` with immediate value.
     - **Mode 8 (`0x00FB04`):** Player Ali X world coordinate (`$FF19F0`).
     - **Mode 9 (`0x00FB14`):** Player Ali Y world coordinate (`$FF19F4`).
     - **Mode 10 (`0x00FB24`):** Spirit companion X world coordinate (`$FF1AAC`) when active (`$FF1AA4 == $1A`). If inactive, fails branch directly.
     - **Mode 11 (`0x00FB40`):** Spirit companion Y world coordinate (`$FF1AB0`) when active (`$FF1AA4 == $1A`). If inactive, fails branch directly.
   - Relational operator jump table at `0x00FB64`:
     - `0`: `==` (Equal)
     - `1`: `!=` (Not Equal)
     - `2`: `>` (Greater)
     - `3`: `>=` (Greater or Equal)
     - `4`: `<=` (Less or Equal)
     - `5`: `<` (Less)

---

## 2. Memory & Architecture Reference

| Symbol | Address | Description |
|---|---|---|
| `MODE_TABLE_BASE` | `0x00FAC2` | 12-entry relative word jump table for test variables |
| `OPERATOR_TABLE_BASE` | `0x00FB64` | 6-entry relative word jump table for relational comparisons |
| `BRANCH_TAKEN_TARGET` | `0x00FBA0` | `adda.w $6(a0), a0; rts` |
| `BRANCH_NOT_TAKEN_TARGET` | `0x00FBA6` | `lea.l $8(a0), a0; rts` |
| `BANK1_BITS` | `$FF0DBC` | Persistent bitmask bank 1 (`bit = comp & 7, byte = comp >> 3`) |
| `BANK2_BITS` | `$FF0E1C` | Persistent bitmask bank 2 |
| `BANK3_BITS` | `$FF0E2C` | Persistent bitmask bank 3 |
| `ALI_POS_X` | `$FF19F0` | Player Ali X coordinate |
| `ALI_POS_Y` | `$FF19F4` | Player Ali Y coordinate |
| `SPIRIT_STATUS` | `$FF1AA4` | Spirit companion active flag (`$1A` = active) |
| `SPIRIT_POS_X` | `$FF1AAC` | Spirit companion X coordinate |
| `SPIRIT_POS_Y` | `$FF1AB0` | Spirit companion Y coordinate |

---

## 3. Cleanroom C++20 Interface

```cpp
namespace thor::script {

void execute_opcode_05_unconditional_jump(
    const uint8_t*& pc,
    int16_t displacement);

void execute_opcode_12_conditional_branch(
    const uint8_t*& pc,
    uint8_t mode,
    uint8_t op,
    uint16_t comp_val,
    int16_t branch_displacement,
    const ScriptEnv& env);

} // namespace thor::script
```
