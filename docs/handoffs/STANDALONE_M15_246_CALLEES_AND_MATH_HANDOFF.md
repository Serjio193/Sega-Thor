# Standalone Track Handoff — M15.246 Connected Callees, Math/Shift Lowering, and Long-Run Capture

> **Scope & Repository Rule:** `C:/Github/Sega-Thor-Standalone` was kept strictly read-only and unmodified during M15.246 (`529900ce0382c0c85f500762cf52b8704ecb8716`). This handoff summarizes the standalone-ready artifacts produced in `C:/Github/Sega-Thor`.

## 1. Standalone Thor IR & Auto-Split C++20 Module (`VERIFIED`)
- **IR & Manifest:** `src/tools/thor_ir/m15_246_callee_ir.json` and `data/thor-fixtures/m15_246_ir_manifest.json`
- **Generated C++20 Headers:** `src/tools/thor_ir/generated_m15_246/exp246_dispatch_*.hpp` (all files <= 260 lines, zero GPGX/Musashi dependencies)
- **Compiled Footprint:** `375` M68000 instructions, `104` CFG basic blocks, `183` CFG edges across 7 unowned roots:
  - `0x00F0D2` (`callee_f0d2_dist2d` -> calls `0x00E6BA`)
  - `0x00E6BA` (`callee_e6ba_isqrt` — 32-bit integer square root)
  - `0x006DD2` (`state_3_entity_action` — entity state offset `0x06`)
  - `0x00BCBA` & `0x00BCD8` (`callee_bcba_hitbox_scan` & alt entry)
  - `0x015AE2` (`callee_15ae2_entity_phys` — entity ground displacement / physics)
  - `0x02ED20` (`callee_2ed20_viewport_proj` — viewport screen projection)
- **Disjoint Ownership:** `0` overlap with the `196` active production helpers, `0` overlap with M15.244 (`130` ops), and `0` overlap with M15.245 (`508` ops). Combined portable IR coverage is now `1,013` distinct static M68000 instructions.

## 2. Standalone Arithmetic, Shift, MOVEM & Indirect Call Primitives (`VERIFIED`)
Implemented in `src/tools/thor_ir/generated_m15_246/exp246_dispatch_common.hpp` via `PortableIrBus246`:
- **`MULS.W` / `DIVU.W` / `DIVS.W`:** Exact M68000 Booth (`m15246_calc_muls_cycles`) and restoring/non-restoring division cycle models (`m15246_calc_divs_cycles`, `m15246_calc_divu_cycles`) verified to 0-cycle tolerance against pinned GPGX `ca4be304...`.
  - Division overflow sets `N|V` (`0x0A` with `X` preserved) and leaves destination register unmodified (verified against GPGX `0x02EE58` overflow fixture).
  - Division by zero yields `PortableContinuationOutcome::OriginalCpuException` with `exception_vector == 5U` before mutating registers or stack.
- **`MOVEM.L` (`-(An)` / `(An)+`) and `MOVEM.W` (`(An)+`):** Exact pre-decrement (`A7..A0, D7..D0`) and post-increment (`D0..D7, A0..A7` with sign extension on `.W`) loops + alignment checks (`Vector 3` on odd address) and exact `8+8n` / `12+{4,8}n` timing.
- **`ASL.L`, `LSL.W`, `LSR.W` (`m15246_shift_imm`):** Iterative 1-bit shift loop preserving exact `V` (signed MSB change across any step of `ASL`), `C`, `X`, `N`, `Z` flags and `(6/8 + 2*cnt)` cycles.
- **Addressing & Misc:** `EXT.W`/`.L`, `EXG Dx, Dy`, `NEG`, `SWAP`, `SF`/`ST Dn`, `ADDA.W`/`.L`, `CMPA.W`/`.L`, `LEA d8(An, Xn), Am` (`disp_ax`), and `JSR (An)` indirect stack call + odd-target `Vector 3` guard.

## 3. Cross-Boundary Continuations & Verification Status
| Component | Status | Notes |
|---|---|---|
| M15.246 IR Mode A vs Mode B vs GPGX Oracle | **VERIFIED** | `30/30` fixtures (`18` authentic 3,600f gameplay + `12` controlled CPU probes) pass bit-exact in Debug & Release |
| Combined M15.244 + M15.245 + M15.246 Execution | **VERIFIED** | Cross-module `0x006E0A -> BSR 0x006E7A -> 0x006E28 RTS` and `0x00E682` `+8 A7` unwind pass |
| Negative & Mutation Contracts (`12/12`) | **VERIFIED** | Opcode guard, sign ext, branch/JSR targets, A7 delta, mem width, cycle delta, IRQ/yield, Vector 2/3/5 traps |
| Long-Run 3,600f Hybrid Gameplay Parity | **PARTIALLY_VERIFIED** | Verified inside hybrid runner (`m15246p`) against GPGX; standalone runtime still relies on uncompiled boundaries for hardware MMIO/VDP/Z80/audio & uncompiled callees (`0x00BE98..`, `0x00E66E`, etc.) |
| Full Standalone Native Frame Loop | **UNVERIFIED** | Outside M15.246 scope; decompressor `0x003820` remains quarantined |
