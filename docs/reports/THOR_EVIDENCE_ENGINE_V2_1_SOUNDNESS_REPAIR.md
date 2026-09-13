# THOR Evidence Engine V2.1 — RAM provenance soundness repair

Status: **READY_FOR_REAUDIT**. V2.1 repairs only the bounded V2 defects found
against baseline `362b45691bf1f292ace2ba58a0f5b20f64a1c579`. No V3 feature,
ROM reverse engineering, runtime change, or SOURCE_OWNED promotion was added.

## ASTRA counterexample matrix

| Finding | Before | Fix | Executed regression | After |
|---|---|---|---|---|
| forged FF188A coverage | metadata changed `INCOMPLETE_CAPTURE` to `PROVEN` | unverified claims rejected; certificate needs contiguous capture basis and receipt lineage | construction and truncated-basis tests | claim rejected; no proof |
| future version | query before write returned the later WRITE version | immutable roots are separate from current state | temporal root identity test | root ID/origin returned |
| empty boundary | zero-length interval implicitly covered | verified non-empty coverage required | epoch-boundary test | no implicit `PROVEN` |
| partial long write | `FFFFFF` write left a ghost byte | range validated before mutation | atomic mutation test | operations, versions and indexes unchanged |
| overlap drop | adapter filtered to exact `FF13CC` | overlapping writes are retained or reject as unknown | adapter overlap guard | no silent discard |
| SQLite splice | outer trace/epoch overwrote embedded identity | payload digest, trace, epoch and references checked atomically | import identity checks and retry | inconsistent payload rejected |
| V1 bypass | old aggregate target was separate from V2 | target exposes four V2 byte IDs; explain and persistence use them | real derive/import check | V2 chain is concrete and reloadable |

## Coverage trust model

`CoverageCertificate` is now an untrusted claim. `RamVersionEngine.add_coverage`
accepts only `VerifiedCoverageCertificate`. Its construction requires the exact
trace/raw hash, epoch, non-empty interval, byte scope, receipt hash, decoder and
rule identity, execution instances, and a contiguous source-event basis. The
certificate stores a content hash of that basis. A matching string alone cannot
authorize `PROVEN`.

The FF13CC adapter builds its certificate from the actual bounded raw event
sequence and validated launch receipt. It no longer widens coverage from fixed
interval constants. Writes with a possible physical intersection that are not
the checked A372 writer fail closed.

## Temporal query and atomic write semantics

Each address has an immutable pre-capture root and a separate current version.
Queries before the first write return the root, never a future writer. A query at
the epoch boundary without verified coverage is not a proof. A write validates
the complete physical range before creating any version or index entry. An
identical immutable event is an idempotent no-op; a different payload with the
same identity is rejected.

## SQLite identity contract

RAM imports validate the embedded trace/epoch, operation and byte-version
identity digests, output references, coverage lineage fields, and referenced
capture epochs before commit. A raw artifact hash is accepted only when the
sealed capture's validated receipt maps it to the requested logical trace.
`import_provenance` keeps RAM import inside its outer transaction, so a later
RAM failure rolls back earlier provenance rows. Existing duplicate import and
reopen export determinism remain unchanged.

## V1 → V2 integration

The selected first A372 execution is `EXEC seq 17`, paired with `WRITE seq 18`
at `FF13CC`. The V2 operation ID is carried by the target and produces four
concrete byte-version IDs for `FF13CC..FF13CF`; the values remain `00 88 09 01`.
`explain()` traverses a V2 byte ID to its operation, execution identity and raw
witnesses. SQLite persists the operation, four output versions, roots and
coverage; the same export is idempotent and survives reopen. The legacy graph
ID is retained only as an explicit compatibility field and is not the target
RAM version.

## FF188A negative oracle

The historical `v01r3-probe-a.raw.jsonl` evidence remains incomplete. A forged
claim with the correct raw hash cannot enter the engine because it lacks a
verified contiguous basis and receipt lineage. No new FF188A runtime evidence
was collected.

## Executed adversarial test matrix

The V2 test executes overlap variants (long/word/byte, partial, unaligned and
same-value), epoch isolation, coverage gaps, unknown transforms, reordered
writes, untrusted construction, truncated basis, temporal root identity,
epoch-boundary behavior, failed `FFFFFF` long writes, duplicate immutable
events, SQLite rollback/retry/idempotence/reopen, and bounded lookup timing.
The real V1 derive path was also run with the checked canary artifacts and
persisted through SQLite. Release CTest passed all five evidence helper tests.

## V1 C01–C12 recheck

The existing V1 canary validator remains green. ROM high24 and incremented
D5.low8 remain separate dependencies; no PC-2 heuristic, low-byte ROM edge,
address-only edge, or input-causal edge was introduced. The selected A372
execution/write pairing is concrete and is now linked to the V2 operation.

## Performance and ownership

The existing bounded lookup timing remains below its 2,000-query test budget.
The implementation still has quadratic growth for much larger synthetic
streams; optimization remains outside V2.1. SOURCE_OWNED is unchanged:

`before = 1,475,368 / 3,145,728`
`after  = 1,475,368 / 3,145,728`
`delta  = 0`

Access-width completeness outside the checked canary, global writer
completeness, IRQ/exception interaction, input causality, DMA/VRAM and broad
register/control provenance remain explicit UNKNOWNs.

## V3 readiness and stop reason

V3 is **not authorized**. The bounded V2.1 repair is ready for independent
re-audit after publication. Stop after commit, push, and exact final-SHA CI;
do not start V3.

FINAL SHA: `7b054d2bd8fc406eef04a0c62a5cb2b8484dbc4b` (implementation commit)
CI: pending publication of this report commit
