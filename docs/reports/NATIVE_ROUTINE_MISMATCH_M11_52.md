# M11.52 — Native routine checkpoint mismatch root-cause closure

## Result

`FIRST_PORTABLE_NATIVE_ROUTINE_PROVEN`

The M11.51 checkpoint mismatch was a hybrid/GPGX continuation defect. The
portable `TableCopyRoutine` data and memory semantics were already correct, but
the native adapter executed the whole routine as one lump-sum timing handoff.
That skipped the GPGX instruction-boundary prefetch/refresh processing which
the original interpreter performs between represented instructions.

The repair is developer-only and generic at the hybrid bridge boundary. It
does not change `oasis_core` semantics, canonicalization, checkpoint bytes or
gameplay behavior. The generated/interpreter path remains available as the
shadow oracle and fallback for other routines.

## Frozen identities and reproduction

All runs used the unchanged canonical USA ROM
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263` and the
pinned instrumented GPGX library
`140c00fc7475cf22ca22415d65dfc7ffc66523de128454f9994e7ab77d9826fd`.

Two unchanged reference runs and two unchanged M11.51 native runs were each
internally deterministic. Frame 60 matched; the first reference/native
checkpoint identity difference was frame 120.

| Measure | Reference / repaired native | M11.51 native |
| --- | ---: | ---: |
| checkpoint aggregate | `251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d` | `ae8887f5b32a4973a8243775612d68b891b588f6f5dc693558a5a8a2489e5403` |
| video sequence | `5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58` | identical |
| interpreter instructions | 6,488,739 | 6,488,739 |
| native routine instructions | 34 | 34 |
| total instructions | 6,488,773 | 6,488,773 |
| copy iterations | 13 | 13 |
| fallback calls | 0 | 0 |

## Exact four-byte mismatch

The M11.51 native run differed from the reference after canonicalizing the
pinned serialized state with the unchanged M11.43 representation layout. The
four isolated bytes were:

| Serialized offset | Reference | M11.51 native | Classification | Owner/evidence |
| ---: | :---: | :---: | --- | --- |
| 3060 | `F4` | `EE` | `RAM` | Genesis work RAM `0xFF0BE4` (`0xFF0000 + 3060 - 16`), CPU-visible semantic memory |
| 144468 | `EE` | `F4` | `PSG` / sound semantic region | Pinned object layout between YM2612 and Z80; exact PSG field name remains unresolved because M11.43 documented a source/object sound-layout discrepancy |
| 144482 | `13` | `2F` | `PSG` / sound semantic region | Same pinned sound-region evidence; outside all host representation spans |
| 144558 | `96` | `87` | `Z80` / interrupt state | `Z80_Regs` base `144504` plus `54`, exactly `iff1`; outside the function-pointer representation span |

The bytes were not host pointers, function pointers or ABI padding. The
unchanged canonicalizer therefore remains correct and was not modified. Raw
serialized buffers from reference and repaired native runs still differ in
representation-only bytes, as expected; their ten per-frame canonical state
hashes and aggregate are identical.

## Temporal localization and shadow blind spot

Before entry, the routine state and source/stack contract matched. Portable
registers, ordered writes, output, restored stack and return PC also matched at
the routine boundary. The M11.51 native call recorded:

```text
entry cycles  = 193626       exit cycles  = 196454
entry refresh = 193808       exit refresh = 194704
```

The reference/shadow call recorded the same cycle values but:

```text
entry refresh = 193808       exit refresh = 196622
```

The old adapter advanced `2828` cycles once and applied one `896`-cycle bus
refresh update. The interpreter processed instruction boundaries, so GPGX
sampled refresh during the routine. The first observable downstream changes
were therefore scheduler-visible RAM, sound and Z80 interrupt-state bytes;
video remained unchanged.

This explains the M11.51 zero-divergence shadow result. Its comparison ended
at portable routine state/output/stack and did not compare the GPGX continuation
fields or the first scheduler boundary. The blind spot is classified as:

`SHADOW_FIELD_NOT_COMPARED`, `POST_RETURN_STATE_NOT_COMPARED`,
`PREFETCH_NOT_FULLY_MODELED`, and `SCHEDULER_PHASE_NOT_COMPARED`.

## Minimal repair

`CandidateApi` now exposes the existing developer-only GPGX operations for
instruction fetch, instruction begin and instruction finish. The 2D66 adapter
uses those callbacks for every represented instruction, validates the exact
opcode/extension-word stream, preserves DBF taken/not-taken PC and cycle
behavior, performs the existing MOVEM dynamic timing operation, and sets the
RTS return/prefetch state only after the stack read. The previous lump-sum
`2828` cycle update and one-shot `skip_bus_refresh` handoff were removed.

This is a hybrid continuation repair, not a portable semantic change and not a
checkpoint-byte or aggregate-hash exception. No new routine or hardware
behavior was added.

The focused candidate regression checks the exact instruction fetch/finish
sequence, DBF extension handling, MOVEM/DBF timing adjustments and that the
old one-shot refresh path is not used. It also reproduces the M11.51 boundary
model and asserts that its refresh result is not the reference result.

## Authoritative retry

The repaired 600-frame `NATIVE_OVERRIDE 0x2D66` run completed with:

```text
mode=NATIVE_OVERRIDE target=0x2D66 natural_calls=1 comparisons=0 divergences=0 override_calls=1
entry cycles  = 193626       exit cycles  = 196454
entry refresh = 193808       exit refresh = 196622
```

All ten canonical checkpoint hashes matched the reference manifest, yielding
the frozen aggregate and exact video identity above. The run had zero fallback,
zero original body starts, zero external interrupts during the call, 34 native
routine instruction executions, 13 iterations and 6,488,773 total guest
instructions. A repaired `SHADOW_NATIVE` run also completed with one comparison
and zero divergence, with the same cycle/refresh boundary values.

The runner's legacy `full_cpu_equivalence` JSON field remains conservative for
standalone `NATIVE_OVERRIDE` runs because it does not itself execute the paired
reference manifest. The authoritative promotion claim in this report is based
on the paired per-frame canonical manifest, raw-diff classification and the
independent continuation regression above.

## Scope and next step

M11.52 closes only the selected `TableCopyRoutine` replacement boundary. It
does not authorize another routine, a full CPU emulator, sound/VDP promotion,
or canonicalization changes. Future routines must independently prove their
own continuation and serialized-state contract.
