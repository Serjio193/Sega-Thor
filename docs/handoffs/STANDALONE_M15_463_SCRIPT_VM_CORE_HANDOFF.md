# Standalone Handoff: Milestone M15.463 — ROM Cutscene & Event Script Virtual Machine Core, 98-Opcode Dispatcher & Camera Pan Engine

**ROM Range:** `0x00F788..0x00F920` (408 bytes)  
**Status:** Reverse-engineered, verified with Capstone M68K disassembly, JSON evidence generated, and unit tests passing.

---

## 1. System Overview

This subsystem implements the foundational Cutscene and Event Script Bytecode Virtual Machine used to drive scripted in-game cutscenes, boss introductions, NPC interactions, and camera sequences:
1. **Script Engine Per-Frame Tick & Delay Controller (`0x00F788..0x00F7B4`):**
   - Checks `$FF17B8` bit 1: if set, script processing is paused/disabled for this frame.
   - Checks delay timer `$FF17B6`: if non-zero, decrements by 1 and exits, stalling execution for `N` frames.
   - Checks camera pan timer `$FF17BD`: if non-zero, delegates to the camera pan state machine at `0x00F894`.
2. **Bytecode Interpretation & Dispatch Loop (`0x00F7B4..0x00F7CE`):**
   - Loads instruction stream pointer `a0` from `$FF17B2`.
   - Loops fetching opcode byte: `d1.b = *(a0)++`.
   - Indexes 98-entry jump table at `0x00F7D0` via `lea.l $F7D0(pc, d1.w * 2), a1; adda.w (a1), a1`.
   - Calls handler: `jsr (a1)`.
   - Handler returns updated `a0`, which is written back to `$FF17B2`.
   - Repeats until an opcode handler exits the loop.
3. **Camera Pan State Machine & Player Lock Release (`0x00F894..0x00F8C8`):**
   - Dispatches direction mode (`$FF183A`, 0..3) via 4-entry mode table at `0x00F8C8`.
   - Mode 0 (Up): camera Y (`$FF1A5E`) -= `speed << 16`.
   - Mode 1 (Right): camera X (`$FF1A5A`) += `speed << 16`.
   - Mode 2 (Down): camera Y (`$FF1A5E`) += `speed << 16`.
   - Mode 3 (Left): camera X (`$FF1A5A`) -= `speed << 16`.
   - Decrements pan timer `$FF17BD`. When reaching 0, clears camera lock flag `$FF1982 = 0` and restores player input suppression `$FF19EC = 0, $FF1A12 = 0`.
4. **Opcode 00 Frame Yield Exit (`0x00F91C..0x00F920`):**
   - Executes `addq.l #4, sp; rts`, popping the dispatch loop return address and concluding script interpretation for the current frame.

---

## 2. Memory & Architecture Reference

| Symbol | Address | Description |
|---|---|---|
| `SCRIPT_STREAM_PC` | `$FF17B2` | Current 32-bit script instruction pointer (ROM/RAM) |
| `SCRIPT_DELAY_TIMER` | `$FF17B6` | Frame delay counter |
| `SCRIPT_FLAGS` | `$FF17B8` | Bit 1: script execution paused/disabled |
| `SCRIPT_PAN_TIMER` | `$FF17BD` | Active camera pan frame counter |
| `SCRIPT_PAN_MODE` | `$FF183A` | Pan direction: 0=Up, 1=Right, 2=Down, 3=Left |
| `SCRIPT_PAN_SPEED` | `$FF183C` | Pan speed in pixels per frame (16-bit integer) |
| `CAMERA_X_POS` | `$FF1A5A` | 32-bit subpixel camera X coordinate |
| `CAMERA_Y_POS` | `$FF1A5E` | 32-bit subpixel camera Y coordinate |
| `CAMERA_LOCK_FLAG` | `$FF1982` | Camera tracking lock flag (cleared when pan finishes) |
| `PLAYER_SUPPRESS_19EC` | `$FF19EC` | Player motion suppression flag |
| `PLAYER_SUPPRESS_1A12` | `$FF1A12` | Player action suppression flag |
| `BYTECODE_JUMP_TABLE` | `0x00F7D0` | 98-entry 16-bit relative word jump table |
| `CAMERA_PAN_TABLE` | `0x00F8C8` | 4-entry 16-bit relative word pan mode table |

---

## 3. Cleanroom C++20 Interface

```cpp
namespace thor::script {

enum class PanDirection : uint16_t {
    Up = 0,
    Right = 1,
    Down = 2,
    Left = 3
};

struct ScriptVmState {
    const uint8_t* pc{nullptr};
    uint8_t delay_timer{0};
    bool paused{false};
    uint8_t pan_timer{0};
    PanDirection pan_mode{PanDirection::Up};
    uint16_t pan_speed{0};
    int32_t camera_x{0};
    int32_t camera_y{0};
    bool camera_locked{false};
    uint16_t player_suppress_19ec{0};
    uint16_t player_suppress_1a12{0};
};

enum class VmTickResult {
    Paused,
    Delayed,
    Panning,
    Interpreted
};

VmTickResult tick_script_vm(
    ScriptVmState& state,
    std::function<bool(uint8_t opcode, ScriptVmState& state)> opcode_dispatcher);

void execute_camera_pan_step(ScriptVmState& state);

} // namespace thor::script
```
