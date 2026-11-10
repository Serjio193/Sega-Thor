# ROM_00E682_STACK_CONTRACT_v1 — Cross-Track Semantic Contract for Routine `0x00E682`

**Contract Version:** `v1.0`  
**Date:** `2026-11-10`  
**Target Routine:** `entity_timer_tick_and_yield` (`0x00E682..0x00E6BA`, 56 bytes)  
**ROM SHA256 (`Beyond Oasis (USA).md`, 3,145,728 bytes):** `EB19BDA4982366A2FD43D65AB8A7F9709D83A8CC902C14A682C088C16359C263`  
**Hybrid Worktree Policy:** Read-only handoff document stored in standalone repo (`C:/Github/Sega-Thor` remains untouched).

---

## 1. ROM_BYTES & ROM_SHA256

- **56-byte complete routine (`0x00E682..0x00E6BA`):**
  `SHA256 = F1A4096C140D7D70DD02E27660AA63DE9F6192D14894A790BF3B1D0A913BE02E`
  `Hex: 08 AE 00 06 00 36 10 2E 00 9D 66 00 00 1C 10 2E 00 82 66 00 00 04 4E 75 52 6E 00 2E 53 40 1D 40 00 82 67 02 58 8F 4E 75 0C 00 00 14 64 00 FF F6 08 EE 00 06 00 36 60 EC`
- **40-byte primary body (`0x00E682..0x00E6AA`):**
  `SHA256 = E1C1CA4F7CE879E1BCDD6B4141DD5EB3381E4468142719C13CC183085D1AF71C`
- **38-byte M16.20 prefix (`0x00E682..0x00E6A8`):**
  `SHA256 = 80E633200E87EC73688A79F27E07B71A8C4F971EA603D7C8614BB7333710115F`

---

## 2. INDEPENDENT_DISASSEMBLY

```m68k
0x00E682: 08 AE 00 06 00 36    bclr.b #6, $0036(a6)     ; Clear sprite flash-hide bit 6
0x00E688: 10 2E 00 9D          move.b $009D(a6), d0      ; Read stun/freeze counter +0x9D
0x00E68C: 66 00 00 1C          bne.w  $00E6AA            ; If frozen ($9D != 0), skip timer logic and pop stack
0x00E690: 10 2E 00 82          move.b $0082(a6), d0      ; Read action/hitstop timer +0x82
0x00E694: 66 00 00 04          bne.w  $00E69A            ; If timer != 0, branch to decrement path
0x00E698: 4E 75                rts                       ; Normal return to immediate caller (SP += 4)
0x00E69A: 52 6E 00 2E          addq.w #1, $002E(a6)      ; Increment animation/delay step counter +0x2E
0x00E69E: 53 40                subq.w #1, d0             ; Decrement timer word in D0
0x00E6A0: 1D 40 00 82          move.b d0, $0082(a6)      ; Store new byte timer back to +0x82
0x00E6A4: 67 02                beq.s  $00E6A8            ; If timer just reached 0, normal return (do NOT pop caller!)
0x00E6A6: 58 8F                addq.l #4, a7             ; STACK SKIP: Discard immediate caller return PC from A7
0x00E6A8: 4E 75                rts                       ; Return to OUTER caller (if 0x00E6A6 ran) or immediate caller

0x00E6AA: 0C 00 00 14          cmpi.w #$0014, d0         ; Check freeze threshold (>= 20)
0x00E6AE: 64 00 FF F6          bcc.w  $00E6A6            ; Jump directly to stack pop + yield
0x00E6B2: 08 EE 00 06 00 36    bset.b #6, $0036(a6)      ; Set flash/hide bit 6 while near end of freeze
0x00E6B8: 60 EC                bra.s  $00E6A6            ; Unconditionally jump to stack pop + yield
```

---

## 3. CALL_GRAPH & RETURN_PC

- **Static ROM `JSR $00E682` count:** `40` call sites across entity update handlers (`0x0057D0` Player, `0x0256B8` Entity `0x002C`, `0x01F08A` / `0x01F25A` / `0x01F41C` Entity `0x006E`, etc.).
- **Typical 2-level call nesting at `0x00E682` entry:**
  - Outer dispatcher `0x00DF16: jsr (a0)` (pushes outer return PC **`0x0000DF1C`** onto `A7`), iterating entities by stride `0xBC` in `A6`.
  - Entity handler (e.g. `0x0256B8: jsr $00E682`) pushes inner return PC **`0x000256BE`** onto `A7`.
  - When `0x00E6A6: addq.l #4, a7; rts` runs, **`0x000256BE` IS SKIPPED** and execution transfers directly to **`0x0000DF1C`** (aborting the rest of the entity's state update for this tick while leaving the dispatcher's loop intact).

---

## 4. LIVE CAPTURED STACK_TRACE (Pinned GPGX `canonical_beach_frame3600.state`)

### Case A: Stack-Pop / Caller-Bypass Path (`timer82_before = 2 -> 1`, Frame 795, Slot 5 Entity `0x002C`)
- **ENTRY_PC:** `0x00E682`
- **CALLER_PC:** `0x0256B8` (`caller_ret_pc = 0x000256BE`)
- **ENTRY_A7:** `0x00FF0BD6`
- **STACK_BYTES_BEFORE (`A7+0..+12`):** `['0x000256BE', '0x0000DF1C', '0x0000000F', '0x00FF1D94']`
- **SKIPPED_RETURN_ADDRESS:** `0x000256BE`
- **ACTUAL_RTS_RETURN_ADDRESS:** `0x0000DF1C`
- **EXIT_A7:** `0x00FF0BDE` (`ENTRY_A7 + 8`)
- **EXIT_PC:** `0x0000DF1C`
- **CALLER_BYPASSED:** `TRUE` (`0x0256BE` skipped; returns directly to entity loop `0x00DF1C`)
- **EXECUTED_INSTRUCTIONS:** `0xE682 -> 0xE688 -> 0xE68C -> 0xE690 -> 0xE694 -> 0xE69A -> 0xE69E -> 0xE6A0 -> 0xE6A4 -> 0xE6A6 -> 0xE6A8 -> 0xDF1C`
- **CYCLES:** `994` GPGX master cycles (`~142` M68K clocks at 7x ratio, sample 1) / `924` master cycles (`~132` M68K clocks, Player sample 2 at F797: `0x0057D0 -> 0x00558A`)
- **RAM_SIDE_EFFECTS:**
  - `($36 + A6)` bit 6 cleared (`bclr #6, $36(a6)`)
  - `$2E(A6)` incremented `0 -> 1`
  - `$82(A6)` decremented `2 -> 1`

### Case B: Normal Return Path (`timer82_before = 0`, Frame 781, Slot 5 Entity `0x002C`)
- **ENTRY_PC:** `0x00E682`
- **CALLER_PC:** `0x0256B8` (`caller_ret_pc = 0x000256BE`)
- **ENTRY_A7:** `0x00FF0BD6`
- **EXIT_A7:** `0x00FF0BDA` (`ENTRY_A7 + 4`)
- **EXIT_PC:** `0x000256BE`
- **CALLER_BYPASSED:** `FALSE`
- **EXECUTED_INSTRUCTIONS:** `0xE682 -> 0xE688 -> 0xE68C -> 0xE690 -> 0xE694 -> 0xE698 -> 0x256BE`

---

## 5. RAM_FIELDS, CONDITIONAL_PATHS & UNRESOLVED_BEHAVIOR

- **RAM Fields Touched:**
  - `+0x2E` (word): Entity animation/yield tick counter incremented on `$82 > 0`.
  - `+0x36` (byte, bit 6): Sprite flash-hide flag cleared at `0x00E682`, optionally set at `0x00E6B2`.
  - `+0x82` (byte): Action/hitstop countdown timer decremented when `> 0`.
  - `+0x9D` (byte): Freeze/stun timer checked at `0x00E688` (if `!= 0`, bypasses `+0x82` decrement and jumps to `0x00E6A6` stack pop).
- **Hybrid IR Impact Note:**
  - Any single-block IR translator assuming `0x00E682` returns cleanly with `SP += 4` will corrupt stack/return control flow whenever `$82(a6) >= 2` or `$9D(a6) != 0`, because `0x00E6A6` increments `A7` by 4 (`SP += 8` across the `RTS`), bypassing the immediate caller (`0x0256BE` / `0x0057D6`).
  - Native C++ / IR interpreters MUST model the non-local return (`CALLER_BYPASSED = TRUE` when `$9D(a6) != 0` or `($82(a6)_before > 1)`).
- **UNRESOLVED_BEHAVIOR:**
  - In `0x00E688`, `move.b $9D(a6), d0` writes only bits `0..7` of `D0`, whereas `0x00E6AA: cmpi.w #$0014, d0` compares bits `0..15`; no caller observed in F775..F815 or F1900..F1940 entered with `$9D(a6) != 0`, so callers' upper `D0` bits on that branch are noted from static ROM analysis only (`STATIC_ANALYSIS_ONLY`).

---

## 6. CROSS_TRACK_RECONCILIATION (M15.243 / M16.22 — `2026-11-10`)

- **Cross-Track Agreement Summary:** Audited against hybrid track `C:/Github/Sega-Thor` (read-only) M15.243 verification:
  - **Routine identity & span:** `entity_timer_tick_and_yield`, `0x00E682..0x00E6AA` (`40` bytes, `SHA256 = E1C1CA4F7CE879E1BCDD6B4141DD5EB3381E4468142719C13CC183085D1AF71C`) and `38`-byte prefix (`SHA256 = 80E633200E87EC73688A79F27E07B71A8C4F971EA603D7C8614BB7333710115F`) match 100%.
  - **Slot stride & stack-pop semantics:** Stride `0xBC` and `0x00E6A6: addq.l #4, a7; rts` non-local return (`CALLER_BYPASSED`) match 100%.
  - **Static caller census (`40 JSR + 4 BSR = 44`):** Independent ROM binary verification confirms `40` `JSR $00E682` sites plus `4` `BSR.W $00E682` sites at `0x01255A`, `0x012F3C`, `0x01388A`, and `0x014848` (`44` total static callers), eliminating the earlier JSR-only ambiguity.
