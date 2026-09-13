# THOR Evidence Engine V2 — reusable RAM byte-version and last-writer provenance

V2 result: **PASS** for the bounded reusable RAM primitive.

BASELINE SHA: `1cf1038fd7b9e09a86421de47b86430689cb525e`.
FINAL SHA: populated by the publication commit.

The implementation is developer-only evidence tooling. It does not alter the
native runtime, ROM ownership, reverse-engineering map, or SOURCE_OWNED bytes.

## RAM version model

`src/tools/thor_evidence/ram_versions.py` is a reusable byte-level temporal
SSA/lifetime engine. Each byte identity includes trace, epoch, address, version
number, temporal sequence and origin. Initial bytes are explicit
`PRE_CAPTURE_ORIGIN` or `EXTERNAL_STATE` roots. A proven reset origin can be
represented as `RESET_INITIALIZATION`; no producer is invented for an initial
byte.

Writes use big-endian 68000 ordering. Byte, word and long writes create one
new version per physical byte, even when the numeric value is unchanged. An
overlap replaces only the covered addresses, retaining the earlier versions
for all other bytes. Epoch state is isolated and equal address/value pairs in
different epochs receive different identities.

## Write operation model

Every operation retains trace, epoch, execution instance, PC, decoded rule,
width, effective address, byte range, raw witness list, previous byte-version
IDs and resulting byte-version IDs. A long write is one operation with four
outputs. The query returns the concrete operation and concrete byte version,
not a numeric-value or address-only merge.

## Coverage contract and last-writer query

`CoverageCertificate` binds a complete address scope and temporal interval to
the engine trace identity and a 64-character evidence hash. `last_writer()`
requires coverage from the epoch capture boundary through the query point. It
returns:

* `PROVEN` with operation/version IDs only when that interval is covered;
* `EXTERNAL_STATE` when complete coverage shows no write before the query;
* `PRE_CAPTURE_ORIGIN` for a query before the epoch boundary;
* `INCOMPLETE_CAPTURE` when an observed writer is separated from the query by
  a coverage gap;
* `UNKNOWN_TRANSFORM` for an explicitly unsupported writer rule;
* `CONFLICT` when overlapping writers share one temporal point.

The implementation never equates the latest observed callback with a proven
last writer. Temporal insertion is ordered; reordered events are rejected.
Per-address sequence indexes keep ordinary lookup demand-driven rather than
scanning every operation.

## Overlap test matrix

| Case | Result |
|---|---|
| long at X, then byte at X+1 | X remains long; X+1 is byte; X+2/X+3 remain long |
| word over long | only the two covered big-endian bytes are replaced |
| long over word | all four physical bytes become a new long operation |
| byte over word | one physical byte is replaced |
| same-value overwrite | a new version and operation are created |
| unaligned overlap | represented by physical addresses; no value deduplication |

## Epoch test matrix

Epoch 1 and epoch 2 hold separate current-byte maps and version counters.
Equal address/value/PC writes do not leak across restore boundaries. A query
before an observed writer resolves to the explicit initial-state root; a query
after an uncovered interval returns `INCOMPLETE_CAPTURE`.

## V1 regression result

The existing FF13CC canary now constructs its six A372 evidence writes through
the reusable engine. For the first selected write, four concrete queries at
`FF13CC..FF13CF` all return `PROVEN`, and reconstruct `00 88 09 01`. The V1
provenance branches remain unchanged: ROM high24, incremented counter low8,
FF188C address formation, FF1858 control selection, and explicit A372
EXECUTION witness.

The rebuilt local certificate SHA is
`d0ef5abff91d1fb6ae2d9a9fa4201d911d1ea21e9b0239ba688030c7a7000e67`.

## Held-out RAM canary

Held-out target: `FF188A`, selected because the existing accepted V0 trace
already contains the documented A42A/A430, B000/B006 and ACE6/ACEC writer
observations and it is outside the FF13CC output query.

Evidence: existing raw trace
`build/thor-evidence/v0/v01r3-probe-a.raw.jsonl`, SHA256
`7b80d5d7999f4623f193a07aa38120ab20a76c6fbbc6087cf901d149a2f1c5f8`.
The observed writer at sequence 82 records value `0006` for FF188A.

Querying that writer through the reusable engine without importing a new
coverage claim returns:

```json
{
  "status": "INCOMPLETE_CAPTURE",
  "address": 16717962,
  "temporal_point": 82,
  "explanation": "latest observed writer is not proven because coverage has a gap"
}
```

This is the required frontier: the engine works, but the existing evidence
does not justify a last-writer proof for FF188A. No new ROM reverse engineering
or synthetic coverage certificate was added.

## SQLite / idempotence

The existing evidence sidecar was extended with `ram_byte_version`,
`ram_write_operation`, `ram_write_output` and `ram_coverage`, plus lookup
indexes by trace/epoch/address/temporal order. `Store.import_ram_engine()` is
transactional, deterministic and idempotent; identity collisions fail closed.
No second provenance database exists.

## Performance

A bounded synthetic stream of 2,000 byte writes and 2,000 per-address queries
completed in approximately `0.20 s` locally. Address sequence indexes avoid
an accidental full operation scan for each query. This is a primitive-level
check, not a performance project.

## Negative tests

Persisted fixtures cover same address/value across epochs, same-value writes,
missing intermediate writers, byte/word overlap, reordered events, duplicate
import, truncated and forged coverage, query-before-writer, uncovered suffixes,
cross-epoch leakage, numeric deduplication and address-only provenance.
All unsafe cases fail closed. The V1 canary test and the new V2 RAM test are
registered with CTest.

## SOURCE_OWNED and known unknowns

SOURCE_OWNED before: `1,475,368 / 3,145,728`.
SOURCE_OWNED after: `1,475,368 / 3,145,728`.
Delta: `0`.

Access-width completeness outside the checked write operation, global writer
completeness, IRQ/exception interaction, input-read causality and broad
register/control provenance remain UNKNOWN. V2 does not generalize registers,
control flow, whole-game tracing or ROM ownership.

## V3 readiness

V3 is **not authorized** by this result. The reusable RAM byte primitive is
ready for another explicitly bounded evidence query, but register/control
provenance, causal reads, interrupt completeness and global coverage remain
separate gates.

## Tests / CI / decision quality / stop reason

Local V0, V1-canary and V2 RAM tests pass; Release CTest and the existing
Release smoke build are the required repository checks. The pre-existing full
Debug MSVC `std::to_string` error in `src/core/ram_flag_routine.cpp` remains
outside this scope. The V2 result uses only reusable temporal state plus
explicit coverage certificates; the held-out frontier is not promoted.

STOP REASON: V2 RAM byte SSA, overlap semantics, epoch isolation, coverage-aware
last-writer behavior, V1 regression and honest held-out frontier are closed.
Stop here; do not start V3.
