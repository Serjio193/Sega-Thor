# M11.36 GPGX Timing / Refresh Bridge Contract

Status: `GPGX_TIMING_REFRESH_BRIDGE_PROVEN` and
`DEMAND_DRIVEN_BLOCK_PROMOTION_PROVEN`.

## Scope and identity

This bounded follow-up started at M11.35 commit
`291012425bdc85f37cb5c11ffede71223916e3a4`. It used only the existing
shortlist `0x3A85E`, `0x3A8BA` and `0x3A88C`, together with the preserved
M11.33 entries. The canonical USA ROM SHA-256 is
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
The external instrumented GPGX checkout is source commit
`d60d079934977aa6973e220d123533387159f66e`; the final developer-only DLL
SHA-256 is `9b345293c239805cbfe22bb3c582e7d42164a701ba2c50e1934e1a8ef80b2ec8`.

## Counter contract

Source evidence from that checkout:

- `core/m68k/m68k.h`: `m68ki_cpu_core.cycles` and
  `refresh_cycles` are signed master-cycle values; `cycle_end` is the current
  `m68k_run` target. The prefetch address/data and interrupt level are fields
  of the same CPU owner.
- `core/m68k/m68kcpu.c:m68k_run`: `m68ki_check_interrupts()` runs at entry;
  the normal loop fetches the opcode, applies the refresh penalty, executes
  the opcode, then applies `USE_CYCLES`. The block hook is called at the top
  of the loop before normal fetch.
- `core/system.c:system_frame` and `system_frame_scd`: at frame end GPGX
  subtracts `mcycles_vdp` from both `m68k.cycles` and `m68k.refresh_cycles`.
  They are therefore accumulated within a frame, not monotonic process-global
  counters and not independent block-local deltas.
- `core/m68k/m68kcpu.c:m68k_hybrid_begin_instruction` and
  `m68k_hybrid_finish_instruction`: generated execution invokes GPGX's own
  refresh check and instruction cycle table. The bridge does not supply a
  second timing table.

## Bounded A–G evidence

The first M11.35 failure was captured around natural `0x3A85E` execution. The
instruction is `TST.W ($00FF1654).L`, with direct successor `0x3A864`.

| point | authoritative boundary | relevant observation |
| --- | --- | --- |
| A | before normal interpreter execution | entry PC `0x3A85E`; pre-rebase counters `cycles=896080`, `refresh=896268` |
| B | after opcode/prefetch entry | IR `0x4A79`, PC `0x3A860`, prefetch `0x3A862`; counters unchanged |
| C | after TST semantic execution | PC `0x3A864`, prefetch `0x3A864/0x4E75`, full SR effect and RAM read exact; timing not yet charged |
| D | after GPGX refresh/timing advancement | expected absolute frame state `cycles=896114`, `refresh=896268` |
| E | before the interrupt/trace boundary | no pending external interrupt; mask remains `0x0100`; same counters as D |
| F | after boundary | no interrupt service occurred in this invocation |
| G | old comparison observer | after frame transition: `cycles=74`, `refresh=228`; both differ from D by `mcycles_vdp=896040` |

The first mismatch was therefore at the observer boundary, not at decode,
semantic execution, RAM access, refresh period or instruction cycle charge.
The corrected bridge emits a generic post-instruction event at D/E, before the
next scheduler/frame transition. Its comparison sees `896114/896268` and
closes the shadow; it does not normalize either counter.

## Interrupt boundary

GPGX polls interrupts at `m68k_run` entry and keeps trace/interrupt handling in
the authoritative core. The bridge's post-instruction event is emitted before
the subsequent trace/interrupt boundary. The six registered blocks have only
bounded ROM/work-RAM accesses in this scenario; the final 600-frame run
observed zero interrupt crossings and zero hardware-visible accesses inside
promoted blocks. A future block with an observable mid-block hardware or
interrupt boundary is not authorized by this result.

## Final gate

The final shadow run completed `185975/185975` comparisons with zero
divergences. Native promotion of the same six-entry registry completed 600
frames with `185975` native entries, `185981` translated guest instruction
executions, zero original target-body starts, zero fallback entries, zero
interrupts and zero hardware-visible accesses. Shadow and native both report
full CPU equivalence and the same checkpoint hash
`66e2a51afdd0ee4e790063b0e603dff77ae678b43e6c411c9da57ed2f27a3cba` and video
sequence hash
`5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`.

No ROM, asset, emulator binary or generated run evidence is repository input.
The DLL and run directories remain local developer artifacts; `game.srm`
remains untouched and untracked.
