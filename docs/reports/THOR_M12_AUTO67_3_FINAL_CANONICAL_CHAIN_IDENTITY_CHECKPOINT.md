# THOR M12 AUTO67.3 — final canonical chain identity checkpoint

Status: **PASS — authoritative final checkpoint**

This report supersedes earlier AUTO67/AUTO67.1/AUTO67.2 iteration reports.
Those files remain historical evidence and are not authoritative for the final
counters below. The implementation stayed chain-store-first: no architecture
change, semantic merge, SOURCE_OWNED promotion, or AUTO68 work was started.

## Identity audit

The positive test varied only frame, epoch, worker, lease, session, and wall
clock. Canonical serialization remained identical and the SHA-256
`CHAIN_HASH` remained identical; only provenance changed.

The selected stored unresolved record contained only `kind`, `pc`, and
`address`. Changing the observed instruction `pc` or observed `address` changed
both canonical serialization and SHA-256. Caller/predecessor, reader/writer,
source/destination, pointer, consumer/producer, selector/value, terminal/root,
and unresolved-frontier fields were absent from that record and were skipped;
no unavailable causal facts were invented.

## Database inspection

The reopened final sidecar contains 264 `live_chain` records, 264 unique
hashes, and zero duplicate hash groups. Canonical body size is 48/73/73 bytes
(min/median/max). All 264 records are `BOUNDED_UNRESOLVED`; rooted records are
zero. The top repeated observation counts are 705, 642, 492, 467, 120, 120,
120, 120, 118, and 118. `live_chain.chain_hash` is the SQLite PRIMARY KEY,
and the restarted database has zero orphan chain records.

## Real BizHawk proof

All runs used BizHawk 2.11.1 with 16 workers and the native operator window.

| Run | Frames | Leases | Returns | Unique | Duplicates | Peak busy | Max frame | >50 ms | Seed age avg/max |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| QuickSave1, first | 1200 | 3368 | 3368 | 214 | 3154 | 16 | 32 ms | 0 | 6.658/16 ms |
| QuickSave1, same DB | 1200 | 3353 | 3353 | 39 | 3314 | 16 | 32 ms | 0 | 5.807/16 ms |
| QuickSave4, same DB | 1200 | 3346 | 3346 | 11 | 3335 | 16 | 37 ms | 0 | 5.935/31 ms |

The final totals are exactly 214 unique first QuickSave1, 39 unique and 3314
duplicates on the second QuickSave1, 11 new QuickSave4, and 264 unique chains
in the sidecar. Queue drops are 0, DB errors are 0, duplicate active claims
are 0, and the raw-event backlog is **NONEXISTENT**. Historical recoverable
chains are 0. Semantic relations are intentionally 0 because none were
witnessed; no semantic merging was added. `SOURCE_OWNED` delta is 0.

Rolling-window overwrite remained active in every 1200-frame run (1184
overwrites). Emulator responsiveness passed. The native window consumed only
bounded snapshots; it does not create a raw-event backlog or block BizHawk,
Dispatcher, workers, or persistence.

## Validation

- Python AUTO67 tests: 22/22 PASS.
- Legacy evidence tests v0, v1-gate, v2-ram, v3, v4, and v9: PASS.
- AUTO67 CTest helpers: 3/3 PASS; standalone read-only identity audit: PASS.
- Debug and Release builds: PASS.
- Source-file limit and `git diff --check`: PASS.

Machine-readable evidence is in
`docs/reports/THOR_M12_AUTO67_3_FINAL_CANONICAL_CHAIN_IDENTITY_CHECKPOINT.json`.
