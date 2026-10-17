# ADR-M15-55 — Execution-Driven Recompilation: 0x008B8E Per-Frame State Gate & Mode Transition Dispatcher Integration
**Status:** Accepted
**Date:** 2026-10-17

**Context:** Following the displacement of `0x005670`, fresh actual-ROM execution census ranked `0x008B8E` #1 with 772 executions from caller `0x008B26` (return PC `0x008B2A`, displacement `+0x0066`).
Disassembly and physical analysis established `0x008B8E` as the **Per-Frame State Gate & Mode Transition Dispatcher** (`0x008B8E..0x008BAC`, 30 bytes, 15 words, 7 instructions):
1. Reads byte `$00FF1864` into `%d0` via `move.b $00FF1864, %d0`.
2. When `$00FF1864 == 0`, branches via `beq.w 0x008BAA` directly to `rts`, popping return PC `0x008B2A` from stack (772 calls in canonical replay).
3. When `$00FF1864 != 0`, tests and clears bit 7 via `bclr #7, $00FF1864` (modifying Z flag only).
4. If bit 7 was clear (Z == 1), executes `bra.w 0x00530C` tail jump to mode handler `0x00530C`.
5. If bit 7 was set (Z == 0), branches via `bne.s 0x008BA6` to execute `bra.w 0x007CC6` tail jump to mode handler `0x007CC6`.

**Decision:**
1. Reverse-engineer complete `0x008B8E` contract:
   - Routine bounds: `0x008B8E..0x008BAC` (30 bytes, 15 words, 7 instructions, terminal exit via RTS `0x008BAA` and tail branches `0x00530C` / `0x007CC6`).
   - Proven caller: `0x008B26` (772 calls, return PC `0x008B2A`).
2. Implement modular C++ instruction stepper:
   - `src/tools/hybrid/m68k_008b8e_step_ops.hpp` (28 lines <= 500)
   - `src/tools/hybrid/m68k_008b8e_step_ops.cpp` (24 lines <= 500)
   - `src/tools/hybrid/m68k_008b8e_instruction_step.hpp` (33 lines <= 500)
   - `src/tools/hybrid/m68k_008b8e_instruction_step.cpp` (183 lines <= 500)
   - Unit tests in `tests/m68k_008b8e_instruction_step_test.cpp` (296 lines <= 500) verifying caller gating, corrupt ROM rejection, odd SP rejection, zero-exit path, bra 0x530c path, bra 0x7cc6 path, and interruption/reentrancy handling.
3. Dual canonical 3,600-frame replay verification:
   - Verified Run A (`0x008B8E` OFF) vs Run B (`0x008B8E` ON).
   - Achieved 100% native displacement: 772 native calls (`0x008B26`: 772), 0 ROM fallbacks, 2,316 stepped instructions (772 zero-exit completions).
   - Achieved bit-exact SHA-256 parity across all 8 canonical acceptance streams (`observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, `call-z80-ram.bin`, `external-events.jsonl`, `normalized-events.jsonl`) with `different_frame_count == 0` (`divergence=NO`).
   - Status: `CPP_REPLACEMENT_VERIFIED_PATH_SPECIFIC = YES`, `RUNTIME_REPLACEMENT_ACTIVE = YES`, `CPP_REPLACEMENT_VERIFIED_GLOBAL = YES`.

**Consequences:** State gate `0x008B8E` now executes 100% natively in C++, completely displacing 772 calls from ROM and establishing `0x008BAC` (772 calls from `0x008B22`) as the new #1 untranslated actual-ROM target.

# ADR-M15-54 — Execution-Driven Recompilation: 0x005670 Player Pre-Action Input, Timers & Action Dispatcher Integration
**Status:** Accepted
**Date:** 2026-10-17

**Context:** Following the displacement of `0x00557A`, fresh actual-ROM execution census ranked `0x005670` #1 with 772 executions from caller `0x005586` (return PC `0x00558A`, displacement `+0x00EA`).
Disassembly and physical analysis established `0x005670` as the **Player Pre-Action Input, Timers & Action Dispatcher** (`0x005670..0x0059B8`, 842 bytes, 421 words, 189 instructions in core sequence + auxiliary loop `0x005B3C..0x005B9E`):
1. Block 1 (`0x005670..0x0057D0`):
   - Stores `%sp` into `$FF1934`.
   - Evaluates input and action readiness flags at `$FF1985`, `$FF1987`, `$FF0DBA`, `$FF1971`, `$FF1982`.
   - Updates motion and attack interval timers at `114(a6)`, `74(a6)`, `142(a6)`.
   - Early `rts` exits available at `0x0056FA` and `0x0057CE`.
   - Calls child subroutine `jsr 0x00E682` at `0x0057D0` (772 calls). Subroutine `0x00E682` returns normally to `0x0057D6` on 753 calls, and pops return PC to grandparent `0x00558A` on 19 calls.
2. Block 2 (`0x0057D6..0x0059B8`):
   - Evaluates sub-actions at `$FF198B`, `108(a6)`, `132(a6)`, `131(a6)`, `56(a6)`.
   - Calls `bsr.w 0x008EF8` at `0x0058FA` and `0x005928` (2 child calls across canonical replay).
   - Reads action code `4(a6)` into `%d0`, calculates offset table address `lea 6(%pc, %d0.w), %a0`, resolves branch target `adda.w (%a0), %a0`, and executes tail dispatch `jmp (%a0)` at `0x0059B8` (753 completions).
3. Block 3 (`0x005B3C..0x005B9E`):
   - Auxiliary interaction branch with `movem.l d1, -(sp)` at `0x005B6C` and `movem.l (sp)+, d1` at `0x005B82`, both charging canonical 56-cycle bus refresh timing (112 cycles total).

**Decision:**
1. Reverse-engineer complete `0x005670` contract:
   - Routine bounds: `0x005670..0x0059B8` + `0x005B3C..0x005B9E`.
   - Proven caller: `0x005586` (772 calls, return PC `0x00558A`).
2. Implement modular C++ instruction stepper:
   - `src/tools/hybrid/m68k_005670_step_ops.hpp` (152 lines <= 500)
   - `src/tools/hybrid/m68k_005670_step_ops1.cpp` (360 lines <= 500)
   - `src/tools/hybrid/m68k_005670_step_ops2.cpp` (338 lines <= 500)
   - `src/tools/hybrid/m68k_005670_step_block1.hpp` (7 lines <= 500)
   - `src/tools/hybrid/m68k_005670_step_block1.cpp` (172 lines <= 500)
   - `src/tools/hybrid/m68k_005670_step_block2.hpp` (7 lines <= 500)
   - `src/tools/hybrid/m68k_005670_step_block2.cpp` (250 lines <= 500)
   - `src/tools/hybrid/m68k_005670_step_block3.hpp` (7 lines <= 500)
   - `src/tools/hybrid/m68k_005670_step_block3.cpp` (70 lines <= 500)
   - `src/tools/hybrid/m68k_005670_instruction_step.hpp` (31 lines <= 500)
   - `src/tools/hybrid/m68k_005670_instruction_step.cpp` (166 lines <= 500)
   - Unit tests in `tests/m68k_005670_instruction_step_test.cpp` (198 lines <= 500) verifying caller gating, corrupt ROM rejection, odd SP rejection, action dispatch path, and interruption/reentrancy protection.
3. Dual canonical 3,600-frame replay verification:
   - Verified Run A (`0x005670` OFF) vs Run B (`0x005670` ON).
   - Achieved 100% native displacement: 772 native calls (`0x005586`: 772), 0 ROM fallbacks, 47,655 stepped instructions: 772 child calls to `0x00E682`, 2 child calls to `0x008EF8`, 753 tail-dispatch completions, 19 grandparent return completions.
   - Achieved bit-exact SHA-256 parity across all 8 canonical acceptance streams (`observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, `call-z80-ram.bin`, `external-events.jsonl`, `normalized-events.jsonl`) with `different_frame_count == 0` (`divergence=NO`).
   - Status: `CPP_REPLACEMENT_VERIFIED_PATH_SPECIFIC = YES`, `RUNTIME_REPLACEMENT_ACTIVE = YES`, `CPP_REPLACEMENT_VERIFIED_GLOBAL = YES`.

**Consequences:** Player dispatcher `0x005670` now executes 100% natively in C++, completely displacing 772 calls from ROM and establishing `0x008B8E` (772 calls from `0x008B26`) as the new #1 untranslated actual-ROM target.

# ADR-M15-53 — Execution-Driven Recompilation: 0x00557A Player Per-Frame State, Footstep Audio & Shadow Coordinator Integration
**Status:** Accepted
**Date:** 2026-10-17

**Context:** Following the displacement of `0x008DD4`, fresh actual-ROM execution census ranked `0x00557A` #1 with 772 executions from caller `0x008B2E` (return PC `0x008B32`, displacement `-0x35B6` / `0xCA4A`).
Disassembly and physical analysis established `0x00557A` as the **Player Per-Frame State, Footstep Audio & Shadow Coordinator** (`0x00557A..0x00566E`, 123 words, 246 bytes, 52 instructions):
1. Clears `$FF1986` (`sf $00FF1986`).
2. Loads player entity slot pointer `A6 = $FF19E8` and invokes child subroutine `bsr.w 0x005670` (return PC `0x00558A`).
3. Reloads player entity slot `A6 = $FF19E8`, unpacks displacement bytes `72(a6)` and `73(a6)`, checks facing flip `23(a6)` bit 0, and writes velocity words to `66(a6)` and `68(a6)`.
4. Manages footstep audio timer `$FF1993` / motion word `138(a6)` to queue sound effects via `jsr 0x060004` (`d0 = 0x00A0038C`).
5. Manages secondary sound timer `$FF1994` / state `$FF1AA4` to queue sound effects via `jsr 0x060004` (`d0 = 0x00A004DD`).
6. Evaluates bit 7 of state byte `111(a6)`. If set, exits via `0x00565E` RTS without updating shadow coordinates (439 calls in canonical replay). If clear, checks player action state `4(a6)`. If action is 0, 2, or 4, updates shadow/footprint coordinate words at `$FF1972` from `8(a6)` and `12(a6)`, exiting via `0x00566E` RTS (333 calls in canonical replay).

**Decision:**
1. Reverse-engineer complete `0x00557A` contract:
   - Routine bounds: `0x00557A..0x00566E` (246 bytes, 123 words, 52 instructions, terminal exits via RTS `0x00565E` and `0x00566E`, child calls to `0x005670` and `0x060004`).
   - Proven caller: `0x008B2E` (772 calls, return PC `0x008B32`).
2. Implement modular C++ instruction stepper:
   - `src/tools/hybrid/m68k_00557a_step_ops.hpp` (73 lines <= 500)
   - `src/tools/hybrid/m68k_00557a_step_ops.cpp` (277 lines <= 500)
   - `src/tools/hybrid/m68k_00557a_step_block1.hpp` (13 lines <= 500)
   - `src/tools/hybrid/m68k_00557a_step_block1.cpp` (70 lines <= 500)
   - `src/tools/hybrid/m68k_00557a_step_block2.hpp` (13 lines <= 500)
   - `src/tools/hybrid/m68k_00557a_step_block2.cpp` (84 lines <= 500)
   - `src/tools/hybrid/m68k_00557a_instruction_step.hpp` (35 lines <= 500)
   - `src/tools/hybrid/m68k_00557a_instruction_step.cpp` (210 lines <= 500)
   - Unit tests in `tests/m68k_00557a_instruction_step_test.cpp` (370 lines <= 500) verifying caller gating, corrupt ROM rejection, shadow skip path, shadow update path, and reentrancy/interruption handling.
3. Dual canonical 3,600-frame replay verification:
   - Verified Run A (`0x00557A` OFF) vs Run B (`0x00557A` ON).
   - Achieved 100% native displacement: 772 native calls (`0x008B2E`: 772), 0 ROM fallbacks, 24,968 stepped instructions, 772 child calls to `0x005670`, 439 shadow-skipped calls, 333 shadow-updated calls.
   - Achieved bit-exact SHA-256 parity across all 8 canonical acceptance streams (`observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, `call-z80-ram.bin`, `external-events.jsonl`, `normalized-events.jsonl`) with `different_frame_count == 0` (`divergence=NO`).
   - Status: `CPP_REPLACEMENT_VERIFIED_PATH_SPECIFIC = YES`, `RUNTIME_REPLACEMENT_ACTIVE = YES`, `CPP_REPLACEMENT_VERIFIED_GLOBAL = YES`.

**Consequences:** Player coordinator `0x00557A` now executes 100% natively in C++, completely displacing 772 calls from ROM and establishing child subroutine `0x005670` (772 calls from `0x005586`) as the new #1 untranslated actual-ROM target.

# ADR-M15-52 — Execution-Driven Recompilation: 0x008DD4 HUD / Screen Spirit / Special Counter Step Handler Integration
**Status:** Accepted
**Date:** 2026-10-17

**Context:** Following the displacement of `0x00C998`, fresh actual-ROM execution census ranked `0x008DD4` #1 with 772 executions from caller `0x008B46` (return PC `0x008B4A`, displacement `+0x028C`).
Disassembly and physical analysis established `0x008DD4` as the **HUD / Screen Spirit / Special Counter Step Handler** (`0x008DD4..0x008E20`, 39 words, 78 bytes, 23 instructions):
1. Reads byte counter `$FF183E` into `%d0` via `move.b $00FF183E, %d0` and evaluates word flag `$FF1AA4` via `tst.w $00FF1AA4`.
2. When `$FF1AA4 == 0` (spirit/mode inactive), branches via `beq.s 0x008E08` to evaluate `%d0`:
   - If `%d0 == 0`, branches immediately via `beq.s 0x008E20` directly to `rts` without writing memory (772 calls in canonical replay).
   - If `%d0 != 0`, decrements `%d0` (or clamps to 2 if `%d0 >= 4`), writes updated value to `$FF183E`, and returns via `rts`.
3. When `$FF1AA4 != 0` (spirit/mode active), increments `%d0` (up to 3) or clamps to 3, evaluates frame counter `$FF199E` and bit 0 of `$FF199F` to optionally advance `%d0` to 4, writes updated value to `$FF183E`, and returns via `rts`.

**Decision:**
1. Reverse-engineer complete `0x008DD4` contract:
   - Routine bounds: `0x008DD4..0x008E20` (78 bytes, 39 words, 23 instructions, terminal exits via RTS `0x008E20`).
   - Proven caller: `0x008B46` (772 calls, return PC `0x008B4A`).
2. Implement modular C++ instruction stepper:
   - `src/tools/hybrid/m68k_008dd4_step_ops.hpp` (15 lines <= 500)
   - `src/tools/hybrid/m68k_008dd4_step_ops.cpp` (39 lines <= 500)
   - `src/tools/hybrid/m68k_008dd4_instruction_step.hpp` (32 lines <= 500)
   - `src/tools/hybrid/m68k_008dd4_instruction_step.cpp` (325 lines <= 500)
   - Unit tests in `tests/m68k_008dd4_instruction_step_test.cpp` (331 lines <= 500) verifying caller gating, corrupt ROM rejection, active increment path, inactive zero-exit path, and interruption/reentrancy protection.
3. Dual canonical 3,600-frame replay verification:
   - Verified Run A (`0x008DD4` OFF) vs Run B (`0x008DD4` ON).
   - Achieved 100% native displacement: 772 native calls (`0x008B46`: 772), 0 ROM fallbacks, 4,632 stepped instructions, 772 zero-exit calls.
   - Achieved bit-exact SHA-256 parity across all 8 canonical acceptance streams (`observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, `call-z80-ram.bin`, `external-events.jsonl`, `normalized-events.jsonl`) with `different_frame_count == 0`.
   - Status: `CPP_REPLACEMENT_VERIFIED_PATH_SPECIFIC = YES`, `RUNTIME_REPLACEMENT_ACTIVE = YES`, `CPP_REPLACEMENT_VERIFIED_GLOBAL = YES`.

**Consequences:** The HUD / screen spirit / special counter step handler `0x008DD4` now executes 100% natively in C++, completely displacing 772 calls from ROM and establishing `0x00557A` (772 calls, caller: `0x008B2E`: 772) as the new sole #1 remaining ROM target.

# ADR-M15-51 — Execution-Driven Recompilation: 0x00C998 Per-Frame Map Script Vector 1 Dispatcher Integration
**Status:** Accepted
**Date:** 2026-10-17

**Context:** Following the displacement of `0x00C980`, fresh actual-ROM execution census ranked `0x00C998` #1 with 772 executions from caller `0x008B52` (return PC `0x008B56`, displacement `+0x3E44`).
Disassembly and physical analysis established `0x00C998` as the **Per-Frame Map Script Vector 1 Dispatcher** (`0x00C998..0x00C9B0`, 13 words, 26 bytes, 7 instructions):
1. `btst #1, $00FF17B8`: Evaluates script inhibit flag bit 1 in RAM `$FF17B8`.
2. `bne.w 0x00C9B0`: If bit 1 is set, branches directly to `rts` at `0x00C9B0`, returning to caller without script execution (30 calls in 3,600 frames).
3. `movea.l $00FF16EC, %a1`: If script is not inhibited, loads map script vector table pointer from RAM `$FF16EC` into `%a1`.
4. `addq.l #2, %a1`: Advances `%a1` by 2 bytes to select Vector 1 displacement entry in table.
5. `adda.w (%a1), %a1`: Reads 16-bit signed displacement for Vector 1 from `(%a1)` and adds to `%a1`.
6. `jmp (%a1)`: Indirect tail jump to active map's Vector 1 handler in ROM (742 calls in 3,600 frames). Target handler executes natively in Musashi; target's `rts` returns directly to outer caller `0x008B56`.

**Decision:**
1. Reverse-engineer complete `0x00C998` contract:
   - Routine bounds: `0x00C998..0x00C9B0` (26 bytes, 13 words, 7 instructions, terminal exits via RTS `0x00C9B0` and indirect JMP `0x00C9AE`).
   - Proven caller: `0x008B52` (772 calls, return PC `0x008B56`).
2. Implement modular C++ instruction stepper:
   - `src/tools/hybrid/m68k_00c998_step_ops.hpp` (13 lines <= 500)
   - `src/tools/hybrid/m68k_00c998_step_ops.cpp` (21 lines <= 500)
   - `src/tools/hybrid/m68k_00c998_instruction_step.hpp` (34 lines <= 500)
   - `src/tools/hybrid/m68k_00c998_instruction_step.cpp` (224 lines <= 500)
   - Unit tests in `tests/m68k_00c998_instruction_step_test.cpp` (297 lines <= 500) verifying caller gating, corrupt ROM rejection, inhibited path, vector 1 dispatch path, and interruption/reentrancy protection.
3. Dual canonical 3,600-frame replay verification:
   - Verified Run A (`0x00C998` OFF) vs Run B (`0x00C998` ON).
   - Achieved 100% native displacement: 772 native calls (`0x008B52`: 772), 0 ROM fallbacks, 4,542 stepped instructions, 30 inhibit exits, 742 indirect vector jumps.
   - Achieved bit-exact SHA-256 parity across all 8 canonical acceptance streams (`observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, `call-z80-ram.bin`, `external-events.jsonl`, `normalized-events.jsonl`) with `different_frame_count == 0`.
   - Status: `CPP_REPLACEMENT_VERIFIED_PATH_SPECIFIC = YES`, `RUNTIME_REPLACEMENT_ACTIVE = YES`, `CPP_REPLACEMENT_VERIFIED_GLOBAL = YES`.

**Consequences:** The per-frame map script Vector 1 dispatcher `0x00C998` now executes 100% natively in C++, completely displacing 772 calls from ROM and establishing `0x008DD4` (772 calls, caller: `0x008B46`: 772) as the new sole #1 remaining ROM target.

# ADR-M15-50 — Execution-Driven Recompilation: 0x00C980 Per-Frame Map Script Vector 0 Dispatcher Integration
**Status:** Accepted
**Date:** 2026-10-17

**Context:** Following the displacement of `0x00B34C`, fresh actual-ROM execution census ranked `0x00C980` #1 with 773 executions across callers `0x008B60` (770 calls, return PC `0x008B64`, displacement `+0x3E1E`) and `0x00CD20` (3 calls, return PC `0x00CD24`, displacement `-0x03A2`). A third static caller exists in ROM at `0x006A3E` (return PC `0x006A42`, displacement `+0x5F40`).
Disassembly and physical analysis established `0x00C980` as the **Per-Frame Map Script Vector 0 Dispatcher** (`0x00C980..0x00C996`, 12 words, 24 bytes, 6 instructions):
1. `btst #1, $00FF17B8`: Evaluates script inhibit flag bit 1 in RAM `$FF17B8`.
2. `bne.w 0x00C996`: If bit 1 is set, branches directly to `rts` at `0x00C996`, returning to caller without script execution (30 calls in 3,600 frames).
3. `movea.l $00FF16EC, %a1`: If script is not inhibited, loads map script vector table pointer from RAM `$FF16EC` into `%a1`.
4. `adda.w (%a1), %a1`: Reads 16-bit signed displacement for Vector 0 from `(%a1)` and adds to `%a1`.
5. `jmp (%a1)`: Indirect tail jump to active map's Vector 0 handler in ROM (743 calls in 3,600 frames). Target handler executes natively in Musashi; target's `rts` returns directly to outer caller `0x008B64` or `0x00CD24`.

**Decision:**
1. Reverse-engineer complete `0x00C980` contract:
   - Routine bounds: `0x00C980..0x00C996` (24 bytes, 12 words, 6 instructions, terminal exits via RTS `0x00C996` and indirect JMP `0x00C994`).
   - Proven callers: `0x008B60` (770 calls, return PC `0x008B64`), `0x00CD20` (3 calls, return PC `0x00CD24`), `0x006A3E` (return PC `0x006A42`).
2. Implement modular C++ instruction stepper:
   - `src/tools/hybrid/m68k_00c980_step_ops.hpp` (23 lines <= 500)
   - `src/tools/hybrid/m68k_00c980_step_ops.cpp` (73 lines <= 500)
   - `src/tools/hybrid/m68k_00c980_instruction_step.hpp` (35 lines <= 500)
   - `src/tools/hybrid/m68k_00c980_instruction_step.cpp` (220 lines <= 500)
   - Unit tests in `tests/m68k_00c980_instruction_step_test.cpp` (294 lines <= 500) verifying caller gating, corrupt ROM rejection, inhibited path, vector 0 dispatch path, and interruption/reentrancy protection.
3. Dual canonical 3,600-frame replay verification:
   - Verified Run A (`0x00C980` OFF) vs Run B (`0x00C980` ON).
   - Achieved 100% native displacement: 773 native calls (`0x008B60`: 770, `0x00CD20`: 3), 0 ROM fallbacks, 3,805 stepped instructions, 30 inhibit exits, 743 indirect vector jumps.
   - Achieved bit-exact SHA-256 parity across all 8 canonical acceptance streams (`observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, `call-z80-ram.bin`, `external-events.jsonl`, `normalized-events.jsonl`) with `different_frame_count == 0`.
   - Status: `CPP_REPLACEMENT_VERIFIED_PATH_SPECIFIC = YES`, `RUNTIME_REPLACEMENT_ACTIVE = YES`, `CPP_REPLACEMENT_VERIFIED_GLOBAL = YES`.

**Consequences:** The per-frame map script Vector 0 dispatcher `0x00C980` now executes 100% natively in C++, completely displacing 773 calls from ROM and establishing `0x00C998` (772 calls, caller: `0x008B52`: 772) as the new sole #1 remaining ROM target.

# ADR-M15-49 — Execution-Driven Recompilation: 0x00B34C Entity Display Mode & Visual Effects Dispatcher Integration
**Status:** Accepted
**Date:** 2026-10-17

**Context:** Following the displacement of `0x00A342`, fresh actual-ROM execution census ranked `0x00B34C` #1 with 773 executions from caller `0x00AD42` (return PC `0x00AD46`, displacement `0x0608`). A second static caller in ROM exists at `0x00B012` (return PC `0x00B016`, displacement `0x0338`).
Disassembly and architectural analysis established `0x00B34C` as the **Entity Display Mode & Visual Effects Dispatcher** (`0x00B34C..0x00B3B0`, 51 words, 102 bytes, 25 instructions):
1. `tst.b $00FF1927`: Checks global pause/freeze flag. If non-zero, exits immediately via `bne.w 0x00B382` RTS.
2. `move.b 57(%fp), %d5`: Loads entity display mode byte from `$39(A6)` into `D5`.
3. `btst #0, %d5`: Tests primary compositor bit. If set, invokes child routine `0x00B3D2` via `bsr.w 0x00B3D2` at `0x00B384` (481 calls in 3,600 frames). Upon return to `0x00B388`, branches to `0x00B362`.
4. `btst #3, %d5`: Tests secondary compositor bit. If set, invokes child routine `0x00B45C` via `bsr.w 0x00B45C` at `0x00B394`. Upon return to `0x00B398`, branches to `0x00B36A`.
5. `btst #4, %d5`: Tests alternative compositor bit. If set, invokes child routine `0x00B4FE` via `bsr.w 0x00B4FE` at `0x00B39C`. Upon return to `0x00B3A0`, branches to `0x00B372`.
6. `btst #1, %d5`: Tests priority overlay bit. If set, invokes child routine `0x00B610` via `bsr.w 0x00B610` at `0x00B3A4`. Upon return to `0x00B3A8`, clears bit 2 in memory via `bclr #2, 57(%fp)`, and branches directly to `0x00B382` RTS.
7. `btst #2, %d5`: Tests background compositor bit. If set, invokes child routine `0x00B56E` via `bsr.w 0x00B56E` at `0x00B38C` (292 calls in 3,600 frames). Upon return to `0x00B390`, branches to `0x00B382` RTS.
8. `0x00B382` RTS: Exits back to caller `0x00AD46` (773 completed calls).

**Decision:**
1. Reverse-engineer complete `0x00B34C` contract:
   - Routine bounds: `0x00B34C..0x00B3B0` (102 bytes, 51 words, 25 instructions, single RTS exit `0x00B382`).
   - Proven callers: `0x00AD42` (773 calls, return PC `0x00AD46`), `0x00B012` (0 calls in canonical replay, return PC `0x00B016`).
   - Child delegation protocol: `0x00B3D2` (481 calls, return PC `0x00B388`), `0x00B56E` (292 calls, return PC `0x00B390`), `0x00B45C` (return PC `0x00B398`), `0x00B4FE` (return PC `0x00B3A0`), `0x00B610` (return PC `0x00B3A8`).
2. Implement modular C++ instruction stepper:
   - `src/tools/hybrid/m68k_00b34c_step_ops.hpp` (17 lines <= 500)
   - `src/tools/hybrid/m68k_00b34c_step_ops.cpp` (56 lines <= 500)
   - `src/tools/hybrid/m68k_00b34c_step_seq.hpp` (12 lines <= 500)
   - `src/tools/hybrid/m68k_00b34c_step_seq.cpp` (63 lines <= 500)
   - `src/tools/hybrid/m68k_00b34c_instruction_step.hpp` (35 lines <= 500)
   - `src/tools/hybrid/m68k_00b34c_instruction_step.cpp` (206 lines <= 500)
   - Unit tests in `tests/m68k_00b34c_instruction_step_test.cpp` (292 lines <= 500) verifying caller gating, global freeze early exit, primary child dispatch, background child dispatch, overlay child dispatch with memory bit clear, and fail-closed rejections.
3. Dual canonical 3,600-frame replay verification:
   - Verified Run A (`0x00B34C` OFF) vs Run B (`0x00B34C` ON).
   - Achieved 100% native displacement: 773 native calls (`0x00AD42`: 773), 0 ROM fallbacks, 12,368 stepped instructions, 481 child calls to `0x00B3D2`, 292 child calls to `0x00B56E`.
   - Achieved bit-exact SHA-256 parity across all 8 canonical acceptance streams (`observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, `call-z80-ram.bin`, `external-events.jsonl`, `normalized-events.jsonl`) with `different_frame_count == 0`.
   - Status: `CPP_REPLACEMENT_VERIFIED_PATH_SPECIFIC = YES`, `RUNTIME_REPLACEMENT_ACTIVE = YES`, `CPP_REPLACEMENT_VERIFIED_GLOBAL = YES`.

**Consequences:** The entity display mode & visual effects dispatcher `0x00B34C` now executes 100% natively in C++, completely displacing 773 calls from ROM and establishing `0x00C980` (773 calls, callers: `0x008B60`: 770, `0x00CD20`: 3) as the new sole #1 remaining ROM target.

# ADR-M15-41 — Execution-Driven Recompilation: 0x01B102 Entity Type 0x38 Damage & Hit Processing Handler Integration
**Status:** Accepted
**Date:** 2026-10-17

**Context:** Following the displacement of `0x01AC90`, fresh actual-ROM execution census ranked `0x01B102` #1 with 890 executions across four observed callers: `0x01B7F8` (102 calls in State 1), `0x01BBFE` (332 calls in State 2), `0x01BF8A` (344 calls in State 3), and `0x01C4E2` (112 calls in State 5). All four callers originate from Entity Type `0x0038` state handlers dispatched by `0x01AC90`.
Physical layout and disassembly analysis established `0x01B102` as the dedicated **Entity Type 0x38 Damage & Hit Processing Handler** (`0x01B102..0x01B1BE`, 94 words, 188 bytes, 45 instructions):
1. `move.w $86(a6), d0` loads the incoming hit packet word from `$86(a6)`.
2. `beq.w 0x01B1BC`: If no hit was received (`$86(a6) == 0`), branches directly to `RTS` at `0x01B1BC` (879 calls in 3,600 frames).
3. If hit was received (`$86(a6) != 0`, 11 calls in 3,600 frames):
   - Clears incoming hit word (`clr.w $86(a6)`).
   - Clears flags: `bclr #2, $39(a6)` and `bclr #2, $38(a6)`.
   - Reads attacker orientation `$84(a6)`, inverts horizontal orientation via `bchg #1, d1`, writes facing `$16(a6)`, indexes 4-entry angle table at `0x01074A` (`{0x0040, 0x0000, 0x00C0, 0x0080}`), and sets knockback angle `$9E(a6)`.
   - Checks heavy hit / SFX bit 13 of D0 (`btst #13, d0`). If set, sets bit 3 of `$39(a6)` and invokes SFX trigger `0x00E7E8` via `jsr 0x00E7E8`.
   - Calculates damage: isolates low 12 bits (`andi.w #$0fff, d0`), subtracts entity defense `$92(a6)`, clamping to minimum 1 point (`moveq #1, d0`).
   - Applies damage to HP `$8A(a6)` (`sub.w d0, d1`). Clamps negative HP to 0.
   - If HP reaches 0 (6 fatal hits in 3,600 frames), invokes death hook `jsr 0x00E4D0` and sets death/heavy recoil flags (`ori.w #$c000, d7`).
   - Checks recoil flags `d7 & 0xC000`:
     - If non-zero (10 recoil calls): sets recoil timer `$10(a6) = 24`, clears `$83(a6)`, sets bit 3 of `$38(a6)`, sets sub-action state `$4(a6) = 2`, calls animation setup `0x008EF8` with `D0 = 10`, sets `$2(a6) = $4a(a6)` and `$4a(a6) = 10`, then exits via `RTS`.
     - If zero (1 idle/light recoil call): sets `$4(a6) = 0`, loads `D0 = 6`, and executes tail jump `jmp 0x008EF8`.

**Decision:**
1. Reverse-engineer complete `0x01B102` contract:
   - Routine bounds: `0x01B102..0x01B1BE` (188 bytes, 94 words, 45 instructions, single RTS exit `0x01B1BC` plus tail call `0x01B18A`).
   - Proven callers: `0x01B7F8` (102 calls, return PC `0x01B7FC`), `0x01BBFE` (332 calls, return PC `0x01BC02`), `0x01BF8A` (344 calls, return PC `0x01BF8E`), `0x01C4E2` (112 calls, return PC `0x01C4E6`).
   - Register ABI: takes entity instance pointer in `A6` (`$0(a6) == 0x0038`).
   - Condition codes and registers preserved identically to Musashi 68000.
2. Implement modular C++ instruction stepper:
   - `src/tools/hybrid/m68k_01b102_instruction_step.hpp` (36 lines <= 500)
   - `src/tools/hybrid/m68k_01b102_step_hit.hpp` (13 lines <= 500)
   - `src/tools/hybrid/m68k_01b102_step_hit.cpp` (402 lines <= 500)
   - `src/tools/hybrid/m68k_01b102_instruction_step.cpp` (228 lines <= 500)
   - Unit tests in `tests/m68k_01b102_instruction_step_test.cpp` (353 lines <= 500) verifying all 4 callers, no-hit path, heavy recoil, voice SFX, fatal hits, and fail-closed rejections.
3. Dual canonical 3,600-frame replay verification:
   - Verified Run A (`0x01B102` OFF) vs Run B (`0x01B102` ON).
   - Achieved 100% native displacement: 890 native calls (`0x01B7F8`: 102, `0x01BBFE`: 332, `0x01BF8A`: 344, `0x01C4E2`: 112), 0 ROM fallbacks, 2,121 stepped instructions (879 no-hit exits, 11 hit paths: 6 fatal, 10 recoil, 1 idle).
   - Achieved bit-exact SHA-256 parity across all 6 canonical acceptance streams (`observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, `call-z80-ram.bin`) plus `external-events.jsonl` and `normalized-events.jsonl` with `different_frame_count == 0`.
   - Status: `CPP_REPLACEMENT_VERIFIED_PATH_SPECIFIC = YES`, `RUNTIME_REPLACEMENT_ACTIVE = YES`, `CPP_REPLACEMENT_VERIFIED_GLOBAL = NO`.

**Consequences:** The entity damage and hit processing routine `0x01B102` now executes 100% natively in C++, completely displacing 890 calls from ROM and establishing `0x00350E` (866 calls, caller: `0x0033FE`: 866) as the fresh #1 remaining ROM target.

# ADR-M15-40 — Execution-Driven Recompilation: 0x01AC90 Entity Type 0x38 Lifecycle State Machine & Jump Table Dispatcher Integration
**Status:** Accepted
**Date:** 2026-10-17

**Context:** Following the displacement of `0x06282E`, fresh actual-ROM execution census ranked `0x01AC90` #1 with 914 executions across three observed callers: `0x00DF1A` (908 calls in the main per-frame entity loop), `0x00FD7A` (2 calls in entity spawn/initialization), and `0x00FDFE` (4 calls in entity transformation/spawn).
Physical layout and disassembly analysis disproved the speculative label "Core Gameplay Entity Handler" and established `0x01AC90` as the dedicated **Entity Type 0x38 Lifecycle State Machine & Jump Table Dispatcher**.
The routine forms an 18-byte jump table dispatch block (`0x01AC90..0x01ACA2`, 9 words, 6 instructions, 1 basic block) followed by a 25-entry PC-relative jump table (`0x01ACA2..0x01ACD4`, 25 words, 50 bytes) for entity states 0..24:
1. `move.b $66(a6), d0` (84 master cycles) loads the current lifecycle state byte from `$66(a6)`.
2. `andi.w #$ff, d0` (56 master cycles) zeroes the high byte of `D0.W`.
3. `add.w d0, d0` (28 master cycles) doubles `D0.W` into a 2-byte word table index.
4. `lea.l $1aca2(pc, d0.w), a0` (84 master cycles) computes the address of the jump table entry.
5. `adda.w (a0), a0` (84 master cycles) reads the signed 16-bit displacement and adds it to `A0`.
6. `jmp (a0)` (56 master cycles) jumps directly to the state handler in ROM with the caller's return PC preserved on stack.
Total cycle count: deterministic 392 master cycles (56 CPU clocks).

All three callers (`0x00DF1A`, `0x00FD7A`, `0x00FDFE`) invoke `0x01AC90` via indirect subroutine call `jsr (a0)` using the entity dispatch table at `0x00DF54 - 0x2C = 0x00DF28`. At index 28 (`$0(a6) = 0x0038`, offset `0x70` from `0x00DF28` or `0x44` from `0x00DF54`), the table points to `0x01AC90`.
Observed runtime states in canonical 3,600-frame replay:
- State 1 (`0x01B7F2`): 105 calls (calls freeze gatekeeper `0x00E682` and hit responder `0x01B102`)
- State 2 (`0x01BBF8`): 337 calls (calls freeze gatekeeper `0x00E682` and hit responder `0x01B102`)
- State 3 (`0x01BF84`): 350 calls (calls freeze gatekeeper `0x00E682` and hit responder `0x01B102`)
- State 5 (`0x01C4DC`): 116 calls (calls freeze gatekeeper `0x00E682` and hit responder `0x01B102`)
- State 9 (`0x01AEA4`): 1 call
- State 10 (`0x01AF60`): 1 call
- State 18 (`0x01AE02`): 1 call
- State 20 (`0x01AFC8`): 1 call
- State 22 (`0x01AFF6`): 2 calls
Total calls: 914 calls, exactly matching observed ROM executions.

**Decision:**
1. Reverse-engineer complete `0x01AC90` contract:
   - Routine bounds: `0x01AC90..0x01ACA2` (18 bytes, 9 words, 6 instructions, 1 basic block, jump table `0x01ACA2..0x01ACD4`).
   - Proven callers: `0x00DF1A` (908 calls, return PC `0x00DF1C`), `0x00FD7A` (2 calls, return PC `0x00FD7C`), `0x00FDFE` (4 calls, return PC `0x00FE00`).
   - Register ABI: takes entity instance pointer in `A6` (`$0(a6) == 0x0038`, `$66(a6) <= 24`). Clobbers `D0.W` (doubled state index) and `A0` (resolved target address). Preserves all other registers.
   - Condition codes: N, Z, V, C, X updated according to `add.w d0, d0`.
   - Timing: deterministic 392 master cycles ($56 \times 7$).
2. Implement modular C++ instruction stepper:
   - `src/tools/hybrid/m68k_01ac90_instruction_step.hpp/.cpp`
   - Unit tests in `tests/m68k_01ac90_instruction_step_test.cpp` verifying all 3 callers, state resolution (0, 1, 2, 3, 5, 7, 24), register masking/doubling, condition codes, and fail-closed rejections for bad callers, invalid SP, wrong entity type, unsupported states (> 24), and ROM mismatch.
3. Dual canonical 3,600-frame replay verification:
   - Verified Run A (`0x01AC90` OFF) vs Run B (`0x01AC90` ON).
   - Achieved 100% native displacement: 914 native calls (`0x00DF1A`: 908, `0x00FD7A`: 2, `0x00FDFE`: 4), 0 ROM fallbacks, 5,484 stepped instructions ($914 \times 6$).
   - Achieved bit-exact SHA-256 parity across all 6 canonical acceptance streams (`observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, `call-z80-ram.bin`) plus `external-events.jsonl` and `normalized-events.jsonl` with `different_frame_count == 0`.
   - Status: `CPP_REPLACEMENT_VERIFIED_PATH_SPECIFIC = YES`, `RUNTIME_REPLACEMENT_ACTIVE = YES`, `CPP_REPLACEMENT_VERIFIED_GLOBAL = NO`.

**Consequences:** The entity lifecycle state dispatcher `0x01AC90` now executes 100% natively in C++, completely displacing 914 calls from ROM and establishing `0x01B102` (890 calls, callers: `0x01B7F8`: 102, `0x01BBFE`: 332, `0x01BF8A`: 344, `0x01C4E2`: 112) as the fresh #1 remaining ROM target.

# ADR-M15-39 — Execution-Driven Recompilation: 0x06282E Sound Channel Attenuation Calculator & Hardware Committer Sub-Tail Entry Integration
**Status:** Accepted
**Date:** 2026-10-16

**Context:** Following the displacement of `0x06281A`, fresh actual-ROM execution census ranked `0x06282E` #1 with 922 executions across a single observed caller (`0x0624C8`: 922 calls in sound driver note-on / volume calculation handling).
Physical layout analysis confirmed that `0x06282E` lies physically inside the already-translated ADSR sound envelope generator (`0x062788..0x06286E`), immediately following the release phase decrement sequence (`0x06281A..0x06282A`), and constitutes the common attenuation calculation and PSG hardware commit sub-tail (`0x06282E..0x06286C`, 31 words, 62 bytes, 16 instructions).
Exhaustive ROM scanning identified exactly seven references to `0x06282E` in the entire ROM:
1. Internal fall-through from `0x06281A`: at `0x06282A` (`move.b d0, $18(a6)` falls through to `0x06282E`).
2. Internal branches from `0x062788`: at `0x0627AC` (`bra.w 0x06282E`), `0x0627B6` (`bra.w 0x06282E`), `0x0627E2` (`bra.w 0x06282E`), `0x0627EC` (`bra.w 0x06282E`), and `0x06280E` (`bra.w 0x06282E`) during envelope attack, decay, sustain, and holding stages.
3. External call from `0x0623AC`: at `0x0624C8` (`bsr.w 0x06282E`, opcode `0x6100 0364` during sound channel note-on / note-trigger attenuation updates).

The routine computes and commits the sound channel attenuation:
1. Clears `D0.W` (`moveq #0, d0`) and reads current envelope level `$18(a6)` (`move.b $18(a6), d0`).
2. Clears `D2.W` (`moveq #0, d2`), reads master channel volume `$12(a6)` (`move.b $12(a6), d2`), and increments by 1 (`addq.b #1, d2`, giving scaling factor 1..16).
3. Multiplies envelope by master volume factor (`mulu.w d2, d0`) and shifts right 8 bits (`lsr.w #8, d0`) to extract the 4-bit attenuation level.
4. Checks tremolo modulation flags: `$00(a6)` bit 2 and `$70(a6)` bit 5. If modulation is active, adds delta `$6F(a6)` and shifts right by 1.
5. Inverts 4-bit attenuation level (`eori.b #15, d0`, where 0 = loud, 15 = mute) and stores to hardware mirror `$11(a6)`.
6. Dispatches to PSG hardware attenuation committer `0x06273A` via `bsr.w 0x06273A`.
7. Caches current envelope `$18(a6)` into shadow field `$13(a6)` (`move.b $18(a6), $13(a6)`) and returns via `rts` (`0x06286C`).

Double-ownership architectural hazard resolved:
- In the initial codebase, `0x0623AC` at `0x0624C8` was invoking `step_bsr_w_internal(0x06282EU)` and duplicating `0x06282E..0x06286C` instructions in `m68k_623ac_subs.cpp`.
- To avoid conflicting instruction ownership and duplicated maintenance:
  * Reused the shared release phase & attenuation tail (`0x06281A..0x062866`) in `src/tools/hybrid/m68k_6281a_step_tail.hpp/.cpp`, which already implemented `0x06282E..0x062866` as part of `dispatch_6281a_tail`.
  * Refactored `0x0623AC` (`src/tools/hybrid/m68k_623ac_block1.cpp:78`) to call `step_bsr_w(api, pc, 0x06282EU, 0x0624CCU, resume_pc, child_call_active, saved_child_sp)`, yielding control as an external child subroutine.
  * Cleaned up obsolete duplicate cases `0x06281A..0x06286C` from `src/tools/hybrid/m68k_623ac_subs.cpp`.
  * Implemented dedicated caller-gated instruction stepper `step_m68k_6282e` in `src/tools/hybrid/m68k_6282e_instruction_step.hpp/.cpp` delegating directly to `step_6281a::dispatch_6281a_tail`.

**Decision:**
1. Reverse-engineer complete `0x06282E` contract:
   - Routine bounds: `0x06282E..0x06286C` (62 bytes, 31 words, 16 instructions, single exit `0x06286C` RTS).
   - Proven external caller: `0x0624C8` (922 calls, return PC `0x0624CC`, displacement `0x0364`).
   - Register ABI: takes sound channel pointer in `A6` (work RAM `$00FF001A..`, size >= `$8A`), sound driver state in `A5` (`$00FF001A`), PSG data port in `A3` (`$00C00011`). Clobbers `D0.W`, `D2.W`.
   - Child hardware delegation: at `0x062862` (`bsr.w 0x06273A`), calls child helper `0x06273A` to commit attenuation byte to PSG data port `0x00C00011`.
2. Implement modular C++ replacement:
   - `src/tools/hybrid/m68k_6282e_instruction_step.hpp/.cpp` (standalone instruction stepper delegating to shared tail).
   - Unit tests in `tests/m68k_6282e_instruction_step_test.cpp` verifying caller gating, master volume multiplication, tremolo delta branch, attenuation inversion, child call to `0x06273A`, envelope caching, and RTS.
3. Dual canonical 3,600-frame replay verification:
   - Verified Run A (`0x06282E` OFF) vs Run B (`0x06282E` ON).
   - Achieved 100% native displacement: 922 native calls (`0x0624C8`: 922), 0 ROM fallbacks, 11,986 stepped instructions, 922 child calls to `0x06273A`.
   - Achieved bit-exact SHA-256 parity across all 6 canonical acceptance streams (`observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, `call-z80-ram.bin`) plus `external-events.jsonl` and `normalized-events.jsonl` with `different_frame_count == 0`.
   - Status: `CPP_REPLACEMENT_VERIFIED_PATH_SPECIFIC = YES`, `RUNTIME_REPLACEMENT_ACTIVE = YES`, `CPP_REPLACEMENT_VERIFIED_GLOBAL = NO`.

**Consequences:** The sound channel attenuation calculator & hardware committer `0x06282E` now executes 100% natively in C++, completely displacing 922 calls from ROM and establishing `0x01AC90` (914 calls, callers: `0x00DF1A`: 908, `0x00FD7A`: 2, `0x00FDFE`: 4) as the new sole #1 remaining ROM target.

# ADR-M15-38 — Execution-Driven Recompilation: 0x06281A Sound Channel Note-Off / Release Phase Attenuation Committer & Mid-Tail Entry Integration
**Status:** Accepted
**Date:** 2026-10-16

**Context:** Following the displacement of `0x017D12`, fresh actual-ROM execution census ranked `0x06281A` #1 with 985 executions across a single observed caller (`0x06243C`: 985 calls in sound driver note-off handling).
Physical layout analysis confirmed that `0x06281A` lies physically inside the already-translated ADSR sound envelope generator (`0x062788..0x06286E`), forming a mid-tail secondary entry point (`0x06281A..0x06286C`, 42 words, 84 bytes, 21 instructions).
Exhaustive ROM scanning identified exactly two references to `0x06281A` in the entire ROM:
1. Internal fall-through / conditional branch from `0x062788`: at `0x0627F4` (`beq.w 0x06281A` when decay phase is inactive / ADSR bit 2 is 0).
2. External call from `0x0623AC`: at `0x06243C` (`bsr.w 0x06281A` during sound channel note-off handling after testing `$01(a6)` bit 4 and clearing flags `$70(a6)`).

The routine computes and commits the sound channel release phase attenuation:
1. Decrements current envelope `$18(a6)` by release rate `$89(a6)` (`move.b $18(a6), d0; move.b $89(a6), d1; sub.b d1, d0`).
2. Clamps underflow borrow (`bcc.w 0x06282A; moveq #0, d0; 0x06282A: move.b d0, $18(a6)`).
3. Scales by channel master volume `$12(a6) + 1` (1..16 factor) via `mulu.w d2, d0; lsr.w #8, d0` to extract the 4-bit attenuation level.
4. Checks tremolo modulation flags: `$00(a6)` bit 2 and `$70(a6)` bit 5. If modulation is active, adds delta `$6F(a6)` and shifts right by 1.
5. Inverts 4-bit attenuation level (`eori.b #15, d0`, where 0 = loud, 15 = mute) and stores to hardware mirror `$11(a6)`.
6. Dispatches to PSG hardware attenuation committer `0x06273A` via `bsr.w 0x06273A`.
7. Caches current envelope `$18(a6)` into shadow field `$13(a6)` and returns via `rts` (`0x06286C`).

Double-ownership architectural hazard resolved:
- In the initial codebase, `0x0623AC` at `0x06243C` was invoking `step_bsr_w_internal(0x06281AU)` and duplicating `0x06281A..0x06286C` instructions in `m68k_623ac_subs.cpp`.
- To avoid conflicting instruction ownership and duplicated maintenance:
  * Extracted the shared release phase & attenuation tail (`0x06281A..0x062866`) into `src/tools/hybrid/m68k_6281a_step_tail.hpp/.cpp`.
  * Refactored `0x062788` (`src/tools/hybrid/m68k_62788_instruction_step.cpp`) to call `step_6281a::dispatch_6281a_tail`.
  * Refactored `0x0623AC` (`src/tools/hybrid/m68k_623ac_block1.cpp:45`) to call `step_bsr_w(api, pc, 0x06281AU, 0x062440U, resume_pc, child_call_active, saved_child_sp)`, yielding control as an external child subroutine.
  * Implemented dedicated caller-gated instruction stepper `step_m68k_6281a` in `src/tools/hybrid/m68k_6281a_instruction_step.hpp/.cpp`.

**Decision:**
1. Reverse-engineer complete `0x06281A` contract:
   - Routine bounds: `0x06281A..0x06286C` (84 bytes, 42 words, 21 instructions, single exit `0x06286C` RTS).
   - Proven external caller: `0x06243C` (985 calls, return PC `0x062440`, displacement `0x03DC`).
   - Register ABI: takes sound channel pointer in `A6` (work RAM `$00FF001A..`, size >= `$8A`), sound driver state in `A5` (`$00FF001A`), PSG data port in `A3` (`$00C00011`). Clobbers `D0.W`, `D1.B`, `D2.W`.
   - Child hardware delegation: at `0x062862` (`bsr.w 0x06273A`), calls child helper `0x06273A` to commit attenuation byte to PSG data port `0x00C00011`.
2. Implement modular C++ replacement:
   - `src/tools/hybrid/m68k_6281a_step_tail.hpp/.cpp` (shared tail dispatch).
   - `src/tools/hybrid/m68k_6281a_instruction_step.hpp/.cpp` (standalone instruction stepper).
   - Unit tests in `tests/m68k_6281a_instruction_step_test.cpp` verifying caller gating, normal release decrement, underflow clamp to 0, master volume multiplication, tremolo delta branch, attenuation inversion, child call to `0x06273A`, envelope caching, and RTS.
3. Dual canonical 3,600-frame replay verification:
   - Verified Run A (`0x06281A` OFF) vs Run B (`0x06281A` ON).
   - Achieved 100% native displacement: 985 native calls (`0x06243C`: 985), 0 ROM fallbacks, 19,773 stepped instructions, 985 child calls to `0x06273A`.
   - Achieved bit-exact SHA-256 parity across all 6 canonical acceptance streams (`observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, `call-z80-ram.bin`) plus `external-events.jsonl` and `normalized-events.jsonl` with `different_frame_count == 0`.
   - Status: `CPP_REPLACEMENT_VERIFIED_PATH_SPECIFIC = YES`, `RUNTIME_REPLACEMENT_ACTIVE = YES`, `CPP_REPLACEMENT_VERIFIED_GLOBAL = NO`.

**Consequences:** The sound channel note-off / release phase attenuation committer `0x06281A` now executes 100% natively in C++, completely displacing 985 calls from ROM and establishing `0x06282E` (922 calls, caller: `0x0624C8`) as the new sole #1 remaining ROM target.

# ADR-M15-37 — Execution-Driven Recompilation: 0x017D12 Collectible Dropped Item & Chest Entity Handler (Entity Type 0x1E / 30)
**Status:** Accepted
**Date:** 2026-10-16

**Context:** Following the displacement of `0x00A8DA`, fresh actual-ROM execution census ranked `0x017D12` tied for #1 with 985 executions across 2 callers (`0x00DF1A`: 981 in the main entity update loop, `0x00FF88`: 4 in the entity spawn/initialization dispatcher). Disassembling `0x017D12..0x017E28` (278 bytes, 139 words, 70 instructions, 4 exits: `0x017D3C`, `0x017DB8`, `0x017DF0`, `0x017E26`) disproved prior working hypotheses and proved that `0x017D12` is the **Collectible Dropped Item & Chest Entity Handler** for Entity Type `0x1E` (30). The entity dispatch table at `0x00DF28 + 0x1E * 4 = 0x00DFA0` routes execution directly to `0x017D12`.
The routine governs the complete lifecycle of dropped items, item pickups, and collectible chests:
1. Spawn Initialization (`0x017D12..0x017D3C`): Checks initialization byte `$66(A6)`. If uninitialized, marks `$66(A6) = 0xFF`, copies initial elevation `$A0(A6)` to coordinate `$10(A6)`, and tests bit 7. If bit 7 is set (pre-opened chest), sets state `$04(A6) = 10`, advances animation `$2A(A6) += 2`, sets timer `$02(A6) = 1`, and exits.
2. Air / Gravity Integration & Ground Bounce (`0x017D3E..0x017D82`): If elevation coordinate `$10(A6)` is positive (falling in air), decrements vertical velocity `$56(A6) -= 0xC000`, integrates elevation `$10(A6) += $56(A6)`. When ground is reached (`$10(A6) < 0`), clamps elevation to 0, rebounds once with upward velocity `$00030000`, flags ground contact at `$A6(A6) = 0xFF`, and triggers bounce sound effect `0x00400376` via `jsr 0x00E7FC`.
3. Player Contact Detection & Pickup Trigger (`0x017D84..0x017DB8`): Evaluates entity state `$04(A6)`. If idle (`state == 0`) and player contact is triggered (`$86(A6) != 0`), transitions state `$04(A6) = 1`, arms timer `$02(A6) = 2`, increments animation `$2A(A6) += 2`, and triggers pickup sound effect `0x00900232` via `jsr 0x00E7FC`.
4. Collection, Inventory Grant & Entity Deactivation (`0x017DBA..0x017DF0`): If pickup is in progress (`state == 1`), decrements timer `$02(A6)`. Upon timer expiration (`$02 == 0`), grants item to player inventory via `jsr 0x00F53C`, sets persistent event flag via `bsr 0x017FF2`, copies item ID `$A2(A6)` and metadata `$A4(A6)` to player record `$4E..$50(A0)`, and clears entity slot `$00(A6) = 0`.
5. Chest Open & Item Grant Phase (`0x017DF2..0x017E26`): When `state == 10`, executes persistent event registration, inventory grant `jsr 0x00F53C`, clears `$04(A0)`, and deactivates entity slot `$00(A6) = 0`.

**Decision:**
1. Reverse-engineer complete `0x017D12` contract, caller contexts, and child delegation:
   - Routine bounds: `0x017D12..0x017E28` (278 bytes, 139 words, 70 instructions, 4 exits: `0x017D3C`, `0x017DB8`, `0x017DF0`, `0x017E26`).
   - Callers: 2 proven callers (`0x00DF1A`: 981 calls, return PC `0x00DF1C`; `0x00FF88`: 4 calls, return PC `0x00FF8A`).
   - Entity type: `0x1E` (30), dispatched via `0x00DF28 + 0x1E * 4 = 0x00DFA0`.
   - Register ABI: takes entity instance pointer in `A6` (`0x00FF1CD8..`); clobbers `D0.L` (sound ID parameters), `A0` (player context pointer); stack pushes return address for child calls (`jsr 0x00E7FC`, `jsr 0x00F53C`, `bsr 0x017FF2`).
   - Child delegation protocol: during nested subroutines (`jsr 0x00E7FC`, `jsr 0x00F53C`, `bsr 0x017FF2`), the instruction step driver pushes the return address to the M68K stack, sets `child_call_active = true`, records `saved_child_sp`, and returns `InstructionStepped`. While active, the driver yields to Musashi until `sp == saved_child_sp && pc == resume_pc`, ensuring bit-exact hardware sound and inventory behavior without replicating external sound drivers.
2. Implement modular instruction-stepped native replacement:
   - Created modular components strictly $\le 500$ lines under `namespace oasis::hybrid::step_017d12` and `oasis::hybrid`:
     - `src/tools/hybrid/m68k_017d12_step_ops.hpp` (40 lines)
     - `src/tools/hybrid/m68k_017d12_step_ops.cpp` (95 lines)
     - `src/tools/hybrid/m68k_017d12_step_init.cpp` (245 lines)
     - `src/tools/hybrid/m68k_017d12_step_update.cpp` (481 lines)
     - `src/tools/hybrid/m68k_017d12_instruction_step.hpp` (36 lines)
     - `src/tools/hybrid/m68k_017d12_instruction_step.cpp` (188 lines)
   - Created comprehensive unit test suite in `tests/m68k_017d12_instruction_step_test.cpp` (310 lines) validating initialization, gravity/air integration, chest open, contact sound dispatch, and timer expiration item collection.
3. Dual canonical 3,600-frame replay verification:
   - Verified Run A (`0x017D12` OFF) vs Run B (`0x017D12` ON).
   - Achieved 100% native displacement: 985 native calls (`0x00DF1A`: 981, `0x00FF88`: 4), 0 ROM fallbacks, 10,025 stepped instructions (`path_init_calls`: 4, `path_update_idle_calls`: 977, `path_update_timer_calls`: 4).
   - Achieved bit-exact SHA-256 parity across all 6 canonical acceptance streams (`observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, `call-z80-ram.bin`) plus `external-events.jsonl` and `normalized-events.jsonl` with `different_frame_count == 0`.
   - Acceptance Status: `CPP_REPLACEMENT_VERIFIED_PATH_SPECIFIC = YES`, `RUNTIME_REPLACEMENT_ACTIVE = YES`, `CPP_REPLACEMENT_VERIFIED_GLOBAL = NO` (Observed canonical update, init, and idle paths verified bit-identically in replay; chest open and falling paths verified in unit tests).

**Consequences:** The collectible item & chest entity handler `0x017D12` now executes 100% natively in C++, completely displacing 985 calls from ROM and establishing `0x06281A` (985 calls) as the new sole #1 remaining ROM target.

# ADR-M15-36 — Execution-Driven Recompilation: 0x00A8DA Direct VDP Sprite Attribute Table Entry Appender & HUD Shadow Record Builder
**Status:** Accepted
**Date:** 2026-10-16

**Context:** Following the displacement of `0x00B15E`, fresh actual-ROM execution census ranked `0x00A8DA` #1 with 1,082 executions across 5 callers (`0x00A886`: 223, `0x00A896`: 223, `0x00A81E`: 212, `0x00A82A`: 212, `0x00A836`: 212). Disassembling `0x00A8DA..0x00A8F0` (22 bytes, 11 words, 10 instructions, 2 basic blocks) proved that `0x00A8DA` is the **Direct VDP Sprite Attribute Table (SAT) Entry Appender & HUD Shadow Record Builder**. It does not perform entity movement integration, velocity application, or collision displacement. Rather, it is the atomic hardware sprite appender invoked by the HUD status bar compositor (`0x00A7E6..0x00A8D8`). For each HUD piece, it validates the current hardware sprite count against the Mega Drive VDP 80-sprite limit (`cmpi.w #80, %d5; bcc.s 0x00A8EE`), increments sprite count `D5.W`, writes the 8-byte SAT entry into the WRAM shadow buffer (`0x00FF13CC + (0x00FF188C)`) via post-increment stores `(A5)+` for Y coordinate (`D2.W`), Link+Size (`(D5.W) + D4.W`), Tile/Priority/Palette attributes (`D3.W`), and X coordinate (`D1.W`), advancing `A5` by 8 bytes.

**Decision:**
1. Reverse-engineer complete `0x00A8DA` contract and relationship to the render pipeline:
   - Routine bounds: `0x00A8DA..0x00A8F0` (22 bytes, 11 words, 10 instructions, 2 basic blocks).
   - Callers: Exactly 5 proven callers in ROM (`0x00A81E`: 212 calls, return PC `0x00A822`; `0x00A82A`: 212 calls, return PC `0x00A82E`; `0x00A836`: 212 calls, return PC `0x00A83A`; `0x00A886`: 223 calls, return PC `0x00A88A`; `0x00A896`: 223 calls, return PC `0x00A89A`). Exhaustive ROM scanning proved 0 additional callers exist in the entire ROM.
   - Register ABI: takes SAT buffer pointer in `A5` (`0x00FF13CC..0x00FF1650`), sprite count in `D5.W`, Y in `D2.W`, X in `D1.W`, size bits in `D4.W`, tile/attr in `D3.W`. Advances `A5` by +8 bytes, increments `D5.W` by +1, clobbers `D0.W` with `(D5.W + 1) + D4.W`, preserving upper words of `D0` and `D5` and all other registers.
   - Exact Musashi cycle timing: `cmpi.w` (56 master cycles), `bcc.s` (70 taken, 56 not taken via -14 byte branch adjustment), `addq.w` (28 cycles), four `move.w reg, (a5)+` (56 cycles each), `move.w reg, reg` (28 cycles), `add.w reg, reg` (28 cycles).
2. Implement modular instruction-stepped native replacement:
   - Created `src/tools/hybrid/m68k_00a8da_instruction_step.hpp/.cpp` (well under 500 lines).
   - Added unit test suite in `tests/m68k_00a8da_instruction_step_test.cpp`.
3. Dual canonical 3,600-frame replay verification:
   - Verified Run A (`0x00A8DA` OFF) vs Run B (`0x00A8DA` ON).
   - Achieved 100% native displacement: 1,082 native calls (`0x00A886`: 223, `0x00A896`: 223, `0x00A81E`: 212, `0x00A82A`: 212, `0x00A836`: 212), 0 ROM fallbacks, 9,738 stepped instructions, 1,082 SAT entries written, 0 SAT full terminations.
   - Achieved bit-exact SHA-256 parity across all 6 canonical acceptance streams (`observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, `call-z80-ram.bin`) plus `external-events.jsonl` and `normalized-events.jsonl` with `different_frame_count == 0`.
   - Status: `CPP_REPLACEMENT_VERIFIED_PATH_SPECIFIC = YES`, `RUNTIME_REPLACEMENT_ACTIVE = YES`, `CPP_REPLACEMENT_VERIFIED_GLOBAL = NO` (Normal append path observed in canonical gameplay; SAT full termination path verified in unit tests).

**Consequences:** The HUD SAT entry appender `0x00A8DA` now executes 100% natively in C++, completely displacing 1,082 calls from ROM and revealing `0x017D12` and `0x06281A` (tied at 985 calls) as the next #1 remaining ROM targets.

# ADR-M15-35 — Execution-Driven Recompilation: 0x00B15E Entity Multi-Piece Sprite Compositor, VDP SAT Shadow Buffer Builder & Dynamic VRAM Tile DMA Request Stager
**Status:** Accepted
**Date:** 2026-10-16

**Context:** Following the displacement of `0x0027A6`, fresh actual-ROM execution census ranked `0x00B15E` #1 with 1,165 executions across 2 callers (`0x00AB7A`: 948, `0x00AC74`: 217). Disassembling `0x00B15E..0x00B28C` (304 bytes, 152 words, 87 instructions, 16 basic blocks) disproved the working label "Object / Actor Collision Check" and proved that `0x00B15E` is the **Entity Multi-Piece Sprite Compositor, VDP SAT Shadow Buffer Builder & Dynamic VRAM Tile DMA Request Stager**. It does not perform actor collision detection, bounding box intersection, or terrain queries. Rather, it is the core graphics engine routine responsible for composing multi-piece hardware sprites for active entities (`A6`), reading master entity metadata tables (`0x03F2FA - 0x14 + (D5.W << 4)`), updating entity animation frame counters, clipping pieces against screen boundaries, constructing 8-byte Mega Drive VDP Sprite Attribute Table (SAT) records in WRAM (`0x00FF13CC`), updating the SAT link index (`0x00FF188A`), staging dynamic VRAM tile DMA requests (`0x00FF10B4..0x00FF1334`), and decrementing per-frame tile budget (`0x00FF1652`).

**Decision:**
1. Reverse-engineer complete `0x00B15E` contract and sprite composition pipeline:
   - Routine bounds: `0x00B15E..0x00B28C` (304 bytes, 152 words, 87 instructions, 16 basic blocks for Path 1; `0x00B28E..` unexecuted in canonical replay).
   - Callers: 2 proven callers (`0x00AB7A`: 948 calls, return PC `0x00AB7E`; `0x00AC74`: 217 calls, return PC `0x00AC78`).
   - Register ABI: takes entity instance pointer in `A6`; preserves `D1-D3, D7, A4` via stack save/restore (`movem.l d1-d3/d7/a4, -(sp)` and `(sp)+`); clobbers `D0, D4, D5, D6, A0, A1, A2, A3, A5`.
   - Cycle & bus exactness: recovered Musashi dynamic cycle models: `USE_CYCLES(s * CYC_SHIFT)` for shifts, `USE_CYCLES(2 * MUL) = +14` for `bset #31, d5` on register bit $\ge 16$, `movem.l` 280 cycles for 5-register push and pop with postincrement dummy read, and `dbf` taken/expired timing adjustments.
2. Implement modular instruction-stepped native replacement:
   - Created modular components strictly $\le 500$ lines: `src/tools/hybrid/m68k_00b15e_instruction_step.hpp/.cpp`, `m68k_00b15e_step_ops.hpp/.cpp`, `m68k_00b15e_step_setup.cpp`, `m68k_00b15e_step_piece.cpp`, and `m68k_00b15e_step_dma.cpp`.
   - Added unit test suite in `tests/m68k_00b15e_instruction_step_test.cpp`.
3. Dual canonical 3,600-frame replay verification:
   - Verified Run A (`0x00B15E` OFF) vs Run B (`0x00B15E` ON).
   - Achieved 100% native displacement: 1,165 native calls (`0x00AB7A`: 948, `0x00AC74`: 217), 0 ROM fallbacks, 130,936 stepped instructions, 1,729 SAT records written, 396 DMA descriptors staged, 479 offscreen clipped pieces, 0 SAT full terminations.
   - Achieved bit-exact SHA-256 parity across all 6 canonical acceptance streams (`observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, `call-z80-ram.bin`) plus `external-events.jsonl` and `normalized-events.jsonl` with `different_frame_count == 0`.
   - Status: `CPP_REPLACEMENT_VERIFIED_PATH_SPECIFIC = YES`, `RUNTIME_REPLACEMENT_ACTIVE = YES`, `CPP_REPLACEMENT_VERIFIED_GLOBAL = NO` (Path 1 canonical replay coverage verified; Path 2 `0x00B28E..` unexecuted in 3,600 frames).

**Consequences:** The primary multi-piece sprite compositor and dynamic tile DMA stager `0x00B15E` now executes 100% in native C++, completely displacing 1,165 calls from ROM and revealing `0x00A8DA` (1,082 calls) as the next #1 remaining ROM target.

# ADR-M15-34 — Execution-Driven Recompilation: 0x0027A6 Register-Parameterized Fixed-Buffer VDP Hardware DMA Committer
**Status:** Accepted
**Date:** 2026-10-16

**Context:** Following the displacement of `0x009BF2`, fresh actual-ROM execution census ranked `0x0027A6` #1 with 1,171 executions across 2 callers (`0x001FC4`: 822, `0x002252`: 349). Disassembling `0x0027A6..0x0027FA` (86 bytes, 43 words, 24 instructions) and the VBlank ISR (`0x001F76`) disproved the working label "VDP DMA Request Queue Stager" and proved that `0x0027A6` is the **Register-Parameterized Fixed-Buffer VDP Hardware DMA Committer (Direct SAT & CRAM VBlank DMA Executor)** — the direct sibling of `0x0027FC` (`0x0027FC..0x002896`, the WRAM Queue-Record VDP Hardware DMA Committer). While ~40 gameplay/graphics routines append 8-byte descriptors to the WRAM DMA queue (`0x00FF10B4..0x00FF1334`, pointer `0x00FF1892`) consumed by `0x0027FC`, the two fixed VBlank buffers — the Sprite Attribute Table (`0x00FF13CC` -> VRAM `0xD000`, Caller A `0x001FC4`) and Color Palette RAM (`0x00FF134C` -> CRAM `0x0000`, Caller B `0x002252`) — bypass the WRAM ring and are committed directly to VDP hardware by `0x0027A6`, sharing the VBlank DMA bandwidth budget (`0x00FF165A`, initialized to `0x00FA` and decremented by `0x0032` per transfer), VDP Mode 2 shadow (`0x00FF0BF4`), Z80 BUSREQ port (`0x00A11100`), and VDP Control/Data ports (`0x00C00004` / `0x00C00000`) with `0x0027FC`.

**Decision:**
1. Reverse-engineer complete `0x0027A6` contract and relationship to `0x0027FC`:
   - Routine bounds: `0x0027A6..0x0027FA` (86 bytes, 43 words, 24 instructions, 3 basic blocks).
   - Callers: 2 proven callers (`0x001FC4`: 822 SAT DMA calls, return PC `0x001FC8`; `0x002252`: 349 CRAM DMA calls, return PC `0x002256`).
   - Register ABI: takes packed VDP command in `D5.L`, packed DMA length registers `#19/#20` in `D6.L`, source buffer base in `A5` (`0x00FF13CC` or `0x00FF134C`), and inline 6-byte ROM DMA source register table pointer in `A6` (`0x00001FB8` or `0x00002240`); preserves `D0-D3, D6-D7, A0-A3, A5, A7`; clobbers `D4.W`, sets `D5.L = D5_in | 0x00800000`, `A4 = 0x00C00004`, `A6 = A6_in + 4`, and preserves `SR` via stack save/restore.
2. Implement modular instruction-stepped native replacement:
   - Created `src/tools/hybrid/m68k_0027a6_instruction_step.hpp/.cpp` and `tests/m68k_0027a6_instruction_step_test.cpp`, keeping `0x0027A6` and `0x0027FC` as distinct modular step units.
3. Dual canonical 3,600-frame replay verification:
   - Verified Run A (`0x0027FC` ON, `0x0027A6` OFF) vs Run B (`0x0027FC` ON, `0x0027A6` ON).
   - Achieved 100% native displacement: 1,171 native calls (`0x001FC4`: 822, `0x002252`: 349), 0 ROM fallbacks, 26,933 stepped instructions, 1,171 DMA transfers (`0x0027FC` simultaneously executed 2,806 native calls / 2,806 DMA transfers).
   - Achieved bit-exact SHA-256 parity across all 6 canonical acceptance streams (`observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, `call-z80-ram.bin`) plus `external-events.jsonl` and `normalized-events.jsonl` with `different_frame_count == 0`.
   - Marked `0x0027A6` as `CPP_REPLACEMENT_VERIFIED_GLOBAL = YES` and `RUNTIME_REPLACEMENT_ACTIVE = YES`.

**Consequences:** Both VDP hardware DMA committers (`0x0027A6` for fixed SAT/CRAM shadow buffers and `0x0027FC` for queued 8-byte WRAM descriptors) now execute 100% in native C++ (3,977 total VBlank DMA transfers across 3,600 frames with zero ROM fallbacks), isolating `0x00B15E` (1,165 calls) as the next #1 remaining ROM target.

# ADR-M15-33 — Execution-Driven Recompilation: 0x009BF2 Entity Bounding-Box Terrain Grid Footprint Bitwise OR/AND Aggregator
**Status:** Accepted
**Date:** 2026-10-16

**Context:** Following Phase 0 closure of `0x009AD6` across all six canonical acceptance streams (`observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, `call-z80-ram.bin`), fresh actual-ROM execution census ranked `0x009BF2` #1 with 1,283 executions across 8 callers (`0x00935C`: 715, `0x0091D4`: 297, `0x009312`: 132, `0x009D26`: 42, `0x009270`: 32, `0x0098D4`: 24, `0x00918A`: 22, `0x0090E8`: 19), each paired with a corresponding `0x009AD6` call immediately afterward. Disassembling `0x009BF2..0x009C40` (78 bytes, 39 words, 32 instructions) and all 8 caller pairs disproved the working label "Coordinate Delta Math" and proved that `0x009BF2` is the **Entity Bounding-Box Terrain Grid Footprint Bitwise OR/AND Aggregator**. Given an active terrain grid descriptor in `A0` (`0(A0)` = grid base `0x00FF5FAC`, `4(A0)` = row stride `64`, `8(A0)` = row shift `6`), center X in `D1.W`, center Y in `D2.W`, and half-extents radius in `D3.W` (`$46(A6)`), `0x009BF2` converts `[X - R, X + R] x [Y - R, Y + R]` into 8x8 cell bounds (`asr.w #3`), scans the 2D cell rectangle via nested `dbf` loops, and returns the bitwise OR of all covered cell bytes in `D4.B` (stored by callers to `$6F(A6)`) and the bitwise AND in `D5.B` (stored by callers to `$6E(A6)` and mapped via `0x0096E8` into `$70(A6)`), before the caller invokes `0x009AD6` to compute exact center ground elevation (`$14(A6)`).

**Decision:**
1. Reverse-engineer complete `0x009BF2` contract and pairing with `0x009AD6`:
   - Routine bounds: `0x009BF2..0x009C40` (78 bytes, 39 words, 32 instructions, 4 basic blocks, nested `dbf` loops).
   - Callers: 8 proven callers (`0x0090E8`, `0x00918A`, `0x0091D4`, `0x009270`, `0x009312`, `0x00935C`, `0x0098D4`, `0x009D26`).
   - Register ABI: preserves `D0, D1, D2, D3, D6, D7, A0, A3-A7` (saving/restoring `D2, D3, D6, D7` via `movem.l d2-d3/d6-d7, -(sp)` and `(sp)+`); clobbers `A1, A2, D4.W, D5.W`.
   - Exact Musashi timing & bus ordering: `movem.l d2-d3/d6-d7, -(sp)` writes 4 registers as eight 16-bit predecrement stores (`+224` master cycles, `skip_bus_refresh`); `movem.l (sp)+, d2-d3/d6-d7` reads 4 registers as 32-bit loads plus a 16-bit post-increment dummy read at `SP` (`+224` master cycles, `skip_bus_refresh`); `asr.w #3` adds `+42` master cycles; `lsl.w d5, d4` adds `14 * shift` master cycles and calls `skip_bus_refresh`; `dbf` adds `-14` master cycles (with extension fetch) when taken and `+14` master cycles (no extension fetch) when expired.
2. Implement modular instruction-stepped native replacement:
   - Created `src/tools/hybrid/m68k_009bf2_instruction_step.hpp/.cpp` and `tests/m68k_009bf2_instruction_step_test.cpp`, keeping `0x009BF2` and `0x009AD6` as separate modular step units.
3. Dual canonical 3,600-frame replay verification:
   - Verified Run A (`0x009AD6` ON, `0x009BF2` OFF) vs Run B (`0x009AD6` ON, `0x009BF2` ON).
   - Achieved 100% native displacement: 1,283 native calls, 0 ROM fallbacks, 124,041 stepped instructions, 18,816 inner cell reads, 4,817 outer row steps.
   - Achieved bit-exact SHA-256 parity across all 6 canonical acceptance streams (`observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, `call-z80-ram.bin`) with `different_frame_count == 0`.
   - Marked `0x009BF2` as `CPP_REPLACEMENT_VERIFIED_GLOBAL = YES` and `RUNTIME_REPLACEMENT_ACTIVE = YES`.

**Consequences:** Both halves of the 1,283-call terrain footprint + elevation query pair (`0x009BF2` and `0x009AD6`) are now active in C++, displacing the top 20 most frequent logical routines in the 3,600-frame canonical trace and isolating `0x0027A6` (1,171 calls) as the next #1 remaining ROM target.

# ADR-M15-32 — Execution-Driven Recompilation: 0x009AD6 Terrain Ground Height Calculation & Elevation Interpolation
**Status:** Accepted
**Date:** 2026-10-16

**Context:** Fresh runtime call census following the displacement of `0x016EBC` identified target `0x009AD6` tied for Rank #1 with 1,283 executions across 8 callers (`0x009106`: 19, `0x0091A8`: 22, `0x0091EE`: 297, `0x00928E`: 32, `0x009330`: 132, `0x009376`: 715, `0x0098E0`: 24, `0x009D40`: 42). Previous working hypothesis assumed a coordinate clamp or bounding-box check; disassembling and tracing runtime state revealed `0x009AD6` to be the core **Terrain Ground Height Calculation & Slope Elevation Interpolation Routine** in the map/world navigation system.

**Decision:**
1. Reverse-engineer authentic terrain calculation contract:
   - Routine bounds: `0x009AD6..0x009BF2` (284 bytes, 142 words, 72 instructions).
   - Callers: 8 distinct callers in entity and player collision/physics update routines (`0x009106`, `0x0091A8`, `0x0091EE`, `0x00928E`, `0x009330`, `0x009376`, `0x0098E0`, `0x009D40`).
   - Register ABI: preserves `D0, D1, D2, D3, D5, D6, D7, A0, A3-A7` (saving/restoring `D1-D3` via `movem.l d1-d3, -(sp)` and `(sp)+`); returns elevation in `D4.W`.
   - Branch paths: Flat ground (`0 <= D4.B < 0x10`, 1,270 calls in canonical replay); Sloped elevation (`0x10 <= D4.B <= 0x23`, 13 calls in canonical replay).
   - Dynamic cycle models: Musashi `MULU.W` requires dynamic cycle formula ($38 + 2 \times N$ cycles $\times 7$ master clock). Musashi `DIVU.W` requires iterative division loop adding dynamic cycles based on quotient bits and calls `skip_bus_refresh`. `MOVEM.L` register transfers add 168 master cycles each for save and restore, calling `skip_bus_refresh`.
2. Implement modular instruction-stepped native replacement:
   - Partition into modular files strictly obeying the $\le 500$ line limit: `m68k_009ad6_instruction_step.hpp/.cpp`, `m68k_009ad6_step_ops.hpp`, and `m68k_009ad6_step_slope.hpp/.cpp`.
   - Add unit test suite in `tests/m68k_009ad6_instruction_step_test.cpp`.
3. Dual canonical 3,600-frame replay verification:
   - Verify Run A (`0x009AD6` OFF) vs Run B (`0x009AD6` ON).
   - Achieve 100% native displacement: 1,283 calls displaced, 0 fallbacks, 31,624 stepped instructions.
   - Achieve bit-exact hash parity across all 6 canonical acceptance streams (`observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, `call-z80-ram.bin`) with `different_frame_count == 0` and `divergence == NO`.
   - Status: `CPP_REPLACEMENT_VERIFIED_PATH_SPECIFIC = YES`, `RUNTIME_REPLACEMENT_ACTIVE = YES`, `CPP_REPLACEMENT_VERIFIED_GLOBAL = NO` (canonical runtime observed flat and slope paths, not void/out-of-range fallback paths).

**Consequences:** `0x009AD6` is fully displaced natively without ROM fallback. The paired hotspot `0x009BF2` (1,283 executions across identical caller call sites) is isolated as the undisputed #1 remaining ROM routine.

# ADR-M15-SECONDARY-WEAPON-SELECTION-AND-USE-1 — Recompiled Secondary Weapon (Bow) Selection + Use Pipeline
**Status:** Accepted
**Date:** 2026-10-14

**Context:** Following the completion of the elemental environment interaction, the vertical slice required introducing the secondary weapon inventory, selection, and usage system. In Beyond Oasis, secondary weapons (Bows and Bombs) are stored in an 8-slot array at RAM `0x00FF0D80..0x00FF0D90` (stride 2 bytes: weapon ID, ammo count). The active selection is indexed by `0x00FF0DB0` (0..7), with ID cached at `0x00FF0DB1` and ammo at `0x00FF198E`. Cycling through weapons (Button::Y) traverses owned slots, skipping empty slots (`id == 0`), and wrapping around. When a secondary weapon such as the Bow (Weapon ID `0x0006`) is selected and ammo > 0, the player can fire it (Button::C), initiating State 14 (`kStateSecondaryAttack = 14`), setting timer 0, and locking player directional movement. The 16-frame sequence comprises Startup (frames 0..11) -> Fire at frame 12 (`0x007260..0x00728E`) -> Recovery (frames 13..15) -> Normal (frame 16, resets state to 0). At frame 12, an Arrow projectile (Type `0x0034`) is spawned with directional offset, 8.0 px/frame cardinal velocity, lifetime 30 ticks, bounding box `[-6, 6] x [-6, 6]`, and sprite frame `0x154C22`. Firing decrements ammo by exactly 1; if ammo drops to 0, the slot is cleared and reverts to Knife in slot 0. The arrow inflicts 6 damage on enemy Type `0x0048` with knockback, and is consumed on enemy hit or wall collision.

**Decision:**
1. Reverse-engineer authentic secondary weapon inventory, selection, and usage:
   - Weapon string table: ROM `0x05CC16` (0: None, 1: Knife, 2: Sword, 3: Broadsword, 4: Death Sword, 5: Omega Sword, 6: Bow, 7: Fire Bow, 8: Metal Bow, 9: Bomb, 10: Power Bomb, 11: Hyper Bomb, 12: Atomic Bow).
   - RAM inventory: `0x00FF0D80` (8 slots, 2 bytes/slot: `+0` ID, `+1` ammo). Selected slot at `0x00FF0DB0`, ID at `0x00FF0DB1`, ammo cache at `0x00FF198E`.
   - Cycle routine: `0x007EC4..0x007F10`. Loops 8 slots, skips unowned (`id == 0`), wraps 0..7.
   - Attack State 14: `0x005C4C` & `0x0071CC..0x00728E`. Gates on ammo > 0, locks movement.
   - Projectile spawning: Frame 12 fire branch at `0x007260`, spawns Arrow Type `0x0034` (`0x00F258`), decrements ammo by 1 (`0x008530`), clears slot and reverts to Knife if ammo reaches 0.
   - Collision & damage: `0x010A74` and `0x01BE30..0x01BEE0`. Deals 6 damage to enemy Type `0x0048`, triggers knockback along arrow direction, deactivates arrow on enemy or wall impact.
2. Implement secondary weapon module:
   - Created `src/game/player/secondary_weapon.hpp` & `secondary_weapon.cpp` with `WeaponInventorySlot`, `WeaponInventory`, `ArrowProjectile`, `cycle_secondary_weapon()`, `try_start_secondary_attack()`, `update_secondary_attack()`, and `update_arrow_projectile()`.
   - Updated `PlayerState` in `src/game/player/player.hpp` with secondary weapon fields and updated `is_attacking()` / `is_secondary_attacking()`.
   - Integrated into `ControlledScreen` (`controlled_screen.hpp` & `controlled_screen.cpp`): Button::Y weapon cycle, Button::C secondary attack (when not facing chest in READY interaction state), projectile updating and drop spawning on kill, arrow rendering via `render_pieces`, transition reset (`arrow_.active = false`), and debug overlay.
3. In-engine HUD diagnostics:
   - Displayed all 7 requested fields: `SECONDARY_WEAPON`, `SECONDARY_AMMO`, `WEAPON_STATE`, `WEAPON_TIMER`, `PROJECTILE_TYPE`, `PROJECTILE_X/Y`, `LAST_WEAPON_HIT`.
4. Deterministic tests:
   - Added 10 unit tests in `tests/secondary_weapon_test.cpp`.
   - Added Section 19 integration test in `tests/native_vertical_slice_test.cpp`.

**Consequences:** The player can now authentically switch between owned secondary weapons, fire arrows with correct directional offsets, spend ammunition, defeat enemies with projectile combat, and return safely to normal movement. All source files strictly obey the $\le 500$ line size limit.

# ADR-M15-WATER-BUBBLE-ENVIRONMENT-INTERACTION-1 — Recompiled Dytto Water Bubble → Flame Environment Interaction
**Status:** Accepted
**Date:** 2026-10-14

**Context:** Following the recompiled Dytto Active Command (Water Bubble projectile, Type `0x0018`), the vertical slice required enabling the bubble to interact with environmental targets using original ROM semantics. In Beyond Oasis, Water Bubble does not only damage/stun enemies (Type `0x0048`), but also extinguishes fire/flame hazards (Type `0x004C`) placed in the world. The collision check sweeps bounding box `[-9, 9] x [-9, 9]` (`0x011EE6..0x011F02`) and branches: if the target is an environmental water-reactive target (`0x004C`), `0x011F58` sets flame substate `$66(A0) = 0xFF` and sets `target_entity_hit = 0x004C`. The bubble transitions to State 22 (Pop) for 4 frames and deactivates. The flame enters a 15-frame extinguish lifecycle (`0x016E80..0x016EEA`), toggling visibility every frame (`(timer & 1) == 0`). At frame 15, the entity slot deactivates (`active = false`, `clr.w $0(a6)`) and records persistence bit in RAM `0x00FF0DBC` (`0x018030`). Subsequent room reloads check `is_persistent_flag_set()`; extinguished flames do not respawn. Active flames deal 15 contact damage to the player (`[-14, 14] x [-8, 8]`), while extinguished flames cease contact damage.

**Decision:**
1. Reverse-engineer authentic environment interaction from ROM:
   - Target selected: Type `0x004C` Flame Entity, located on Screen `{1, 1}` at `(192, 160)`.
   - Collision volume: `[-9, 9] x [-9, 9]` bounding box around bubble origin.
   - Water discriminator: `0x011EFC..0x011F02` distinguishes enemy (`0x0048`) vs environment target (`0x004C`).
   - Interaction trigger: `0x011F58` sets flame substate `$66 = 0xFF` and transitions bubble to State 22 (Pop) for 4 frames.
   - Extinguish lifecycle: 15 frames duration with alternating frame visibility (`flame_visible_ = ((flame_timer_ & 1) == 0)`).
   - Slot deactivation & persistence: Slot deactivates at frame 15 and sets persistent flag bit in `0x00FF0DBC` (`0x018030`).
   - Screen reload persistence: Checked during `trigger_screen_transition()`. Extinguished entities remain inactive.
2. Integrate environmental interaction into native runtime:
   - `src/game/entities/screen_entity.hpp` & `screen_entity.cpp`: added flame extinguish fields, timer, visibility toggle, contact damage hitbox `[-14, 14] x [-8, 8]`.
   - `src/game/spirits/spirit_slots.cpp`: updated `update_dytto_ability()` to detect flame collision, trigger `extinguish()`, set `target_entity_hit = 0x004C`, and transition bubble to pop.
   - `src/game/screen_instance.cpp`: added flame entity on screen `{1, 1}` at `(192, 160)` with persistent ID 1.
   - `src/game/controlled_screen.hpp` & `controlled_screen.cpp`: added persistent room flag storage (`persistent_flags_` array), check on screen load/transition, flame contact damage, and flame slot deactivation flag setting.
3. In-engine HUD diagnostics:
   - Added all 7 requested fields: `ENV_TARGET_TYPE`, `ENV_TARGET_STATE`, `ENV_TARGET_X/Y`, `WATER_REACTIVE`, `LAST_ELEMENT_HIT`, `WORLD_FLAG`, `BUBBLE_STATE`.
4. Deterministic tests:
   - Added 8 unit tests in `tests/environment_interaction_test.cpp` and Section 18 integration tests in `tests/native_vertical_slice_test.cpp`.

**Consequences:** Water Bubble now authentically interacts with environmental flame entities, extinguishing them with flashing visual feedback, updating room event flags, deactivating hazardous hitboxes, and persisting extinguished state across room transitions and reloads.

# ADR-M15-DYTTO-ACTIVE-COMMAND-1 — Recompiled Dytto Active Command (Water Bubble) Pipeline
**Status:** Accepted
**Date:** 2026-10-14

**Context:** With Dytto summonable and persistent across screen transitions, the vertical slice required enabling the player to command the spirit to execute an authentic special ability. In Beyond Oasis, Button::A is repurposed when a spirit is active: tapping A produces Command 1 (Water Bubble), double-tapping produces Command 2 (Heal), and holding A produces Command 3 (Magic Storm). Command 1 triggers Dytto's Water Bubble ability, costing 5 SP (`0x011B28` & `0x011D8A`), transitioning Dytto to State 14 (Bubble Cast) with animation selector 6 (`0x011DA8`), and spawning a Water Bubble projectile (Type `0x0018`, `$A2 = 0xFF`) at frame 24 (`0x011E04..0x011EAC`) with 16x16 px piece geometry decoded from ROM `0x141412` (tile 248, pal 3). The bubble flies along player facing direction at 4.0 px/frame for up to 24 frames (`0x011F6A`), checks collision with enemies in bounding box `[-9, 9] x [-9, 9]` (`0x011EE6`), inflicts damage and stun upon hit, transitions to State 22 (Pop) with animation selector 36, and deactivates. Dytto returns to Follow State 2 with hover animation selector 4.

**Decision:**
1. Reverse-engineer authentic spirit command decoding and dispatch:
   - Input decoder: `0x011A4C..0x011B26` monitors Button::A (`0x40` in `$FF165E`). Fresh press and release before 20 frames initiates a 6-frame double-tap window (`$FF19A1`). When the window expires, Command 1 (`$FF19A2 = 1`) is emitted.
   - Command dispatch: `0x0121B4..0x0121CE` routes Command 1 to routine `0x011D8A`.
   - SP gate & deduction: Player SP (`+$8E(A5)`) must be $\ge 5$; subtracts 5 SP (`player.sp -= 5`). If SP < 5, command is rejected and spirit remains in follow state.
   - Cast sequence: Dytto enters State 14 (`+$04 = 14`), animation selector 6 (`+$2A = 6`), timer 24 ticks (`0x011DA8`).
   - Projectile spawning: At frame 24, spawns Water Bubble entity (Type `0x0018`, substate `$A2 = 0xFF`), State 16 (Fly), velocity 4.0 px/frame cardinal, lifetime 24 ticks, pieces decoded from ROM `0x141412` (tile 248, pal line 3).
   - Collision & reaction: Checks bounding box `[-9, 9] x [-9, 9]`. When colliding with enemy (Type `0x0048`), deals damage/stun, transitions bubble to State 22 (Pop) for 4 frames, and clears slot (`0x011FA0: clr.w $0(a6)`).
   - Return to follow: Dytto returns to State 2 (Follow/Idle) with animation selector 4 upon cast completion.
   - Room transition safety: Screen transition calls `cleanup_spirit_ability()`, clearing any active projectile and restoring Dytto to State 2.
2. Integrate active command into native runtime:
   - `src/game/spirits/spirit_slots.hpp` and `spirit_slots.cpp`: added `decode_dytto_command_input()`, `try_execute_dytto_command()`, `update_dytto_ability()`, `cleanup_spirit_ability()`, and command/projectile fields in `ActiveSpirit`.
   - `src/game/controlled_screen.cpp`: hooked command decoding when spirit is active, updated ability and projectile rendering in `render_rom_backed()`, safely reset ability in `trigger_screen_transition()`, and extended debug overlay.
3. In-engine HUD diagnostics:
   - Added all 7 requested fields: `SPIRIT_CMD`, `SPIRIT_CMD_STATE`, `SPIRIT_CMD_TIMER`, `SPIRIT_SP_COST`, `SPIRIT_EFFECT_TYPE`, `SPIRIT_EFFECT_X/Y`, `SPIRIT_TARGET`.
4. Deterministic tests:
   - Added 10 unit tests in `tests/spirit_slots_test.cpp` and Section 17 integration test in `tests/native_vertical_slice_test.cpp`.

**Consequences:** Dytto now responds to real player command inputs, executes the authentic Water Bubble ability, consumes 5 SP, spawns a ROM-backed 16x16 projectile, damages and stuns enemies, pops on contact or range expiry, and returns to follow state. All source files remain strictly $\le 500$ lines.

# ADR-M15-SPIRIT-SUMMONING-PIPELINE-1 — Recompiled Authentic Spirit Summoning Pipeline
**Status:** Accepted
**Date:** 2026-10-14

**Context:** Following the completion of the enemy death and item pickup loop, the vertical slice required introducing the core mechanic of Beyond Oasis / The Story of Thor: the spirit summoning system. In Beyond Oasis, spirits are summoned by the player via the Gold Armlet (Button::A) when the corresponding spirit slot has been acquired (tracked in `0x00FF0DBA` bits 0..3) and player magic resource (SP at `+$8E(A5)`) is available. Summoning allocates the spirit into the singleton spirit slot in RAM at `0x00FF1AA4` (stride `0xBC` after player `0x00FF19E8`), sets the spirit type (Dytto = Type `0x0018`), initializes elevation `z = z + 16` (`0x018D08`), enters State 26 (Appear) with animation selector 34 (`0x22`), and transitions to State 2 (Follow/Idle) with hover animation selector 4. The spirit tracks the player with smooth directional deltas (`0x01215E`) and hover elevation offset (`0x0123B4`), drains 1 SP every 45 frames (`0x011B72`), despawns upon SP exhaustion, and persists across screen transitions by teleporting to destination player coordinates (`0x03A464..0x03A47E`).

**Decision:**
1. Reverse-engineer authentic spirit summoning pipeline from ROM:
   - First spirit selected: Dytto (Water Spirit), Entity Type `0x0018` (Row 11 at ROM `0x03F35A`).
   - Slot mapping: Event `0x16` sets bit 0 of `0x00FF0DBA` (Slot 0). Acquired mask checked via `SpiritSlots::acquired(0)`.
   - RAM slot: `0x00FF1AA4` (singleton spirit entity slot).
   - Spawn routine: `0x014668..0x0146BE` and `0x018CDE..0x018D4C`.
   - Initial position: `(player.x, player.y, player.z + 16)` with elevation offset `0x10.0000` (16 px) above summon point (`0x018D08`).
   - Initial state: State 26 (`0x1A`), animation selector 34 (`0x22`).
   - State transition: After appear timer (20 frames), advances to State 2 (`kSpiritStateFollow`), anim selector 4 (hover).
   - Follow model: calculates delta vector towards player (`0x01215E`), hover elevation 48 px (`0x0123B4`), updates facing direction, steps towards player within leash bounds.
   - Resource drain: Player SP at `+$8E(A5)`. Decrements 1 SP every 45 frames (`0x2D` ticks in `0x011B72`).
   - Despawn: Upon SP == 0, spirit deactivates (`active = false`, State 28 `0x1C`).
   - Screen transition: Spirit persists across rooms, teleporting to player destination coordinates (`0x03A464..0x03A47E`).
   - Duplicate slot protection: Second summon attempt rejected while spirit is active (`NO_DUPLICATE_SLOT`).
2. Integrate spirit pipeline into native runtime:
   - `src/game/spirits/spirit_slots.hpp` and `spirit_slots.cpp`: added `ActiveSpirit`, constants, `try_summon_spirit()`, `update_active_spirit()`, `reposition_spirit_on_screen_transition()`, `despawn_spirit()`.
   - `src/game/player/player.hpp`: added `sp{100}` and `max_sp{100}` resource fields.
   - `src/game/controlled_screen.cpp`: integrated Button::A summon, spirit update, spirit sprite piece rendering, screen transition repositioning, and debug overlay.
3. In-engine HUD diagnostics:
   - Added all 8 requested spirit fields to `render_debug_overlay()`: `SPIRIT_ACTIVE`, `SPIRIT_TYPE`, `SPIRIT_SLOT`, `SPIRIT_STATE`, `SPIRIT_X/Y`, `SPIRIT_TARGET_X/Y`, `SPIRIT_RESOURCE`, and `SPIRIT_ANIM`.
4. Deterministic tests:
   - Added 10 unit tests in `tests/spirit_slots_test.cpp` and Section 16 integration test in `tests/native_vertical_slice_test.cpp`.

**Consequences:** The first authentic spirit (Dytto) is summonable in the native vertical slice, accurately following the player, consuming magic resource, persisting across screen transitions, and deactivating on resource exhaustion. File line limit (<=500 lines) and frozen helper island policies are strictly preserved.

# ADR-M15-ENEMY-DEATH-LOOT-DROP-PICKUP-1 — Recompiled Authentic Enemy Death -> Loot Drop -> Player Pickup Pipeline
**Status:** Accepted
**Date:** 2026-10-14

**Context:** Following the player melee attack and enemy damage/death response, the vertical slice required completing the gameplay feedback loop: defeated enemies must produce authentic ROM-derived item drops, which spawn as authentic entities in the world and can be collected by the player to apply real game effects (such as health restoration). In Beyond Oasis, enemy death does not immediately delete the entity: it triggers a 16-frame flashing death state, evaluates an 8-byte loot table via the drop spawn routine, allocates an item entity from the object pool (Entity Type 0x0008), initializes coordinates with elevation and vertical bounce velocity, and sets the item in its idle ground state. When the player overlaps the item's hitbox, the item effect routine restores player health, latches single consumption, deactivates the item entity, and screen transitions properly isolate screen-local drops.

**Decision:**
1. Reverse-engineer authentic M68K enemy death and drop pipeline:
   - Death completion routine: `0x017E82..0x017F6A`. Upon HP <= 0, enemy enters State 2 (Death), animation 2, timer `$2(a6) = 16` ticks (`0x10`). Counts down for 16 frames while flashing sprite visibility (`bchg #1, $36(a6)`). Upon timer expiration: loads loot table `lea.l $17f6c(pc), a1`, calls drop routine `jsr $f476.l`, deactivates slot `clr.w $0(a6)`.
   - Drop table at ROM `0x017F6C`: 8 bytes `[0x0000, 0x1E01, 0x0000, 0x1E01]`. Selects Item `0x1E` (cheese / healing food) and quantity 1.
   - Drop decision & item spawn routine: `0x00F476..0x00F538`. Allocates entity slot from pool `$ff2abc` via `bsr.w $d9be`, sets type to Type `0x0008` via `bsr.w $8d06`, copies enemy position `(x, y, z + 1)` with elevation offset `addq.w #1, d0`, initial vertical velocity `vz = 0x80000`, sets item state `+$04 = 2` (Ground / active idle).
2. Model Item Entity (Type `0x0008`) using canonical entity descriptor:
   - Row 3 at ROM `0x03F35A`: `dim_x = 8, dim_y = 8`, GFX descriptor `0x155AF6`, animation descriptor `0x155D76`.
   - Decodes frame 0 sprite piece from `0x155B76`: 1 piece, 3x3 tiles (`w=3, h=3`), offset `(0, -12)`, base tile index 1268, palette line 3.
   - Spawns drop entity using standard `ScreenEntity` runtime; rendered with existing entity multi-tile renderer without synthetic icons.
3. Model pickup hitbox and one-time consumption:
   - Pickup overlap check (`0x0082F8` / `0x00B852`): Item AABB `[-4, 4] x [-4, 4]` against player footprint radius 10 (`[-10, 10] x [-10, 10]`), elevation gate `abs(player_z - item_z) <= 16`.
   - One-time consumption: When collected, `item.collect()` deactivates slot (`active_ = false`), preventing duplicate pickup.
4. Recompile item effect routine (`0x0053A2..0x0053E8`):
   - Item `0x1E`: restores `player.max_hp / 4` (25 HP for max 100), clamped to max HP: `player.hp = min(player.max_hp, player.hp + 25)`.
   - Tracks `last_pickup_type = 0x001E`, `last_pickup_heal = 25`, increments `item_count`.
5. Screen lifetime / persistence:
   - Drops are screen-local; screen transitions discard temporary drops when reloading destination screen entities from ROM.
6. Extend in-engine HUD diagnostics with all 7 requested fields:
   `DROP_TYPE`, `DROP_STATE`, `DROP_X/Y`, `DROP_ACTIVE`, `PICKUP_TARGET`, `PLAYER_HP/CURRENCY/ITEM_COUNT`, and `LAST_PICKUP`.
7. Add 10 deterministic tests in `tests/native_vertical_slice_test.cpp` verifying drop spawn, position matching, activity, rendering, range gating, pickup overlap, single effect application, removal, screen transition clearing, and deterministic RNG.

**Consequences:** Defeated enemies authentically produce loot drops that can be picked up to restore player HP, completing the full combat loop on the native vertical slice. All files strictly comply with the <=500 lines limit. Frozen helper island policy is maintained.

# ADR-M15-PLAYER-MELEE-ATTACK-ENEMY-DAMAGE-1 — Recompiled Player Melee Attack Path and Enemy Damage/Death Response
**Status:** Accepted
**Date:** 2026-10-14

**Context:** Following the player ↔ moving enemy (Type 0x0048) harmful contact and damage pipeline, the vertical slice required the reciprocal combat path: player melee attack capability using original ROM semantics. In Beyond Oasis, basic melee attacks (Button::B) are not instantaneous AABB volumes: they involve structured startup, active, and recovery phases, movement lock, directional weapon bounding boxes looked up from ROM tables, hit-once-per-swing latches to prevent multi-hit exploits, and enemy-side damage reception, hurt/recoil state machines, and death deactivation.

**Decision:**
1. Reverse-engineer authentic M68K player attack path:
   - Button::B attack input handler (`0x004A8E: btst.b #4, $03(a6)`).
   - Attack state transition: sets movement state `+$04 = 10` (State 10: Melee Attack), zeros velocity `(+$4E, +$52) = (0, 0)`, and resets attack timer `+$36(A6) = 0`.
   - Frame timing: 9 frames total duration comprising 4 startup frames (0..3, `attack_active = false`), 2 active hit frames (4..5, `attack_active = true`), and 3 recovery frames (6..8, `attack_active = false`). Directional movement is locked during the entire 9-frame attack sequence.
2. Reconstruct authentic player weapon hitbox from ROM table `0x058176` (Record 0, dagger/basic melee):
   - Offsets relative to player center `(world_x, world_y)`:
     * Dir 0 (Up): `[-8, 7] x [-40, -9]`
     * Dir 1 (Right): `[8, 47] x [-8, 7]`
     * Dir 2 (Down): `[-8, 7] x [8, 47]`
     * Dir 3 (Left): `[-47, -8] x [-8, 7]`
   - Hitbox is active *only* during frames 4 and 5; returns `std::nullopt` during startup and recovery.
3. Recompile enemy damage receiver, hurt state, and death lifecycle for Type `0x0048`:
   - Single-hit latch: `PlayerState::attack_hit_latched` (ROM `bit 3 of $37(a6)`) prevents multiple damage applications within a single swing.
   - Damage application (`ScreenEntity::apply_damage()`): decrements enemy HP by 10 (Type 0x0048 HP: 20 -> 10 on first hit).
   - Hurt reaction: when HP > 0, actor enters State 4 (Hurt), setting hurt timer to 20, invulnerability timer to 20, knockback timer to 6, and knockback velocity to 3.0 px/frame (`0x30000`) along attack direction. Recovers to State 1 upon timer expiry.
   - Death transition: when HP <= 0 on second hit (10 -> 0), actor enters State 2 (Death), clearing active flag `clr.w $0(a6)` (`active_ = false`) to deactivate the slot and exclude it from collision and updates.
4. Model player attack fields in `PlayerState`: `attack_state (0)`, `attack_timer (0)`, `attack_active (false)`, `attack_direction (0)`, `attack_hit_latched (false)`, `last_hit_damage (0)`, and `is_attacking()` predicate.
5. Recompile portable C++20 routines in `src/game/player/player.cpp`:
   - `start_player_attack()`: initiates State 10 attack and movement lock.
   - `update_player_attack()`: advances attack frames, toggles active window, and restores normal control upon completion.
   - `player_attack_hitbox()`: evaluates directional transforms from ROM table `0x058176`.
6. Enforce screen transition safety: screen transitions clear player attack state and reset enemy combat latches.
7. Extend in-engine HUD diagnostics with all 8 requested combat fields:
   `PLAYER_ATK_STATE`, `PLAYER_ATK_TIMER`, `PLAYER_ATK_ACTIVE`, `PLAYER_ATK_BOX`, `ENEMY_HP`, `ENEMY_HURT`, `ENEMY_STATE`, and `LAST_HIT_DAMAGE`.

**Consequences:** The player can now engage and defeat moving enemies using authentic ROM combat mechanics. All files strictly comply with the <=500 lines limit. Frozen helper island policy is maintained.

# ADR-M15-ENEMY-CONTACT-DAMAGE-PIPELINE-1 — Recompiled Authentic Player ↔ Moving Enemy (Type 0x0048) Contact and Damage Pipeline
**Status:** Accepted
**Date:** 2026-10-14

**Context:** Following the recompiled moving actor state machine for Type 0x0048, the vertical slice required authentic gameplay interaction between the moving actor and the player. In Beyond Oasis, enemy contact is not simply generic AABB intersection: physical movement blocking must be strictly distinguished from directional harmful attack collision, damage gating must respect invulnerability timers and hurt states, knockback vectors must follow attacker facing direction with authentic speed and duration, knockback movement must be grounded in the terrain collision grid, and control must return to normal upon recovery.

**Decision:**
1. Reverse-engineer the authentic M68K combat and collision pipeline:
   - Entity-player collision loop (`0x00BC8E` -> `0x00BDE0`): tests active pool entities against player `0xFF19E8`.
   - First-layer invulnerability gate (`0x00BE4C: tst.b $82(a0); bne $be5c`): blocks all harmful contact while invulnerability timer `$82 > 0`.
   - Second-layer invulnerability bit test (`0x00BDEE: btst.b #3, $38(a0); bne $be5e`).
   - Contact resolution (`0x00BEBA` / `0x00BED4`): sets attacker pointer `+$7E(A0)`, invulnerability timer `+$82(A0) = 30` frames, damage code `+$86(A0)`, knockback timer `+$83(A0) = 6` frames, and hit direction `+$84(A0) = attacker.direction`.
   - Knockback vector calculation (`0x00BF2E`): cardinal speed `0x0300` (fixed-point `0x30000`, 3.0 px/frame) and diagonal `0x02C0` dispatched via `0x00E2D4` along hit direction `d5`, written to knockback velocity `(+$4E, +$52)`.
   - Player damage handler (`0x00557A` / `0x0057FA..0x005942`): resolves raw damage (`10 HP` for Type 0x0048 from Record 0 byte +2 at ROM `0x058C82`), computes effective damage (`max(1, raw - defense)`), decrements player HP at `+$8A`, and sets player movement state `+$04 = 24` (State 24: Hurt / Knockback).
   - Hurt update routine (`0x005CC8`): decrements knockback timer `+$83`, displaces player by knockback velocity `(+$4E, +$52)` with terrain collision gating, and recovers to normal control State 0 (`+$04 = 0`) via `0x0062CC` when timer expires.
   - Invulnerability timer countdown (`0x00E682`): decrements `+$82` each frame down to 0.
2. Distinguish physical blocking from directional attack hitboxes:
   - Physical blocking uses `hitbox()` (`8x8` half-extents `4x4`), blocking player movement without damage.
   - Harmful attack uses `attack_hitbox()`, an authentic directional box from behavior descriptor Record 0 at ROM `0x058C82`:
     * Dir 0 (Up): `[-8, 8] x [-16, -4]`
     * Dir 1 (Right): `[4, 16] x [-6, 6]`
     * Dir 2 (Down): `[-8, 8] x [4, 16]`
     * Dir 3 (Left): `[-16, -4] x [-6, 6]`
   - Attack hitbox is active *only* when Type 0x0048 is in State 2 (Charge). In States 1 (Wait) and 3 (Return) or on static props (Type 0x002A chest), `attack_hitbox()` returns `nullopt`.
3. Model player damage fields in `PlayerState`: `hp (100)`, `max_hp (100)`, `invuln_timer (0)`, `hurt_timer (0)`, `knockback_x_fixed (0)`, `knockback_y_fixed (0)`, `contact_entity (0)`, and `damage_gate_open (false)`.
4. Recompile portable C++20 damage routines in `src/game/player/player.cpp`:
   - `apply_enemy_contact_damage()`: gates damage on invulnerability/hurt timers; decrements HP by 10; sets hurt timer to 6, invulnerability timer to 30, movement state to 24, and directional knockback velocity.
   - `update_player_hurt_knockback()`: updates timers, applies knockback velocity with terrain collision checks via `evaluate_terrain_gate`, and recovers to State 0 when knockback completes.
5. Enforce screen transition isolation: screen transitions clear `contact_entity`, `damage_gate_open`, `hurt_timer`, `invuln_timer`, and knockback velocity.
6. Extend in-engine HUD diagnostics with the 7 required fields: `PLAYER_HP`, `PLAYER_STATE`, `INVULN`, `KNOCKBACK_X/Y`, `CONTACT_ENTITY`, `DAMAGE_GATE`, `ACTOR_STATE`.

**Consequences:** Moving Type 0x0048 actors interact authentically with the player using exact ROM gameplay semantics. No synthetic physics or arbitrary knockback values. All source files strictly comply with the <=500 lines limit.

# ADR-M15-MOVING-ENTITY-STATE-MACHINE-1 — Recompiled First Real Moving ROM-Backed Entity State Machine
**Status:** Accepted
**Date:** 2026-10-14

**Context:** Following static props, multi-tile sprite rendering, animation playback, and player collision/interaction (Type 0x002A chest), the vertical slice required dynamic actors with original ROM state machines. Enemies and NPCs in Beyond Oasis rely on bounded state routines driven by timers, directional detection corridors, velocity calculation, terrain collision gating, and cyclic return-to-anchor behavior.

**Decision:**
1. Select Type 0x0048 (Row 35 in table 0x0003F2FA), a recurring patrol enemy/hazard with dimensions 8x8, GFX descriptor table 0x165E12 (6 multi-tile sprite frames), animation sequence table 0x040F4A, and update handler dispatched via enemy jump table 0x00DF54 to 0x01778C.
2. Reverse-engineer the 4-state state machine at 0x01778C:
   - State 0 (0x0177A6): Init / Anchor setup ($a8 = $8, $aa = $c, $2a = 0, state = 1).
   - State 1 (0x0177C6): Wait / Line-of-sight detection using 4-directional table 0x1783E (24 tiles = 192 px range) checked against player bounding box via 0x00C1CE. When player is detected, sets fixed-point velocity via 0x00E2D4, anim_selector = 2, timer = $9e (30 ticks), state = 2.
   - State 2 (0x01784E): Walk / Charge movement. Advances position by fixed-point velocity. Decrements timer. Tests terrain collision via ByteGridView and native directional gate 0x00938E (evaluate_terrain_gate). When timer expires or terrain blocks, toggles direction bit 1 (bchg.b #1, $17(a6)), anim_selector = 4, state = 3.
   - State 3 (0x0178B2): Return to anchor ($a8, $aa) via 0x00E45E. Steps towards anchor. When distance == 0, restores forward direction, anim_selector = 0 (Idle), state = 1.
3. Introduce native `ActorRuntime` model in `src/game/entities/screen_entity.hpp` holding state, substate, timer, direction, anchor coordinates, velocity deltas, animation selector, wait duration, speed, and halted flag.
4. Recompile state transitions in pure C++20 (`update_actor_state_machine()`), coupling sprite frame selection (Frame 0 idle, Frame 3 walk/charge, Frame 1 turn) to state.
5. Enforce fail-closed behavior on unsupported states (`actor_.halted = true`), freezing the actor and keeping the last safe frame.
6. Integrate Type 0x0048 onto Screen 0x0100 at (160, 120) with direction 2 (Down), coexisting with chest Type 0x002A at (192, 160).
7. Extend HUD diagnostics with actor state machine values (`ACT_ST=... TMR=... DIR=...`).

**Consequences:** The native engine now features fully authentic ROM-driven dynamic actor movement, animation, terrain collision, and patrol cycles. Helper count remains preserved (frozen helper island policy). All files strictly comply with the <=500 lines limit.

# ADR-M15-ENTITY-COLLISION-INTERACTION-1 — ROM-Backed Player ↔ Entity Collision and Interaction
**Status:** Accepted
**Date:** 2026-10-14

**Context:** Following authentic multi-tile sprite rendering and ROM-driven animation playback for ScreenEntity in ControlledScreen, entities remained purely visual. Player movement could pass directly through physical objects (such as chests), and interactive state transitions (opening chests on player action input) were not modeled.

**Decision:**
1. Reconstruct M68K bounding box conventions from ROM consumers `0x009C7C..0x009CFE` (static overlap test) and `0x00BB18..0x00BBA6` (entity pool collision). Establish that positions are center-based (`world_x, world_y, world_z`) and dimensions (`dim_x = 24, dim_y = 20` for Type 0x002A at table `0x0003F2FA`) define half-extents (`radius_x = dim_x / 2 = 12, radius_y = dim_y / 2 = 10`, mirrored at pool offsets `+0x42` and `+0x44`).
2. Implement `EntityHitbox` (`src/game/entities/screen_entity.hpp`) with `left, top, right, bottom, z_min, z_max` and `overlaps()` method.
3. Integrate entity collision into the player movement update pipeline: candidate movement -> terrain collision -> entity collision check -> accept/revert position. If candidate overlaps an active entity hitbox, movement is rejected (`moved = false, blocked = true`).
4. Implement interaction probe checking proximity (within 8 px expanded hitbox) and directional facing. When player faces an entity within range, candidate is acquired (`interaction_target_type = entity.type_id()`, `interaction_state = READY`).
5. Wire action button inputs (`Button::A`, `Button::B`, `Button::C`) to trigger entity interaction (`trigger_interaction()`). For Type 0x002A (chest), trigger begins opening animation sequence from frame 0 -> frame 1 -> frame 2, where it permanently halts in `OPENED` state without looping.
6. Enforce screen transition isolation: screen transitions clear `overlap_detected_`, `interaction_target_type_`, and reset `interaction_state_str_ = IDLE`. ScreenInstance entity ownership remains authoritative.
7. Extend in-engine debug HUD to 24 lines, adding `ENTITY_HITBOX`, `PLAYER_HITBOX`, `OVERLAP`, `INTERACTION_TARGET`, and `INTERACTION_STATE`.

**Consequences:** Physical blocking on all four sides and diagonal approaches is authentic. Chest interaction transitions from IDLE -> READY -> TRIGGERED (OPENING) -> OPENED. Screen transition guarantees zero dangling collision references. All files remain strictly <= 500 lines.

# ADR-M15-CONTROLLED-SCREEN-TRANSITION-1 — Native ControlledScreen Multi-Screen Transitions
**Status:** Accepted
**Date:** 2026-10-13

**Context:** Following the initial rendering of canonical ROM Screen 0x0100, the native engine must move from a single isolated screen to connected screens without process restarts. The transition must ensure explicit resource ownership, eliminate dangling pointers to decompressed grid views, update collision boundaries immediately, and provide seamless player and camera handoff.

**Decision:**
1. Introduce `ScreenInstance` (`src/game/screen_instance.hpp`/`.cpp`) encapsulating screen descriptor data, decompressed primary stream bytes, 512 Genesis tiles, CRAM palettes, decoded 64x28 tilemap words, and the 64x64 `world::ByteGridView`.
2. Manage screen lifetimes using `std::unique_ptr<ScreenInstance> current_screen_` and `previous_screen_` in `ControlledScreen`. On transition, `previous_screen_` retains the old screen while `current_screen_` is instantiated from the target `ScreenId`, re-binding `terrain_` safely to the new screen's byte grid without dangling pointers.
3. Select adjacent Screen B as setup `0x0101` (`Group: 1, Index: 1`), descriptor `0x02E612`, primary stream `0x212684` (1,590 compressed -> 12,460 decompressed bytes), sharing GFX resources `[13, 14, 15, 16]`.
4. Support both programmatic/edge transitions (`EDGE_BOUNDARY_TRIGGER`) and debug transitions (`DEBUG_KEY_TRANSITION` mapped to `Button::X` / `'T'`).
5. Handoff player position deterministically: entering 0x0101 at `(160, 156)` and returning to 0x0100 at `(160, 180)`, preserving direction and physics. Re-center camera bounded within `[0, 512-320] x [0, 512-224]`.
6. Extend debug overlay with `CURRENT_SCREEN_SETUP_ID`, `PREVIOUS_SCREEN_SETUP_ID`, `TRANSITION_COUNT`, `PRIMARY_STREAM`, `GRID_BASE/OFFSET`, and `PLAYER_CELL`.

**Consequences:** Seamless runtime transitions between authentic ROM screens 0x0100 and 0x0101 occur without restarting the executable. Differential collision proves the new screen's collision grid is immediately active. All files remain strictly below 500 lines.

# ADR-M15-CONTROLLED-SCREEN-ROM-BACKED-1 — Native ControlledScreen Integration of Canonical ROM Screen 0x0100
**Status:** Accepted
**Date:** 2026-10-13

**Context:** The existing `ControlledScreen` rendered a synthetic 32x28 obstacle course fixture with uniform colored rectangles. To advance towards a native game engine, the executable must render authentic game screens directly from the canonical Beyond Oasis ROM using real graphics decompression, Genesis VDP tile representation, screen descriptors, tilemaps, palettes, and collision byte-grids.

**Decision:**
1. Extend `ControlledScreen` with an optional `std::span<const std::uint8_t> rom` constructor parameter. If empty, the deterministic synthetic fixture is preserved.
2. When a canonical ROM is supplied, wire Screen Setup `0x0100` (`Group: 1, Index: 0`), reading descriptor at ROM `0x02E57E`.
3. Decompress primary resource `STREAM_211F7A` at ROM `0x211F7A` (1,801 compressed -> 12,460 decompressed bytes) via `decompress_graphics()`.
4. Bind the 64x64 collision grid from decompressed offset `0x204` to `world::ByteGridView`.
5. Load GFX resources `[13, 14, 15, 16]` from ROM via `load_graphics_resource()`, converting into 512 8x8 4bpp Genesis tiles.
6. Parse CRAM palette lines 1 and 2 from decompressed offset `0x3004..0x3044` using `cram_to_rgb()`.
7. Render 64x28 Genesis VDP tilemap words from decompressed offset `0x1204..0x2004` with priority, palette selection, horizontal/vertical flipping, and bounded camera scrolling.
8. Retain existing native player physics at `TEST_SPAWN (160, 160)` with collision evaluated against the ROM-derived byte grid.
9. Provide an in-engine toggleable 8-line diagnostic overlay (`Button::Mode` / `Tab` / `M`) via a dedicated modular bitmap font renderer (`oasis::game::render::draw_text`).

**Consequences:** The native executable `oasis.exe` now visibly renders an authentic ROM screen (screen setup `0x0100`) with authentic background tiles, palette, and collision constraints, while retaining zero regressions on synthetic fixture tests. Source files adhere to the strict <=500 lines constraint.

# ADR-M13-SCHEDULER-INSTRUCTION-STEP-FIXTURE-1 — Isolated GPGX Scheduler Oracle
**Status:** Accepted for the tested synthetic instruction subset only
**Date:** 2026-10-13

**Context:** The regular M68K hook can make a native helper return before or
after the ROM routine's line-budget crossing. Comparing only routine outputs
cannot establish whether Z80, VDP, refresh, or IRQ ordering remains equivalent.

**Decision:** Keep timing experiments in a dedicated copy of the external
GPGX source with `HOOK_CPU=1`. The fixture invokes GPGX's actual
`system_frame_gen()` scheduler and records core-native M68K cycle/cycle_end/
refresh state, Z80 cycles/state, VDP line timing, IRQ transitions, ordered
instruction markers, and writes. Its test block hook replaces one instruction
at a time only for the supported synthetic opcodes, charges the core cycle
table, and returns to the normal core loop after each step. The observation
hook must return the original hooked read value unchanged. The installed helper
and all production runtime paths remain untouched.

**Consequences:** Ten synthetic ROM-versus-step cases pass in GNU/Linux Debug
and Release, including refresh, line-budget crossings, active Z80/VDP
interleave, IRQ entry/RTE, and resumed execution. This verifies the primitive
only for the tested instruction subset and fixture scheduler configuration; it
does not verify routine `0x9D70`, arbitrary instruction sequences, global
replacement, or production routing. All ROM evidence authorities remain
unchanged.

**Evidence:** `docs/reports/THOR_SCHEDULER_FIXTURE_20261013.md` and machine
receipts under ignored `build/scheduler-fixture-20261013/`.

# ADR-M12-STRUCTURAL-DESCRIPTOR-STREAM-V3 — Descriptor-Selected Variable-Length Structural Contributions
**Status:** Accepted for structural consumer-family evidence only
**Date:** 2026-10-02

**Context:** The existing fixed-size and pointer-table contribution schemas do
not represent one family resource that owns both a descriptor record and its
parser-bounded variable-length target, with a directly replayable provenance
chain. Extending the established variable-stream v2 validator in place would
invalidate its pinned source hashes and risk changing already accepted family
contracts.

**Decision:** Add the isolated `thor.static-family-contribution.v3` schema in
`static_family_descriptor_stream.py`. It records descriptor and stream ranges
per resource, source-artifact hashes, exact parser boundary evidence, a family
descriptor contract, and root→descriptor→stream→consumer→boundary edges. The
AUTO49 adapter requires agreement between deterministic Python and native
decoder consumed ends and output lengths, and builds a removable structural
view by exact range union and intersection.

**Consequences:** v1, v2, the runtime bitmap, `TOTAL_PROVEN`, `SOURCE_OWNED`, and
canonical SQLite remain unchanged. This schema establishes structural
membership only and does not claim runtime execution or graphics semantics.

# ADR-M12-W5C-EXACT-AUDIO-RESOURCE-OWNERSHIP-PROMOTION-V1 — Exact Audio Resource Ownership, Partition Split, and Canonical Promotion
**Status:** Accepted for M12 W5c
**Date:** 2026-09-21

**Context:**
Following W5 round-trip verification, two Mode 0 audio resources (`AUDIO_RESOURCE_FORMAT_A_0001` at `0x0BD540..0x0BF768`, 8,744 bytes, and `AUDIO_RESOURCE_FORMAT_A_0002` at `0x0BC95C..0x0BD540`, 3,044 bytes, combined 11,788 bytes) were proven byte-identical to canonical ROM without original nibble/byte dependence in the encoder. The pre-promotion canonical ROM knowledge map holds emission range `0x0B8000..0x0BF768` (30,568 bytes) as unowned `UNKNOWN` (`source_owned = 0`). Under ADR-0044, exact promoters alone may update ownership under their current gates.

**Decision:**
1. **Ownership Eligibility Qualification**:
   - Qualify exact ownership eligibility for `AUDIO_RESOURCE_FORMAT_A_0001` (8,744 bytes) and `AUDIO_RESOURCE_FORMAT_A_0002` (3,044 bytes) under claim type `RECONSTRUCTION_VERIFIED` (`STATIC_VERIFIED`).
   - Confirm boundaries, format, deterministic decode, semantic inverse encode, byte-identical round-trip, encoder independence, and canonical ROM identity.
2. **Canonical Emission Partition Split**:
   - Authorize canonical knowledge tooling (`w5_audio_promote.py`) to split canonical emission range `0x0B8000..0x0BF768` into three exact disjoint intervals:
     * `[0x0B8000, 0x0BC95C)` (18,780 bytes): unpromoted `UNKNOWN`, `source_owned = 0`, artifact `blobs/0B8000_0BC95C.bin`.
     * `[0x0BC95C, 0x0BD540)` (3,044 bytes): `AUDIO_RESOURCE_FORMAT_A_0002`, `SOUND_DATA_CONTAINER_CONFIRMED`, `DATA_KNOWN`, `source_owned = 1`, `DATA`, artifact `data/audio_0BC95C.bin`.
     * `[0x0BD540, 0x0BF768)` (8,744 bytes): `AUDIO_RESOURCE_FORMAT_A_0001`, `SOUND_DATA_CONTAINER_CONFIRMED`, `DATA_KNOWN`, `source_owned = 1`, `DATA`, artifact `data/audio_0BD540.bin`.
3. **Canonical Claims and Proof Evidence**:
   - Create `rom_object` entries (`AUDIO_DATA`), `claim` entries (`SOURCE_OWNED = true, STATIC_VERIFIED`, `SOURCE_CLASS`, `FORMAT_SPEC`), and `evidence_ref` entries linking W5 acceptance proofs (`w5_audio_format_spec.json`, `w5_audio_resources.json`, `w5_roundtrip_receipt.json`, `w5_witness_alignment.json`).
4. **Idempotence and Non-Duplication**:
   - Re-running promotion on an already-promoted map produces `NEW_SOURCE_OWNED_BYTES = 0` and leaves hashes and partitions byte-for-byte identical.
5. **Strict Scope Prohibition**:
   - Descriptor tables (`0x0B8000..0x0B8050`), bank padding (`0x0BF768..0x0C0000`), Mode 1 resources, unverified neighbouring candidate resources, and whole audio banks remain strictly unpromoted as `UNKNOWN` or pre-existing padding until covered by independent reconstruction proof.
6. **SOURCE_OWNED Accounting**:
   - Increment `SOURCE_OWNED` from `1,475,600` to `1,487,388` bytes (`+11,788` bytes, 47.2827911377% of ROM).

**Consequences:**
1. Canonical knowledge map `docs/reports/THOR_M12_CANONICAL_ROM_KNOWLEDGE_MAP_2D.json` and its report are regenerated with updated hashes and metrics.
2. Partition coverage remains complete across all 3,145,728 bytes with 0 gaps, 0 overlaps, and 0 out-of-bounds rows.
3. Checkpoint emitted: `PASS_EXACT_AUDIO_RESOURCE_OWNERSHIP_V1`.

# ADR-M12-W5-EXACT-AUDIO-RESOURCE-FORMAT-ROUNDTRIP-V1 — Non-Linear Delta-PCM (Format A), Bank Descriptor Tables, Canonical IR, and Byte-Identical Round-Trip
**Status:** Accepted for M12 W5
**Date:** 2026-09-21

**Context:**
Following the W4 Level-3 causal witness from early-boot banked ROM reads (`0x0BEDE4..0x0BEDED`) to YM2612 DAC writes (`0x4001`), the exact audio resource format, true resource boundaries, decoding logic, encoding logic, and hardware alignment must be proven end-to-end without guesswork or promotional semantics.

**Decision:**
1. **Exact Consumer & Decoder Routines**:
   - The Z80 sound driver is located at ROM `0x062E38..0x064E38` (copied to Z80 RAM `0x0000..0x1FFF` during M68K boot).
   - Sound command `0x11` at `0x064F` invokes descriptor parser and bank switcher at `0x0704..0x076D`.
   - Bank switching writes 9 bits sequentially into Genesis bank latch `0x6000` via routine `0x0A0B..0x0A40`.
   - Mode 0 sample loop resides at `0x080D` (high nibble) and `0x0854` (low nibble), with loop advancement at `0x0880` (`INC HL; DEC BC`) and termination check (`OR B, C; JP Z, 0x0892`).
   - Hardware DAC emitter resides at `0x0968..0x0980`, writing to YM2612 port 0 register 0x2A (`(IY+0)=0x2A; ADD A, D; LD D, A; LD (IY+1), A`).
2. **Exact Descriptor Table Layout**:
   - Each of the 8 32KB audio banks (ROM `0x080000..0x0B8000`) begins with an 80-byte table (`0x8000..0x804F`) containing 16 5-byte entries.
   - Entry structure: bytes 0..1 = 16-bit logical start address (`0x8000..0xFFFF`); bytes 2..3 = 16-bit compressed byte length; byte 4 = playback mode (0 = standard, 1 = hold/interpolate via `0x0999`).
   - Compressed audio data starts immediately following the table at offset `0x8050` and packs contiguously until the end of the last resource, followed by `0xFF` bank padding.
3. **Format Semantics (AUDIO_FORMAT_A)**:
   - Initial accumulator `D = 0x80` (128 decimal, unsigned 8-bit PCM midpoint / silence).
   - Lookup table at Z80 RAM `0x0008..0x0016`:
     `[0, +1, +2, +6, +12, +24, +48, +96, -96, -48, -24, -12, -6, -2, -1]`.
   - Nibble 0: repeat previous delta `E` for 3 consecutive sample ticks.
   - Non-zero nibbles: load delta from table, add to accumulator `D`, emit sample.
4. **Exact Boundaries for Primary Resource**:
   - `AUDIO_RESOURCE_FORMAT_A_0001`: Bank 0x17, Entry 7 (`0x0BD540..0x0BF768`, 8,744 bytes, mode 0).
   - Strictly encloses W4 witness cluster `0x0BEDE4..0x0BEDED` at byte offset 6,308.
   - Decoded sample at byte offset 6,308 low nibble matches observed W4 DAC write `0x78` exactly.
5. **Canonical IR & Byte-Identical Round-Trip**:
   - Pure Intermediate Representation (`AudioResourceIR` / `AudioToken`) represents tokenized delta events and decoded PCM samples without storing original binary blobs.
   - Deterministic encoder reconstructs original byte stream with 100% byte-identical SHA-256 match for primary resource (`AUDIO_RESOURCE_FORMAT_A_0001`), secondary resource (`AUDIO_RESOURCE_FORMAT_A_0002`), and across multiple audio banks.
6. **SOURCE_OWNED Reconciliation**:
   - All Gates A through J verified green.
   - Manifest promotion delta deferred (`SOURCE_OWNED_DELTA = 0`) to preserve ADR-0044 manifest invariants until a full audio bank re-partitioning manifest is formally authorized.
   - Checkpoint emitted: `PASS_EXACT_AUDIO_RESOURCE_ROUNDTRIP_V1`.

**Consequences:**
1. Complete deterministic codec implemented in `src/tools/thor_evidence/w5_audio_decode.py` and `w5_audio_encode.py`.
2. All 9 required W5 acceptance artifacts emitted in `build/m12-w5-acceptance/`.
3. 18 comprehensive tests pass in `tests/w5_audio_format_test.py`.
4. Zero promotional or musical names used.

# ADR-M12-W4-ACTIVE-AUDIO-RESOURCE-ANALYSIS-V1 — Four Causal Levels, Last-Writer Memory Handoff, Port-Local Audio Sinks, and Neutral Audio Resource Classification
**Status:** Accepted for M12 W4
**Date:** 2026-09-21

**Context:**
Connecting M68K sound commands to Z80 sound driver execution, Z80 RAM state, and physical audio outputs (YM2612 FM synthesis and PSG) requires strict evidentiary separation between temporal coincidence, memory dependencies, instruction dataflow, and strict end-to-end causal chains. Hardware audio interfaces require port-local latching semantics (YM2612 Part 1: 0x4000->0x4001; Part 2: 0x4002->0x4003) and exact hardware register recognition (DAC data 0x2A, enable 0x2B; PSG 0x7F11). Candidate audio resources in canonical ROM must be classified strictly with neutral taxonomy (AUDIO_COMMAND_CANDIDATE, AUDIO_TABLE_CANDIDATE, AUDIO_PATCH_CANDIDATE, AUDIO_RESOURCE_RANGE_CANDIDATE) without premature promotional music semantics (SONG, TRACK, INSTRUMENT).

**Decision:**
1. **Four Causal Levels**:
   - Level 0 (`TEMPORAL_ASSOCIATION`): Temporal ordering ($T_1 < T_2$) within same execution context `(run_id, epoch)`. Necessary but not sufficient for causality.
   - Level 1 (`MEMORY_DEPENDENCY`): Exact last-writer rule. M68K writes to Z80 window ($A00000..$A01FFF), Z80 reads matching value at $T_{read} > T_{write}$, and no intervening write occurs. Yields `OBSERVED_HANDOFF` (`DERIVED_EXACT`).
   - Level 2 (`DATAFLOW_DEPENDENCY`): Exact Z80 instruction dataflow tracking across registers (A, B, C, D, E, H, L, IX, IY), memory, stack, and shadow registers. Unsupported opcodes terminate tracking conservatively without speculation.
   - Level 3 (`STRICT_CAUSAL_CHAIN`): Provenance chain connecting verified source (M68K handoff or banked ROM read) to hardware audio sink through unbroken Level 2 dataflow (`DERIVED_EXACT`).
2. **Port-Local YM2612 Latching Semantics**:
   - Part 1: Port 0x4000 latches register address for Port 0x4001.
   - Part 2: Port 0x4002 latches register address for Port 0x4003.
   - Cross-port pairing (e.g. 0x4000 pairing with 0x4003) is strictly prohibited. An address write overwrites any previous unconsumed latch on that port.
   - YM2612 register 0x2A is DAC data, 0x2B is DAC enable. Port 0x7F11 is PSG.
3. **Input Evidence Authenticity**:
   - Evidence records must exist in raw binary input files processed by the active pipeline run to be classified as `OBSERVED`. Historical records or prior run references absent from the active binary stream remain `WITNESS_CANDIDATE` (`HYPOTHESIS`).
4. **Spatial Cluster Span vs. Resource Boundary**:
   - Spatial span `[min_addr..max_addr]` of observed ROM reads defines an `OBSERVED_ROM_CLUSTER`, not an exact resource boundary. Resource ranges remain `HYPOTHESIS` until bounded by static or runtime closure proofs.
5. **Neutral Resource Taxonomy**:
   - All candidate audio structures must use strictly neutral types (`AUDIO_COMMAND_CANDIDATE`, `AUDIO_TABLE_CANDIDATE`, `AUDIO_PATCH_CANDIDATE`, `AUDIO_RESOURCE_RANGE_CANDIDATE`). Promotional labels (`SONG`, `TRACK`, `INSTRUMENT`, `VOICE`, `SAMPLE`) are strictly forbidden until behavioral parity is established.

**Consequences:**
1. Analysis pipeline (`w4_audio_pipeline.py`) emits 9 verified acceptance artifacts under `build/m12-w4-acceptance/`.
2. Over 7,168 M68K->Z80 handoffs verified with exact last-writer matching and 0 intervening writes.
3. 2,048 YM2612 register writes reach Level 3 Strict Causal Chain with exact Z80 dataflow provenance.
4. Zero promotional or unverified music semantics introduced into the repository.

# ADR-M12-W3-CANONICAL-CROSS-CPU-TIMELINE-V1 — Canonical Cross-CPU Temporal Identity and Causal Closure
**Status:** Accepted for M12 W3
**Date:** 2026-09-21

**Context:**
Dual-CPU co-capture records concurrent execution of M68000 and Z80 processors.
To guarantee sound, non-speculative cross-CPU timeline analysis and causal relations
(such as 68K writes to Z80 RAM followed by Z80 reads and YM2612 audio register writes),
the exact semantics, source, units, frame reset behavior, and tie-breaking rules of
`master_time` across both CPU cores must be mathematically verified and strictly enforced.

**Decision:**
1. **Master Time Source and Units**:
   - `m68k.cycles` (`core/m68k/m68kcpu.c`) scales instruction cycles by `MUL = 7` (Genesis Master Clock / 7 = 7.67 MHz M68K clock).
   - `Z80.cycles` (`core/z80/z80.c`) scales instruction cycles by `15` in `cc_op` tables (Genesis Master Clock / 15 = 3.58 MHz Z80 clock).
   - Both cycle counters are directly expressed in the exact same physical unit: **Genesis Master Clock cycles** (3420 mcycles per scanline, 896,040 mcycles per standard NTSC frame). The conversion ratio is 1:1.
2. **Temporal Semantics and Canonical Identity**:
   - At every frame boundary, `core/system.c` executes `m68k.cycles -= mcycles_vdp` and `Z80.cycles -= mcycles_vdp`.
   - Therefore, `master_time` is strictly `FRAME_RELATIVE` (rebasing to ~0 each frame).
   - The canonical timeline key is defined as `(run_id, epoch, frame, master_time)`. Comparing `master_time` alone across different frames is strictly prohibited.
3. **Cross-CPU Tie-Breaking Rule**:
   - If M68K and Z80 share `(frame, master_time)`, the relation is defined as `SAME_MASTER_TIME_UNORDERED`.
   - `stream_sequence` reflects host thread scheduling slices and CANNOT be used to prove causal precedence between same-timestamp events.
   - Causal interactions require `STRICT_BEFORE` temporal precedence (`T_write < T_read`).
4. **Timeline Integrity and Isolation**:
   - Cross-run and cross-epoch events are strictly disjoint; joining or comparing events across differing `run_id` or `epoch` raises an error.
5. **Retained Segment Metric**:
   - `retained_segment_data` is defined as `CUMULATIVE_BYTES_PROCESSED` (cumulative audit throughput streamed to disk), not resident live memory. Live heap remains strictly bounded at 160 MiB.

**Consequences:**
1. Authoritative cross-CPU sorting in `w3_cross_cpu_timeline.py` respects frame precedence first, followed by master cycles.
2. Tests A–G pass with 100% verification, preventing speculative causality, timestamp collisions, and cross-run contamination.
3. Memory behavior remains proven `BOUNDED_OSCILLATING` under sustained gameplay.

# ADR-M12-W3-SUSTAINED-10MIN-PROFILE-V1 — 10-Minute Sustained Gameplay Memory Stability and Safety Verification
**Status:** Accepted for M12 W3
**Date:** 2026-09-21

**Context:**
Following the configuration of the 2,097,152-slot default ring buffer (128 MiB slot storage, 96 MiB raw),
it was necessary to prove that the default ring capacity is safe under prolonged, real gameplay emulation
rather than brief, open-and-immediately-close acceptance campaigns. The system was subjected to a sustained
Beyond Oasis emulation session for at least 10 minutes with 128 Workers and depth 100, sampling process
working set, private bytes, native heap, ring allocation, worker counts, and record throughput every 5 seconds.

**Decision:**
1. Maintain the startup default ring capacity at 2,097,152 slots.
2. Formally evaluate memory behavior using linear regression slope over sustained emulation (minutes 1 to 10).
3. Require 0 retention failures, 0 dropped captures, 0 identity collisions, 0 stale ACKs, and 0 runtime errors
   under full 128-Worker continuous turnover.
4. Verify graceful END GAME shutdown via IPC and confirm post-cleanup return to steady-state.

**Consequences:**
1. Under 610.5 seconds of continuous Beyond Oasis gameplay (283,648 audited segments, 828,506,454 records produced,
   395 ring wraps), Private Bytes remained strictly bounded between 301.35 MB and 306.72 MB with a slope of
   -0.3350 MB/min, and Working Set remained bounded between 466.85 MB and 470.18 MB with a slope of +0.2237 MB/min.
2. Memory behavior was classified as `BOUNDED_OSCILLATING`, proving the absence of memory leaks, unbounded queues,
   or growth runaway.
3. Retention failures remained exactly 0 throughout all 395 wraps.
4. Upon triggering END GAME, final analysis completed without spikes (peak Private Bytes 303.93 MB), and the emulator
   process terminated cleanly with code 0, releasing all resources.
5. The 2,097,152-slot default capacity is verified as safe, robust, and permanent for the W3 architecture.

# ADR-M12-W3-LARGE-BOUNDED-RING-ACCEPTANCE-V1 — Large Bounded Ring and Final W3 Acceptance
**Status:** Accepted for M12 W3
**Date:** 2026-09-21

**Context:**
Dual-CPU co-capture (M68K + Z80) at scale increases stream record throughput.
Under 128 active workers and deep chains (depth 100, 512, 1000), the initial 16,384-slot
transport ring required frequent wraps (1,783 wraps per 100 rounds at depth 20).
To ensure robust retention without dynamic reallocation or converting the ring into an
unbounded log, a bounded large ring with constant-time hot-path indexing was needed.

**Decision:**
1. Set the default startup ring capacity to 2,097,152 slots (48-byte records, 96 MiB raw,
   128 MiB slot storage). Keep capacity startup-configurable via `oasis_lf_set_ring_capacity()`
   and command-line flag `--ring-capacity`.
2. Prohibit dynamic resizing during emulation: the ring remains strictly bounded cyclic transport.
3. Optimize hot-path record append with power-of-two bitmask indexing:
   `(sequence - 1u) & (cap - 1u)` replaces division/modulo operations on the hot path.
4. Scale worker result buffers with capture depth: 256 KiB for depth 100, 512 KiB for depth 512,
   and 1024 KiB for depth 1000, ensuring `depth_limit_endings` are achieved without premature
   buffer exhaustion.
5. Invariant preservation: ring expansion does NOT alter Worker admission or capture-depth semantics.
   Cross-CPU ordering strictly uses `(run_id, epoch, frame, master_time)`.

**Consequences:**
1. Ring turnover dropped by 137x (from 1,783 wraps to 13 wraps per 100 rounds at depth 20).
2. Official acceptance at 128 Workers x Depth 100 passed 12,800/12,800 segments with 0 retention
   failures and 0 overwrites.
3. Deep-chain stress tests at Depth 512 (82.1M records) and Depth 1000 (138.2M records) passed
   with 100% depth-limit completion, 0 retention failures, and 0 invalid captures.
4. Frame timing under dual-CPU recording remained within normal emulation latency (p50 = 16-17 ms).

# ADR-M12-W3-Z80-COCAPTURE-CROSS-CPU-TIMELINE-V1 — Z80 Co-Capture and Cross-CPU Timeline
**Status:** Accepted for M12 W3
**Date:** 2026-09-21

**Context:**
M12 Workers had M68K instruction and bus capture (W1), resource classification (W2),
and frame coherence (W2.1), but lacked Z80 co-capture. Cross-CPU interactions
(e.g., M68K writing to Z80 RAM, Z80 reading and writing to YM2612/PSG) could not
be ordered on a common timeline, and Z80 banked ROM accesses could not resolve
to physical addresses without external context. The 32-byte record was insufficient
to losslessly represent common emulated time and multi-byte Z80 instructions.

**Decision:**
1. Upgrade `oasis_lf_record` to 48 bytes with explicit lossless fields:
   `stream_sequence`, `instruction_sequence`, `master_time`, `pc`, `address`,
   `value`, `kind_flags`, `cpu_id`, `length_or_width`, `domain`, `reserved`,
   and `auxiliary`.
2. Keep M68K and Z80 `instruction_sequence` strictly independent monotonic counters.
3. Use frame-relative master cycles (`master_time`) as the authoritative common
   timing basis across M68K and Z80.
4. Make ring capacity configurable at initialization (default 16,384 slots).
5. Capture ordered `BANK_REGISTER_CHANGE` events and resolve physical Genesis/ROM
   addresses directly into the record `auxiliary` field for all banked ROM reads.
6. Prohibit higher-level audio semantics (no SONG, TRACK, INSTRUMENT, VOICE,
   SAMPLE, NOTE, MUSIC_RESOURCE).
7. Validate cross-CPU causal relations only under strict temporal precedence
   (T1 < T2 < T3) and matching addresses/values without speculative inference.

**Consequences:**
Native trace hooks cleanly capture both M68K and Z80 instruction and bus events.
Cross-CPU timeline sorting by `master_time` produces deterministic temporal ordering.
The C test suite (`oasis_live_forward_z80_test`) covers all 22 required test cases
A–V. Python tools provide lossless decoding, bank tracking, physical address
resolution, timeline construction, and primitive event classification.

# ADR-M12-R11-EVENT-DRIVEN-SUBPROCESS-WAIT-1 — Event-driven Stage 7 waits
**Status:** Accepted for M12 R11
**Date:** 2026-09-20

**Context:** Stage 7 and Stage 8 external tools were launched through a
bounded runner that checked `Popen.poll()` only after a 250 ms sleep. The
retained R10 workload showed 400 external decoder invocations, so the polling
quantization was material to short child processes even though decoder
semantics were correct.

**Decision:** Replace process-completion polling with a blocking
`Popen.wait()` waiter thread and completion event. Keep dedicated stdout and
stderr drain threads, bounded joins, the existing timeout classifications,
and terminate-then-kill shutdown. A separate heartbeat thread may publish
progress but cannot determine child completion; callback failures are returned
to the caller after bounded cleanup.

**Consequences:** Fast children return at process-exit latency instead of a
250 ms quantum. The Stage 7 worker pool, candidate ordering, proof,
promotion, decoder contract, and temporary-output boundaries are unchanged.
The checkout exposes no public cancellation API, so no cancellation contract
was altered. The same sealed input produced identical request, unsupported,
promotion, and SOURCE_OWNED results while reducing Stage 7 wall time.

**Evidence:** `src/tools/thor_evidence/stage7_subprocess.py`,
`tests/stage7_subprocess_test.py`, the R10 sealed receipt, and the three R11
same-input optimized receipts under `build/thor-evidence/r11-event-wait-stage7`.

# ADR-M12-R5-IN-PROCESS-POSTRUN-COORDINATOR-1 — In-process Stage 1–9
**Status:** Accepted for M12 R5
**Date:** 2026-09-19

**Context:** R4 established ROM + MASTER V2 as the fail-closed startup
authority, but post-run still crossed a runtime/subprocess/status-file boundary.

**Decision:** Add `PostRunCoordinator` with explicit `PostRunContext`,
`PostRunResult`, and in-memory stage events. Normal Stage 1–9 execution runs in
one non-UI background thread owned by the runtime process. JSON status/report files
are derived diagnostics only; the legacy subprocess launcher remains available
for shadow/regression comparison. Disk-backed FLOW and Stage 5/6/7/8 truth
semantics are unchanged. Atomic diagnostic snapshots retry transient Windows
reader locks.

**Consequences:** Closing the analysis window cannot terminate coordinator work;
UI consumers may resubscribe to the event stream. Stage 7/8 external tools remain
bounded subprocesses. A replay of an already-absorbed run remains fail-closed
with `STOP_KNOWLEDGE_IMPORT_NONIDEMPOTENT`.

**Evidence:** `tools/bizhawk-native-ring/live_forward_postrun_coordinator.py`,
`tests/live_forward_postrun_coordinator_test.py`, and fresh runtime campaign
`build/thor-evidence/live-worker-control-2h/campaign-desktop-20260919-231221-978`.

# ADR-M12-R4-MASTER-STARTUP-AUTHORITY-1 — MASTER-only initialization
**Status:** Accepted for M12 R4
**Date:** 2026-09-19

**Context:** R3 made MASTER V2 the read authority for canonical, provenance and
outcome state, but normal startup still had no explicit fail-closed contract.

**Decision:** Add `master_startup_authority.py` as the sole startup validator.
It resolves one atomic pointer, verifies the MASTER file/logical/section hashes,
ROM identity, lineage, canonical emission partition, SOURCE_OWNED, provenance,
outcomes and absorption history, then exposes the existing canonical and outcome
views in memory. Legacy state is never opened on the production path. The
`--legacy-shadow-compare` flag is the explicit comparison boundary. Missing or
invalid authority stops with `STOP_MASTER_STARTUP_UNAVAILABLE` or
`STOP_MASTER_STARTUP_INTEGRITY` and never falls back to legacy files.

**Consequences:** A MASTER-only environment can initialize and prepare a
deterministic N+1 candidate descriptor without legacy persistent files. The
current runtime transport and Stage 5–9 semantics remain unchanged; deletion of
legacy artifacts is deferred.

**Evidence:** `build/thor-evidence/master-v2/current.json`,
`build/thor-evidence/master-v2/r4-startup-report.json`, and
`tests/master_startup_authority_test.py`.

# ADR-AUTO67-CARTOGRAPHER-SEAM-1 — Worker local chain to MAP-1
**Status:** Accepted for AUTO67
**Date:** 2026-09-15

**Context:** Clean AUTO67 Worker materialization produced bounded evidence but
was written only to the legacy live-chain sidecar. MAP-1 already owns stable
provenance graph identity and merge authority.

**Decision:** Add one downstream local-chain adapter and proof gate. Accept all
valid predecessor-produced `REGISTER_REACHING_DEFINITION` steps, construct
stable `ROM_INSTRUCTION` nodes and register-specific edges, and call the
existing Cartographer merge. Keep one bounded `LiveMapSink` queue/writer.
Cartographer is created, used and closed by that writer thread; snapshots
return cached graph metrics. Runtime occurrence, lease, frame and worker IDs
remain local evidence and never define global graph identity.

**Consequences:** Worker never sees map state or novelty results. Exact stable
replays and different occurrences share an import identity; incomplete steps do
not discard valid siblings and no-proof chains are not counted as map drops.
`--map-db` enables durable map output; without it the Worker still produces
local diagnostics and no output queue. Historical descriptor persistence is
isolated for offline AUTO67.3/AUTO67.4 consumers. No frontiers, scheduler or
upstream capture changes are introduced.

**Evidence:** `docs/reports/THOR_M12_AUTO67_CARTOGRAPHER_SEAM_1.md`.
# ADR-AUTO67-RAM-SESSION-MAP-1 — RAM session map and offline GLOBAL merge
**Status:** Accepted for AUTO67
**Date:** 2026-09-15

**Decision:** Keep the canonical GLOBAL SQLite unopened during runtime. The
existing bounded queue and single writer use an in-memory SQLite Cartographer;
clean shutdown backs it up to a retained session file. A deterministic offline
merge imports that session into a temporary GLOBAL copy under
`session-map:<session_graph_hash>`, validates, fsyncs and atomically replaces
GLOBAL. Replace or validation failure preserves the original GLOBAL bytes.

**Consequences:** Runtime map writes cannot stall on GLOBAL I/O, repeated
session imports are idempotent, and shutdown remains bounded by the existing
writer join and Dispatcher stop event. No new queue, capture path or Cartographer
identity is introduced.

**Evidence:** `docs/reports/THOR_M12_AUTO67_RAM_SESSION_MAP_1.md`.

# ADR-AUTO67-MAILBOX-CLEAN-1 — minimal Dispatcher-to-Worker lease message
**Status:** Accepted for AUTO67
**Date:** 2026-09-15

**Context:** The occurrence-only Dispatcher still copied branch, context,
seed, worker and duplicate lease identity into each mailbox task. Those fields
were not consumed by the Worker boundary and made the lease contract appear
to carry scheduling semantics.

**Decision:** Keep one detached `event` copy, one occurrence/window-derived
`investigation_id`, one `lease_id`, optional capsule/capture fields and the
bounded dispatch trace. The Worker consumes those IDs directly; CapsulePool
metadata and persistence retain their factual worker/lease provenance. Remove
mailbox branch/context/seed/worker/occurrence aliases and legacy profiler names
without changing selection order, capsule limits, or shutdown mechanisms.

**Consequences:** Final-only and same-branch occurrences remain independently
leasable, capsule identity mismatches fail closed, and shutdown can release a
worker waiting on a stopped emulator through the existing stop event. No queue
or backlog is introduced.

**Evidence:** `docs/reports/THOR_M12_AUTO67_MAILBOX_CLEAN_1.md`.

# ADR-0065 — chain-primary bounded AUTO65 campaign
**Status:** Accepted for AUTO65
**Date:** 2026-09-13

**Context:** AUTO64 reaches a useful fixed point with one unresolved upstream
queue. Repeating its QuickSave1 address-focused capture would not scale and
would violate the M12 evidence boundary.

**Decision:** Use one scheduler-selected, materially different bounded
gameplay scenario for a cheap broad discovery. Normalize each observed chain
into typed nodes and edges, compare structure before fingerprints, reuse known
prefixes, cluster shared dependencies, and batch investigations. Static
evidence precedes focused runtime; anti-repeat fingerprints reject equivalent
captures. A second encounter is persisted as `KNOWN_NEW_INSTANCE` and is
cancelled. Closure remains fail-closed and ownership promotion remains under
the existing M12 contracts.

**Consequences:** AUTO65 can support 100+ pending chains and process smaller
batches without becoming a serial address queue. A single broad capture can
serve many investigations, while unresolved activity remains an explicit
frontier. The campaign remains developer-only and non-owning.

**Evidence:** `docs/reports/THOR_M12_AUTO65_MULTI_CHAIN_CAMPAIGN.md`.

# ADR-0042 — Keep 0x062AE0 natural evidence developer-only
**Status:** Accepted for M11.61
**Date:** 2026-09-10

**Context:** The natural callee crossed by M11.60 G0 preserves A5 exactly and
has a complete observed safe-RAM/G0-relative/stack effect set. Its static CFG
also contains an unresolved indexed JSR at `0x062CEC` and multiple latent RTS
exits.

**Decision:** Record only the natural preservation/effect contract with a
developer-only observer and regression test. Classify the static boundary as
`INDIRECT_CFG`; retain the M11.60 parent-lifetime transaction gate and typed
data block. Do not recurse into a subsystem or add a production abstraction.

**Consequences:** One G0 callee dependency is reduced to an exact natural
fact. The latent indirect path and other G0 callees remain explicit blockers.

**Evidence:** `docs/reports/CALLEE_062AE0_CONTRACT_M11_61.md`.

# ADR-0041 — Keep the 0x060182 A5 lifetime parent-owned
**Status:** Accepted for M11.60
**Date:** 2026-09-10

**Context:** The natural generation written by `0x060182 LEA FF001A,A5` has a
bounded CFG and exact +0/+4 consumers, while the +7 arm is dead. Its endpoint
is the parent's `0x06027E MOVEM` restore. Calls crossed while G0 is live do not
yet have complete whole-callee effect proofs, and M11.59 retains broader raw
aliases and external writers.

**Decision:** Record the generation with a developer-only hybrid observer and
decoder regression test. Classify it as `A5_LIFETIME_MERGES_WITH_PARENT` and
keep the transaction and typed-data gates fail-closed. Do not add a type,
subsystem, production helper, or native routine.

**Consequences:** The exact consumer/lifetime evidence is reusable for one
future callee-preservation closure. Parent frame, GPGX, timing, ROM-PC and
raw-storage ownership remain outside `oasis_core`.

**Evidence:** `docs/reports/A5_CONSUMER_LIFETIME_M11_60.md`.

# ADR-0040 — Preserve raw ownership boundary after M11.59 census
**Status:** Accepted for M11.59
**Date:** 2026-09-10

**Context:** M11.58 left the typed-data gate open because fixed bytes had
external writers and the A5-derived range had unresolved alias/lifetime
evidence. M11.59 found three A5 materializers; one path saves and reloads A5
and post-increments it through a broad range, while other routines consume
multiple derived offsets. The sibling 0x60BCC writers also retain a hardware
prefix.

**Decision:** Keep `ParentSuffix + RamFlag` raw and parent-parameterized. Do
not add a typed structure, shared-memory owner, subsystem wrapper or
0x60BCC promotion. Permit only a standalone decoder provenance regression
test; keep ROM/GPGX, hardware ordering, lifetime and continuation evidence in
developer-only tooling and the parent adapter.

**Consequences:** The aliasing boundary is proven negative for typed
replacement while the behavior-cluster contract remains valid. M11.60 must
close one bounded A5 consumer/lifetime contract before revisiting typed data.

**Evidence:** `docs/reports/RAW_DATA_OWNERSHIP_M11_59.md`.

# ADR-0039 — Keep the first behavior cluster raw and parent-parameterized
**Status:** Accepted for M11.58
**Date:** 2026-09-10

**Context:** M11.57 proved a parent-owned suffix helper that composes the
portable RamFlagRoutine. M11.58 audited its exact raw footprint and bounded
ROM/runtime provenance. The fixed `FF0010..FF0014` bytes have known external
writers, the flag addresses are used by other bounded code, and the
`FF001A + 5..7` derived range has unresolved alias and lifetime boundaries.

**Decision:** Treat the existing ParentSuffix + RamFlag composition as the
portable behavior-cluster contract, retaining raw address parameters and each
component's independent continuation tokens. Do not add a typed shared-memory
structure, opaque replacement wrapper, subsystem owner or new routine. Keep
parent frame/SR/hardware/epilogue/RTS ownership and all ROM/GPGX provenance in
the parent or developer-only hybrid adapter.

**Consequences:** The cluster can be tested and shadow/native-proven without
inventing gameplay meaning or hiding unresolved ownership. M11.59 must close
one dominant alias/lifetime and external-writer blocker before any typed data
replacement is considered. `PORTABLE_SUBSYSTEM_BOUNDARY_PROVEN` remains false.

**Evidence:** `docs/reports/PORTABLE_BEHAVIOR_CLUSTER_M11_58.md`.

# ADR-0031 — Portable mechanical primitive layer in `oasis_core`
**Status:** Accepted for M11.50
**Date:** 2026-09-09

**Context:** M11.49 proved one generic resumable executor for four exact
copy/clear plus DBF loops, but its implementation still lived in the
developer-only hybrid library. The semantics were reusable; the registry,
canonical bytes, GPGX bridge, shadow comparator and evidence were not.

**Decision:** Move only the generic mechanical contract and executor into
`oasis_core`. Use opaque instruction tokens, a portable machine interface and
explicit continuation state. Keep ROM PCs, candidate names, canonical opcodes
and displacement, GPGX/BasicBlock timing/prefetch adapters, shadow snapshots,
metrics and reporting in `tools/hybrid`. Add a standalone core test and a
repository-visible dependency-boundary check. Do not add new primitive forms,
gameplay semantics or hardware modeling.

**Consequences:** There is one implementation of copy/clear/DBF semantics and
it is independently linkable without GPGX/libretro. The hybrid adapter remains
responsible for hardware/bus mapping and canonical provenance. The extracted
layer preserves the M11.49 shadow/native identity; future native routines may
compose these operations only after separate evidence closes their contracts.

**Affected files/milestones:** `src/core/mechanical_primitive.*`, hybrid
adapter/registry, standalone core and boundary tests, CMake and M11.50 report.

# ADR-0030 — Metadata-driven mechanical primitive family
**Status:** Accepted for M11.49 developer-only hybrid tooling
**Date:** 2026-09-09

**Context:** M11.48 proved one resumable `CLR.B`/`DBF` loop. M11.47 also
identified two byte copy loops and one word clear loop, but the family
abstraction, exact source/destination and width contracts, and coexistence
behavior had not been proven.

**Decision:** Generalize the architecture-neutral primitive executor around a
metadata-only `MechanicalLoopContract`. Promote only the exact two
`MOVE.B (A2)+,(A1)+` plus DBF forms and `CLR.W (A0)+` plus DBF form that pass
deterministic vectors, generated-oracle shadow, existing bus/timing/refresh
contracts and unchanged native identity. Keep ROM PCs in registry metadata,
keep generated code as oracle/fallback, use explicit ordered byte copy and
width-specific clear writes, and fail closed on unsupported forms. Do not
broaden hardware emulation or add a production dependency.

**Consequences:** Four mechanical loops share one reusable implementation and
preserve exact 32-bit address, DBF, CCR/X, bus ordering and interruption
behavior. Native execution represents 42,384 guest instructions while the
generated-plus-mechanical translated-equivalent count remains 6,241,765;
checkpoint/video, yields, resumptions and CPU equivalence remain unchanged.
The promoted family has no hardware-visible access; the existing hardware
boundary remains unchanged. Extraction is a future boundary, not part of M11.49.

**Affected files/milestones:** `mechanical_primitive.*`, runner report,
synthetic primitive test, M11.49 governance and evidence report.

# ADR-0029 — Resumable native mechanical primitive layer
**Status:** Accepted for M11.48 developer-only hybrid tooling
**Date:** 2026-09-09

**Context:** M11.47 proved repeated safe-memory copy/clear structures but did
not define a higher-level replacement contract. The first replacement must
preserve instruction-boundary timing, refresh, event/interrupt behavior and
continuation state, while remaining independent of GPGX internals.

**Decision:** Add a small architecture-neutral `MechanicalMachine` interface
and resumable primitive registry in `src/tools/hybrid/mechanical_primitive.*`.
Promote only the exact `CLR.B (A5)+` plus `DBF D0` loop at `0x061266`/`0x061268`.
Dispatch metadata identifies the proven ROM loop; the implementation executes
one guest body/DBF pair at a time and fails closed on canonical-byte or bridge
contract mismatch. Keep generated blocks and generic basic-block glue separate;
retain the generated path as the shadow oracle and fallback for all other PCs.
Do not add gameplay meaning, hardware emulation or a production dependency.

**Consequences:** The native path represents 3,780 guest instructions through
1,890 resumable clear iterations and preserves 86 synthetic/observed
mid-operation yields in shadow evidence. The unchanged 600-frame proof retains
checkpoint/video/CPU identity, 150,135 total yields, 288 resumptions and zero
fallback/hardware accesses. The three other discovered loops remain unpromoted.

**Affected files/milestones:** M11.48 mechanical primitive API, runner/CMake,
synthetic primitive test, governance and evidence report.

# ADR-0028 — Bounded safe-memory semantic tranche and mechanical primitives
**Status:** Accepted for M11.47 developer-only hybrid tooling
**Date:** 2026-09-09

**Context:** M11.46 resolved runtime memory classes for a bounded set of
interpreter PCs. Seven high-payoff rows were observed only in safe main RAM,
but their exact instruction semantics and bus effects still required proof.
Repeated copy/clear loops also appeared mechanically structured, while their
native higher-level replacement contract was not yet needed.

**Decision:** Independently verify only the exact seven forms required by the
M11.47 safe-memory tranche, emit them through the decoder-owned mechanical
generator and shared runtime helpers, and certify each through the existing
instruction-boundary GPGX shadow and unchanged 600-frame native gates. Keep
hardware-reachable, mixed, unresolved, indirect-CFG and decoder rows
fail-closed. Record copy/clear structures as mechanical future replacement
candidates only; do not replace loops or assign gameplay semantics in M11.47.

**Consequences:** Seven generated ranges remove exactly 42,047 interpreter
executions, raising translated execution to 96.1933% while preserving the
authoritative checkpoint/video identity, bus/timing/event contract and zero
hardware-visible native accesses. The remaining ledger closes at 247,008
executions. Generated bodies, registry metadata, shared helpers and handwritten
boundary glue remain separate.

**Affected files/milestones:** M11.47 hybrid generator/runtime, generated
block registry, semantic regression test and M11.47 reports.

# ADR-0027 — Observe runtime addresses without promotion
**Status:** Accepted for M11.46 developer-only hybrid tooling
**Date:** 2026-09-09

**Context:** M11.45 left 149,678 interpreter executions in 635
`UNKNOWN_WITH_EVIDENCE` register-address PCs. Static decoding could not prove
their runtime memory classes, while the existing GPGX path already exposes
instruction and top-level data-bus boundaries.

**Decision:** Add a separate observer mode around the existing GPGX hook and
M11.45 block registry. Record PC counts, data-bus address/width/direction/order
and A-register transitions, and classify only observed addresses using the
existing Genesis address contract. Keep mapper-dependent cartridge SRAM
unproven when address alone is insufficient. Do not promote semantics, broaden
hardware emulation, add address-specific execution bodies or connect the
observer to production runtime.

**Consequences:** Three independent processes matched the M11.45 identity and
metrics. Runtime classes were resolved for 147,847 executions; 1,831
address-computation-only executions remain exact-evidence unresolved. The result
is `BOUNDED_RUNTIME_ADDRESS_PROVENANCE_PROVEN`; M11.45 history and all prior
negative gates remain intact.

# Architecture Decision Log

Use this file for decisions that can redirect architecture, dependencies, scope, or reverse-engineering strategy.

## ADR-0026 — Bounded semantic closure with independent shadow vetoes
**Status:** Accepted for M11.45 developer-only hybrid tooling
**Date:** 2026-09-09

**Context:** M11.44 left 560,968 interpreter executions. The highest-payoff
decoder-backed semantic/proven candidates could exceed the 236,530 execution
threshold needed for a 95% result, but exact GPGX timing and flags remained
unproven for new forms.

**Decision:** Select only decoder-owned rows with proven static memory class and
exact mechanical support, excluding register-based/other memory, hardware,
indirect CFG, decoder gaps and unknown runtime addresses. Verify exact helpers
with deterministic vectors, generate all candidate bodies mechanically, and let
the existing per-boundary shadow gate veto candidates independently. Do not add
candidate-specific timing constants or hardware behavior.

**Consequences:** 551 candidates passed 6,199,718 shadow comparisons and native
proof, removing 271,913 executions. LSR.W, ROR.W and CMPI.B were retained as
fail-closed shadow rejections. The resulting 95.5453% translated share proves
`REMAINING_ATTRIBUTION_AND_DYNAMIC_COVERAGE_95_PROVEN`; the final ledger still
accounts for every 289,055 interpreter execution.

## ADR-0025 — Promote only the canonical 0x03A7AE generated block
**Status:** Accepted for M11.44 developer-only hybrid tooling
**Date:** 2026-09-09

**Context:** After M11.43 restored the authoritative checkpoint identity, the
M11.42 restart gate reproduced the frozen 661,916 interpreter remainder. The
hottest pair, 0x03A7AE `TST.W ($00FF1654).L` and 0x03A7B4 `BNE.W`, retained the
M11.35 IR/prefetch rejection (`actual 0x4E73`, expected `0x4A79`).

**Decision:** Re-run the exact decoder-owned pair through the current generic
timing/refresh, instruction-boundary, checkpoint-canonicalization and GPGX
shadow contracts. Promote it only as generator output with no address-specific
semantic body or timing constant. Keep all other remaining forms fail-closed,
retain 0x060BA4 as hardware-visible fallback, and close the complete remainder
with a per-PC ledger rather than using an `other` bucket.

**Consequences:** The historical rejection is preserved but classified obsolete.
The pair passed 100,948 per-instruction shadow comparisons with zero divergence;
the unchanged 600-frame native proof preserves checkpoint/video/CPU/boundary
identity and removes 100,948 interpreter executions. Final translated share is
91.3548%, below the 95% gate, so the result is
`REMAINING_INTERPRETER_ATTRIBUTION_PROVEN_SEMANTICS_BLOCKED`.

## ADR-0024 — Canonicalize only the proven pinned-GPGX representation layout
**Status:** Accepted for M11.43 developer-only hybrid tooling
**Date:** 2026-09-09

**Context:** M11.42 found that the M11.41 adapter left a YM2612 host pointer at
serialized offset `140734`. The pinned DLL's raw state also showed its Z80
`daisy` pointer at `144576`. The available external `state.c` source and stale
build objects were not byte-for-byte synchronized with that DLL: source-only
arithmetic predicts an earlier Z80 position, while raw cart mapping and pointer
evidence prove the pinned binary position.

**Decision:** Model the raw-saved YM2612/Z80 structs with machine-checked x64
`sizeof`/`offsetof` assertions and construct one non-overlapping representation
span table from those fields. Guard `STATE_SIZE=0xfd000` and version
`GENPLUS-GX 1.7.6`; reject unknown values. Canonicalization copies the raw
buffer and clears only the table's 55 host-pointer, one function-pointer and
111 ABI-padding spans. It does not clear semantic bytes or broaden hardware
serialization. The active USA cart path is separately serialized field-by-field;
optional SVP wholesale state is out of scope for this active baseline.

**Consequences:** The complete contract produces the new authoritative aggregate
`251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d` across five
current proofs and current/exact-historical/M11.41 raw replay. M11.41's
`c9236218...` remains historical and is superseded because its adapter erased
semantic bytes. M11.42 PHASE 2 remains unstarted.

## ADR-0023 — Keep M11.42 closed on checkpoint identity mismatch
**Status:** Accepted for M11.42 gate governance
**Date:** 2026-09-09

**Context:** The M11.41 repair was expected to reproduce authoritative
checkpoint aggregate `c9236218...` from commit `5c19e22`. Two fresh runs matched
all execution and video metrics but produced `d5de401c...`. Raw evidence shows
the M11.41 canonicalization leaves a host-pointer byte at serialized offset
`140734`.

**Decision:** Treat the restart as blocked. Do not classify the remaining
interpreter executions, broaden semantic forms, generate candidates, run shadow
promotion, broaden hardware behavior or weaken the checkpoint identity gate.
Repair and independently re-prove the developer-only identity contract in a
separate bounded task before resuming M11.42.

**Consequences:** M11.39, M11.40 and M11.41 evidence remain preserved; no
coverage or attribution result is claimed from this run. Production/native
architecture is unchanged.

## ADR-0022 — Deterministic developer-only checkpoint identity
**Status:** Accepted for M11.41 developer-only hybrid tooling
**Date:** 2026-09-09

**Context:** M11.40 matched ROM, GPGX, video, instruction and boundary metrics
but could not reproduce the historical checkpoint aggregate. Byte evidence
showed that GPGX v1.7.6 wholesale-serialized host pointers and ABI padding in
the YM2612 and Z80 contexts. The first mismatch was frame 60, state offset
`140654`, inside `FM_SLOT.DT`.

**Decision:** Preserve the complete raw `retro_serialize()` buffer in ignored
developer evidence, but compute the authoritative checkpoint identity from a
copy with only the proven pointer/padding spans cleared for the recognized
`STATE_SIZE=0xfd000` format. Reject unknown state sizes. Keep the existing
frame cadence and aggregate order; retain all semantic state bytes and record
both raw and authoritative per-record hashes.

**Consequences:** Checkpoint identity is deterministic across the current and
exact historical checkout: the repaired 600-frame aggregate is
`c9236218f55fb18f7f1d5095e4970b25f228de588bd03bd7cbbffccc2e225fd1`. Raw
evidence remains available for future audits but is not tracked. This does not
start M11.40 PHASE 2 and does not add a production emulator dependency.

## ADR-0021 — Bounded hot-path profile and exact multi-block promotion
**Status:** Accepted for M11.39 developer-only hybrid tooling
**Date:** 2026-09-09

**Context:** M11.38 proved generic instruction-boundary continuation, but
661,916 interpreter executions remained after the exact M11.38 registry. The
next coverage step needed a dynamic-payoff ranking without becoming an
indefinite discovery pass or weakening the exactness gate.

**Decision:** Emit a deterministic interpreter-PC profile from the existing
M11.38 registry run, rank by dynamic instruction executions, and consider a
bounded maximum of 40 decoder-owned ranges. Promote only candidates whose exact
forms pass the independent semantic harness, whose bodies are generated from
canonical ROM provenance, and whose every instruction boundary passes the
existing GPGX shadow contract. Keep indirect control unresolved and hardware
visible candidates in fallback unless an existing bridge already proves them.

The M11.39 set contains `[0x000380,0x0003A0)` and `[0x03A864,0x03A868)`.
The first required one new exact semantic form, `ADD.W (An)+,Dn`; the second
reused the verified direct Bcc form. Generated code and metadata remain
separate from handwritten helper/registry glue, and promotion remains offline.

**Consequences:** Native translated dynamic share rose from 63.5261% to
89.7991% with exact 600-frame checkpoint/video, CPU, interrupt and boundary
evidence. The remaining profile is reported conservatively; no latent function
boundaries or indirect target sets are claimed. M11.40 remains a separate
milestone and is not implemented here.

## ADR-0020 — Generic instruction-boundary yields for rejected multi-instruction blocks
**Status:** Accepted for M11.38 developer-only hybrid tooling
**Date:** 2026-09-08

**Context:** M11.37 rejected four mechanically generated two-instruction ranges
because natural execution showed interrupt interleaving inside their ranges.
GPGX owns interrupt service, trace handling, cycle/refresh state and the VDP/Z80
schedule; treating the ranges as atomic would change observable ordering.

**Decision:** Make generated execution instruction-granular at the bridge
boundary. After each guest instruction, the generated body asks one generic
GPGX-owned boundary callback and returns `BlockExit` with the exact next PC,
reason and executed count when continuation is required. The shadow adapter
compares registers, SR, PC, prefetch, RAM/bus effects, cycle/refresh and
interrupt-visible fields at every boundary. Native continuation re-enters the
same generated block at the exact instruction entry PC; GPGX performs any
pending interrupt before the continuation. Do not add candidate-specific
timing, a second scheduler, atomic blocks, runtime JIT or production linkage.

**Consequences:** The four frozen M11.37 ranges pass the per-boundary shadow
gate and native 600-frame equivalence; 274 naturally observed continuations
resumed after actual GPGX interrupt service. The capability is conservative and
instruction-granular, and further coverage still requires a separate milestone.

## ADR-0019 — Controlled dynamic block promotion remains offline and generic
**Status:** Accepted for M11.37 developer-only hybrid tooling
**Date:** 2026-09-08

**Context:** M11.36 made the GPGX timing/refresh boundary authoritative. M11.37
needed to expand coverage beyond six entries without turning discovery into a
runtime JIT or trusting generated code by construction.

**Decision:** Use one bounded natural trace, an explicit candidate queue,
independent semantic verification of the used Bcc/DBcc/TST forms, existing
decoder-owned mechanical generation, and the same GPGX shadow gate before
promotion. Store block boundaries and instruction counts as generated metadata;
the registry dispatches through that metadata rather than a candidate-specific
hard-coded list. Reject a candidate if its block crosses an observed interrupt
or hardware-visible boundary. Keep all discovery, generated bodies and GPGX
bridge code in developer-only tooling; do not add runtime JIT, whole-ROM
translation or ROM-byte coverage claims.

**Consequences:** Sixteen new single-instruction natural blocks passed the
full shadow/native gate. Four multi-instruction candidates remain unpromoted
because the trace showed interrupt interleaving; a hardware-visible candidate
was rejected fail-closed. Further expansion requires a new bounded milestone.

## ADR-0018 — GPGX post-instruction bridge comparison boundary
**Status:** Accepted for M11.36 developer-only hybrid tooling
**Date:** 2026-09-08

**Context:** M11.35 compared a prediction sampled before a direct TST entry with
an execution event observed at the next instruction boundary. GPGX rebases its
frame-relative `m68k.cycles` and `refresh_cycles` counters at frame end, so the
first mismatch was `896114/896268` versus `74/228` even though the instruction
effect itself was exact.

**Decision:** Keep GPGX as the timing, refresh, hardware and interrupt oracle.
Expose one generic post-instruction hook from the normal interpreter after
semantic and cycle/refresh advancement, before the next scheduler/frame
transition. Close shadow comparison there. Keep the existing entry hook for
block dispatch. A translated block remains valid only for bounded code whose
accesses cannot expose a GPGX-observable boundary; interrupt polling and trace
handling remain outside generated semantics and are never crossed atomically.

**Consequences:** No baseline subtraction, magic constant, candidate-address
patch or parallel timing engine is needed. The old M11.35 negative result stays
historical. The existing M11.33 plus three M11.35 blocks pass the full shadow
and native 600-frame gate; future blocks still require the same exact contract.

## ADR-0017 — Fail-closed demand-driven block promotion gate
**Status:** Accepted for M11.35 developer-only pilot
**Date:** 2026-09-08

**Context:** M11.33 generated three proven blocks and M11.34 independently
verified their used instruction semantics. The next bounded experiment needs a
repeatable way to select naturally executed blocks without turning the project
into a whole-ROM recompiler or trusting generated output by construction.

**Decision:** Permit only one bounded natural trace, exact decoder-owned block
IR, independent vectors for newly required forms, provenance-bound generated
bodies and full GPGX shadow comparison before promotion. Keep candidate bodies
and discovery evidence in developer-only tooling. If semantic or runtime shadow
evidence fails, do not promote the candidate and retain interpreter fallback.
Do not add runtime JIT, automatic trust, or whole-game ranking in this pilot.

**Consequences:** M11.35 may end in a negative gate result while preserving
useful discovery, semantic and generator evidence. The first runtime blocker is
recorded rather than hidden by running native execution. Any future promotion
must establish the missing cycle/refresh and interrupt-boundary contract in
a separately bounded task.

**Result:** `DEMAND_DRIVEN_PROMOTION_RUNTIME_BLOCKED` at `0x3A85E`, with
`actual_cycles=74 expected_cycles=896114 actual_refresh=228
expected_refresh=896268`.

## ADR-0016 — Test-only independent M68K semantic oracle
**Status:** Accepted for M11.34 verification tooling
**Date:** 2026-09-08

**Context:** M11.33 established decoder-owned generated execution, but shared
decoder, emitter and helper assumptions could agree while encoding an incorrect
68000 rule.

**Decision:** Verify only the seven currently emitted M11.33 combinations with
a small deterministic reference model transcribed from the Motorola/NXP 68000
Programmer's Reference Manual. Keep it in tests/developer tooling, separate
from the decoder, emitter and production targets; do not broaden the instruction
surface to make the verification table larger.

**Consequences:** A mismatch must remain visible as a first-mismatch vector and
be fixed only with sufficient evidence. Passing this checkpoint establishes
independent verification of the bounded used subset, not a general CPU core or
whole-ROM semantic claim.

**Reference:** `https://www.nxp.com/docs/en/reference-manual/M68000PRM.pdf`

## ADR-0015 — Decoder-owned generated basic-block bodies
**Status:** Accepted for developer-only migration tooling
**Date:** 2026-09-08

**Context:** M11.32 proved three manually written basic-block bodies, while
the next milestone must remove routine-specific instruction authoring without
changing the proven GPGX timing and hardware boundary.

**Decision:** M11.33 uses the existing decoder/exact IR to generate C++ calls
to a small hybrid instruction-helper boundary. Generated functions retain the
guest PC, opcode and decoded assembly as provenance. The initial generated set
is exactly the three M11.32 blocks; unsupported forms fail closed. The oracle
prediction path remains separate until M11.34 verifies common semantics.

**Consequences:** Adding an ordinary supported block can use generator output
rather than a handwritten body, while the hybrid bridge, GPGX state and
generated artifact remain developer-only. This does not create a production
CPU emulator or authorize semantic promotion.

**Affected files/milestones:** `src/tools/hybrid/`, `CMakeLists.txt`, M11.33.

## ADR-0014 — Developer-only GPGX basic-block replacement boundary
**Status:** Accepted for migration experiments only
**Date:** 2026-09-08

**Context:** The M11.30 atomic override skipped GPGX instruction-body timing
and diverged in serialized VDP/sound state. A bounded proof needs native
mechanical execution while preserving GPGX's own fetch, memory bus, prefetch,
cycle and refresh behavior.

**Decision:** Add one developer-only block callback immediately before GPGX's
opcode dispatch. A registered block may perform its exact bounded operations
through helper functions implemented inside GPGX, then return control to the
normal CPU loop. The initial registry contains only `0x2D66`, `0x604BC` and
`0x61032`, each with an explicit state/effect contract. This is not a generic
M68K replacement engine and is not linked by production Sega-Thor targets.

**Consequences:** The three-block 600-frame proof can reuse GPGX hardware and
timing semantics and has passed with exact state/video equivalence. Every new
block still requires an independent shadow proof; uncertain side effects must
fail closed. The bridge, ROM PCs, emulated memory and GPGX remain confined to
developer tooling. See `docs/reports/BASIC_BLOCK_RECOMPILATION_TIMING_M11_32.md`.

## ADR-0001 — Native C++ reimplementation, not a general emulator
**Status:** Accepted

**Context:** The goal is to make Beyond Oasis run natively on modern systems while preserving original behavior.

**Decision:** Translate game routines and implement only the Mega Drive hardware semantics required by the game. Do not build a general 68000/Mega Drive emulator as the main architecture.

**Consequences:**
- more reverse-engineering work up front;
- clearer native game code long term;
- hardware compatibility layer must remain narrow;
- address/routine mappings must be preserved for traceability.

## ADR-0002 — User-supplied ROM owns commercial data
**Status:** Accepted

**Decision:** The repository never contains the original ROM or extracted commercial assets. Runtime/tools read a locally supplied ROM.

**Consequences:**
- Git repository remains source-only;
- CI tests use synthetic/non-copyrighted fixtures;
- local developer tools may export ignored files for inspection.

## ADR-0003 — Fidelity before enhancements
**Status:** Accepted

**Decision:** Reproduce original gameplay and rendering semantics before widescreen, HD, QoL, remaster behavior or Story of Thor 2 work.

## ADR-0004 — 500-line hard file limit
**Status:** Accepted

**Decision:** Source and project documentation files must not exceed 500 lines.

**Reason:** Keep modules understandable for humans and AI agents, discourage monoliths, make review and context retrieval reliable.

## ADR-0005 — USA reference binary, region-independent reconstructed game
**Status:** Accepted
**Date:** 2026-09-03

**Context:** The project aims to reconstruct the complete game so the native C++ runtime can target desktop, mobile, and future consoles. Regional retail binaries differ, while existing public reverse-engineering work and known addresses are based on the USA `Beyond Oasis` release.

**Decision:**
- Use the clean USA retail `Beyond Oasis` binary as the canonical engineering reference for addresses, traces, and differential verification.
- The reconstructed game model must be region-independent and must not embed USA ROM addresses in gameplay code.
- Europe and Japan are secondary evidence sources and future data profiles, not separate game implementations.
- ROM-specific offsets belong only in extraction/reverse-engineering metadata.
- Runtime gameplay code consumes normalized game data structures rather than raw ROM addresses.


**Alternatives considered:**
- Europe as primary reference: rejected because current public address knowledge targets USA.
- Supporting all regions equally from the beginning: rejected because it multiplies binary-diff work before core behavior is understood.
- Building a USA-only final runtime: rejected because it conflicts with the portable reconstruction goal.

**Consequences:**
- address annotations default to USA reference addresses;
- later regional support maps region-specific data to the same C++ game model;
- regional differences are documented rather than forked into separate engines;
- reference identity must be established before translating substantial 68000 routines.

**Affected files/milestones:** M2 onward, all reverse-engineering documentation and extraction code.

## ADR-0006 — Separate reassembly exactness from execution trust
**Status:** Accepted
**Date:** 2026-09-06

**Context:** M11.14 `CODE_VERIFIED` proved exact decoder/assembler bytes but did
not prove that each reconstructed range is executed code. Treating those facts
as one classification could make weak structural evidence appear trusted.

**Decision:** Keep the existing full-ROM ownership and ASM artifacts, while
reporting an evidence ladder: `ASM_ROUNDTRIP_EXACT`, `CODE_STATIC_SUPPORTED`,
`CODE_EXECUTED` and `BEHAVIOR_VERIFIED`. Static support requires an independent
anchor, vector/startup provenance, dynamic evidence, or an exact incoming xref
from an already trusted caller. Direct-caller counts and Ghidra boundary
agreement alone do not raise trust. The automatic promoter records a successful
round trip at the lowest level unless explicit evidence qualifies it.

**Consequences:** Existing ranges can be downgraded without changing bytes or
the exact full-ROM rebuild. Audits must retain artifact identity, ROM identity,
entry/range linkage and concrete xref sources. Missing dynamic evidence remains
unknown and is never synthesized.

## ADR-0007 — Natural runtime evidence is range-local and fail-closed
**Status:** Accepted
**Date:** 2026-09-06

**Context:** M11.16 needs to distinguish a naturally observed program counter
from a forced hook, a stale report, or a report whose provenance artifact is
missing. Dynamic reachability of one routine must not silently trust callers,
callees, or adjacent ranges.

**Decision:** Accept only `DYNAMIC_NATURAL` evidence whose ROM identity,
scenario, artifact hash, target address and audited half-open range all match.
Reuse of an existing artifact is preferred to rerunning an emulator. Missing
or forced evidence remains non-promoting, and trust changes are local to the
observed range. A pre-run selection report is mandatory for bounded target
passes.

**Consequences:** Dynamic passes can end with a reachability-limited result
without weakening static exactness. Historical runtime claims without a
retained artifact are explicitly reported as unaccepted context. Callers and
callees require their own evidence.

## ADR-0008 — Structured data classifications are explicit and non-heuristic
**Status:** Accepted
**Date:** 2026-09-06

**Context:** The full-ROM split contains millions of bytes whose apparent
shape is not proof of data. M11.17 needs a useful data inventory while keeping
unknown bytes and code/data boundaries honest.

**Decision:** Accept `DATA_STRUCTURE_SUPPORTED` only when an exact range,
element width, deterministic count/end, canonical byte identity and a proven
consumer or parser are present. Use `DATA_REGION_SUPPORTED` for a bounded
non-code region whose field semantics are incomplete. A trusted data range
creates a conflict record on code overlap and vetoes future code promotion;
weak or unknown data hypotheses do not veto.

**Consequences:** Structured data can be reported without replacing the
blob-backed full-ROM representation. Resource payloads remain unknown until
their own compressed boundaries are proven, and code/data conflicts cannot be
resolved silently.

## ADR-0009 — Bounded native controlled-screen platform seam
**Status:** Accepted
**Date:** 2026-09-06

**Context:** M11.18 requires the first interactive native vertical slice while
reconstructed-source and broad RE expansion are frozen. The repository has
portable runtime/game logic but no window or input backend.

**Decision:** Add a small `oasis_platform` adapter. On Windows it owns a native
Win32 window, keyboard polling, focus-loss clearing and scaled software-DIB
presentation. The game layer owns the deterministic fixture, movement and
software rasterization. Non-Windows builds keep a compile-only unavailable
adapter until a concrete backend is justified.

**Consequences:** The core/game path remains platform-independent and has no
new third-party dependency. The first playable runtime is Windows-only; this
is an explicit bounded limitation, not a claim of cross-platform GUI support.
The screen geometry is synthetic and cannot be used as evidence for an
original room.

**Affected files/milestones:** `src/platform/`, `src/game/controlled_screen.*`,
`src/game/render/`, M11.18.

## ADR-0010 — Address-level GPGX manual runtime evidence
**Status:** Accepted
**Date:** 2026-09-07

**Context:** The instrumented Genesis Plus GX core now provides persistent
manual-realtime executed-PC bitmaps for the canonical Beyond Oasis ROM. This
is stronger evidence than static reachability for individual instruction
starts, but it does not establish function boundaries, semantics, or complete
range execution.

**Decision:** Add `GPGX_MANUAL_REALTIME` as a provenance-bound evidence source
that records only `CODE_EXECUTED_AT_ADDRESS` facts. The importer must verify
the canonical ROM identity, bitmap size and metadata/file hashes, retain
unsupported decoder results, and reject odd, out-of-range or data-conflicting
addresses. Evidence is duplicate-safe by ROM, source and capture identity.
It may report existing-range coverage, but it must not promote or otherwise
change range classifications.

**Alternatives considered:** Promoting an entire static range from one
executed PC was rejected because it invents boundaries and branch coverage.
Reclassifying trusted data on runtime overlap was rejected because conflicts
must remain explicit and fail-closed. Replay or input automation was rejected
because this source is specifically manual realtime capture.

**Consequences:** Runtime execution evidence is available to later trust
analysis at address granularity. Unknown executed addresses remain separately
reported as `RUNTIME_EXECUTED_UNKNOWN`; `RUNTIME_DATA_CONFLICT` is retained as
a critical diagnostic. No gameplay code, `main`, ROM or extracted asset is
changed.

**Affected files/milestones:** `src/tools/gpgx_import_gpgx_coverage.cpp`,
`CMakeLists.txt`, M11.19.

## ADR-0011 — Developer-only hybrid migration experiment
**Status:** Accepted
**Date:** 2026-09-07

**Context:** The user explicitly replaces M11.27 manual ID3 hunting with a
bounded hybrid execution PoC for the already translated `0x3820` decompressor.

**Decision:** A developer-only adapter may observe natural GPGX calls, run
existing C++ translations on copied bounded inputs and compare their effects.
Its dispatch states are `EMULATED`, `SHADOW_NATIVE`, `NATIVE_OVERRIDE`. Shadow
always preserves the original CPU result. Override must fail closed until all
CPU, memory, return and execution-timing effects needed by continuation are
proven; output equivalence alone cannot authorize it.

**Consequences:** GPGX remains an external developer tool. No production target
may depend on the emulator, its CPU context, original PCs or its RAM layout
through this experiment. Exactly one routine is in scope; no emulator rewrite,
AI generation, routine expansion or manual gameplay search is authorized.

## ADR-0012 — Target-specific developer-only native override boundary
**Status:** Accepted
**Date:** 2026-09-07

**Context:** M11.29 requires one native override whose CPU/hardware contract is
simpler than `0x3820`, while preserving the original GPGX execution path and
avoiding a generic replacement engine.

**Decision:** Use the naturally executed `0x2D66` leaf as the single override
target. Its adapter captures and proves only its bounded registers, full SR,
source/output RAM and saved stack window. On override it applies those effects
and uses GPGX's existing `m68k_set_reg(PC)` transition rather than reimplementing
prefetch or IR state. The adapter stays in developer-only hybrid tooling.

**Consequences:** The 600-frame neutral scenario proves one native replacement
and exact checkpoint/video continuation. The proof does not generalize to
other routines; `0x3820` remains blocked by its separate timing/CCR.X/prefetch
contract. Production targets and dependencies remain unchanged.

## ADR template
Copy this block for new decisions:

```text
## ADR-NNNN — Title
Status: Proposed | Accepted | Superseded | Rejected
Date: YYYY-MM-DD

Context:

Decision:

Alternatives considered:

Consequences:

Affected files/milestones:
```

## ADR-0010 — Minimal developer-only hybrid replacement registry
**Status:** Accepted for migration experiments only
**Date:** 2026-09-07

**Context:** M11.29 proved one target-specific override. M11.30 tests repeatability
across a small natural batch without making the emulator hook a production
runtime dependency or a generic M68K replacement engine.

**Decision:** Keep a small registry mapping explicit ROM PCs to target-owned
`EMULATED`, `SHADOW_NATIVE` and `NATIVE_OVERRIDE` adapters. Each adapter owns
its exact state/effect contract; the registry only routes hook events and
aggregates metrics. Return/prefetch transitions use GPGX's internal bridge
state. Timing and hardware phase are not synthesized by the registry.

**Consequence:** Multiple routines can share the migration boundary, while
unsafe candidates fail closed. The M11.30 batch shadow is clean, but override
promotion remains blocked until the exact GPGX bus-refresh/VDP/sound phase
contract is proven. Production Sega-Thor code remains emulator-free.

## ADR-0013 — Comparative projects are method references only
**Status:** Accepted
**Date:** 2026-09-08

**Context:** M11.31 compares public Streets of Rage 2/3 disassembly,
extraction and static-recompilation projects with the existing Beyond Oasis
evidence. The comparison found no public exact-ROM reassembly proof and no
evidence sufficient to establish shared programmer style, binary modules or
sound-driver lineage.

**Decision:** Transfer only the bounded evidence workflow: canonical identity,
address-preserving maps, explicit unknown/indirect-entry ledgers, natural
evidence gates, and separation of generated/mechanical output from native
helpers. Do not import SoR labels, RAM/object/audio assumptions, generated CPU
recompiler machinery or external code/data. Keep sound lineage as a separate
specialized investigation.

**Consequences:** M11.32, if started, remains a localized Beyond Oasis pass
using existing decoders/contracts. Similarity claims stay separated into
programmer style, binary structure, RE method and sound lineage. No production
runtime dependency changes.

**Affected files/milestones:** M11.31 report and governance documents; no
production source or ROM data.
# ADR-0032 — Keep first portable native routine shadow-only pending identity proof
**Status:** Accepted for M11.51
**Date:** 2026-09-09

**Context:** M11.29 supplied complete structural evidence for the bounded
`0x2D66..0x2D84` table-copy leaf, and M11.50 supplied the portable mechanical
ownership boundary. A structured `TableCopyRoutine` can now be extracted, but
the current pinned GPGX authoritative replacement does not preserve the frozen
M11.50 checkpoint aggregate even though video and isolated accounting match.

**Decision:** Keep the portable routine and zero-divergence shadow adapter as
developer-visible evidence, but do not promote it as authoritative execution.
The hybrid generated/interpreter path remains the oracle/fallback. Any future
promotion must independently close CPU, RAM, VDP, sound, interrupt,
timing/refresh and continuation identity against the frozen 600-frame run.
No timing constant, hardware behavior or gameplay meaning is invented to force
the gate.

**Consequences:** `oasis_core` demonstrates the next abstraction boundary
without weakening behavioral parity. The milestone result is
`PORTABLE_NATIVE_ROUTINE_SHADOW_PROVEN_REPLACEMENT_BLOCKED`; the next task must
explain the exact identity mismatch before another authoritative replacement.

**Affected files/milestones:** `src/core/table_copy_routine.*`, the 2D66
developer adapter and tests, M11.51 governance and report.

## ADR-0033 — Per-instruction hybrid continuation is required for native routine promotion
**Status:** Accepted for M11.52
**Date:** 2026-09-09

**Context:** M11.51's portable `TableCopyRoutine` shadow matched, but its
authoritative native adapter changed four canonical checkpoint bytes after
frame 120. The adapter's routine-level lump-sum cycle update produced the same
exit cycle count as the interpreter while missing intermediate GPGX refresh
sampling. The shadow comparison stopped before the scheduler-visible state
where the mismatch appeared.

**Decision:** Keep portable routine semantics in `oasis_core`. Extend the
developer-only hybrid bridge with instruction fetch and per-instruction
begin/finish callbacks, and require the adapter to reconstruct the exact
opcode/extension stream, DBF continuation decision, MOVEM dynamic timing and
RTS prefetch state. Do not use checkpoint canonicalization or aggregate-hash
exceptions to promote a native routine. Promotion requires paired per-frame
canonical checkpoint identity, video identity, continuation timing and
execution accounting.

**Alternatives considered:** Keep the lump-sum adapter; rejected because it
misses refresh/scheduler semantics. Canonicalize the four bytes; rejected
because RAM, sound and Z80 interrupt fields are semantic. Add a candidate-local
checkpoint patch or a full CPU emulator; rejected by project scope and evidence
rules.

**Consequences:** The 2D66 routine is promoted with a generic hybrid
continuation contract, while the generated/interpreter path remains the
oracle/fallback for other routines. Future routine adapters must prove their
own post-return and serialized-state boundary.

**Affected files/milestones:** `src/tools/hybrid/replacement.hpp`,
`src/tools/hybrid/gpgx_bridge.c`, `src/tools/hybrid/runner.cpp`, the 2D66
adapter/test and M11.52 evidence/governance.
# ADR-0034 — Second portable native routine contract
Status: Accepted for M11.53
Date: 2026-09-09

Context: M11.52 proved that native replacement requires per-instruction GPGX
continuation. M11.53 had one complete natural candidate, 0x604BC, whose old
adapter used a lump-sum timing handoff.

Decision: Extract only the structured BSET/Scc/LEA/RTS semantics of
0x604BC..0x604E6 into oasis_core with opaque tokens, portable registers and
memory, and explicit resumable boundaries. Keep ROM metadata, canonical
opcode/extension validation, GPGX fetch/begin/finish, prefetch/refresh,
boundary sampling, block-hook continuation return and RTS return state in
tools/hybrid. Promote only after
paired dual-routine checkpoint/video identity and separate accounting.

Consequences: TableCopyRoutine and the second routine coexist through the
existing registry without shared candidate state. No gameplay meaning,
hardware behavior, subsystem abstraction or second timing model is added.
The old lump-sum adapter path is removed for 0x604BC; generated execution
remains the oracle/fallback.

Affected files/milestones: src/core/ram_flag_routine.*, the 0x604BC adapter
and regression test, replacement accounting, M11.53 report.

# ADR-0035 — Stop at the first proven native routine cluster
Status: Accepted for M11.54
Date: 2026-09-09

Context: M11.52 and M11.53 authoritatively proved two portable native routine
contracts, TableCopyRoutine at 0x2D66..0x2D84 and RamFlagRoutine at
0x604BC..0x604E6. M11.54 reproduced the dual proof twice and audited exact
caller/callee and raw-memory evidence.

Decision: Classify the RamFlag direct callers at 0x604F6 and 0x60BCC, together
with the raw 0x00FF0010..0x00FF0016 window, as a CALL_GRAPH_CLUSTER and
MEMORY_STRUCTURE_CLUSTER. Do not call it a gameplay subsystem or move callers
into oasis_core until caller CFG, data ownership and hardware ordering close.
Keep TableCopy and RamFlag as separate ownership islands. Do not add a third
routine or typed data in M11.54.

Consequences: The architectural result is PORTABLE_ROUTINE_CLUSTER_PROVEN,
not FIRST_PORTABLE_SUBSYSTEM_BOUNDARY_IDENTIFIED. The next proposed task is a
bounded RamFlag caller/data closure audit. The existing dependency direction
is preserved: oasis_core owns portable semantics/tokens, while tools/hybrid
owns ROM provenance, GPGX continuation, oracle/accounting and hardware.

Affected files/milestones: M11.54 governance documents and
reports/NATIVE_ROUTINE_CLUSTER_M11_54.md; no production source change.

# ADR-0036 — Keep RamFlag caller regions developer-only after M11.55 closure
Status: Accepted for M11.55
Date: 2026-09-09

Context: M11.54 had static-only caller edges for `0x0604F6` and `0x060BCC`.
M11.55 added a natural-entry observer and reproduced the unchanged dual-native
proof twice. The observer deterministically attributes one `0x0604F6` entry
and three `0x060BCC` entries, while exact bounded slices expose continuation,
sibling-call, A5-relative memory and hardware-boundary gaps.

Decision: Accept `RAMFLAG_CALLER_CONTRACTS_PROVEN` only as a bounded caller-
region result. Keep attribution, ROM byte provenance, GPGX timing/hooks,
address provenance and the `0x00A11100` hardware interaction in
`tools/hybrid`. Treat the `0x0604F0` path and `0x060BC4` path as partial
contracts, not portable routines. Do not create a typed structure for
`FF0010..FF0016`; offset `FF0015`, A5-relative effects, lifetime and aliasing
remain unknown. Do not implement a caller or subsystem unless a future task
closes its complete continuation and memory/hardware contract.

Consequences: Dynamic caller provenance is now reproducible and fail-closed for
unknown caller classes without widening `oasis_core`. The exact
`0x060BC4` hardware-prefix/RamFlag-suffix ordering is recorded, but the wider
caller remains boundary-blocked. `PORTABLE_ROUTINE_CLUSTERS` remains 1 and
`PORTABLE_SUBSYSTEM_BOUNDARIES` remains 0. M11.56 must choose one falsifiable
continuation or sibling/data closure and is not executed by this decision.

Affected files/milestones: caller attribution observer/test, extracted hybrid
library wrapper, M11.55 governance documents and report; no production source
change.

# ADR-0037 — Keep the parent-owned 0x604F0 suffix internal
Status: Accepted for M11.56
Date: 2026-09-09

Context: Exact natural evidence identifies the previously unknown 0x611D6
destination as the shared saved-register/SR epilogue of the 0x60004/0x6042A
parent. 0x604F0 has no ordinary standalone return at A7; it inherits 58
bytes of parent state. The old decoder budget also includes another arm.

Decision: Record THIRD_ROUTINE_NOT_A_STANDALONE_ROUTINE with dominant blocker
ENCLOSING_ROUTINE_BOUNDARY. Do not manufacture a third routine by attaching
the shared restore/RTS tail or absorbing the parent's hardware prefix.
Retain full-SR/event continuation as partial. Keep the bounded read-only
observer and local evidence validator in tools/hybrid; do not change core,
RamFlag semantics, typed data or subsystem ownership.

Consequences: Structural continuation ownership is now proven for the natural
path, while a portable independent entry contract is still absent. Inventory
remains two authoritative routines and one cluster, with no subsystem boundary.
A future M11.57 may test an explicit parent/suffix handoff contract, preserving
the distinction between a portable internal helper and a complete routine.

Affected files/milestones: caller continuation observer/validator/test,
M11.56 report and governance. No production implementation.
# ADR-0038 — Parent-owned portable suffix helper
**Status:** Accepted for M11.57
**Date:** 2026-09-10

**Context:** M11.56 proved that `0x604F0` is an internal fallthrough carrying
the parent frame into a shared epilogue, so it cannot be promoted as a third
standalone routine. Its hardware-free suffix nevertheless has a closed
entry/exit contract and composes with the already-proven RamFlag routine.

**Decision:** Add one small architecture-neutral `ParentSuffixMachine` core
executor with opaque phase and parent-continuation tokens. It owns only the
five safe-RAM SF writes and structural RamFlag call. Keep ROM addresses,
canonical bytes, GPGX fetch/timing, nested BSR representation and the mapping
to `0x611D6` in the developer-only hybrid adapter. Keep parent frame/SR,
hardware, shared epilogue and RTS ownership with the parent.

**Consequences:** The exact suffix can be tested and resumed independently
without inventing gameplay meaning or a subsystem boundary. The native helper
is counted as `PORTABLE_INTERNAL_HELPER`; it is never called a third routine.
Full-SR and hardware behavior remain outside the helper and require a separate
milestone if they are ever considered.

**Affected files/milestone:** `src/core/parent_suffix.*`,
`src/tools/hybrid/candidate_parent_suffix.*`, `tests/parent_suffix_test.cpp`,
M11.57 report and hybrid accounting.

# ADR-0044 — Scoped temporal provenance sidecar for M12

**Status:** Accepted design contract; implementation stages not completed
**Date:** 2026-09-12

**Context:** Controlled BizHawk capture is working, but repeated manual
RAM/register/ROM dependency reconstruction duplicates work. Existing Carver
IntervalDB owns the ROM partition and non-owning evidence reports; its graph
does not encode instruction instances, memory versions or capture completeness.
Callback PC and lagged input observations already demonstrate false-proof risks.

**Decision:** Design a developer-only Python/SQLite sidecar that separates
immutable observations, temporal byte/register versions and scoped relations.
Use demand-driven slicing, explicit data/address/control dependencies,
coverage certificates and proof-obligation scheduling. Reuse the controlled
harness and M12 static analyzers. Export non-owning ROM evidence to Carver;
existing exact promoters alone may update ownership under their current gates.
V1 must recover the known canary with minimal RAM/register semantics, not
merely import an expected graph or log A372 followed by UNKNOWN.

**Consequences:** No monolithic rewrite, new production dependency, graph
server or general symbolic/emulator implementation. Repeats establish
reproducibility rather than independent semantic proof. Savestate roots are
scope boundaries, not invented reset provenance. Hardware visibility and
static completeness have separate gates. V0 capability validation precedes
V1 implementation; no milestone completion or ownership change is implied.

**Evidence/design:** `docs/reports/THOR_EVIDENCE_ENGINE_ARCHITECTURE.md` and
`docs/reports/THOR_EVIDENCE_ENGINE_STAGES.md`; verified baseline manifest
contains 1,475,368 SOURCE_OWNED bytes. ADR-0043 remains authoritative for the
M12 ASM → M13 rebuilt parity → M14 systematic C++ sequence.

# ADR-0045 — V0.1 raw completion and launch identity gate

**Status:** Accepted bounded repair; V1 remains gated
**Date:** 2026-09-12

**Context:** The V0 gate review reproduced sealing of a raw prefix ending at a
valid epoch, incorrect collector attribution, and capability conclusions that
survived missing evidence. A sealed transport must not be mistaken for a
complete execution or a causal proof.

**Decision:** Require a strict `thor.evidence.raw.v0.1` header/footer envelope
and a completed local launch receipt before normalization. The receipt binds
the actual Lua collector, normalizer, harness, emulator/core/config, watch-plan
order, mode/reverse flag, ROM/state and scenario. Report capabilities are
derived from runtime witnesses or hashed BizHawk API source receipts; absent or
contradictory evidence yields UNKNOWN/ERROR. Historical V0 captures remain
valid only as historical evidence when their execution identity cannot be
reconstructed.

**Consequences:** V0.1 can establish a truthful capture boundary and a local
FF13CC precondition matrix without implementing causal provenance. Generic bus
width, overlap, IRQ, same-value completeness and input causality remain explicit
frontiers. No ownership mutation, ROM discovery or production dependency is
introduced.

**Evidence:** `docs/reports/THOR_EVIDENCE_ENGINE_V0_1.md`, the V0.1 receipt,
raw-envelope and report tests, and the three repeated bounded captures under
ignored `build/thor-evidence/v0`.

# ADR-0046 — Local FF13CC V1-gate remains fail-closed

**Status:** Accepted bounded gate; V1 remains blocked
**Date:** 2026-09-13

**Context:** The accepted V0.1 capture proves ordered exact-hook observations
around one `A372` execution, but does not provide dense instruction coverage or
an interruption boundary. Treating an exact hook miss as absence of an
overlapping writer would create the causal false proof the V1 gate is intended
to prevent.

**Decision:** Add only a local gate certificate and adversarial validator. The
checked static `MOVE.L D2,(A5)+` form, selected ordered pairing and four-byte
temporal representation may be certified. Writer completeness requires an
explicit dense instruction list with `NO_MEMORY_WRITE`, `WRITE_DISJOINT`, or
`WRITE_TARGET_OVERLAP`; any `UNKNOWN_MEMORY_EFFECT`, omitted target coverage,
or unresolved interruption blocks the gate. One-byte overlap and same-value
writes remain writes. No provenance graph, general last-writer engine,
register propagation or ownership action is introduced.

**Consequences:** The local result is `PARTIAL / V1 BLOCKED`: pairing is
closed, while dense writer coverage and interruption remain explicit frontier
items. A future V1 authorization must supply those evidence classes before any
causal claim.

**Evidence:** `docs/reports/THOR_EVIDENCE_ENGINE_V1_GATE.md`,
`src/tools/thor_evidence/v1_gate.py`, and the persisted negative fixtures.

# ADR-0043 — Complete ASM reconstruction before systematic C++ migration
**Status:** Accepted for M12.0
**Date:** 2026-09-10

**Context:** The existing exact reassembly proof is a local-ROM-backed split
with 13,550 exact 68000 ASM bytes and 3,132,178 blob bytes. The prior roadmap
would have started Inventory/UI/Save as a portable C++ milestone before the
executable ROM was represented as complete assembler/source.

**Decision:** Rebase the project sequence to ROM -> complete reassemblable ASM
-> rebuilt-ROM runtime parity -> systematic ASM-to-portable-C++ migration.
Define ASM_CODE_COMPLETE, ASM_ROM_MAP_COMPLETE, ASM_REASSEMBLY_BYTE_EXACT and
ASM_REBUILT_ROM_BOOT_PROVEN as separate gates. Keep current native C++ proofs
preserved and set CPP_MIGRATION=PAUSED_PENDING_ASM_COMPLETION. Do not begin
M12.1 implementation in this decision.

**Consequences:** The former Inventory/UI/Save proposal is no longer the
immediate M12 milestone. Executable code cannot be hidden in local blobs;
unknown data and copyrighted payloads may remain local only under deterministic
tooling and explicit classification. M13 owns rebuilt-ROM boot/runtime parity
and M14 owns systematic C++ migration.

**Evidence:** docs/reports/ASM_COMPLETION_CENSUS_M12_0.md.
# ADR-0047 — Close only local dense FF13CC coverage before V1

**Status:** Accepted for V1-GATE-COVERAGE
**Date:** 2026-09-13

**Context:** The prior gate had a proven A372 pairing but exact-address hooks
could not establish that every instruction in the local interval was observed.
It also lacked a local interruption boundary. BizHawk 2.11.1 exposes
`event.on_bus_exec_any`, so a bounded dense capture can answer those two
questions without implementing provenance.

**Decision:** Use one bounded dense capture (with one corrected retry only for
capture serialization) from the proven `A370` pre-boundary through the `A374`
post-fetch boundary. Decode every captured execution instance with the existing
checked ROM decoder, compute concrete memory-write ranges from the pre-execution
register snapshot, retain all target-overlapping and same-value writes, and
fail closed on unknown effects or discontinuity. Treat `A374` as a boundary
witness, not as an unobserved post-boundary instruction.

**Consequences:** The selected interval has complete local execution coverage,
one concrete `FF13CC..FF13CF` writer and a continuous `A370 -> A372 -> A374`
control-flow boundary. This remains a local capability certificate. It does
not establish global IRQ behavior, causal input provenance, a last-writer
engine, a provenance graph, or V1 readiness by itself.

**Evidence:** `docs/reports/THOR_EVIDENCE_ENGINE_V1_GATE_COVERAGE.md`.
# ADR-0048 — First V1 provenance slice is engine-derived and canary-bounded
**Status:** Accepted for V1 FF13CC canary
**Date:** 2026-09-13

**Context:** V0 and the local V1 gate established sealed transport, checked
static forms, dense execution coverage and explicit capability frontiers. The
next authorized step is one causal query for the existing FF13CC canary.

**Decision:** Build provenance only from the checked ROM-bound static producer
and sealed dynamic witnesses. Represent temporal value versions, register
bit-slices, execution instances and dependencies with explicit VALUE, ADDRESS
and CONTROL roles. Persist the derived certificate in a separate SQLite
sidecar table. Reject unknown transforms, address-only edges, PC heuristics,
ROM low-byte substitution and cross-epoch merges. Keep access width, overlap,
same-value writer completeness, IRQ/exception and input-read capabilities
explicitly UNKNOWN.

**Consequences:** The engine can explain the first `FF13CC` value-version as
ROM high24 plus incremented `D5.low8`, and can be re-imported idempotently. The
certificate is not a general last-writer proof and does not authorize V2 or
source ownership.

**Evidence:** `docs/reports/THOR_EVIDENCE_ENGINE_V1_FF13CC_CANARY.md`.
# ADR-0049 — Reusable V2 RAM byte versions remain coverage-gated
**Status:** Accepted for V2 RAM provenance
**Date:** 2026-09-13

**Context:** V1 proved one FF13CC output through a bounded engine-derived
chain, but its RAM output versions were still local to the canary builder.

**Decision:** Generalize only RAM byte temporal versions and write operations.
Represent byte/word/long writes in big-endian physical order, preserve
previous versions and explicit pre-capture roots, isolate restore epochs, and
require a trace-bound complete coverage certificate for `PROVEN` last-writer
results. Return explicit frontiers for gaps, unsupported transforms and
conflicts. Extend the existing SQLite sidecar; do not create another store.
Reuse the primitive for the V1 canary and leave the held-out FF188A query at
`INCOMPLETE_CAPTURE` when existing coverage is insufficient.

**Consequences:** V2 can answer bounded RAM byte last-writer queries without
numeric-value or address-only identity collapse. Register/control provenance,
global coverage, IRQ/input causality, ROM RE and V3 remain separate gates.

**Evidence:** `docs/reports/THOR_EVIDENCE_ENGINE_V2_RAM_PROVENANCE.md`.

# ADR-0050 — V2.1 verified coverage and V1 RAM identity repair
**Status:** Accepted for V2.1 soundness repair
**Date:** 2026-09-13

**Context:** The independent V2 gate audit reproduced false promotion from
metadata-only coverage, future-version temporal queries, partial-write ghosts,
SQLite identity splicing and a V1 adapter path that discarded overlaps.

**Decision:** Keep the V2 byte engine and SQLite sidecar, but separate
unverified coverage claims from verified certificates. A certificate must be
content-bound to a contiguous capture basis, receipt, epoch, byte scope,
decoder/rule and execution instances. Keep immutable initial roots separate
from current state, validate complete write ranges before mutation, reject
inconsistent imported identities atomically, and expose the V1 target through
the concrete V2 byte versions and operation.

**Consequences:** The bounded FF13CC canary remains the only promoted query.
The historical FF188A evidence remains a negative frontier. Global coverage,
IRQ/input/DMA and V3 provenance remain outside this repair.

**Evidence:** `docs/reports/THOR_EVIDENCE_ENGINE_V2_1_SOUNDNESS_REPAIR.md`.

# ADR-0051 — V3 register/control provenance build skeleton
**Status:** Accepted for V3 BUILD
**Date:** 2026-09-13

**Context:** V2.1 re-audit left known soundness defects open, while the next
authorized milestone requires reusable register, execution and local control
structures before stabilization.

**Decision:** Add a developer-only V3 skeleton with temporal D/A register
slices, checked local M68K rules, explicit execution instances, bounded
call/return and local control facts, typed dependency roles, RAM interop,
canonical graph/export and explain APIs. Extend the existing SQLite sidecar;
do not create a second database. New generalized claims default to
OBSERVED/PROVISIONAL/UNKNOWN/CONFLICT and no V2 path is widened.

**Consequences:** V3 can represent the FF13CC and one existing execution
fixture, including partial register writes and address/control roles. The
engine is not a sound causal proof system. V2.1 defects remain in the known
defect ledger and V4 is prohibited until the V3 build gate is published.

**Evidence:** `docs/reports/THOR_EVIDENCE_ENGINE_V3_REGISTER_CONTROL_BUILD.md`.

# ADR-0052 — V4 ROM/resource/hardware cross-domain build bridge
**Status:** Accepted for V4 BUILD
**Date:** 2026-09-13

**Context:** V3 provides temporal register/control structure but has no shared
domain model for canonical ROM roots, resource transforms, DMA or video-memory
targets. V5 needs a graph boundary that can consume these identities without
turning bounded observations into ownership or causal proof.

**Decision:** Extend the existing developer-only engine with typed ROM,
constant, external, RAM/register and hardware roots; checked resource
transform contracts including the existing `0x3820`/Ancient interop; bounded
VRAM/CRAM/VSRAM/SAT and DMA nodes; role-preserving cross-domain edges; and
transactional SQLite persistence in the existing sidecar. New relations remain
OBSERVED/PROVISIONAL/UNKNOWN/CONFLICT by default and SOURCE_OWNED promotion is
forbidden.

**Consequences:** V4 can assemble ROM→resource→hardware and RAM/register→DMA
paths for later static/differential scheduling. It does not establish DMA
timing, same-frame VDP publication, complete aliases, IRQ causality or proof
system soundness.

**Evidence:** `docs/reports/THOR_EVIDENCE_ENGINE_V4_ROM_RESOURCE_HARDWARE.md`.

# ADR-0053 — V5 bounded static request and non-owning Carver bridge
**Status:** Accepted for V5 BUILD
**Date:** 2026-09-13

**Context:** V4 provides cross-domain runtime/resource identities, while the
Carver already has deterministic static analyzers and an ownership-preserving
IntervalDB. The next stage needs a machine-readable handoff without allowing
static hints to become SOURCE_OWNED bytes.

**Decision:** Add bounded trace/ROM/range-bound static requests, validated
responses with structure/domain/boundary certificates, runtime-seeded query
generation, typed static graph merge and Carver evidence export. Static output
is always evidence-only; requests and responses retain unresolved frontiers and
never carry automatic ownership or promotion transactions.

**Consequences:** V5 can route runtime seeds into existing static analyzers and
return deterministic Carver-compatible evidence while preserving the exact
manifest ownership metric. Indirect-CFG completeness, parser closure and
promotion remain separate gates.

**Evidence:** `docs/reports/THOR_EVIDENCE_ENGINE_V5_STATIC_CARVER_BRIDGE.md`.

# ADR-0054 — V6 isolated multi-scenario differential
**Status:** Accepted for V6 BUILD
**Date:** 2026-09-13

**Context:** V5 can route bounded static evidence but cannot compare controlled
neutral repeats with motivated alternate arms without risking cross-capture
temporal splicing.

**Decision:** Add scenario identities bound to environment, trace and arm;
retain an independent graph per scenario; classify exact normalized fact
repetition as observation-only; and reject graph edges whose endpoints leave
their scenario. Differential exports must contain no causal claims or automatic
ownership changes.

**Consequences:** V6 supplies deterministic COMMON and scenario-specific
manifests while preserving event provenance references and scenario isolation.
Repeated address/value observations remain non-causal until a later bounded
capability closes their writer and timing semantics.

**Evidence:** `docs/reports/THOR_EVIDENCE_ENGINE_V6_MULTI_SCENARIO_DIFFERENTIAL.md`.

# ADR-0055 — V7 bounded automatic frontier scheduling
**Status:** Accepted for V7 BUILD
**Date:** 2026-09-13

**Context:** V6 can compare isolated scenarios but has no deterministic way to
select the next unresolved evidence request while preventing unbounded tracing.

**Decision:** Add a frontier inventory ranked by information gain, confidence,
cost and risk; generate only bounded requests with explicit evidence classes;
and enforce a two-pass non-progress exhaustion policy. Whole-ROM trace requests
are rejected and scheduler output never promotes ownership.

**Consequences:** Later stages can drive bounded evidence acquisition and detect
fixed points without conflating scheduling with proof. The scores remain a
heuristic and must be evaluated against a real held-out frontier in V8.

**Evidence:** `docs/reports/THOR_EVIDENCE_ENGINE_V7_FRONTIER_SCHEDULER.md`.

# ADR-0056 — V8 held-out frontier evaluation
**Status:** Accepted for V8 BUILD
**Date:** 2026-09-13

**Context:** V7 scheduling needs a real unknown frontier evaluation to show that
the assembled machine can obtain bounded static and runtime structure without
silently resolving the capability.

**Decision:** Evaluate the existing canonical `0x03BDA6/0x03BDD8` frontier using
the existing static analyzer and a ROM-bound runtime receipt. Join results only
through an explicit UNKNOWN relation when the runtime observation does not close
the static termination condition; never promote ownership.

**Consequences:** V8 demonstrates useful machine-derived structure and preserves
the unresolved stream boundary. It does not authorize causal provenance,
same-value writer completeness or semantic labeling.

**Evidence:** `docs/reports/THOR_EVIDENCE_ENGINE_V8_HELD_OUT_FRONTIER.md`.

# ADR-0057 — V9 bounded operational M12 cycle
**Status:** Accepted for V9 BUILD
**Date:** 2026-09-13

**Context:** V4–V8 provide domain, static, differential, scheduling and
held-out-frontier components, but no single reproducible workflow persists and
exports their state.

**Decision:** Add a one-cycle orchestrator and small CLI that import a sealed
capture into the existing SQLite sidecar, seed and request bounded frontiers,
merge evidence through the non-owning static bridge, and emit deterministic
manifests including persistent-store and Carver state. Reject trace splicing and
keep ownership delta at zero.

**Consequences:** The assembled machine can run and audit one bounded M12 cycle.
Production UX, retries, causal soundness and stabilization remain post-V9 work.

**Evidence:** `docs/reports/THOR_EVIDENCE_ENGINE_V9_OPERATIONAL_INTEGRATION.md`.

# ADR-0058 — stabilization P0 evidence and persistence trust boundary
**Status:** Accepted for stabilization
**Date:** 2026-09-13

**Context:** The assembled V2/V3/V9 machine still allowed fabricated coverage
tags, semantically inconsistent RAM output rows and an under-validated V2
certificate branch to reach stronger trust states.

**Decision:** Require content-attested coverage events with checked execution
semantics; validate RAM output cardinality, producer/address/value/predecessor
relationships before SQLite insertion; and route V2 canary validation through a
dedicated identity and coverage validator. Missing or malformed evidence fails
closed and SOURCE_OWNED remains unchanged.

**Consequences:** P0 false-proof and persistence paths are bounded by explicit
regressions. The attestation is a lineage contract for validated capture input;
it does not authorize ownership promotion or resolve causal frontiers.

**Evidence:** Stabilization P0 regressions in
`tests/thor_evidence_v2_ram_test.py` and `tests/thor_evidence_v1_canary_test.py`.

# ADR-0059 — explicit V1/V2 target bridge during stabilization
**Status:** Accepted for stabilization
**Date:** 2026-09-13

**Context:** The canary result carried the legacy V1 target graph and concrete
V2 RAM byte outputs in separate namespaces. A validator that inspected only
the V2 branch could accept a detached legacy explanation.

**Decision:** Emit and validate one identity-checked `causal_bridge` that binds
the legacy FF13CC value-version, the concrete MOVE_LONG RAM operation, all four
byte-version outputs, and the shared write witness. Missing or detached bridge
data fails closed.

**Consequences:** The bounded canary has an explicit V1-to-V2 handoff. This
does not infer input causality, access-width capabilities outside the checked
operation, or whole-program provenance.

**Evidence:** `test_ram_target_requires_v1_v2_bridge` and the dedicated
`canary_validation.py` validator.

# ADR-0060 — completed epoch boundary is part of coverage attestation
**Status:** Accepted for stabilization
**Date:** 2026-09-13

**Context:** Event hashes and contiguous sequence numbers alone could still
describe a truncated prefix that had been relabeled as a complete coverage
interval.

**Decision:** A verified coverage basis must carry an attested `EPOCH_END` with
`data.reason == COMPLETE`; missing completion, NOTE substitution, unknown
effects, alias or overlap markers fail closed.

**Consequences:** The coverage contract distinguishes a sealed complete epoch
from a syntactically coherent fragment. The contract still does not claim
global writer completeness or causal input provenance.

**Evidence:** `test_coverage_rejects_note_substitution_and_unknown_effect`.

# ADR-0061 — RAM SQLite root/output semantic separation
**Status:** Accepted for stabilization
**Date:** 2026-09-13

**Context:** RAM persistence validated hashes and output references but did not
fully reject a root byte being repurposed as a write output.

**Decision:** Import requires root versions to have an allowed root origin and
no predecessor; operation-bound versions must be WRITE_OPERATION outputs at
the operation temporal sequence. Coverage intervals and addresses are also
shape-checked before insertion.

**Consequences:** SQLite cannot silently reinterpret initial state as a writer
or accept malformed coverage bounds. Transaction rollback remains the failure
boundary.

**Evidence:** `test_sqlite_rejects_root_as_write_output`.

# ADR-0062 — preserve explicit FF13CC capability frontier
**Status:** Accepted for stabilization
**Date:** 2026-09-13

**Context:** A `PROVEN` FF13CC target could lose its capability frontier if
validation ignored the explicit UNKNOWN list.

**Decision:** The bounded canary validator requires the five declared
capabilities (`access_width`, `overlap_range`, `same_value_writers`,
`irq_exception`, `input_reads`) to remain present and UNKNOWN, and rejects an
input-causal edge or PC heuristic.

**Consequences:** The canary can prove its checked byte write while retaining
the unclosed interrupt, input and broader memory frontiers.

**Evidence:** `test_ram_target_requires_v1_v2_bridge` and the RAM validator.

# ADR-0063 — held-out receipt type strictness
**Status:** Accepted for stabilization
**Date:** 2026-09-13

**Context:** The held-out runtime join coerced arbitrary truthy values with
`bool(...)`, so a malformed string such as `"false"` could become OBSERVED.

**Decision:** Runtime receipts require exact boolean reachability, nonnegative
frame/sequence integers, nonempty string addresses and backend/scenario fields.
Malformed receipts fail before the UNKNOWN join is evaluated.

**Consequences:** The 03BDD8 frontier remains UNKNOWN when the receipt is
incomplete or malformed; no static extent or ownership promotion is inferred.

**Evidence:** malformed-receipt cases in `tests/thor_evidence_v8_test.py`.

# ADR-0064 — duplicate capture imports are idempotent in the manifest
**Status:** Accepted for stabilization
**Date:** 2026-09-13

**Context:** SQLite import is idempotent, but the V9 operational manifest could
list the same trace more than once after a repeated capture call.

**Decision:** Keep one trace identity per operational cycle while allowing the
underlying transactional import to remain idempotent.

**Consequences:** Replays cannot look like independent captures in the
manifest. A different trace is still rejected as a splice.

**Evidence:** duplicate capture assertion in `tests/thor_evidence_v9_test.py`.
# ADR-0066 — Repository-driven multi-scenario campaign

Date: 2026-09-13

Status: Accepted for M12 developer-only evidence tooling.

AUTO66 reuses AUTO65's normalized chain, novelty, prefix and closure engine.
The new layer owns only scenario-pool fingerprinting, fixed-point transition,
equivalent-capture rejection, bounded discovery dispatch and per-scenario
accounting. Scenario files are the authority for deterministic inputs and
watch configuration; Luna does not provide an address or manually choose a
chain. A sealed report may be replayed to repair a persisted receipt, but that
operation is not counted as new runtime evidence. This preserves the
ROM-loader/evidence/contract boundary and keeps knowledge non-owning.

The first accepted campaign stops after the repository's two useful scenarios:
the AUTO65 capture is the fixed-point starting point, and the scheduler selects
the distinct idle scenario, which adds one bounded ROM-activity branch. A new
savestate or gameplay state is required before further scenario coverage.

# ADR-0067 — Live opportunistic RE machine and snapshot operator view

Date: 2026-09-13

Status: Accepted for M12 developer-only evidence tooling.

AUTO67 replaces scenario-driven runtime dispatch with current human-gameplay
sampling. The emulator publishes a fixed rolling window of minimal events;
unclaimed observations are overwritten rather than placed in a persistent raw
event queue. One Dispatcher atomically performs preliminary identity, known
coverage and active-claim checks, then leases work through one mailbox per
worker. Investigations and worker leases are separate, so WAITING_RUNTIME,
KNOWN, MERGED, PROVEN, BLOCKED and EXHAUSTED work releases its worker.

The operator view is served by the same launcher and reads bounded status
snapshots only. UI publication is outside callbacks and does not hold the
Dispatcher claim lock while workers run. Live observation remains non-owning;
M12 promotion contracts and canonical byte-exact reconstruction remain
authoritative.

Evidence: `tests/thor_evidence_auto67_test.py` and
`docs/reports/THOR_M12_AUTO67_LIVE_OPPORTUNISTIC_RE.md`.

2026-09-13 operator-test correction: the original implementation serialized
all investigations and its dashboard displayed static transition examples.
The performance repair limits UI publication to recent rows and real bounded
worker history, with file/HTTP work in a separate publisher. Burst capture is
opt-in because lost writes can break causal chains; continuous capture already
samples every 16th write and is not a complete trace either. No task dispatch,
observed-PC identity or worker return counter constitutes a causal proof.
AUTO67's current worker path records unresolved observations rather than
executing the full provenance engine. Completion of that integration is a
separate outstanding task; this repair does not silently change the RE model.

## AUTO67.1 — Fixed frozen capsule experiment

Date: 2026-09-13

AUTO67.1 keeps the existing rolling-window and claim authority but adds a
developer-only pool of exactly sixteen reusable 128 KiB capsules. At most four
capsules can be live targeted captures at once. A capsule freezes before
analysis, is released on every result including KNOWN/MERGED and unresolved
outcomes, and returns to FREE for reuse. BizHawk receives bounded targeted
hooks with a per-frame callback budget; discovery remains a replaceable ring,
not a raw-event FIFO or per-event UI stream. Capsule records are immutable
after freeze and are not part of dashboard payloads.

This is an explicit lossy experiment: a 128 KiB capsule and callback budget
cannot establish causal completeness or chain closure. The existing AUTO67
dashboard is extended with capsule status and worker history rather than a
second UI. No production runtime, SOURCE_OWNED bytes, or AUTO68 work is
introduced.

AUTO67.1 operator correction: the default view is a separate native
`auto67_window.py` child process consuming the same replaceable status snapshot.
It is kept outside callbacks, Dispatcher claims, workers and knowledge
persistence. The prior HTTP dashboard remains available only when explicitly
requested with `--view-mode browser`; this keeps the operator UI choice from
changing the live RE data path.

## AUTO67.2 — Continuous hunt with scarce focused capture

The live worker pool must keep hunting current bounded rolling-window evidence
while focused BizHawk capture slots are occupied. A capsule is optional for a
lease: when the configured focused-capture limit is reached, the worker takes
the fresh candidate through quick check and analysis instead of waiting behind
the hook. The default focused-capture count is one; higher counts remain an
explicit measurement setting. Sampling remains lossy and never proves absence;
no raw-event backlog is permitted. The decision is based on the AUTO67.2 real
run: 16 workers reached peak busy 16, recorded 169 hunt-success assignments
after returns, and produced no frame over 33 ms with one focused slot.

## AUTO67.3 — Persistent chain store first through the existing sidecar

Date: 2026-09-13

Status: Accepted for M12 developer-only evidence tooling.

The AUTO67.2 in-memory yield ledger was not durable evidence: its generic
observation/context/edge counters could increase once per lease, and the live
worker completion path incremented `new_edges` unconditionally. It is disabled
from the acceptance path. AUTO67.3 keeps the runtime, capsule and dispatcher
architecture unchanged and establishes persistent completed-chain storage
before any DB-growth or semantic-yield analysis.

Completed workers hand a compact descriptor to one bounded nonblocking queue.
Only its background writer opens the existing `Store` connection and extends
the same SQLite sidecar with live session/context/observation/obligation rows.
The AUTO67.3 acceptance path is now chain-store-first. The former
knowledge-yield analyzer and seed/context durable classifications are disabled;
before worker execution the Dispatcher rejects only an active claim (plus its
normal already-dispatched window token). A completed worker result becomes an
immutable canonical chain record. Its SHA-256 hash excludes frame, epoch,
worker, lease and session identifiers, while those values remain provenance.
Exact hashes update bounded observation metadata in `live_chain`; the chain body
is never duplicated and similar chains are not semantically merged. Unresolved
chains are valid records. The native window reports objective store growth only:
unique chains, session new, exact duplicates, writes/errors, unresolved and
rooted counts. No raw-event backlog, second database, dispatcher SQLite call,
or SOURCE_OWNED promotion is introduced.
# ADR-0067.4 — bounded frozen capsule materialization
**Status:** Accepted for AUTO67.4
**Date:** 2026-09-14

**Context:** AUTO67.3 proved that the focused Lua capsule contained richer
runtime records than the seed-only worker descriptor, but the authoritative
worker path did not read the frozen body.

**Decision:** Version the existing capsule file as O67V v2, retain a bounded
O67C decoder for historical files, validate capsule/lease identity in the
worker, and materialize only observed runtime records into separate evidence
fields. Persist new descriptors as `MATERIALIZED_CHAIN`; preserve historical
rows as `SEED_ONLY`. The unresolved frontier remains explicit.

**Consequences:** The existing dispatcher, hot workers, bounded queue and
SQLite sidecar remain in place. No semantic merging, ownership promotion,
SOURCE_OWNED change, raw-event backlog or AUTO68 work is introduced.

**Evidence:** `docs/reports/THOR_M12_AUTO67_4_FROZEN_CAPSULE_REAL_WORKER_CHAIN.md`.

## ADR-0067.5 — Separate observed runtime facts from causal dependencies

**Status:** Accepted for AUTO67.5

**Context:** AUTO67.4 placed additional BUS_WRITE_PC capsule observations in a
field named `causal_facts`. Those records establish that a PC/address pair was
observed, but do not establish a producer, register, RAM-version or control
dependency.

**Decision:** Version new materialized payloads as schema 2. Store capsule
observations in `runtime_observations` and direct witnessed pairs in
`observed_facts`. Emit `causal_facts` only from generic, fail-closed static
M68K instruction semantics. Do not create `chain_steps` without an ordered
dependency proof; leave register/RAM provenance in `unresolved_frontier`.
Historical AUTO67.4 databases remain read-only and interpretable.

**Consequences:** The 0x0027EC canary proves a MOVE memory-write source and
destination-address dependency from the ROM opcode, but not the A4/A5 register
origins. The unrelated 0x06009A seed remains unresolved because its opcode is
not a supported memory-writing MOVE. No semantic merging, SOURCE_OWNED change,
ownership promotion or AUTO68 work is introduced.

**Evidence:** `docs/reports/THOR_M12_AUTO67_5_CAUSAL_INTEGRITY_GATE.md`.

## ADR-0067.6R — Targeted register predecessor evidence

**Status:** Accepted for AUTO67.6R

**Context:** AUTO67.5 proved generic instruction source and destination
semantics but intentionally left A4/A5 register provenance unresolved. The
existing focused BizHawk hooks already provide the minimum mechanism for a
bounded predecessor slice.

**Decision:** Add a versioned O67P v1 sidecar only for investigations whose
static facts require A4/A5. Reuse `event.on_bus_exec_any` and targeted
`emu.getregister` reads, retain only a bounded predecessor ring, and resolve a
register only when epoch, sequence, consumer occurrence, interval completeness
and static producer semantics all agree. Reject overwrite, gap, truncation,
unsupported decode and identity mismatch. Do not migrate historical AUTO67.4
data, merge graphs, change SOURCE_OWNED or begin AUTO68.

**Consequences:** The worker can emit the first register reaching-definition
step when a complete real interval exists. The AUTO67.6R real run currently
fails closed because the canary predecessor interval is incomplete; no producer
PC is inferred. The bounded negative result is authoritative until a later
task changes capture scheduling explicitly.

**Evidence:** `docs/reports/THOR_M12_AUTO67_6R_TARGETED_REGISTER_PREDECESSOR.md`.
# ADR-0067.6R2 — one global bounded prehistory ring
**Status:** Accepted as AUTO67.6R2 checkpoint; performance follow-up required
**Date:** 2026-09-14

**Context:** AUTO67.6 targeted predecessor hooks started too late to recover
the history before a BUS_WRITE consumer. A per-investigation execution-hook
fan-out was already known to damage BizHawk responsiveness.

**Decision:** Use one continuously active developer-only global
`event.on_bus_exec_any` hook with a bounded 4096-record compact ring. At an
exact discovery/execution join, freeze one bounded O67P v2 slice and read only
the requested A4/A5 values at the consumer. Keep discovery sequence and
execution sequence separate. Because BizHawk's second callback argument is a
bus value, resolve v2 instruction semantics from canonical ROM bytes at the
captured runtime PC; do not treat that callback value as an opcode.

**Consequences:** AUTO67.6R2 can prove the first real generic reaching-
definition steps without a per-worker hook or raw-event backlog. The canary
proves A4/A5 reaching definitions. The real run also measures a hard negative:
the global Lua callback produces 120/120 frames over 50 ms and a 679 ms maximum,
so increasing ring capacity is not an optimization and smooth gameplay is not
accepted by this checkpoint. No semantic merge, SOURCE_OWNED change or AUTO68
work is implied.

**Evidence:** `docs/reports/THOR_M12_AUTO67_6R2_CONTINUOUS_PREHISTORY.md` and
its machine-readable JSON proof.

## ADR-0067.6R3 — existing low-overhead source audit
**Status:** Negative checkpoint; no source accepted
**Date:** 2026-09-14

**Context:** AUTO67.6R2 proves the A4/A5 reaching-definition contract through a
bounded global Lua execution ring, but its real QuickSave1 run is not
gameplay-safe. Before adding capture machinery, existing BizHawk TraceLogger,
native GPGX hooks, hybrid block translation and helper/plugin paths were
audited.

**Decision:** Do not increase the ring and do not add a new source in R3. The
only connected complete source is `event.on_bus_exec_any`, and it fails the
performance gate. BizHawk TraceLogger exposes no machine-readable bounded
stream. The existing GPGX `HOOK_CPU`/block hook is a separate developer-only
libretro path without BizHawk QuickSave1 integration; existing hybrid blocks
are fixed translation proofs, not a generic ordered runtime history. A native
BizHawk helper/plugin would be new architecture and is outside this checkpoint.

**Consequences:** R2 semantic correctness remains accepted, while normal
gameplay performance remains blocked. No AUTO68, semantic merging,
`SOURCE_OWNED`, chain depth, database schema, or runtime architecture changes
are introduced. The next source must attach to the authoritative BizHawk path
and preserve exact block/order/branch/interrupt fail-closed evidence.

**Evidence:** `docs/reports/THOR_M12_AUTO67_6R3_LOW_OVERHEAD_PREHISTORY.md`.

## ADR-0067.6R3B — targeted producer-triggered execution burst
**Status:** Accepted as a positive bounded feasibility checkpoint
**Date:** 2026-09-14

**Context:** AUTO67.6R2 proved the reaching-definition contract with a global
execution ring, but its always-on Lua callback was not gameplay-safe. The
targeted-burst hypothesis keeps the global hook absent during idle periods and
opens it only around the two known static producer PCs.

**Decision:** Add a developer-only bounded burst source with two targeted
producer hooks, one active global hook slot, a 64-record maximum, exact stop at
consumer `0x0027EC`, and fail-closed budget exhaustion. Retain at most 16
flushed O67P v2 slices. Do not increase the continuous ring, merge graphs,
change SOURCE_OWNED, redesign workers, or begin AUTO68. Do not expose a path
for a bounded slice unless the sidecar was actually flushed.

**Consequences:** The real QuickSave1 gate proved both generic reaching
definitions for the canary while reducing frame maximum from the R2 679 ms
failure to 23 ms, with zero frames over 50 ms. The bounded proof retains only
16 sidecars; exact joins beyond that retention are not advertised as persisted
paths. The targeted source is accepted for this feasibility checkpoint and is
not a claim of full-ROM capture coverage.

**Evidence:** `docs/reports/THOR_M12_AUTO67_6R3B_TARGETED_BURST_FEASIBILITY.md`
and its machine-readable JSON proof.

## ADR-0067.6R3C — generic static register-writer targeting
**Status:** Negative checkpoint; generic full-ROM targeting rejected by the
performance gate
**Date:** 2026-09-14

**Context:** R3B proved the bounded burst semantics around two producer PCs,
but its runtime target list was not yet generated from the full existing
static writer set. R3C required removing those runtime literals and measuring
the generic static candidate set before accepting a broader hook installation.

**Decision:** Reuse `register_writes()` to enumerate statically proven A4/A5
writer candidates and generate the bounded runtime target file. Keep the Lua
burst source fail-closed and producer-agnostic. Stop the generic acceptance
gate after a real 500-hook prefix reached 63 ms maximum frame time and one
frame over 50 ms. Do not install the remaining 30,042 hooks, redesign the
capture path, increase SOURCE_OWNED, merge graphs, or begin AUTO68.

**Consequences:** The generic configuration path is proven to be generated and
contains both canary writer PCs without Lua hard-coding. Generic gameplay
acceptance is negative at the measured bounded scale. The earlier R3B semantic
proof remains valid and is not replaced by an unsafe full-ROM run.

**Evidence:** `docs/reports/THOR_M12_AUTO67_6R3C_GENERIC_REGISTER_WRITER_TARGETING.md`
and its machine-readable JSON.

## ADR-MAP-1 — Persistent global provenance cartographer

**Decision:** Add a small generic SQLite graph beside (not inside) the historical
AUTO67 persistence schema. Import only already-proven machine-readable evidence;
canonicalize static ROM entities, retain runtime occurrence/value-version scope,
and keep OBSERVED/PROVEN/UNRESOLVED/CONFLICT distinct.

**Consequences:** CPU/register, SAT/DMA, graphics/resource, and selector/control
evidence can share one deterministic persistent graph with lineage and frontiers.
Legacy proof databases remain readable and untouched. No semantic merging,
runtime capture, or SOURCE_OWNED promotion is introduced.

**Evidence:** `docs/reports/THOR_M12_MAP_1_GLOBAL_PROVENANCE_CARTOGRAPHER.md` and
`tests/thor_evidence_map1_test.py`.

## ADR-WALKER-1 — Bounded two-window sparse advancement

**Decision:** Accept ASM Walker only as a bounded developer-only transport for
validated blocks. The first experiment uses boundary guards around one
straight-line body, disables `event.on_bus_exec_any` while that body executes,
and re-enables it for one bounded successor window. Cartographer receives
stable static identities; run, restore, capture, and fragment identities remain
in the runtime evidence and are required by join validation.

**Consequences:** The QuickSave1 checkpoint proved one real off-body interval,
one non-oracle W2 dependency, positive first merge, zero-growth identical
replay, and fail-closed rejection of five false joins. This does not establish
ROM-wide scaling or exact data-writer attribution. Any future block must supply
its own static contract and remain bounded; a missing contract is a frontier.

**Evidence:** `docs/reports/THOR_M12_WALKER_1_TWO_WINDOW_SPARSE_ADVANCEMENT.md`,
its JSON proof, and `tests/thor_evidence_walker1_test.py`.

## ADR-DISPATCHER-1 — Occurrence-only AUTO67 leasing

**Decision:** Remove branch-level active suppression from the AUTO67
Dispatcher. A `branch_fingerprint` remains diagnostic metadata, while
`dispatch_state=LEASED` on the exact current `RollingWindow` item is the only
event-level redispatch guard. Free workers may therefore lease simultaneous
runtime occurrences with identical kind/address/PC.

**Identity:** `live_opportunistic.lua` supplies monotonic `epoch + seq` as the
runtime occurrence identity. The Dispatcher adds a bounded `window_item_id`
for the exact stored window item; investigation and lease identifiers include
both identities. This is runtime occurrence identity only and does not assert
causal sameness or semantic novelty.

**Consequences:** The existing one-mailbox-per-worker model and capsule limits
(`16` total, `4` live) remain unchanged. Capture-slot exhaustion still uses
`WAITING_CAPTURE_SLOT`; no raw-event backlog or per-worker queue is added.
Compatibility metrics for active collisions, duplicate active claims, and
pre-dispatch merges remain present but obsolete and are no longer incremented.

**Evidence:** `docs/reports/THOR_M12_DISPATCHER_1_OCCURRENCE_ONLY.md`, its JSON
counterpart, and `tests/thor_evidence_dispatcher1_test.py`.
# ADR-AUTO67-WORKER-CLEAN-1 — Keep Worker factual and bounded
**Status:** Accepted for M12 AUTO67
**Date:** 2026-09-15

**Context:** AUTO67.occurrence-only leasing already made the Dispatcher the
single claim authority, but the Worker still retained an unbounded investigation
map and assigned semantic novelty/status during processing. That mixed evidence
collection with downstream interpretation and allowed live materialization to
emit prescriptive unresolved frontiers.

**Decision:** Keep only the bounded 16-entry `recent_investigations` deque.
Worker outcomes are factual capture/decode/materialization observations;
`known_during_work` and Worker global KNOWN/PROVEN/DUPLICATE/MERGED decisions
are removed. Capsules are temporary resource containers released by id and no
longer carry semantic flags. Live materialization retains observed and proven
causal facts plus bounded register/capture diagnostics, while omitting
`unresolved_frontier` and `next`. Persistence remains an optional downstream
sink and occurrence provenance is preserved in descriptors.

**Consequences:** Worker completion and capsule release are independent of
persistence availability. Existing Dispatcher occurrence identity, exact-item
LEASED guard, rolling window, mailbox shape, and capsule limits remain unchanged.
Semantic deduplication and any future frontier scheduling stay downstream.

**Evidence:** `docs/reports/THOR_M12_AUTO67_WORKER_CLEAN_1.md`.
# ADR-AUTO67-RING-CLEAN-1 — Current-event rings have no semantic state
**Status:** Accepted for M12 AUTO67
**Date:** 2026-09-15

**Context:** After Worker cleanup, the Python `RollingWindow` still exposed a
`retained` counter and a misleading `max_utilization` snapshot field. The
current-event ring must remain a temporary bounded scheduling window, separate
from the Lua transport ring and from the predecessor ring.

**Decision:** Remove `RollingWindow.retained`, `rolling_window.retained`,
`max_utilization`, and the unused `current()` method. Keep only capacity,
items and overwrite count, while preserving exact-item `dispatch_state=LEASED`
and all occurrence identity fields. Leave the Lua ring implementation intact
because its bounded overwrite-oldest/order/identity contract already passes the
focused fixture audit.

**Consequences:** The Python ring cannot be mistaken for proof retention or a
backlog. Lua remains the transport window and Python remains the Dispatcher
current-event window; neither ring is merged or enlarged. Dispatcher, Worker,
CapsulePool, Cartographer, MAP-1, Walker-1, AUTO68, SOURCE_OWNED and C++ are
unchanged.

**Evidence:** `docs/reports/THOR_M12_AUTO67_RING_CLEAN_1.md`.

# ADR-AUTO67-PREDISPATCH-TRANSPORT-CLEAN-1 — Identity cursor at the status boundary
**Status:** Accepted for M12 AUTO67
**Date:** 2026-09-15

**Context:** The launcher previously tracked only one process-local `seq` while
reading replaceable Lua snapshots. That left epoch rollover implicit and made
the pre-dispatch transport contract depend on ad-hoc runner code.

**Decision:** Put snapshot consumption in `PreDispatchTransport`. Keep one
identity cursor `(epoch, seq, occurrence_id)`, accept only unseen monotonic
records, and drop duplicate, stale, or malformed records. Support `events` and
legacy `discovery` as input aliases. Return records immediately to Dispatcher;
do not retain raw events, perform semantic lookup, or claim work in transport.

**Consequences:** Epoch changes cannot be lost when sequence numbers restart,
and replaceable snapshot loss remains intentional and bounded. Dispatcher,
Lua rings, predecessor capture, Worker, CapsulePool, Cartographer, MAP-1,
SOURCE_OWNED and production runtime behavior remain unchanged.

**Evidence:** `docs/reports/THOR_M12_AUTO67_PREDISPATCH_TRANSPORT_CLEAN_1.md`.

# ADR-M12-POSTRUN-PROGRESS-HEARTBEAT-2I1 — Truthful post-run completion
**Status:** Accepted for M12 2I.1
**Date:** 2026-09-19

**Context:** The compact 2I analyzer wrote only a stage name and the post-run
window treated every `PASS...` report as full completion. Long SQLite/FLOW
work therefore looked frozen and could display `ANALYSIS COMPLETE` while
later pipeline stages had not run.

**Decision:** Publish an atomically replaced JSON snapshot at a bounded 4 Hz
heartbeat. Each of the nine existing stages has an explicit state and factual
N/M counters when a denominator exists; unknown totals use indeterminate UI
animation. The UI derives heartbeat age and progress age independently,
keeps raw JSON behind DETAILS, and reports compact-master-only output as
`PARTIAL ANALYSIS COMPLETE`. Backend failure is terminal and does not kill or
depend on the UI process.

**Consequences:** A reopened window can restore the latest stage and counters
from disk, closing the window does not stop the analyzer, and a stale backend
is visible instead of an endless spinner. FLOW_V1, Worker lifecycle/scaling,
END GAME boundary behavior, rolling-master semantics, canonical truth, 2E/2F
promotion rules, and SOURCE_OWNED are unchanged.

**Evidence:** `tests/live_forward_postrun_progress_test.py` and
`docs/reports/THOR_M12_END_GAME_ROLLING_MASTER_2I.md`.

# ADR-M12-END-GAME-ROLLING-MASTER-2I — Graceful close and compact post-run master
**Status:** Accepted for the developer-only 2I compact orchestration pass
**Date:** 2026-09-19

**Decision:** The separate Worker Control window writes one atomic `END_GAME`
sentinel. Lua consumes it only between complete Worker rounds and exits through
the supported EmuHawk client path. After the sealed receipt, a background
post-run process validates the raw/index hashes and writes a compact SQLite
rolling generation containing deduplicated instruction-PC facts, control edges,
terminal facts, occurrence counts and run provenance. A hash-bound `current.json`
pointer is replaced only after SQLite integrity validation.

The compact pass keeps raw FLOW and the segment index until the canonical
Cartographer/Archivist/decoder refresh is independently available. It does not
change FLOW_V1, Worker lifecycle/scaling semantics, SOURCE_OWNED or production
AUTO67.

**Evidence:** `docs/reports/THOR_M12_END_GAME_ROLLING_MASTER_2I.md` and the
ignored rolling generation under `build/thor-evidence/`.

# ADR-M12-CANONICAL-ROM-KNOWLEDGE-MAP-2D — Separate ranges, facts and emission
**Status:** Accepted for the 2D checkpoint
**Date:** 2026-09-18

**Context:** Accepted M12 reconstruction classifications, exact emission
intervals, Carver hypotheses and 2B runtime linkage existed in separate
artifacts. AUTO60/Carver reported 1,427,873 `SOURCE_OWNED` bytes while the
accepted current manifest reported 1,475,600 bytes; runtime occurrence
lineage also must remain out of canonical object identity.

**Decision:** Build a deterministic, ROM-SHA-scoped SQLite knowledge layer
with separate emission intervals, canonical ROM ranges/objects, claims,
relations and evidence references. Reconcile AUTO60 to AUTO61 through the
accepted GFX-2/GFX-MAX/AUTO61 manifests before importing current ownership.
Keep Carver findings as `HYPOTHESIS`; import each of the 330 exact 2B
instruction ranges once and reference its occurrence lineage through saved
artifact hashes. An `EXECUTED_NEXT` relation requires captured M68K instruction
objects at both endpoints; exception-event edges remain in source evidence but
are not miscast as instruction adjacency. Preserve terminal `next_pc` facts
as address-only `OBSERVED_NEXT_PC`. No runtime campaign, hook, ownership
promotion, or Worker/FLOW/AUTO67 change is part of this checkpoint.

**Consequences:** The map covers all 3,145,728 ROM bytes with a disjoint exact
emission partition, while UNKNOWN remains explicit. Stable object identity
depends on ROM hash, byte bounds and type, not run/Worker/capture identity.
Raw session/FLOW data and the working SQLite database remain local; the
committed receipt provides deterministic structure, evidence references,
metrics and hashes for independent review. This checkpoint does not complete
the broader M12 reconstruction milestone or begin M13.

**Evidence:** `docs/reports/THOR_M12_CANONICAL_ROM_KNOWLEDGE_MAP_2D.md`, its
JSON receipt and `tests/rom_knowledge_map_test.py`.

# ADR-M12-AUTO67-LIVE-FORWARD-WORKER-1A — One shared stream, one live Worker
**Status:** Accepted for the bounded developer-only 1A prototype
**Date:** 2026-09-17

**Context:** The existing AUTO67 native snapshot freezes an earlier execution
window and its Worker input can lack a later native instruction stream. The
first live-forward proof must start at the CPU's current instruction boundary
without waiting for predecessor resolution or adding callbacks to each
instruction.

**Decision:** In the isolated BizHawk 2.11.1/GPGX developer build, record the
main M68K stream once into a bounded native ring. A single Worker attaches at a
future instruction boundary, stores full ENTRY/EXIT CPU state, counts observed
control-flow transitions, and seals an exact copied range on depth, memory,
retention, CPU-stop, or capture-error conditions. The result is immutable and
is validated outside the instruction hot path before it is called
`READY_FOR_CARTOGRAPHER`.

**Consequences:** This proves a local factual FLOW_V1 segment only. The separate
Sega CD sub-CPU remains unsupported; predecessor resolution, Cartographer,
production AUTO67, `SOURCE_OWNED`, and the project roadmap do not change. A
multi-Worker or Cartographer-adapter design needs a later explicit checkpoint;
1A stops here.

**Evidence:** `docs/reports/THOR_M12_AUTO67_LIVE_FORWARD_WORKER_1A.md` and its
JSON receipt.

# ADR-M12-AUTO67-LIVE-FORWARD-WORKER-1B — Bounded multi-Worker scaling
**Status:** Accepted for the developer-only 1B scaling checkpoint
**Date:** 2026-09-17

**Context:** Worker 1A proves one capture descriptor against a shared M68K
stream. Capacity measurements require several different overlapping windows,
while keeping the CPU-side instruction recorder single-copy and the configured
result budget bounded.

**Decision:** Replace the fixed 64-entry singleton lifecycle with a dynamically
allocated, preflighted descriptor/result pool and ordered pending/active queues.
At most one pending descriptor attaches at each confirmed instruction
boundary. Instructions continue to be recorded once in the same shared ring;
each result is copied only into its own bounded slot at completion. Use the
same 64-KiB result budget in both phases and vary depth only for forced
concurrency. Reject a request before allocation when the checked native,
Waterbox, host-transport or process memory budget does not fit. The experiment
is scoped to the explicitly requested counts through 100,000; that scope ceiling
is not reported as a measured capacity limit.

**Consequences:** Phase A reports natural `CAPTURING`, completed and occupied
peaks independently. Phase B reports forced simultaneous capacity separately.
Every tested `WORKER_COUNT` must complete 100 serialized lifecycle cycles on
each Worker. Each segment requires a fresh capture ID and generation, a live
execution epoch, COMPLETE and ANALYZING states, full host validation of the
binary record segment, an exact ACK, and a verified return to FREE before the
next cycle. The gate is `WORKER_COUNT * 100` completed and validated segments;
a pool startup or one successful capture is never a PASS. Depth/memory
terminations remain visible bounded results; a slot is never reused before its
exact ACK. Per-slot native counters prove capture start, completion, first
ANALYZING transition and release across every generation. The host exports each
segment twice while CPU execution advances and compares metadata, ENTRY/EXIT
state and every binary record before accepting it. The single shared M68K stream
remains the only per-instruction trace.
Production AUTO67, predecessor handling, Cartographer, `SOURCE_OWNED` and the
project roadmap remain unchanged. Stop after recording the first factual
natural saturation or forced/resource/correctness limit.

**Evidence:** `docs/reports/THOR_M12_AUTO67_LIVE_FORWARD_WORKER_1B_SCALING.md`
and its JSON receipt.

# ADR-M12-AUTO67-LIVE-FORWARD-CARTOGRAPHER-2A — RAM execution map and Archivist
**Status:** Accepted for the developer-only 2A checkpoint
**Date:** 2026-09-17

**Context:** Worker 1B proves immutable FLOW_V1 execution windows and lifecycle
identity, but does not persist observed execution structure. MAP-1 already owns
stable node/edge identity, lineage union and proof-status ordering.

**Decision:** Add a separate live-forward adapter after the 1B host audit and
before its exact ACK. It accepts only validated FLOW_V1 record sequences,
creates ROM-scoped instruction/event nodes and `OBSERVED` `EXECUTED_NEXT` edges,
and records each execution's run/epoch/Worker/capture/generation/hash/bounds
lineage. The RAM Cartographer stages per-segment lineage in its in-memory SQLite
and folds it into MAP-1 rows once after emulator exit, before graph hashing and
backup; no full-graph hash or proof-component scan runs before each ACK. Each
runtime starts with a fresh RAM Cartographer and never reads the master. After
EmuHawk exits, SQLite backup saves a retained session; a separate
Archivist validates MAP-1/session identity and uses the existing atomic merger
to seed a missing master or merge a compatible session. New merge conflicts
fail before atomic replacement.

**Consequences:** Repeated structure and overlapping Worker windows merge into
one graph while each observation remains in lineage; distinct targets remain
branch alternatives. Runtime facts remain `OBSERVED`, cannot self-promote to
`PROVEN`, and do not change SOURCE_OWNED. The proof adapter and production AUTO67
path remain independent. Worker/native/Lua instruction hooks are unchanged.

**Evidence:** `tests/live_forward_cartographer_test.py` and the two-run
`M12-AUTO67-LIVE-FORWARD-CARTOGRAPHER-2A` receipt.

# ADR-M12-ROM-RANGE-LINKAGE-2B — Exact executed ROM instruction ranges
**Status:** Accepted for the developer-only 2B checkpoint
**Date:** 2026-09-17

**Context:** Worker 1B FLOW_V1 records preserve the runtime PC and opcode, while
Cartographer 2A preserves the observed execution graph. Neither currently
proves which canonical ROM bytes encode each captured instruction or records
the terminal `next_pc` as an address-only fact.

**Decision:** Reuse the existing bounded M68K decoder and explicit 24-bit
memory-region resolver in a developer-only native batch helper. After one
bounded 1B run completes, project supported records into stable,
ROM-SHA/start/end/full-bytes `ROM_INSTRUCTION_RANGE` nodes and
`OBSERVED` `EXECUTED_FROM_ROM` edges. Preserve run/epoch/Worker/capture/
generation/record identity in edge lineage, and represent each segment's
terminal `next_pc` through an `OBSERVED_NEXT_PC` edge to an address node only.
Non-ROM locations remain unlinked; unsupported exact lengths remain unresolved
and prevent a PASS result. Export observed and unique byte coverage separately
from source ownership, and independently reconcile every saved instruction
occurrence and terminal fact against raw records, canonical bytes, a fresh
bounded decode and persisted MAP-1 identities.

**Consequences:** Runtime evidence can identify exact executed instruction
encodings without claiming complete ROM classification, `SOURCE_OWNED`, or
`PROVEN` source knowledge. FLOW_V1 raw records and 2A graph facts remain
unchanged. No Worker/native/Lua hot-path instrumentation, 1B scaling semantics,
production AUTO67, predecessor logic or roadmap status changes.

**Evidence:** `tests/live_forward_rom_link_test.py`,
`tools/bizhawk-native-ring/live_forward_rom_link_audit.py`, and
`docs/reports/THOR_M12_ROM_RANGE_LINKAGE_2B.md` (full runtime and saved-result
acceptance passed).

# ADR-M12-ARCHIVIST-CANONICAL-KNOWLEDGE-PIPELINE-2G — Atomic post-run bridge
**Status:** Accepted for the developer-only 2G checkpoint
**Date:** 2026-09-18

**Context:** Accepted closed MAP-1 sessions could be merged by Archivist and
canonicalized by 2D, but the handoff was manual. Updating the Archivist master
and canonical knowledge DB separately could expose a half-published checkpoint.

**Decision:** Keep Cartographer, Archivist, and 2D responsibilities separate.
Archivist returns a hash-bound merge receipt without inline lineage. A
delta-oriented adapter validates the session/master graph chain and translates
only supported factual MAP-1 records to stable ROM-SHA/range/type identities;
runtime coordinates and occurrences remain evidence locators. Build the
master and knowledge databases as one ignored immutable generation, replay the
import to prove idempotence, independently audit it, then atomically publish a
single `current.json` pointer to the pair. A failed import or audit leaves the
previous pointer and generation untouched. Unsupported factual types stop with
`UNMAPPED_FACT_TYPE`; runtime observations cannot change SOURCE_OWNED or the
emission partition. 2E pointer, offset, and selected table relations are
accepted only from exact-byte synthetic fixtures until a canonical witness
exists.

**Consequences:** Canonical knowledge can be refreshed automatically after a
closed session with rollback-safe publication. No CPU/runtime callback,
Worker/FLOW behavior, production AUTO67, predecessor semantics, ownership, or
emission classification changes. Synthetic relation fixtures remain isolated
from the canonical Beyond Oasis map.

**Evidence:** `tests/rom_knowledge_pipeline_test.py`,
`docs/reports/THOR_M12_ARCHIVIST_KNOWLEDGE_PIPELINE_2G.md`, and its compact JSON
receipt.

# ADR-M12-LIVE-WORKER-CONTROL-WINDOW-2H — Snapshot-only operator window
**Status:** Accepted for the developer-only 2H checkpoint
**Date:** 2026-09-18

**Context:** The accepted 1B runtime already owns exact Worker transitions,
per-Worker lifecycle counters, and the authoritative native allocation
planner, while the accepted 2G launch provides canonical Beyond Oasis
execution. Operators need live, legible status and a way to persist a future
configuration without changing a running pool.

**Decision:** Keep the operator window in a separate process. It reads one
replaceable, bounded status snapshot and writes only a deterministic next-run
configuration plus a replaceable preview request. The current launcher
captures its configuration once before EmuHawk starts. During execution Lua
may call only the existing native memory planner and a new read-only Worker
status query at a bounded rate; it may not resize, reallocate, switch
generations, or wait for settings. The launcher remains responsible for
system/process memory sampling and measured evidence-size rate, and uses the
same resource-budget calculation for preview and startup rejection.

**Consequences:** Closing or slowing the window cannot back-pressure the
runtime; Worker presentation uses native lifecycle state and current
control-flow depth instead of a guessed progress value. Arbitrarily large
positive decimal requests can be saved without UI clamping, then fail closed
at exact native representability, allocation-plan, or process-budget checks.
No CPU hook, Worker/FLOW semantics, production AUTO67, predecessor behavior,
Cartographer/Archivist/2D/2E semantics, SOURCE_OWNED, or emission changes.

**Evidence:** `tests/live_worker_control_test.py` and
`docs/reports/THOR_M12_LIVE_WORKER_CONTROL_WINDOW_2H.md` with its compact JSON
receipt. Five real canonical-ROM campaigns completed 6,400 independently
audited segments; window-off/on timing, current-run configuration immutability,
and next-launch application are recorded in the report.

# ADR-AUTO67-PREDISPATCH-TRANSPORT-CLEAN-1R1 — Final snapshot closes the cursor
**Status:** Accepted for M12 AUTO67
**Date:** 2026-09-15

**Context:** The initial transport cleanup consumed periodic status snapshots
but left the final replaceable Lua snapshot outside the transport cursor. It
also retained a sequence-only state option on the live-opportunistic runner,
and accepted mismatched explicit occurrence identifiers.

**Decision:** Consume `lua_final` with the same `PreDispatchTransport` instance
after emulator exit and before `Dispatcher.stop()`. Validate explicit
`occurrence_id` as exactly `epoch=<epoch>:seq=<seq>`; synthesize it only for
legacy records. Remove `--state` and `OASIS_LIVE_STATE` from this runner path
because `live_opportunistic.lua` has no such consumer. Keep shutdown on the
existing stop event and bounded capsule wait.

**Consequences:** Final-only records reach Dispatcher exactly once, shutdown
does not wait for new emulator data, and identity mismatches fail closed. No
ring, Dispatcher, Worker, capture, persistence, graph, ownership, or C++ path
is redesigned.

**Evidence:** `docs/reports/THOR_M12_AUTO67_PREDISPATCH_TRANSPORT_CLEAN_1.md`.

# ADR-M12-END-GAME-POSTRUN-2I.2 — fail-closed stages 5–9

**Status:** Accepted for developer-only post-run orchestration.

**Decision:** Reuse the existing 2I.1 progress publisher and invoke one
post-run coordinator after rolling-master compaction. The coordinator may
construct a closed MAP-1 session only from a sealed, hash-checked ordered FLOW
spool, and may publish a new canonical generation only through an atomic staged
master/knowledge pair with idempotent replay and unchanged SOURCE_OWNED and
emission hashes. Control provenance inspects ordered FLOW; aggregate rolling
counts cannot supply predecessor evidence. ASM closure, full ROM audit, and
cleanup remain fail-closed until an accepted runner and all required inputs are
present. A STOP retains raw evidence and leaves later stages pending.

**Consequences:** Normal END GAME now enters the real stage 5–9 path and shows
an exact STOP reason when the current tree lacks a required accepted input.
The UI does not claim completion from a displayed stage list. Worker/Lua/FLOW
runtime semantics and production ownership remain unchanged.
# ADR-M12-END-GAME-POSTRUN-2I.2a — terminal state coherence

**Status:** Accepted for developer-only post-run orchestration.

**Decision:** All terminal transitions use one progress finalizer. It clears
the active stage, terminalizes the current stage, blocks later mandatory
stages after STOP/ERROR, persists backend and exception metadata, and writes a
single atomic snapshot. Heartbeat staleness remains non-terminal while the
backend PID is alive. Malformed terminal snapshots are repaired before reuse.

**Consequences:** `COMPLETE`, `PARTIAL_COMPLETE`, `STOPPED` and `FAILED` can
never coexist with an ACTIVE stage. Raw evidence and accepted generations are
retained on STOP/ERROR. This change affects status orchestration and UI display
only; Worker/FLOW/Lua semantics and 2F reconstruction remain unchanged.

# ADR-M12-INTERACTIVE-PREFLIGHT-POSTRUN-GATE — unsealed runs do not analyze

**Status:** Accepted for developer-only interactive orchestration.

**Decision:** Launch the post-run analyzer only after an interactive runtime
has emitted a sealed terminal stop outcome. A resource, native-allocation, or
host-transport preflight rejection is a visible Worker Control result, but it
does not create a Stage 1/9 analyzer window. The rejection receipt records the
exact reason and `RUN_NOT_SEALED`.

**Consequences:** A failed start cannot be misreported as a post-run analysis
failure. The resource reserve and fail-closed allocation policy are unchanged;
only the secondary UI orchestration is gated. Worker, Lua, FLOW, and production
runtime semantics remain unchanged.

# ADR-M12-DESKTOP-MEMORY-FLOOR — one GiB host reserve

**Status:** Accepted for developer-only Worker Control launch preflight.

**Decision:** Reserve one GiB of currently available physical RAM for the host
when calculating the desktop Worker Control native allocation budget. Keep the
existing native/process/core caps and reject when the measured remainder cannot
hold the requested plan. The evidence disk reserve remains independent.

**Consequences:** A 32/64 GiB desktop with a few GiB currently free can start
a small or medium requested Worker plan; severe host pressure still fails
closed before EmuHawk starts. No Worker, Lua, FLOW, or production semantics
change.
# ADR-M12-STAGE5-COMPACT-SESSION — deterministic post-run MAP-1 refresh

**Status:** Accepted for developer-only post-run analysis.

**Decision:** Build one closed MAP-1 session from ordered FLOW in a run-specific
analysis directory. Use compact lineage occurrence summaries, exact ROM-range
decoding, and atomic canonical generations. Validate the accepted rolling-master
proof separately; preserve the canonical emission partition and `SOURCE_OWNED`
bytes exactly, failing closed on identity, ROM, count, or ownership conflicts.

**Consequences:** Repeated observations remain auditable without materializing
millions of duplicate lineage entries. `EXECUTED_NEXT` is emitted only for
adjacent instruction records; terminal next-PC observations remain separate.
Worker/Lua/native scaling semantics are unchanged.
# ADR-M12-STAGE7 — Current-generation MAP-driven executed ASM closure

Date: 2026-09-19

The post-run pipeline consumes the accepted canonical generation directly and
recomputes executed, not-fully-owned M68K candidate islands from current map
objects and observed runtime relations. A candidate is promotable only after
exact range decoding, complete control-flow closure, vasm byte round-trip, and
an independent full-ROM rebuild. Dependency closure is fail-closed; unresolved
islands remain blocked records. Promotion publishes a new immutable generation
and updates the canonical pointer atomically. Stage 8 starts automatically and
rebuilds the resulting split independently. No runtime campaign or production
Worker/FLOW semantics are part of this decision.

# ADR-M12-STAGE6-HANDOFF — receipt-bound control provenance lifecycle

Date: 2026-09-19

Stage 6 must consume the exact sealed receipt and ordered FLOW artifact already accepted by Stage 5. Its result is structured as PASS, NO_DELTA, STOP, or ERROR, including run identity, raw FLOW hashes, relation counters, generation, and failure details. NO_DELTA completes Stage 6 and immediately starts Stage 7; it is not a pipeline terminal state. The existing terminal finalizer handles STOP/ERROR so no terminal snapshot may retain an ACTIVE Stage 6. The Worker, Lua, native pool, and Stage 7 semantics remain unchanged.

# ADR-M12-STAGE7-PREFLIGHT — lineage-bound emission partition handoff

Date: 2026-09-19

Stage 7 preflight must compare the current canonical map with a materialized
emission/ownership manifest selected from the explicit generation parent chain.
A historical path or latest/glob selection is not an authority. The preflight
records both authority hashes and exact category totals, fails closed on any
range, ROM, generation, or ownership mismatch, and keeps UNKNOWN fields outside
the emission partition. This preserves the Stage 7 gate while allowing a
post-2F canonical generation to reuse its own coherent materialized split.
Worker, Lua, native scaling, predecessor, and FLOW semantics are unchanged.

# ADR-M12-STAGE6-STOP — unresolved provenance is not an integrity failure

Date: 2026-09-19

Stage 6 may observe real indirect consumers whose register predecessor or
transform cannot be resolved within bounded evidence. Those events remain in
the diagnostic receipt and count as unresolved/unsupported; they do not stop
the pipeline globally. Global STOP is reserved for ROM identity, FLOW
continuity, impossible occurrence identity, or equivalent evidence-integrity
failures. The result contract remains PASS/NO_DELTA/STOP/ERROR and the exact
receipt is published before terminalization.

# ADR-M12-STAGE9-ABSORPTION — permanent reclaim only after semantic proof

Date: 2026-09-19

Stage 9 may permanently delete run-scoped raw FLOW and reconstruction artifacts
only after the exact run is present in the accepted rolling master, all prior
stages and ROM identities pass, and a streaming semantic-equivalence audit
reconciles every instruction, relation and terminal fact against compact
master rows. Preparation writes an explicit delete manifest and an atomic
absorption receipt before deletion; deletion uses direct filesystem unlink with
bounded WinError 32 retry and no recycle-bin path. Replay consumes the durable
manifest/receipt idempotently, while any missing protected state or failed
post-delete integrity check stops closed and never reports cleanup PASS.

The rolling master, canonical map, ownership/emission state, Stage 5–8
receipts, final report/status and ROM are protected. Worker/FLOW runtime
semantics and SOURCE_OWNED truth are unchanged.

# ADR-M12-R1 — Shadow self-contained MASTER V2

**Status:** Accepted for shadow validation only.

**Decision:** Encode the accepted rolling master, canonical map/knowledge and
Stage 5–9 semantic outcomes into a deterministic, section-hashed MASTER V2
container while leaving all legacy pointers and artifacts authoritative. The
container is independently decoded and compared against a streamed semantic
projection of the legacy inputs. No runtime or cleanup path reads MASTER V2.

**Reason:** R2–R10 require a self-contained authority before replacing the
legacy split state. R1 proves the representation, deterministic size and
corruption detection without changing runtime truth semantics or risking the
accepted `PASS_ABSORBED_RAW_PERMANENT_RECLAIM_V1` path.

**Consequences:** The current accepted master remains the only authority. The
shadow currently measures `1,272,891,699` bytes for run `1789872670`; later
migration stages must account for this size before changing authority.

## ADR-M12-R2 — MASTER V2 canonical read authority (2026-09-19)

**Decision:** Use one `MasterCanonicalView` access layer as the canonical read
source for map, ownership and emission data. The R2 pointer selects a verified,
deterministic MASTER V2 shadow generated from the accepted legacy state. Stage 5
materializes a local scratch generation through the API; Stages 6–8 keep their
existing consumers and semantics. Legacy canonical files remain present and
unchanged for hash comparison only.

**Rationale:** This changes exactly one authority boundary while avoiding a
representation rewrite, runtime capture change, or independent MASTER V2
parsers in individual stages. Fail-closed pointer verification prevents a stale
shadow from becoming canonical. The R2 shadow is not itself committed as a
GiB-scale artifact.

**Validation:** Two real generations were byte-identical and semantically equal
to the current legacy state; API counts, `SOURCE_OWNED`, ownership/emission,
materialized SQLite integrity and targeted Stage 5–9 regressions passed.
# ADR-M12-R3-MASTER-OUTCOME-AUTHORITY-1 — durable provenance and outcomes
**Status:** Accepted for M12 R3
**Date:** 2026-09-19

**Context:** R2 made MASTER V2 the canonical map/ownership/emission read
authority, but Stage 6 provenance, Stage 5–9 receipts and absorbed-run proof
still lived in external JSON/SQLite artifacts.

**Decision:** Add independently hashed MASTER V2 `provenance`, `outcomes` and
`absorption_history` sections and expose them through `MasterOutcomeView` and
`MasterProvenanceView`. Legacy receipts remain shadow comparison inputs. A
missing or corrupt section raises `STOP_MASTER_OUTCOME_UNAVAILABLE`; the read
layer never falls back silently to legacy JSON.

**Consequences:** Accepted provenance, terminal stage outcomes, lineage,
semantic-equivalence proof, deleted-byte totals and deletion mode remain
available after raw FLOW deletion. Runtime capture, Stage 5–9 execution and
SOURCE_OWNED semantics are unchanged. Legacy receipt removal is deferred to a
later checkpoint.

**Evidence:** `build/thor-evidence/master-v2-shadow-r3/r3-acceptance.json`.

## ADR-M12-RUN-CONTRIBUTION-RECONCILIATION — run-scoped V2 idempotency

Date: 2026-09-20

**Decision:** Add a hashed `run_contributions` section to MASTER V2 and a
run-scoped contribution boundary. A contribution is applied once by `run_id`
and its logical hash; an identical replay returns `NO_DELTA_ALREADY_APPLIED`,
while a differing contribution stops with `STOP_RUN_CONTRIBUTION_CONFLICT`.
Occurrence deltas are retained even when a fact already exists globally.
`VERIFY_ONLY` and forensic replay do not mutate counters, legacy generations,
or raw evidence. Historical lineage membership alone remains ambiguous.

**Consequences:** The current V2 startup pointer now verifies 54 ledger entries
without changing canonical, provenance, Stage 5–9, Worker/FLOW or
`SOURCE_OWNED` semantics. Six sealed runs are classified `NOT_COMMITTED` and
are eligible only for a later single-run replay; 28 runs remain `AMBIGUOUS`.
No historical raw/session data is deleted and no mass replay is started.

## ADR-M12-SINGLE-V2-RUNTIME-WRITE-AUTHORITY — emulator post-run promotion

Date: 2026-09-20

**Decision:** Interactive Worker runs launched with the desktop control flow
must use MASTER V2 startup and the in-process post-run coordinator. Stage 5–9
may create only a campaign-local rolling scratch materialized from V2. The
sealed contribution is calculated before cleanup, then the completed Stage 9
result is written through an atomic V2 candidate/pointer replacement. The
legacy persistent `rolling-master-2i` tree is not a write authority.

**Consequences:** Future emulator runs append to the single V2 authority while
preserving Stage 5–9 truth semantics and cleanup behavior. The old subprocess
post-run mode is rejected for interactive runs so it cannot recreate a legacy
rolling master. Legacy canonical shadow verification remains available only
for explicit historical fixtures; current V2 startup does not require those
files.

## ADR-M12-R6-STAGE5-IN-MEMORY-CARTOGRAPHER — Stage 5 RAM session boundary

Date: 2026-09-20

**Decision:** Production Stage 5 uses an explicit `Stage5SessionMemory` backed
by SQLite `:memory:` for semantic FLOW aggregation and MAP-1 construction.
The sealed FLOW spool remains the input authority. Exact decoder exchange files
are OS-managed ephemeral inputs/outputs only; they are not persistent Stage 5
evidence. Canonical candidate/master files remain the durable outputs.

**Consequences:** Normal R6 runs report zero Stage 5 SQLite bytes and zero
persistent Stage 5 temporary files, release the in-memory session before Stage
6–9 continues, and preserve the accepted MASTER V2, ownership/emission,
provenance and cleanup semantics. The old disk session path remains available
only in a named legacy shadow module for comparison and regression.

## ADR-M12-R7-FLOW-IN-MEMORY-HANDOFF — Ordered FLOW without persistent spool

Date: 2026-09-20

**Decision:** The interactive R7 path transfers sealed ordered FLOW segments
through a bounded in-memory queue. One runtime owns the handoff; a consumer
feeds Stage 5 and control-provenance streaming consumers, and the coordinator
closes the RAM session only after the queue drains. The existing disk FLOW
implementation remains available as an explicit legacy/shadow path.

**Consequences:** R7 reports explicit segment/record/hash accounting,
backpressure, and zero FLOW disk reads/writes. Stage 5–9 truth semantics,
canonical refresh, MASTER V2 promotion, ownership, emission and cleanup rules
remain unchanged. The in-memory Cartographer connection is safe across the
runtime consumer and post-run coordinator thread boundary.

## ADR-M12-R8-BOUNDED-STAGE7-DECODE — profile-first analysis optimization

Date: 2026-09-20

**Decision:** Parallelize only the independent external Stage 7 candidate
decodes with a bounded four-worker executor. Keep candidate proof, round-trip
validation, promotion and graph/master merge sequential and ordered exactly as
before. Publish timing and R7 handoff counters in `analysis_performance`.

**Consequences:** The accepted R7 truth boundary is unchanged: Stage 5/6/7/8
meaning, candidate ordering, fail-closed errors, `SOURCE_OWNED`, emission,
cleanup and MASTER V2 authority remain the same. The measured median post-run
wall time fell from `85.656 s` to `60.407 s` (`29.48%`), while Stage 7 fell
from `51.906 s` to a `25.141 s` median (`51.57%`). Runtime coverage can still
produce new facts; the second optimized campaign added 57 instructions and 66
relations with zero source-owned delta.

**Evidence:** R8 baseline and optimized campaign reports under
`build/thor-evidence/live-worker-control-2h/`, plus the focused decode and
post-run coordinator regression tests.

## ADR-M12-R9-MEASURED-DECODE-DEDUP — no cache when requests are unique

Date: 2026-09-20

**Decision:** Measure complete Stage 7 external decode request identity before introducing memoization. The measured accepted workload had 139 unique keys for 139 requests (0% duplication), so no completed-result cache, in-flight coalescing table, eviction policy or persistent cache is added. Instead, reuse the immutable initial/final SQLite snapshot when no promotion can have changed it; retain fresh scans after every promotion.

**Consequences:** Decoder key telemetry remains available in the Stage 7 receipt, including request counts, unique keys, duplicate rate, external wall, pool occupancy and key construction cost. Snapshot reuse preserves candidate ordering, proof, promotion, Stage 8, ownership and MASTER semantics while reducing fresh full-run median Stage 7 wall from `28.906 s` to `16.711 s`. R7/R8 in-memory FLOW and zero-disk guarantees remain unchanged.

**Evidence:** R9 reference and optimized campaigns under `build/thor-evidence/live-worker-control-2h/`, direct sealed-generation profile, and `tests/stage7_decode_test.py` metric/key regressions.

## ADR-M12-R10-STAGE7-DECODER-CONTRACT — explicit range decoder role

Date: 2026-09-20

**Decision:** Keep the Stage 5 PC-list decoder and Stage 7 bounded-range
decoder as separate executable roles. Stage 7 requires the existing
`oasis_re_assemble_range.exe` contract
`ROM START END ASM_OUTPUT JSON_OUTPUT`; startup validation requires its exact
`oasis.stage7.range.v1` capability marker. The PC-list helper remains the
Stage 5 `PCS_INPUT TSV_OUTPUT` tool and cannot be substituted silently.

**Consequences:** Invocation/configuration failures are fail-closed with a
decoder-contract, input, output or tool classification and are no longer
reported as unsupported M68K instructions. Stage 7 still owns bounded decode,
candidate ordering and proof handoff; Stage 8 retains independent full-ROM
assembly/round-trip validation. Cleanup validates both the pre-closure input
generation and the promoted current generation when Stage 7 advances canonical
ownership.

**Evidence:** R10 canonical-ROM fixtures, capability probes, the sealed
`campaign-r10-contract-20260920-1` report, and
`tests/stage7_decoder_contract_test.py`.

## ADR-M12-W1-NATIVE-BUS-EVENT-SIDEBAND

Date: 2026-09-20

**Decision:** Extend the existing 32-byte native ordered ring with explicit
`EVENT` records for 68K data reads, data writes and frame boundaries. The event
subtype occupies otherwise unused kind-flag bits. Bus events use the existing
record fields for address/value and pack value high bits, width, mapped domain
and CPU into `auxiliary`. Opcode and extension fetch helpers are excluded;
32-bit top-level accesses produce one event. Frame events use instruction
sequence zero and carry a monotonic per-epoch frame number.

**Consequences:** Existing instruction/exception records and FLOW projection
remain binary-compatible. Host consumers must preserve raw event records but
filter them when deriving instruction-only control-flow, ROM-link and
Cartographer facts. Domains are emulator memory-map facts only; no sprite,
music, DMA or other semantic resource classification is introduced. The
instrumentation remains developer-only and does not change `SOURCE_OWNED`.

**Evidence boundary:** Native unit/parser tests and source-level GPGX syntax
checks pass. Real WBX build, gameplay counts, retention measurements and
`PASS_NATIVE_BUS_EVENT_SIDEBAND_V1` remain pending until the Waterbox sysroot
and isolated runtime campaign are available.

## ADR-M12-W2-ACTIVE-RESOURCE-CLASSIFICATION

Date: 2026-09-20

**Decision:** Interpret immutable W1 bus events in a separate bounded Worker
analysis layer. Primitive labels are mapped-memory facts only. Instruction
relations require exact ordered event association and a closed decoded
instruction rule; equal values alone are insufficient. VDP register/control
and CPU DMA programming are decoded from observed writes, while internal DMA
payload, resource ownership, sprite identity, song and instrument semantics
remain explicitly unclaimed. S1–S8 inputs are read-only and cross-links require
coherent run/frame/range identity.

**Consequences:** W2 produces deterministic analysis artifacts, bounded
candidates and fail-closed gaps without modifying the native capture path,
ring ABI, Worker scheduling, FLOW semantics, S1–S8 artifacts or
`SOURCE_OWNED`. `OBSERVED` and `DERIVED_EXACT` remain distinct, and
`HYPOTHESIS` is not emitted by this checkpoint.

**Evidence:** `docs/reports/THOR_M12_WORKER_ACTIVE_RESOURCE_CLASSIFICATION_V1.md`
and `build/m12-w2-acceptance/w2-active-resource-classification.json`.

## ADR-M12-W2.1-FRAME-COHERENT-WORKER-EVIDENCE

Date: 2026-09-20

**Decision:** Anchor each accepted Worker segment with `entry_frame` and
`exit_frame` snapshots of the already-authoritative W1 `frame_number`. Equal
snapshots are single-frame only when no contained boundary contradicts them;
changed snapshots require contained FRAME_BOUNDARY markers for exact
partitioning and otherwise remain unresolved. Frame identity is the tuple
`(run_id, epoch, frame)`.

**Consequences:** The fixed 32-byte raw record and native bus semantics remain
unchanged. W2 events, relations, VDP/DMA facts and candidates carry exact
frame identity when the segment proof permits it, otherwise `UNRESOLVED`.
S1–S8 inputs remain read-only and cross-run links are explicitly forbidden.
No graphics meaning, Z80 implementation, Worker scheduling change or
`SOURCE_OWNED` change is introduced.

**Evidence:** `docs/reports/THOR_M12_WORKER_FRAME_COHERENT_EVIDENCE_V1.md`
and `build/m12-w2-1-acceptance/w2-frame-coherent-evidence.json`.

# ADR-M13.2-GENERIC-OBSERVED-CFG-AND-RAW-RETENTION

**Date:** 2026-09-23

**Status:** Accepted for the M13.2 developer-only closure pipeline.

**Context:** The M13.1 closure received millions of normalized FLOW rows but no
decoded instructions, memory facts, or exact range inputs. It emitted no graph
facts, and raw FLOW was deleted before method gaps could be independently
replayed. Existing normalized rows preserve a PC/opcode/next-PC subset, but omit
CPU/domain identity and register snapshots.

**Decision:** The generic closure core may materialize an observed PC-to-next-PC
edge when and only when the source opcode matches canonical ROM bytes. It must
retain an explicit observed-edge truth label; it must not infer static branch
semantics, candidate extents, subsystem meaning, or SOURCE_OWNED changes from
that edge. The live stage must run twice deterministically and emit a hashed
gap ranking. Permanent raw-FLOW reclamation must require successful generic
closure, the ranking artifact, and deterministic replay acceptance.

**Consequences:** Missing instruction identity, CPU/domain, registers, bus
width/value/order, and byte-round-trip boundaries remain unresolved. Observed
CFG recovery can improve evidence navigation but by itself has no measurable
ROM ownership payoff. A failed or missing acceptance artifact prevents cleanup.

**Evidence:** `docs/reports/THOR_M13_2_GENERIC_CLOSURE_GAP_DRIVEN_IMPROVEMENT_V1.md`
and the replay artifacts beneath `build/thor-evidence/live-worker-control/`.

# ADR-M13.3-VERSIONED-GENERIC-FLOW-NORMALIZATION

**Date:** 2026-09-23

**Status:** Accepted as an additive, partial normalization contract; M13.3
capture acceptance remains open.

**Context:** M13.2 showed that the closure adapter discarded fields already
present in native FLOW V2, including CPU ID, bus width/domain, stream and
instruction sequence, and next-PC. The fixed 48-byte record does not carry a
per-instruction register snapshot or complete M68K instruction bytes/width.

**Decision:** Preserve all available fields through `oasis.m13.normalized-
generic-corpus.v2`, materialize generic instruction, memory, ROM-read,
control-flow, call, return, and indirect-target collections, and classify
captured M68K opcodes against the canonical ROM without dropping mismatches.
Use only native `cpu_id` for CPU identity. Carry the exact Worker segment
entry/exit register snapshots from host-audited metadata and link them to the
first/last instruction identities; never imply they describe intervening
instructions. Missing per-instruction snapshots, decode, width, and
address-space classifications remain explicit unresolved fields. Generic
closure consumes normalized memory, ROM-read, call, and return facts while
maintaining compatibility with v1 synthetic fixtures.

**Consequences:** Available evidence is no longer lost at normalization, but
the register reaching-definition contract is incomplete until native
per-instruction bounded snapshots and references are captured. V2 status alone
does not pass M13.3 acceptance and cannot release raw FLOW/index files. No
ownership promotion or production runtime change is implied.

**Evidence:** M13.3 normalizer tests, live generic-stage tests, and the
preserved validation capture receipt (when emitted).
# ADR-M14.2A — Canonical Global Evidence Graph Persistence
**Status:** Proposed; safe-admission acceptance remains incomplete
**Date:** 2026-09-24

**Decision:** Add derivation and map-proposal records to the existing canonical
ROM knowledge SQLite database. Keep `emission` as the sole ROM partition and
`SOURCE_OWNED` authority. Runtime captures remain scoped evidence; proposals
are bound to exact parent/map/graph/validator identities and are not canonical
facts. MASTER V2 must preserve all added rows. This decision does not authorize
graph fusion or ownership changes.

**Acceptance limitation:** Runtime evidence import has not yet been migrated
to admit individual scoped occurrence identities; therefore M14.2A safe
admission is not complete.

**M14.2A safe-admission completion:** The live importer now validates the
existing scoped runtime occurrence identity and creates one evidence reference
per occurrence, with individual claim/relation support references. The session
occurrence log remains source-session evidence; canonical `knowledge.sqlite`
retains the references. Focused replay, overlap-window, CPU/domain, persistence,
and emission invariance acceptance passed. This does not fuse subsystems or
change emission ownership.
# ADR-ROM-PROPERTY-MAP-V1 — Direct observed-property map

**Status:** Accepted for developer tooling; emulator runtime acceptance open
**Date:** 2026-09-27

**Context:** Required raw FLOW retention makes direct facts expensive to use
for bounded ROM classification. The canonical emission map also records a
different concern: reconstructed/source-owned partitioning.

**Decision:** Add an independent dense `uint16_t` runtime property map. Its
bits represent only evidence-backed use of exact physical ROM bytes. Zero is
UNKNOWN. Checkpoints bind the map and per-run contributions to exact ROM,
schema, proof-contract, core/build, run, generation, capability, and validation
identity. Compatible contributions may be OR-merged and can be excluded and
replayed. Ranges are exported as a full-ROM interval partition and may be
overlaid on the existing canonical map without changing canonical class,
truth, or ownership. No GPU, raw event archive, or semantic promotion is
required for these direct facts.

The byte-exact property and rejection contracts live in
`src/tools/thor_evidence/runtime_rom_properties_contract.json`; the checkpoint
identity pins its SHA-256. Emulator callbacks must prove backing and completion
before calling low-level map operations. Unsupported cases stay zero.

**Consequences:** Checkpoint, merge, exporter, M68K decoder-backed span
primitive, Z80 fetched-byte validation, exact-copy RAM origins, and a
canonical overlay are implemented as developer tooling. The tools do not yet
establish that the emulator calls these primitives. Audio payload promotion,
live callback acceptance, behavioral parity, and runtime performance remain
unproven. This ADR does not change ROM reconstruction, emission authority, or
`SOURCE_OWNED` gates.

# ADR-M14-CONSUMER-EVIDENCE-IMPORT — Source-separated accepted artifacts

**Status:** Accepted for developer tooling
**Date:** 2026-09-28

**Context:** W5 contains exact, accepted resource boundaries and byte-identical
Format-A round-trip evidence for resources not read in a selected runtime
scenario. Runtime checkpoints intentionally bind to one core and one global
runtime proof-contract hash, so importing these facts into OASROMP1 would either
misstate their source or weaken runtime merge compatibility.

**Decision:** Store `ACCEPTED_ARTIFACT` claims in a separate, versioned sparse
contribution ledger. Each claim binds ROM identity, exact half-open range,
property and evidence kind, source artifact identity/hash/schema, source proof
contract identity/hash, adapter identity/version/hash, and validation checks.
The first adapter accepts only the W5 secondary `AUDIO_FORMAT_A_MODE0`
resource `[0x0BC95C,0x0BD540)` as `AUDIO_PAYLOAD_PROVEN`, after checking the W5
IR descriptor, exact ROM bytes, accepted round-trip/functional receipts, and W4
consumer evidence. Runtime maps and canonical `knowledge.sqlite` remain
independent authorities. A combined display is derived by replaying both
sources; imported bits are never relabeled as runtime observations and never
change `SOURCE_OWNED`.

**Consequences:** Candidate ranges, heuristics, whole banks, descriptors,
control data, temporal-only W4 evidence, and properties stronger than the
source proof are rejected. Removing an artifact contribution means replaying
the remaining ledger on the immutable runtime base; bit subtraction from an
OR-combined map is forbidden. The current implementation emits an ignored
build artifact and acceptance receipt without changing the runtime checkpoint,
ROM, canonical map, or emulator hot path.

**Accepted extension (2026-09-28):** Add a second narrow adapter for the
already accepted M12-GFX-2 caller closure at
`[0x18A4BE,0x18CC6E)`, using the existing `COMPRESSED_GRAPHICS_SOURCE` property.
The original ignored JSON receipt was unavailable, so the adapter reconstructs
a versioned machine artifact from pinned accepted Git objects, the accepted
M12-GFX-2 caller rows, the AUTO8 sequential-consumer record, and deterministic
Ancient decoder replay. It records the missing original receipt's historically
reported SHA separately and does not represent it as recovered. Its generated
contribution remains source-separated and is removable by replaying the
runtime base and remaining ledgers.

**Accepted extension (2026-09-28):** Reuse the same
`M12_GFX2_CALLER_CLOSED_RESOURCE_V1` semantics for the TOP-20 exact caller rows
whose physical streams independently replay to the accepted end in the same
Ancient 0x3820 decoder. Stream mode may be command or bit without changing the
property meaning: both prove compressed decoder-input bytes, not decoded asset
semantics or VDP state. Record each range in its own ledger so any one claim
can be removed and replayed independently. The missing original closure JSON
remains marked absent; the batch manifest is reconstructed from pinned
accepted evidence. Runtime, canonical ownership and SOURCE_OWNED remain
unchanged.

## ADR-ROM-PROPERTY-VDP-DIRECT-MOVE — Same-instruction VDP source

**Status:** Accepted for implementation; live acceptance pending
**Date:** 2026-09-27

**Context:** The direct DMA adapter identifies physical ROM sources but cannot
cover a CPU data-port write where one instruction reads a ROM word and forwards
it directly to VRAM, CRAM, or VSRAM. General register tracking is not required
for this bounded case.

**Decision:** Reuse the existing VDP source bits. On Mode 5 CPU data-port
writes, retain at most one exact ROM-backed 16-bit data read in the current
M68K instruction. Set the matching VDP bit only after the write reaches the
accepted hardware data path and the instruction completes as a memory-source
`MOVE.W` whose value is unchanged. Reject other opcodes, address-register and
data-register sources, immediate sources, extra reads, mismatches, incomplete
instructions, and DMA-fill trigger writes. The contract hash changes so old
and new contributions cannot be merged without an explicit migration.

**Consequences:** This adds no property bit, canonical-map mutation, or generic
CPU taint engine. It covers only a direct same-instruction ROM-to-port path.
Register-mediated paths, RAM copies, graphics decompression, and audio remain
separate proof contracts. Live capture, parity, and performance results are
required before runtime acceptance.

## ADR-ROM-PROPERTY-W5-AUDIO-CONSUMER — Exact Format-A Decoder Read Classification

**Status:** Accepted; fresh 3600-frame runtime capture and exact replay passed
**Date:** 2026-09-27

**Context:** W4 established a live banked-ROM → Z80 decoder → YM2612 DAC chain.
W5 independently closed two Format-A mode-0 resource ranges with a
byte-identical decode/inverse-encode roundtrip. Generic Z80 data reads and
sound-bank membership alone do not prove audio payload use.

**Decision:** Reuse `AUDIO_PAYLOAD_PROVEN` only for byte-verified banked-ROM
reads at decoder PCs `0x080E` and `0x0855` whose physical ROM offsets lie in
`[0x0BC95C,0x0BD540)` or `[0x0BD540,0x0BF768)`. Mark only the exact byte read.
Reject descriptors, other PCs, and all other banked ROM addresses. Bind these
fixed inputs to the proof-contract hash.

**Consequences:** This adds no property bit, whole-resource promotion, generic
register taint, or canonical ownership change. Existing checkpoints use the
prior proof contract and cannot be merged with contributions under this rule.

## ADR-ROM-PROPERTY-3820-COMPRESSED-GRAPHICS — Completed decoder input span

**Status:** Implemented; fresh runtime replay passed, coverage gain is bounded
**Date:** 2026-09-27

**Context:** The ROM has a verified `0x3820` graphics decompressor. Its `A0`
argument is the compressed input pointer and returns immediately after consumed
input. The routine has no nested call. Direct VDP source bits cannot describe
compressed input without overstating what the hardware consumed.

**Decision:** Add `COMPRESSED_GRAPHICS_SOURCE`. Capture `A0` only when the live
M68K instruction stream enters exactly `0x3820`. At its completed `RTS`, accept
only a nonempty span of at most 64 KiB whose every byte resolves to contiguous
physical ROM offsets. Mark exactly `[A0_entry,A0_return)`. Exceptions,
unsupported mappings, other entry points, and incomplete calls fail closed.

**Consequences:** This adds one independent property bit and changes the proof
contract identity. It does not imply direct VRAM use, change canonical
ownership, track transformed RAM output, or promote all statically known
resources. Older checkpoints remain valid only under their own contract hash.

## ADR-ROLLING-MASTER-RUNTIME-MAP-VIEWER — Read-only runtime address visualization

**Status:** Accepted
**Date:** 2026-09-28

**Context:** A run-local `oasis.m12.rolling-master.v1` SQLite generation records
observed instruction PCs, execution counts, control edges, and run receipts. It
does not contain the canonical emission partition or property bitmap. Reusing
the property-checkpoint UI for this schema would merge separate evidence domains
and could make runtime observations look like byte ownership.

**Decision:** Provide a separate read-only viewer for rolling MASTER snapshots.
Validate the schema and stored ROM identity, optionally verify a supplied ROM
file, and display address-binned runtime PCs/hits with 1 KiB → 32-byte → byte
zoom and exact instruction details. An empty cell means no instruction PC was
recorded in that master; it does not mean proven UNKNOWN. The viewer does not
write the database, promote the generation, or infer assets, ownership, or player
semantics.

**Consequences:** Runtime occurrence heat is now inspectable without changing
existing OASROMP1 visualization or canonical-map authority. The viewer is a
developer tool for one immutable snapshot and does not claim a literal 3D scene
or complete ROM coverage.

## ADR-POSTRUN-RUNTIME-MAP-CONTINUITY — Live-to-post-run visualization

**Status:** Accepted
**Date:** 2026-09-28

**Context:** Worker Control publishes bounded live metrics, while a run-local
rolling MASTER map is published only during post-run compaction. The large
post-run report and SQLite generation must not block the Tk event loop or the
emulator callback path.

**Decision:** Keep live worker visualization on its existing snapshot reader and
add a runtime-address overview tab to the post-run window. Resolve only the
run-local atomic `current.json`, validate the generation remains under that run
root and verify the database SHA-256. Read SQLite and build bins on a daemon
thread; refresh only when the pointer identity changes. Parse the post-run
report only when its file identity changes. Label this view as a 2D runtime
address heatmap, separate from canonical ownership and any future 3D scene.

**Consequences:** No new work enters the emulator hot path. The post-run screen
can first show the base runtime generation and then follow a newly published
rolling generation; map availability still depends on post-run publication.

# ADR-M14-Z80-DRIVER-IMAGE-ARTIFACT — Separate proof for Z80 image upload

**Status:** Implemented; import completed under ADR-M14-ACCEPTED-ARTIFACT-PROPERTY-V2
**Date:** 2026-09-28

**Context:** Accepted evidence identifies an exact 8 KiB ROM-to-Z80-RAM driver
image transfer and separately observes Z80 execution and YM2612 output. Runtime
property contracts do not currently express this upload relation. Treating the
whole image as executed code would overstate the evidence, while adding a runtime
property would require a runtime proof contract and capture.

**Decision:** Define `Z80_DRIVER_IMAGE_SOURCE` solely in a new
`ACCEPTED_ARTIFACT` proof contract for exact ROM `[0x062E38,0x064E38)`. Bind it
to accepted upload, destination, and execution/sink source identities and hashes.
The claim means the exact image is uploaded to Z80 RAM; it does not mean all
bytes execute. Keep runtime `OASROMP1` vocabulary, checkpoints, and property bits
unchanged. The accepted-artifact contribution uses the distinct typed property
namespace defined by ADR-M14-ACCEPTED-ARTIFACT-PROPERTY-V2.

**Consequences:** The accepted-artifact layer can represent this physical
consumer relation without conflating it with runtime observations or altering
canonical ownership. The imported artifact-layer contribution has zero overlap
and adds 8,192 unique bytes; runtime property masks and canonical ownership
remain unchanged.

# ADR-M14-ACCEPTED-ARTIFACT-PROPERTY-V2 — Keep artifact properties outside runtime masks

**Status:** Accepted for the Z80 driver image contribution
**Date:** 2026-09-28

**Context:** The runtime 16-bit property map has no bit for the accepted Z80
image-upload relation. Reusing `Z80_EXECUTED_ENCODING` would falsely claim each
byte executed; extending the runtime mask would mix static accepted-artifact
evidence into `OASROMP1` and change its contract.

**Decision:** Represent this import in a typed, independently removable
`thor.accepted-artifact.property-contribution.v2` ledger carrying the exact
property string `Z80_DRIVER_IMAGE_SOURCE` and range. Keep it outside runtime
property masks. Compute total-proven coverage as the union of runtime-classified
bytes and accepted-artifact ranges, and require removal/rebuild to reproduce the
prior coverage bitmap byte-for-byte.

**Consequences:** The exact 8,192-byte image contributes to total-proven coverage
without changing runtime checkpoint bits or asserting `Z80_EXECUTED_ENCODING`.
The ledger can be independently validated and removed; the runtime schema and
canonical ownership remain unchanged.

# ADR-GFX-PUBLICATION-WINDOW-V1 — Bounded cross-frame publication window

**Status:** Accepted for diagnostic collection; resource closure remains pending
**Date:** 2026-09-30

**Context:** A live bounded run shows completed 0x3820 decoder calls and VDP
activity, but the existing bridge expired output origin at each frame boundary.
This can miss a later publication and cannot distinguish absence of a path from
an observation window that was too short. RAM output locations may also be
reused in later frames, so extending lifetime without byte-level overwrite
invalidation would introduce false links.

**Decision:** Keep completed output origin per byte for at most 120 frames.
Only actual VDP bytes whose current RAM origin still names that exact completed
transaction may match. Every transfer receipt records its own frame ID. Run
change, reset/loadstate, overwritten bytes, capacity exhaustion, and timeout
remain fail-closed. The bounded diagnostic ring records observed VDP operation
families separately and cannot contribute ROM properties or canonical evidence.

**Consequences:** The longer window can find delayed publication while keeping
false-positive control local to exact byte origins and transaction identity.
It may retain up to eight bounded transactions and can still produce false
negatives after 120 frames. A matching publication alone does not prove SAT,
palette use, or visible pixels; no property or ownership authority changes.
# ADR-GFX-RESOURCE-TRANSACTION-V1 — Ephemeral graphics lineage

**Status:** Accepted for the V1 receipt/reducer boundary
**Date:** 2026-09-29

**Context:** The accepted 0x3820 runtime property hook proves exact compressed
ROM input bytes, while S2–S8 independently prove same-frame VDP/SAT/pixel facts.
Existing receipts do not prove that one decoder call's RAM output supplied the
VRAM tiles consumed by a visible SAT sprite. Joining unrelated runs or frames
would invent causality.

**Decision:** Represent one bounded resource as an ephemeral, deterministic
transaction receipt. Closure requires exact same-ROM/run/frame identity, a
completed 0x3820 call, exact decoder RAM output, ordered RAM-to-VRAM transfers
with matching content hashes, SAT references fully covered by those transfers,
exact same-frame CRAM bytes for each palette, and at least one visible
nontransparent pixel. Palette is a CRAM dependency only;
this contract does not claim a ROM palette source. The receipt/reducer cannot
write the runtime property map or canonical knowledge map.

**Consequences:** This adds no second canonical map and no new ROM property. The
current implementation defines and tests the fail-closed host contract; emulator
collection and an accepted live closure remain future work.

# ADR-AUDIO-STRICT-CAUSAL-SOURCE-V1 — Sparse strict audio evidence layer

**Status:** Accepted
**Date:** 2026-09-30

**Context:** `AUDIO_PAYLOAD_PROVEN` is an existing whole-resource/runtime
consumer property with a separate accepted contract. Controlled Format-A
captures also establish strict causal chains for sparse ROM input roots; many
payload bytes have no accepted root. Reusing the whole-resource property would
overstate those captures, while merging controlled and natural observations
would erase how each result was obtained.

**Decision:** Store exact strict roots under the distinct
`AUDIO_STRICT_CAUSAL_SOURCE` name in the independently removable
`accepted_artifact_only` namespace. The contribution contract is
ROM byte → exact Format-A input → accepted strict provenance → decoded sample →
YM2612 DAC write. Ledgers bind exact resource/range unions, replay identity,
source/build hashes, and evidence origin. Controlled command-path captures use
`CONTROLLED_CONSUMER_PROOF`; naturally observed Entry 7 remains explicitly
`NATURAL_GAMEPLAY_OBSERVATION`. Neither evidence kind changes the runtime
property ABI or `AUDIO_PAYLOAD_PROVEN` semantics, and gaps remain unknown.

**Consequences:** Imported strict ranges add 164,394 unique bytes in this
acceptance; Entry 7 contributes additional proof depth over already classified
bytes. Each resource and the complete batch can be removed by rebuilding from
the surviving ledgers. No canonical ownership or `SOURCE_OWNED` state changes.
# ADR-GRAPHICS-VRAM-CAUSAL-SOURCE-V1 — Sparse decoder-to-VRAM evidence

**Status:** Accepted
**Date:** 2026-09-30

**Context:** The graphics transaction bridge has a strict stage-1 contract for
exact ROM input → completed `0x3820` decoder → exact RAM output → RAM-origin
VRAM publication. This proves publication but not SAT/plane use, palette
dependency, or visible pixels. Existing `COMPRESSED_GRAPHICS_SOURCE` describes
source-format evidence and must not be reused to imply this runtime causal
chain.

**Decision:** Store only replayed exact ROM input ranges under the distinct
`GRAPHICS_VRAM_CAUSAL_SOURCE` name in the removable `accepted_artifact_only`
namespace. Each resource ledger binds ROM/build/overlay identity, both run
receipts, decoder/RAM/VRAM ranges and hashes, and the stage-1 proof contract.
The property claims exact publication to VRAM only; SAT, plane, palette, and
visible-pixel semantics remain unclaimed.

**Consequences:** The runtime property ABI, OASROMP1, canonical SQLite and
`SOURCE_OWNED` remain unchanged. Each transaction can be independently removed
and the prior total-proven bitmap rebuilt byte-for-byte.

# ADR-STATICALLY-ENUMERATED-CONSUMER-SOURCE-V1 — Structural family evidence plane

**Status:** Accepted
**Date:** 2026-10-01

**Context:** Closed selector/table/consumer contracts can establish exact
physical ROM intervals for every structurally valid family member without
observing every member in a runtime scenario. Treating these facts as runtime
properties would overstate execution; discarding them would lose scalable,
reproducible boundary evidence. Canonical `SOURCE_OWNED` is a separate authority.

**Decision:** Store closed-family ranges in an independently removable
`STATICALLY_ENUMERATED_CONSUMER_SOURCE` evidence plane, bound to ROM identity,
contract entries, source artifact hashes, and exact physical interval union.
The structural view is derived separately and is excluded from the runtime
property bitmap and `TOTAL_PROVEN`. It makes no natural-execution or
graphics/audio semantic claim. Runtime causal classes and canonical ownership
remain unchanged.

**Consequences:** Whole-ROM analysis can count exact structurally explained
bytes and rank reusable contracts without requiring one runtime observation per
entry. Reports must show structural-family bytes separately from runtime
property coverage and `SOURCE_OWNED`. Each contribution can be removed and the
derived structural view rebuilt independently.
# ADR-M14-STRUCTURAL-VARIABLE-FAMILY-PROVENANCE-V1 — Variable-Length Structural Family Ledger
**Status:** Accepted
**Date:** 2026-10-01

**Context:** The existing independently removable structural ledger represents
fixed-size entries. `GFX_SCREEN_ROOTS` contains variable-length decoder streams
whose ends must be established by the exact decoder/parser contract. Mutating
the accepted v1 validator would invalidate its source-artifact provenance.

**Decision:** Add a versioned v2 structural contribution model for
`STATICALLY_ENUMERATED_CONSUMER_SOURCE` with `family -> roots[] -> descriptor
-> stream [start,end)` records and explicit `root -> descriptor -> stream`
provenance edges. Every range binds its decoder/consumer, boundary rule, exact
ROM SHA, and source-artifact hashes. The GFX screen family is accepted only
after deterministic Debug/Release census equality, exact interval-union
accounting, source verification, and byte-exact removal replay. Keep the v1
module and its accepted contribution unchanged. Structural bytes remain outside
the runtime bitmap, `TOTAL_PROVEN`, canonical SQLite, and `SOURCE_OWNED`.

**Consequences:** Variable-size reusable resource families can be enumerated
without runtime observation of every entry when exact consumer and boundary
contracts close. Their ranges remain independently removable structural
evidence and make no runtime-execution or semantic graphics claim.
# ADR-M14-STRUCTURAL-TABLE-VARIABLE-FAMILY-V3 — Selector-Indexed Variable-Length Structural Family
**Status:** Accepted
**Date:** 2026-10-02

**Context:** The next large reusable family is selected through a global audio
index into eight bank descriptor tables. The existing variable-range v2
validator is bound to GFX_SCREEN_ROOTS' pointer-table and decoder-terminator
contract; changing it would invalidate that accepted contribution. The audio
parser instead provides a different exact end rule: descriptor length loaded
into BC and consumed by the decoder loop.

**Decision:** Preserve v1 and v2 schemas unchanged. Add a versioned table-rooted
structural contribution schema that binds the complete 0..127 selector domain,
all eight 16-entry descriptor tables, the 67 exact Mode-0 members, physical
ROM mapping, descriptor-length boundary, root→descriptor→stream edges, ROM
identity, and source hashes. Accept `AUDIO_MODE0_BANK_RESOURCES` only after
independent enumeration through direct ROM parsing and the existing W5
descriptor parser, exact interval overlap accounting, and removable-view
replay. Keep this contribution exclusively in the structural evidence plane.

**Consequences:** A closed parser/table contract can establish family ranges
without runtime observation of each entry. Mode-1 descriptors remain outside
this family, and no audio semantics, runtime property, canonical SQLite, or
`SOURCE_OWNED` claim follows from structural membership.

# ADR-M14-STRUCTURAL-ABSOLUTE-POINTER-FAMILY-V4 — Exact Decoder-Bounded Pointer Family
**Status:** Accepted
**Date:** 2026-10-02

**Context:** `AUDIO_MODE0_BANK_RESOURCES` uses fixed-size descriptors whose
length field provides each boundary. The indexed graphics table at
`0x05CE96..0x05D046` instead contains absolute pointers to self-terminating
streams. Reusing the descriptor-length schema would encode the wrong boundary
contract; extending its family-specific validator would invalidate the accepted
Mode-0 contribution's source hashes.

**Decision:** Preserve structural v1/v2/v3 validators. Add an independently
versioned pointer-table v4 contribution model for exact big-endian absolute ROM
targets with per-target parser-consumed half-open ends. Require a complete slot
census, exact table mapping, ROM and source hashes, explicit
table-entry→pointer→reader→decoder→stream edges, deterministic A/B enumeration,
exact overlap accounting, and byte-exact baseline restoration when the one
contribution is removed. Reject the family as a whole on any invalid target or
boundary. Keep it in `STATICALLY_ENUMERATED_CONSUMER_SOURCE` only.

**Consequences:** The 107-member `GFX3820_INDEXED_POINTER_TABLE` can be admitted
without runtime observation of every entry. This schema establishes structural
membership only; it does not update the runtime bitmap, `TOTAL_PROVEN`, canonical
SQLite, or `SOURCE_OWNED`, and it makes no asset-semantic claim.

# ADR-M14-STRUCTURAL-SENTINEL-LIST-FAMILY-V1 — Sentinel-Bounded List Families
**Status:** Accepted
**Date:** 2026-10-04

**Context:** The structural evidence plane already supports fixed descriptors,
variable decoder streams, pointer tables, and direct roots. AUTO13_BCEA uses a
different reusable boundary rule: signed relative pointers select lists whose
44-byte records end at a negative signed-word key. Reusing a length-field or
decoder-end schema would obscure whether the sentinel itself is included.

**Decision:** Add the independently versioned
`thor.static-family-sentinel-list-contribution.v1` ledger. Each member binds
its field-3 table references, selector slot, raw signed-relative word, exact
physical start, record count/width, sentinel address/value, two-byte included
sentinel extent, ROM identity, and source-artifact hashes. Admission requires
complete slot accounting, exact parser instruction bytes, independent A/B
enumeration, exact physical union and overlap, and byte-exact removal replay.
The shared structural-view dispatcher accepts this schema while runtime
property evidence, `TOTAL_PROVEN`, `SOURCE_OWNED`, and canonical SQLite remain
outside its inputs and outputs.

**Consequences:** Exact sentinel-list families can be independently removed
and regenerated without asserting natural runtime selection or per-entry
execution. The `AUTO13_BCEA` contribution covers only the five exact physical
union ranges and makes no graphics/audio semantic claim.

# ADR-PORTABILITY-GFX3820-FILE-OVERRIDE-V1 — Hash-Pinned ResourceID File Backend
**Status:** Accepted for backend prototype
**Date:** 2026-10-04

**Context:** The accepted `GFX3820_INDEXED_POINTER_TABLE` structural family
already has 107 exact parser-bounded compressed streams and fixed 4,096-byte
outputs. The native path was ID3-only. A portability test needs to replace the
compressed payload source while preserving the existing decoder and canonical
ROM behavior.

**Decision:** Add a deterministic ResourceID 1..107 manifest pinned to the
accepted census, canonical ROM identity, exact compressed bytes, and decoded
hashes. `resource_loader` may use `assets/gfx3820/<id>.bin` only after exact
manifest validation and exact decoder/output verification; every rejected
override falls back to the canonical ROM stream and records a reason. A
separate explicit replacement sidecar/hash mode represents intentional
modified payloads and is never labeled an exact copy. Backend/migration status
is a separate portability plane. The accepted prototype uses exact copies
only; it adds no runtime, structural, SOURCE_OWNED, or canonical SQLite
authority and assigns no resource semantics.

**Consequences:** The game can keep all 107 resources ROM-backed without an
assets directory, and exact file-backed replacements can be tested before any
decoded-tile or semantic asset layer is introduced. The contract does not
claim natural selection of every ID, visible identity, or gameplay use.

# ADR-PORTABILITY-GFX-SCREEN-ROOTS-FILE-OVERRIDE-V1 — Stream-ID Screen Root Backend
**Status:** Accepted for exact-copy portability prototype
**Date:** 2026-10-04

**Context:** The accepted `GFX_SCREEN_ROOTS` structural family preserves root
addresses, descriptor addresses, and exact GFX3820-bounded streams. Those are
not one-to-one: 21 roots produce 167 path uses, 163 unique descriptors, and
159 unique streams; decoded lengths also vary.

**Decision:** Keep root-path, descriptor, and stream identities as separate
manifest columns. Use unique physical stream IDs (`STREAM_<ROM start>`) for
external file names, while retaining every root/descriptor path and alias
group. The dedicated `screen_resource_loader` validates manifest and ROM
provenance, compressed length/hash, full decoder consumption, and decoded
length/hash. Any absent or rejected exact-copy override uses the canonical ROM
stream. Do not add semantic names or modified-resource mode in this prototype.

**Consequences:** This enables deterministic file-backed copies without
collapsing aliased descriptors or roots into ambiguous IDs. The manifest is
portability metadata, not a structural admission or runtime selection record;
all coverage and source-owned authorities remain unchanged.

# ADR-CODE-TRANSLATION-AUDIO-FORMAT-A-V1 — Native Bounded Z80 Audio Decoder
**Status:** Accepted as a translation candidate; dispatch replacement deferred
**Date:** 2026-10-04

**Context:** W5 already establishes a bounded Format-A Z80 decoder contract,
exact audio descriptors, a deterministic Python decoder/encoder and two
byte-identical resource roundtrips. The accepted Mode-0 descriptor family
contains 67 exact payloads (205,208 bytes); the complete descriptor catalog
contains 96 active selector rows across Modes 0 and 1. A native implementation
can be checked against the independent Python path before integration with any
sound-command or playback system.

**Decision:** Add a portable C++ descriptor parser and Mode-0/Mode-1 sample
decoder in `src/game/audio_format_a.*`, plus synthetic tests and a local-ROM
whole-catalog differential tool. The stable selector identity is the original
0..127 bank/entry selector; inactive slots remain absent. The native function
consumes only the exact descriptor-bounded compressed span. Keep it as a
candidate API without sound-command dispatch, hardware timing, or a claim that
all catalog selectors are naturally reached. Mode-1 differential equality
against W5 Python is not a new runtime proof.

**Consequences:** The complete native decode transform is now mechanically
comparable across all 96 active catalog rows, while runtime alignment remains
limited to the existing 18-sample W4 witness. No runtime-property, structural,
`TOTAL_PROVEN`, `SOURCE_OWNED`, or canonical SQLite authority is changed. A
future dispatch replacement requires separate timing, state-continuation, and
playback integration evidence.

# ADR-PORTABILITY-M68K-FIXED-COUNT-RECORDS-V1 — Row-Preserving Raw Stream Backend
**Status:** Accepted as exact-copy portability prototype
**Date:** 2026-10-04

**Context:** The accepted `M68K_FIXED_COUNT_RECORD_FIELD0` census contains 96
active table rows but only 78 unique stream starts. The shared four-consumer
contract bounds a stream by a big-endian inclusive count and six-byte records.
The table aliases are meaningful identity relationships even though the
records remain semantically unnamed.

**Decision:** Keep `TABLE_ROW_ID`, stable `STREAM_<ROM start>`, and ROM range
as separate manifest fields. The generic loader resolves any active row to its
canonical unique stream, validates exact file size, count-derived extent, raw
hash, and ROM identity, then returns raw bytes and neutral parsed words. On any
override rejection it reads the exact ROM span. Exact-copy files remain
optional; no resource payload is checked in.

**Consequences:** The portability layer can now represent decoder-backed
graphics, variable-size screen graphics, and count-bounded record streams as
separate formats. It establishes no gameplay semantics or runtime selection and
does not affect runtime, structural, SOURCE_OWNED, or canonical SQLite data.

# PROPOSAL — Reversible path-specific M68K helper dispatch
**Status:** Proposed; not accepted or implemented
**Date:** 2026-10-08

**Context:** Three helpers (`0x94A2`, `0x8EF8`, `0x16A46`) have independent
`CPP_REPLACEMENT_VERIFIED_PATH_SPECIFIC` contracts. None is globally verified
or active. Their ABIs differ and computed indirect entries are not excluded.

**Proposal:** If implementation is separately accepted, dispatch at each
helper entry after the guest BSR/JSR has executed, but only when a trustworthy
observed transfer-source PC belongs to that helper's accepted direct caller
set. Require a per-helper opt-in, matching canonical ROM identity, matching
contract version/hash, and helper-specific input preconditions. Otherwise leave
the original ROM entry untouched. Keep typed adapters separate, stage effects
before mutation, use ROM-authoritative cloned-state reference comparison in
development, and preserve three independently reversible enable flags.

**Not decided:** This proposal does not authorize production wiring, active
runtime replacement, or cycle/prefetch equivalence. Exact call-source
instrumentation and GPGX continuation validation remain prerequisites. The
existing target-only experimental registry is not sufficient unchanged.

**Reference:** `docs/reports/THOR_M68K_HYBRID_DISPATCH_DESIGN_20261008.md`.

# ADR — Opt-in developer runtime route for M68K `0x8EF8`
**Status:** Accepted for the developer-only HOOK_CPU runner; production dispatch remains inactive
**Date:** 2026-10-09

**Context:** The verified cohort contains three path-specific helpers. Exact
transfer-source provenance is now available for supported direct calls. A
minimal reversible route was requested to exercise one helper in the executing
M68K core, without generalizing the registry or changing the shipped game.

**Decision:** Route only `0x8EF8` in the developer runner, only when
`--enable-8ef8` is supplied and canonical ROM identity, helper entry, accepted
direct caller PC, BSR.W target, immediate hook ordering, stack return, and
bounded record-span checks all pass. Stage the record effect before committing;
fallback executes the untouched ROM helper. Route through the helper's existing
RTS so the ordinary call frame is returned. The flag is off by default.

**Consequences:** The opt-in HOOK_CPU test runner reports runtime activity when
the path-specific route actually executes. The production game remains
ROM/native-runtime-independent and unchanged. Functional register/record
parity is tested for six direct callsite instructions, but cycle/prefetch,
write-bus timing, and natural gameplay reachability are not established. The
helper's verification status remains path-specific; no ROM evidence authority
or coverage total changes.

**Reference:** `docs/reports/THOR_M68K_8EF8_OPT_IN_LIVE_ROUTE_20261009.md`.

# ADR — Replay-only native island attempt for M68K `0x9D70`
**Status:** Rejected as a replay-equivalent island; retained as default-off diagnostic code
**Date:** 2026-10-12

**Context:** Ten helpers have path-specific native equivalence receipts. The
first proposed multi-helper replay used the exact `0x009144 BSR.W` contract for
`0x9D70` as its smallest activation step.

**Decision:** Keep the typed `0x9D70` adapter available only to the developer
replay runner and default it off. A ROM-only 3,600-frame replay passed, but an
enabled one-helper run produced a deterministic landmark divergence at frame
2655 after one native execution. Stop the enable-set ladder at this failure;
do not claim replay-native island validation or route the other helpers yet.
The all-ROM run after the failure passed. Existing helper verification remains
path-specific, global status remains NO, and production runtime replacement
remains inactive.

**Consequences:** The C++ route and its unit gates are useful instrumentation,
but the exact cause of the replay divergence is unresolved; timing/prefetch,
continuation handling, and state effects remain candidates for diagnosis. The
available corpus also lacks natural calls to
`0x16A46` and `0xB730`. Revisit only with a closed mismatch diagnosis and a
session that reaches the missing caller paths. No ROM evidence authority or
coverage total changes.

**Reference:** `docs/reports/THOR_REPLAY_NATIVE_ISLAND_ATTEMPT_20261012.md`.

**Follow-up (2026-10-12):** Strict instruction/PC mismatch is separated from
normalized equivalence: native helper-body elision makes strict trace identity
inapplicable as a pass criterion, but does not waive caller-visible state or
scheduler checks. The matched call has normalized event and M68K boundary-state
parity, but the native route is 294 cycles shorter and misses the M68K line
budget crossing that advances Z80/VDP by one scanline. Z80 RAM then differs at
return; normalized replay and timing equivalence both remain NO. No cycle compensation was attempted, and no route expansion follows from this
diagnostic. See
`docs/reports/THOR_REPLAY_NATIVE_9D70_TIMING_STATE_DIAGNOSTIC_20261012.{md,json}`.

**Timing follow-up (2026-10-12):** The regular pre-instruction HOOK_CPU route
is unsuitable for `0x9D70` substitution because ROM execution crosses the
current M68K line budget before the RTS continuation, while the native route
does not; this changes whether Z80/VDP scheduling occurs before caller
continuation. Do not compensate with a single aggregate cycle delta: ROM
branch paths have different measured costs, and refresh charging is checked at
instruction boundaries. The existing developer block hook and per-instruction
timing callbacks are the only selected candidate for a future experiment, but
remain UNVERIFIED for scheduler equivalence until a synthetic line/refresh
crossing micro-oracle passes. No compensation, route change, or helper-status
change is authorized by this result. See
`docs/reports/THOR_REPLAY_NATIVE_9D70_CYCLE_ACCOUNTING_20261012.{md,json}`.

**Scheduler fixture and selected instruction-step replay follow-up (2026-10-13):**
The isolated fixture now verifies its ten documented instruction-step microcases
across line-budget, refresh, Z80/VDP, IRQ, and resume behavior. Applying the
stepper to one natural `0x009144 → 0x009D70` call preserved the measured 420-cycle
helper cost and line-boundary crossing. A 3,600-frame replay matched the explicit
per-frame CPU/scheduler/observable-memory trace, call boundaries, post-return
sync windows, external events, helper events, and sampled Z80 RAM. The legacy
landmark validator still reports only serializer `state_sha256` mismatches at
60 checkpoints, which also occur in duplicate ROM-only restores; those hashes
remain a disclosed limitation and are not silently normalized. This supports
only the tested single caller-path call and replay surface. Keep the route
diagnostic-only, `RUNTIME_REPLACEMENT_ACTIVE=NO`, and helper verification
path-specific; do not imply arbitrary-opcode, unsupported-caller, or future
session equivalence. See
`docs/reports/THOR_9D70_INSTRUCTION_STEP_REPLAY_20261013.{md,json}`.

**Serializer acceptance follow-up (2026-10-13):** Gameplay-session replay
landmarks now use the existing version- and size-checked canonical checkpoint
hash; raw serializer hashes remain diagnostics. Duplicate ROM-only restores
matched canonical checkpoints 60/60, and the selected instruction-step replay
matched ROM-only 60/60. A fixture-only save-state tail now preserves
`m68k.refresh_cycles`; roundtrip continuation matched 3,600/3,600 frames.
This does not broaden the `0x9D70` route claim, enable production dispatch, or
change any evidence authority. See
`docs/reports/THOR_SERIALIZER_CANONICAL_REPLAY_20261013.md`.

# ADR-0046 — ROM-Backed Screen Entity Model and Multi-Tile Sprite Rasterization
**Status:** Accepted for M15.3
**Date:** 2026-10-14

**Context:** The vertical slice previously rendered real ROM background tiles, collision byte grids, and screen transitions for screens 0x0100 and 0x0101, but lacked real non-player entities and Genesis sprite rendering.

**Decision:**
1. Derive entity definitions directly from the canonical ROM type table at `0x0003F2FA` (32 bytes per row, indexed by `type / 2 - 1`), matching the original engine's `0x008D06` initialization contract.
2. Bind screen entity lifetime directly to `ScreenInstance`. On screen load/transition, entities are created or destroyed with the screen instance, guaranteeing zero stale objects or leaks across screen boundaries.
3. Rasterize multi-tile Genesis sprite pieces using the ROM frame descriptor format (proven by helper `0x0000B730`), translating world coordinates to viewport screen space via active camera coordinates.
4. Keep the native recompilation helper island frozen (no helper #9 added).

**Consequences:** Real ROM-backed entities appear and render on authentic screens, survive camera movements, reload cleanly on screen transitions, and maintain 100% backward compatibility with synthetic test fixture replay hashes.

# ADR-0047 — Authentic ROM-Driven Entity Animation Playback and Fail-Closed Decoding
**Status:** Accepted for M15.4
**Date:** 2026-10-14

**Context:** ScreenEntity previously rendered static multi-tile Genesis sprites from the initial frame descriptor (0x167DAC), but did not animate. The animation descriptor format and runtime driver needed reverse engineering and integration into the vertical slice without adding new helper routines.

**Decision:**
1. Derive entity animation sequence pointers from offset +20 (0x14) of the canonical ROM type table at `0x0003F2FA` (`0x00041052` for chest Type `0x002A`), which maps to `+0x26(A6)` in the M68K RAM entity pool.
2. Reconstruct the 4-byte frame record format proven by M68K subroutine `0x008FA4..0x009040`: `[Word 0: frame index, Word 1: raw duration & loop flag]`, where `ticks = (duration & 0x7FFF) >> 1` and `0x8000` signals sequence termination / loop reset.
3. Map frame indices to frame descriptors via the graphics table at `0x00167DAC` using authentic relative word offset addressing: `frame_addr = entry_addr + read_be16(entry_addr)`.
4. Implement a fail-closed decoder: any invalid sequence header, out-of-bounds offset, or unsupported opcode marks the animation as `halted`, preserving the current visual frame without undefined behavior.
5. Keep the native recompilation helper island frozen (no helper #9 added).

**Consequences:** Entities on authentic ROM screens play authentic animations at authentic ROM tick rates, seamlessly loop, reload cleanly across screen transitions, and report their live animation state via the HUD.

# ADR-0048 — Execution-Driven Hybrid Recompilation Strategy and Central Native Replacement Registry
**Status:** Accepted
**Date:** 2026-10-14

**Context:**
The project was previously operating with isolated instruction-step helper islands (`0x9D70, 0x9D4E, 0x9DB4, 0x9D92, 0x94A2, 0x9C40, 0x8EF8, 0xE268`) on one hand, and a full native vertical slice in `ControlledScreen` on the other. A strategy change shifts the workflow to execution-driven hybrid recompilation: the emulator remains the authoritative fallback executor for untranslated M68K code, while verified C++ replacements progressively displace ROM execution without scattering PC hooks.

Before activating native player replacement, audit discrepancies regarding player record base (historical `0x00FF19E8` vs audit `0x00FF1800`) and entry PCs (`0x004A20` vs `0x00557A`) required runtime verification.

**Decision:**
1. **Canonicalize Player Record Base**: Confirmed via runtime execution traces that the player entity record base is `0x00FF19E8` (`PLAYER_A6_RUNTIME_BASE = 0x00FF19E8`). The audit reference to `0x00FF1800` is documented as inaccurate. `0x004A20` belongs to an early boot sound initialization path with 0 runtime hits in 3,600 frames; the canonical player update entry in ROM is `0x00557A` -> `0x005670` -> `0x0059B2`.
2. **Central Native Replacement Registry**: Create `HybridExecutionRegistry` managing `NativeRegion` descriptors (`entry_pc`, `internal_pcs`, `exit_pcs`, `continuation_pc`, `supported_precondition`, `handler`, `timing_mode`, `fallback_policy`, `verification_status`).
3. **Supported States & Fail-Closed Fallback**: Native execution is allowed strictly for States 0 (normal locomotion), 10 (melee attack), 14 (secondary weapon/bow), and 24 (hurt/knockback). Any other state triggers fail-closed ROM fallback, recording full M68K registers (`D0-D7`, `A0-A7`, `SR`), stack top, frame, and input state.
4. **First-Divergence Oracle**: Support differential execution comparing D/A registers, SR/CCR/X, PC, stack, and player RAM record, halting on first meaningful divergence.
5. **Eliminate Child Provenance Rejects**: When the native player dispatcher owns the call path, calls to `0x00E268`, `0x00938E`, and `0x009BF2` use existing native equivalents without triggering provenance rejects or weakening global gates for other callers.

**Consequences:**
A unified architecture allows C++ components from `ControlledScreen` to progressively displace M68K execution while preserving the emulator as an authoritative fallback, with zero evidence authority changes.

# ADR-0049 — Main Loop Frame Tick Coordinator (0x0033D2..0x003424) and Instruction-Stepped E268 Caller Extension
**Status:** Accepted
**Date:** 2026-10-14

**Context:**
Runtime call census in the canonical 3,600-frame session revealed 410,548 ROM fallbacks (≈99.9% of all remaining fallback traffic) converging on helper `0x00E268`:
- `CALLER 0x00340E -> 0x00E268`: 364,289 hits
- `CALLER 0x0033E2 -> 0x00E268`: 46,259 hits

Reverse engineering of cluster `0x0033D2..0x003424` disproved that these were player locomotion or gameplay calls. The cluster is the engine's Main Loop Frame Tick / Delay Coordinator, which sets frame counters at `0x00FF1658` (`0x005E` for initial delay, `0x0001` per frame tick) and spins waiting for VBlank interrupts (`0x0021F0` `subq.w #1, ($00FF1658).l`). During each spin iteration, the engine invokes `0x00E268` to churn the PRNG 16-bit LFSR seed at `0x00FF1928` and burn CPU cycles.

The existing instruction-stepped helper `step_m68k_e268` correctly modeled the 29-word opcode sequence and scheduler/interleaving timing, but hardcoded caller validation to `0x00359A` only. Calls from `0x0033E2` and `0x00340E` were rejected with provenance errors and fell back to ROM.

**Decision:**
1. **Stale Region Accounting Clean-up**: Update `HybridExecutionRegistry` to distinguish active from inactive regions. Mark `0x004A20` as inactive (`active = false`, `VerificationStatus::StaleStaticCandidate`) because it had 0 executions in the canonical 3,600-frame session and belongs to boot sound initialization. The canonical player update region is `0x00557A` (`active = true`).
2. **Instruction-Stepped Timing Classification**: Classify cluster `0x0033D2..0x003424` and child helper `0x00E268` strictly as `INSTRUCTION_STEPPED_NATIVE`. Whole-function block substitution is explicitly rejected because synchronous spinning stalls the emulator scheduler and starves VBlank/Z80/VDP interrupts.
3. **Extend E268 Caller Provenance**: Update `step_m68k_e268` to accept verified callers `kProvenCallers = {0x00359A, 0x0033E2, 0x00340E}`. For each caller, strictly verify:
   - ROM opcode at `caller_pc` is `jsr $0000E268` (`0x4EB9 0x0000 0xE268`).
   - Stack top `(SP)` contains exact return address `caller_pc + 6`.
   - Global provenance is preserved: all unverified callers fail closed to ROM fallback.
4. **Register Main Loop Coordinator Region**: Register `Main_Loop_Frame_Tick_Coordinator_0033D2` in `HybridExecutionRegistry` with entry `0x0033D2`, exits `0x003424` and `0x003464`, and continuation `0x003428` in instruction-stepped timing mode.

**Consequences:**
The 410,548 ROM fallbacks from callers `0x00340E` (364,289) and `0x0033E2` (46,259) are completely eliminated without disturbing cycle-stepped timing, scheduler, IRQ, Z80, or VDP interleaving. All 28 hybrid unit tests pass 100%. All source files remain <= 500 lines.

## ADR-0050: Bounded Native Displacement of Entity Oscillation Coordinator 0x0613B2

**Context:**
Following complete elimination of the 410,548 E268 spin-wait fallbacks, the next highest-frequency runtime dependency encountered during the canonical 3,600-frame gameplay session was `0x0613B2` (26,233 hits across callers `0x061AC2` with 16,673 hits and `0x062576` with 9,560 hits).
Analysis of ROM bytes and control flow established that `0x0613B2..0x0613F6` is a self-contained 17-instruction leaf routine (70 bytes, 0 callees, 0 VDP/hardware accesses) that updates entity slot floating/bobbing oscillation displacements in Work RAM relative to `(A6)`. Additionally, the apparent ambiguity between `0x003408` and `0x00340E` was canonicalized: `0x003408` is the immediate address extension word of `btst.b #7, ($00FF165F).l` (`0x003402`), not an instruction boundary.

**Decision:**
1. **Implement Native Region**: Implement `Entity_Oscillation_Bobbing_Coordinator_0613B2` in `src/tools/hybrid/entity_oscillation_region.hpp` and `.cpp`.
2. **Strict Provenance & Preconditions**: Require caller PC to match proven callers `{0x061AC2, 0x062576}`, verify `A6` is in valid Work RAM, and verify stack top `(SP)` contains `caller_pc + 4`. Unknown callers or malformed stack frames fail closed to ROM fallback.
3. **Register Region**: Register `0x0613B2` in `HybridExecutionRegistry` as `VerifiedPathSpecific` with instruction-stepped timing semantics.
4. **Hardware Classification**: Classified as `PURE_CPU_DATA_TRANSFORM` with 0 direct VDP/SAT interactions; safe for native execution without cycle drift.

**Consequences:**
The 26,233 ROM fallbacks targeting `0x0613B2` are completely displaced with zero divergence. Total remaining ROM fallbacks drop from 187,390 to 161,157. Next runtime dependency identified is `0x061934` (20,916 hits).

## ADR-M15-28: Modular Native Child Displacement of 0x06138E and Dynamic Shift Timing Recovery

**Status:** Accepted for Milestone M15.28
**Date:** 2026-10-16

**Context:**
Census analysis after displacing gamepad debouncer `0x002992` and VDP committer `0x0027FC` identified `0x06138E` as an untranslated ROM child routine invoked 2,609 times across the canonical 3,600-frame replay from two already-native parent subsystems:
- Caller A: `0x061A12` inside native `0x061934` (1,687 calls, displacement `0xF97A`, return PC `0x061A16`).
- Caller B: `0x0624A0` inside native `0x0623AC` (922 calls, displacement `0xEEEC`, return PC `0x0624A4`).

Historical working documentation labeled this routine as a "Coordinate Unpacker". Forensic disassembly and runtime analysis disproved this label: `0x06138E` is Sound Channel Envelope Parameter Staging & Modulation Init inside the Ancient Music Driver. It reads envelope configuration fields from channel RAM context `(A6)` (`+$78` -> `+$7E`, `+$79` -> `+$7F`, `+$7C` -> `+$80`), divides modulation frequency by 2 via `lsr.w #1, d6` (`+$7A` -> `D6` -> `+$82`), resets modulation timer `+$76` to 0, and returns via RTS.

An initial implementation produced a divergence at frame 523 due to missing dynamic shift cycle compensation on `lsr.w #1, d6` (`0xE24E`) and emulating RTS inside the stepper rather than yielding `NotHandled` for native Musashi instruction retirement.

**Decision:**
1. **Disprove Legacy Semantics**: Formally reclassify `0x06138E` as Sound Channel Envelope Parameter Staging & Modulation Init. Confirm zero coordinate/display/VDP interactions.
2. **Modular Child Dispatch**: Wire callers `0x061A12` and `0x0624A0` to invoke `step_m68k_6138e_instruction()` upon entry `PC == 0x06138E`.
3. **Exact Memory Bus Ordering**: Replicate exact Musashi bus interleaving:
   - Fetch source displacement word from PC (`pc + 2`).
   - Read source byte/word from RAM context `d16(a6)`.
   - Fetch destination displacement word from PC (`pc + 2`).
   - Write destination byte/word to RAM context `d16(a6)`.
4. **Dynamic Shift Timing Recovery**: For `0xE24E` (`lsr.w #1, d6`), add `shift_count * 2 * 7 = +14` master cycles to the core step before `finish_instruction`.
5. **Native RTS Pipeline Handoff**: Return `NotHandled` at opcode `0x4E75` (RTS at `0x0613B0`), allowing Musashi to natively execute SP pop, return address tracking, and prefetch.
6. **Census Reconciliation**: Reconcile call census to explicitly preserve `0x00E268` in the active `--native-island` helper set (`CPP_REPLACEMENT_VERIFIED_PATH_SPECIFIC = YES`), distinguishing logical call volume from actual remaining untranslated ROM execution.

**Consequences:**
The 2,609 ROM calls to `0x06138E` are displaced with 100% native execution and 0 fallbacks across all 3,600 frames. Bit-for-bit identity is preserved across all six diagnostic streams (`observed-landmarks`, `full-frame-state`, `architectural-sync`, `scheduler-sync`, `z80-ram-sync`, `call-z80-ram`) with `different_frame_count == 0`. The new #1 remaining untranslated ROM target is identified as `0x00E682` (2,104 calls).

## ADR-M15-29: Modular Native Displacement of 0x00E682 and Caller Stack Bypass Architecture

**Status:** Accepted for Milestone M15.29
**Date:** 2026-10-16

**Context:**
Following displacement of child helper `0x06138E`, the census model ranked `0x00E682` as the #1 remaining untranslated ROM target with 2,104 calls across 3,600 canonical frames. Disassembly and runtime tracing revealed calls from 6 distinct callers:
- `0x0057D0`: 772 calls (Player update status/animation tick, `A6 = 0x00FF19E8`)
- `0x01A54E`: 424 calls (Entity handler)
- `0x01BF84`: 350 calls (Entity handler)
- `0x01BBF8`: 337 calls (Entity handler)
- `0x01C4DC`: 116 calls (Entity handler)
- `0x01B7F2`: 105 calls (Entity handler)

Static analysis of `0x00E682..0x00E6BA` (56 bytes, 28 words, 16 instructions) revealed it is the Entity Action / Animation Freeze Timer & Damage Stun Gatekeeper. It tests entity state in WRAM `(A6)`:
1. `+$9D(A6)`: Stun timer / hurt flash counter.
   - If non-zero, checks if $\ge 20$. If $< 20$, sets bit 6 of `$36(A6)` (hurt flash flag).
   - Pops the caller return address via `addq.l #4, sp` and executes `rts` at `0x00E6A8`, returning directly to the caller's caller (the parent entity dispatcher), completely bypassing the caller's remaining execution for that frame.
2. `+$82(A6)`: Action / animation freeze timer.
   - If non-zero, increments animation counter `$2E(A6)` and decrements `$82(A6)`.
   - If `$82(A6)` decrements to zero (`beq.s` taken to `0x00E6A8`), exits via normal `rts` without popping the caller.
   - If `$82(A6)` is still $> 0$, executes `addq.l #4, sp` at `0x00E6A6` and exits via `rts` at `0x00E6A8`, popping the caller's stack frame and returning directly to the grandparent entity dispatcher.
3. If both `+$9D(A6) == 0` and `+$82(A6) == 0`, clears bit 6 of `$36(A6)` (`bclr #6, $36(a6)`) and exits normally via `rts` at `0x00E698`.

This explained why historical call census reported `COMPLETED_RETURNS: 2061` out of 2,104 calls (exactly 43 calls took the stack-popping bypass path).

**Decision:**
1. **Model Stack Bypass Flow**: Implement `m68k_00e682_instruction_step` in `src/tools/hybrid/m68k_00e682_instruction_step.hpp` and `.cpp`. When executing `addq.l #4, sp` (`0x584F` at `0x00E6A6`), pop the caller's return PC off the guest stack so that the subsequent `rts` at `0x00E6A8` returns cleanly to the grandparent return address with full prefetch and cycle tracking.
2. **Exact Branch Timing**: For `bne.w` (`0x6600`) and `bcc.w` (`0x6400`), add `+14` master cycles for not-taken branches via `api.word_not_taken_adjustment`. For `beq.s` (`0x6700`), add `-14` master cycles via `api.byte_not_taken_adjustment`.
3. **Strict Caller Verification**: Restrict native entry strictly to the 6 verified callers `{0x0057D0, 0x01A54E, 0x01BF84, 0x01BBF8, 0x01C4DC, 0x01B7F2}`, validating that `(SP)` matches `caller_pc + 6` and ROM opcode at `caller_pc` is `jsr $0000E682` (`0x4EB9 0000 E682`).
4. **Reconciled Census Ranking**: Preserve all 22 active native helpers in `--native-island`. Use remaining untranslated ROM executions (Ranking B) for the NEXT target selection.

**Consequences:**
All 2,104 calls to `0x00E682` are displaced with 100% native execution and 0 fallbacks across all 3,600 frames (2,030 normal path 1, 31 timer expired path 2a, 43 caller bypass path 2b). All 6 diagnostic streams match bit-for-bit with `different_frame_count == 0`. All top 16 logical call targets are now 100% native. The new #1 remaining untranslated ROM target is `0x0613F8` (1,687 calls from `0x061A16` inside native `0x061934`).

## ADR-M15-30: Modular Native Displacement of 0x0613F8 and Dual-Slot Modulation Initializer Verification

**Status:** Accepted for Milestone M15.30
**Date:** 2026-10-16

**Context:**
Following displacement of entity timer gatekeeper `0x00E682`, the census model ranked `0x0613F8` as the #1 remaining untranslated ROM target with 1,687 calls across 3,600 canonical frames, called exclusively from `0x061A16` (immediately following `0x061A12 -> 0x06138E`) inside native `0x061934`.

Disassembly and runtime forensic analysis of `0x0613F8..0x06147E` (134 bytes, 67 words, 32 instructions) revealed it is the Sound Channel Dual Auxiliary Voice / Pitch-Modulation Slot Initializer:
1. Outer Routine (`0x0613F8..0x061438`):
   - Preserves `D0, A0, A1` on stack via `movem.l d0/a0-a1, -(sp)`.
   - Slot 1 check (`+$B6(A6)`): checks bits 4 and 5 (`btst #4, 0(a0)`, `btst #5, 0(a0)`). If both are set, calls `0x06143A` via `bsr.w 0x06143A`.
   - Slot 2 check (`+$C0(A6)`): checks bits 4 and 5 (`btst #4, 0(a0)`, `btst #5, 0(a0)`). If both are set, calls `0x06143A` via `bsr.w 0x06143A`.
   - Restores registers via `movem.l (sp)+, d0/a0-a1` and returns via `rts` at `0x061438`.
2. Sub-helper Routine (`0x06143A..0x06147C`):
   - Unpacks status byte lower 4 bits into `5(A0)`, clears `6(A0)`, `7(A0)`, and `4(A0)` via `sf`, copies initial parameter from `3(A0)` to `6(A0)`.
   - Compares `8(A0)` with `0xB0`:
     * If $\ge \text{0xB0}$ (Default Path): loads byte from `+$18(A1)` (`+$B5(A6)`) to `9(A0)`, returns via `rts` at `0x06146E`.
     * If $< \text{0xB0}$ (Table Path): shifts `d0 >>= 2` (`lsr.b #2, d0`, +28 master cycles), zero-extends to word, loads byte from table `0(A1, D0.W)` (`+$9D(A6) + index`) to `9(A0)`, returns via `rts` at `0x06147C`.
3. In the 3,600-frame canonical gameplay, both slots have bit 4 clear across all 1,687 calls, traversing the fast path (8 instructions per call, 13,496 total stepped instructions).

**Decision:**
1. **Instruction-Stepped Native Replacement**: Implement `step_m68k_0613f8` in `src/tools/hybrid/m68k_0613f8_instruction_step.hpp` and `.cpp`.
2. **Exact MOVEM Bus & Cycle Accounting**:
   - `movem.l d0/a0-a1, -(sp)`: add 168 master cycles ($3 \times 8 \times 7$), write order A1, A0, D0, call `skip_bus_refresh`.
   - `movem.l (sp)+, d0/a0-a1`: post-increment dummy word read at `new_sp`, add 168 master cycles ($3 \times 8 \times 7$), call `skip_bus_refresh`.
3. **Fail-Closed Contract**: Strictly validate caller `0x061A16`, return PC `0x061A1A`, A6 in work RAM (`0x00FF0000..0x01000000`), and ROM bytes match canonical image.
4. **Reconciled Census Model**: Keep all 23 accepted native helpers active in `--native-island`. Use remaining untranslated ROM executions (Ranking B) for NEXT target selection.

**Consequences:**
All 1,687 calls to `0x0613F8` are displaced with 100% native execution and 0 fallbacks across all 3,600 frames. All 6 diagnostic streams (`observed-landmarks`, `full-frame-state`, `architectural-sync`, `scheduler-sync`, `z80-ram-sync`, `call-z80-ram`) match bit-for-bit with `different_frame_count == 0`. All top 17 logical call targets are now 100% native. The new #1 remaining untranslated ROM target is `0x016EBC` (1,310 calls).

## ADR-M15-31: Modular Native Displacement of 0x016EBC and Interactive Entity Handler Verification

**Status:** Accepted for Milestone M15.31
**Date:** 2026-10-16

**Context:**
Following displacement of Ancient audio modulation slot initializer `0x0613F8`, census Ranking B identified `0x016EBC` as the #1 remaining untranslated ROM target with 1,310 calls across the 3,600-frame canonical replay. Disassembly and forensic analysis revealed that `0x016EBC` is an interactive world object / chest / event entity update and initialization handler dispatched from the main entity update loop (`0x00DF1A`, 1,306 calls) and entity spawn/setup routine (`0x00FF88`, 4 calls).
The routine spans `0x016EBC..0x016F0E` (84 bytes, 42 words, 23 instructions) with two primary execution paths:
1. **Initialization Path (`0x016EBC..0x016EE8`, RTS)**:
   - Executed when `0xA6(a6) == 0`. Sets initialization flag `0xA6(a6) = 0xFF`.
   - Copies initial sprite/frame index: `16(a6) = 140(a6)` (`move.w $8C(a6), $10(a6)`).
   - If property `158(a6) == 0` (`0x9E(a6)`): initializes delay/timer `74(a6) = 16` (`move.w #16, $4A(a6)`).
   - If property `158(a6) == 1`: sets interaction/collision flag bit 0 of `60(a6)` (`bset #0, $3C(a6)`).
   - Returns via `rts` at `0x016EE8`. (Observed 4 calls from `0x00FF88`).
2. **Update Path (`0x016EBC..0x016F0E`, RTS)**:
   - Executed when `0xA6(a6) != 0`. Inspects interaction/defeat trigger byte `102(a6)` (`0x66(a6)`).
   - If `0x66(a6) == 0` (idle/waiting): branches directly to `0x016F0E` and returns via `rts`. (Observed 1,306 calls from `0x00DF1A`).
   - If `0x66(a6) != 0` (triggered/defeat/open): unobserved in canonical replay; increments counter `2(a6)` up to 6, clears slot `0(a6) = 0`, calls `0x018030` to lookup persistent flag bit, and executes `bset d0, (a0)`.

**Decision:**
1. **Instruction-Stepped Native Replacement**: Implement `step_m68k_016ebc` in `src/tools/hybrid/m68k_016ebc_instruction_step.hpp` and `.cpp`.
2. **Exact Memory-to-Memory Ordering**:
   - `move.w $8C(a6), $10(a6)`: fetch source displacement `0x008C`, read source word from `a6 + 0x8C`, fetch destination displacement `0x0010`, write destination word to `a6 + 0x10`, update N/Z flags.
3. **Fail-Closed Contract**: Validate callers `0x00DF1A` and `0x00FF88` (return PCs `0x00DF1C` and `0x00FF8A`), valid SP in work RAM, valid A6 entity structure in work RAM (`0x00FF0000..0x01000000`), and ROM image verification (42 words). Unobserved trigger paths fall back to Musashi ROM fail-closed.
4. **Reconciled Census Model**: Keep all 24 accepted native helpers active in `--native-island`. Use remaining untranslated ROM executions (Ranking B) for NEXT target selection.

**Consequences:**
All 1,310 calls to `0x016EBC` are displaced with 100% native execution and 0 fallbacks across all 3,600 frames. All 6 diagnostic streams (`observed-landmarks`, `full-frame-state`, `architectural-sync`, `scheduler-sync`, `z80-ram-sync`, `call-z80-ram`) match bit-for-bit with `different_frame_count == 0`. All top 18 logical call targets are now 100% native. The new #1 remaining untranslated ROM target is `0x009AD6` (1,283 calls, tied with `0x009BF2`).

## ADR-M15-42: Modular Native Displacement of 0x00350E and Title Screen Blinking Text Dispatcher

**Status:** Accepted for Milestone M15.42
**Date:** 2026-10-17

**Context:**
Following displacement of Entity Type 0x38 Damage & Hit Processing Handler `0x01B102`, the authoritative fresh actual-ROM execution census ranked `0x00350E` as the #1 remaining untranslated ROM target with 866 calls across 3,600 canonical frames, called exclusively from title screen wait loop `0x0033FE` (return PC `0x003402`).

Disassembly and runtime forensic analysis of `0x00350E..0x003534` (38 bytes, 19 words, 9 instructions) revealed it is the Title Screen "PRESS START BUTTON" Blinking String Dispatcher:
1. Operation:
   - Evaluates frame counter `0x00FF1654`: `(0x00FF1654 & 0x7F) < 0x60` (visible phase, 96 frames of 128-frame cycle) vs `(0x00FF1654 & 0x7F) >= 0x60` (blank phase, 32 frames of 128-frame cycle).
   - If visible (`bcs.s 0x3526` taken): sets `a6 = 0x00003534` (pointer to ASCII `"PRESS START BUTTON\0"`). Observed 655 times.
   - If blank (`bcs.s` not taken): falls through to `0x3522` and sets `a6 = 0x00003547` (pointer to ASCII `"                  \0"`). Observed 211 times.
   - Sets VDP VRAM Plane A write command: `d5 = 0x4A160003`.
   - Sets base tile/palette attribute: `d6.w = 0x61C1` (preserving upper 16 bits of `D6`).
   - Clears CCR condition codes `N=0, Z=0, V=0, C=0` (preserving `X`).
   - Tail-branches via `bra.w 0x002CE4` to general text string VDP tilemap renderer at `0x002CE4`.
2. Total execution in 3,600 frames: 866 calls (655 visible branches, 211 blank branches), 7,139 stepped instructions ($655 \times 8 + 211 \times 9 = 7139$).

**Decision:**
1. **Instruction-Stepped Native Replacement**: Implement `step_m68k_00350e` in `src/tools/hybrid/m68k_00350e_instruction_step.hpp` and `.cpp`.
2. **Exact Tail-Branch Hand-off**: Execute `bra.w 0x002CE4` by updating CPU return state to `0x002CE4` and completing instruction stepping with `active = false`.
3. **Fail-Closed Contract**: Strictly validate caller `0x0033FE` (return PC `0x003402`), SP in Work RAM (`0x00FF0000..0x01000000`), and all 19 ROM opcode words against canonical ROM image.
4. **Reconciled Census Model**: Keep all 36 accepted native helpers active in `--native-island`. Use remaining untranslated ROM executions for NEXT target selection.

**Consequences:**
All 866 calls to `0x00350E` are displaced with 100% native execution and 0 fallbacks across all 3,600 frames. All 6 canonical diagnostic streams (`observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, `call-z80-ram.bin`) plus `external-events.jsonl` and `normalized-events.jsonl` match bit-for-bit with `different_frame_count == 0` and `divergence=NO`. The new #1 remaining untranslated ROM target is `0x008E22` (842 calls).

## ADR-M15-43: Modular Native Displacement of 0x008E22 and Hardware Animated Palette CRAM Engine

**Status:** Accepted for Milestone M15.43
**Date:** 2026-10-17

**Context:**
Following displacement of Title Screen Blinking Text Dispatcher `0x00350E`, the authoritative fresh actual-ROM execution census ranked `0x008E22` as the #1 remaining untranslated ROM target with 842 calls across 3,600 canonical frames, called from:
- `0x0007B8` (70 calls, return PC `0x0007BE`, VBlank ISR)
- `0x008B3E` (772 calls, return PC `0x008B42`, Main Graphics Loop)

Disassembly and runtime forensic analysis of `0x008E22..0x008E90` (110 bytes, 55 words, 29 instructions) revealed it is the Hardware Animated Palette CRAM Engine:
1. Operation:
   - Evaluates active flag byte `0x00FF164F`: if zero, fast exits via `beq.w 0x008E8E` (`RTS`).
   - If non-zero (842 calls): loads animated palette record list head pointer `A0` from `0x00FF17A2`, sets CRAM shadow destination pointer `A1 = 0x00FF134C`, and clears dirty flag `D5.B = 0`.
   - Iterates through linked record list at `(A0)`:
     - Decrements frame countdown timer `6(A0)`.
     - When timer expires (728 keyframe advances): advances frame offset `4(A0) += 4`, loads new duration from `0(A0, D1.W)`. If duration == 0, wraps back to keyframe 0 (`D1 = 8`, 164 loops). Writes updated offset to `4(A0)`, reads color word from `2(A0, D1.W)`, writes color word to `0(A1, D0.W)` (where `D0 = 2(A0)`), and marks dirty `D5.B = 0xFF` (728 CRAM buffer writes).
     - Saves remaining timer back to `6(A0)`.
     - Advances `A0 += (A0)` until next offset is 0 (3,508 records processed).
   - If dirty (`D5 != 0`), sets DMA transfer pending flag bit 0 of `0x00FF164D` (173 DMA sets).
   - Pops return PC from stack and returns via `RTS`.
2. Total execution in 3,600 frames: 842 calls (70 VBlank ISR, 772 Graphics loop), 47,465 stepped instructions, 0 ROM fallbacks.

**Decision:**
1. **Instruction-Stepped Native Replacement**: Implement `step_m68k_008e22` in `src/tools/hybrid/m68k_008e22_instruction_step.hpp`, `.cpp`, and `m68k_008e22_step_cycle.hpp`, `.cpp` (split to ensure each file <= 500 lines).
2. **Fail-Closed Contract**: Strictly validate callers `0x0007B8` (return PC `0x0007BE`) and `0x008B3E` (return PC `0x008B42`), SP in Work RAM, and all 55 ROM opcode words against canonical ROM image.
3. **Canonical Replay Verification**: Verify bit-for-bit SHA-256 equivalence across all 6 canonical streams plus `external-events.jsonl` and `normalized-events.jsonl` with `different_frame_count == 0` and `divergence=NO`.

**Consequences:**
All 842 calls to `0x008E22` are displaced with 100% native execution and 0 fallbacks across all 3,600 frames. All 8 diagnostic streams match bit-for-bit. The new #1 remaining untranslated ROM target is `0x00A7E6` (811 calls).

## ADR-M15-44: Modular Native Displacement of 0x00A7E6 and HUD Status Overlay Sprite Generator

**Status:** Accepted for Milestone M15.44
**Date:** 2026-10-17

**Context:**
Following displacement of `0x008E22`, the authoritative fresh actual-ROM execution census ranked `0x00A7E6` as the #1 remaining untranslated ROM target with 811 calls across 3,600 canonical frames, called from:
- `0x00A336` (773 calls, return PC `0x00A33A`, HUD update path)
- `0x00A694` (38 calls, return PC `0x00A698`, alternate HUD update path)

Disassembly and runtime forensic analysis of `0x00A7E6..0x00A8DA` (244 bytes, 122 words, 51 instructions) revealed it is the HUD Status Overlay Sprite Generator & VRAM DMA Transfer Queuer:
1. Operation:
   - Reads HUD inhibit flag byte `0x00FF1656`: if bit 7 is set (376 calls), fast exits via `bne.w 0x00A878` (`RTS`).
   - If bit 7 is clear (435 calls): loads base sprite table address `A5 = 0x00FF13CC + $ff188c`, reads sprite link/index `D5 = $ff188a`, loads Y coordinate base `D2 = 0x00E4`.
   - Tests bit 6 of `0x00FF1656`:
     - Mode A (bit 6 clear, 212 calls): sets `D1 = 0x00F0`, `D4 = 0x0C00`, `D3 = 0x879B`, calls child helper `0x00A8DA` 3 times with `D1` advancing by `0x0020` each time and `D3` cycling `0x879B`, `0x879F`, `0x87A3` (636 child calls). Subtracts base `A5 -= 0x00FF13CC` and writes offset back to `0x00FF188C`, updates `0x00FF188A = D5`. Checks transfer state `0x00FF17C0`: if not 1, sets it to 1, writes 10 bytes to DMA queue at `(A5)+` from pointer `0x00FF1892`, and updates write pointer (7 DMA transfer queues).
     - Mode B (bit 6 set, 223 calls): sets `D1 = 0x010C`, `D4 = 0x0800`, `D3 = 0x879B`, calls child helper `0x00A8DA` 2 times with `D1` advancing by `0x0018`, `D4 = 0x0400`, `D3 = 0x879E` (446 child calls). Subtracts base `A5 -= 0x00FF13CC` and writes offset back to `0x00FF188C`, updates `0x00FF188A = D5`. Checks transfer state `0x00FF17C0`: if not -1, sets it to -1, writes 10 bytes to DMA queue at `(A5)+` from pointer `0x00FF1892`, and updates write pointer (7 DMA transfer queues).
   - All 5 child `bsr.w 0x00a8da` calls target the already-accepted native instruction-stepped helper `m68k_00a8da_instruction_step.cpp`.
2. Total execution in 3,600 frames: 811 calls (773 from `0x00A336`, 38 from `0x00A694`), 12,017 stepped instructions, 1,082 child calls, 0 ROM fallbacks.

**Decision:**
1. **Instruction-Stepped Native Replacement**: Implement `step_m68k_00a7e6` modularized into `src/tools/hybrid/m68k_00a7e6_instruction_step.hpp`, `.cpp`, `m68k_00a7e6_step_mode_a.hpp`, `.cpp`, and `m68k_00a7e6_step_mode_b.hpp`, `.cpp` (strictly preserving the <= 500 lines per file policy).
2. **Direct Child BSR Dispatch**: Dispatch `0x00A8DA` BSR calls directly prior to opcode preparation so that the child helper seamlessly manages its own instruction stepping and stack frames without word-boundary desynchronization.
3. **Fail-Closed Contract**: Strictly validate callers `0x00A336` (return PC `0x00A33A`) and `0x00A694` (return PC `0x00A698`), SP in Work RAM, and all 122 ROM opcode words against canonical ROM image.
4. **Canonical Replay Verification**: Verify bit-for-bit SHA-256 equivalence across all 6 canonical streams plus `external-events.jsonl` and `normalized-events.jsonl` with `different_frame_count == 0` and `divergence=NO`.

**Consequences:**
All 811 calls to `0x00A7E6` are displaced with 100% native execution and 0 fallbacks across all 3,600 frames. All 8 diagnostic streams match bit-for-bit. The new #1 remaining untranslated ROM target is `0x008E90` (775 calls).

## ADR-0044: Instruction-Stepped Native Displacement of Active Entity Pool Scan & Master Physics/Animation Update Loop (`0x008E90`)

**Context:**
The canonical 3,600-frame replay census identified `0x008E90` as the #1 remaining untranslated actual-ROM target with 775 calls.
Disassembly and forensic execution analysis of `0x008E90..0x008EF6` and its branch sub-networks (`0x008FA4..0x00938C`, `0x009708..0x0098E8`, total 505 instructions) revealed it is the Active Entity Pool Scan & Master Physics/Animation Update Loop:
1. Operation:
   - Scans three active entity pools:
     * Pool 1: `0xFF1770` (32 slots, stride 90 bytes).
     * Pool 2: `0xFF19E8` (21 slots, stride 188 bytes).
     * Pool 3: `0xFF2D8C` (6 slots, stride 90 bytes).
   - In each active entity slot, performs active flag tests, collision mask checks, animation sequencing, and X/Y physics calculations with terrain obstacle checking.
   - Dispatches child helper subroutines: `0x009AD6` (1,241 calls), `0x009BF2` (1,241 calls), `0x009D4E` (113 calls), `0x009D70` (186 calls), `0x009D92` (232 calls), `0x009DB4` (507 calls).
   - Manages non-native child routines (`0x00938E`, `0x0094D2`, `0x0095AA`, `0x0098EC`, `0x009DD6`, `0x009EBA`, `0x009FA0`, `0x00A09A`, 1,360 total calls) via return PC latching and stack pointer tracking.
2. Architectural findings:
   - Dynamic shift cycle adjustments: immediate shifts (`asr.l #1`, `asr.w #3`) add `shift * 14` master cycles.
   - `MOVEM.W` register popping: M68000 sign-extends 16-bit words into 32-bit registers upon stack pop. Preserving the upper word caused frame 2027 stack residue divergence (`0xFF0BD1 = 0x80` instead of `0x00`), which sign-extension resolved bit-for-bit.
   - Loop branch cycles: `dbf` looping subtracts 14 master cycles; loop expiration adds 14 master cycles.

**Decision:**
1. **Modular Instruction Stepping**: Modularize `0x008E90` across multiple small files under `namespace oasis::hybrid::step_008e90` (`m68k_008e90_instruction_step.cpp`, `m68k_008e90_step_ops.cpp`, `m68k_008e90_step_scan.cpp`, `m68k_008e90_step_anim.cpp`, `m68k_008e90_step_move_x.cpp`, `m68k_008e90_step_move_y.cpp`, `m68k_008e90_step_move_alt.cpp`), strictly keeping every file under 500 lines.
2. **Fail-Closed Verification**: Reject any call not originating from `0x008B42` (return PC `0x008B46`) or `0x00CD24` (return PC `0x00CD28`), or having an invalid SP, or mismatched ROM bytes.
3. **Dual Replay Validation**: Validate bit-identical SHA-256 equivalence across all 8 canonical output streams with `different_frame_count == 0` and `divergence=NO`.

**Consequences:**
All 775 calls to `0x008E90` in canonical replay are stepped in C++ with 0 ROM fallback calls and 430,427 instructions stepped. All 8 streams match bit-for-bit. The new #1 remaining non-displaced target is `0x00F788` (775 calls).

## ADR-0045: Instruction-Stepped Native Displacement of Event Script & Cutscene / Camera Scroll Sequencer (`0x00F788`)

**Context:**
The canonical 3,600-frame replay census identified `0x00F788` as the undisputed #1 remaining untranslated actual-ROM target with 775 calls (callers `0x008B2A`: 772 in the main game loop, `0x00CD02`: 3 in secondary loop).
Analysis and reverse-engineering of `0x00F788..0x00F920` (and child script commands at `0x00F854..0x010252`) revealed it is the Event Script & Cutscene / Camera Scroll Sequencer:
1. Operation:
   - Evaluates master event script inhibit flag: `btst #1, $FF17B8`. If bit 1 is set, exits immediately via `0x00F794` RTS (30 calls in 3,600 frames).
   - Evaluates script wait countdown timer: loads `$FF17B6`. If non-zero, decrements `$FF17B6` and exits via `0x00F7A8` RTS (15 calls in 3,600 frames).
   - Evaluates camera pan tick countdown: loads `$FF17BD`. If non-zero, dispatches camera pan tick via jump table `0x00F8C8` for directions 0..3 (`0x00F8D0`, `0x00F8E4`, `0x00F8F6`, `0x00F908`), decrements `$FF17BD`, and clears pan flags at expiration via `0x00F8C6` RTS.
   - If camera pan countdown is zero, enters the Script Sequencer Loop (`0x00F7B4`): loads script byte pointer from `$FF17B2`, reads command bytes through `(A0)+`, indexes jump table at `0x00F7D0`, and dispatches child script commands via `jsr (A1)` (760 child script commands executed across 730 script calls).
   - Early script command terminations: Commands 00, 06, 07, 08, 09, 16, 18, 19, and 1E pop the child return address from the stack (`addq.l #4, sp`) and execute `rts` straight to the caller of `0x00F788` (`0x008B2E` / `0x00CD06`).
2. Architectural findings:
   - Early child script terminations pop the JSR return address (`0x00F7C8`) and return directly to the outer caller. Native tracking must recognize caller return addresses (`0x008B2E`, `0x00CD06`) and early exit points (`0x00F890/0x00F892`, `0x00F91C/0x00F91E`, `0x00FE06/0x00FE08`, `0x00FF94/0x00FF96`) to avoid state latching across frames.

**Decision:**
1. **Modular Instruction Stepping**: Modularize `0x00F788` into small files under `namespace oasis::hybrid::step_00f788` (`m68k_00f788_instruction_step.cpp`, `m68k_00f788_step_ops.cpp`, `m68k_00f788_step_seq.cpp`), strictly keeping every file under 500 lines.
2. **Fail-Closed Verification**: Reject any call not originating from `0x008B2A` (return PC `0x008B2E`) or `0x00CD02` (return PC `0x00CD06`), or having an invalid SP, or mismatched ROM words.
3. **Dual Replay Validation**: Validate bit-identical SHA-256 equivalence across all 8 canonical output streams with `different_frame_count == 0` and `divergence=NO`.

**Consequences:**
All 775 calls to `0x00F788` in canonical replay are stepped in C++ with 0 ROM fallback calls and 11,189 instructions stepped (30 inhibit exits, 15 timer exits, 730 script entries, 760 child script commands, 632 command 00/early exits). All 8 streams match bit-for-bit. The new #1 remaining non-displaced target is `0x00A196` (773 calls).

## ADR-0046: Instruction-Stepped Native Displacement of Sprite Table Committer & Multi-Pool Display Dispatcher (`0x00A196`)

**Context:**
The canonical 3,600-frame replay census identified `0x00A196` as the #1 remaining untranslated actual-ROM target with 773 calls (callers `0x008B86`: 770 in the main game loop, `0x00CD28`: 3 in secondary loop).
Analysis and reverse-engineering of `0x00A196..0x00A33E` revealed it is the Sprite Table Committer & Multi-Pool Display Dispatcher:
1. Operation:
   - Clears `$FF1651` via `sf $FF1651`, dispatches base sprite builder `0x00A342` via `bsr.w 0x00A342` (773 calls).
   - Evaluates `$FF1996` via `tst.b $FF1996`; if non-zero, calls HUD sprite generator `0x00A7E6` via `bsr.w 0x00A7E6` at `0x00A336` (773 calls).
   - Scans 5 display entity pools: pool `$FF2954` (4 slots * 90 bytes), pool `$FF19C8` (4 slots * 8 bytes), pool `$FF19E8` (21 slots * 188 bytes), pool `$FF2ABC` (8 slots * 90 bytes), and pool `$FF2D8C` (6 slots * 90 bytes).
   - Arbitrates candidate priorities across slots using `D7` bitmask manipulation (`bset` and `btst`).
   - Dispatches child helper subroutines by pushing return addresses onto the stack (`$00A1F8`, `$00A1BA`, `$00A270`, `$00A258`, `$00A1D0`) and branching via `beq.w 0x00AD76` (6,246 calls), `bra.w 0x00A9A0` (2,691 calls), or `jmp (a0)` (90 calls).
   - Completes SAT and stages DMA (`0x00A2A6`): writes zero-terminator `-5(a6, d7.w) = 0`, sets bits 0 and 1 of `$FF164E` (`orib #3, $FF164E`) to trigger SAT DMA commit, and exits via `0x00A2C6` RTS.
2. Architectural findings:
   - Motorola 68000 / Musashi `BSET Dn, Dm` cycle scaling: Musashi's `m68k_op_bset_32_r_d` evaluates `if ((1 << (bit & 31)) & 0xFFFF0000) USE_CYCLES(2 * MUL);`. When `bit >= 16`, Musashi adds `2 * MUL = 14` master cycles. In the 21-iteration loop starting with `d6 = 20`, exactly 5 iterations (bits 20, 19, 18, 17, 16) added 14 cycles each, causing a 70-cycle delta when omitted. Implementing `cycle_adj = (bit >= 16U) ? 14 : 0` in `step_bset_reg_reg` achieved bit-identical cycle parity.

**Decision:**
1. **Modular Instruction Stepping**: Modularize `0x00A196` into small files under `namespace oasis::hybrid::step_00a196` (`m68k_00a196_instruction_step.cpp`, `m68k_00a196_step_ops.cpp`, `m68k_00a196_step_seq.cpp`), strictly keeping every file under 500 lines.
2. **Fail-Closed Verification**: Reject any call not originating from `0x008B86` (return PC `0x008B8A`) or `0x00CD28` (return PC `0x00CD2C`), or having an invalid SP, or mismatched ROM words.
3. **Dual Replay Validation**: Validate bit-identical SHA-256 equivalence across all 8 canonical output streams with `different_frame_count == 0` and `divergence=NO`.

**Consequences:**
All 773 calls to `0x00A196` in canonical replay are stepped in C++ with 0 ROM fallback calls and 882,033 stepped instructions. All 8 streams match bit-for-bit across all 3,600 frames. The new #1 remaining non-displaced target is `0x00A342` (773 calls).

## ADR-0047: Instruction-Stepped Native Displacement of Base Sprite Builder & VDP DMA Queue Stager (`0x00A342`)

**Context:**
The canonical 3,600-frame replay census identified `0x00A342` as the #1 remaining untranslated actual-ROM target with 773 calls (caller `0x00A19C`: 773).
Analysis and reverse-engineering of `0x00A342..0x00A436` revealed it is the Base Sprite Builder & VDP DMA Queue Stager:
1. Operation:
   - Sets up shadow SAT buffer pointer `A5 = $FF13CC + $FF188C` and loads current sprite link index from `$FF188A` into `D5`.
   - Tests `$FF1858` to select primary sprite template table (`0x00A438`) or alternate template table (`0x00A480`).
   - Copies 6 base sprite attribute records (Loop 1, 6 iterations) from ROM table `(A0)+` into shadow SAT `(A5)+`, incrementing link counter `D5` and adding screen offset `$FF185A` to each sprite's X position (3,865 iterations total).
   - If secondary/inverted flag `$FF184F` is non-zero, computes `D3 = 344 - $FF185A` and copies 3 additional sprite entries (Loop 2).
   - If cursor/status sprite flag `$FF1856` is non-zero and `D5 < 80`, increments `D5` and emits an 8-byte sprite entry (Y=280, link=`D5`, tile=`0x85C7`, X=`$FF185A + 104`).
   - If byte `$FF1854` differs from `$FF1855`, updates `$FF1855`, computes ROM address `D0 = 0x000AAebb + D0 * 144`, disables interrupts via `ori.w #0x0700, sr`, stages an 8-byte DMA transfer record (ROM source `D0`, VRAM dest `0xB8E0`, length `0x0090`) into the DMA staging queue at `$FF1892`, advances pointer `$FF1892`, and re-enables interrupts via `andi.w #0xF9FF, sr` (1 DMA record queued).
   - Computes updated SAT displacement `suba.l #$FF13CC, a5`, writes back displacement to `$FF188C`, writes back link counter `D5` to `$FF188A`, and returns via `rts`.

**Decision:**
1. **Modular Instruction Stepping**: Modularize `0x00A342` into small files under `namespace oasis::hybrid::step_00a342` (`m68k_00a342_instruction_step.cpp`, `m68k_00a342_step_ops.cpp`, `m68k_00a342_step_seq.cpp`), strictly keeping every file under 500 lines.
2. **Fail-Closed Verification**: Reject any call not originating from `0x00A19C` (return PC `0x00A1A0`) or `0x00A6A0` (return PC `0x00A6A4`), or having an invalid SP, or mismatched ROM words.
3. **Dual Replay Validation**: Validate bit-identical SHA-256 equivalence across all 8 canonical output streams with `different_frame_count == 0` and `divergence=NO`.

**Consequences:**
All 773 calls to `0x00A342` in canonical replay are stepped in C++ with 0 ROM fallback calls and 55,011 stepped instructions. All 8 streams match bit-for-bit across all 3,600 frames. The new #1 remaining non-displaced target is `0x00B34C` (773 calls).

## ADR-M15-56: Instruction-Stepped Native Displacement of Player Menu, Pause & Inventory Coordinator (`0x008BAC`)

**Context:**
The canonical 3,600-frame replay census identified `0x008BAC` as the #1 remaining untranslated actual-ROM target with 772 calls (caller `0x008B22`: 772).
Analysis of `0x008BAC..0x008C5A` revealed it is the Player Menu, Pause & Inventory Coordinator:
1. Operation:
   - Resets pending menu request via `clr.b $FF185E`.
   - Checks system/mode inhibit flags: `$FF1987`, `$FF17B8` (bit 2), `$FF1983`, and `$FF1982`. If any flag is asserted, branches directly to `rts` at `0x008C34`.
   - Reads controller state words `$FF165E`, `$FF165F`, `$FF1660`, testing pause and start triggers.
   - If menu/inventory is requested, sets `$FF185E` (values 1, 2, or 3) and executes tail jump to menu loop `0x003B3E`.
   - In canonical gameplay, menu is never requested, resulting in all 772 calls completing through input evaluation and returning cleanly via `0x008C34` `rts`.

**Decision:**
1. **Modular Instruction Stepping**: Modularize `0x008BAC` into `m68k_008bac_instruction_step.{hpp,cpp}` and `m68k_008bac_step_ops.{hpp,cpp}`, strictly observing the 500-line limit.
2. **Fail-Closed Verification**: Reject any call not originating from `0x008B22` (return PC `0x008B26`), or having an odd/invalid SP, or mismatched ROM words.
3. **Dual Replay Validation**: Verify bit-identical SHA-256 equivalence across all 8 canonical streams across all 3,600 frames (`different_frame_count == 0`, `divergence=NO`).

**Consequences:**
All 772 calls to `0x008BAC` in canonical replay are stepped in C++ with 0 ROM fallback calls (100% displaced, 17,262 instructions stepped: 772 clean RTS completions). All 8 canonical streams match bit-for-bit. The new #1 remaining non-displaced target is `0x00DEEC` (772 calls).

## ADR-M15-57: Instruction-Stepped Native Displacement of Master Active Entity Tick & Isometric Projection Loop (`0x00DEEC`)

**Context:**
The canonical 3,600-frame replay census identified `0x00DEEC` as the #1 remaining untranslated actual-ROM target with 772 calls (caller `0x008B32`: 772).
Analysis of `0x00DEEC..0x00DF52` revealed it is the Master Active Entity Tick & Isometric Velocity Projection Loop:
1. Operation:
   - Evaluates inhibit flag `$00FF1988`; if set, immediately returns via `rts` at `0x00DF50` (30 occurrences).
   - Iterates through 20 entity slots (`%a6 = $00FF1AA4`, stride 188 bytes, `%d7` counter 19..0).
   - Evaluates state code `4(%a6)`. Inactive/sub-threshold entities (`%d6 < 22`) advance directly.
   - For active entities (`%d6 >= 22`), saves `%d7/%a6` to stack via `movem.l`, invokes indirect handler via `0x00DF28(%pc, %d6.w)`, restores `%d7/%a6`, unpacks velocity deltas `72(%a6)` and `73(%a6)`, computes isometric position adjustments, and advances to the next slot.
   - Canonical M68K cycle timing requires accurate handling of `dbf` branches (taken: -14 master cycles, expired: +14 master cycles), `movem.l` push/pop (112 master cycles each with bus refresh cycles and pop dummy read), and `lsr.w #1` (+14 master cycles).

**Decision:**
1. **Modular Instruction Stepping**: Modularize `0x00DEEC` into `m68k_00deec_instruction_step.{hpp,cpp}` and `m68k_00deec_step_ops.{hpp,cpp}`, strictly observing the 500-line limit.
2. **Fail-Closed Verification**: Reject any call not originating from `0x008B32` (return PC `0x008B36`), or having an odd/invalid SP, or mismatched ROM words.
3. **Exact Cycle Modeling**: Model exact hardware cycle timing adjustments for `dbf`, `movem.l`, and `lsr.w` in the instruction-stepping loop.
4. **Dual Replay Validation**: Verify bit-identical SHA-256 equivalence across all 8 canonical streams across all 3,600 frames (`different_frame_count == 0`, `divergence=NO`).

**Consequences:**
All 772 calls to `0x00DEEC` in canonical replay are stepped in C++ with 0 ROM fallback calls (100% displaced, 161,708 instructions stepped: 30 inhibit exits, 742 loop completions, 4,895 child jsr calls). All 8 canonical streams match bit-for-bit. The new #1 remaining non-displaced target is `0x00E0BA` (772 calls).

## ADR-M15-58: Instruction-Stepped Native Displacement of Secondary / Auxiliary Entity Slot Dispatch Loop (`0x00E0BA`)

**Context:**
The canonical 3,600-frame replay census identified `0x00E0BA` as the #1 remaining untranslated actual-ROM target with 772 calls (caller `0x008B36`: 772).
Analysis of `0x00E0BA..0x00E0F4` revealed it is the Secondary / Auxiliary Entity Slot Dispatch Loop:
1. Operation:
   - Evaluates inhibit bit: `btst #1, $00FF17B8`; if set, immediately branches via `bne.w 0x00E0F2` to `rts` (30 occurrences).
   - Iterates through 6 entity slots (`%a6 = $00FF2D8C`, stride 90 bytes, `%d7` counter 5..0).
   - Evaluates active flag `0(%a6)` and handler pointer `86(%a6)`. Inactive or null slots branch directly to `0x00E0EA`.
   - For active slots with valid handlers, saves `%d7/%a6` to stack via `movem.l`, invokes indirect handler via `jsr (%a0)`, restores `%d7/%a6`, and advances to the next slot via `lea 90(%a6), %a6; dbf %d7, 0x00E0CE`.
   - Canonical M68K cycle timing requires accurate handling of `dbf` branches (taken: -14 master cycles, expired: +14 master cycles) and `movem.l` push/pop (112 master cycles each with bus refresh cycles and pop dummy read).
   - In canonical replay, all 6 slots are empty across non-inhibited frames, resulting in fast slot traversal via `beq.w 0x00E0EA`.

**Decision:**
1. **Modular Instruction Stepping**: Modularize `0x00E0BA` into `m68k_00e0ba_instruction_step.{hpp,cpp}` and `m68k_00e0ba_step_ops.{hpp,cpp}`, strictly observing the 500-line limit.
2. **Fail-Closed Verification**: Reject any call not originating from `0x008B36` (return PC `0x008B3A`), or having an odd/invalid SP, or mismatched ROM words.
3. **Exact Cycle Modeling**: Model exact hardware cycle timing adjustments for `dbf` and `movem.l` in the instruction-stepping loop.
4. **Dual Replay Validation**: Verify bit-identical SHA-256 equivalence across all 8 canonical streams across all 3,600 frames (`different_frame_count == 0`, `divergence=NO`).

**Consequences:**
All 772 calls to `0x00E0BA` in canonical replay are stepped in C++ with 0 ROM fallback calls (100% displaced, 21,608 instructions stepped: 30 inhibit exits, 742 loop completions, 0 child jsr calls). All 8 canonical streams match bit-for-bit. The new #1 remaining non-displaced target is `0x00E0F4` (772 calls).
