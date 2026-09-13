# THOR Evidence Engine V3 register/control build

BASELINE: `f66be50d47bcb4c5f3b4edf32a71056fb82458ae`
FINAL SHA: publication commit; exact SHA and CI are recorded in the final gate response.

## BUILD PHASE STATUS

V3 BUILD: PASS. The module is usable for bounded offline analysis while V2.1
soundness defects remain explicitly carried.

## ARCHITECTURE ADDED

`register_versions.py` owns temporal D/A slices and checked local rules.
`execution_model.py` owns execution instances, bounded call/return relations
and local control facts. `v3_graph.py` exports role-preserving dependencies and
an explain path. `store.py` extends the existing SQLite sidecar.

## REGISTER VERSION MODEL

Identity contains trace, epoch, execution instance, register, bit slice and
version sequence. D-register byte/word writes replace only the low slice;
long writes replace all bits. A-registers are long-only. Same numeric values
remain separate versions. MOVEA.W and MOVEQ sign-extend their sources.

## SUPPORTED RULE MATRIX

The skeleton covers MOVE.B/W/L, MOVEA.W/L, MOVEQ, zero writes, LEA and named
effective-address modes, ADD/ADDI/ADDQ, SUB/SUBI/SUBQ, ADDA/SUBA, AND/OR/EOR,
LSL/LSR/ASL/ASR, EXT, SWAP, TST/CMP/CMPI-shaped control observations and
RAM_READ/RAM_WRITE interop. Unsupported transforms produce `UNKNOWN`.

## EXECUTION INSTANCE MODEL

The identity is `(trace, epoch, event sequence, PC, rule)`. Repeated PCs and
the same PC in another epoch cannot collapse.

## CONTROL MODEL

`ExecutionModel.branch()` records condition rule, selected execution, outcome,
status and witnesses. It is local/provisional and does not claim global
postdominator analysis.

## CALL/RETURN MODEL

Observed CALL and RETURN relations are bounded edges with witnesses. No global
call graph completeness is claimed.

## RAM INTEROP

V3 accepts the existing V2 RAM byte engine and records VALUE and ADDRESS edges
from register inputs to a RAM write. Inherited uncertain RAM status is not
upgraded by a known register rule.

## FF13CC INTEGRATION FIXTURE

The fixture represents FF188C-derived A5/address state, FF188A/D5 low-byte
update, ROM/high-byte plus D5 low-byte composition and a D2-to-RAM write. It
is an architecture fixture, not a new causal certificate.

## SECOND FIXTURE

The second fixture repeats a PC across execution instances and epochs while
exercising A-register construction, D-register slices, CALL/RETURN and a
branch fact. It uses existing evidence shapes and performs no new ROM RE.

## SQLITE MIGRATION

Six V3 tables are added to the existing schema for execution instances,
register versions/operations, control facts, execution relations and typed
dependencies. Imports are transactional, identity-bound and idempotent for
identical payloads; conflicts fail.

## GRAPH EXPORT / EXPLAIN API

The canonical graph retains VALUE, ADDRESS, CONTROL and EXECUTION roles. The
explain result includes target, producer, dependencies and unresolved frontier
for register or RAM byte versions.

## KNOWN DEFECTS CARRIED FORWARD

See `THOR_EVIDENCE_ENGINE_KNOWN_DEFECTS.md`. V3 does not repair fabricated V2
coverage, V1 validator bypass, legacy graph termination or the FF188A path.

## NEW DEFECTS DISCOVERED

None blocking construction. V3 is deliberately local and does not provide
full 68000 flag semantics, global control dependence, IRQ/exception causality,
or complete RAM writer coverage.

## BLOCKERS FIXED DURING BUILD

No V2.1 blocker was repaired. V3-only construction issues were kept bounded to
the new modules and their tests.

## DEFERRED DEFECTS / PERFORMANCE

Full symbolic CPU semantics, global call graph completeness, broad access
width/overlap guarantees and large-scale optimization remain deferred.

SOURCE_OWNED before/after/delta: `1,475,368 / 3,145,728` ->
`1,475,368 / 3,145,728`, delta `0`.

NEXT BUILD STAGE: V4 ROM ROOTS / RESOURCE / HARDWARE INTEGRATION.

STOP REASON: Stop after V3 implementation, report, commit, push and exact CI;
do not begin V4.

## VALIDATION

Windows Release build passed. Release CTest passed 169/169 when excluding the
existing repository-wide `project_file_line_limit` scan, which is dominated by
the large preserved untracked analysis workspace; the tracked source-size
check covered 575 files with zero violations. The six evidence helpers (V0,
V1 gate, dense gate, V1 canary, V2 RAM and V3) passed on Windows and in the
GNU/Linux-equivalent `/tmp` build. `git diff --check` and Python compilation
passed. No SOURCE_OWNED promotion or V4 work was started.
