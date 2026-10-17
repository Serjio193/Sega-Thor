# M15.58 Execution-Driven Recompilation: 0x00E0BA Instruction-Stepped Native Replacement — ACTIVE

Planned:
- Reverse-engineer and displace runtime target `0x00E0BA` called 772 times in 3,600 canonical frames from caller `0x008B36` (return PC `0x008B3A`).
- Implement instruction-stepped native C++ handler with zero ROM fallbacks.
- Verify bit-identical 3,600-frame dual A/B replay across all 8 canonical streams.

# M15.57 Execution-Driven Recompilation: 0x00DEEC Dual Replay Verification & Displacement — DONE

Delivered:
- Recompiled runtime-discovered target `0x00DEEC` (Master Active Entity Tick & Isometric Velocity Projection Loop) called 772 times in 3,600 canonical frames from caller `0x008B32` (772 calls, return PC `0x008B36`).
- Semantics & architecture:
  * Inhibit check: `tst.b $00FF1988`. If non-zero, immediately early exits via `0x00DF50` `rts` (observed 30 times in canonical replay).
  * Iterates across 20 entity slots starting at `0x00FF1AA4` with stride 188 bytes (`0xBC`).
  * Entity state check `4(%a6)`: if `>= 22` (`0x16`), invokes indirect tick handler via `0x00DF28(%pc, %d6.w)`, saving/restoring `%d7/%a6` across the call with canonical `movem.l` cycle adjustments.
  * Applies isometric velocity projection from delta bytes 72 and 73 to entity coordinates and clears displacement bytes.
  * Advances slot via `dbf %d7, 0x00DF00` with exact Musashi-equivalent taken (-14 master cycles) and not-taken (+14 master cycles) cycle adjustments.
  * Completes loop and returns via `0x00DF50` `rts` (742 loop completions).
- Achieved 100% bit-for-bit identical state across all 3,600 frames between Run A (`0x00DEEC` OFF) and Run B (`0x00DEEC` ON):
  * `different_frame_count: 0`, `divergence=NO`.
  * ROM_EXEC_00DEEC: Run A = 772; Run B = 0 (100% displaced, 0 fallbacks, 161,708 stepped instructions: 30 inhibit exits, 742 loop completions, 4,895 child jsr calls).
  * NATIVE_EXEC_00DEEC: Run B = 772; Run A = 0.
  * Matching SHA256 hashes across all 8 canonical streams (`observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, `call-z80-ram.bin`, `external-events.jsonl`, `normalized-events.jsonl`).
  * Status: `CPP_REPLACEMENT_VERIFIED_PATH_SPECIFIC = YES`, `RUNTIME_REPLACEMENT_ACTIVE = YES`, `CPP_REPLACEMENT_VERIFIED_GLOBAL = YES`.
- Kept `0x00DEEC` active in native island chain (`step:...,00deec`) and ranked fresh call census for remaining ROM targets:
  1. `0x00E0BA`: 772 remaining ROM calls (caller: `0x008B36`: 772) -> ACTIVE TARGET #1 (Milestone M15.58)
  2. `0x00E0F4`: 772 remaining ROM calls (caller: `0x008B3A`: 772)
  3. `0x00B3B2`: 759 remaining ROM calls (caller: `0x00AD4E`: 759)
  4. `0x00CEEA`: 743 remaining ROM calls (callers: `0x02E604`: 89, `0x02E6F6`: 190, `0x02E87C`: 464)
  5. `0x00D188`: 743 remaining ROM calls (caller: `0x00D0BC`: 743)
  6. `0x00D344`: 743 remaining ROM calls (caller: `0x00D2B4`: 743)
  7. `0x00D398`: 743 remaining ROM calls (callers: `0x02E60A`: 89, `0x02E6FC`: 190, `0x02E882`: 464)

# M15.56 Execution-Driven Recompilation: 0x008BAC Dual Replay Verification & Displacement — DONE

Delivered:
- Recompiled runtime-discovered target `0x008BAC` (Player Menu, Pause & Inventory Input Coordinator) called 772 times in 3,600 canonical frames from caller `0x008B22` (772 calls, return PC `0x008B26`).
- Semantics & architecture:
  * Resets pending menu request via `clr.b $00FF185E`.
  * Mode inhibit checks: `$00FF1987`, `$00FF17B8` (bit 2), `$00FF1983`, and `$00FF1982`. If any flag is non-zero/set, early exits via `0x008C34` `rts`.
  * Evaluates controller input words `$00FF165E`, `$00FF165F`, `$00FF1660` for start / pause button triggers.
  * If menu requested, sets mode `$00FF185E` (1, 2, or 3) and executes tail jump to menu loop `0x003B3E`.
  * In canonical replay, menu is not requested, resulting in all 772 calls completing through input evaluation and returning cleanly via `0x008C34` `rts`.
- Achieved 100% bit-for-bit identical state across all 3,600 frames between Run A (`0x008BAC` OFF) and Run B (`0x008BAC` ON):
  * `different_frame_count: 0`, `divergence=NO`.
  * ROM_EXEC_008BAC: Run A = 772; Run B = 0 (100% displaced, 0 fallbacks, 17,262 stepped instructions: 772 clean RTS completions).
  * NATIVE_EXEC_008BAC: Run B = 772; Run A = 0.
  * Matching SHA256 hashes across all 8 canonical streams (`observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, `call-z80-ram.bin`, `external-events.jsonl`, `normalized-events.jsonl`).
  * Status: `CPP_REPLACEMENT_VERIFIED_PATH_SPECIFIC = YES`, `RUNTIME_REPLACEMENT_ACTIVE = YES`, `CPP_REPLACEMENT_VERIFIED_GLOBAL = YES`.
- Kept `0x008BAC` active in native island chain (`step:...,008bac`) and ranked fresh call census for remaining ROM targets:
  1. `0x00DEEC`: 772 remaining ROM calls (caller: `0x008B32`: 772) -> ACTIVE TARGET #1 (Milestone M15.57)
  2. `0x00E0BA`: 772 remaining ROM calls (caller: `0x008B36`: 772)
  3. `0x00E0F4`: 772 remaining ROM calls (caller: `0x008B3A`: 772)
  4. `0x00B3B2`: 759 remaining ROM calls (caller: `0x00AD4E`: 759)
  5. `0x00CEEA`: 743 remaining ROM calls (callers: `0x02E604`: 89, `0x02E6F6`: 190, `0x02E87C`: 464)
  6. `0x00D188`: 743 remaining ROM calls (caller: `0x00D0BC`: 743)
  7. `0x00D344`: 743 remaining ROM calls (caller: `0x00D2B4`: 743)


# M15.55 Execution-Driven Recompilation: 0x008B8E Dual Replay Verification & Displacement — DONE

Delivered:
- Recompiled runtime-discovered target `0x008B8E` (Per-Frame State Gate & Mode Transition Dispatcher) called 772 times in 3,600 canonical frames from caller `0x008B26` (772 calls, return PC `0x008B2A`).
- Semantics & architecture:
  * Reads state/mode byte `$00FF1864` into `%d0` via `move.b $00FF1864, %d0`.
  * If `$00FF1864 == 0`, branches via `beq.w 0x008BAA` directly to `rts`, popping return PC `0x008B2A` from stack (772 executions in canonical replay).
  * If `$00FF1864 != 0`, tests and clears bit 7 via `bclr #7, $00FF1864` (updating Z flag only).
  * If bit 7 was clear (Z == 1), executes `bra.w 0x00530C` tail jump to mode handler `0x00530C`.
  * If bit 7 was set (Z == 0), branches via `bne.s 0x008BA6` to execute `bra.w 0x007CC6` tail jump to mode handler `0x007CC6`.
- Achieved 100% bit-for-bit identical state across all 3,600 frames between Run A (`0x008B8E` OFF) and Run B (`0x008B8E` ON):
  * `different_frame_count: 0`, `divergence=NO`.
  * ROM_EXEC_008B8E: Run A = 772; Run B = 0 (100% displaced, 0 fallbacks, 2,316 stepped instructions: 772 zero-exit completions).
  * NATIVE_EXEC_008B8E: Run B = 772; Run A = 0.
  * Matching SHA256 hashes across all 8 canonical streams (`observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, `call-z80-ram.bin`, `external-events.jsonl`, `normalized-events.jsonl`).
  * Status: `CPP_REPLACEMENT_VERIFIED_PATH_SPECIFIC = YES`, `RUNTIME_REPLACEMENT_ACTIVE = YES`, `CPP_REPLACEMENT_VERIFIED_GLOBAL = YES`.
- Kept `0x008B8E` active in native island chain (`step:...,008b8e`) and ranked fresh call census for remaining ROM targets:
  1. `0x008BAC`: 772 remaining ROM calls (caller: `0x008B22`: 772) -> ACTIVE TARGET #1 (Milestone M15.56)
  2. `0x00DEEC`: 772 remaining ROM calls (caller: `0x008B32`: 772)
  3. `0x00E0BA`: 772 remaining ROM calls (caller: `0x008B36`: 772)
  4. `0x00E0F4`: 772 remaining ROM calls (caller: `0x008B3A`: 772)
  5. `0x00B3B2`: 759 remaining ROM calls (caller: `0x00AD4E`: 759)
  6. `0x00CEEA`: 743 remaining ROM calls (callers: `0x02E604`: 89, `0x02E6F6`: 190, `0x02E87C`: 464)
  7. `0x00D188`: 743 remaining ROM calls (caller: `0x00D0BC`: 743)

# M15.54 Execution-Driven Recompilation: 0x005670 Dual Replay Verification & Displacement — DONE

Delivered:
- Recompiled runtime-discovered target `0x005670` (Player Pre-Action Input, Timers & Action Dispatcher) called 772 times in 3,600 canonical frames from caller `0x005586` (772 calls, return PC `0x00558A`).
- Semantics & architecture:
  * Block 1 (`0x005670..0x0057D0`): saves `%sp` to `$FF1934`, evaluates input/action readiness flags (`$FF1985`, `$FF1987`, `$FF0DBA`, `$FF1971`, `$FF1982`), updates motion/attack interval timers (`114(a6)`, `74(a6)`, `142(a6)`), and calls child subroutine `jsr 0x00E682` at `0x0057D0` (772 calls). Child returns normally to `0x0057D6` on 753 calls, or pops return address to grandparent `0x00558A` on 19 calls.
  * Block 2 (`0x0057D6..0x0059B8`): evaluates sub-action state bytes (`$FF198B`, `108(a6)`, `132(a6)`, `131(a6)`, `56(a6)`), invokes `bsr.w 0x008EF8` at `0x0058FA` and `0x005928` (2 child calls), and dispatches via jump table at `0x0059B8` (`jmp (%a0)`, 753 tail-dispatch completions).
  * Block 3 (`0x005B3C..0x005B9E`): auxiliary branch with `movem.l d1, -(sp)` at `0x005B6C` and `movem.l (sp)+, d1` at `0x005B82` adding canonical 56-cycle bus refresh timing.
- Achieved 100% bit-for-bit identical state across all 3,600 frames between Run A (`0x005670` OFF) and Run B (`0x005670` ON):
  * `different_frame_count: 0`, `divergence=NO`.
  * ROM_EXEC_005670: Run A = 772; Run B = 0 (100% displaced, 0 fallbacks, 47,655 stepped instructions: 772 child calls to `0x00E682`, 2 child calls to `0x008EF8`, 753 dispatch completions, 19 grandparent return completions).
  * NATIVE_EXEC_005670: Run B = 772; Run A = 0.
  * Matching SHA256 hashes across all 8 canonical streams (`observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, `call-z80-ram.bin`, `external-events.jsonl`, `normalized-events.jsonl`).
  * Status: `CPP_REPLACEMENT_VERIFIED_PATH_SPECIFIC = YES`, `RUNTIME_REPLACEMENT_ACTIVE = YES`, `CPP_REPLACEMENT_VERIFIED_GLOBAL = YES`.
- Kept `0x005670` active in native island chain (`step:...,005670`) and ranked fresh call census for remaining ROM targets:
  1. `0x008B8E`: 772 remaining ROM calls (caller: `0x008B26`: 772) -> ACTIVE TARGET #1
  2. `0x008BAC`: 772 remaining ROM calls (caller: `0x008B22`: 772)
  3. `0x00DEEC`: 772 remaining ROM calls (caller: `0x008B32`: 772)
  4. `0x00E0BA`: 772 remaining ROM calls (caller: `0x008B36`: 772)
  5. `0x00E0F4`: 772 remaining ROM calls (caller: `0x008B3A`: 772)
  6. `0x00B3B2`: 759 remaining ROM calls (caller: `0x00AD4E`: 759)
  7. `0x00CEEA`: 743 remaining ROM calls (callers: `0x02E604`: 89, `0x02E6F6`: 190, `0x02E87C`: 464)

# M15.53 Execution-Driven Recompilation: 0x00557A Dual Replay Verification & Displacement — DONE

Delivered:
- Recompiled runtime-discovered target `0x00557A` (Player Per-Frame State, Footstep Audio & Shadow Coordinator) called 772 times in 3,600 canonical frames from caller `0x008B2E` (772 calls, return PC `0x008B32`).
- Semantics & architecture:
  * Clears `$FF1986` via `sf $00FF1986`.
  * Loads player entity slot pointer `A6 = $FF19E8` and invokes child subroutine `bsr.w 0x005670` (return PC `0x00558A`).
  * Reloads player entity slot `A6 = $FF19E8`, unpacks displacement bytes `72(a6)` and `73(a6)`, checks facing flip `23(a6)` bit 0, and writes velocity words to `66(a6)` and `68(a6)`.
  * Manages footstep audio timer `$FF1993` / motion word `138(a6)` to queue sound effects via `jsr 0x060004` (`d0 = 0x00A0038C`).
  * Manages secondary sound timer `$FF1994` / state `$FF1AA4` to queue sound effects via `jsr 0x060004` (`d0 = 0x00A004DD`).
  * Shadow coordinator: evaluates bit 7 of state byte `111(a6)`. If set, exits via `0x00565E` RTS without updating shadow (439 calls). If clear, checks player action state `4(a6)`. If action is 0, 2, or 4, updates shadow coordinates at `$FF1972` from `8(a6)` and `12(a6)`, exiting via `0x00566E` RTS (333 calls).
- Achieved 100% bit-for-bit identical state across all 3,600 frames between Run A (`0x00557A` OFF) and Run B (`0x00557A` ON):
  * `different_frame_count: 0`, `divergence=NO`.
  * ROM_EXEC_00557A: Run A = 772; Run B = 0 (100% displaced, 0 fallbacks, 24,968 stepped instructions: 772 child calls to `0x005670`, 439 shadow skips, 333 shadow updates).
  * NATIVE_EXEC_00557A: Run B = 772; Run A = 0.
  * Matching SHA256 hashes across all 8 canonical streams (`observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, `call-z80-ram.bin`, `external-events.jsonl`, `normalized-events.jsonl`).
  * Status: `CPP_REPLACEMENT_VERIFIED_PATH_SPECIFIC = YES`, `RUNTIME_REPLACEMENT_ACTIVE = YES`, `CPP_REPLACEMENT_VERIFIED_GLOBAL = YES`.
- Kept `0x00557A` active in native island chain (`step:...,00557a`) and ranked fresh call census for remaining ROM targets:
  1. `0x005670`: 772 remaining ROM calls (caller: `0x005586`: 772) -> ACTIVE TARGET #1
  2. `0x008B8E`: 772 remaining ROM calls (caller: `0x008B26`: 772)
  3. `0x008BAC`: 772 remaining ROM calls (caller: `0x008B22`: 772)
  4. `0x00DEEC`: 772 remaining ROM calls (caller: `0x008B32`: 772)
  5. `0x00E0BA`: 772 remaining ROM calls (caller: `0x008B36`: 772)
  6. `0x00E0F4`: 772 remaining ROM calls (caller: `0x008B3A`: 772)
  7. `0x00B3B2`: 759 remaining ROM calls (caller: `0x00AD4E`: 759)
  8. `0x00CEEA`: 743 remaining ROM calls (callers: `0x02E604`: 89, `0x02E6F6`: 190, `0x02E87C`: 464)
  9. `0x00D188`: 743 remaining ROM calls (caller: `0x00D0BC`: 743)
  10. `0x00D344`: 743 remaining ROM calls (caller: `0x00D2B4`: 743)
  11. `0x00D398`: 743 remaining ROM calls (callers: `0x02E60A`: 89, `0x02E6FC`: 190, `0x02E882`: 464)
  12. `0x00C9B2`: 742 remaining ROM calls (callers: `0x02E5B0`: 89, `0x02E64E`: 190, `0x02E774`: 463)
  13. `0x061EF4`: 731 remaining ROM calls (caller: `0x061EE4`: 731)
  14. `0x02E704`: 653 remaining ROM calls (callers: `0x02E64A`: 190, `0x02E770`: 463)
  15. `0x00F91C`: 630 remaining ROM calls (caller: `0x00F7C6`: 630)

# M15.52 Execution-Driven Recompilation: 0x008DD4 Dual Replay Verification & Displacement — DONE

Delivered:
- Recompiled runtime-discovered target `0x008DD4` (HUD / Screen Spirit / Special Counter Step Handler) called 772 times in 3,600 canonical frames from caller `0x008B46` (772 calls, return PC `0x008B4A`).
- Semantics & architecture:
  * Reads byte counter `$FF183E` into `%d0` via `move.b $00FF183E, %d0` and evaluates word flag `$FF1AA4` via `tst.w $00FF1AA4`.
  * Inactive state (`$FF1AA4 == 0`): branches via `beq.s 0x008E08`. If `%d0 == 0`, branches immediately via `beq.s 0x008E20` directly to `rts` without writing memory (772 calls in canonical replay). If `%d0 != 0`, decrements `%d0` (or clamps `%d0 = 2` if `%d0 >= 4`), writes back to `$FF183E`, and returns via `rts`.
  * Active state (`$FF1AA4 != 0`): increments `%d0` (up to 3) or clamps `%d0 = 3` and inspects frame counter `$FF199E` and bit 0 of `$FF199F` to optionally advance `%d0` to 4, writes back to `$FF183E`, and returns via `rts`.
- Achieved 100% bit-for-bit identical state across all 3,600 frames between Run A (`0x008DD4` OFF) and Run B (`0x008DD4` ON):
  * `different_frame_count: 0`, `divergence=NO`.
  * ROM_EXEC_008DD4: Run A = 772; Run B = 0 (100% displaced, 0 fallbacks, 4,632 stepped instructions).
  * NATIVE_EXEC_008DD4: Run B = 772; Run A = 0.
  * Matching SHA256 hashes across all 8 canonical streams (`observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, `call-z80-ram.bin`, `external-events.jsonl`, `normalized-events.jsonl`).
  * Status: `CPP_REPLACEMENT_VERIFIED_PATH_SPECIFIC = YES`, `RUNTIME_REPLACEMENT_ACTIVE = YES`, `CPP_REPLACEMENT_VERIFIED_GLOBAL = YES`.
- Kept `0x008DD4` active in native island chain (`step:...,008dd4`) and ranked fresh call census for remaining ROM targets:
  1. `0x00557A`: 772 remaining ROM calls (caller: `0x008B2E`: 772) -> SOLE NEXT TARGET #1
  2. `0x005670`: 772 remaining ROM calls (caller: `0x005586`: 772)
  3. `0x008B8E`: 772 remaining ROM calls (caller: `0x008B26`: 772)
  4. `0x008BAC`: 772 remaining ROM calls (caller: `0x008B22`: 772)
  5. `0x00DEEC`: 772 remaining ROM calls (caller: `0x008B32`: 772)
  6. `0x00E0BA`: 772 remaining ROM calls (caller: `0x008B36`: 772)
  7. `0x00E0F4`: 772 remaining ROM calls (caller: `0x008B3A`: 772)
  8. `0x00B3B2`: 759 remaining ROM calls (caller: `0x00AD4E`: 759)

# M15.51 Execution-Driven Recompilation: 0x00C998 Dual Replay Verification & Displacement — DONE

Delivered:
- Recompiled runtime-discovered target `0x00C998` (Per-Frame Map Script Vector 1 Dispatcher) called 772 times in 3,600 canonical frames across caller `0x008B52` (772 calls, return PC `0x008B56`).
- Semantics & architecture:
  * Script inhibit evaluation (`0x00C998`): tests bit 1 of byte `$FF17B8` via `btst #1, $00FF17B8`. If bit 1 is set (non-zero), branches via `bne.w 0x00C9B0` directly to `rts`, popping return address from `(sp)` and returning to caller (30 executions in 3,600 frames).
  * Vector table pointer load (`0x00C9A4`): loads map script vector table pointer from `$FF16EC` into `%a1` via `movea.l $00FF16EC, %a1`.
  * Vector 1 entry indexing (`0x00C9AA`): advances `%a1` by 2 bytes via `addq.l #2, %a1` to select table entry 1.
  * Vector 1 displacement resolution (`0x00C9AC`): reads 16-bit signed displacement at `(%a1)` and adds to `%a1` via `adda.w (%a1), %a1`.
  * Indirect tail dispatch (`0x00C9AE`): dispatches directly to Vector 1 handler via `jmp (%a1)` (742 executions in 3,600 frames). Musashi takes over native execution at target; target handler returns straight to outer caller `0x008B56`.
- Achieved 100% bit-for-bit identical state across all 3,600 frames between Run A (`0x00C998` OFF) and Run B (`0x00C998` ON):
  * `different_frame_count: 0`, `divergence=NO`.
  * ROM_EXEC_00C998: Run A = 772; Run B = 0 (100% displaced, 0 fallbacks, 4,542 stepped instructions).
  * NATIVE_EXEC_00C998: Run B = 772; Run A = 0.
  * Matching SHA256 hashes across all 8 canonical streams (`observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, `call-z80-ram.bin`, `external-events.jsonl`, `normalized-events.jsonl`).
  * Status: `CPP_REPLACEMENT_VERIFIED_PATH_SPECIFIC = YES`, `RUNTIME_REPLACEMENT_ACTIVE = YES`, `CPP_REPLACEMENT_VERIFIED_GLOBAL = YES`.
- Kept `0x00C998` active in native island chain (`step:...,00c998`) and ranked fresh call census for remaining ROM targets:
  1. `0x008DD4`: 772 remaining ROM calls (caller: `0x008B46`: 772) -> SOLE NEXT TARGET #1
  2. `0x00557A`: 772 remaining ROM calls (caller: `0x008B2E`: 772)
  3. `0x005670`: 772 remaining ROM calls (caller: `0x005586`: 772)
  4. `0x008B8E`: 772 remaining ROM calls (caller: `0x008B26`: 772)
  5. `0x008BAC`: 772 remaining ROM calls (caller: `0x008B22`: 772)
  6. `0x00DEEC`: 772 remaining ROM calls (caller: `0x008B32`: 772)
  7. `0x00E0BA`: 772 remaining ROM calls (caller: `0x008B36`: 772)
  8. `0x00E0F4`: 772 remaining ROM calls (caller: `0x008B3A`: 772)

# M15.50 Execution-Driven Recompilation: 0x00C980 Dual Replay Verification & Displacement — DONE

Delivered:
- Recompiled runtime-discovered target `0x00C980` (Per-Frame Map Script Vector 0 Dispatcher) called 773 times in 3,600 canonical frames across callers `0x008B60` (770 calls, return PC `0x008B64`) and `0x00CD20` (3 calls, return PC `0x00CD24`).
- Semantics & architecture:
  * Script inhibit evaluation (`0x00C980`): tests bit 1 of byte `$FF17B8` via `btst #1, $00FF17B8`. If bit 1 is set (non-zero), branches via `bne.w 0x00C996` directly to `rts`, popping the return address from `(sp)` and returning to caller (30 executions in 3,600 frames).
  * Vector 0 address resolution (`0x00C98C`): loads map script vector table pointer from `$FF16EC` into `%a1` via `movea.l $00FF16EC, %a1`.
  * Vector 0 displacement indexing (`0x00C992`): reads 16-bit displacement at `(%a1)` and adds it to `%a1` via `adda.w (%a1), %a1`.
  * Indirect tail dispatch (`0x00C994`): dispatches directly to Vector 0 handler via `jmp (%a1)` (743 executions in 3,600 frames). Musashi takes over native execution at target; target handler returns straight to outer caller `0x008B64` or `0x00CD24`.
- Achieved 100% bit-for-bit identical state across all 3,600 frames between Run A (`0x00C980` OFF) and Run B (`0x00C980` ON):
  * `different_frame_count: 0`, `divergence=NO`.
  * ROM_EXEC_00C980: Run A = 773; Run B = 0 (100% displaced, 0 fallbacks, 3,805 stepped instructions).
  * NATIVE_EXEC_00C980: Run B = 773; Run A = 0.
  * Matching SHA256 hashes across all 8 canonical streams (`observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, `call-z80-ram.bin`, `external-events.jsonl`, `normalized-events.jsonl`).
  * Status: `CPP_REPLACEMENT_VERIFIED_PATH_SPECIFIC = YES`, `RUNTIME_REPLACEMENT_ACTIVE = YES`, `CPP_REPLACEMENT_VERIFIED_GLOBAL = YES`.
- Kept `0x00C980` active in native island chain (`step:...,00c980`) and ranked fresh call census for remaining ROM targets:
  1. `0x00C998`: 772 remaining ROM calls (caller: `0x008B52`: 772) -> SOLE NEXT TARGET #1
  2. `0x00557A`: 772 remaining ROM calls (caller: `0x008B2E`: 772)
  3. `0x005670`: 772 remaining ROM calls (caller: `0x005586`: 772)
  4. `0x008B8E`: 772 remaining ROM calls (caller: `0x008B26`: 772)
  5. `0x008BAC`: 772 remaining ROM calls (caller: `0x008B22`: 772)
  6. `0x008DD4`: 772 remaining ROM calls (caller: `0x008B46`: 772)
  7. `0x00DEEC`: 772 remaining ROM calls (caller: `0x008B32`: 772)
  8. `0x00E0BA`: 772 remaining ROM calls (caller: `0x008B36`: 772)
  9. `0x00E0F4`: 772 remaining ROM calls (caller: `0x008B3A`: 772)

# M15.49 Execution-Driven Recompilation: 0x00B34C Dual Replay Verification & Displacement — DONE

Delivered:
- Recompiled runtime-discovered target `0x00B34C` (Entity Display Mode & Visual Effects Dispatcher) called 773 times in 3,600 canonical frames across caller `0x00AD42` (return PC `0x00AD46`).
- Semantics & architecture:
  * Global pause/freeze evaluation (`0x00B34C`): tests byte `$FF1927` via `tst.b $FF1927`. If non-zero, immediately exits via `0x00B382` RTS.
  * Entity visual flags evaluation (`0x00B356`): loads display mode byte `57(%fp)` (`0x39(A6)`) into `D5`.
  * Primary child dispatch (`0x00B35A..0x00B35E`): tests bit 0 of `D5` via `btst #0, %d5`. If set, calls child `0x00B3D2` via `bsr.w 0x00B3D2` at `0x00B384` (481 calls in 3,600 frames). Upon return at `0x00B388`, branches to `0x00B362`.
  * Secondary child dispatch (`0x00B362..0x00B366`): tests bit 3 of `D5`. If set, calls child `0x00B45C` via `bsr.w 0x00B45C` at `0x00B394` (0 calls in canonical replay).
  * Alternative child dispatch (`0x00B36A..0x00B36E`): tests bit 4 of `D5`. If set, calls child `0x00B4FE` via `bsr.w 0x00B4FE` at `0x00B39C` (0 calls in canonical replay).
  * Priority overlay dispatch & flag clear (`0x00B372..0x00B376`): tests bit 1 of `D5`. If set, calls child `0x00B610` via `bsr.w 0x00B610` at `0x00B3A4` (0 calls in canonical replay), clears bit 2 in memory via `bclr #2, 57(%fp)` at `0x00B3A8`, and branches directly to `0x00B382` RTS.
  * Background child dispatch (`0x00B37A..0x00B37E`): tests bit 2 of `D5`. If set, calls child `0x00B56E` via `bsr.w 0x00B56E` at `0x00B38C` (292 calls in 3,600 frames). Upon return at `0x00B390`, branches to `0x00B382` RTS.
  * Epilogue: exits via `0x00B382` RTS back to outer caller `0x00AD46` (773 completed calls).
- Achieved 100% bit-for-bit identical state across all 3,600 frames between Run A (`0x00B34C` OFF) and Run B (`0x00B34C` ON):
  * `different_frame_count: 0`, `divergence=NO`.
  * ROM_EXEC_00B34C: Run A = 773; Run B = 0 (100% displaced, 0 fallbacks, 12,368 stepped instructions).
  * NATIVE_EXEC_00B34C: Run B = 773; Run A = 0.
  * Matching SHA256 hashes across all 8 canonical streams (`observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, `call-z80-ram.bin`, `external-events.jsonl`, `normalized-events.jsonl`).
  * Status: `CPP_REPLACEMENT_VERIFIED_PATH_SPECIFIC = YES`, `RUNTIME_REPLACEMENT_ACTIVE = YES`, `CPP_REPLACEMENT_VERIFIED_GLOBAL = YES`.
- Kept `0x00B34C` active in native island chain (`step:...,00b34c`) and ranked fresh call census for remaining ROM targets:
  1. `0x00C980`: 773 remaining ROM calls (callers: `0x008B60`: 770, `0x00CD20`: 3) -> SOLE NEXT TARGET #1
  2. `0x00557A`: 772 remaining ROM calls (caller: `0x008B2E`: 772)
  3. `0x005670`: 772 remaining ROM calls (caller: `0x005586`: 772)
  4. `0x008B8E`: 772 remaining ROM calls (caller: `0x008B26`: 772)
  5. `0x008BAC`: 772 remaining ROM calls (caller: `0x008B22`: 772)

# M15.48 Execution-Driven Recompilation: 0x00A342 Dual Replay Verification & Displacement — DONE

Delivered:
- Recompiled runtime-discovered target `0x00A342` (Base Sprite Builder & VDP DMA Queue Stager) called 773 times in 3,600 canonical frames across caller `0x00A19C` (return PC `0x00A1A0`).
- Semantics & architecture:
  * Destination setup: `lea $FF13CC, a5`, adds displacement `$FF188C`, reads base link count `$FF188A` into `D5`.
  * Sprite template selection: checks `$FF1858`; selects template table `0x00A438` or alternate table `0x00A480`.
  * Loop 1 (`0x00A36C..0x00A37C`): iterates 6 times (`D0 = 5`), copying 8-byte sprite attribute records, setting link index `D5`, and adjusting X position with screen offset `$FF185A` (3,865 iterations total).
  * Status/cursor sprite generation (`0x00A3AA`): checks `$FF1856`; writes special 8-byte sprite entry (Y=280, link=`D5`, tile=`0x85C7`, X=`$FF185A + 104`).
  * VDP DMA staging queue (`0x00A3DA`): checks if byte `$FF1854` differs from `$FF1855`. If changed, updates `$FF1855`, computes ROM source address `D0 = 0x000AAebb + D0 * 144`, disables interrupts (`ori.w #0x0700, sr`), stages 8-byte DMA entry (ROM source `D0`, VRAM dest `0xB8E0`, length `0x0090`) into DMA queue buffer at pointer `$FF1892`, updates pointer `$FF1892`, and re-enables interrupts (`andi.w #0xF9FF, sr`) (1 DMA entry queued).
  * Epilogue & SAT metrics update (`0x00A424`): calculates updated buffer displacement `suba.l #$FF13CC, a5`, writes back word to `$FF188C`, writes back link counter `D5` to `$FF188A`, and executes RTS to caller (`0x00A1A0`).
- Achieved 100% bit-for-bit identical state across all 3,600 frames between Run A (`0x00A342` OFF) and Run B (`0x00A342` ON):
  * `different_frame_count: 0`, `divergence=NO`.
  * ROM_EXEC_00A342: Run A = 773; Run B = 0 (100% displaced, 0 fallbacks, 55,011 stepped instructions).
  * NATIVE_EXEC_00A342: Run B = 773; Run A = 0.
  * Matching SHA256 hashes across all 8 canonical streams (`observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, `call-z80-ram.bin`, `external-events.jsonl`, `normalized-events.jsonl`).
  * Status: `CPP_REPLACEMENT_VERIFIED_PATH_SPECIFIC = YES`, `RUNTIME_REPLACEMENT_ACTIVE = YES`, `CPP_REPLACEMENT_VERIFIED_GLOBAL = YES`.
- Kept `0x00A342` active in native island chain (`step:...,00a342`) and ranked fresh call census for remaining ROM targets:
  1. `0x00B34C`: 773 remaining ROM calls (caller: `0x00AD42`: 773) -> SOLE NEXT TARGET #1
  2. `0x00C980`: 773 remaining ROM calls (callers: `0x008B60`: 770, `0x00CD20`: 3)
  3. `0x00557A`: 772 remaining ROM calls (caller: `0x008B2E`: 772)
  4. `0x005670`: 772 remaining ROM calls (caller: `0x005586`: 772)
  5. `0x008B8E`: 772 remaining ROM calls (caller: `0x008B26`: 772)

# M15.47 Execution-Driven Recompilation: 0x00A196 Dual Replay Verification & Displacement — DONE

Delivered:
- Recompiled runtime-discovered target `0x00A196` (Sprite Table Committer & Multi-Pool Display Dispatcher) called 773 times in 3,600 canonical frames across 2 callers (`0x008B86`: 770 in the main game loop, `0x00CD28`: 3 in secondary loop).
- Semantics & architecture:
  * Initial setup & base sprite builder (`0x00A196`): clears `$FF1651` via `sf $FF1651`, calls base sprite builder child `0x00A342` via `bsr.w 0x00A342` (773 calls).
  * HUD sprite generator trigger (`0x00A1A0`): evaluates `$FF1996` via `tst.b $FF1996`; if non-zero, calls HUD sprite generator `0x00A7E6` via `bsr.w 0x00A7E6` at `0x00A336` (773 calls).
  * Multi-pool display scans: scans pool `$FF2954` (4 slots * 90 bytes), pool `$FF19C8` (4 slots * 8 bytes), pool `$FF19E8` (21 slots * 188 bytes), pool `$FF2ABC` (8 slots * 90 bytes), and pool `$FF2D8C` (6 slots * 90 bytes).
  * Active entity candidate arbitration: bitmask tracking in `D7` using `bset` and `btst` across slots, selecting active entity slot with highest priority.
  * Child routine dispatch: pushes return address (`$00A1F8`, `$00A1BA`, `$00A270`, `$00A258`, `$00A1D0`) onto stack and dispatches child subroutines via `beq.w 0x00AD76` (6,246 calls), `bra.w 0x00A9A0` (2,691 calls), or indirect jump via `jmp (a0)` (90 calls).
  * SAT finalization & DMA staging (`0x00A2A6`): sets terminal entry marker `-5(a6, d7.w) = 0`, sets bits 0 and 1 of `$FF164E` (`orib #3, $FF164E`) to trigger VDP SAT DMA, and exits via `0x00A2C6` RTS (773 completed calls).
  * Architectural cycle resolution: identified Motorola 68000 / Musashi `BSET Dn, Dm` timing property (`(1 << (bit & 31)) & 0xFFFF0000` adds `2 * MUL = 14` master cycles when `bit >= 16`). Incorporating this rule into `step_bset_reg_reg` resolved the 70-cycle delta across all 5 upper-word bit iterations (bits 20, 19, 18, 17, 16) with zero stream divergence.
- Achieved 100% bit-for-bit identical state across all 3,600 frames between Run A (`0x00A196` OFF) and Run B (`0x00A196` ON):
  * `different_frame_count: 0`, `divergence=NO`.
  * ROM_EXEC_00A196: Run A = 773; Run B = 0 (100% displaced, 0 fallbacks, 882,033 stepped instructions).
  * NATIVE_EXEC_00A196: Run B = 773; Run A = 0.
  * Matching SHA256 hashes across all 8 canonical streams (`observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, `call-z80-ram.bin`, `external-events.jsonl`, `normalized-events.jsonl`).
  * Status: `CPP_REPLACEMENT_VERIFIED_PATH_SPECIFIC = YES`, `RUNTIME_REPLACEMENT_ACTIVE = YES`, `CPP_REPLACEMENT_VERIFIED_GLOBAL = YES`.
- Kept `0x00A196` active in native island chain (`step:...,00a196`) and ranked fresh call census for remaining ROM targets:
  1. `0x00A342`: 773 remaining ROM calls (caller: `0x00A19C`: 773) -> SOLE NEXT TARGET #1
  2. `0x00B34C`: 773 remaining ROM calls (caller: `0x00AD42`: 773)
  3. `0x00C980`: 773 remaining ROM calls (callers: `0x008B60`: 770, `0x00CD20`: 3)
  4. `0x00557A`: 772 remaining ROM calls (caller: `0x008B2E`: 772)
  5. `0x005670`: 772 remaining ROM calls (caller: `0x005586`: 772)

# M15.46 Execution-Driven Recompilation: 0x00F788 Dual Replay Verification & Displacement — DONE

Delivered:
- Recompiled runtime-discovered target `0x00F788` (Event Script & Cutscene / Camera Scroll Sequencer) called 775 times in 3,600 canonical frames across 2 callers (`0x008B2A`: 772 in the main game loop, `0x00CD02`: 3 in secondary loop).
- Semantics & architecture:
  * Inhibit flag evaluation (`0x00F788`): tests `bit 1` of `$FF17B8` via `btst #1, $FF17B8`. If bit 1 is set, immediately exits via `0x00F794` RTS (30 calls in 3,600 frames).
  * Wait timer countdown (`0x00F796`): loads byte from `$FF17B6`. If non-zero, decrements and writes back, exiting via `0x00F7A8` RTS (15 calls in 3,600 frames).
  * Camera pan tick countdown (`0x00F7AA`): loads byte from `$FF17BD`. If non-zero, dispatches pan tick via jump table `0x00F8C8` for directions 0..3 (`0x00F8D0`, `0x00F8E4`, `0x00F8F6`, `0x00F908`), decrements countdown, and clears pan status at expiration via `0x00F8C6` RTS.
  * Script sequencer loop (`0x00F7B4`): loads script pointer from `$FF17B2`. In loop (`0x00F7BA..0x00F7CE`), fetches command byte from `(A0)+`, doubles index, looks up jump table at `0x00F7D0`, and calls child command via `jsr (A1)` at `0x00F7C6` (760 child script command dispatches across 730 script calls).
  * Script command early exits: Commands 00, 06, 07, 08, 09, 16, 18, 19, and 1E pop the child return address (`addq.l #4, sp`) and execute `rts` straight to the outer caller (`0x008B2E` / `0x00CD06`), totaling 632 command 00 / early exits.
- Achieved 100% bit-for-bit identical state across all 3,600 frames between Run A (`0x00F788` OFF) and Run B (`0x00F788` ON):
  * `different_frame_count: 0`, `divergence=NO`.
  * ROM_EXEC_00F788: Run A = 775; Run B = 0 (100% displaced, 0 fallbacks, 11,189 stepped instructions: 30 inhibit exits, 15 timer exits, 730 script entries, 760 child commands, 632 early exits).
  * NATIVE_EXEC_00F788: Run B = 775; Run A = 0.
  * Matching SHA256 hashes across all 8 canonical streams (`observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, `call-z80-ram.bin`, `external-events.jsonl`, `normalized-events.jsonl`).
  * Status: `CPP_REPLACEMENT_VERIFIED_PATH_SPECIFIC = YES`, `RUNTIME_REPLACEMENT_ACTIVE = YES`, `CPP_REPLACEMENT_VERIFIED_GLOBAL = YES`.
- Kept `0x00F788` active in native island chain (`step:613b2,61e48,61934,6147e,623ac,6273a,62788,60000,60286,62ae0,626bc,0027fc,002992,6138e,00e682,0613f8,016ebc,009ad6,009bf2,0027a6,00b15e,00a8da,017d12,6281a,6282e,01ac90,01b102,00350e,008e22,00a7e6,008e90,00f788`) and ranked fresh call census for remaining ROM targets:
  1. `0x00A196`: 773 remaining ROM calls (callers: `0x008B86`: 770, `0x00CD28`: 3) -> SOLE NEXT TARGET #1
  2. `0x00A342`: 773 remaining ROM calls (caller: `0x00A19C`: 773)
  3. `0x00B34C`: 773 remaining ROM calls (caller: `0x00AD42`: 773)
  4. `0x00C980`: 773 remaining ROM calls (callers: `0x008B60`: 770, `0x00CD20`: 3)
  5. `0x00557A`: 772 remaining ROM calls (caller: `0x008B2E`: 772)

# M15.39 Execution-Driven Recompilation: 0x06282E Dual Replay Verification & Displacement — DONE

Delivered:
- Recompiled runtime-discovered target `0x06282E` (Sound Channel Attenuation Calculator & Hardware Committer Sub-Tail Entry) called 922 times in 3,600 canonical frames from caller `0x0624C8` (sound driver note-on / volume calculation handling).
- Proved sub-tail entry relationship and avoided double ownership:
  * `0x06282E..0x06286C` (31 words, 62 bytes, 16 instructions) forms the shared attenuation calculation and PSG hardware commit sub-tail physically located inside `0x062788..0x06286E`.
  * Seven ROM references: 1 fall-through from `0x06281A` (`0x06282A`), 5 internal branches from `0x062788` (`0x0627AC`, `0x0627B6`, `0x0627E2`, `0x0627EC`, `0x06280E`), and 1 external call from `0x0623AC` (`0x0624C8: bsr.w 0x06282E`).
  * Reused shared tail `src/tools/hybrid/m68k_6281a_step_tail.hpp/.cpp`, which already implemented `0x06282E..0x062866` in `dispatch_6281a_tail`.
  * Refactored `0x0623AC` (`0x0624C8`) from `step_bsr_w_internal` to `step_bsr_w(0x06282EU, 0x0624CCU, ...)`, yielding to the standalone child stepper and eliminating duplicated instruction translation.
  * Cleaned up dead sub-blocks from `m68k_623ac_subs.cpp`.
- Semantics & hardware delegation:
  * Clears `D0.W` (`moveq #0, d0`), loads envelope level `$18(a6)`.
  * Clears `D2.W` (`moveq #0, d2`), loads master volume `$12(a6)`, increments by 1 (scaling factor 1..16).
  * Scales envelope by master volume via `mulu.w d2, d0; lsr.w #8, d0` to extract 4-bit attenuation level.
  * Evaluates tremolo modulation flags `$00(a6)` bit 2 and `$70(a6)` bit 5, adds delta `$6F(a6)` and shifts right by 1 if active.
  * Inverts 4-bit attenuation level (`eori.b #15, d0`), stores to hardware mirror `$11(a6)`.
  * Dispatches to PSG hardware attenuation committer `0x06273A` via `bsr.w 0x06273A`.
  * Caches envelope into `$13(a6)` and returns via `rts` (`0x06286C`).
- Achieved 100% bit-for-bit identical state across all 3,600 frames between Run A (`0x06282E` OFF) and Run B (`0x06282E` ON):
  * `different_frame_count: 0`.
  * ROM_EXEC_06282E: Run A = 922; Run B = 0 (100% displaced, 0 fallbacks, 11,986 stepped instructions, 922 child calls to `0x06273A`).
  * NATIVE_EXEC_06282E: Run B = 922; Run A = 0.
  * Matching SHA256 hashes across all 6 canonical acceptance streams (`observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, `call-z80-ram.bin`) plus `external-events.jsonl` and `normalized-events.jsonl`.
  * Status: `CPP_REPLACEMENT_VERIFIED_PATH_SPECIFIC = YES`, `RUNTIME_REPLACEMENT_ACTIVE = YES`, `CPP_REPLACEMENT_VERIFIED_GLOBAL = NO`.
- Kept `0x06282E` active in native island chain (`step:e268,613b2,61e48,61934,6147e,623ac,6273a,62788,60000,60286,62ae0,626bc,0027fc,002992,6138e,00e682,0613f8,016ebc,009ad6,009bf2,0027a6,00b15e,00a8da,017d12,6281a,6282e`) and ranked fresh call census for remaining ROM targets:
  1. `0x01AC90`: 914 remaining ROM calls (callers: `0x00DF1A`: 908, `0x00FD7A`: 2, `0x00FDFE`: 4) -> SOLE NEXT TARGET #1
  2. `0x01B102`: 890 remaining ROM calls (callers: `0x01B7F8`: 102, `0x01BBFE`: 332, `0x01BF8A`: 344, `0x01C4E2`: 112)
  3. `0x00350E`: 866 remaining ROM calls (caller: `0x0033FE`: 866)
  4. `0x008E22`: 842 remaining ROM calls (callers: `0x0007B8`: 70, `0x008B3E`: 772)
  5. `0x00A7E6`: 811 remaining ROM calls (callers: `0x00A336`: 773, `0x00A694`: 38)

# M15.38 Execution-Driven Recompilation: 0x06281A Dual Replay Verification & Displacement — DONE

Delivered:
- Recompiled runtime-discovered target `0x06281A` (Sound Channel Note-Off / Release Phase Attenuation Committer & Mid-Tail Entry) called 985 times in 3,600 canonical frames from caller `0x06243C` (sound driver note-off handling).
- Proved mid-tail entry relationship and resolved conflicting instruction ownership:
  * `0x06281A..0x06286C` (42 words, 84 bytes, 21 instructions) forms the shared release phase & attenuation tail physically located inside `0x062788..0x06286E`.
  * Two ROM entries: internal branch from `0x062788` (`0x0627F4: beq.w 0x06281A`), and external call from `0x0623AC` (`0x06243C: bsr.w 0x06281A`).
  * Extracted shared tail into `src/tools/hybrid/m68k_6281a_step_tail.hpp/.cpp`, called by `step_m68k_62788` and `step_m68k_6281a`.
  * Refactored `0x0623AC` (`0x06243C`) from `step_bsr_w_internal` to `step_bsr_w(0x06281AU, 0x062440U, ...)`, yielding to the standalone child stepper and eliminating duplicated instruction translation.
- Semantics & hardware delegation:
  * Decrements current envelope `$18(a6)` by release rate `$89(a6)`, clamps underflow borrow to 0.
  * Scales by master volume `$12(a6) + 1` (factor 1..16), shifts 8 bits to compute 4-bit attenuation level.
  * Evaluates tremolo modulation flags `$00(a6)` bit 2 and `$70(a6)` bit 5, adds delta `$6F(a6)` if active.
  * Inverts 4-bit attenuation level (`eori.b #15, d0`), stores to `$11(a6)`.
  * Dispatches to PSG hardware attenuation committer `0x06273A` via `bsr.w 0x06273A`.
  * Caches envelope into `$13(a6)` and returns via `rts` (`0x06286C`).
- Achieved 100% bit-for-bit identical state across all 3,600 frames between Run A (`0x06281A` OFF) and Run B (`0x06281A` ON):
  * `different_frame_count: 0`.
  * ROM_EXEC_06281A: Run A = 985; Run B = 0 (100% displaced, 0 fallbacks, 19,773 stepped instructions, 985 child calls to `0x06273A`).
  * NATIVE_EXEC_06281A: Run B = 985; Run A = 0.
  * Matching SHA256 hashes across all 6 canonical acceptance streams (`observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, `call-z80-ram.bin`) plus `external-events.jsonl` and `normalized-events.jsonl`.
  * Status: `CPP_REPLACEMENT_VERIFIED_PATH_SPECIFIC = YES`, `RUNTIME_REPLACEMENT_ACTIVE = YES`, `CPP_REPLACEMENT_VERIFIED_GLOBAL = NO`.
- Kept `0x06281A` active in native island chain (`step:e268,613b2,61e48,61934,6147e,623ac,6273a,62788,60000,60286,62ae0,626bc,0027fc,002992,6138e,00e682,0613f8,016ebc,009ad6,009bf2,0027a6,00b15e,00a8da,017d12,6281a`) and ranked fresh call census for remaining ROM targets:
  1. `0x06282E`: 922 remaining ROM calls (caller: `0x0624C8`: 922) -> SOLE NEXT TARGET #1
  2. `0x01AC90`: 914 remaining ROM calls (callers: `0x00DF1A`: 908, `0x00FD7A`: 2, `0x00FDFE`: 4)
  3. `0x01B102`: 890 remaining ROM calls (callers: `0x01B7F8`: 102, `0x01BBFE`: 332, `0x01BF8A`: 344, `0x01C4E2`: 112)
  4. `0x00350E`: 866 remaining ROM calls (caller: `0x0033FE`: 866)
  5. `0x008E22`: 842 remaining ROM calls (callers: `0x0007B8`: 70, `0x008B3E`: 772)

# M15.37 Execution-Driven Recompilation: 0x017D12 Dual Replay Verification & Displacement — DONE

Delivered:
- Recompiled runtime-discovered target `0x017D12` (Collectible Dropped Item & Chest Entity Handler, Entity Type 0x1E / 30) called 985 times in 3,600 canonical frames across 2 callers (`0x00DF1A`: 981 in the entity main loop, `0x00FF88`: 4 in the entity spawn/initialization dispatcher).
- Disproved working label and proved true entity handler role:
  * Entity Type `0x1E` (30), dispatched through entity jump table at `0x00DF28 + 0x1E * 4 = 0x00DFA0`.
  * Spawn Initialization (`0x017D12..0x017D3C`): tests `$66(A6)`, marks `$66(A6) = 0xFF`, sets initial elevation from `$A0(A6)` into `$10(A6)`, handles pre-opened chests (`bit 7` set -> state `$04 = 10`, anim `$2A += 2`, timer `$02 = 1`).
  * Falling / Air Gravity (`0x017D3E..0x017D82`): integrates velocity `$56(A6) -= 0xC000` into coordinate `$10(A6)`. On ground hit, clamps to ground, rebounds once with upward velocity `$00030000`, sets bounce flag `$A6(A6) = 0xFF`, and plays sound effect `0x00400376` via `jsr 0x00E7FC`.
  * Player Touch / Pickup Detection (`0x017D84..0x017DB8`): if idle (`state == 0`) and contact triggered (`$86(A6) != 0`), arms pickup timer `$02(A6) = 2`, advances state `$04(A6) = 1`, ticks anim `$2A(A6) += 2`, and plays pickup sound `0x00900232` via `jsr 0x00E7FC`.
  * Collection / Inventory Grant (`0x017DBA..0x017DF0`): decrements timer `$02(A6)`. Upon expiration, calls inventory grant `jsr 0x00F53C`, sets persistent event flag `bsr 0x017FF2`, writes item metadata `$A2..$A4(A6)` to player record `$4E..$50(A0)`, and clears entity slot `$00(A6) = 0`.
  * Chest Open Phase (`0x017DF2..0x017E26`): persistent event registration, item grant, and deactivation.
- Child subroutine delegation: implemented stack-push, `child_call_active`, `saved_child_sp`, and `resume_pc` delegation yielding to Musashi during `jsr 0x00E7FC`, `jsr 0x00F53C`, and `bsr 0x017FF2`.
- Achieved 100% bit-for-bit identical state across all 3,600 frames between Run A (`0x017D12` OFF) and Run B (`0x017D12` ON):
  * `different_frame_count: 0`.
  * ROM_EXEC_017D12: Run A = 985; Run B = 0 (100% displaced, 0 fallbacks, 10,025 stepped instructions).
  * NATIVE_EXEC_017D12: Run B = 985; Run A = 0.
  * Matching SHA256 hashes across all 6 canonical acceptance streams (`observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, `call-z80-ram.bin`) plus `external-events.jsonl` and `normalized-events.jsonl`.
  * Status: `CPP_REPLACEMENT_VERIFIED_PATH_SPECIFIC = YES`, `RUNTIME_REPLACEMENT_ACTIVE = YES`, `CPP_REPLACEMENT_VERIFIED_GLOBAL = NO`.
- Kept `0x017D12` active in native island chain (`step:e268,613b2,61e48,61934,6147e,623ac,6273a,62788,60000,60286,62ae0,626bc,0027fc,002992,6138e,00e682,0613f8,016ebc,009ad6,009bf2,0027a6,00b15e,00a8da,017d12`) and ranked fresh call census for remaining ROM targets:
  1. `0x06281A`: 985 remaining ROM calls (caller: `0x06243C`: 985) -> SOLE NEXT TARGET #1
  2. `0x06282E`: 922 remaining ROM calls (caller: `0x0624C8`: 922)
  3. `0x01AC90`: 914 remaining ROM calls (callers: `0x00DF1A`: 908, `0x00FD7A`: 2, `0x00FDFE`: 4)
  4. `0x01B102`: 890 remaining ROM calls (callers: `0x01B7F8`: 102, `0x01BBFE`: 332, `0x01BF8A`: 344, `0x01C4E2`: 112)
  5. `0x00350E`: 866 remaining ROM calls

# M15.36 Execution-Driven Recompilation: 0x00A8DA Dual Replay Verification & Displacement — DONE

Delivered:
- Recompiled runtime-discovered target `0x00A8DA` (Direct VDP Sprite Attribute Table Entry Appender & HUD Shadow Record Builder) called 1,082 times in 3,600 canonical frames across 5 callers (`0x00A886`: 223, `0x00A896`: 223, `0x00A81E`: 212, `0x00A82A`: 212, `0x00A836`: 212).
- Disproved working label ("movement / position update helper") and proved true render pipeline role:
  * Atomic hardware sprite appender invoked by HUD status bar compositor (`0x00A7E6..0x00A8D8`).
  * Checks hardware sprite count against 80-sprite limit (`cmpi.w #80, %d5; bcc.s 0x00A8EE`).
  * Writes 8-byte SAT entry into WRAM shadow buffer (`0x00FF13CC + (0x00FF188C)`) via post-increment stores `(A5)+` for Y coordinate, Link+Size, Tile/Palette/Priority attributes, and X coordinate.
  * Increments sprite count `D5.W` and advances `A5` by +8 bytes.
- Recovered exact Musashi instruction cycles and bus timing:
  * `cmpi.w #80, d5`: 56 master cycles ($8 \times 7$).
  * `bcc.s 0x00A8EE`: 70 master cycles taken, 56 master cycles not taken ($10 \times 7 - 14$).
  * `addq.w #1, d5`: 28 master cycles ($4 \times 7$).
  * `move.w reg, (a5)+`: 56 master cycles ($8 \times 7$) per word write.
  * `move.w reg, reg` & `add.w reg, reg`: 28 master cycles ($4 \times 7$) each.
- Achieved 100% bit-for-bit identical state across all 3,600 frames between Run A (`0x00A8DA` OFF) and Run B (`0x00A8DA` ON):
  * `different_frame_count: 0`.
  * ROM_EXEC_00A8DA: Run A = 1,082; Run B = 0 (100% displaced, 0 fallbacks, 9,738 stepped instructions, 1,082 SAT entries written).
  * NATIVE_EXEC_00A8DA: Run B = 1,082; Run A = 0.
  * Matching SHA256 hashes across all 6 canonical acceptance streams (`observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, `call-z80-ram.bin`) plus `external-events.jsonl` and `normalized-events.jsonl`.
  * Status: `CPP_REPLACEMENT_VERIFIED_PATH_SPECIFIC = YES`, `RUNTIME_REPLACEMENT_ACTIVE = YES`, `CPP_REPLACEMENT_VERIFIED_GLOBAL = NO`.
- Kept `0x00A8DA` active in native island chain (`step:e268,613b2,61e48,61934,6147e,623ac,6273a,62788,60000,60286,62ae0,626bc,0027fc,002992,6138e,00e682,0613f8,016ebc,009ad6,009bf2,0027a6,00b15e,00a8da`) and ranked fresh call census for remaining ROM targets:
  1. `0x017D12`: 985 remaining ROM calls (callers: `0x00DF1A`: 981, `0x00FF88`: 4) -> TIED NEXT TARGET #1
  2. `0x06281A`: 985 remaining ROM calls (caller: `0x06243C`: 985) -> TIED NEXT TARGET #1
  3. `0x06282E`: 922 remaining ROM calls (caller: `0x0624C8`: 922)
  4. `0x01AC90`: 914 remaining ROM calls (callers: `0x00DF1A`: 908, `0x00FD7A`: 2, `0x00FDFE`: 4)
  5. `0x01B102`: 890 remaining ROM calls (callers: `0x01B7F8`: 102, `0x01BBFE`: 332, `0x01BF8A`: 344, `0x01C4E2`: 112)

# M15.35 Execution-Driven Recompilation: 0x00B15E Dual Replay Verification & Displacement — DONE

Delivered:
- Recompiled runtime-discovered target `0x00B15E` (Entity Multi-Piece Sprite Compositor, VDP SAT Shadow Buffer Builder & Dynamic VRAM Tile DMA Request Stager) called 1,165 times in 3,600 canonical frames across 2 callers (`0x00AB7A`: 948, `0x00AC74`: 217).
- Disproved working label "Object / Actor Collision Check" and proved true graphics composition semantics:
  * Reads master entity metadata table at `0x03F2FA - 0x14 + (D5.W << 4)`.
  * Computes orientation, ticks animation frame counters, clips multi-piece sprites against screen boundaries.
  * Builds 8-byte Mega Drive VDP Sprite Attribute Table records into WRAM shadow buffer (`0x00FF13CC`), updates SAT link index (`0x00FF188A`), stages dynamic VRAM tile DMA requests (`0x00FF10B4`), and tracks per-frame tile budget (`0x00FF1652`).
- Recovered exact Musashi instruction cycles and bus timing:
  * Dynamic shift cycle model: `USE_CYCLES(s * CYC_SHIFT)` with `CYC_SHIFT = 14`.
  * `bset #31, d5` (`0x00B1D6`): +14 master cycles when testing/setting bit $\ge 16$.
  * `movem.l` 5 registers save and restore: 280 master cycles each with postincrement dummy read and `skip_bus_refresh`.
  * `dbf` loop timing adjustments: $-14$ master cycles when taken, $+14$ when expired.
- Achieved 100% bit-for-bit identical state across all 3,600 frames between Run A (`0x00B15E` OFF) and Run B (`0x00B15E` ON):
  * `different_frame_count: 0`.
  * ROM_EXEC_00B15E: Run A = 1,165; Run B = 0 (100% displaced, 0 fallbacks, 130,936 stepped instructions, 1,729 SAT records written, 396 DMA descriptors staged, 479 offscreen clipped pieces).
  * NATIVE_EXEC_00B15E: Run B = 1,165; Run A = 0.
  * Matching SHA256 hashes across all 6 canonical acceptance streams (`observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, `call-z80-ram.bin`) plus `external-events.jsonl` and `normalized-events.jsonl`.
  * Status: `CPP_REPLACEMENT_VERIFIED_PATH_SPECIFIC = YES`, `RUNTIME_REPLACEMENT_ACTIVE = YES`, `CPP_REPLACEMENT_VERIFIED_GLOBAL = NO`.
- Kept `0x00B15E` active in native island chain (`step:e268,613b2,61e48,61934,6147e,623ac,6273a,62788,60000,60286,62ae0,626bc,0027fc,002992,6138e,00e682,0613f8,016ebc,009ad6,009bf2,0027a6,00b15e`) and ranked fresh call census for remaining ROM targets:
  1. `0x00A8DA`: 1,082 remaining ROM calls (callers: `0x00A886`: 223, `0x00A896`: 223, `0x00A81E`: 212, `0x00A82A`: 212, `0x00A836`: 212) -> NEXT TARGET #1
  2. `0x017D12`: 985 remaining ROM calls (callers: `0x00DF1A`: 981, `0x00FF88`: 4)
  3. `0x06281A`: 985 remaining ROM calls (caller: `0x06243C`: 985)
  4. `0x06282E`: 922 remaining ROM calls (caller: `0x0624C8`: 922)
  5. `0x01AC90`: 914 remaining ROM calls (callers: `0x00DF1A`: 908, `0x00FD7A`: 2, `0x00FDFE`: 4)

# M15.34 Execution-Driven Recompilation: 0x0027A6 Dual Replay Verification & Displacement — DONE

Delivered:
- Recompiled runtime-discovered target `0x0027A6` (Register-Parameterized Fixed-Buffer VDP Hardware DMA Committer — Direct SAT & CRAM VBlank DMA Executor) called 1,171 times in 3,600 canonical frames.
- Disproved working label "VDP DMA Request Queue Stager" and proved exact sibling relationship with `0x0027FC`:
  * `0x0027FC` (2,806 calls) consumes 8-byte descriptors from the WRAM DMA queue (`0x00FF10B4..0x00FF1334`, pointer `0x00FF1892`).
  * `0x0027A6` (1,171 calls) bypasses the WRAM descriptor queue and commits the two fixed VBlank shadow buffers directly to VDP hardware, sharing the VBlank DMA bandwidth budget (`0x00FF165A`, initialized to `0x00FA` and decremented by `0x0032` per transfer), VDP Mode 2 shadow (`0x00FF0BF4`), Z80 BUSREQ port (`0x00A11100`), and VDP Control/Data ports (`0x00C00004` / `0x00C00000`).
- Verified 2-caller ABI in VBlank ISR (`0x001F76`):
  * Caller A (`0x001FC4`, 822 calls, return PC `0x001FC8`): Sprite Attribute Table (SAT) DMA from `A5 = 0x00FF13CC` to VRAM `0xD000` (`D5.L = 0x50000003`, `A6 = 0x00001FB8` -> `[0x95E6, 0x9689, 0x977F]`, `D6.L = 0x94xx93yy` from sprite count `0x00FF188A`).
  * Caller B (`0x002252`, 349 calls, return PC `0x002256`): Color Palette RAM (CRAM) DMA from `A5 = 0x00FF134C` to CRAM `0x0000` (`D5.L = 0xC0000000`, `A6 = 0x00002240` -> `[0x95A6, 0x9689, 0x977F]`, `D6.L = 0x94009340` = 64 words).
- Achieved 100% bit-for-bit identical state across all 3,600 frames between Run A (`0x0027A6` OFF) and Run B (`0x0027A6` ON):
  * `different_frame_count: 0`.
  * ROM_EXEC_0027A6: Run A = 1,171; Run B = 0 (100% displaced, 0 fallbacks, 26,933 stepped instructions, 1,171 DMA transfers).
  * NATIVE_EXEC_0027A6: Run B = 1,171; Run A = 0.
  * Matching SHA256 hashes across all 6 canonical acceptance streams (`observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, `call-z80-ram.bin`) plus `external-events.jsonl` and `normalized-events.jsonl`.
  * Status: `CPP_REPLACEMENT_VERIFIED_GLOBAL = YES`, `RUNTIME_REPLACEMENT_ACTIVE = YES`.
- Kept `0x0027A6` active in native island chain (`step:e268,613b2,61e48,61934,6147e,623ac,6273a,62788,60000,60286,62ae0,626bc,0027fc,002992,6138e,00e682,0613f8,016ebc,009ad6,009bf2,0027a6`) and ranked fresh call census for remaining ROM targets:
  1. `0x00B15E`: 1,165 remaining ROM calls (callers: `0x00AB7A`: 948, `0x00AC74`: 217) -> NEXT TARGET #1
  2. `0x00A8DA`: 1,082 remaining ROM calls (callers: `0x00A886`: 223, `0x00A896`: 223, `0x00A81E`: 212, `0x00A82A`: 212, `0x00A836`: 212)
  3. `0x017D12`: 985 remaining ROM calls (callers: `0x00DF1A`: 981, `0x00FF88`: 4)
  4. `0x06281A`: 985 remaining ROM calls (caller: `0x06243C`: 985)
  5. `0x06282E`: 922 remaining ROM calls (caller: `0x0624C8`: 922)

# M15.33 Execution-Driven Recompilation: 0x009BF2 Dual Replay Verification & Displacement — DONE

Delivered:
- Closed Phase 0 acceptance for `0x009AD6` across all six canonical streams (`observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, `call-z80-ram.bin`), marking `0x009AD6` as `CPP_REPLACEMENT_VERIFIED_PATH_SPECIFIC = YES`, `RUNTIME_REPLACEMENT_ACTIVE = YES`, `CPP_REPLACEMENT_VERIFIED_GLOBAL = NO`.
- Recompiled runtime-discovered target `0x009BF2` (Entity Bounding-Box Terrain Grid Footprint Bitwise OR/AND Aggregator) called 1,283 times in 3,600 canonical frames.
- Verified 8-caller ABI and exact pairing with `0x009AD6`:
  * `0x00935C` (`-> 0x009376`): 715 calls
  * `0x0091D4` (`-> 0x0091EE`): 297 calls
  * `0x009312` (`-> 0x009330`): 132 calls
  * `0x009D26` (`-> 0x009D40`): 42 calls
  * `0x009270` (`-> 0x00928E`): 32 calls
  * `0x0098D4` (`-> 0x0098E0`): 24 calls
  * `0x00918A` (`-> 0x0091A8`): 22 calls
  * `0x0090E8` (`-> 0x009106`): 19 calls
- Resolved semantics & invariants:
  * Inputs: `A0` (active terrain grid descriptor `0x00FF1766` or `0x00FF1770`: `0(A0)` = grid base `0x00FF5FAC`, `4(A0)` = row stride `64`, `8(A0)` = row shift `6`), `D1.W` (center X px), `D2.W` (center Y px), `D3.W` (footprint half-size radius `$46(A6)`, observed `8` or `10` px).
  * Outputs: `D4.B` = bitwise OR of all 8x8 terrain cells in `[X - R, X + R] x [Y - R, Y + R]` (written by callers to `$6F(A6)`), `D5.B` = bitwise AND of all covered cells (written by callers to `$6E(A6)` and mapped via `0x0096E8` into `$70(A6)`).
  * Preserved registers: `D0, D1, D2, D3, D6, D7, A0, A3-A7` (saving/restoring `D2, D3, D6, D7` via `movem.l d2-d3/d6-d7, -(sp)` and `(sp)+`).
- Exact Musashi timing & bus alignment:
  * `movem.l d2-d3/d6-d7, -(sp)`: eight 16-bit predecrement writes, `+224` master cycles ($4 \times 8 \times 7$), `skip_bus_refresh`.
  * `movem.l (sp)+, d2-d3/d6-d7`: four 32-bit postincrement reads + one 16-bit dummy read at `SP`, `+224` master cycles, `skip_bus_refresh`.
  * `asr.w #3, Dn`: `+42` master cycles.
  * `lsl.w d5, d4`: `14 * shift` master cycles + `skip_bus_refresh`.
  * `dbf Dn, disp`: `-14` master cycles (with extension fetch) when taken; `+14` master cycles (no extension fetch) when expired.
- Achieved 100% bit-for-bit identical state across all 3,600 frames between Run A (`0x009BF2` OFF) and Run B (`0x009BF2` ON):
  * `different_frame_count: 0`.
  * ROM_EXEC_009BF2: Run A = 1,283; Run B = 0 (100% displaced, 0 fallbacks, 124,041 stepped instructions, 18,816 inner cell reads, 4,817 outer row steps).
  * NATIVE_EXEC_009BF2: Run B = 1,283; Run A = 0.
  * Matching SHA256 hashes across all 6 canonical acceptance streams: `observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, and `call-z80-ram.bin`.
  * Status: `CPP_REPLACEMENT_VERIFIED_GLOBAL = YES`, `RUNTIME_REPLACEMENT_ACTIVE = YES`.
- Kept `0x009BF2` active in native island chain (`step:e268,613b2,61e48,61934,6147e,623ac,6273a,62788,60000,60286,62ae0,626bc,0027fc,002992,6138e,00e682,0613f8,016ebc,009ad6,009bf2`) and ranked fresh call census for remaining ROM targets:
  1. `0x0027A6`: 1,171 remaining ROM calls (callers: `0x001FC4`: 822, `0x002252`: 349) -> NEXT TARGET #1
  2. `0x00B15E`: 1,165 remaining ROM calls (callers: `0x00AB7A`: 948, `0x00AC74`: 217)
  3. `0x00A8DA`: 1,082 remaining ROM calls (callers: `0x00A886`: 223, `0x00A896`: 223, `0x00A81E`: 212, `0x00A82A`: 212, `0x00A836`: 212)
  4. `0x017D12`: 985 remaining ROM calls (callers: `0x00DF1A`: 981, `0x00FF88`: 4)
  5. `0x06281A`: 985 remaining ROM calls (caller: `0x06243C`: 985)

# M15.32 Execution-Driven Recompilation: 0x009AD6 Dual Replay Verification & Displacement — DONE

Delivered:
- Recompiled runtime-discovered target `0x009AD6` (Terrain Ground Height Calculation & Slope Elevation Interpolation Routine) called 1,283 times in 3,600 canonical frames.
- Verified 8-caller ABI in collision/physics and map update subsystems:
  * `0x009106`: 19 calls
  * `0x0091A8`: 22 calls
  * `0x0091EE`: 297 calls
  * `0x00928E`: 32 calls
  * `0x009330`: 132 calls
  * `0x009376`: 715 calls
  * `0x0098E0`: 24 calls
  * `0x009D40`: 42 calls
- Resolved semantics & invariants:
  * Inputs: `A0` (terrain grid descriptor), `A6` (`8(A6)` = X, `12(A6)` = Y).
  * Output: `D4.W` (Ground elevation/height).
  * Preserved registers: `D0, D1, D2, D3, D5, D6, D7, A0, A3-A7` (saving/restoring `D1-D3` via `movem.l d1-d3, -(sp)` and `(sp)+`).
  * Flat terrain path (`0x009AD6..0x009B26`): reads map cell byte, shifts `lsl.w #4, d4` for flat height `0..15`, returns in `D4.W` (1,270 calls in 3,600 frames).
  * Sloped terrain path (`0x009B28..0x009BEA`): handles slope types `0x10 <= type <= 0x23`, scans contiguous slope run, interpolates elevation via `mulu.w` and `divu.w` (13 calls in 3,600 frames).
- Exact Musashi timing & cycle alignment:
  * `push_initial_regs` & `pop_final_regs`: 32-bit register transfers require 168 master cycles ($3 \times 8 \times 7$) and `skip_bus_refresh`.
  * Dynamic `MULU.W`: bit-exact $38 + 2 \times N$ CPU cycles $\times 7$ master clock.
  * Dynamic `DIVU.W`: bit-exact $76 \times 7$ master clock plus 16-cycle iterative non-restoring quotient loop and `skip_bus_refresh`.
  * Branch instructions (`Bcc`): verified that taken/not-taken cycles do not suppress DRAM refresh.
- Achieved 100% bit-for-bit identical state across all 3,600 frames between Run A (0x009AD6 OFF) and Run B (0x009AD6 ON):
  * `different_frame_count: 0`.
  * ROM_EXEC_009AD6: Run A = 1,283; Run B = 0 (100% displaced, 0 fallbacks, 31,624 stepped instructions).
  * NATIVE_EXEC_009AD6: Run B = 1,283; Run A = 0.
  * Matching SHA256 hashes across all 6 canonical acceptance streams: `observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, and `call-z80-ram.bin`.
  * Status: `CPP_REPLACEMENT_VERIFIED_PATH_SPECIFIC = YES`, `RUNTIME_REPLACEMENT_ACTIVE = YES`, `CPP_REPLACEMENT_VERIFIED_GLOBAL = NO`.
- Kept 0x009AD6 active in native island chain (`step:e268,613b2,61e48,61934,6147e,623ac,6273a,62788,60000,60286,62ae0,626bc,0027fc,002992,6138e,00e682,0613f8,016ebc,009ad6`) and ranked fresh call census for remaining ROM targets:
  1. `0x009BF2`: 1,283 remaining ROM calls (8 callers matching 0x009AD6 call sites) -> NEXT TARGET #1
  2. `0x0027A6`: 1,171 remaining ROM calls (callers: `0x001FC4`: 822, `0x002252`: 349)
  3. `0x00B15E`: 1,165 remaining ROM calls (callers: `0x00AB7A`: 948, `0x00AC74`: 217)
  4. `0x00A8DA`: 1,082 remaining ROM calls (callers: `0x00A886`: 223, `0x00A896`: 223, `0x00A81E`: 212)
  5. `0x017D12`: 985 remaining ROM calls (callers: `0x00DF1A`: 981, `0x00FF88`: 4)

# M15.31 Execution-Driven Recompilation: 0x016EBC Dual Replay Verification & Displacement — DONE

Delivered:
- Recompiled runtime-discovered target `0x016EBC` (Interactive Entity / World Object Update & Initialization Handler) called 1,310 times in 3,600 canonical frames.
- Verified dual-caller ABI:
  * `0x00DF1A` inside main entity update loop (`0x00DEF8..0x00DF52`): 1,306 calls (`jsr (a0)`, return PC `0x00DF1C`).
  * `0x00FF88` inside entity spawn / setup dispatcher: 4 calls (`jsr (a0)`, return PC `0x00FF8A`).
- Resolved semantics & invariants:
  * `A6`: Pointer to 188-byte (`0xBC`) entity context in WRAM (`0x00FF0000..0x01000000`).
  * Initialization path (`0x016EBC..0x016EE8`):
    - Triggered when `0xA6(a6) == 0`. Sets initialization flag `0xA6(a6) = 0xFF`.
    - Copies initial sprite/frame index: `16(a6) = 140(a6)` (`move.w $8C(a6), $10(a6)`).
    - If property `158(a6) == 0` (`0x9E(a6)`): initializes delay/timer `74(a6) = 16` (`move.w #16, $4A(a6)`).
    - If property `158(a6) == 1`: sets interaction/collision flag bit 0 of `60(a6)` (`bset #0, $3C(a6)`).
    - Returns via `rts` at `0x016EE8`. (Observed 4 calls from `0x00FF88`).
  * Update path (`0x016EBC..0x016F0E`):
    - Triggered when `0xA6(a6) != 0`. Inspects interaction/defeat trigger byte `102(a6)` (`0x66(a6)`).
    - If `0x66(a6) == 0` (idle/waiting): branches directly to `0x016F0E` and returns via `rts`. (Observed 1,306 calls from `0x00DF1A`).
    - Unobserved trigger paths (`0x66(a6) != 0`): increments counter `2(a6)` up to 6, clears slot `0(a6) = 0`, calls `0x018030` to lookup persistent flag bit, and executes `bset d0, (a0)`; falls back to Musashi ROM fail-closed.
- Exact Musashi execution & memory ordering:
  * `move.w $8C(a6), $10(a6)`: fetches source displacement `0x008C`, reads source word from `a6 + 0x8C`, fetches destination displacement `0x0010`, writes destination word to `a6 + 0x10`, updates CCR N/Z.
- Achieved 100% bit-for-bit identical state across all 3,600 frames between Run A (0x016EBC OFF) and Run B (0x016EBC ON):
  * `different_frame_count: 0`.
  * ROM_EXEC_016EBC: Run A = 1,310; Run B = 0 (100% displaced, 0 fallbacks, 5,260 stepped instructions).
  * NATIVE_EXEC_016EBC: Run B = 1,310; Run A = 0.
  * Matching SHA256 hashes across all 6 diagnostic streams: `observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, and `call-z80-ram.bin`.
- Kept 0x016EBC active in native island chain (`step:e268,613b2,61e48,61934,6147e,623ac,6273a,62788,60000,60286,62ae0,626bc,0027fc,002992,6138e,00e682,0613f8,016ebc`) and ranked fresh call census for remaining ROM targets:
  1. `0x009AD6`: 1,283 remaining ROM calls (8 callers) -> NEXT TARGET (tied)
  2. `0x009BF2`: 1,283 remaining ROM calls (8 callers) -> NEXT TARGET (tied)
  3. `0x0027A6`: 1,171 remaining ROM calls (callers: `0x001FC4`: 822, `0x002252`: 349)
  4. `0x00B15E`: 1,165 remaining ROM calls (callers: `0x00AB7A`: 948, `0x00AC74`: 217)
  5. `0x00A8DA`: 1,082 remaining ROM calls (callers: `0x00A886`: 223, `0x00A896`: 223, `0x00A81E`: 212)

# M15.30 Execution-Driven Recompilation: 0x0613F8 Dual Replay Verification & Displacement — DONE

Delivered:
- Recompiled runtime-discovered target `0x0613F8` (Sound Channel Dual Auxiliary Voice / Pitch-Modulation Slot Initializer) called 1,687 times in 3,600 canonical frames.
- Verified single-caller ABI:
  * `0x061A16` inside native `0x061934`: 1,687 calls (`bsr.w 0x0613F8`, displacement `0xF9E0`, return PC `0x061A1A`).
- Resolved semantics & invariants:
  * `A6`: Sound channel record in WRAM (`0x00FF0000..0x01000000`).
  * Outer routine (`0x0613F8..0x061438`):
    - Preserves `D0, A0, A1` via `movem.l d0/a0-a1, -(sp)`.
    - Evaluates Slot 1 (`+$B6(A6)`) status byte bits 4 and 5; calls sub-routine `0x06143A` if both are set.
    - Evaluates Slot 2 (`+$C0(A6)`) status byte bits 4 and 5; calls sub-routine `0x06143A` if both are set.
    - Restores `D0, A0, A1` via `movem.l (sp)+, d0/a0-a1` and exits via RTS at `0x061438`.
  * Sub-helper routine (`0x06143A..0x06147C`):
    - Unpacks status byte lower 4 bits into `5(A0)`, clears `6(A0)`, `7(A0)`, and `4(A0)` via `sf`, copies initial parameter from `3(A0)` to `6(A0)`.
    - Path Default (`8(A0) >= 0xB0`): reads default pitch byte from `+$18(A1)` (`+$B5(A6)`) to `9(A0)`, returns via `0x06146E`.
    - Path Table (`8(A0) < 0xB0`): shifts `d0 >>= 2` (`lsr.b #2, d0`, +28 master cycles), reads table byte `(A1, D0.W)` (`+$9D(A6) + index`) to `9(A0)`, returns via `0x06147C`.
  * Relationship to `0x06138E`: `0x061A12` calls `0x06138E` (envelope config `+$78..+$82`), followed immediately by `0x061A16 -> 0x0613F8` (modulation slots `+$B6..+$BF`, `+$C0..+$C9`, table `+$9D..+$B5`). They do not overwrite each other.
- Exact Musashi execution & timing recovery:
  * `movem.l d0/a0-a1, -(sp)`: 168 master cycles ($3 \times 8 \times 7$), write order A1, A0, D0, calls `skip_bus_refresh`.
  * `movem.l (sp)+, d0/a0-a1`: post-increment dummy read at `new_sp`, 168 master cycles ($3 \times 8 \times 7$), calls `skip_bus_refresh`.
- Achieved 100% bit-for-bit identical state across all 3,600 frames between Run A (0x0613F8 OFF) and Run B (0x0613F8 ON):
  * `different_frame_count: 0`.
  * ROM_EXEC_0613F8: Run A = 1,687; Run B = 0 (100% displaced, 0 fallbacks, 13,496 stepped instructions).
  * NATIVE_EXEC_0613F8: Run B = 1,687; Run A = 0.
  * Matching SHA256 hashes across `observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, and `call-z80-ram.bin`.
- Kept 0x0613F8 active in native island chain (`step:e268,613b2,61e48,61934,6147e,623ac,6273a,62788,60000,60286,62ae0,626bc,0027fc,002992,6138e,00e682,0613f8`) and ranked fresh call census for remaining ROM targets:
  1. `0x016EBC`: 1,310 remaining ROM calls (callers: `0x00DF1A`: 1,306, `0x00FF88`: 4) -> NEXT TARGET
  2. `0x009AD6`: 1,283 remaining ROM calls (8 callers)
  3. `0x009BF2`: 1,283 remaining ROM calls (8 callers)
  4. `0x0027A6`: 1,171 remaining ROM calls (callers: `0x001FC4`: 822, `0x002252`: 349)
  5. `0x00B15E`: 1,165 remaining ROM calls (callers: `0x00AB7A`: 948, `0x00AC74`: 217)

# M15.29 Execution-Driven Recompilation: 0x00E682 Dual Replay Verification & Displacement — DONE

Delivered:
- Recompiled runtime-discovered target `0x00E682` (Entity Action/Animation Freeze Timer & Damage Stun Gatekeeper) invoked 2,104 times across 6 measured callers.
- Verified 6-caller ABI:
  * `0x0057D0` (772 calls): Player update status/animation tick (`A6 = 0x00FF19E8`).
  * `0x01A54E` (424 calls): Entity handler.
  * `0x01BF84` (350 calls): Entity handler.
  * `0x01BBF8` (337 calls): Entity handler.
  * `0x01C4DC` (116 calls): Entity handler.
  * `0x01B7F2` (105 calls): Entity handler.
- Resolved semantics & invariants:
  * `A6`: Entity context pointer in work RAM (`0x00FF0000..0x01000000`).
  * Operation:
    - Path 1 (Normal, 2,030 calls): `+$9D == 0` and `+$82 == 0`. Clears hurt flash flag (`bclr #6, $36(a6)`), exits via RTS at `0x00E698`.
    - Path 2A (Timer Expired, 31 calls): `+$9D == 0` and `+$82 == 1`. Increments animation tick `$2E(a6)`, decrements `$82(a6)` to zero. `beq.s` taken to `0x00E6A8`, exits via normal RTS.
    - Path 2B (Timer Active > 1, 43 calls): `+$9D == 0` and `+$82 > 1`. Increments animation tick `$2E(a6)`, decrements `$82(a6)`. Executes `addq.l #4, sp` at `0x00E6A6` and exits via RTS at `0x00E6A8`, popping caller return address and returning directly to grandparent entity dispatcher.
    - Path 3A (Stun $\ge 20$, 0 calls in 3,600-frame replay, unit-tested): `addq.l #4, sp` pops caller, returns to grandparent.
    - Path 3B (Stun $< 20$, 0 calls in 3,600-frame replay, unit-tested): sets hurt flash flag (`bset #6, $36(a6)`), `addq.l #4, sp` pops caller, returns to grandparent.
- Solved caller stack bypass architecture:
  * When freeze timer or stun is active, popping SP (`addq.l #4, sp`) causes RTS to bypass caller remainder and return directly to grandparent.
  * Reconciles why call census showed exactly 2,061 completed returns out of 2,104 calls (2,104 - 2,061 = exactly 43 bypass calls).
- Achieved 100% bit-for-bit identical state across all 3,600 frames between Run A (0x00E682 OFF) and Run B (0x00E682 ON):
  * `different_frame_count: 0`.
  * ROM_EXEC_00E682: Run A = 2,104; Run B = 0 (100% displaced, 0 fallbacks, 10,859 stepped instructions).
  * NATIVE_EXEC_00E682: Run B = 2,104; Run A = 0.
  * Matching SHA256 hashes across `observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, and `call-z80-ram.bin`.
- Kept 0x00E682 active in native island chain (`step:e268,613b2,61e48,61934,6147e,623ac,6273a,62788,60000,60286,62ae0,626bc,0027fc,002992,6138e,00e682`) and ranked fresh call census for remaining ROM targets:
  1. `0x0613F8`: 1,687 remaining ROM calls (caller: `0x061A16` inside native `0x061934`) -> NEXT TARGET
  2. `0x016EBC`: 1,310 remaining ROM calls (callers: `0x00DF1A`: 1,306, `0x00FF88`: 4)
  3. `0x009AD6`: 1,283 remaining ROM calls (8 callers)
  4. `0x009BF2`: 1,283 remaining ROM calls (8 callers)
  5. `0x0027A6`: 1,171 remaining ROM calls (callers: `0x001FC4`: 822, `0x002252`: 349)

# M15.28 Execution-Driven Recompilation: 0x06138E Dual Replay Verification & Displacement — DONE

Delivered:
- Recompiled runtime-discovered target `0x06138E` (Sound Channel Envelope Parameter Staging & Modulation Init) used by two already-native parent branches.
- Verified dual-caller ABI:
  * Caller A (`0x061A12` inside native `0x061934`): 1,687 calls (`bsr.w 0x06138E`, displacement `0xF97A`, return PC `0x061A16`).
  * Caller B (`0x0624A0` inside native `0x0623AC`): 922 calls (`bsr.w 0x06138E`, displacement `0xEEEC`, return PC `0x0624A4`).
- Resolved semantics & invariants:
  * `A6`: Sound channel context in WRAM (`0x00FF0000..0x01000000`).
  * Operation: copies attack/decay envelope configuration (`+$78` -> `+$7E`, `+$79` -> `+$7F`, `+$7C` -> `+$80`), divides modulation rate by 2 via `lsr.w #1, d6` (`+$7A` -> `D6` -> `+$82`), resets channel phase timer (`+$76` -> `0x0000`), and returns to parent via RTS.
  * Disproved legacy working assumption: NOT a coordinate or graphics unpacker; pure audio driver channel envelope staging.
- Exact Musashi execution & timing recovery:
  * Interleaved memory ordering: `fetch source displacement` -> `read source` -> `fetch destination displacement` -> `write destination`.
  * Dynamic shift cycle timing on `lsr.w #1, d6`: added $+14$ master cycles ($2 \text{ CPU cycles} \times 7$) before `finish_instruction`.
  * Native RTS handoff: cleanly returns `NotHandled` at `0x0613B0` for Musashi native pipeline retirement, identical to the standard native island architecture.
- Reconciled census & verified historical helper cohort:
  * Verified `0x00E268` as fully verified native helper (`CPP_REPLACEMENT_VERIFIED_PATH_SPECIFIC = YES`), preserved active in `--native-island`.
- Achieved 100% bit-for-bit identical state across all 3,600 frames between Run A (0x06138E OFF) and Run B (0x06138E ON):
  * `different_frame_count: 0`.
  * ROM_EXEC_06138E: Run A = 2,609; Run B = 0 (100% displaced, 0 fallbacks, 18,263 stepped instructions).
  * NATIVE_EXEC_06138E: Run B = 2,609 (caller `0x061A12`: 1,687; caller `0x0624A0`: 922); Run A = 0.
  * Matching SHA256 hashes across `observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, and `call-z80-ram.bin`.
- Kept 0x06138E active in native island chain (`step:e268,613b2,61e48,61934,6147e,623ac,6273a,62788,60000,60286,62ae0,626bc,0027fc,002992,6138e`) and ranked fresh call census for remaining ROM targets:
  1. `0x00E682`: 2,104 remaining ROM calls (callers: `0x0057D0`: 772, `0x01A54E`: 424, `0x01B7F2`: 105, `0x0182E4`: 18, `0x017FF4`: 14, `0x0180F4`: 11) -> NEXT TARGET
  2. `0x0613F8`: 1,687 remaining ROM calls (caller: `0x061A16`: 1,687 inside native `0x061934`)
  3. `0x016EBC`: 1,310 remaining ROM calls (callers: `0x00DF1A`: 1,306, `0x00FF88`: 4)
  4. `0x009AD6`: 1,283 remaining ROM calls (8 callers)
  5. `0x009BF2`: 1,283 remaining ROM calls (8 callers)

# M15.27 Execution-Driven Recompilation: 0x002992 Dual Replay Verification & Displacement — DONE

Delivered:
- Recompiled runtime-discovered target `0x002992` (Mega Drive Hardware Gamepad Reader & Debouncer) discovered from runtime call census as the #1 untranslated gameplay routine.
- Verified caller ABI:
  * `0x002188` in VBlank handler immediately following `0x0027FC`: 2,714 calls (`bsr.w 0x002992`, displacement `0x0808` (+2056), return PC `0x00218C`).
- Resolved semantics & invariants:
  * `A0`: Port 1 Data Register `0x00A10003`.
  * `A1`: 6-byte Gamepad State Record in work RAM `0x00FF165C`.
  * Operation: Asserts Z80 BUSREQ (`0x00A11100`), toggles TH pin on `0x00A10003` (`#$40` high, `#$00` low) across multiple polling cycles, executes Sega 6-button controller handshake protocol, debounces button transitions by comparing current against previous state `(A1)` to compute newly pressed masks (`SACBRLDU` and `Mode/X/Y/Z`), writes the 6-byte record, releases Z80 bus, and exits via RTS.
  * Disproved working assumption: ZERO VDP accesses (0 DMA, 0 VRAM, 0 CRAM). It is dedicated gamepad controller I/O and debouncing logic.
- Runtime path distribution across all 2,714 calls:
  * Path A (6-button controller detected, 2,712 calls): reads extra Mode/X/Y/Z buttons, writes full 6-byte state, releases Z80 bus, exits at RTS (`0x002A54`).
  * Path B (3-button controller detected, 1 call): normal 3-button decode, writes 6-byte record, releases Z80 bus, exits at RTS (`0x002A88`).
  * Path C (invalid/disconnected controller handshake, 1 call): clears controller record, releases Z80 bus, exits at RTS (`0x002AE0`).
- Exact timing recovery:
  * Factored dynamic cycle timing for instruction 87 (`0xE502`: `asl.b #2, d2`): `shift * 2 * 7 = +28` master cycles.
  * Handled 43 NOP instructions (`0x4E71`) using exact 4-cycle base table timing.
- Achieved 100% bit-for-bit identical state across all 3,600 frames between Run A (0x002992 OFF) and Run B (0x002992 ON):
  * `different_frame_count: 0`.
  * ROM_EXEC_002992: Run A = 2,714; Run B = 0 (100% displaced, 0 fallbacks, 279,482 stepped instructions).
  * NATIVE_EXEC_002992: Run B = 2,714; Run A = 0.
  * Matching SHA256 hashes across `observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, and `call-z80-ram.bin`.
- Kept 0x002992 active in native island chain (`step:613b2,61e48,61934,6147e,623ac,6273a,62788,60000,60286,62ae0,626bc,0027fc,002992`) and ranked fresh call census for next targets:
  1. `0x00E268`: 455,656 calls (callers: `0x00340E`: 364,289, `0x0033E2`: 46,259, `0x00359A`: 45,047) -> NEXT TARGET (ultra-high frequency engine math/coordinate utility)
  2. `0x06138E`: 2,609 calls (callers: `0x061A12`: 1,687, `0x0624A0`: 922 in audio island)
  3. `0x00E682`: 2,104 calls (callers: `0x0057D0`: 772, `0x01A54E`: 424, `0x01B7F2`: 105)
  4. `0x0613F8`: 1,687 calls (caller: `0x061A16`: 1,687)
  5. `0x016EBC`: 1,310 calls (callers: `0x00DF1A`: 1,306, `0x00FF88`: 4)
  6. `0x0027A6`: 1,171 calls (callers: `0x001FC4`: 822, `0x002252`: 349 in VBlank/main loop)

# M15.26 Execution-Driven Recompilation: 0x0027FC Dual Replay Verification & Displacement — DONE

Delivered:
- Recompiled runtime-discovered target `0x0027FC` (VDP DMA Queue Record Consumer & Direct Hardware Committer) discovered from runtime call census as the #1 untranslated gameplay routine.
- Verified caller ABIs:
  * `0x002156` in VBlank/DMA main loop: 2,806 calls (`bsr.w 0x0027FC`, displacement `0x06A4` (+1700), return PC `0x00215A`).
  * `0x002678` static alternate path: 0 calls in 3,600 frames (displacement `0x0182` (+386), return PC `0x00267C`).
- Resolved semantics & invariants:
  * `A0`: 8-byte DMA transfer record pointer in work RAM (`0x00FF0BE4`).
  * `FF0BF4`: VDP Mode 2 shadow register word.
  * Operation: Disables interrupts via SR mask, asserts and waits for Z80 BUSREQ (`0x00A11100`), writes VDP Mode 2 with DMA enable bit (0x10) to control port `0x00C00004`, programs DMA length (regs 0x14, 0x13) and source address (regs 0x15, 0x16, 0x17), triggers DMA via destination address words, restores Mode 2 without DMA bit, sends control command and writes initial word to VDP data port `0x00C00000` via `A5 = (A0)*2`, releases Z80 bus, restores SR, and returns via RTS.
- Forensic timing recovery:
  * Resolved 28 master cycle (4 M68k cycle) discrepancy at frame 118 traced to dynamic shift timing of `0xE55D` (`rol.w #2, d5`).
  * Implemented exact `shift * 2 * 7` (+28 master cycles) dynamic cycle compensation in `step_rol_w_imm_reg`, yielding 100% bit-for-bit cycle and scheduler parity from frame 1 to 3,600.
- Achieved 100% bit-for-bit identical state across all 3,600 frames between Run A (0x0027FC OFF) and Run B (0x0027FC ON):
  * `different_frame_count: 0`.
  * ROM_EXEC_0027FC: Run A = 2,806; Run B = 0 (100% displaced, 0 fallbacks, 123,464 stepped instructions, 2,806 DMA transfers committed).
  * NATIVE_EXEC_0027FC: Run A = 0; Run B = 2,806.
  * Matching SHA256 hashes across `observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, and `call-z80-ram.bin`.
- Kept 0x0027FC active in native island chain (`step:613b2,61e48,61934,6147e,623ac,6273a,62788,60000,60286,62ae0,626bc,0027fc`) and ranked fresh call census for next targets:
  1. `0x002992`: 2,714 calls (caller `0x002188: 2,714` in main frame loop / VBlank processor) -> NEXT TARGET
  2. `0x06138E`: 2,609 calls (callers: `0x061A12`: 1,687, `0x0624A0`: 922)
  3. `0x00E682`: 2,104 calls (callers: `0x0057D0`: 772, `0x01A54E`: 424, etc.)
  4. `0x0613F8`: 1,687 calls (caller: `0x061A16`: 1,687)
  5. `0x016EBC`: 1,310 calls (callers: `0x00DF1A`: 1,306, `0x00FF88`: 4)

# M15.25 Execution-Driven Recompilation: 0x0626BC Dual Replay Verification & Displacement — DONE

Delivered:
- Recompiled runtime-discovered target `0x0626BC` (Ancient Music Driver Sound Script Bytecode Lookahead & Loop Marker Detector).
- Verified dual caller ABIs directly connected inside two already-native subsystems:
  * `0x061ABA` inside native `0x061934`: 1,961 calls (`bsr.w 0x0626BC`, return PC `0x061ABE`, displacement `0x0C00`).
  * `0x062544` inside native `0x0623AC`: 1,009 calls (`bsr.w 0x0626BC`, return PC `0x062548`, displacement `0x0176`).
  * Total calls across 3,600 canonical frames: 2,970 calls.
- Resolved semantics & invariants:
  * `A0`: script bytecode stream pointer.
  * `A6`: work RAM channel context.
  * Scans bytecode for loop marker `0xFD` (or sub-commands under `0xF4`). If `(A0) == 0xFD`, sets bit 1 in channel status byte `0(A6)` and increments `A0` (`addq.l #1, a0`). Both callers follow RTS with `move.l a0, 2(a6)`.
  * 0 direct MMIO accesses (pure CPU bytecode analysis and RAM context bit setting).
- Runtime path distribution across all 2,970 calls:
  * Path 1 (160 calls): `(a0) == 0xFD` -> `bset #1, 0(a6)`, `addq.l #1, a0`, `rts` at `0x0626CC`.
  * Path 2 (2,810 calls): `(a0) != 0xFD && (a0) != 0xF4` -> `rts` at `0x0626D6`.
  * Paths 3–7 (0 calls in canonical gameplay, fully implemented and tested): F4 sub-command lookaheads (`1(a0) == 1`, `2(a0) == 0xFD`, `7(a0) == 0xFD`, `9(a0) == 0xFD`).
- Achieved 100% bit-for-bit identical state across all 3,600 frames between Run A (0x0626BC OFF) and Run B (0x0626BC ON):
  * `different_frame_count: 0`.
  * ROM_EXEC_0626BC: Run A = 2,970; Run B = 0 (100% displaced, 0 fallbacks, 11,880 stepped instructions).
  * NATIVE_EXEC_0626BC: Run A = 0; Run B = 2,970.
  * Matching SHA256 hashes across `observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, and `call-z80-ram.bin`.
- Kept 0x0626BC active in native island chain (`step:613b2,61e48,61934,6147e,623ac,6273a,62788,60000,60286,62ae0,626bc`) and ranked fresh call census for next targets: `0x06138E` (2,609 calls, shared coordinate unpacker called inside `0x061934` and `0x0623AC`) and `0x0027FC` (2,806 calls in main loop).

# M15.24 Execution-Driven Recompilation: 0x062AE0 Dual Replay Verification & Displacement — DONE

Delivered:
- Recompiled runtime-discovered target `0x062AE0` (Ancient Music Driver FM Voice Parameter Processor / Channel 6 Sequencer).
- Verified caller ABI (`0x0601E2`: 3,486 calls, displacement `0x28FC`) directly connected inside parent `0x060000`.
- Verified Channel 6 invariant registers (`A6 = 0x00FF05B2`, `D7 = 6`, `A4 = 0x00FF077C`, `A5 = 0x00FF001A`).
- Resolved hardware boundary: 0 direct MMIO accesses; stages sound command packets into 68000 work RAM buffer `(A4)+` subsequently committed to Z80 by native `0x060286`.
- Fully covered observed paths: Path 1 (311 muted skips), Path 2 (26 BGM voice parameter loads), Path 3 (3,175 disabled channel buffer terminations); fail-closed ROM fallback for unobserved `0x062B1C`.
- Achieved 100% bit-for-bit identical state across all 3,600 frames between Run A (0x062AE0 OFF) and Run B (0x062AE0 ON):
  * `different_frame_count: 0`.
  * ROM_EXEC_062AE0: Run A = 3,486; Run B = 0 (100% displaced, 0 fallbacks, 27,085 stepped instructions).
  * NATIVE_EXEC_062AE0: Run A = 0; Run B = 3,486.
  * Matching SHA256 hashes across `observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, and `call-z80-ram.bin`.
- Kept 0x062AE0 active in native island chain (`step:613b2,61e48,61934,6147e,623ac,6273a,62788,60000,60286,62ae0`) and ranked fresh call census for next target: `0x0626BC` (2,970 calls, dual callers inside already-native `0x061934` and `0x0623AC`).

# M15.23 Execution-Driven Recompilation: 0x060286 Dual Replay Verification & Displacement — DONE

Delivered:
- Recompiled runtime-discovered target `0x060286` (Ancient Music Driver Z80 Sound Command Commit & Bus Controller).
- Verified caller ABI (`0x06027A`: 3,486 calls, displacement `+0x000A`) directly connected inside parent `0x060000`.
- Resolved hardware boundary: Z80 BUSREQ assertion/release (`0x00A11100`), bus-grant wait loop, double-buffered packet transfer to Z80 RAM (`0x00A00017` / `0x00A001D9`), mailbox signaling (`0x00A00004`), and 68000 buffer pointer reset (`A4 = 0x00FF077C`).
- Achieved 100% bit-for-bit identical state across all 3,600 frames between Run A (0x060286 OFF) and Run B (0x060286 ON):
  * `different_frame_count: 0`.
  * ROM_EXEC_060286: Run A = 3,486; Run B = 0 (100% displaced, 0 fallbacks, 445,912 stepped instructions).
  * NATIVE_EXEC_060286: Run A = 0; Run B = 3,486.
  * Matching SHA256 hashes across `observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, and `call-z80-ram.bin`.

# M15.22 Execution-Driven Recompilation: 0x060000 Dual Replay Verification & Displacement — DONE

Delivered:
- Recompiled runtime-discovered target `0x060000` (Ancient Music Driver Frame Audio/Graphics Coordinator).
- Verified caller ABI (`0x002380`: 3,486 calls) called in main gameplay loop once per active frame.
- Resolved coordinator semantics: orchestrates native child subsystems `0x061934` (20,916 calls), `0x0623AC` (13,944 calls), `0x062AE0` (3,486 calls), and `0x060286` (3,486 calls).
- Achieved 100% bit-for-bit identical state across all 3,600 frames between Run A (0x060000 OFF) and Run B (0x060000 ON):
  * `different_frame_count: 0`.
  * ROM_EXEC_060000: Run A = 3,486; Run B = 0 (100% displaced, 0 fallbacks, 186,101 stepped instructions).
  * NATIVE_EXEC_060000: Run A = 0; Run B = 3,486.
  * Matching SHA256 hashes across all 6 diagnostic sync streams.

# M15.21 Execution-Driven Recompilation: 0x062788 Dual Replay Verification & Displacement — DONE

Delivered:
- Recompiled runtime-discovered target `0x062788` (ADSR Sound Envelope Generator & Tremolo Modulator).
- Verified caller ABI (`0x062556`: 9,287 calls) directly connected inside parent `0x0623AC`.
- Resolved hardware boundary: pure CPU ADSR envelope calculation, master volume scaling (`mulu.w`), and tremolo modulation, with 0 direct MMIO accesses; invokes already-native child `0x06273A` at `0x062862` (9,287 hits) which performs the single write to SN76489 PSG port `0x00C00011`.
- Preserved internal tail path `0x06281A..0x06286C` for sibling caller `0x06243C` (985 calls) inside `0x0623AC`.
- Achieved 100% bit-for-bit identical state across all 3,600 frames between Run A (0x062788 OFF) and Run B (0x062788 ON):
  * `different_frame_count: 0`.
  * ROM_EXEC_062788: Run A = 9,287; Run B = 0 (100% displaced, 0 fallbacks, 248,168 stepped instructions).
  * NATIVE_EXEC_062788: Run A = 0; Run B = 9,287.
  * Matching SHA256 hashes across `observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, and `call-z80-ram.bin`.
- Kept 0x062788 active in native island chain (`step:613b2,61e48,61934,6147e,623ac,6273a,62788`) and ranked fresh call census for next target: `0x060000` (3,486 calls, highest-frequency untranslated target in canonical gameplay, called at `0x002380` in main game loop).

# M15.20 Execution-Driven Recompilation: 0x06273A Dual Replay Verification & Displacement — DONE

Delivered:
- Recompiled runtime-discovered target `0x06273A` (SN76489 PSG Channel Attenuation / Volume Synthesizer).
- Verified both caller ABIs (`0x062862`: 11,194 calls; `0x062620`: 109 calls) directly connected inside `0x0623AC` (11,303 total calls across 3,600 frames). Whitelisted static callers: `0x062862`, `0x062620`, `0x062430`, `0x062572`.
- Resolved hardware boundary: exactly 1 byte written to SN76489 PSG data port `0x00C00011` via `A3` (`move.b d0, (a3)`). 0 VDP Data/Control accesses, 0 DMA triggers.
- Reconstructed distance attenuation arithmetic (`d1 = distance >> 3`, `d0 = min(15, base_volume + d1) | 0x10 | (d7 & 0xF0)`), mute path (`d7 | 0x1F`), and dual RTS exits (`0x06277C`, `0x062786`).
- Achieved 100% bit-for-bit identical state across all 3,600 frames between Run A (0x06273A OFF) and Run B (0x06273A ON):
  * `different_frame_count: 0`.
  * ROM_EXEC_06273A: Run A = 11,303; Run B = 0 (100% displaced, 0 fallbacks, 130,241 stepped instructions).
  * NATIVE_EXEC_06273A: Run A = 0; Run B = 11,303.
  * Matching SHA256 hashes across `observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, and `call-z80-ram.bin`.
- Kept 0x06273A active in native island chain (`step:613b2,61e48,61934,6147e,623ac,6273a`) and ranked fresh call census for next target: `0x062788` (9,287 calls, highest-frequency untranslated target in canonical gameplay).

# M15.19 Execution-Driven Recompilation: 0x0623AC Dual Replay Verification & Displacement — DONE

Delivered:
- Recompiled runtime-discovered target `0x0623AC` (Entity Script Interpreter & Sprite Animator).
- Verified all 4 caller ABIs (`0x060234`, `0x060242`, `0x060250`, `0x060276`), each invoking `0x0623AC` 3,486 times for 13,944 total calls.
- Resolved hardware boundary: pure CPU script interpretation and animation timer updates, with SN76489 PSG audio commands emitted to `0x00C00011` (0 VDP port writes).
- Solved dynamic cycle model and refresh order: `api.add_cycles` before `api.skip_bus_refresh()` on register shifts (`asr.w`, `lsr.w`) and MOVEM, interleaved mem-to-mem operand fetches for `0x06138E`, and MULU dynamic cycles.
- Achieved 100% bit-for-bit identical state across all 3,600 frames between Run A (0x0623AC OFF) and Run B (0x0623AC ON):
  * `different_frame_count: 0`.
  * ROM_EXEC_0623AC: Run A = 13,944; Run B = 0 (100% displaced, 0 fallbacks, 916,314 stepped instructions).
  * NATIVE_EXEC_0623AC: Run A = 0; Run B = 13,944.
  * Matching SHA256 hashes across `observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, and `call-z80-ram.bin`.
- Kept 0x0623AC active and ranked fresh call census for next target: `0x06273A` (11,303 calls, directly called inside 0x0623AC).

# M15.18 Execution-Driven Recompilation: 0x06147E Dual Replay Verification & Displacement — DONE

Delivered:
- Recompiled runtime-discovered target `0x06147E` (Entity Coordinate Screen Projection & Auxiliary Component Driver).
- Verified single caller `0x061AC6` (16,673 calls in 3,600 frames).
- Fully displaced ROM execution (16,673 native calls, 0 ROM fallbacks, 133,384 stepped instructions).
- Bit-identical across 3,600 frames (`different_frame_count: 0`).

# M15.17 Execution-Driven Recompilation: 0x061934 Dual Replay Verification & Displacement — DONE

Delivered:
- Recompiled runtime-discovered target `0x061934` (Display List Priority Coordinator & Sprite Tier Driver).
- Proven 6 caller ABIs (`0x0601EE`, `0x0601FA`, `0x060206`, `0x060212`, `0x06021E`, `0x060260`), each invoking `0x061934` 3,486 times for 20,916 total calls.
- Connected native calls from `0x061934` to child routines: `0x061E48` (16,693 hits) and `0x0613B2` (16,673 hits).
- Isolated and resolved frame 426 7-byte divergence: fixed `0x0619E2` opcode `0x0882 0007` from `btst` to `bclr #7, d2`, ensuring D2 bit 7 is cleared when storing into entity record offsets 6(A6) and 7(A6).
- Achieved 100% bit-for-bit identical state across all 3,600 frames between Run A (0x061934 OFF) and Run B (0x061934 ON):
  * `different_frame_count: 0`.
  * ROM_EXEC_061934: Run A = 20,916; Run B = 0 (100% displaced, 0 fallbacks, 1,134,188 stepped instructions).
  * NATIVE_EXEC_061934: Run A = 0; Run B = 20,916.
  * Matching SHA256 hashes across `observed-landmarks.jsonl`, `full-frame-state.jsonl`, `architectural-sync.jsonl`, `scheduler-sync.jsonl`, `z80-ram-sync.bin`, and `call-z80-ram.bin`.
- Kept 0x061934 active and ranked fresh call census for next target: `0x06147E` (16,673 calls, called inside 0x061934 at 0x061AC6).

# M15.16 Execution-Driven Recompilation: 0x061E48 Dual Replay Verification & Displacement — DONE

Delivered:
- Recompiled runtime-discovered target `0x061E48` (Sprite Tile Allocation / Sub-Sprite Animation Cycling Coordinator).
- Verified single caller `0x061964` (16,693 calls in 3,600 frames).
- Fully displaced ROM execution (16,693 native calls, 0 ROM fallbacks).
- Bit-identical across 3,600 frames (`different_frame_count: 0`).

# M15.15 Execution-Driven Hybrid Recompilation Strategy & Player Dispatcher Registry — DONE

Delivered:
- Executed strategy change: emulator remains the authoritative fallback executor for untranslated M68K code, while verified C++ replacements progressively displace ROM execution.
- Canonicalized player ABI at runtime: resolved player record base discrepancy to `PLAYER_A6_RUNTIME_BASE = 0x00FF19E8` (disproving audit assumption of `0x00FF1800`), verified `PLAYER_A5_RUNTIME_BASE = 0x0000AD56`, identified `0x004A20` as 0-hit sound init and `0x00557A` as canonical player update, and mapped all 18 exact player record offsets.
- Implemented central `HybridExecutionRegistry` with `NativeRegion` abstraction (`src/tools/hybrid/hybrid_registry.hpp/.cpp`).
- Restricted native player execution strictly to States 0, 10, 14, and 24, routing all unsupported states to fail-closed ROM fallback with full register and context logging.
- Implemented `FirstDivergenceOracle` (`src/tools/hybrid/first_divergence_oracle.hpp/.cpp`) supporting differential execution and halting at first meaningful divergence.
- Connected existing native player implementations (`try_move`, terrain gate, melee attack, secondary weapon/bow, hurt recovery), eliminating player-path provenance rejects for `0x00E268`, `0x00938E`, and `0x009BF2`.
- Added unit tests in `tests/hybrid_registry_test.cpp`, verified against vertical slice, and validated file limits.

# M15.14 Gameplay Slice Connected Recompilation Audit & Cluster Selection — DONE

Delivered:
- Audited the entire playable vertical slice across Screen 0x0100 and 0x0101 (movement, terrain collision, enemy AI, melee combat, loot drop & pickup, Dytto summon, Dytto Water Bubble command, flame extinguish interaction, Bow weapon cycling, Arrow projectile combat, and screen transitions).
- Reconciled execution ownership between the pure native vertical slice (100% C++20 with ROM `DATA_ONLY` reads) and the hybrid M68K replay engine (8 frozen `HYBRID_STEPPED` helpers, 410,911 `ROM_FALLBACK` calls, remaining ~90% `M68K_ROM`).
- Collected 3,600-frame baseline M68K metrics: 456,879 target entries (45,968 native, 410,911 fallbacks), ~28.45M instructions, 1,420 unique PCs.
- Mapped native-to-ROM boundary graph and evaluated the top 10 bottlenecks (`0xE268`, `0x4A20`, `0x938E`, `0xBC8E`, `0x9BF2`, `0xE45E`, `0x9C7C`, `0xA342`, `0xB730`, `0x99D6`).
- Selected the next connected recompilation cluster: **Player State & Action Dispatcher (`0x004A20..0x004B80`)**, ~160 M68K instructions.
- Delivered translation plan closing entry (`0x004A20`), exits, RAM dependencies, fail-closed boundaries, and projected reduction (>220,000 boundary crossings eliminated, >5.5M instructions shifted native).
- Comprehensive report recorded in `docs/reports/THOR_M15_GAMEPLAY_SLICE_CONNECTED_AUDIT_20261014.md`.

# M15.13 Native Controlled Screen Secondary Weapon Selection + Use (Bow) Pipeline — DONE

Delivered:
- Reverse-engineered authentic M68K secondary weapon inventory, selection, and usage architecture:
  * Weapon inventory table in RAM: `0x00FF0D80..0x00FF0D90` (8 slots, stride 2 bytes: `byte 0` = weapon ID, `byte 1` = ammo/uses; `0xFF` = infinite/unlimited).
  * Weapon string & name table: ROM `0x05CC16` (0: None, 1: Knife, 2: Sword, 3: Broadsword, 4: Death Sword, 5: Omega Sword, 6: Bow, 7: Fire Bow, 8: Metal Bow, 9: Bomb, 10: Power Bomb, 11: Hyper Bomb, 12: Atomic Bow).
  * Selected weapon slot index: `0x00FF0DB0` (0..7), active weapon ID at `0x00FF0DB1`, ammo cache at `0x00FF198E`.
  * Weapon select cycle routine: `0x007EC4..0x007F10`. Traverses owned slots, skips unowned (`id == 0`), wraps across the 8 slots, and synchronizes selection index, active weapon ID, and ammo count.
  * Secondary attack state: State 14 (`0x005C4C` & `0x0071CC..0x007280`). Button::C gates on ammo > 0 and transitions player movement state to State 14, timer 0, locking directional movement.
  * Frame progression: 16-frame sequence comprising Startup (frames 0..11) -> Fire at frame 12 (`0x007260..0x00728E`) -> Recovery (frames 13..15) -> Normal (frame 16, resets state to 0 and unlocks movement).
  * Arrow entity spawn: `0x00F258..0x00F350`. Allocates projectile slot, sets Type `0x0034`, initial offset by facing direction `(dx, dy)`, cardinal speed 8.0 px/frame (`0x80000`), lifetime 30 ticks, bounding box `[-6, 6] x [-6, 6]`.
  * Sprite frame: ROM `0x154C22` (multi-tile arrow sprite pieces).
  * Ammo decrement: `0x008530..0x008580`. Decrements slot ammo by 1 exactly once at frame 12. If ammo drops to 0, clears the inventory slot (`clr.w 0(a0, d0.w)`) and reverts active weapon to Knife in slot 0 (`move.b #1, $ff0db1`).
  * Arrow collision & damage: `0x010A74` and `0x01BE30..0x01BEE0`. Arrow overlapping enemy Type `0x0048` inflicts 6 damage (`kArrowDamage`), records `last_weapon_hit = 0x0048`, triggers enemy hurt knockback along arrow direction, and consumes arrow entity. Arrow is also consumed on wall collision (`ByteGridView` wall cell `0x05`).
- Extended in-engine HUD diagnostics with all 7 requested fields:
  * `SECONDARY_WEAPON`, `SECONDARY_AMMO`, `WEAPON_STATE`, `WEAPON_TIMER`, `PROJECTILE_TYPE`, `PROJECTILE_X/Y`, and `LAST_WEAPON_HIT`.
- Added 10 deterministic unit tests in `tests/secondary_weapon_test.cpp`:
  * `WEAPON_SELECTION`, `UNOWNED_WEAPON_SKIPPED`, `NO_AMMO_REJECTED`, `USE_INPUT_STARTS_STATE`, `PROJECTILE_SPAWN`, `SPAWN_OFFSET_BY_FACING`, `AMMO_DECREMENT_ONCE`, `PROJECTILE_HIT`, `PROJECTILE_REMOVAL`, `RETURN_TO_NORMAL`.
- Added Section 19 integrated weapon selection, firing, enemy damage, and return-to-normal tests on `ControlledScreen` in `tests/native_vertical_slice_test.cpp`.

# M15.12 Native Controlled Screen Dytto Water Bubble → Flame / Environment Interaction Pipeline — DONE

Delivered:
- Reverse-engineered authentic M68K environmental interaction pipeline:
  * Environmental target selected: Flame / Fire Entity (Type `0x004C`), active hazard on screen `{1, 1}` at `(192, 160)`.
  * Collision volume: Bounding box `[-9, 9] x [-9, 9]` checked against Water Bubble projectile (Type `0x0018`, State 16 `kSpiritStateBubbleFly`).
  * Water collision discriminator: `0x011EFC..0x011F02` distinguishes enemy (`0x0048`) from environmental target (`0x004C`).
  * Flame substate trigger: `0x011F58` sets flame substate `$66(A0) = 0xFF` and records `target_entity_hit = 0x004C`.
  * Bubble reaction: Water bubble immediately transitions to State 22 (`kSpiritStateBubblePop`), plays pop animation for 4 frames, and deactivates (`clr.w $0(a6)`).
  * Flame extinguish lifecycle: `0x016E80..0x016EEA` runs a 15-frame extinguish sequence where visibility flashes (`flame_visible_ = ((flame_timer_ & 1) == 0)`).
  * Slot deactivation: At frame 15, entity slot deactivates (`active = false`, `clr.w $0(a6)`).
  * Room persistence: `0x018030` sets persistent room event flag bit in `0x00FF0DBC` (`bset d0, ($ff0dbc).l`).
  * Room reload & transition persistence: Screen transition and reloading screen `{1, 1}` checks `is_persistent_flag_set()`; extinguished flames do not respawn.
  * Contact damage: Active flame inflicts 15 HP contact damage on player with bounding box `[-14, 14] x [-8, 8]`; extinguished flame inflicts zero damage.
- Extended in-engine HUD diagnostics with all 7 requested fields:
  * `ENV_TARGET_TYPE`, `ENV_TARGET_STATE`, `ENV_TARGET_X/Y`, `WATER_REACTIVE`, `LAST_ELEMENT_HIT`, `WORLD_FLAG`, `BUBBLE_STATE`.
- Added 8 deterministic tests in `tests/environment_interaction_test.cpp`:
  * `NON_WATER_TARGET_UNAFFECTED`, `WATER_COLLISION_DETECTED`, `WATER_INTERACTION_TRIGGERED`, `STATE_EXTINGUISHED`, `EXTINGUISH_VISUAL_TRIGGERED`, `WORLD_OR_TILE_MUTATION_APPLIED`, `EXTINGUISH_PERSISTED`, `NO_DOUBLE_TRIGGER`.
- Added Section 18 integration tests in `tests/native_vertical_slice_test.cpp`.

# M15.11 Native Controlled Screen Dytto Active Command (Water Bubble) Pipeline — DONE

Delivered:
- Reverse-engineered authentic M68K spirit command input decoding & dispatch:
  * Command input decoder: `0x011A4C..0x011B26` evaluates Button::A (`0x40` in `$FF165E`). Fresh press and release before 20 frames starts a 6-frame double-tap window (`$FF19A1`). On window expiration without a second press, emits Command 1 (Single Tap) in `$FF19A2`.
  * Command dispatch: `0x0121B4..0x0121CE` checks `$FF19A2`. Command 1 (`0x011D8A`) triggers Dytto's Water Bubble ability.
  * SP consumption gate: `0x011B28` & `0x011D8A` deducts 5 SP from player (`+$8E(A5)`). If SP < 5, command is rejected and spirit remains in follow state.
  * Spirit state transition: Dytto transitions to State 14 (`+$04 = 14`), animation selector 6 (`+$2A = 6`, cast animation), setting command timer to 24 frames (`0x011DA8..0x011DAE`).
  * Water Bubble entity spawn: `0x011E04..0x011EAC`. Spawns projectile entity (Type `0x0018`, `$A2 = 0xFF`), GFX descriptor `0x141374`, Frame 0 at `0x141412` (1 piece, 2x2 cells = 16x16 px, base tile 248, pal line 3). Initial position `(spirit.x, spirit.y, spirit.z + 10)`.
  * Velocity & lifetime: Cardinal velocity 4.0 px/frame along player direction. Maximum lifetime 24 frames (`0x18` frames at `0x011F6A`).
  * Return to follow: After 24 frames of cast, Dytto returns to State 2 (Follow/Idle) with hover animation selector 4 (`0x011DE4`).
  * Projectile collision & effect: `0x011ED2..0x011F88` checks bounding box `[-9, 9] x [-9, 9]`. When projectile overlaps enemy (Type `0x0048`), deals damage / stun (`0x011F2A..0x011F46`), and transitions to State 22 (`0x16`, Pop) with anim 36 for 4 frames before slot deactivation (`0x011FA0: clr.w $0(a6)`).
  * Room transition safety: Screen transitions cancel active projectiles and reset Dytto to State 2 without leaking slots.
- Extended in-engine HUD diagnostics with all 7 requested fields:
  * `SPIRIT_CMD`, `SPIRIT_CMD_STATE`, `SPIRIT_CMD_TIMER`, `SPIRIT_SP_COST`, `SPIRIT_EFFECT_TYPE`, `SPIRIT_EFFECT_X/Y`, and `SPIRIT_TARGET`.
- Added 10 deterministic unit tests in `tests/spirit_slots_test.cpp` and Section 17 integration tests in `tests/native_vertical_slice_test.cpp`.

# M15.10 Native Controlled Screen Authentic Spirit Summoning Pipeline — DONE

Delivered:
- Reverse-engineered authentic M68K spirit summoning pipeline:
  * Spirit acquisition & slot flags: Event `0x16` maps to bit 0 of `0x00FF0DBA` (Dytto / Water Spirit, Type `0x0018`).
  * Spirit slot RAM layout: Singleton spirit slot at `0x00FF1AA4` (`stride 0xBC` after player `0x00FF19E8`), owner link `0x00FF19E8` in register `a5`.
  * Summon command & gate: Button::A (`btst #6, $03(a5)`) checks slot 0 acquired in `0x00FF0DBA` and player SP > 0 (`+$8E(A5)`). Duplicate summon rejected while spirit slot is already active (`+$00(0x00FF1AA4) != 0`).
  * Spawn routine: `0x014668..0x0146BE` and `0x018CDE..0x018D4C`. Elevates initial position $z = z + 16$ (`0x018D08: addi.l #$100000, d2`), sets state `+$04 = 26` (`0x1A`), animation selector 34 (`0x22`) (Appear sequence), sets active flag `bset.b #0, $ff0d7a.l`.
  * Entity type record: Type `0x0018` (Row 11 at ROM `0x03F35A` / `0x03F45A`). Half-extents `dim_x = 2, dim_y = 2`.
  * GFX & animation descriptor: GFX descriptor `0x11F360`, Frame 0 at `0x11F506` (3 pieces, base tile 773, palette line 3). Animation descriptor `0x0405E2`.
  * State machine: State 26 (Appear) runs 20 frames, transitions to State 2 (Follow/Idle) with anim 4 (hover).
  * Follow & leash tracking: `0x01215E` & `0x0123B4`: maintains 48 px elevation offset above player, computes directional delta towards `(player.x, player.y - 16)`, updates facing direction (0..3), and steps towards player within leash bounds.
  * SP magic resource consumption: `0x011B64..0x011B86`: drains 1 SP every 45 frames (`0x011B72`). When SP reaches 0 (`0x011B7C`), triggers despawn (`0x012314`) and clears slot `clr.w $0(a6)`.
  * Screen transition persistence: `0x03A464..0x03A47E`: spirit persists across screen transitions and teleports to destination player coordinates `(player.x, player.y, player.z + 16)`.
- Extended in-engine HUD diagnostics with all 8 requested spirit fields:
  * `SPIRIT_ACTIVE`, `SPIRIT_TYPE`, `SPIRIT_SLOT`, `SPIRIT_STATE`, `SPIRIT_X/Y`, `SPIRIT_TARGET_X/Y`, `SPIRIT_RESOURCE`, and `SPIRIT_ANIM`.
- Added 10 deterministic tests in `tests/spirit_slots_test.cpp` and Section 16 integration tests in `tests/native_vertical_slice_test.cpp`.

# M15.9 Native Controlled Screen Enemy Death -> Loot Drop -> Player Pickup Pipeline — DONE

Delivered:
- Reverse-engineered authentic M68K enemy death, loot drop, and pickup pipeline:
  * Death completion routine: `0x017E82..0x017F6A`. On HP reaching 0, state transitions to State 2 (Death), animation 2, timer `$2(a6) = 16` ticks (`0x10`), invulnerability bit set (`bset #3, $38(a6)`). State 2 decrements timer each frame while flashing sprite visibility (`bchg #1, $36(a6)`). On timer 0: loads drop table `lea.l $17f6c(pc), a1`, calls drop routine `jsr $f476.l`, deactivates slot `clr.w $0(a6)`.
  * Drop table: ROM `0x017F6C` (8 bytes: `[0x0000, 0x1E01, 0x0000, 0x1E01]`). Selects item `0x1E` (cheese/food), count 1.
  * Drop decision & item spawn routine: `0x00F476..0x00F538`. Allocates entity slot from pool `$ff2abc` via `bsr.w $d9be`, sets type to Type `0x0008` via `bsr.w $8d06`, copies enemy position `(x, y, z + 1)` with elevation offset `addq.w #1, d0`, initial vertical velocity `vz = 0x80000`, sets item state `+$04 = 2` (Ground / active idle).
  * Item entity type: Type `0x0008` (Row 3 at ROM `0x03F35A`). Dimensions: `dim_x = 8, dim_y = 8`.
  * Item GFX descriptor: `0x155AF6`. Frame 0 piece descriptor at `0x155B76`: 1 piece, 3x3 tiles (`w=3, h=3`), offset `(0, -12)`, tile index 1268, palette line 3.
  * Item animation descriptor: `0x155D76`.
  * Pickup routine: `0x0082F8` / `0x00B852`.
  * Pickup hitbox semantics: Item AABB `[-4, 4] x [-4, 4]` against player footprint radius 10 (`[-10, 10] x [-10, 10]`), elevation gate `abs(player_z - item_z) <= 16`.
  * Item effect routine: `0x0053A2..0x0053E8`.
  * Effect result: Item `0x1E` restores `player.max_hp / 4` (25 HP for max 100), clamped to max HP: `player.hp = min(player.max_hp, player.hp + 25)`. Sets `last_pickup_type = 0x001E`, `last_pickup_heal = 25`, increments `item_count`.
  * One-time consumption: Item marked inactive (`collect()`, `active_ = false`) upon pickup, preventing repeat collection.
  * Persistence status: Screen-local. Screen transitions reload destination entities and discard temporary drops.
- Extended in-engine HUD diagnostics with all 7 requested fields:
  * `DROP_TYPE`, `DROP_STATE`, `DROP_X/Y`, `DROP_ACTIVE`, `PICKUP_TARGET`, `PLAYER_HP/CURRENCY/ITEM_COUNT`, and `LAST_PICKUP`.
- Added 10 deterministic tests in Section 15 of `tests/native_vertical_slice_test.cpp`.

# M15.8 Native Controlled Screen Player Melee Attack & Enemy Damage/Death Response — DONE

Delivered:
- Reverse-engineered authentic M68K player attack input path and state transition:
  * Input handler: `0x004A8E: btst.b #4, $03(a6)` detects Button::B (Attack).
  * State transition: transitions player movement state `+$04(A6)` to State 10 (Melee Attack), clears velocity deltas `(+$4E, +$52) = (0, 0)`, and resets attack timer `+$36(A6) = 0`.
  * Attack frame sequence: 9 frames total duration comprising 4 startup frames (0..3, `attack_active = false`), 2 active frames (4..5, `attack_active = true`), and 3 recovery frames (6..8, `attack_active = false`). Locks directional player movement during all 9 frames.
- Reconstructed player melee weapon hitbox from ROM table `0x058176` (Record 0, dagger/basic melee):
  * Directional bounding boxes relative to player origin `(world_x, world_y)`:
    - Dir 0 (Up): `[-8, 7] x [-40, -9]` (width 16, height 32, offset upward)
    - Dir 1 (Right): `[8, 47] x [-8, 7]` (width 40, height 16, offset rightward)
    - Dir 2 (Down): `[-8, 7] x [8, 47]` (width 16, height 40, offset downward)
    - Dir 3 (Left): `[-47, -8] x [-8, 7]` (width 40, height 16, offset leftward)
  * Hitbox active only during attack frames 4 and 5; returns `std::nullopt` during startup and recovery.
- Recompiled enemy damage receiver, hurt state, and death lifecycle for Type `0x0048`:
  * Hit-once-per-swing latch: `PlayerState::attack_hit_latched` (ROM `bit 3 of $37(a6)`) prevents multiple damage applications within a single swing.
  * Damage application: `ScreenEntity::apply_damage()` decrements enemy HP by 10 (Type 0x0048 HP: 20 -> 10 on first hit).
  * Hurt reaction: when HP > 0, actor enters State 4 (Hurt), setting hurt timer to 20, invulnerability timer to 20, knockback timer to 6, and knockback velocity to 3.0 px/frame (`0x30000`) along attack direction. Recovers to State 1 upon timer expiry.
  * Death transition: when HP <= 0 on second hit (10 -> 0), actor enters State 2 (Death), clearing active flag `clr.w $0(a6)` (`active_ = false`) to deactivate the slot and exclude it from collision and updates.
- Extended in-engine HUD diagnostics with all 8 requested combat fields:
  * `PLAYER_ATK_STATE`, `PLAYER_ATK_TIMER`, `PLAYER_ATK_ACTIVE`, `PLAYER_ATK_BOX`, `ENEMY_HP`, `ENEMY_HURT`, `ENEMY_STATE`, and `LAST_HIT_DAMAGE`.
- Added 11 deterministic unit test cases and an integrated screen combat test in Section 14 of `tests/native_vertical_slice_test.cpp`.

# M15.7 Native Controlled Screen Player ↔ Moving Enemy (Type 0x0048) Contact and Damage Pipeline — DONE

Delivered:
- Reverse-engineered authentic M68K combat and collision pipeline:
  * Entity-player collision loop: `0x00BC8E` -> `0x00BDE0` iterating entity pool against player `0xFF19E8`.
  * Invulnerability gate: `0x00BE4C: tst.b $82(a0); bne $be5c` blocks all damage while timer > 0.
  * Contact resolution: `0x00BEBA` / `0x00BED4` sets attacker pointer `+$7E(A0)`, invulnerability timer `+$82(A0) = 30` frames, damage code `+$86(A0)`, knockback timer `+$83(A0) = 6` frames, hit direction `+$84(A0) = attacker.direction`.
  * Knockback vector: `0x00BF2E` / `0x00E2D4` computes fixed-point velocity `0x30000` (3.0 px/frame cardinal) along hit direction into `(+$4E, +$52)`.
  * Player damage handler: `0x00557A` / `0x0057FA..0x005942` resolves raw damage (10 HP for Type 0x0048 from Record 0 byte +2 at ROM `0x058C82`), decrements HP at `+$8A`, sets movement state `+$04 = 24` (State 24: Hurt / Knockback).
  * State 24 update routine: `0x005CC8` decrements knockback timer `+$83`, displaces player by knockback velocity with terrain collision checks, and exits hurt state via `0x0062CC`, clearing velocity and restoring control to State 0 (`+$04 = 0`).
  * Invulnerability countdown: `0x00E682` decrements `+$82` each frame down to 0.
- Distinguished physical movement blocking from directional harmful attack collision:
  * Physical blocking uses `hitbox()` (`8x8`, half-extents 4x4) to prevent player overlap without damage.
  * Harmful attack uses `attack_hitbox()` (Record 0 at ROM `0x058C82`), active only in charge State 2.
- Extended `PlayerState` with: `hp (100)`, `max_hp (100)`, `invuln_timer (0)`, `hurt_timer (0)`, `knockback_x_fixed (0)`, `knockback_y_fixed (0)`, `contact_entity (0)`, and `damage_gate_open (false)`.
- Recompiled pure C++20 damage routines in `src/game/player/player.cpp`: `apply_enemy_contact_damage()` and `update_player_hurt_knockback()`.
- Grounded knockback in terrain collision grid via `world::evaluate_terrain_gate`.
- Enforced screen transition cleanup of combat and damage fields.
- Extended in-engine HUD diagnostics with the 7 required fields: `PLAYER_HP`, `PLAYER_STATE`, `INVULN`, `KNOCKBACK_X/Y`, `CONTACT_ENTITY`, `DAMAGE_GATE`, and `ACTOR_STATE`.
- Added comprehensive deterministic test coverage in Section 13 of `tests/native_vertical_slice_test.cpp`.

# M15.6 Native Controlled Screen Moving ROM-Backed Entity State Machine — DONE

Delivered:
- Reverse-engineered the authentic M68K enemy dispatch and state machine architecture:
  * Spawn and type-dispatch jump table at ROM `0x00DF54` (indexed via `0x00FDE2..0x00FE00` for type_id >= 0x16).
  * 4-state state machine at ROM `0x01778C..0x0178F0` for Type `0x0048` (Row 35 in table `0x0003F2FA`).
- Selected and integrated dynamic entity Type `0x0048`:
  * Dimensions: `dim_x = 8, dim_y = 8` (`hw = 4, hh = 4`).
  * GFX descriptor table: `0x165E12` (6 multi-tile sprite frames: `0x165E1E`, `0x165E2C`, `0x165E3A`, `0x165E48`, `0x165E56`, `0x165E64`).
  * Animation sequence table: `0x040F4A` (Idle Frame 0, walk sequence Frame 3).
  * State handler: `0x01778C`.
- Recompiled bounded 4-state state machine in pure C++20 (`ScreenEntity::update_actor_state_machine()`):
  * State 0: Anchor initialization `($a8 = $8, $aa = $c, anim_selector = 0, state = 1)`.
  * State 1: Wait / Line-of-sight detection using 4-directional table `0x1783E` (24 tiles = 192 px range) checked against player bounding box. On detection, sets fixed-point velocity, timer = 30, anim_selector = 2, state = 2.
  * State 2: Walk / Charge movement. Advances position by fixed-point velocity. Decrements timer. Evaluates terrain collision via `ByteGridView` and native gate `evaluate_terrain_gate` (0x00938E). When blocked or timer expires, toggles direction bit 1 (`bchg.b #1, $17(a6)`), anim_selector = 4, state = 3.
  * State 3: Return to anchor `($a8, $aa)`. Steps towards anchor. When distance reaches zero, restores original facing direction, anim_selector = 0 (Idle), state = 1.
- Built native `ActorRuntime` model in `src/game/entities/screen_entity.hpp`.
- Coupled sprite piece decoding and VRAM tile mapping directly to actor state transitions.
- Enforced fail-closed behavior on unsupported states (`actor_.halted = true`).
- Integrated dynamic entity Type `0x0048` at `(160, 120)` with direction 2 (Down) onto Screen 0x0100 alongside static chest Type `0x002A` at `(192, 160)`.
- Extended in-engine HUD diagnostics with live actor state values (`ACT_ST=... TMR=... DIR=...`).
- Added deterministic regression coverage in `tests/native_vertical_slice_test.cpp`.

# M15.5 Native Controlled Screen ROM-Backed Player ↔ Entity Interaction & Collision — DONE

Delivered:
- Reverse-engineered the authentic M68K bounding-box overlap test at ROM `0x009C7C..0x009CFE` and entity pool collision at `0x00BB18..0x00BBA6`.
- Established bounding box conventions: center-based world coordinates (`world_x, world_y, world_z`) and half-extents (`radius_x = dim_x / 2`, `radius_y = dim_y / 2`) derived from ROM table `0x0003F2FA` (chest Type `0x002A`: `dim_x = 24`, `dim_y = 20`, half-extents 12 and 10; player Type `0x0002`: radius 10).
- Introduced native `EntityHitbox` (`src/game/entities/screen_entity.hpp`) with `left, top, right, bottom, z_min, z_max` and `overlaps()` method.
- Integrated candidate entity collision into player movement pipeline: candidate movement -> terrain collision -> entity collision check -> accept/reject movement. Player cannot walk through physical obstacles (static chest blocking verified on all four sides and diagonal approach).
- Implemented interaction probe checking proximity (8 px expanded hitbox) and directional facing. When player faces an entity within range, candidate is acquired (`interaction_target_type = entity.type_id()`, `interaction_state = READY`).
- Wired action button inputs (`Button::A`, `Button::B`, `Button::C`) to trigger entity interaction: chest transitions to `TRIGGERED (OPENING)`, progresses through authentic ROM animation sequence (frame 0 -> 1 -> 2), and permanently halts at frame 2 in `OPENED` state without looping.
- Verified screen transition isolation: screen transitions clear `overlap_detected_`, `interaction_target_type_`, and reset `interaction_state_str_ = IDLE`. ScreenInstance entity ownership remains authoritative.
- Extended in-engine debug HUD to 24 lines, adding `ENTITY_HITBOX`, `PLAYER_HITBOX`, `OVERLAP`, `INTERACTION_TARGET`, and `INTERACTION_STATE`.
- Added comprehensive regression tests in `tests/native_vertical_slice_test.cpp`.

# M15.4 Native Controlled Screen Authentic ROM-Driven Entity Animation Playback — DONE

Delivered:
- Reverse-engineered the authentic M68K sprite animation playback driver at ROM `0x008FA4..0x009040` and descriptor table at `0x0003F2FA` +20 (`0x00041052` for chest Type `0x002A`).
- Reconstructed the 4-byte animation sequence record format `[Word 0: frame index, Word 1: raw duration & loop flag]`, with deterministic tick countdown `(raw_duration & 0x7FFF) >> 1` and loop/termination flag `0x8000`.
- Mapped sequence frame indices to Genesis sprite frame descriptors via table `0x00167DAC` with authentic relative word offset indexing:
  * Frame 0 (closed): index `0`, duration `4` ticks, address `0x00167DB2`.
  * Frame 1 (opening): index `1`, duration `4` ticks, address `0x00167DE4`.
  * Frame 2 (open): index `2`, duration `4` ticks with loop flag `0x8008`, address `0x00167E16`.
- Implemented `AnimationState` and fail-closed animation player in `ScreenEntity::update(rom)`.
- Verified screen transition animation safety: entities on newly loaded screens instantiate fresh animation state without cross-screen leakage or stale script pointers.
- Extended in-engine HUD to 19 lines, adding `ENTITY_ANIM_PTR`, `ENTITY_ANIM_CURSOR`, `ENTITY_FRAME_PTR`, `ENTITY_TICKS`, and `ENTITY_ANIM_STATE`.
- Added comprehensive regression coverage in `tests/native_vertical_slice_test.cpp`.

# M15.3 Native Controlled Screen ROM-Backed Entity Spawning and Multi-Tile Sprite Rendering — DONE

Delivered:
- Spawned real non-player entities from the canonical Beyond Oasis ROM onto the active 0x0100 / 0x0101 vertical slice.
- Identified and decoded the ROM entity type architecture:
  * Entity allocator routine at ROM `0x00DA2A` (17 general pool slots at RAM `0x00FF1CD8`, stride `0xBC`).
  * Entity initializer routine at ROM `0x008D06` (populates dimensions, hitboxes, graphics descriptor `+0x1A`, animation descriptor `+0x1E`, callback `+0x22`).
  * Entity type table at ROM `0x0003F2FA` (99 entries, 32-byte stride, indexed by `type / 2 - 1`).
- Implemented `ScreenEntity`, `EntityDescriptor`, and `SpritePiece` in `src/game/entities/screen_entity.hpp`/`.cpp`.
- Integrated multi-tile sprite rasterization into `ControlledScreen::render()`:
  * Decoded multi-piece multi-tile layout from ROM frame descriptors (such as `0x167DAC` for Type `0x002A`).
  * Mapped entity world coordinates through camera transform to screen coordinates.
  * Rendered column-major 8x8 tiles with authentic palette and transparency.
  * Preserved fixed world coordinates under camera movement (`ENTITY_SURVIVES_CAMERA_MOVEMENT = YES`).
- Bound entity lifetime to `ScreenInstance`:
  * Screen 0x0100 spawns authentic Type `0x002A` (chest) at `(192, 160)`.
  * Screen 0x0101 spawns two Type `0x002A` entities at `(224, 144)` and `(160, 200)`.
  * Transitions clean up previous entities and reload new screen entities without leaks or stale objects (`ENTITY_LIFETIME_MATCHES_SCREEN = YES`).
- Extended 14-line in-engine debug overlay with `ENTITY_COUNT`, `ENTITY_ID/TYPE`, `ENTITY_WORLD_X/Y`, `ENTITY_FRAME`, `ENTITY_TILE_BASE`, `ENTITY_SCREEN_LOCAL`.
- Added comprehensive regression tests in `tests/native_vertical_slice_test.cpp`.

# M15.2 Native Controlled Screen Multi-Screen Transition (0x0100 <-> 0x0101) — DONE

Delivered:
- Multi-screen runtime transition between canonical ROM Screen 0x0100 and adjacent Screen 0x0101 without process restart.
- Introduced `ScreenInstance` architecture (`src/game/screen_instance.hpp`/`.cpp`) cleanly decoupling screen data lifecycle and avoiding dangling `ByteGridView` pointers.
- Integrated Screen 0x0101 (`Group: 1, Index: 1`):
  * Descriptor: ROM `0x02E612`
  * Primary stream: `0x212684` (1,590 compressed -> 12,460 decompressed bytes)
  * GFX resources: `[13, 14, 15, 16]` (512 tiles)
  * 64x64 collision grid: 4,096 bytes with 690 differing cells vs 0x0100
  * 64x28 tilemap: 1,792 words with 41 differing words vs 0x0100
- Differential collision verification proving Screen B's byte grid is immediately active and Screen A's grid is inactive.
- Supported deterministic player coordinate handoff and bounded camera repositioning.
- Implemented edge-boundary triggering and interactive debug key triggering (`Button::X` / `'T'`).
- Extended 8-line in-engine HUD with `CURRENT_SCREEN_SETUP_ID`, `PREVIOUS_SCREEN_SETUP_ID`, `TRANSITION_COUNT`, `PRIMARY_STREAM`, `GRID_BASE/OFFSET`, and `PLAYER_CELL`.

# M15 Native Controlled Screen Authentic ROM Screen 0x0100 Rendering — REAL_ROM_SCREEN_RENDERED = YES

Delivered:
- Replaced synthetic background in `ControlledScreen` with real ROM-backed game screen rendering when canonical ROM is provided.
- Screen Setup `0x0100` (`Group: 1, Index: 0`) integrated end-to-end:
  * Descriptor: ROM `0x02E57E`
  * Primary resource: `STREAM_211F7A` (ROM `[0x211F7A, 0x212683)`, decompressed into 12,460 bytes)
  * GFX resources: `[13, 14, 15, 16]` loaded and decompressed into 512 Genesis 4bpp 8x8 tiles
  * Palette: CRAM palettes 1 and 2 loaded from stream offset `0x3004..0x3044`
  * Tilemap: 64x28 Genesis VDP attribute words loaded from stream offset `0x1204..0x2004`
  * Collision: 64x64 byte grid bound to `world::ByteGridView` from stream offset `0x204..0x1204`
- Native player controllable on authentic terrain with real collision constraints (`TEST_SPAWN (160, 160)`).
- 8-line in-engine diagnostic overlay displaying screen identity, descriptor/resource addresses, player/grid/camera coordinates, toggleable via `Button::Mode` (`Tab` / `'M'`).
- Preserved 100% backward compatibility for synthetic test fixture and replay hash when no ROM is provided.

# M12 Controlled Entity / Player Provenance V1 — STOP

The automatic `CONTROLLED ENTITY PROVENANCE` stage is implemented after
`GAMEPLAY RAM / ENTITY CANDIDATES` and before `ASM CLOSURE`. The preserved real
corpus selected one exact candidate and confirmed controller-port/input-RAM
observations, but did not close an exact same-control-flow input→selected-entity
mutation. The stage therefore remains `STOP` with `ENTITY_ROLE =
ENTITY_CANDIDATE`, `PLAYER_LABEL_PROVEN = NO`, and SOURCE_OWNED delta 0. The
next step is a new bounded evidence capture that carries controller causality
through the selected entity record; do not promote or label the candidate before
that proof.

# M12 Worker W6 — PASS_LONG_LIVE_GAME_DISCOVERY_RUN_V1

Sustained >=10 minute (35,677 frames, 610.5s) live game discovery run executed in
BizHawk/Waterbox with 128 Workers at depth 512 against canonical Beyond Oasis ROM.
Natural controller input navigated past early boot into active in-game gameplay.
Zero health failures (retention failures = 0, dropped = 0, collisions = 0, OOM = 0)
and clean `STOPPED_END_GAME` shutdown achieved across 106 non-truncated wave chunks
and 51,233,653 raw records.

Post-run discovery analysis establishes extensive novelty against current Thor Brain:
4,261 new M68K execution PCs, 284 new Z80 execution PCs, 338 new ROM physical ranges,
35 new banked ROM ranges, 455 cross-CPU handoff addresses, 2,324 runtime hits on
known `AUDIO_FORMAT_A_MODE0` Resource 1 (`0x0BD540..0x0BF768`), and 288,650 new
cross-CPU causal chains. All 13 canonical discovery artifacts are emitted in
`build/m12-w6-live-discovery/`. Strict SOURCE_OWNED preservation enforced
(`SOURCE_OWNED_DELTA = 0`, value remains 1,487,388).

# M12 Worker W5 — PASS_EXACT_AUDIO_RESOURCE_ROUNDTRIP_V1

Exact audio resource format `AUDIO_FORMAT_A`, boundary closure, canonical IR, and
byte-identical round-trip are fully closed and verified end-to-end starting from the
W4 early-boot DAC witness (`0x0BEDE4`).

The exact Z80 sound driver routines (descriptor parse at `0x0704`, 9-bit bank latch at `0x0A0B`,
sample loops at `0x080D`/`0x0854`, pointer advancement at `0x0880`, and DAC emission at `0x0968`)
and bank descriptor tables (16 5-byte entries at `0x8000` of each 32KB audio bank) are verified.

Primary resource `AUDIO_RESOURCE_FORMAT_A_0001` (ROM `0x0BD540..0x0BF768`, 8,744 bytes) strictly
encloses the W4 DAC witness cluster, with decoded PCM sample at offset 6,308 matching the observed
runtime DAC write `0x78` exactly. Secondary resource `AUDIO_RESOURCE_FORMAT_A_0002` (ROM `0x0BC95C..0x0BD540`,
3,044 bytes) and resources across multiple audio banks achieve 100% byte-identical round-trip
via pure deterministic decoder and encoder modules without reading or storing original binary blobs.
All 9 acceptance artifacts are emitted in `build/m12-w5-acceptance/`. All 18 tests pass green.

# M12 Worker W4 — PASS_ACTIVE_AUDIO_RESOURCE_ANALYSIS_V1

Active audio resource analysis V1 connects M68K sound dispatch commands to Z80
driver processing, Z80 RAM state, and hardware audio sinks (YM2612 and PSG).
Port-local address latch pairing (0x4000->0x4001, 0x4002->0x4003) is strictly
enforced without cross-port leakage. DAC data (reg 0x2A) and enable (reg 0x2B)
registers are identified as exact hardware facts.

The 4 distinct causal levels (Level 0 Temporal, Level 1 Memory last-writer,
Level 2 Dataflow provenance, Level 3 Strict Causal Chain) are fully validated.
Over 7,168 M68K->Z80 handoffs are verified with zero intervening writes.
2,048 YM2612 register writes reach Level 3 Strict Causal Chain with exact Z80
dataflow provenance. All candidate resources follow strict neutral types
(AUDIO_COMMAND_CANDIDATE, AUDIO_TABLE_CANDIDATE, AUDIO_PATCH_CANDIDATE,
AUDIO_RESOURCE_RANGE_CANDIDATE) with zero promotional song/instrument labels.
The 9 canonical acceptance artifacts are emitted in `build/m12-w4-acceptance/`.

# M12 Worker W3 — PASS_Z80_COCAPTURE_CROSS_CPU_TIMELINE_V1

Z80 execution and bus evidence are now co-captured in the Worker evidence stream
on a common master_time basis. Clean V2 48-byte records preserve independent CPU
instruction sequences, opcode bytes (up to 4-byte prefixed forms), logical and
resolved physical addresses for banked ROM, ordered bank register changes, YM2612
and PSG writes, and cross-CPU causal witness chains. Zero song or instrument
semantics are introduced.

Large bounded ring capacity (2,097,152 slots / 96 MiB raw / 128 MiB slot storage)
is deployed with O(1) bitmask hot-path indexing `(sequence - 1u) & (cap - 1u)`.
Official 128 Workers x Depth 100 acceptance campaign (12,800 segments) and
deep-chain stress tests (Depth 512 with 82.1M records, Depth 1000 with 138.2M records)
are 100% verified with 0 retention failures, 0 overwrites, and 0 invalid captures.
Next audio milestone will address container-level track/voice isolation.

# M12 VDP S8 — PASS_ONE_PROVEN_LOGICAL_VDP_FRAME_V1

Frame 779 now has a bounded, coherent VDP snapshot and deterministic logical
composition for Plane A, Plane B, Window, backdrop, and the accepted S7 sprite
raster. Raw register events, full VRAM/CRAM/VSRAM, name-table words, tile bytes,
scroll state, and pixel provenance are retained in ignored runtime artifacts.
This is not a full-screen/timing emulator equivalence claim. Next graphics work
is frame-set/animation grouping; object/entity association and PNG publication
remain separate milestones. Audio remains at container ownership without song
or instrument semantics.

# M12 Sprite S7 — PASS_SPRITE_SCANLINE_RASTER_RULES_V1

The unchanged S6 logical sprite frame now passes through a developer-only
Genesis VDP scanline evaluator. Runtime mode evidence selects H40/V28 for
frame 779 and derives 20-sprite/320-dot limits. SAT-link order is preserved;
transparent and X-offscreen pieces consume evaluation width, X=0 masking is
recorded, and a final sprite can be partially accepted when only part of the
dot budget remains. Frame 779 does not exercise overflow, so its runtime truth
class remains `OBSERVED_RUNTIME_NO_OVERFLOW`; overflow behavior is
`STATIC_VERIFIED_TEST_VERIFIED` only. Next graphics work is animation/frame
set grouping, while backgrounds remain outside this boundary.

# M12 Sprite S6 — PASS_ONE_PROVEN_HARDWARE_SPRITE_FRAME_V1

The S5 catalog now closes one complete active SAT chain into a deterministic
sprite-only frame artifact. Frame 779 traverses eight PROVEN entries in exact
hardware link order and retains raw SAT/shadow/tile/CRAM evidence plus
pixel-level provenance. Composition uses transparent pixels and earlier-SAT
wins overlap semantics while retaining explicit limits for producer-to-DMA
causality, background priority/layers, and scanline behavior. The next
graphics step is grouping proven frames into an animation set; object/entity
association remains unproven.

# M12 Sprite S5 — PASS_SAT_PERSISTENCE_CAUSALITY_V1

The remaining S4 observations now inherit publication provenance only through
complete SAT/shadow mutation watches, unchanged bytes, stable SAT base, and
entry-specific frame coherence. Direct and persisted publication remain
distinct. Producer-to-DMA causality is retained as unproven where ordered
evidence is absent. Next graphics work is frame and animation grouping.

# M12 Sprite S4 — PASS_SPRITE_EVIDENCE_GAP_CLOSURE_V1

Measured S3 gaps showed that tile and CRAM evidence already existed, while
publication evidence was filtered to SAT base `0xD000`. The bounded capture
now records DMA segments across the SAT window and closes that measured gap
for 183 observations. Remaining 140 observations require a separate
same-frame persistence/causality mechanism and remain explicitly incomplete.
Next graphics work is frame and animation grouping.

# M12 Sprite S3 — PASS_HARDWARE_SPRITE_PIECE_CATALOG_V1

The S2 witness machinery now catalogs frame/SAT-entry observations without
deduplicating temporal evidence. Multi-cell pieces use corrected column-major
Genesis tile addressing; each entry retains tile bytes, CRAM, decoded pixels,
transparency, palette indices, priority limits, and a semantic fingerprint.
Missing publication or asset evidence stays incomplete or linkage-only, and
conflicts fail closed. Next graphics work is frame/animation grouping.

# M12 Sprite S2 — PASS_ONE_HARDWARE_SPRITE_PIECE_V1

One runtime-backed, same-frame one-cell sprite piece is now proven from the
observed producer/shadow SAT through VRAM SAT, exact tile bytes, CRAM bytes,
the corrected Genesis decoder, and structured pixels. The proof is bounded to
the selected frame and does not claim producer-to-DMA causality beyond the
retained callback sequence. Next graphics work is animation/frame grouping;
audio remains at container ownership with no song or instrument semantics.

# M12 Archivist → Canonical Knowledge Pipeline 2G — PASS_ARCHIVIST_CANONICAL_KNOWLEDGE_PIPELINE_V1

The post-run pipeline now carries a closed exact-ROM Cartographer MAP-1
session through Archivist merge, delta canonical import, atomic paired map
publication, independent audit, and a compact receipt. A bounded canonical
BizHawk/GPGX campaign completed 100 repeated captures for each of 16 Workers
(1,600/1,600 segments audited). Idempotent replay, stable identities,
evidence accumulation, fail-closed unsupported facts, and unchanged
`SOURCE_OWNED` (1,475,600 bytes; delta 0) and emission are verified. Details
are in `docs/reports/THOR_M12_ARCHIVIST_KNOWLEDGE_PIPELINE_2G.md` and its JSON
receipt. This 2G checkpoint is complete; the broader M12 reconstruction
milestone remains active.

# M12 Live Worker Control Window 2H — PASS_LIVE_WORKER_CONTROL_WINDOW_V1

The separate snapshot-only Worker dashboard and next-run-only configuration
are validated against five canonical BizHawk runs. Window-off and window-on
16×20 runs each completed and independently audited 1,600/1,600 segments; a
live save kept the active 16×20 pool unchanged and the subsequent 8×21 run
loaded the exact saved configuration. Across the five runs, 6,400/6,400
segments passed independent audit, each configured Worker completed 100
cycles, and retention/runtime errors remained zero. `SOURCE_OWNED` remains
1,475,600 bytes (delta 0); no Worker/FLOW, AUTO67, Cartographer/Archivist,
2D/2E, or emission semantics changed. Report and compact receipt:
`docs/reports/THOR_M12_LIVE_WORKER_CONTROL_WINDOW_2H.md` and `.json`. The
broader M12 reconstruction milestone remains active.

# M12 Canonical ROM Knowledge Map 2D — PASS_CANONICAL_ROM_KNOWLEDGE_MAP_V1

The accepted AUTO61 `SOURCE_OWNED` manifest was reconciled to the earlier
AUTO60 base through the intervening GFX-2, GFX-MAX and AUTO61 checkpoints. The
compact SQLite map now covers the full canonical ROM with an exact disjoint
ASM/DATA/ASSET/INCBIN emission partition, typed static/runtime objects,
evidence-backed claims and only evidence-supported relations. It imports the
330 exact 2B instruction ranges and 199,630 instruction occurrence references;
`SOURCE_OWNED` remains 1,475,600 bytes. No new runtime campaign or hooks were
used. This completes the 2D checkpoint; the broader M12 reconstruction
milestone remains active. See
`docs/reports/THOR_M12_CANONICAL_ROM_KNOWLEDGE_MAP_2D.md` and its compact
deterministic JSON receipt.

# M12 Runtime Control Provenance 2C — STOP_NO_RUNTIME_PROVENANCE_GAP

Phase 0 classified the saved, accepted 2B runtime trace: 199,630 instructions,
zero indirect JMP/JSR occurrences, and 290 direct absolute-long JSRs. The
observed indirect-control provenance gap required to justify a sensor was not
present in this bounded execution. No native hooks or runtime campaign were
added. This is not a whole-game negative; any follow-up must begin from a real
runtime indirect transfer that existing evidence cannot resolve. See
`docs/reports/THOR_M12_RUNTIME_CONTROL_PROVENANCE_2C.md`.

# M12-CARVER-1 — Evidence/Gap Orchestration Baseline — STAGE 1 COMPLETE / STOPPED

Stage 1 imports the latest pushed AUTO60 manifest into a deterministic
`[0x000000,0x300000)` IntervalDB with zero coverage gaps and overlaps. It
preserves the exact AUTO60 SOURCE_OWNED count of `1,427,873` bytes, adds typed
evidence and a directed provenance graph, records blocking conflicts, and
produces deterministic UNKNOWN-gap and ranked-campaign reports. Existing
promoters remain authoritative; Carver does not create ownership, begin M13,
or migrate ASM to C++.

The reusable tooling is documented in
`docs/reports/THOR_ROM_CARVER_M12_STAGE1.md`. Stage 1 stops after its report;
detector expansion and any campaign execution require a separate authorization.

# M12-CARVER-2 — Graph-guided evidence expansion — STOPPED AT STABLE FIXED POINT

The analysis-only Stage 2 pass follows the confirmed 99-row table field1
provenance into the existing graphics census and emits five non-owning raw
size hypotheses totaling 17,408 bytes. No exact raw-resource consumer/boundary
contract was available, so SOURCE_OWNED remains `1,427,873` bytes and no
promoter transaction is run. The graph-guided result and deterministic output
hashes are recorded in `docs/reports/THOR_ROM_CARVER_M12_STAGE2.md`. M13,
ASM-to-C++ migration, and automatic detector expansion remain prohibited.

# M12-CARVER-3 — Global runtime-evidence sweep — COMPLETE / GLOBAL NEGATIVE

The complete local canonical-ROM evidence root was swept once through typed
runtime-span aggregation and Carver fixed-point reconciliation. It covered
every available stored runtime scenario and checkpoint artifact, but external
BizHawk/MAME capture runners were unavailable on this host. SOURCE_OWNED stays
`1,427,873` bytes; all 758 remaining UNKNOWN ranges have a global blocker
classification in `docs/reports/THOR_ROM_CARVER_M12_GLOBAL_SWEEP.md`. No new
ownership is inferred from runtime observation, and M13/ASM-to-C++ remains
prohibited.

# M12-CARVER-4 — Global static consumer recovery — COMPLETE / FIXED POINT

The global static pass consumed existing non-runtime re-slice, xref, candidate,
table and promotion evidence across the canonical ROM. It emitted 24,172 typed
references and 136 consumer families for all 758 UNKNOWN ranges, but recovered
no new exact boundary or ownership transaction. SOURCE_OWNED remains 1,427,873
bytes; B/F/G remain 113/56/589 ranges and the 168 blocking conflicts remain
explicit. The result is documented in
`docs/reports/THOR_ROM_CARVER_M12_STATIC_CONSUMER_RECOVERY.md`. Stage 2,
M13, and ASM-to-C++ migration remain prohibited.

# M12-AUTO56 — Source-owned ROM map checkpoint — 45.3426678975% / BELOW-90 CONTINUING

AUTO56 adds 24 bytes from the closed unknown prefix of the overlapping
PC-relative word table `[0x062DA8,0x062DC8)`. The exact consumer at `0x061A22`
closes 16 masked, doubled word selectors; the final four words are already
owned by AUTO28 and are not reclassified. The map reaches 1,426,357 / 3,145,728
bytes, remains below the 90% gate, preserves the canonical ROM identity, and
does not begin M13 or C++ migration.

# M12-AUTO55 — Source-owned ROM map checkpoint — 45.3419049581% / BELOW-90 CONTINUING

AUTO55 adds 124 bytes from the closed unknown suffix of the eight-record
`0x03B8DE` descriptor table and reaches 1,426,333 / 3,145,728 bytes. The
canonical ROM remains byte-exact. The 90% integer threshold is 2,831,156
bytes; 1,404,823 bytes remain. No C++ migration is started.

# M12-AUTO52 — Source-owned ROM map checkpoint — 45.3196525574% / BELOW-90 CONTINUING

AUTO52 adds 11,883 exact bytes from three direct-loader graphics streams. The
map reaches 1,425,633 / 3,145,728 bytes. Canonical ROM identity remains exact;
M13/C++ migration remains prohibited.

# M12-AUTO51 — Source-owned ROM map checkpoint — 44.9419021606% / BELOW-90 CONTINUING

AUTO51 adds nine static-pointer graphics streams totaling 28,000 bytes and
reaches 1,413,750 bytes. Unclosed parser-only or boundary-conflicting
candidates remain UNKNOWN.

# M12-AUTO50 — Source-owned ROM map checkpoint — 44.0518061320% / BELOW-90 CONTINUING

AUTO50 adds 76,019 bytes from two multi-resource families and three direct
loader streams. The proven family gap remains UNKNOWN; no ownership is inferred
from adjacency.

# M12-AUTO49 — Source-owned ROM map checkpoint — 41.6352272034% / BELOW-90 CONTINUING

AUTO49 adds ten descriptor-backed graphics streams totaling 72,284 bytes. The
unproven adjacent stream `0x1F66A0..0x1F683C` remains outside the map.

# M12-AUTO48..46 — Source-owned ROM map checkpoints — BELOW-90 CONTINUING

AUTO48 adds 36 bytes, AUTO47 adds 1,026 bytes, and AUTO46 adds 246 bytes from
closed dispatch/table contracts. Their cumulative checkpoints reach 39.3373807271%,
39.3362363180%, and 39.3036206563%, respectively. All preserve byte-exact ROM
identity and do not start M13/C++ migration.

# M12-AUTO45 — exact indexed offset table

AUTO45 adds the 32-byte table `[0x00AD56,0x00AD76)` after closing its
PC-relative base and 16-entry selector range. The map reaches
1,236,139 / 3,145,728 bytes (39.2958005269%), remains below the 90% gate,
and does not begin M13 or C++ migration. The adjacent `0x00AD76` code and the
dynamic `0x002888` DMA-queue source remain UNKNOWN/evidence-only.

# M12-AUTO44 — Source-owned ROM map checkpoint — 39.2947832743% / BELOW-90 CONTINUING

AUTO44 adds 1,732 exact bytes from 27 runtime-correlated count-bounded
6-byte record regions consumed by readers `0x00AF16` and `0x00ABB4`. Their
count tiling closes every promoted boundary; already-owned AUTO11 regions
are not double-counted. M13/C++ migration remains prohibited.

# M12-AUTO43 — Source-owned ROM map checkpoint — 39.2397244771% / BELOW-90 CONTINUING

AUTO43 adds the exact small caller-backed routine `[0x0083F8,0x00846C)` (116
bytes). Its exact `BSR` caller is `0x006230`; all 33 starts are runtime
observed/decoded, the range is bracketed by RTS instructions, and vasm
round-trip is byte-exact. M13/C++ migration remains prohibited.

# M12-AUTO42 — Source-owned ROM map checkpoint — 39.2360369364% / BELOW-90 CONTINUING

AUTO42 adds the exact caller-backed routine `[0x03B358,0x03B486)` (302
bytes). Static `BSR` callers at `0x03A900` and `0x03AA2A`, complete runtime
observation/decoding of 73 starts, closed conditional branches, and a
byte-exact vasm round-trip establish its boundary. M13/C++ migration remains
prohibited.

# M12-AUTO41 — Source-owned ROM map checkpoint — 39.2264366150% / BELOW-90 CONTINUING

AUTO41 adds the exact runtime-observed 68000 routine `[0x03AE74,0x03B092)`
(542 bytes). All 100 instruction starts are decoded and runtime-observed;
the bounded interval is bracketed by an earlier `RTS` and its terminal
`RTS` at `0x03B090`, with a byte-exact vasm round-trip. The map remains below
90%; M13/C++ migration remains prohibited.

# M12-AUTO40 — Source-owned ROM map checkpoint — 39.2092068990% / BELOW-90 CONTINUING

AUTO40 adds the exact runtime-observed 68000 routine `[0x00B79A,0x00B852)`
(184 bytes). The entry PC is present in canonical GPGX realtime evidence;
the routine is fully decoded, terminates at `RTS` `0x00B850`, and an
independent vasm round-trip matches every ROM byte. The map remains below
90%; M13/C++ migration remains prohibited.

# M12-AUTO39 — Source-owned ROM map checkpoint — 39.2033576965% / BELOW-90 CONTINUING

AUTO39 adds the exact ten-byte relative selector table `[0x15A9A6,0x15A9B0)`;
its five selected payloads remain UNKNOWN pending independent boundaries. The
canonical ROM remains byte-exact and M13/C++ migration remains prohibited.

# M12-AUTO38 — Source-owned ROM map checkpoint — 39.2030398051% / BELOW-90 CONTINUING

AUTO38 adds the exact contiguous menu range `[0x15B9D4,0x15BAC2)` (238 bytes):
an 8-byte relative offset table and four count-bounded six-byte record streams.
The following AUTO36 graphics range remains unchanged; the canonical ROM stays
byte-exact and M13/C++ migration remains prohibited.

# M12-AUTO37 — Source-owned ROM map checkpoint — 39.1954739889% / BELOW-90 CONTINUING

AUTO37 adds the exact 64-byte enum lookup `[0x05CE56,0x05CE96)` selected by
`0x007A6C`; its `0..4` values and `0..0x3F` selector range are closed. The
canonical ROM remains byte-exact and M13/C++ migration remains prohibited.

# M12-AUTO36 — Source-owned ROM map checkpoint — 39.1934394836% / BELOW-90 CONTINUING

AUTO36 adds three exact direct-consumer graphics streams totaling 5,085 bytes
at `0x15BAC2..0x15C238`, `0x15C238..0x15CA9B`, and `0x15CA9C..0x15CEA0`.
The one-byte `0xFF` separator remains UNKNOWN; M13/C++ migration remains
prohibited.

# M12-AUTO35 — Source-owned ROM map checkpoint — 39.0317916870% / BELOW-90 CONTINUING

AUTO35 adds the exact 64-byte bit-7 lookup `[0x05CE16,0x05CE56)`. Its
consumer masks the index to `0..63`; the adjacent payload remains UNKNOWN.
The canonical ROM remains byte-exact and M13/C++ migration remains prohibited.

# M12-AUTO34 — Source-owned ROM map checkpoint — 39.0297571818% / BELOW-90 CONTINUING

AUTO34 adds the exact 108-entry compressed-resource pointer table
`[0x05CE96,0x05D046)` (432 bytes), while leaving all already-owned target
streams and unknown semantics unchanged. The canonical ROM remains
byte-exact and M13/C++ migration remains prohibited.

# M12-AUTO33 — Source-owned ROM map checkpoint — 39.0160242716% / BELOW-90 CONTINUING

AUTO33 adds the exact 64-record item-label table `[0x05CC16,0x05CE16)`
(512 bytes), with two fixed-stride consumers and a printable-record boundary.
The canonical ROM remains byte-exact and M13/C++ migration remains
prohibited.

# M12-AUTO32 — Source-owned ROM map checkpoint — 38.9997482300% / BELOW-90 CONTINUING

AUTO32 adds six exact 8-byte menu-label records in `[0x05CBA6,0x05CBD6)`;
the selector is closed at `0..5` by the exact switch and fallback consumer
path. The canonical ROM remains byte-exact and M13/C++ migration remains
prohibited.

# M12-AUTO31 — Source-owned ROM map checkpoint — 38.9982223511% / BELOW-90 CONTINUING

AUTO31 adds the exact sentinel-terminated threshold table
`[0x05D906,0x05D918)` (18 bytes). Its consumer has a closed word stride and
the ninth-word `0xFFFF` sentinel; the canonical ROM remains byte-exact and
M13/C++ migration remains prohibited.

# M12-AUTO30 — Source-owned ROM map checkpoint — 38.9976501465% / BELOW-90 CONTINUING

AUTO30 adds the exact wholly UNKNOWN pointer-table container
`[0x00DF54,0x00E0B8)` (356 bytes): 89 absolute longwords consumed by three
identical state dispatchers. The pointed-to handler bodies remain unowned;
the canonical ROM remains byte-exact and M13/C++ migration remains
prohibited.

# M12-AUTO29 — Preserved code-census negative checkpoint — 38.9863332113% / BELOW-90 CONTINUING

AUTO29 reconstructs 496 bounded function ranges from the preserved candidate
map and exercises the strict transactional promoter. It discovers 534
candidates but finds 0 eligible, 0 attempted, and 0 accepted candidates, so
it adds no ownership. This exhausts only the current preserved automatic
code-census evidence set; exact data/parser/consumer provenance remains in
scope. The canonical ROM remains byte-exact and M13/C++ migration remains
prohibited.

# M12-AUTO28 — Source-owned ROM map checkpoint — 38.9863332113% / BELOW-90 CONTINUING

AUTO28 adds the exact 16-entry word lookup `[0x062DC0,0x062DE0)` (32 bytes).
Its consumer masks `D3` with `0xF` before indexed word access; the canonical
ROM remains byte-exact and M13/C++ migration remains prohibited.

# M12-AUTO27 — Source-owned ROM map checkpoint — 38.9853159587% / BELOW-90 CONTINUING

AUTO27 adds the wholly UNKNOWN `[0x00532C,0x005378)` 38-entry signed-relative
event dispatch table (76 bytes). The exact `0x00530C` selector contract and
full-ROM rebuild preserve the canonical ROM; no handler code or mixed payload
is inferred and M13/C++ migration remains prohibited.

# M12-AUTO26 — Source-owned ROM map checkpoint — 38.9828999837% / BELOW-90 CONTINUING

AUTO26 adds 54 bytes from two exact `86(A6)` callback entrypoints: the
52-byte routine at `[0x010000,0x010034)` and the `RTS` stub at
`[0x030000,0x030002)`. The canonical ROM remains byte-exact; the
F-line/data-like candidate at `0x80000` stays UNKNOWN and M13/C++ migration
remains prohibited.

# M12-AUTO25 — Source-owned ROM map checkpoint — 38.9811833700% / BELOW-90 CONTINUING

AUTO25 adds 352 bytes from exact PC-relative lookup, fixed-record, copied-record,
and VDP initialization consumers. The canonical ROM remains byte-exact and
M13/C++ migration remains prohibited.

# M12-AUTO24 — Source-owned ROM map checkpoint — 38.9699935913% / BELOW-90 CONTINUING

AUTO24 adds the exact caller-backed static island `[0x0167BE,0x01685A)`
(156 bytes). Its bounded decoder and vasm round-trip pass after a generated
ASM operand spelling correction for the terminal `C9 4E` (`EXG A4,A6`). The
canonical ROM remains byte-exact; M13/C++ migration remains prohibited.

# M12-AUTO23 — Source-owned ROM map checkpoint — 38.9650344849% / BELOW-90 CONTINUING

AUTO23 adds 296 bytes from two exact save-serialization ranges and preserves
the canonical ROM. The primary slot has a closed six-tag, 130-byte,
checksum-bounded stride-2 layout; the secondary promotion stops after its
confirmed six-tag/four-byte prefix. M13/C++ migration remains prohibited.

# M12-AUTO22 — Source-owned ROM map checkpoint — 38.9556248983% / BELOW-90 CONTINUING

AUTO22 adds 58 bytes from three exact fixed tables and preserves the canonical
ROM. AUTO21's CC-B0 target-table contract is also retained; M13/C++ migration
remains prohibited while bounded provenance work continues.

# M12-AUTO21 — Source-owned ROM map checkpoint — 38.9537811279% / BELOW-90 CONTINUING

AUTO21 adds 7,238 bytes from exact 256-slot signed-relative CC-B0 target
tables. Existing selected slots remain intact, no nested records are inferred,
and the canonical ROM remains byte-exact.

# M12-AUTO20 — Source-owned ROM map checkpoint — 38.7236913045% / BELOW-90 CONTINUING

M12-AUTO20 adds 115,011 bytes of wholly UNKNOWN graphics streams selected by
exact `field1` pointers in the 99-row table `[0x3F306,0x3FF66)`. Independent
decoder census boundaries and overlap checks pass; the canonical ROM remains
byte-exact and M13/C++ migration remain prohibited.

# M12-AUTO19 — Source-owned ROM map checkpoint — 35.0675900777% / BELOW-90 CONTINUING

M12-AUTO19 adds the bounded Ancient Music Driver sound-data container
`[0x064E38,0x07D780)` (100,680 `DATA_KNOWN` bytes) through the exact consumer
`0x060B50` and validated offset-table groups. The next `0xFF` run supplies the
terminal boundary; tertiary pointers outside it remain UNKNOWN. The canonical
ROM is unchanged and M13/C++ migration remain prohibited.

# M12-AUTO18 — Source-owned ROM map checkpoint — 31.8670590719% / BELOW-90

M12-AUTO18 adds two exact static-consumer graphics streams totaling 1,063
bytes through `LEA → D9A4 → 37D2 → 0x3820` contracts and preserves the
canonical ROM. The remaining payload and census-only candidates stay UNKNOWN;
M13/C++ migration remain prohibited.

# M12-AUTO17 — Source-owned ROM map checkpoint — 31.8332672119% / BELOW-90 BLOCKER

M12-AUTO17 adds two exact runtime-correlated graphics streams totaling
13,166 bytes. Both are read by the executed `0x3820` decoder, have matching
local decoder boundaries and static pointer literals, and preserve the
canonical ROM. The remaining payload and census-only candidates stay UNKNOWN;
M13/C++ migration remain prohibited.

# M12-AUTO16 — Source-owned ROM map checkpoint — 31.4147313436% / BELOW-90 BLOCKER

M12-AUTO16 closes nine exact runtime-correlated 68000 probe slices totalling
1,430 bytes after AUTO15. The slices are bounded by local decoder metadata,
canonical-equal probe binaries and PC-level runtime corroboration; adjacent
mixed ranges remain UNKNOWN. The canonical ROM remains unchanged and M13/C++
migration remain prohibited.

# M12-AUTO15 — Source-owned ROM map checkpoint — 31.3692728678% / BELOW-90 BLOCKER

M12-AUTO15 closes 15 unique constant-D0 caller-selected relative-pointer
slots (30 bytes) after the AUTO14 CC-B0 group table. Dynamic callers and nested
table extents remain UNKNOWN; the canonical ROM remains unchanged and M13/C++
migration remain prohibited.

# M12-AUTO14 — Source-owned ROM map checkpoint — 31.3683191935% / BELOW-90 BLOCKER

M12-AUTO14 closes the 128-byte `0x00CCB0` group pointer table
`[0x04371E,0x04379E)`. The nested target subtables remain UNKNOWN; the
canonical ROM remains unchanged and M13/C++ migration remain prohibited.

# M12-AUTO13 — Source-owned ROM map checkpoint — 31.3643% / BELOW-90 BLOCKER

M12-AUTO13 extends the byte-exact source-owned map to 986,634 of 3,145,728
bytes (31.364250183%). It adds 7,516 bytes from 100 BCEA-selected field3 list
views, merged into five exact ranges using signed relative pointers, 44-byte
rows, and negative-key sentinels. The surrounding container remains UNKNOWN;
the canonical ROM is unchanged and M13/C++ migration remain prohibited.

The 90% threshold is not reached. The remaining 1,844,522 bytes lack a
complete independent code/data/resource provenance graph.

# M12-AUTO11 — Source-owned ROM map checkpoint — 31.0906% / BELOW-90 BLOCKER

M12-AUTO11 extends the byte-exact source-owned map to 978,026 of 3,145,728
bytes (31.090609233%). It adds the 99-record table
`[0x3F306,0x3FF66)` and 46,858 bytes of count-bounded six-byte record streams
selected by exact 68000 consumers. The canonical ROM remains unchanged; M13 and
ASM-to-C++ migration remain prohibited. Full evidence is in
`docs/reports/ASM_AUTONOMOUS_PROVENANCE_TO_90_PERCENT_M12_AUTO2.md`.

The 90% threshold is not reached. The remaining 1,853,130 bytes lack a
complete independent code/data/resource provenance graph; unknown spans remain
conservative blobs.

# M12-AUTO10 — Source-owned ROM map checkpoint — 29.5003% / BELOW-90 BLOCKER

M12-AUTO10 extends the byte-exact source-owned map to 928,000 of 3,145,728
bytes (29.500325521%). It adds two caller-backed exact 68000 islands totaling
222 bytes and fixes developer-only baseline ASM mapping for header/vector
entries. The canonical ROM remains unchanged; M13 and ASM-to-C++ migration
remain prohibited. Full evidence is in
`docs/reports/ASM_AUTONOMOUS_PROVENANCE_TO_90_PERCENT_M12_AUTO2.md`.

The 90% threshold is not reached. The remaining 1,903,156 bytes needed for
the threshold lack a complete independent code/data/resource provenance graph;
unknown spans remain conservative blobs.

# M12-AUTO9 — Source-owned ROM map checkpoint — 29.4933% / BELOW-90 BLOCKER

M12-AUTO9 extends the byte-exact source-owned map to 927,778 of 3,145,728
bytes (29.493268331%). It adds 16 exact `0xFF` erased-alignment ranges totaling
132,630 bytes, each ending on a 4 KiB ROM boundary. The canonical ROM remains
unchanged; M13 and ASM-to-C++ migration remain prohibited. Full evidence is in
`docs/reports/ASM_AUTONOMOUS_PROVENANCE_TO_90_PERCENT_M12_AUTO2.md`.

The 90% threshold is not reached. The remaining 1,903,378 bytes needed for
the threshold lack a complete independent code/data/resource provenance graph;
unknown spans remain conservative blobs.

# M12-AUTO8 — Source-owned ROM map checkpoint — 25.2771% / BELOW-90 BLOCKER

M12-AUTO8 extends the byte-exact source-owned map to 795,148 of 3,145,728
bytes (25.277074178%). It adds five exact continuation streams selected by
direct `0x3820` consumers and decoder-advanced `A0`, while revalidating three
already-owned anchors. The canonical ROM remains unchanged; M13 and ASM-to-C++
migration remain prohibited. Full evidence is in
`docs/reports/ASM_AUTONOMOUS_PROVENANCE_TO_90_PERCENT_M12_AUTO2.md`.

The 90% threshold is not reached. The remaining 2,036,008 bytes needed for
the threshold lack a complete independent code/data/resource provenance graph;
unknown spans remain conservative blobs.

# M12-AUTO — Source-owned ROM map checkpoint — 8.884% / GLOBAL BLOCKER

M12-AUTO advances the exact source-owned map from 16,478 to 279,468 bytes
while preserving the canonical 3,145,728-byte ROM. It adds 181 caller-backed
exact 68000 islands, all 107 streams from the proven compressed-resource
pointer table, and 47 deterministic alignment bytes. Full-ROM CRC32, SHA-1,
and SHA-256 remain canonical with zero gaps and overlaps. The final report is
`docs/reports/ASM_AUTONOMOUS_TO_90_PERCENT_M12_AUTO.md`.

The 90% target is not reached. The remaining 2,866,260 bytes lack a complete
independent code/data/resource provenance map; they remain conservative blobs.
This is a global evidence blocker, not permission to begin M13 or C++
migration. The next M12 action is a new independent resource/pointer or
consumer-evidence track for the largest remaining intervals.

# M12.5 — Transactional ASM promotion of P0 region 0x003B3E..0x004A92 — DONE

M12.5 completed as an evidence-backed partial exact promotion before the
M12-AUTO combined checkpoint below. It promotes 1,426 exact nested ASM bytes.
Full evidence is in `docs/reports/ASM_PROMOTION_003B3E_004A92_M12_5.md`.

# M12.4 — Transactional ASM promotion of P0 region 0x000000..0x0007C4 — DONE

M12.4 completed as `M12_4_ROM_START_PROMOTION_PARTIAL_EXACT`. It promotes
512 vector/header bytes, 700 exact 68000 ASM bytes, and 108 structured-data
bytes, leaving 668 conservative bytes classified as POSSIBLE_CODE,
UNKNOWN_DATA, or UNRESOLVED_BOUNDARY. Full-ROM materialization remains exact
with 244 ASM ranges, 147 blob ranges, zero gaps, and zero overlaps. Exactly one
M12.5 proposal is recorded: `0x003B3E..0x004A92`, not started. Full evidence is
in `docs/reports/ASM_PROMOTION_000000_0007C4_M12_4.md`.

# M12.3 — Transactional ASM promotion of P0 region 0x006516..0x0083D4 — DONE

M12.3 completed an evidence-backed partial exact promotion against baseline
`b9b55fc46fad88eb2ff1285e85bd9d8169d0289d`. Fourteen independently bounded
intervals are now source-owned 68000 ASM, totaling 696 bytes of the 7,870-byte
target. The target retains 7,174 conservative unresolved-boundary bytes with
`POSSIBLE_CODE_BYTES=0`, `UNKNOWN_DATA_BYTES=0`, and no structured-data or
padding ownership. Full evidence is in
`docs/reports/ASM_PROMOTION_006516_0083D4_M12_3.md`.

The whole-ROM materialization now has 236 ASM ranges / 15,158 ASM bytes and
147 blob ranges / 3,130,570 blob bytes, with 0 gaps and 0 overlaps. The
canonical ROM round trip remains byte-exact. Exactly one M12.4 proposal is
recorded: `0x000000..0x0007C4` (1,988 bytes, 23 observed PCs, 3 static xrefs),
not started. STOP after M12.3.

# M12.2 — Transactional ASM promotion of P0 region 0x00DE00..0x00E338 — DONE

M12.2 completed an evidence-backed partial exact promotion against baseline
`0dac30bcca103ae03a6372d24c2aca25e1fb6460`. Thirteen independently bounded
intervals are now source-owned 68000 ASM, totaling 366 bytes of the 1,336-byte
target. The target retains 970 conservative unknown bytes with
`POSSIBLE_CODE_BYTES=0`, `UNKNOWN_DATA_BYTES=0`, and
`UNRESOLVED_BOUNDARY_BYTES=970`. No structured data or padding ownership was
claimed; the indirect dispatch at `0x00E2F0` remains blob-backed.

Result: `M12_2_P0_CODE_PROMOTION_PARTIAL_EXACT`. The whole-ROM materialization
now has 222 ASM ranges / 14,462 bytes and 144 blob ranges / 3,131,266 bytes,
with 0 gaps and 0 overlaps. Every promoted slice and the complete
3,145,728-byte ROM reassemble byte-for-byte exactly. No C++ gameplay/runtime
migration or emulator expansion was started. Full evidence is in
`docs/reports/ASM_PROMOTION_00DE00_00E338_M12_2.md`.

The recomputed queue proposes exactly one M12.3 candidate:
`0x006516..0x0083D4` (7,870 bytes, 25 observed PCs, 0 static xrefs), not
started. STOP after M12.2.

# M12.0 — Complete ASM reconstruction — COMPLETE / BLOCKED

M12.0 rebases the ordered project sequence to ROM -> complete reassemblable
ASM -> rebuilt-ROM runtime parity -> systematic ASM-to-portable-C++ migration.
The exact completion gates and current full-ROM census are recorded in
docs/reports/ASM_COMPLETION_CENSUS_M12_0.md.

The former M12 Inventory / UI / Save proposal is withdrawn as an immediate
C++ milestone. Those subsystem tracks remain future work inside the
reconstruction/migration phases. CPP_MIGRATION is
PAUSED_PENDING_ASM_COMPLETION. No new portable C++ routine is authorized in
M12.0.

Historical M12.0 result:
M12_0_CENSUS_COMPLETE_ASM_COMPLETION_BLOCKED. The current exact split owns
13,550 bytes as 203 68000 ASM ranges and 3,132,178 bytes as 136 local-ROM
blobs, with 0 gaps and 0 overlaps. M11.17 separately classifies 1,164
bounded bytes as structured data, but those bytes remain blob-backed and are
not yet source-owned. Eight coarse blobs contain observed execution evidence;
the single M12.1 target was 0x06042A..0x0611F4 and is covered above.

# M13 — Rebuilt ROM runtime parity — TODO

Start only after ASM_CODE_COMPLETE, ASM_ROM_MAP_COMPLETE and
ASM_REASSEMBLY_BYTE_EXACT. ASM_REBUILT_ROM_BOOT_PROVEN is the M13 gate.

# M14 — Systematic ASM -> C++ migration — TODO

Start only after rebuilt-ROM runtime parity is proven. Inventory, UI, Save,
Audio and other subsystem tracks belong here or in the preceding ASM
reconstruction work as evidence dictates.

# Runtime ROM property map M0–M9 — PARTIAL / ADAPTER ACCEPTANCE OPEN

The independently versioned runtime property tooling is approved by
ADR-ROM-PROPERTY-MAP-V1. M0 proof contracts, M1 map/checkpoint foundations,
M6 compatible union primitives, M7 interval export, and M8 non-promoting
canonical overlay are implemented. M2 has limited live acceptance for direct
M68K reads and completed instruction spans. M3 has limited live `Z80_DATA_READ`
acceptance; its permanent `ARG16()` regression passes, while Z80 executed
encoding remains unproven. M4 has limited live `VDP_VRAM_SOURCE` acceptance;
CRAM/VSRAM remain open. M5 exact-copy provenance has host-side primitives but
no live acceptance; audio remains open and does not gate the base map. M9 has
fresh corrected seals, range replays, canonical overlays, a three-repeat A/B/C
benchmark, and neutral plus controlled-input parity. Full-game parity and
remaining subsystem coverage are still missing. See
`docs/reports/THOR_RUNTIME_ROM_PROPERTIES_RESEAL_20260927.md`.

# M11.64 — G0 portability boundary consolidation — DONE

Result: `G0_PORTABILITY_BOUNDARY_PROVEN_M11_LINE_CLOSED`. M11.60–M11.63 evidence
is consolidated in `RE_LEDGER.md` and the closure report. G0 remains parent-owned,
0x0623AC is a real VDP hardware boundary, typed data is blocked, and 0x060286 is
deferred with `NO_ARCHITECTURAL_DECISION_CHANGE`. No production code changed.

# M11.63 — Exact 0x0623AC natural G0 callee/effect closure — DONE

Result: `CALLEE_0623AC_BLOCKED_HARDWARE`. Four parent call sites produce
1,944 exact entries/returns with A5 preservation; natural nested direct and
indirect calls return exactly. The static slice is `INDIRECT_CFG` at `0x062878`.
The complete natural effects include 407 VDP writes at `0xC00011`, so call
effect closure remains hardware-blocked. Evidence:
`reports/CALLEE_0623AC_CONTRACT_M11_63.md`.

Recomputed bounded-G0 ledger: `0x061934` is `PROVEN_NATURAL_CONTRACT`;
`0x062AE0` remains `EFFECTS_BLOCKED` by its latent `0x062CEC` indirect edge;
`0x0623AC` is `EFFECTS_BLOCKED` by hardware; `0x060286` is
`NOT_YET_CLOSED`. No sole remaining blocker is claimed.

M11.64 is proposal-only and STOP applies.

# M11.62 — Exact 0x061934 natural G0 callee/effect closure — DONE

Result: `CALLEE_061934_NATURAL_CONTRACT_PROVEN`. The six parent call sites
produce 2,916 entries/returns with exact A5 preservation; +0/+4 counts are
2,430/1,361. Natural nested direct and indirect calls return exactly. Static
status remains `INDIRECT_CFG` at 0x061F60, but its bounded natural targets are
closed. Transaction and typed-data gates remain fail-closed. Evidence:
`reports/CALLEE_061934_CONTRACT_M11_62.md`.

Next proposed milestone: M11.63 may target only the highest-ranked remaining
natural G0 blocker, currently 0x0623AC or 0x060286.
# M11.61 — Exact 0x062AE0 callee preservation/effect closure — DONE

Result: `CALLEE_A5_PRESERVATION_PROVEN_EFFECTS_BLOCKED`. Natural G0 calls
preserve A5 exactly and have a complete observed safe-RAM/G0-relative/stack
effect set. The static callee boundary retains an unresolved indexed JSR at
`0x062CEC`, so M11.60 transaction and typed-data gates remain fail-closed.
Evidence: `reports/CALLEE_062AE0_CONTRACT_M11_61.md`.

Next proposed milestone: M11.62 may follow only the dominant remaining blocker
inside the already-bounded G0 lifetime.

# M11.60 — Bounded A5 consumer/lifetime closure — DONE

Result: `BOUNDED_A5_CONSUMER_LIFETIME_PROVEN`. The natural G0 rooted at
`0x060182` has an exact parent-owned CFG, one taken path, selected +0/+4
consumers and a `0x06027E` parent restore endpoint. Transaction promotion is
blocked by parent lifetime and incomplete callee effects; typed data remains
fail-closed. Evidence: `reports/A5_CONSUMER_LIFETIME_M11_60.md`.

Next proposed milestone: M11.61 may close preservation/effects for one direct
callee crossed by G0, starting at `0x062AE0`; do not execute it as part of M11.60.

# M11.59 — Raw data alias/lifetime and external-writer closure — DONE

Result: `RAW_DATA_ALIASING_BOUNDARY_PROVEN_TYPED_DATA_BLOCKED`. The exact
all-ROM census and repeated developer-only provenance show that A5 escapes to
the stack, is reloaded and post-incremented across a broad range, with
multiple consumers of derived offsets. Fixed bytes have multiple writers;
the 0x60BCC siblings are hardware-ordered writers with the same raw order.
No typed structure, production abstraction or 0x60BCC promotion was made.
Evidence: `reports/RAW_DATA_OWNERSHIP_M11_59.md`.

Next proposed milestone: M11.60 should close one bounded A5 consumer/lifetime
contract or produce an exact negative for that blocker only.

# M11.58 — Portable behavior cluster boundary — DONE

Result: `PORTABLE_BEHAVIOR_CLUSTER_CONTRACT_PROVEN_REPLACEMENT_BLOCKED`. The
existing ParentSuffix contract is the first evidence-backed raw-parameter
composition of RamFlagRoutine and a parent-owned suffix. Its exact ordering,
opaque continuation and 600-frame shadow/native identity are proven. Typed data
is blocked by shared writers and unresolved alias/lifetime boundaries; no new
structure, subsystem or routine was added. Evidence:
`reports/PORTABLE_BEHAVIOR_CLUSTER_M11_58.md`.

Next proposed milestone: M11.59 must close one dominant raw-data alias/lifetime
and external-writer blocker only; do not execute it as part of M11.58.

# M11.57 — Parent-owned suffix handoff — DONE

Result: FIRST_PORTABLE_INTERNAL_HELPER_PROVEN. The exact seven-instruction
0x604F0 suffix is represented by a small architecture-neutral core helper
composing RamFlagRoutine and returning an opaque parent continuation token.
Natural shadow and authoritative 600-frame native runs preserve frozen
checkpoint/video/accounting identity with zero fallback/divergence. Parent
frame, hardware prefix, full SR, shared epilogue and RTS remain outside the
helper. Evidence: reports/PARENT_SUFFIX_HANDOFF_M11_57.md.

# M11.56 — 0x604F0 caller continuation closure — DONE (negative)

Result: THIRD_ROUTINE_NOT_A_STANDALONE_ROUTINE. The natural internal path
inherits a 58-byte saved frame from 0x60004/0x6042A and uses the shared
0x611D6 epilogue to return to original caller 0x424. The seven-instruction
span ends at 0x60516, where another arm enters inside the old decoder budget.
Dominant blocker: ENCLOSING_ROUTINE_BOUNDARY; whole-parent promotion would
also absorb observed hardware and partial full-SR/event semantics. No third
routine or subsystem was implemented. Debug/Release/GCC-UCRT CTest 68/68 each
and pre/post dual-native gates passed. Evidence:
reports/RAMFLAG_CALLER_ROUTINE_M11_56.md.

M11.57 subsequently closed the explicit parent-owned suffix handoff while
keeping full-SR/event, hardware and shared-epilogue ownership with the parent.

# M11.55 — RamFlag caller and shared-data contract closure — DONE

Result: RAMFLAG_CALLER_CONTRACTS_PROVEN. Dynamic attribution deterministically
matches every natural RamFlag entry in the unchanged dual-native 600-frame
scenario: one `0x0604F6` caller and three `0x060BCC` callers, zero unknowns.
Exact bounded CFGs, fixed RAM address provenance and the `0xA11100` hardware
prefix are recorded, but continuation ownership, sibling effects, typed shared
data and a complete hardware boundary remain blocked. No production source,
third routine or subsystem was implemented. Evidence:
reports/RAMFLAG_CALLER_DATA_CLOSURE_M11_55.md.

Next proposed milestone: M11.56 must select one falsifiable continuation or
sibling/data closure; implementation remains conditional.

# M11.54 — Native routine cluster and first subsystem boundary discovery — DONE

Result: PORTABLE_ROUTINE_CLUSTER_PROVEN. The unchanged M11.53 dual-native
baseline reproduced twice with exact frozen checkpoint/video/accounting and
continuation identity. Static ROM evidence proves one TableCopy caller and
two RamFlag callers. The RamFlag-centered call-graph/raw-memory cluster is
useful but not a portable subsystem: caller CFG/semantics are incomplete,
0x060BCC is hardware-coupled, and no typed shared structure is proven.
No third routine was implemented. Evidence:
reports/NATIVE_ROUTINE_CLUSTER_M11_54.md.

Next proposed milestone: bounded closure of 0x0604F6/0x060BCC caller CFGs and
the raw 0x00FF0010..0x00FF0016 window; implementation remains conditional.

# M11.53 — Second portable native routine — DONE

Result: SECOND_PORTABLE_NATIVE_ROUTINE_PROVEN. RamFlagRoutine at
0x0604BC..0x0604E6 was extracted into oasis_core and promoted beside
TableCopyRoutine with exact paired 600-frame checkpoint/video/accounting and
one event yield/resumption. Evidence:
reports/SECOND_PORTABLE_NATIVE_ROUTINE_M11_53.md.

# M11.52 — Native routine checkpoint mismatch root-cause closure — DONE

The single `TableCopyRoutine` replacement at `0x2D66..0x2D84` is now
authoritatively proven. M11.51's four-byte canonical checkpoint mismatch was
caused by a hybrid adapter that collapsed the routine into one lump-sum timing
handoff, omitting GPGX instruction-boundary prefetch/refresh processing. The
adapter now performs the exact instruction fetch/begin/finish continuation and
preserves the RTS/DBF/MOVEM contract. Paired 600-frame evidence matches the
frozen checkpoint aggregate
`251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d`, video,
CPU timing/refresh boundary and all counts: 6,488,773 total, 34 native routine
instructions, 13 iterations and zero fallback. Result:
`FIRST_PORTABLE_NATIVE_ROUTINE_PROVEN`. No second routine is started by this
milestone. Evidence: `reports/NATIVE_ROUTINE_MISMATCH_M11_52.md`.

# Historical M11.51 — First portable native routine reconstruction — CLOSED

The structural `TableCopyRoutine` (`0x2D66..0x2D84`) is implemented in
`oasis_core` with an external token/ROM adapter, independent differential and
CFG tests, and a zero-divergence 600-frame shadow proof. The authoritative
replacement is blocked because the current pinned GPGX native run produces
checkpoint aggregate `ae8887f5b32a4973a8243775612d68b891b588f6f5dc693558a5a8a2489e5403`
instead of frozen M11.50 aggregate
`251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d`.
Video and isolated instruction accounting remain exact; no promotion or
hardware broadening is authorized.

# M11.50 — Extract proven mechanical primitive layer into `oasis_core` — DONE

The four M11.49 copy/clear loops now use a portable core executor and
continuation state. `oasis_core` has no ROM candidate constants, canonical
opcode metadata, GPGX/libretro dependency or hybrid link dependency; the
developer-only hybrid layer retains registry/provenance, adapters, oracle and
reporting. Standalone synthetic tests and the dependency-boundary check pass.
The unchanged 600-frame shadow/native identity remains exact: 42,384/42,384
mechanical shadow comparisons, zero divergence, checkpoint aggregate
`251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d`, 6,488,773
total, 6,199,381 generated, 42,384 mechanical, 247,008 interpreter, 587
ranges, 150,135 yields and 288 resumptions. Result:
`PORTABLE_MECHANICAL_PRIMITIVE_LAYER_PROVEN`.
Evidence: `reports/PORTABLE_MECHANICAL_PRIMITIVE_LAYER_M11_50.md`.

# M11.49 — Mechanical primitive family closure — DONE

The M11.48 metadata-driven primitive was generalized without adding production
dependencies or address-specific executor branches. Two `MOVE.B (A2)+,(A1)+`
plus DBF loops (`0x003A0C`/`0x003A0E` and `0x00389E`/`0x0038A0`) and the
`CLR.W (A0)+` plus DBF loop (`0x0003F0`/`0x0003F2`) joined the existing byte
clear family member. Synthetic vectors, the unchanged 600-frame mechanical
shadow and native proof all passed with zero divergence. Native totals are
6,199,381 generated translated, 42,384 mechanical and 247,008 interpreter
instructions out of 6,488,773; checkpoint/video, 150,135 yields and 288
resumptions are unchanged. The promoted family had no hardware-visible access;
`0x060BA4` remains hardware-boundary blocked. Result:
`NATIVE_MECHANICAL_PRIMITIVE_FAMILY_PROVEN`.
Evidence: `reports/MECHANICAL_PRIMITIVE_FAMILY_M11_49.md`.

# M11.48 — First proven native mechanical primitive replacement — DONE

The M11.47 `MEMORY_CLEAR` candidate at `0x061266` plus `DBF` at `0x061268`
was promoted through a reusable architecture-neutral, resumable primitive API.
The generated/basic-block path remains the differential oracle and fallback.
Synthetic mid-iteration interruption tests passed; the 600-frame shadow
completed 3,780/3,780 combined primitive comparisons with zero divergence and
86 mid-operation yields/resumptions. Native proof preserved checkpoint/video,
full CPU equivalence, 150,135 aggregate yields, 288 interrupted resumptions,
zero fallback and zero hardware-visible accesses. The result is 6,488,773
total, 6,237,985 generated translated, 3,780 mechanical, and 247,008
interpreter executions across 587 ranges. Result:
`FIRST_NATIVE_MECHANICAL_PRIMITIVE_PROVEN`.
Evidence: `reports/NATIVE_MECHANICAL_PRIMITIVE_M11_48.md`.

# M11.47 — Safe-memory semantic closure and primitive discovery — DONE

The M11.46 runtime provenance was joined with the M11.45 final ledger. Seven
high-payoff `SAFE_MEMORY_OBSERVED` exact forms were independently verified and
mechanically generated: `0x00026A`, `0x003A0C`, `0x00389E`, `0x0003F0`,
`0x06193C`, `0x061954` and `0x061266`. The unchanged 600-frame GPGX shadow
completed 6,241,765 comparisons with zero divergence; native proof preserved
checkpoint/video identity, 150,135 yields, 288 resumptions, zero unexpected
fallbacks, zero hardware-visible accesses and zero starts inside translated
ranges. Final execution is 6,241,765 translated and 247,008 interpreter
instructions (96.1933%) across 587 ranges. The exhaustive remainder ledger
closes exactly, and repeated copy/clear structures are classified as mechanical
future replacement candidates without replacing them in this milestone.
Result: `SAFE_MEMORY_SEMANTIC_CLOSURE_PROVEN`.
Evidence: `reports/SAFE_MEMORY_SEMANTIC_CLOSURE_M11_47.md` and
`reports/REMAINING_INTERPRETER_ATTRIBUTION_M11_47.md`.

# M11.46 — Runtime address provenance and memory-class resolution — DONE

The unchanged M11.45 native baseline was reproduced twice. Three independent
600-frame `BASIC_BLOCK_ADDRESS_PROVENANCE` processes preserved the same
checkpoint/video identity and execution metrics. The existing GPGX developer
hook recorded exact PC counts, data-bus address/width/direction/order and A0–A7
transitions. All 635 register-address unknown PCs were observed, totaling
149,678 executions; 147,847 received runtime memory classification and 1,831
address-computation-only executions remain `UNKNOWN_WITH_EVIDENCE`. No semantic
candidate was promoted and no hardware contract was broadened.
Result: `BOUNDED_RUNTIME_ADDRESS_PROVENANCE_PROVEN`.
Evidence: `reports/RUNTIME_ADDRESS_PROVENANCE_M11_46.md`.

# Development Roadmap

The roadmap is ordered. Do not skip ahead unless a blocking dependency is documented.

## M11.45 — Bounded semantic closure toward 95% dynamic coverage — DONE

The M11.44 baseline was reproduced exactly twice. A bounded eligible semantic
tranche of 551 decoder-owned generated candidates passed independent exact-form
vectors, mechanical generation/provenance and the existing GPGX boundary shadow:
6,199,718/6,199,718 comparisons, zero divergence. Three forms remain fail-closed
after independent shadow rejection: LSR.W at 0x0038E0, ROR.W at 0x06115A and
CMPI.B at 0x0038AA. The unchanged native 600-frame scenario preserved checkpoint,
video, CPU/RAM/VDP/sound, event/interrupt boundaries, 149,059 yields,
288 resumptions and zero fallback/hardware-visible accesses. Final coverage is
6,199,718 translated and 289,055 interpreter executions, 95.5453%, across 580
registered ranges. Result: `REMAINING_ATTRIBUTION_AND_DYNAMIC_COVERAGE_95_PROVEN`.
Evidence: `reports/SEMANTIC_CLOSURE_M11_45.md` and
`reports/REMAINING_INTERPRETER_ATTRIBUTION_M11_45.md`.

## M11.44 — Restart and remaining interpreter attribution — DONE

M11.42 PHASE 1 was reproduced exactly twice from the M11.43 identity baseline:
6,488,773 total, 5,826,857 translated, 661,916 interpreter, 28 ranges,
140,065 yields, 274 interrupted resumptions, zero starts inside translated
ranges, checkpoint aggregate `251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d`
and video hash `5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`.

Gate result: `REMAINING_INTERPRETER_ATTRIBUTION_PROVEN_SEMANTICS_BLOCKED`.
The exhaustive decoder-backed baseline ledger closes exactly at 661,916. The
historical 0x03A7AE rejection is obsolete under the repaired bridge,
canonicalization and current boundary contract. Its exact generated
0x03A7AE–0x03A7B8 block passed 100,948 shadow comparisons with zero divergence
and the unchanged 600-frame native proof. Final native execution translated
5,927,805 instructions (91.3548%) and left 560,968 interpreter executions.
No 95% claim is made; 0x060BA4 remains hardware-visible fallback and the
remaining final Pareto is dominated by unverified semantic forms and proven
unknown register-address evidence. Evidence:
`reports/REMAINING_INTERPRETER_ATTRIBUTION_M11_44.md` and
`reports/REMAINING_INTERPRETER_ATTRIBUTION_FINAL_M11_44.md`.

## Status legend
- `ACTIVE` — current milestone being worked on
- `NEXT` — immediate queued milestone when useful to distinguish it from general TODO work
- `TODO` — planned
- `BLOCKED` — cannot proceed until documented dependency is resolved
- `DONE` — acceptance criteria met

## M11.43 — Complete GPGX checkpoint canonicalization contract — DONE
Starting from M11.42's checkpoint canonicalization blocker, reconstruct the
pinned GPGX v1.7.6 wholesale-struct layout, prove its ABI representation spans,
and make the developer-only identity adapter preserve every semantic byte.

Gate result: `CHECKPOINT_CANONICALIZATION_COMPLETED_NEW_AUTHORITATIVE_IDENTITY`.
The exact layout verifier proves the x64 sizes/offsets for YM2612 and Z80,
including the observed pinned-DLL Z80 base `144504`. Five independent current
proofs and raw replay from current, exact M11.39 and M11.41 evidence agree on
aggregate `251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d`.
No M11.42 attribution, candidate promotion or 95% coverage work was performed.
Evidence: `reports/CHECKPOINT_CANONICALIZATION_M11_43.md`.

## M11.42 — Restart: remaining interpreter attribution and 95% coverage gate — BLOCKED
The restart gate was run from M11.41 commit `5c19e22` with the restored target
aggregate `c9236218f55fb18f7f1d5095e4970b25f228de588bd03bd7cbbffccc2e225fd1`.
ROM/DLL, video, execution counts, 28-range registry, yields, resumptions and
zero starts inside translated ranges matched, but two fresh runs produced
`d5de401ceb64da875219d3ca2564160b954d55a217f4bb47ff0dc204c547ae36`.
Byte evidence shows M11.41 canonicalization leaves the next `FM_SLOT.DT`
pointer at serialized offset `140734`. Stop before attribution, semantics,
generation, shadow, hardware or native promotion. Evidence:
`reports/REMAINING_INTERPRETER_ATTRIBUTION_M11_42.md`.

## M11.41 — Checkpoint identity provenance and reproduction repair — DONE
The M11.40 baseline blocker was traced to host pointer and ABI representation
bytes included by the external GPGX wholesale state serializer. The first
difference was frame 60, serialized state offset `140654`, inside
`FM_SLOT.DT`. Raw state evidence is opt-in and ignored; the authoritative
identity canonicalizes only the proven representation spans for the recognized
GPGX state format. Three current and two exact historical-checkout repaired
runs agree on aggregate
`c9236218f55fb18f7f1d5095e4970b25f228de588bd03bd7cbbffccc2e225fd1`, video and
all recorded execution metrics. M11.40 PHASE 2 was not performed. Evidence:
`reports/CHECKPOINT_IDENTITY_M11_41.md`.

## M11.40 — Remaining interpreter attribution and 95% coverage gate — NEXT
The required M11.39 baseline reproduction matches the canonical ROM, external
GPGX DLL, 28-range registry, instruction counts, video sequence, boundary
yields and interrupted resumptions. M11.41 restored the authoritative repaired
aggregate `c9236218f55fb18f7f1d5095e4970b25f228de588bd03bd7cbbffccc2e225fd1`.
M11.40 stopped before remainder classification or promotion. No 95% claim was
made. Evidence: `reports/REMAINING_INTERPRETER_ATTRIBUTION_M11_40.md` and
`reports/CHECKPOINT_IDENTITY_M11_41.md`.

## M11.39 — Hot-path multi-block coverage expansion — DONE
Starting from M11.38, profile only the remaining interpreter executions and
select at most 40 decoder-owned candidates by dynamic instruction payoff. The
selected ranges must pass exact semantics, mechanical generation, provenance,
per-boundary GPGX shadow and the unchanged 600-frame native gate.

Gate result: `HOT_PATH_DYNAMIC_COVERAGE_80_PROVEN`. Two new ranges were
promoted: the 16-instruction RAM-only `ADD.W (A0)+,D0` loop
`[0x000380,0x0003A0)` and the direct `BNE.W` range `[0x03A864,0x03A868)`.
Native translated 5,826,857 of 6,488,773 guest instructions (89.7991%);
shadow completed 5,826,857 comparisons with zero divergence. Checkpoint/video,
interrupt continuation, hardware-access and no-start-inside-range gates stayed
exact. The remaining-interpreter Pareto ledger is conservative and recorded in
`reports/HOT_PATH_MULTI_BLOCK_COVERAGE_M11_39.md`; M11.40 is not implemented.

## M11.38 — Interrupt-safe multi-instruction block execution — DONE
Starting from M11.37, use only its four rejected two-instruction ranges and
prove a generic event/interrupt-safe continuation contract. Preserve the
M11.37 rejection evidence, yield after a guest instruction when GPGX reports a
boundary, and compare every boundary before native promotion. No atomic blocks,
runtime JIT, second scheduler or additional candidate discovery.

Gate result: `INTERRUPT_SAFE_MULTI_INSTRUCTION_BLOCKS_PROVEN`. All four frozen
ranges passed `4,122,062` per-instruction shadow comparisons with zero
divergence. Native execution preserved the current 600-frame checkpoint/video
equivalence, translated 4,122,062 of 6,488,773 guest instructions (63.5261%),
recorded 111,009 event boundary yields and 274 continuations after actual
GPGX interrupt service. No hardware-visible access or original start inside a
translated range occurred. Further coverage requires a new bounded milestone.

## M11.37 — Controlled dynamic coverage expansion — DONE
Starting from the M11.36 bridge proof, freeze exact before metrics for the
unchanged canonical 600-frame scenario, then use only natural execution to
build a bounded queue of approximately 10–25 blocks outside the existing six.
Every selected block must pass exact decode, independent semantic verification,
mechanical generation/provenance and authoritative GPGX shadow certification
before bounded native promotion. No runtime JIT or whole-ROM coverage claim.

Gate result: `CONTROLLED_DYNAMIC_COVERAGE_EXPANSION_PROVEN`. Sixteen new
natural single-instruction blocks were promoted beside the six existing
entries. Shadow completed 388,308/388,308 with zero divergence; native
promotion completed the unchanged 600-frame scenario with exact checkpoint
and video hashes, 388,314 translated guest instructions, 12 interpreter
fallback entries, zero starts inside translated blocks and a 5.9844%
translated-instruction share. Four multi-instruction candidates were rejected
fail-closed after observed interrupt interleaving; one hardware-visible
candidate was rejected before promotion. No runtime JIT or ROM-byte coverage
claim was added.

## M11.36 — GPGX timing / refresh bridge contract — DONE
Resolve the M11.35 cycle/refresh observer boundary without expanding the
candidate set. GPGX source evidence establishes frame-relative accumulated
`m68k.cycles`/`refresh_cycles`, frame rebasing through `mcycles_vdp`, normal
interrupt polling at `m68k_run` entry, and the post-instruction comparison
boundary before the next scheduler transition.

Gate result: `GPGX_TIMING_REFRESH_BRIDGE_PROVEN` and
`DEMAND_DRIVEN_BLOCK_PROMOTION_PROVEN`. The existing six-entry registry passed
185,975/185,975 shadow comparisons with zero divergence; native promotion then
passed the unchanged 600-frame scenario with exact checkpoint/video hashes,
185,975 native entries, zero original-body starts and zero observed interrupts.
The M11.35 negative result remains historical evidence.

## M11.35 — Demand-driven block promotion pilot — DONE (initial gate blocked)
Run one bounded 600-frame natural trace, select only a small set of additional
guest block entries, decode them through the existing exact IR, independently
verify newly required forms, generate provenance-bound C++ and require exact
GPGX shadow equivalence before any native promotion. Keep failed candidates on
the interpreter fallback and do not implement a runtime JIT or automatic trust.

Gate result: `DEMAND_DRIVEN_PROMOTION_RUNTIME_BLOCKED`. Discovery recorded 2,188
unique PCs and the bounded shortlist was `0x3A85E`, `0x3A8BA`, `0x3A88C`.
Independent TST vectors and generator/provenance checks passed. Shadow stopped
at the first candidate with `actual_cycles=74 expected_cycles=896114` and
`actual_refresh=228 expected_refresh=896268`; no M11.35 candidate was promoted.
The first blocker is the GPGX cycle/refresh and interrupt-boundary contract, not
a semantic pass or a whole-game coverage result.

## M11.34 — M68K semantic core — DONE
Extract and independently verify common instruction semantics, beginning with
CCR/SR and the instruction families emitted by M11.33. Keep the oracle and
generated hybrid boundary separate until the semantic helpers have their own
tests and machine-derived contracts.

Gate result: `M68K_SEMANTIC_CORE_INDEPENDENTLY_VERIFIED`. The exact seven
M11.33 emitted combinations passed 23 deterministic independent-reference
vectors, decode/length/provenance checks, and the unchanged M11.33 600-frame
GPGX shadow/native regression. No broader instruction or production surface was added.

## M11.33 — Recomp generator v1 — DONE
Generate the three M11.32 basic blocks from the shared decoder/exact IR and
execute the generated instruction sequence through developer-only hybrid
helpers. Generated output retains guest provenance and unsupported forms fail
closed. The existing 600-frame GPGX proof remains the behavioral gate.

Gate result: `MECHANICAL_BLOCK_GENERATION_PROVEN`. The canonical USA ROM
regenerated the three blocks byte-for-input deterministically; GPGX shadow and
native runs both completed 600 frames with 14/14 calls and matching state/video.

## M11.32 — Basic-block recompilation timing proof — DONE
Three naturally executed blocks (`0x2D66`, `0x604BC`, `0x61032`) now share a
minimal developer-only GPGX block boundary. Shadow comparisons are 14/14 with
zero divergence; native override completes the 600-frame neutral scenario with
matching serialized state/video and zero original-body instruction starts.
Keep this migration infrastructure separate from production runtime. Do not
expand coverage, repair `0x3820`, investigate ID3 or add AI generation here.

## M0 — Repository bootstrap — DONE
C++20/CMake bootstrap, ROM loader, minimal memory/VDP scaffolding and smoke test established.

## M1 — Project governance and documentation — DONE
Mandatory AI workflow, architecture/file map, 500-line rule, decision/research/work logs and task handoff are enforced.

## M2 — Identify supported ROM revision — DONE
Canonical engineering reference is USA retail `Beyond Oasis`; final C++ model remains region-independent.

Confirmed USA fingerprint:
- size `3145728`;
- CRC32 `C4728225`;
- SHA-1 `2944910c07c02eace98c17d78d07bef7859d386a`;
- SHA-256 `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

## M3 — Graphics decompression `0x00003820` — DONE
Verified native C++ translation of original routine `[0x3820, 0x3B3E)`.

Reference vectors:
- format A `0x16943C`: `1217 -> 3072`, SHA-256 `65e99e74020fedbdcb97c8249a5ccfe540aca5bb5d29bfb260352cd6f388c31a`;
- format B `0x1894EA`: `112 -> 128`, SHA-256 `167d4e5409f6b075b3b6f2bc61dbb747e8d8c857e8699745184ddf48d83bcda9`.

## M4 — Local asset inspection tool — DONE
`oasis_inspect` performs supported-ROM validation, native decompression, Genesis 4bpp tile decoding and local PGM/PPM export. Generated commercial data remains local/ignored.

## M5 — VDP data model and rendering primitives — DONE
Verified 64 KiB VRAM, 128-byte CRAM, 80-byte VSRAM, bounded byte/word access, standard tile pattern-name decoding, minimal plane/sprite data, explicit unsupported emulator semantics and green tests.

## M6 — Runtime frame/input skeleton — DONE
Portable controller snapshots, explicit integer frame stepping and deterministic replay-style tests are established without wall-clock or platform dependencies.

## M7 — World/map/collision foundations — DONE
Goal: decode room/map structures and collision semantics before translating player behavior.

Verified:
- screen dispatcher `0xC8F0` uses high/low bytes of the screen ID with group table `0xC92C` and resolves 26-byte descriptors;
- multiple USA ROM screen IDs resolve to fixed descriptor addresses and are covered by ROM-backed native tests;
- auxiliary byte-grid descriptors at `0xFF1766` / `0xFF1770` expose backing pointer, row stride and row shift;
- world coordinates map to 8-pixel cells; `0x9C40` is the confirmed single-cell reader;
- `0x9BF2` aggregates OR/AND values across an entity footprint and is reproduced by `ByteGridView`;
- low-nibble terrain codes are mapped by tables `0x96E8` / `0x96F8` into movement/height classes;
- `0x938E` is a confirmed directional terrain movement gate: carry set blocks, carry clear allows;
- portable byte-grid, footprint and terrain-gate tests pass;
- final build-test, reference-ROM workflow and M7 probe are green.

## M8 — Player system — DONE
Goal: translate player movement/state from verified binary behavior before attacks and presentation layers.

Tasks:
- identify player entity/update entry and call path;
- trace controller bits into movement intent;
- connect player movement to the M7 terrain gate;
- identify exact position/delta/facing/movement-state fields used by the path;
- translate the smallest portable movement slice with integer/fixed-point semantics;
- add deterministic movement and blocked-movement tests;
- add ROM-backed behavior evidence where practical;
- only then expand into animation and attacks.

Acceptance criteria:
- player movement/update path documented with addresses and confidence labels;
- portable movement state/update exists without platform/render dependencies;
- collision uses the single M7 world model;
- deterministic tests cover free and blocked movement;
- reverse-engineering ledger/worklog updated;
- files remain <= 500 lines;
- CI green.

Delivered on `main`: the portable movement/state slice, M7 terrain-gated integration,
ROM-backed evidence, raw velocity context boundary, local Debug/Release verification
and successful GitHub Actions CI. The commercial ROM remains local-only.

## M9 — Entities/enemies/NPCs — DONE
Goal: translate the common entity framework and one evidence-backed representative
non-player callback-dispatch path.

Delivered on `main`: raw pool descriptors and bounded record views, the shared
movement field-access contract, a representative `FF2954` dispatch gate, local
Debug/Release verification and successful GitHub Actions CI. AI, attacks,
animation and spawn semantics remain explicitly outside this milestone.

## M10 — Spirits — DONE
Goal: reproduce one evidence-backed spirit lifecycle slice: summon/slot state,
target selection and one ability/interaction contract. The first accepted slice
now includes the caller-gated raw summon-entry seed at `0x7A10 -> 0x846C`,
four event-backed slot bits, raw dispatch and first-match target selection.
Rendering, audio and unproven spirit semantics remain outside the milestone.
The milestone acceptance slice is complete with local Debug/Release verification
and successful GitHub Actions CI; remaining unknown relationships stay documented
as unknown and may be revisited when later evidence requires them.

## M11 — Scripts/events/dialogue — DONE
Goal: reproduce game progression and event semantics. The first task is to
prove one bounded event/script router operation from its ROM data source.

Current slice: the type-8 producer at `0x82AE` transfers raw fields to
`FF1976/FF1978/FF197A`, and `0x7A28` maps the raw event byte to bounded handler
addresses. Event, progression and dialogue meanings remain unknown.

The evidence-backed bounded M11 slice is complete with native tests, local
USA-ROM verification and CI. The post-M11 RE-acceleration checkpoints are also
complete: separate bounded decoder/report tooling covers `0x60004` and the
representative routines `0x3820`, `0x8E90`, `0xA6A4` and `0xD3B2`, plus a
bounded dynamic trace scenario for `0xA7D4..0xA7E4`, without adding gameplay
behavior. A fourth bounded comparison tool fingerprints the user-supplied USA
Beta 1994-11-01 ROM and compares only the requested `0x3820`, `0x60004`,
`0x82AE`, `0x7A28` and `0xA6A4` targets using normalized opcode/CFG signatures.
Unknown dialogue, progression, driver and cross-revision semantics remain
explicitly unimplemented. Full-game tracing, broader similarity search,
whole-ROM discovery, recompilation and M12 remain out of scope until explicitly
authorized.

Post-M11.5 evidence checkpoint: M11.6 verified static translation PoC is DONE.
It verified three small developer-only compiled C++ slices (`0x3820`, `0xA8DA`
and `0x62CC`) with bounded differential fixtures and no runtime interpreter.
The result is `STATIC_TRANSLATION_POC_NEEDS_FIXUPS`; mechanical translation is
retained only as a verification aid. No next-step implementation is authorized
by that checkpoint.

Post-M11.6.1 evidence checkpoint: M11.7 single-target reachability root cause
is DONE. Static incoming analysis and watch-only BizHawk observations show
`CALLER_NOT_REACHED` for `0x62CC` under both existing scenarios: none of its
33 direct incoming PCs executed. The next recommendation is one separately
authorized minimal natural scenario; no scenario is created here.

Post-M11.7 evidence checkpoint: M11.8 is `ROOT_CAUSE_ADVANCED`. A new
hardware-reset natural scenario with title/start, movement, attack/use and
interaction/room input phases ran 1800 frames with exact hooks for all 33
incoming sites, player/event owners and the `0x3820` positive control. The
positive control remained live, while the target and all incoming sites stayed
unreached. The probe now records input schedule, frame PCs and selected RAM
state. The next bounded action is a timing/hold-input sweep around the
startup-to-gameplay transition; production C++ and M12 remain out of scope.

Any later follow-up remains evidence-gathering only while its implementation-
slice confidence remains below 90%; production C++ must not expand until
caller/data or downstream-handler evidence raises that slice to >=90%.

## M11.5 — Recursive structural exploration — DONE
Goal: automate bounded, provable 68000 structural exploration while retaining
explicit uncertainty frontiers and a reusable ROM address map.

Delivered: developer-only oasis_re_explore reuses re_slice_decoder, re_atlas
and re_candidate_map; it applies tiered provenance seeds, a deterministic
priority worklist, guarded recursive control-flow traversal, explicit
analysis states, compact map ranges and stable blocker identities. Bounded
validation runs before the optional single ROM-wide measurement.

The fresh USA Ghidra 12.1.3 raw export was repeated A/B with identical
390972-byte SHA-256
613A3AA6DEB8D2DCF994C82ADC6A6939B7D5F27AF67A51D05C16F090D60A5315.
The bounded corpus passed: 15 entries and all six current direct-edge
anchors were recovered. The gated ROM-wide evidence run processed 537 entries,
decoded 19623 instructions and emitted 148 frontiers; 451 entries were
analyzed, 25 stopped on indirect flow and 28 on unsupported instructions.
The result is structural evidence only; no Atlas classification or production
runtime behavior was changed. The dynamic ant PoC remains the next separately
authorized checkpoint and is not implemented here.

## M11.5 — Single-ant closed-loop evidence bridge — DONE
Goal: resolve exactly one current indirect-flow frontier through one bounded
natural emulator job, preserve its provenance, and measure one static rerun.

Delivered: deterministic `oasis.m68k.re-ant-job.v1` and
`oasis.m68k.re-ant-result.v1` tooling, one BizHawk natural-reset worker,
natural-only A/B validation and a provenance-tagged dynamic explorer edge.
Frontier `0x020E:0x045A` resolved to `0x307A` twice with result hash
`0x21238399`; the rerun closed one frontier and added 480 instruction bytes.
The worker is a developer-only bridge; production runtime and architecture are
unchanged. No scheduler, swarm, second frontier job or M12 work is authorized
by this checkpoint.

## M11.5 — Single-worker sequential ant queue PoC — DONE
Goal: process a small bounded frontier batch through one emulator worker while
preserving deterministic identity, lifecycle, natural-only provenance and
honest failure states.

Delivered: frozen `oasis.m68k.re-ant-queue.v1` queue of exactly five existing
ant jobs, one-claim lifecycle with stale recovery and resolved-duplicate
suppression, deterministic A/B comparison, sequential BizHawk process restarts,
and one batch merge into `oasis_re_explore`. Three jobs produced natural
dynamic edges (`0x307A`, `0x6211A`, `0x62900`); two bounded fallback jobs were
`FAILED_FINAL`/`NOT_REACHED`. Explorer frontiers changed `148 -> 145`, with
three provenance-tagged dynamic edges. This checkpoint ends before parallel
workers, a scheduler, a larger queue, or M12.

## M11.5 — Ant reachability diagnostic — DONE
Delivered: a bounded reachability-only extension to the existing natural
BizHawk probe, a deterministic ten-context sample, two existing scenario
checks, static source validation and a fresh positive control. All sampled
contexts remained `NOT_REACHED`; the environment was healthy, so the result is
`SCENARIO_COVERAGE_INSUFFICIENT`. The next recommendation is a separately
authorized small set of natural gameplay scenarios. No queue expansion,
parallelism, checkpoint farm or production runtime work was added.

## M11.9 — Reassemblable disassembly pipeline PoC — DONE
Post-M11 tooling checkpoint M11.9 is DONE: five selected slices (330 instructions,
882 bytes) round-trip exactly through decoder-owned typed IR and vasm. The local
bounded split also matches. Result: `REASSEMBLABLE_DISASM_POC_HIGH_VALUE`.
Next recommendation is A, expand to 25–50 verified routines; not implemented.
M12 remains TODO.

## M11.10 — Diverse reassembly coverage expansion — DONE
The bounded corpus now contains exactly 25 mass/explorer-selected routines,
including the three M11.9 controls. Shared decoder metadata and ASM emission
cover 105 observed practical forms with 105/105 exact round trips; the expanded
split and legacy M11.9 mixed split both match. No routine-specific encoding
patches or raw opcode directives were used. Runtime meaning remains unknown.
Decision: `DIVERSE_REASSEMBLY_HIGH_VALUE`. The one queued recommendation is A,
establish a full-ROM split baseline; it is deferred and not implemented here.

## M11.11 — Full-ROM split reassembly baseline — DONE
The trusted M11.10 ranges now anchor a deterministic manifest spanning the
canonical ROM. The full 3,145,728-byte output matches byte-for-byte; uncertain
regions remain local-ROM-backed blobs, with zero gaps and overlaps. Decision:
`FULL_ROM_SPLIT_EXACT`. Recommendation A — replace verified blob regions with
ASM/data — is deferred and not implemented here.

## M11.12 — Automated blob-to-source promotion PoC — DONE
Existing candidate-map and mass-verification evidence now drives deterministic
bounded promotions. Twenty-one of 25 attempted candidates were accepted after
exact slice and full-ROM checks; ASM coverage increased from 1,846 to 2,632
bytes and the canonical hashes remained exact. Decision:
`AUTO_PROMOTION_HIGH_VALUE`. Recommendation A — increase the automatic batch to
100 candidates — is deferred and not implemented here.

## M11.13 — Automated promotion scale pass — DONE
The post-M11.12 manifest was carried forward without repeating accepted
candidates. One hundred deterministic candidates were attempted and 84 accepted
after exact slice and full-ROM checks. ASM coverage increased from 2,632 to
6,462 bytes; canonical full-ROM hashes remained exact. A generic
immediate-to-CCR emitter syntax fix (.b for CCR) was regression-tested and
accepted both prior CCR-blocked candidates. Decision: AUTO_PROMOTION_SCALE_HIGH_VALUE.
Recommendation A — continue automatic code promotion with another large batch —
is deferred.

## M11.14 — Automatic promotion large batch II — DONE
The post-M11.13 manifest excluded prior attempts and carried two old slice
mismatch retries. Ninety-one attempts accepted 73 (80.22%); acceptance windows
remained stable and ASM grew to 13,550 bytes. Full-ROM hashes stayed exact.
Rejects are clustered by systemic form; unsupported and nonrepresentable vasm
forms remain explicit. Decision: AUTO_PROMOTION_BATCH2_HIGH_VALUE.
Recommendation A — run another automatic code batch — is deferred.

## M11.15 — Evidence integrity audit and classification trust repair — DONE
All 203 M11.14 ranges were audited with per-artifact round-trip checks, ROM
identity, entry/range provenance, concrete incoming xrefs and caller trust.
The resulting ladder is 197 `ASM_ROUNDTRIP_EXACT`, 5
`CODE_STATIC_SUPPORTED`, 1 `CODE_EXECUTED` and 0 `BEHAVIOR_VERIFIED`; the full
ROM remains exact. One known Ghidra boundary mismatch at `0x3820` remains a
bounded fixup. Decision: `EVIDENCE_TRUST_NEEDS_FIXUPS`. Recommendation B —
targeted dynamic confirmation of critical code — is deferred.

## M11.16 — Targeted dynamic code confirmation — DONE
Five critical ranges were selected before any run and checked against retained
natural reports. None had a verifiable target-hit artifact; the existing
`0x6121A` natural positive control passed exact ROM/scenario/hash checks. No
promotion, emulator run, or scenario expansion was performed. Decision:
`TARGETED_DYNAMIC_REACHABILITY_LIMITED`. The single deferred recommendation is
D — one bounded timing/hold-input sweep around the M11.8 startup transition.

## M11.17 — Structured data classification PoC — DONE
The bounded proof accepted nine exact structured-data ranges and one weaker
header region, covering 1,164 bytes without changing the blob-backed full-ROM
representation. Invalid pointer-like data and code/data overlap are rejected
or recorded as conflicts, and the future code-promotion gate vetoes only
trusted data classifications. Decision: `STRUCTURED_DATA_HIGH_VALUE`. The
single deferred recommendation is A — return to the native vertical slice.

## M11.18 — Native controlled screen vertical slice — DONE
Goal: prove the first interactive native path with explicit synthetic geometry:
input, deterministic logical update, player movement, terrain collision, software
framebuffer and visible window.

Delivered: `ControlledScreen` reuses the M6/M7/M8 runtime, player and byte-grid
contracts; a 32x28 test fixture renders free terrain, walls, a corner and an
isolated obstacle; `oasis_platform` provides a minimal Windows Win32 input/window
adapter with focus-loss clearing and scaled DIB presentation. A 600-frame input
replay compares state after every frame and is invariant across presentation
intervals. The framebuffer has a fixed SHA-256 oracle, and canonical ROM identity
is enforced before opening the window.

Validation closure: the canonical-ROM Win32 executable was manually checked for
window creation, cardinal and diagonal movement, release, focus-loss clearing,
focus restoration, visible fixture-wall collision and responsiveness. The
fixture is synthetic and does not claim an original room. MSVC, Linux,
deterministic replay/hash and ROM identity checks passed locally. GitHub Actions
run `34052574735` passed for the closure commit; the final documentation-only
commit `4118867a487227275d18246c93b8ebbe7980d8f2` also passed exact-SHA CI in
run `34052627403`. The non-Windows
GUI adapter remains intentionally unavailable and is recorded as the next
recommendation rather than an M11.18 Win32 runtime blocker. Decision:
`NATIVE_VERTICAL_SLICE_PLAYABLE`. The single recommendation is D — platform
portability.

Post-M11.25 native resource checkpoint: resource ID 3 is now loaded from the
verified canonical-ROM table contract, decompressed by the existing native
routine, transferred to native VRAM `0x4000..0x4FFF` and rendered as a
diagnostic 128-tile atlas. The result is
`NATIVE_ROM_RESOURCE_ID3_HIGH_VALUE`; no room, background, player or sprite
semantic role was promoted. The single deferred recommendation is A — identify
the resource's palette/visual role.

## M11.28 — Hybrid native migration PoC — DONE (shadow); override BLOCKED
M11.27 manual ID3 runtime hunting is stopped by explicit user direction.
The developer-only `0x3820` hook observes six natural calls in the existing
600-frame neutral startup scenario. Three shadow runs provide 18 clean bounded
comparisons and identical emulator checkpoints/video to EMULATED. Result:
`HYBRID_SHADOW_PROVEN_OVERRIDE_BLOCKED`. CCR.X, interrupt-aware timing and
prefetch/IR continuation remain unproven, so override is rejected before
execution. No production emulator dependency is introduced. STOP; no follow-up
routine, gameplay search or tooling expansion is active.

## M11.29 — Minimal safe native override target — DONE
The naturally executed `0x2D66` leaf was selected from the existing 600-frame
neutral PC bitmap: ten decoded instructions, direct `RTS`, one local `DBF`, no
hardware access, no nested call and bounded full-SR/RAM/stack effects. One
SHADOW_NATIVE call compared cleanly with zero divergences. One NATIVE_OVERRIDE
call skipped all original target-body instruction starts, completed 600 frames,
and matched EMULATED checkpoint and video sequences exactly. Result:
`HYBRID_NATIVE_OVERRIDE_MINIMAL_PROVEN`. The adapter remains developer-only;
`0x3820` timing/CCR.X/prefetch work, ID3, manual gameplay, coverage expansion
and AI generation remain out of scope.

## M11.30 — Hybrid migration small batch — DONE (shadow); override not repeatable
The shared developer-only registry covers `0x2D66`, `0x604BC` and `0x61032`.
All 14 natural calls shadow cleanly with zero divergences and no interrupts.
The simultaneous native run skips every original body and completes the
scenario, but serialized VDP/sound state diverges from EMULATED because the
minimal boundary lacks the exact per-instruction bus-refresh/hardware phase
contract. Result: `HYBRID_OVERRIDE_NOT_YET_REPEATABLE`. Stop here; do not expand
timing work or production dependencies.

## M11.31 — Comparative disassembly method transfer — DONE
Pinned public Streets of Rage 2/3 projects were compared with existing Beyond
Oasis evidence. The RuiNelson disassembly/recompilation workflow is retained as
`PARTIAL_TRANSFERABILITY`; IDA assembly and Pancakes are `SPECIALIZED_ONLY`.
Programmer-style similarity is `NON_DISCRIMINATING`, binary structure is only
`PARTIAL`, and sound-driver lineage is `SPECIALIZED_ONLY`/unresolved. The
`0x2D66` dry-run confirms workflow compatibility without importing code or
changing runtime dependencies. No high-confidence template was found.
Possible M11.32: one bounded Beyond Oasis evidence-led pass using the adapted
sequence in the report. Do not broaden coverage or repair hybrid timing here.

## Future subsystem tracks after M14

Inventory/UI/Save, Audio and other gameplay subsystems remain TODO tracks
inside the ASM reconstruction and systematic migration work. They are not
standalone milestones ahead of the M12 -> M13 -> M14 gates. Each track requires
its own evidence and architectural decision before implementation.

## M15 — Portability and packaging — TODO
Initial targets: Windows, Linux and macOS. Additional platforms are later decisions.

## M16 — Optional enhancements — TODO
Only after base parity: scaling/resolution, controller UX, widescreen experiments, QoL and rendering enhancements. Faithful mode remains available.

## Deferred
**The Story of Thor 2 / The Legend of Oasis** is explicitly deferred until the first project reaches a mature parity milestone.
# M11.53 — Second portable native routine — DONE

M11.53 extracted the bounded direct-RTS RAM flag/output leaf
0x604BC..0x604E6 into oasis_core. Its adapter validates the exact
opcode/extension stream and drives every instruction through GPGX
fetch/begin/finish, boundary sampling and RTS return/prefetch handoff. The
dual 0x2D66,0x604BC 600-frame native runs match the reference aggregate,
video and accounting: 6,488,699 interpreter, 34 TableCopy and 40 second
routine instructions, zero fallback/divergence. Result:
SECOND_PORTABLE_NATIVE_ROUTINE_PROVEN. Evidence:
reports/SECOND_PORTABLE_NATIVE_ROUTINE_M11_53.md.
The shared block-hook continuation recorded one event yield and one resumption.
# M12-AUTO54 — Source-owned ROM map checkpoint — 45.3379631042% / BELOW-90 CONTINUING

AUTO54 adds 376 bytes from 15 closed PC-relative table/literal contracts and
reaches 1,426,209 / 3,145,728 bytes. Mixed code/data candidates remain
UNKNOWN; canonical ROM identity remains exact and M13/C++ migration remains
prohibited.

# M12-AUTO53 — Source-owned ROM map checkpoint — 45.3260103861% / BELOW-90 CONTINUING

AUTO53 adds 200 bytes from five fixed PC-relative lookup-table contracts and
reaches 1,425,833 bytes. No neighboring bytes are promoted from adjacency.
# M12-AUTO57 — Source-owned ROM map checkpoint — 45.346736907958984% / BELOW-90 CONTINUING

AUTO57 adds 128 bytes `[0x03BD86,0x03BE06)` from the exact bounded
word-transform routine `0x03B7C0`. The source-owned map is now 1,426,485 of
3,145,728 bytes. The integer `>=90%` threshold is 2,831,156 bytes; 1,404,671
bytes remain. This is still ASM/data provenance work only; C++ migration has
not started.
# M12-AUTO59 — Source-owned ROM map checkpoint — 45.35404841105143% / BELOW-90 CONTINUING

AUTO59 adds 94 bytes from two exact header/count-closed word-copy tables:
`[0x000472,0x0004B4)` and `[0x0031FC,0x003218)`. The map is now 1,426,715 of
3,145,728 bytes; 1,404,441 bytes remain to the integer 90% threshold. This is
still ASM/data provenance only; C++ migration has not started.

# M12-AUTO58 — Source-owned ROM map checkpoint — 45.351060231526695% / BELOW-90 CONTINUING

AUTO58 adds the exact static ASM routine `[0x003260,0x0032E8)` (136 bytes).
The isolated source round-trip is exact; the inherited full-layout duplicate
label blocker remains unchanged. The map was 1,426,621 bytes at this step.
# M12-AUTO60 — Source-owned ROM map checkpoint — 45.39086023966471% / BELOW-90 CONTINUING

AUTO60 adds 1,158 bytes from 15 exact direct-caller-to-RTS static ASM islands.
The systematic audit tested 33 candidates and rejected 18 non-exact cases. The
map is now 1,427,873 of 3,145,728 bytes; 1,403,283 bytes remain to the integer
90% threshold. C++ migration remains out of scope.

# M12-CARVER-5 — Whole-ROM format/container reconstruction — DONE / BELOW-75

The global structural pass fingerprinted all 758 remaining UNKNOWN ranges,
checked 858,249 bounded BO-decoder candidate starts, and attached 4,494 typed
evidence records to the IntervalDB. It found no independently closed reusable
format/container contract, so SOURCE_OWNED remains 1,427,873 bytes (45.3908602397%).
The complete B/F/G map is unchanged. No ROM/assets, unknown `dc.b`, M13, or
ASM-to-C++ work was added. See
`docs/reports/THOR_ROM_CARVER_M12_FORMAT_CONTAINER_RECONSTRUCTION.md`.
# M12 Post-Run Gameplay RAM / Entity Candidates V1 — PASS

The first gameplay-state analysis layer is integrated after Sprite/SAT analysis
and before ASM closure. It consumes preserved W3 V2 runtime evidence, retains
uncertain identities and frame gaps fail-closed, proves exact strides and
RAM-to-SAT field chains only where evidence supports them, and does not promote
ROM ownership. The preserved real fixture reached `PASS_POSTRUN_GAMEPLAY_RAM_ENTITY_CANDIDATES_V1`
with `SOURCE_OWNED_DELTA = 0`; semantic player/enemy/NPC classification remains
out of scope.
# M12 Targeted Dynamic Sprite Capture V1 — PASS

The bounded natural-input capture now records one SAT snapshot per frame and
shadow writes with runtime PCs. It closes dynamic SAT-entry to `0xFF13CC`
shadow-source correlation across a measured one-frame DMA/publication lag;
object/player semantics remain outside this checkpoint. See
`docs/reports/THOR_M12_TARGETED_DYNAMIC_SPRITE_CAPTURE_V1.md`.
# M12 Dynamic SAT Producer Backtrace V1 — PASS

The accepted dynamic SAT entry now has an exact bounded producer backtrace:
the X-field source is read from canonical ROM `0x0017435C` at `0x00B768`,
transformed by `ADD.W D3,D6`, and stored at `0xFF13D0` by `0x00B76E`
(`0x00B772` callback identity). Twenty-three transitions validate the same
path. Upstream A0 construction and gameplay/object meaning remain unresolved;
PLAYER labeling is prohibited at this boundary.
# M12 Dynamic SAT Caller-Side A0 Provenance V1 — PASS

The accepted dynamic tail is now tied to runtime caller `0x03B448`, exact
`JSR.L` entry at `0x00B730`, and A0 reaching definition `0x03B436`. The path
is validated across 23 transitions; selector semantics remain unassigned and
`SOURCE_OWNED` is unchanged.
# M12 Dynamic SAT Selector / A6 Provenance V1 — PASS

The accepted caller-side path now proves A6 base `0x00FFAFCE`, field `8(A6)`,
its ROM writer, and exact selector-to-ROM-record mapping across 23 transitions.
Entity/player semantics remain unassigned and SOURCE_OWNED is unchanged.

# ROM Property Map v1 live-runtime continuation — PARTIAL / IN PROGRESS

The corrected 2026-09-27 runtime has a fresh Debug/Release manifest and
Release live checkpoint. Range replay and canonical overlay pass with exact
ROM/build/contract identity, 0 coverage gaps or overlaps, exact bitmap
roundtrip, and zero SOURCE_OWNED delta. Neutral-input parity matches at 120,
1,200, and 10,000 frames, and the 3,600-frame controlled-input trace also
matches. Three-repeat A/B/C benchmarks are recorded; A→B has a managed-assembly
difference and remains qualified. M2 has bounded live acceptance. M3 has
direct Z80 data-read evidence plus the `ARG16()` regression; Z80 instruction
bytes remain open. M4 has direct VRAM source evidence; CRAM/VSRAM are
unobserved. M5 live provenance is unaccepted. M9 remains partial pending
full-game parity and remaining subsystem coverage. See
`docs/reports/THOR_RUNTIME_ROM_PROPERTIES_RESEAL_20260927.md`.
