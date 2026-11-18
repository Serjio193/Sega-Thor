# Standalone Handoff: Milestone M15.464 — ROM Script Virtual Machine Opcodes $02..$04: Parameter Initializers & Relational Wait-Condition Engine

**ROM Range:** `0x00F920..0x00FAAC` (396 bytes)  
**Status:** Reverse-engineered, verified with Capstone M68K disassembly, JSON evidence generated, and unit tests passing.

---

## 1. System Overview

This subsystem implements three fundamental opcodes of the Story of Thor Cutscene & Event Script Virtual Machine:
1. **Opcode `$02` (`0x00F920..0x00F942`): Parameter Word Buffer Initializer:**
   - Reads a sequence of 16-bit words from the script stream `(a0)+` terminated by `$FFFF`.
   - Copies up to 16 words into `$FF17C6` (script word parameter buffer).
2. **Opcode `$03` (`0x00F942..0x00F96A`): Target Coordinate Pair Table Initializer:**
   - Reads coordinate / entity target word pairs `(X, Y)` from script stream into `$FF17E6` (up to 16 pairs / 32 bytes).
   - If `$FFFF` terminator is reached before 16 pairs, pads all remaining pairs in the table with `($FFFF, $FFFF)`.
3. **Opcode `$04` (`0x00F96A..0x00FAAC`): Wait-Until Condition & Relational Expression Evaluator:**
   - Evaluates a condition and holds execution (yields the frame via `bra.w $F91C`) until the condition evaluates to true.
   - Decodes:
     - `d0.b`: Variable source selector:
       - `0`: Quest / game scenario state variable via subroutine `0x0101EC`.
       - `1`: Persistent bit in Bank 1 (`$FF0DBC`).
       - `2`: Persistent bit in Bank 2 (`$FF0E1C`).
       - `3`: Persistent bit in Bank 3 (`$FF0E2C`).
       - `4`: Player Ali current X coordinate (`$FF19F0`).
       - `5`: Player Ali current Y coordinate (`$FF19F4`).
       - `6`: Spirit companion X coordinate (`$FF1AAC`) when active (`$FF1AA4 == $1A`).
       - `7`: Spirit companion Y coordinate (`$FF1AB0`) when active (`$FF1AA4 == $1A`).
       - default: 0.
     - `d1.b`: Comparison operator:
       - For bit tests (`d0 = 1..3`): polarity (`0` = expect bit set, `!= 0` = expect bit clear).
       - For numerical comparisons:
         - `0`: `==` (Equal)
         - `1`: `!=` (Not Equal)
         - `2`: `>` (Greater)
         - `3`: `>=` (Greater or Equal)
         - `4`: `<=` (Less or Equal)
         - `5`: `<` (Less)
     - `d2.w`: Comparison operand / bit index.
   - If condition is true: pops handler stack frame (`addq.l #4, sp; rts`) to advance to next script instruction.
   - If condition is false: stores current script PC to `$FF17B2` and branches to `$F91C`, yielding the current frame and retrying on subsequent frames.

---

## 2. Memory & Architecture Reference

| Symbol | Address | Description |
|---|---|---|
| `SCRIPT_PARAM_BUFFER` | `$FF17C6` | 16-word script argument / parameter buffer |
| `SCRIPT_COORD_TABLE` | `$FF17E6` | 16-pair coordinate / entity target table |
| `BANK1_BITS` | `$FF0DBC` | RAM persistent bitmask bank 1 (`bit = d2 & 7, byte = d2 >> 3`) |
| `BANK2_BITS` | `$FF0E1C` | RAM persistent bitmask bank 2 |
| `BANK3_BITS` | `$FF0E2C` | RAM persistent bitmask bank 3 |
| `ALI_POS_X` | `$FF19F0` | Player Ali X world coordinate |
| `ALI_POS_Y` | `$FF19F4` | Player Ali Y world coordinate |
| `SPIRIT_STATUS` | `$FF1AA4` | Companion active status (`$1A` = active) |
| `SPIRIT_POS_X` | `$FF1AAC` | Companion Spirit X coordinate |
| `SPIRIT_POS_Y` | `$FF1AB0` | Companion Spirit Y coordinate |
| `OPERATOR_TABLE` | `0x00F9EC` | 6-entry relational jump table |

---

## 3. Cleanroom C++20 Interface

```cpp
namespace thor::script {

struct ScriptEnv {
    std::span<uint8_t> bank1;
    std::span<uint8_t> bank2;
    std::span<uint8_t> bank3;
    int16_t ali_x{0};
    int16_t ali_y{0};
    uint16_t spirit_status{0};
    int16_t spirit_x{0};
    int16_t spirit_y{0};
    std::function<int16_t()> get_quest_var;
};

void execute_opcode_02_set_params(
    const uint8_t*& pc,
    std::array<uint16_t, 16>& param_buffer);

void execute_opcode_03_set_coords(
    const uint8_t*& pc,
    std::array<std::pair<uint16_t, uint16_t>, 16>& coord_table);

bool execute_opcode_04_wait_condition(
    const uint8_t*& pc,
    const ScriptEnv& env);

} // namespace thor::script
```
