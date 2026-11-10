# M15.244 Read-Only Standalone Handoff: Entity State Dispatch & Nonlocal Sub-Action Abort

**Status:** Read-only engineering reference produced by `C:/Github/Sega-Thor` (Milestone M15.244).  
**Standalone Worktree Policy:** `C:/Github/Sega-Thor-Standalone` remains strictly untouched (`f7f56e5f227870ce61785d838ed953d69cd5c61a`).  
**ROM Evidence:** `Beyond Oasis (USA).md` (`3,145,728` bytes, SHA256 `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`) + pinned GPGX oracle (`ca4be3049e56b1fdfba9a7c727bebbdc645b3d50203d635a468048ba81165134`).

---

## 1. Critical Architectural Warnings for Standalone Implementation

1. **Do NOT import guest CPU machinery:** Standalone C++20 gameplay code must express these mechanics as native entity/action methods and typed state-machine control flow (e.g., `enum class ActionResult { Continue, AbortParentAction }` or explicit early return / structured result), **never** M68000 registers (`D0..D7`, `A0..A7`), CCR flags, or stack-pointer manipulation.
2. **Do NOT assume universal numeric state IDs:** In original ROM memory, `$0004(A6)` stores an **even byte offset** (`2 * table_index`), **not** an abstract enum ID shared across unrelated structs. Notice e.g. `0x005C4C` writes `#0xE` (14 = `2 * 7`, index 7!) into `4(A6)` while `0x005D7E` advances `4(A6)` by `+2` (single table step) and `0x005DAA` writes `#0x5E` (`0x5E / 2 = 47`, entry index 47!). Treat `4(A6)` strictly as a typed dispatcher key scoped to this player/entity sub-action state table until all writers and remaining 52 handlers are audited.

---

## 2. Nonlocal Sub-Action Abort Routine: `0x00E682..0x00E6A8`

### Semantic Identity (Proved across 49 live GPGX captures)
- **Canonical role:** Clear player action lock/timer fields and abort the active sub-action directly back to the grandparent caller when input / cooldown clears.
- **Trigger pattern on the M68000 stack:**
  - `JSR/BSR` caller pushes immediate return PC at `[A7]`; grandparent dispatcher return PC sits at `[A7 + 4]`.
  - At `0x00E6A6`, `ADDQ.L #4, A7` (`0x588F`) discards the immediate caller return PC, and `0x00E6A8 RTS` (`0x4E75`) returns directly to `[A7 + 4]`, producing `a7_delta = +8` bytes and reducing call depth by `2` in a single return.
- **Live Scenario C / A / B proof (`data/thor-evidence/rom_00e682_stack_proof_m15_244.json`):**
  - Total observed authentic nonlocal returns: **49** (`scenario_c`: `4`, `scenario_a`: `43`, `scenario_b`: `2`).
  - Observed bypassed immediate callers: `0x0057D0`, `0x01A54E`, `0x01B7F2`, `0x01BBF8`, `0x01BF84`, `0x01C4DC`, `0x0256B8`.
  - Observed actual grandparent RTS targets: `0x00558A` (via `0x0057D0`, popping stack longword `0x0057D6`), `0x00DF1C` (via enemy/AI/other callers popping `0x01C4E0`, `0x0256BE`, etc.).
- **RAM writes performed inside `0x00E682..0x00E6A8`:**
  - Clears `0x00FF1A1E` (byte), `0x00FF1A16` (word), writes `0x00FF1A6A` (byte) (`2` -> `1` on clear path; exact conditional branch behavior documented in `data/thor-evidence/rom_00e682_semantic_audit_m15_243.json`).
- **Native C++ equivalent:** Return a structured abort signal (`SubActionStatus::AbortToParent`) from the helper to the caller so the caller immediately returns without running its post-call logic.

---

## 3. Player/Entity State Jump Table: `0x0059A8..0x005A30`

- **Dispatcher instructions (`0x0059A8..0x0059B8`):**
  1. `0x0059A8`: `MOVE.W $00FF165E.L, D7` (loads input / controller state word into `D7`)
  2. `0x0059AE`: `MOVE.W $0004(A6), D0` (loads signed 16-bit table byte offset `2 * index` from entity field `+0x04`)
  3. `0x0059B2`: `LEA 6(PC, D0.W), A0` (`A0 = (0x0059BA + (int16_t)D0) & 0xFFFFFF`; hardware does **not** trap here even if out-of-bounds or odd)
  4. `0x0059B6`: `ADDA.W (A0), A0` (reads signed 16-bit PC-relative offset at `A0` — traps Vector 3 Address Error if `A0` is odd; resolves target `(A0 + *(int16_t*)A0) & 0xFFFFFF`)
  5. `0x0059B8`: `JMP (A0)`
- **Exact table bounds (`data/thor-evidence/entity_dispatch_table_manifest_m15_244.json`):**
  - Range: `0x0059BA .. 0x005A30` (**59 entries**, 118 bytes).
  - First instruction immediately after table: `0x005A30` (`BTST.B #$01, $0040(A6)`, opcode `082E 0001 0040`).
  - Unique targets: **57** (`0x005CF8` shared by indices `13, 16`; `0x005DC8` shared by indices `14, 17`). All 59 entries resolve inside `[0x000200, 0x300000)` on even addresses.
  - Index `15` (`0x0059D8`) resolves to `0x005DE4`.
  - Authentic gameplay coverage across Scenarios C (`858`), A (`753`), B (`297`): **1,908** hits across **25 unique indices** (`[0, 1, 2, 4, 5, 6, 7, 11, 12, 13, 14, 15, 19, 23, 24, 27, 28, 29, 32, 49, 50, 52, 53, 57, 58]`) and **25 unique targets**; `0` invalid or out-of-bounds indices observed.

---

## 4. Verified Entity Field Offsets (`A6` Record) & Global Addresses

Observed across the 6 connected compiled routines (`130` instructions, `43` CFG blocks, `data/thor-evidence/entity_dispatch_cfg_source_map_m15_244.json`):

| Field / Global | Width | Routines Reading/Writing | Observed Purpose |
| :--- | :--- | :--- | :--- |
| `+0x02` (`2(A6)`) | `.w` | `0x005CF8`, `0x005DC8`, `0x00780E`? (reads/writes `#0xA`/`#0x14`, `SUBQ.W #1`) | Frame / phase countdown timer |
| `+0x04` (`4(A6)`) | `.w` | `0x0059AE`, `0x005C4C`, `0x005CB4`, `0x005D7E`, `0x005DAA`, `0x005DD8` | Jump-table byte offset (`2 * state_idx`); cleared to `0` on reset (`0x005CB4`) or incremented `+2` (`0x005D7E`, `0x005DD8`) |
| `+0x10` (`16(A6)`) | `.l`/`.w` | `0x005C3A`, `0x005D40`, `0x005D5C` | Vertical / height Z accumulator (`CLR.L` on landing `0x005D5C`) |
| `+0x16` (`22(A6)`) | `.w` | `0x005CA2`, `0x00782C`, `0x007836` | Action / directional index written by `0x005C82` (`D1=3`) & `0x00780E` (`D1` mapped from `+0x30`, or `#2`) |
| `+0x17` (`23(A6)`) | `.b` | `0x007846` | Sub-byte at `+0x17` overwritten from `$00FF198A` |
| `+0x2A` (`42(A6)`) | `.w` | `0x005C46`, `0x005CBA`, `0x005D7A`, `0x005D96`, `0x005DB0`, `0x005DB8`, `0x005DC0` | Animation / sub-step state (`#6`, `#0`, `+=2`, tests `#0x28` / `#0x26` -> `#0x68` or `#0x6C`) |
| `+0x30` (`48(A6)`) | `.w` | `0x007810` | Facing direction code tested against `0x04`, `0x08`, `0x0C` -> `D1 in {3,2,1,0}` |
| `+0x36..0x39` (`54..57(A6)`) | `.b` | `0x005CD0`, `0x005CD6`, `0x005CE0`, `0x00783C`, `0x005C62`, `0x005D60`, `0x005D88`, `0x005DDC` | Entity status bitfields (air/gravity/collision/lock bits via `BSET`/`BCLR`/`BTST`/`ORI`) |
| `+0x40..0x41` (`64..65(A6)`) | `.b` | `0x005CFE`, `0x005D16` | X/Y wall-clip/stop bit #6 flags clearing velocity `+0x4E` / `+0x52` |
| `+0x4A` (`74(A6)`) | `.w` | `0x005CF8`, `0x005DC8` | Set to `#0x10` in air / timer states |
| `+0x4E`, `+0x52`, `+0x56` (`78, 82, 86(A6)`) | `.l` | `0x005C20..0x005C5A`, `0x005CE6..0x005CF2`, `0x005D06..0x005D4E` | Fixed-point X velocity (`+0x4E`), Y velocity (`+0x52`), Z velocity (`+0x56`; gravity `-0xC000` + `0x005D3A` edge tweak) |
| `+0x72`, `+0x76` (`114, 118(A6)`) | `.l` | `0x005C3E`, `0x005C56`, `0x005C5E`, `0x005CEA`, `0x005CF2`, `0x005D4E`, `0x005D56` | Fixed-point world/screen position X (`+0x72`) & Y/Z (`+0x76`) |
| `+0x83` (`131(A6)`) | `.b` | `0x005CC8` | Velocity persistence countdown (`SUBQ.B #1`) |
| `+0x8A` (`138(A6)`) | `.w` | `0x005D74`, `0x005DA6` | Secondary state/input flag checked on air landing (`0x005D5C`) |
| `+0x9A` (`154(A6)`) | `.b` | `0x005D66` | Set to `#0xF` on landing |
| `+0x9E` (`158(A6)`) | `.w` | `0x005C42` | Cleared on knockback setup (`0x005C1A`) |
| `$00FF165E.L` | `.w` | `0x0059A8`, `0x005C82`, `0x005C8A` | Input / control bitmask (`D7` on dispatch; bit `D0` and bit `#0xD` tested in `0x005C82`) |
| `$00FF16EA.L` | `.w` | `0x005C7A` | Cleared after sound/effect call in `0x005C1A` |
| `$00FF1934.L` | `.l` | `0x005CAC`, `0x005CC0` | Saved base stack pointer (`MOVEA.L $00FF1934, A7` performs nonlocal stack unwind before `RTS`) |
| `$00FF196E.L` | `.w` | `0x005C34` | Knockback Z/Y delta added into `+0x10` and `+0x76` |
| `$00FF1970.L` | `.b` | `0x005C94`, `0x005CA6` | Combo/stagger counter (`< 0x0C` guard in `0x005C82`) |
| `$00FF1987.L` | `.b` | `0x005D6C` | Global mode flag affecting landing transition |
| `$00FF198A.L` | `.b` | `0x007846` | Directional/action parameter copied to `+0x17` |
| `$00FF198E.L` | `.b` | `0x005C68` | Cleared (`SF.B`) in `0x005C1A` |

---

## 5. Control-Flow & Uncompiled Exits Leaving These 6 Compiled Routines

1. **`0x005C1A` (table index 23, `D0 = 0x2E`):** Calls `BSR.B 0x005C82` (`D0=2, D1=3`). If `0x005C82` overwrites `A7 = $00FF1934` and returns (`0x005CAC`/`0x005CC0`), `0x005C20..0x005C80` is **skipped** (nonlocal unwind!). Otherwise continues at `0x005C20`, invokes uncompiled `JSR 0x00060004` (`D0 = 0x800340` SFX trigger at `0x005C74`), and resumes `0x005C7A..0x005C80 RTS`.
2. **`0x005CC8` (table index 12, `D0 = 0x18`):** Decrements `+0x83`; if zero, clears bit 6 of `+0x36` & bit 3 of `+0x39` and exits via uncompiled tail `BRA.W 0x0062CC` (`ValidUncompiledPc`); otherwise integrates velocities `+0x4E/+0x52` into `+0x72/+0x76` and `RTS`.
3. **`0x005CF8` (shared table indices 13 & 16):** Calls `JSR 0x00060004` (`D0 = 0x80035D` at `0x005D10`, `0x005D28` if wall-stop bits hit), calls `BSR.W 0x00B79A` (`0x005D46`) while falling above ground, or transitions into landing (`0x005D5C`) updating `+0x04` and `+0x2A` before `RTS`.
4. **`0x005DC8` (shared table indices 14 & 17):** Sets `+0x4A = 0x10`, decrements `+0x02`, and when zero advances `+0x2A += 2`, `+0x04 += 2`, clears bit 3 of `+0x39`, then `RTS`.
5. **`0x00780E` (table index 32, `D0 = 0x40`):** Maps `+0x30` in `{4,8,12}` to `+0x16` in `{3,2,1,0}`, calls uncompiled `BSR.W 0x00BCBA` (`0x007832`, `D0 = 2`), resumes at `0x007836` setting `+0x16 = 2`, checks bit 1 of `+0x37` (`0x00783C`), either `RTS` or copies `$00FF198A` -> `+0x17` and tail-branches `BRA.W 0x0062CC`.
6. **Remaining 52 uncompiled table targets:** `0x0061F6` (`idx=0`), `0x0062E4` (`idx=1`), `0x006338` (`idx=2`), `0x006516` (`idx=4`), `0x007112` (`idx=11`), etc. — remain uncompiled exits (`ValidUncompiledPc`) transferred seamlessly back to the hybrid scheduler/GPGX core.

---

## 6. Frozen Trace & Artifact Hashes for Regression Verification

- **CFG & Source Map:** `data/thor-evidence/entity_dispatch_cfg_source_map_m15_244.json` (`130` instructions, `43` blocks, `123` edges, `generated_ir_sha256 = b3c5e512355d2f8197e43d32591cfcfd6d6025dcdb1fd28be3cf183308d0ab40`, `generated_cpp_umbrella_sha256 = a4ee3898f36cb6424244a2f4773b4fdbdbd4678f77c4d59db52d72d224f0e393`).
- **20 GPGX Reference Fixtures:** `data/thor-fixtures/m15_244_entity_dispatch_fixtures.json` (`10 AUTHENTIC_GAMEPLAY_ENTRY` + `10 CONTROLLED_CPU_ENTRY`, 20/20 3-way differential bit-exact & cycle-exact match).
- **Hybrid Gameplay Runner Baseline (`24/24` bit-exact stream hashes across Scenarios C, A, B):**
  - `scenario_c` `full-frame-state.jsonl`: `188f329c8919400406c72c783c7ada5992905490d465a6e7463a885e4017d550`
  - `scenario_a` `full-frame-state.jsonl`: `499ac47714012e61a02f93aed4a32e016cc6fad8cb9407f3ce4c004bfb331040`
  - `scenario_b` `full-frame-state.jsonl`: `f2e3fcdbd51475562e3cb123acd505aa02e4f4970c3daa18cdca9eebe639d0c8`
