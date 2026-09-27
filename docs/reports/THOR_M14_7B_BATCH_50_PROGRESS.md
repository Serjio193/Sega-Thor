# M14.7B — unique-blob batch 50 completion checkpoint

Status: `COMPLETED_MAP_PASS_PAUSED_STORAGE_GROWTH`. The first 50 unique W3
blob identities are closed. The 50 semantic imports all finished `MERGED`; one
additional physical copy of blob 48 was reconciled as a separate
`NO_NEW_KNOWLEDGE` experiment. No selected item is open or invalid. Batch 50
map acceptance passed, but the storage acceptance target did not: during the
resume segment C: free space fell by about 2.30 GB. Item 51 remains untouched.

## Acceptance record

```text
BATCH_SIZE=50
CLOSED=50 unique semantic blobs (51 experiment receipts including duplicate)
MERGED=50
NO_NEW_KNOWLEDGE=1 duplicate physical capture
INVALID=0
OPEN=0 in batch; item 51 remains OPEN
INPUT_EVENTS=81390
ACCOUNTED_EVENTS=81390
UNACCOUNTED_EVENTS=0
RAW_BYTES_DELETED=3988128
STAGING_BYTES_DELETED=390430966
CANONICAL_BYTES_GROWN=2301565042
NET_DISK_CHANGE=+2298839040 bytes occupied during resume-to-item-50
FREE_SPACE_BATCH_START=NOT_CAPTURED_BEFORE_ITEMS_1_46
FREE_SPACE_BEFORE_RESUME=95920517120
FREE_SPACE_BATCH_END=93621678080
SOURCE_OWNED_DELTA=0
CANONICAL_SELF_CHECK=PASS
STAGING_LEAKS=0
NEXT_QUEUE_ITEM=51 (849d416a1d563de2a24de62de78e0ce3167dc17f3e4a99e927327a84fc183d39; raw SHA verified)
```

## Counts and map checks

| Metric | Result |
|---|---:|
| Unique blobs closed | 50 / 50 |
| Experiment receipts | 51 (50 semantic imports + 1 duplicate-copy closure) |
| MERGED / NO_NEW_KNOWLEDGE / INVALID / OPEN | 50 / 1 / 0 / 0 |
| INPUT_EVENTS / ACCOUNTED_EVENTS / UNACCOUNTED_EVENTS | 81,390 / 81,390 / 0 |
| MERGED_EVENTS / ALREADY_KNOWN_EVENTS / UNRESOLVED / REJECTED | 6,913 / 74,477 / 0 / 0 |
| SOURCE_OWNED before / after / delta | 1,487,672 / 1,487,672 / 0 |
| Final generation / map hash | `gen-b39c0e9c75ea375e-4135d7db` / `aae5653373ec6ea3ba61851ca3ef342018e258904e84e53bb1076b69042706b1` |
| Final graph structure hash / ROM emission hash | `4b77d26778e52a620602c556e25ecba3382c8a309f13a7ab27525a6114d341bc` / `4c8a1951a65a5683012f25cd037c918dcbeaad2b3806a5deaa6d8b5d8e9e317a` |
| Canonical self-check / both SQLite integrity and FK checks | PASS / PASS |
| Occurrence, path, provenance, unresolved checks for items 47–50 | PASS; positive occurrence/path/provenance rows, zero unresolved |

All four imported raws have individual receipts, archived canonical import
receipts/reports, and post-delete checks. The blob 48 duplicate reconciliation
proves byte-identical SHA-256 content across two different run IDs and all 16
worker record hashes; it was semantically ingested once, then both physical
copies were deleted under their own closed receipts. Blob 47's raw SHA was
verified before processing; the old partial staging had no import receipt or
map-import row. It was then successfully imported and closed.

The receipt and staging audit scanned 87 closed experiments. All receipt-named
raw and temporary targets were absent. It found 82 leftover empty, SHA-bound
staging directories and removed only those verified empty directories. The
follow-up scan found zero staging directories or files. ROM SHA remained
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
Audit details are in
[THOR_M14_7B_BATCH_50_STAGING_AUDIT.json](THOR_M14_7B_BATCH_50_STAGING_AUDIT.json).

## Storage accounting

| Metric | Result |
|---|---:|
| RAW_BYTES_DELETED for batch 50, including blob 48's extra physical copy | 3,988,128 |
| STAGING_BYTES_DELETED, receipt-bound plus local task staging mirrors | 390,430,966 |
| CANONICAL_BYTES_GROWN during resume of items 47–50 | 2,301,565,042 |
| FREE_SPACE_BEFORE_RESUME | 95,920,517,120 |
| FREE_SPACE_AFTER_ITEM_50 | 93,621,678,080 |
| NET_DISK_CHANGE, resume-to-item-50 occupied bytes | +2,298,839,040 (usage increased) |
| Latest free-space sample after periodic audit | 93,618,081,792 |

The original free-space sample before items 1–46 was not recorded in the
interrupted progress report, so this reports the fully observed resume-to-end
interval. Canonical growth is the exact summed file size of the four immutable
generations created for items 47–50. The batch removed raw and staging data,
but canonical generation snapshots outweighed those deletions. This fails the
requested `NET_DISK_CHANGE < 0` storage target, so ingestion paused before item
51. That blocker was later resolved by verified one-map generation compaction;
see `docs/reports/THOR_M14_7B_GENERATION_COMPACTION.json`.

Per-blob free-space-after-ingest and true peak-staging samples were not
persisted by the interrupted runner. Raw/staging deletion sizes, generation
sizes, pre-resume free space, item-50 post-cleanup free space, and the final
periodic-audit sample are verified; unsampled telemetry is not inferred.

## Per-item continuation results

- Blob 47 `23f38bb2eb757044…`: `MERGED`, 1,688 events; 139 merged, 1,549
  already known; 0 unresolved/rejected.
- Blob 48 `c6769993044691da…`: primary `MERGED`, 1,696 events; 140 merged,
  1,556 already known. Its second run identity is `NO_NEW_KNOWLEDGE`, with all
  1,696 events accounted as already known.
- Blob 49 `ce64da553e864004…`: `MERGED`, 1,821 events; 140 merged, 1,681
  already known; 0 unresolved/rejected.
- Blob 50 `b6f9f8ac70477b41…`: `MERGED`, 2,273 events; 241 merged, 2,032
  already known; 0 unresolved/rejected.

## Next safe action

Queue item 51 is blob `849d416a1d563de2a24de62de78e0ce3167dc17f3e4a99e927327a84fc183d39`.
The storage gate passed: one full canonical generation remained and measured C:
free space rose by 30,228,242,432 bytes after reclaiming verified ancestors.
The item-51 preflight and canonical attempt are recorded below.

## Blob 51 continuation checkpoint — CLOSED / MERGED

The raw SHA and 16-worker W3 lineage index passed; all 2,462 input records were
accounted (242 accepted, 2,220 duplicates, zero unresolved/rejected/unaccounted).
The first import failed closed because the saved session had skipped the exact
ROM linker. Replaying the same audited segments through the existing
`LiveForwardRomLinker` linked all 56 M68K instruction nodes to exact ROM ranges
from 705 instruction occurrences. The two `EXECUTED_NEXT` edges adjacent to an
exception event were excluded by the importer contract. Canonical import and
independent audit then passed, adding 56 evidence refs and one import row with
no new ROM objects/relations or unresolved facts; `SOURCE_OWNED` stayed
1,487,672.

The current generation is `gen-6f8777084fd64470-ece7acab` with map hash
`2a9dd405b6670404df7b93f955f18e741fb3edb3a8c10507afa3e7d4f6c40e91`. GC plan
PASS confirms one full generation pair. Canonical self-check, integrity/FK,
ROM, object/edge, occurrence, 2,984 path, order, capture-window, loop and
alternate-tail checks passed. Receipt-authorized `cleanup --closed-only`
deleted the exact raw plus 10 temporary artifacts (11 files / 12,727,391
bytes); every target is absent afterward. The ROM SHA and map hash remained
stable during cleanup. Final sampled C: free space is 123,232,952,320 bytes;
there was no immediate pre-cleanup sample. The six unsealed canonical staging
directories remain blocked and untouched. Full evidence is in
`docs/reports/m14-7b-closure-receipts/w3-wave-000007-849d416a/`.

**Next safe action:** preflight the next queued unique blob under the same
closed-only cleanup lifecycle; blob 51 is now closed and consumed.
