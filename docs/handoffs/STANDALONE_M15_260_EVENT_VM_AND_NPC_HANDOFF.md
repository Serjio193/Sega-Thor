# STANDALONE M15.260 EVENT SCRIPT VM & NPC DIALOGUE HANDOFF

**Milestone:** SEGA-THOR HYBRID — M15.260  
**Repository:** `C:/Github/Sega-Thor`  
**Branch:** `codex/helper10-99d6-bounded-scout`  
**Baseline Git Commit:** `229a7e64dc23f0e19a7a375f2d73c55e1f31b2ff`  
**Standalone Repository (`C:/Github/Sega-Thor-Standalone`):** STRICTLY READ-ONLY / UNTOUCHED  
**Machine-Readable Evidence Output:** `data/thor-evidence/rom_event_script_vm_m15_260.json`  
**Extraction Tool:** `tools/extract_rom_event_script_vm.py`  
**Deterministic Test Suite:** `tests/rom_event_script_vm_test.py`

---

## 1. Phase 0 — Baseline & Current Standalone Audit (`Standalone M16.46 -> M16.47`)

A read-only audit of `C:/Github/Sega-Thor-Standalone` (M16.46 commit `81afa46` and worktree preparation for M16.47 in `src/game/npc_dialogue_handler.cpp`, `event/event_vm.hpp`, and `data/standalone-fixtures/npc_interaction_differential_m16_47.json`) revealed the exact status of the Standalone event runtime:

1. **Dialogue Table Addressing Discrepancy:**
   - Standalone's provisional prototype in `npc_interaction_differential_m16_47.json` hypothesized that the dialogue table was at `0x051520` and that Village Elder was Message ID `14` (`0x078C` offset).
   - **Authentic ROM Architecture Discovery:** The authentic M68k dialogue engine at `0x00C3FE` loads from table base `0x051514` via self-relative 16-bit signed offsets (`a0 = entry + *(int16_t*)entry`).
   - In the authentic table, Village Elder is **Message ID 5** (entry `0x05151E`, offset `0x026E` -> ROM text address `0x05178C`). Message ID 14 is actually the story recount ("He found Efreet, a spirit of fire").
2. **Event VM Opcodes:**
   - Standalone M16.47 implemented only 5 hardcoded opcodes (`0x00`, `0x07`, `0x0A`, `0x14`, `0x15`).
   - The complete ROM opcode jump table at `0x00F7D0` contains **31 opcodes (`0x00` through `0x1E`)**.
3. **Room Spawn Script Pipeline:**
   - Standalone lacked the master area table at `0x04371E` which dynamically indexes room spawn scripts for all 188 rooms across all 21 areas using 16-bit self-relative offsets.
4. **NPC Interaction & Dialogue Execution:**
   - Standalone hardcoded raw dialogue offsets (`0x05178C` and `0x0517F2`) rather than passing entity message IDs (`$A0(entity)`) to the universal dialogue engine (`0x00C326`).

---

## 2. Phase 1 — ROM Event Script VM Architecture

### 2.1 Core Routines & Call Graph

| Routine | ROM Address | Role & Registers | Semantics |
|---|---|---|---|
| `step_room_spawn_script_vm` | `0x00F788..0x00F7D0` | Input: `$FF17B2` (script PC), `$FF17B6` (wait timer), `$FF17B8:1` (pause flag) | Decrements `$FF17B6` if active; if zero and `$FF17BD == 0`, enters instruction loop reading `(a0)` -> dispatches via `0x00F7D0`. |
| `opcode_jump_table` | `0x00F7D0..0x00F80E` | Input: `d1.w = opcode * 2` | Jump table of 31 self-relative 16-bit words (`target = entry + *(int16_t*)entry`). Exactly opcodes `0x00`..`0x1E`. |
| `preload_room_spawn_script_vm` | `0x00F80E..0x00F854` | Called during room load (`0x00D406`) | Executes leading opcodes `0x0A`, `0x12`, `0x13`, `0x14` prior to tilemap decompression. Bails on any other opcode. |
| `master_area_spawn_table` | `0x04371E` | 21 32-bit pointers (one per world area) | Area 1 pointer is `0x046332`. Each area table contains 16-bit self-relative offsets (`entry + *(int16_t*)entry`) for each room. |
| `dialogue_table` | `0x051514` | 98 16-bit self-relative offsets | Text address is `entry + *(int16_t*)entry`. Delimiters: `0x01` newline, `0x07` page pause, `0x00` end, `#` speaker border tag. |
| `dialogue_engine_entry` | `0x00C326..0x00C4AA` | Input: `d0.w = message_id`, `d6.w/d7.w = window_params` | Decompresses font graphics to `$FF2FA8` via `0x003820`, queues DMA, steps text stream, renders glyphs, handles button waits. |
| `dialogue_frame_step_loop` | `0x00C84E` | Internal loop during text render | Sets `$FF185D = 0xFF`, requests frame `$FF1654 = 2`, updates sound via `0x008E22`, waits for VINT, refreshes SAT via `0x00A196`. |
| `dialogue_button_wait` | `0x00C536` | Controller wait for page/dialogue advance | Polls `$FF165F` bits 4 (B), 5 (C), 6 (A). If any pressed, advances; otherwise calls `0x00C84E`. |
| `npc_interaction_check` | `0x018BBC` & `0x018C28` | Proximity check (`dx <= 28 && dy <= 28`) | Locks player (`bset.b #1, $36(player)` = `0x36`), reads `move.w $A0(npc), d0`, calls `0x00C2EC` to start dialogue. |

---

## 3. Phase 2 — Complete Opcode Inventory (31 Opcodes)

| Opcode | Handler | Name | Operand Length | Types | Semantics & State Changes |
|---|---|---|---|---|---|
| `0x00` | `0x00F91C` | `VM_OP_HALT_YIELD` | 0 | None | Pops caller return frame (`addq.l #4, a7; rts`); yields execution until next frame/trigger. |
| `0x01` | `0x00FCCE` | `VM_OP_SPAWN_ENEMY` | 3 | `u8:flag, u16:type` | Checks active enemy cap (`$FF17B7`); allocates entity slot via `0x00DA2A`; sets type and calls `0x008D06`. |
| `0x02` | `0x00F920` | `VM_OP_LOAD_WORD_TABLE_17C6` | Var | `u16[]:words_until_ffff` | Populates parameter array at `$FF17C6` until `0xFFFF` terminator. |
| `0x03` | `0x00F942` | `VM_OP_LOAD_WORD_PAIRS_17E6` | Var | `(u16,u16)[]:pairs` | Populates coordinate pair array at `$FF17E6` until `0xFFFF` terminator. |
| `0x04` | `0x00F96A` | `VM_OP_CONDITIONAL_BRANCH` | 4 | `u8:cond, u8:param, i16:disp` | Evaluates condition (0=entity dead/count, 1=player coord, 2=item, 3=frame timer, 4=wave defeat); branches on match. |
| `0x05` | `0x00FAAC` | `VM_OP_BRANCH_UNCONDITIONAL` | 2 | `i16:relative_disp` | Relative branch in script: `adda.w 2(a0), a0`. |
| `0x06` | `0x00FC6C` | `VM_OP_LOOP_DECREMENT_BRANCH` | 3 | `u8:counter_idx, i16:disp` | Decrements counter at `$FF1826[idx]`; if non-zero, branches by displacement. |
| `0x07` | `0x00FC94` | `VM_OP_SET_COUNTER` | 3 | `u8:counter_idx, u16:count` | Initializes loop counter at `$FF1826[idx]` to count. |
| `0x08` | `0x00FCA6` | `VM_OP_WAIT_FRAMES` | 1 | `u8:frames` | Sets wait countdown in `$FF17B6`, saves PC to `$FF17B2`, yields execution. |
| `0x09` | `0x010034` | `VM_OP_SPAWN_PROJECTILE_SPECIAL` | 7 | `u8:mode, u16:params, i16:x, i16:y` | Allocates entity slot via `0x00D9BE`; initializes projectile type 8 at world coordinates. |
| `0x0A` | `0x00FCB8` | `VM_OP_PRELOAD_NEMESIS_DMA` | 7 | `u8:flag, u32:rom_src, u16:vram_dst` | Enqueues Nemesis pattern decompression/DMA stream via `0x00D9A4` into VRAM destination. |
| `0x0B` | `0x00FCC4` | `VM_OP_SET_MAX_ENEMIES` | 1 | `u8:max_count` | Updates active enemy concurrency cap in `$FF17B7`. |
| `0x0C` | `0x010086` | `VM_OP_LOAD_PALETTE_STAGE` | 2 | `u16:palette_idx` | Copies 32 bytes from `0x05D046` to `$FF136C` without immediate VDP reload flag. |
| `0x0D` | `0x0100AA` | `VM_OP_LOAD_PALETTE_APPLY` | 2 | `u16:palette_idx` | Copies 32 bytes from `0x05D046` to `$FF136C` and flags VDP reload (`bset.b #0, $FF164D`). |
| `0x0E` | `0x00FD8C` | `VM_OP_SPAWN_COMPLEX_ENEMY` | 11 | `u8:flags, u16:type, i16:x, i16:y, u32:extra` | Checks enemy cap, allocates slot, configures multi-piece enemy via `0x0102BE`. |
| `0x0F` | `0x010140` | `VM_OP_SET_GLOBAL_FLAG` | 3 | `u8:mode, u16:flag_index` | Sets (1), clears (0), or toggles (2) bitflag in global table `$FF0DBC` (`bit = idx & 7`, `byte = idx >> 3`). |
| `0x10` | `0x010170` | `VM_OP_SET_AREA_FLAG` | 3 | `u8:mode, u16:flag_index` | Sets (1), clears (0), or toggles (2) bitflag in area table `$FF0E1C`. |
| `0x11` | `0x0101A0` | `VM_OP_SET_ROOM_FLAG` | 3 | `u8:mode, u16:flag_index` | Sets (1), clears (0), or toggles (2) bitflag in room table `$FF0E2C`. |
| `0x12` | `0x00FAB2` | `VM_OP_TILEMAP_SPECIAL_ACTION` | Var | `u8:action_id, varies` | Sub-dispatcher via `0x00FAC2` (12 sub-actions: animated tiles, plane priority, water current). |
| `0x13` | `0x0101D0` | `VM_OP_CALL_SUBROUTINE` | 3 | `u24:rom_target` | Saves current PC in `$FF1836` and jumps to target bytecode address. |
| `0x14` | `0x0101E4` | `VM_OP_RETURN_SUBROUTINE` | 0 | None | Restores script PC from `$FF1836` (subroutine return). |
| `0x15` | `0x00FEDE` | `VM_OP_SPAWN_GATED_PROP` | 17 | `u8:bank, u16:type, u16:flag_idx, u8:f6, u8:f7, u16:msg, i16:x, i16:y, i16:z, u16:vram` | Tests bitflag. If clear, spawns entity via `0x00DA2A + 0x00DF54`. If set, skips 18-byte record. |
| `0x16` | `0x00FF1A` | `VM_OP_SPAWN_STATIC_PROP` | 17 | Same 18-byte record as 0x15 | Unconditionally spawns entity using identical record layout without flag condition test. |
| `0x17` | `0x00FE0A` | `VM_OP_SPAWN_ENEMY_WAVE` | 5 | `u8:flags, u16:type_a, u16:type_b` | Spawns paired wave entities (`#$72` and `#$70`) and synchronizes wave scheduler via `0x010300`. |
| `0x18` | `0x00FF98` | `VM_OP_SPAWN_ITEM_CHEST` | 11 | `u8:payload, u16:type, u32:func, i16:x, i16:y` | Allocates slot via `0x00D9F0`; spawns chest/item entity with payload ID and open callback `0x009AD6`. |
| `0x19` | `0x00FFDE` | `VM_OP_SPAWN_TRIGGER_OBJECT` | 17 | `u8:payload, u16:type, i16:x, i16:y, varies` | Spawns interactive trigger entity (door, switch, chest) with default trigger callback `0x017A96`. |
| `0x1A` | `0x0100D6` | `VM_OP_PLAY_SOUND_EFFECT` | 8 | `u16:sound_id, u16:vol, u32:pan` | Passes sound effect IDs and volume/panning to Z80 sound driver via `0x00E78E`. |
| `0x1B` | `0x0100E4` | `VM_OP_PLAY_BGM_TRACK` | 4 | `u32:bgm_command` | Sends BGM playback command to sound driver via `0x060004`. |
| `0x1C` | `0x0100EE` | `VM_OP_SET_ROOM_PARAMETER` | 2 | `u16:param` | Stores room-specific parameter word in `$FF17C4`. |
| `0x1D` | `0x0100F8` | `VM_OP_PLANE_SCROLL_EFFECT` | 3 | `u8:plane_sel, i16:scroll_delta` | Configures auto-scroll delta on Plane A (`$FF1716`) or Plane B (`$FF173E`) via `0x00DB64`. |
| `0x1E` | `0x00F854` | `VM_OP_CAMERA_CUTSCENE_LOCK` | 5 | `u8:mode, u16:cam_x, u16:cam_y` | Locks camera on coordinates, sets cutscene active `$FF1982=0xFF`, sets camera mode `$FF19EC=2`, yields. |

---

## 4. Phase 3 — Village Elder Screen(1,8) Execution Sequence

### 4.1 Room 0x0108 Spawn Script (`0x0466B2`)
In Area 1 spawn table (`0x046332`), entry 8 (`0x046342`) contains offset `0x0370`, resolving to script `0x0466B2`:

```text
0x0466B2: 0A 00 00 16 61 20 92 00
          -> Opcode 0x0A (VM_OP_PRELOAD_NEMESIS_DMA):
             ROM src = 0x166120, VRAM dst = 0x9200 (Tile base 0x0490)

0x0466BA: 15 01 00 3C 00 8D 00 00 00 00 00 A0 00 A0 02 32 04 90
          -> Opcode 0x15 (VM_OP_SPAWN_GATED_PROP):
             Flag bank = 1 ($FF0DBC Global Flags)
             Entity type = 0x003C (Village Elder)
             Flag index = 141 (0x008D -> byte $FF0DBC+17, bit 5)
             World pos = (X=160, Y=160, Z=562)
             VRAM tile base = 0x0490

0x0466CC: 00
          -> Opcode 0x00 (VM_OP_HALT_YIELD)
```

### 4.2 Interaction & Text Decoding
1. **Trigger Condition:**
   - Player position `(px, py)` within distance `dx <= 28, dy <= 28` of Elder `(160, 160)`.
   - Player facing North (`direction_code == 0` or `py > 160`).
   - Button A or C pressed.
2. **Lock State:**
   - Player state set to `0x36` (`bset.b #1, $36(player)` in `0x018C72`), freezing movement.
3. **Dialogue Invocation:**
   - If Event Flag 141 is clear: reads Message ID 5.
   - Entry address: `0x051514 + 5 * 2 = 0x05151E`.
   - Offset: `0x026E` -> Text address `0x05178C`.
   - **Page 1:**
     ```text
     Prince Ali, are you all right?
     I don't know who they were,
     but you were their target.
     ```
   - Control code `0x07` (Page Pause): halts, waits for player to press A, B, or C (`0x00C536`).
   - **Page 2:**
     ```text
     They may have attacked the
     castle as well.  You must check
     on your family.
     ```
   - Control code `0x00` (End of Dialogue): waits for final confirmation press.
4. **Completion:**
   - Sets Event Flag 141 (`$FF0DBC + 17`, bit 5).
   - Clears dialogue active flag `$FF185D`.
   - Restores player state `0x00`, unfreezing simulation.

---

## 5. Phase 4 — Proposed Standalone C++20 Architecture

```cpp
namespace oasis::game::event {

// Delimiters in authentic ROM dialogue
enum class TextControlCode : std::uint8_t {
    EndOfDialogue = 0x00,
    LineFeed      = 0x01,
    ClearPage     = 0x02,
    FrameDelay    = 0x03,
    WaitButton    = 0x05,
    ScrollLines   = 0x06,
    PagePause     = 0x07,
};

// Decoded dialogue sequence from authentic ROM table at 0x051514
struct DialogueRecord {
    std::uint16_t message_id{0};
    std::uint32_t text_rom_addr{0};
    std::vector<std::vector<std::string>> pages{};
};

// Decodes dialogue using authentic self-relative addressing
DialogueRecord decode_dialogue_record(
    std::span<const std::uint8_t> rom,
    std::uint16_t message_id);

// Minimal Event & Spawn VM
class EventScriptVm {
public:
    void reset() noexcept;
    bool execute_preload(std::span<const std::uint8_t> rom, std::uint32_t script_addr) noexcept;
    void step_frame(std::span<const std::uint8_t> rom, bool advance_input) noexcept;

    [[nodiscard]] bool get_global_flag(std::uint16_t flag_idx) const noexcept;
    void set_global_flag(std::uint16_t flag_idx, bool val = true) noexcept;

private:
    std::array<std::uint8_t, 64> global_flags_{}; // $FF0DBC
    std::array<std::uint8_t, 16> area_flags_{};   // $FF0E1C
    std::array<std::uint8_t, 16> room_flags_{};   // $FF0E2C
    std::uint32_t pc_{0};
    std::uint8_t wait_timer_{0};
};

} // namespace oasis::game::event
```

---

## 6. Implementation Order & Acceptance Criteria for Standalone M16.47

1. **Step 1: Correct Dialogue Table Base to `0x051514`**
   - Update `decode_dialogue_from_rom` to index `0x051514 + msg_id * 2` with self-relative signed 16-bit offset.
   - Set Village Elder Message ID to `5` (`0x05178C`).
2. **Step 2: Wire Village Elder Spawn Record**
   - In `Screen(1, 8)` initialization, evaluate Flag 141 (`$FF0DBC+17` bit 5).
   - If clear, spawn entity `0x003C` at `(160, 160, 562)` with VRAM tile `0x0490`.
3. **Step 3: Connect NPC Proximity & Talk Button**
   - Proximity check: `dx <= 28 && dy <= 28`.
   - Player talk lock state: `0x36`.
   - Dialogue advance on Button A, B, or C.
   - Upon page 2 completion, set Flag 141 and unlock player.
4. **Step 4: Regression Verification**
   - Verify holdout differential tests remain green with 0 regression across all 21 combat frames (`F775..F795`).
