# GPGX scheduler fixture — 2026-10-13

## Result

The isolated scheduler-capable fixture passes all ten ROM-versus-instruction-step
micro-cases with the canonical ROM. The test core is a dedicated copy of the
external GPGX checkout; the normal emulator core and production routing are not
changed. The comparison includes the complete ordered trace emitted by each
one-frame run, not only the synthetic instruction result.

```text
SCHEDULER_FIXTURE = VERIFIED (tested micro-case scope)
BLOCK_HOOK_INSTRUCTION_STEP = VERIFIED (MOVEQ, NOP, supported short branches)
REFRESH_EQUIVALENCE = YES
LINE_BUDGET_EQUIVALENCE = YES
Z80_VDP_INTERLEAVE_EQUIVALENCE = YES
IRQ_BOUNDARY_EQUIVALENCE = YES
TIMING_PRIMITIVE_STATUS = VERIFIED (tested micro-case scope)
9D70_REPLAY_NATIVE_ROUTE_EQUIVALENT = NO
RUNTIME_REPLACEMENT_ACTIVE = NO
AUTHORITY_DELTAS = 0
```

The fixture validates instruction stepping across the scheduler cases below. It
does not establish timing equivalence for arbitrary M68K instructions or for
routine `0x9D70`; that routine remains blocked on its own instruction path and
natural replay check.

## Fixture architecture

`prepare_scheduler_fixture.py` copies the prepared external GPGX source into the
dedicated ignored build directory `build/scheduler-fixture-20261013`, enables
`HOOK_CPU=1` only in that copy, then instruments `system_frame_gen()` and the
M68K core. `oasis_hybrid_scheduler_fixture` owns the call to
`retro_hybrid_fixture_run_frame()`, which enters GPGX's actual frame scheduler:
M68K line-budget slice, Z80 slice, VDP/scanline advancement, pending IRQ
processing, and next M68K slice. The fixture does not substitute a second
scheduler implementation.

The bridge reads native core state directly:

| Value | Source |
|---|---|
| M68K cycles, `cycle_end`, refresh deadline | `m68k.cycles`, `m68k.cycle_end`, `m68k.refresh_cycles` |
| M68K PC, SR, pending level/mask, stopped state | core register access and `m68k` fields |
| Z80 cycles and registers | `Z80.cycles` and `Z80` state |
| VDP scanline, line-cycle position, pending IRQ state | `v_counter`, `mcycles_vdp`, VDP fields |
| Z80 enable/bus state | `zstate` |
| instruction boundary reason | exact pending-interrupt, trace, and `cycle_end` predicates from the core |
| scheduler targets and IRQ transitions | ordered events at actual M68K/Z80 scheduler calls, VDP advances, and interrupt entry/exit |

Instruction entry/exit markers capture opcode, PC, cycles, refresh deadline,
SR, and post-instruction boundary reason. Bus writes are kept in order. The
instruction-step callback handles only MOVEQ, NOP, and selected short branches;
it charges the core's opcode cycle count and exact not-taken branch adjustment,
then returns to the real core loop so refresh and scheduler checks remain in
their normal order.

## Important instrumentation correction

The first IRQ attempt exposed a bridge defect: the observation hook returned
zero for every hooked CPU read, unintentionally changing ROM execution. The
bridge now returns the original value after notifying the observer. The IRQ
case then read the patched autovector as `0x00FF1200`, reached the synthetic
handler, and returned through RTE. The synthetic vector patch writes the
internal M68K memory map used by the exception path. Results from before the
transparent-read correction are invalid and are not part of acceptance.

## Micro-cases and results

Every case runs once with pure ROM and once with block-hook instruction
substitution. The acceptance runner compares ordered scheduler/CPU events,
instruction entries/exits, bus writes, final D/A/SR, scheduler state, Z80 state,
VDP state, and a 64 KiB work-RAM hash.

| Case | Coverage | Result |
|---|---|---|
| A_BEFORE | instructions complete before the line budget | PASS |
| B_EXACT | instruction reaches the line budget | PASS |
| B_NEAR | instruction finishes just before the line budget | PASS |
| C_CROSS | instruction crosses the line budget | PASS |
| D_TAKEN | conditional branch taken with its ROM cost | PASS |
| D_NOT_TAKEN | conditional branch not taken with core adjustment | PASS |
| E_REFRESH | refresh deadline without a line crossing | PASS |
| F_REFRESH_CROSS | refresh and line crossing in the same case | PASS |
| G_IRQ | level-4 IRQ pending, exception entry, handler/RTE | PASS |
| H_RESUME | substitute resumes on a later scheduler turn | PASS |

Each trace stayed below capture limits (maximum 28,451 events, 12,611
instruction entries, and 2 writes). The IRQ case recorded 262 Z80 scheduler
slices and 262 VDP advances, with one pending interrupt, one M68K IRQ entry,
and one IRQ exit to `0x00FF1200`. ROM and step traces were identical for all
compared fields in all ten cases.

## Build and receipt identity

- Canonical ROM SHA-256: `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`
- External GPGX source revision: `784e5580cb43103b98272a7349fcf927da67d0b9`
- Fixture core `HOOK_CPU=1`, SHA-256: `926fbcc5228c0a08c143e62388184bad230ee91001af2344f6fd04961f60387f`
- GNU/Linux Debug runner, SHA-256: `e884f4935e95f4e11f1fcd418acdcc8e1e130425b4131cadcdc12f2eea44b226`
- GNU/Linux Release runner, SHA-256: `1ab311f30326c83badf6d8ed4db62f196b26684d4d816b46401d8caea4a0b9dc`
- Machine receipts and full paired traces: ignored build directory `build/scheduler-fixture-20261013/acceptance-transparent-hook/` and `build/scheduler-fixture-20261013/acceptance-linux-release/`.

Both GNU/Linux Debug and Release runners passed all ten cases. MSVC Debug and
Release runner targets also built successfully; the Linux shared core was not
loaded by the Windows executables. No ROM or copyrighted asset was added to the
repository. Runtime, structural, and SOURCE_OWNED totals are unchanged.

## Next step

Use the verified stepping primitive only for an instruction path whose opcodes,
cycle costs, branch behavior, refresh interactions, and caller continuation
are separately closed. Then run the `0x9D70` instruction path through the same
fixture before reconsidering its timing-route equivalence. Do not enable
production replacement based on these synthetic cases.
