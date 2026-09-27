# M12 map-driven executed ASM closure 2I.3

Status: `PASS_POSTRUN_MAP_DRIVEN_ASM_CLOSURE_V1`

Stage 7 was ported from the accepted 2F proof rules into the current 2I
post-run pipeline. It reads the accepted canonical generation
`gen-b18a8a86d5d2eb93`, recomputes candidates from current executed M68K
objects, `EXECUTED_FROM_ROM`, and observed `EXECUTED_NEXT` relations, and
does not launch BizHawk or add runtime evidence.

The current map contained 471 executed instruction objects and 244 executed
objects that were not fully source-owned. Thirty disjoint candidate islands
were audited with the exact range decoder, control-flow closure, vasm
round-trip, and full-ROM rebuild gates. Four islands individually reached
`PASS_CLOSED_ASM_RANGE`; dependency closure retained only
`[0x002AA4,0x002ACE)`, 42 bytes and 14 executed instructions. The accepted
promotion changed `SOURCE_OWNED` from 1,475,600 to 1,475,642 bytes and kept
all non-code emission bytes unchanged. The new canonical generation is
`gen-d2f971f0afc879c4` with map hash
`bbd0eebe3cf7a1dcacf528394852e6f9471700c47e6055c020669643cfcdbeec`.

Stage 8 automatically rebuilt the complete 3,145,728-byte ROM independently
and matched SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
A second Stage 7 invocation recomputed 29 remaining islands, promoted zero
bytes, and repeated the independent full-ROM audit (`NO_DELTA`, idempotent).
Blocked islands remain explicit `STOP_CONTROL_FLOW_ESCAPE_UNRESOLVED` records;
they are not promoted and do not become inferred runtime facts.

The canonical pointer remains under `generations/`, and the generated
materialized split, SQLite maps, and audit files remain ignored local
evidence. Production AUTO67, Worker 1B scaling, predecessor logic, and
Worker/FLOW runtime semantics were not changed.
