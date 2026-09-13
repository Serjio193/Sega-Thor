# THOR Evidence Engine V5 — static enumeration and non-owning Carver bridge

BASELINE: `c4234c376ffeb6c2756bea50866964e3291d3217`
FINAL SHA: publication commit; exact SHA and CI are recorded in the final gate response.
CI: exact publication CI recorded in the final gate response.
PASS/PARTIAL/BLOCKED: **PASS**

## Architecture added

`static_bridge.py` defines deterministic static request/response identities,
bounded structure/domain/boundary certificates, runtime-seeded static requests,
typed static records and a non-owning Carver export. It reuses the existing
`IntervalDB` and does not add a second promoter or ownership writer.

## Request/response contract

Requests bind trace, canonical ROM, half-open range, query kind, seed IDs and
constraints. Responses reject out-of-range records, malformed certificate
types and any `source_owned` or promotion transaction claim. Unresolved static
frontiers remain explicit.

## Runtime to static and static to graph

`request_from_runtime()` converts a bounded runtime seed into a static query.
Validated response records and certificates enter the existing Carver graph as
`EVIDENCE_ONLY` records and typed edges. The merge records identical
SOURCE_OWNED counts before and after and never changes manifest ranges.

## Carver evidence export

`carver_export()` emits the current IntervalDB and deterministic gap/campaign
report while retaining the V5 bridge lineage. Existing Carver adapters and
promoters remain authoritative; no automatic promotion is attempted.

## Tests

`thor_evidence_v5_test.py` covers request bounds, deterministic request
identity, ownership-claim rejection, non-owning merge, unresolved frontier
retention and Carver export. V0–V4 helpers remain green.

## Known defects carried

All V2.1, V3 and V4 ledger entries remain carried. Static responses are
evidence-only and do not repair fabricated V2 coverage, incomplete dynamic
effects, legacy graph termination, DMA timing or hardware alias gaps.

## New defects

No blocker discovered. V5 does not implement a universal 68000 enumerator,
indirect-CFG completeness, parser semantics or promotion authorization.

## Blockers repaired / deferred defects

No prior blocker was repaired. Static decoder/IR breadth, indirect targets,
exact boundary closure and conflict resolution remain deferred to later bounded
work and stabilization.

SOURCE_OWNED before/after/delta: `1,475,368 / 3,145,728` ->
`1,475,368 / 3,145,728`, delta `0`.

NEXT STAGE: V6 multi-scenario differential.

TRANSITION DECISION: V5 PASS. Continue sequentially to V6 after this stage's
publication and exact CI.
