# M14.7B W3 to canonical lineage bridge

Status: `PASS_MERGED` for the selected 1,304-record W3 wave. This closes one
historical experiment; it does not claim the full 10-blob batch is complete.

## Contract audit

| W3 field | Source meaning | Sufficient for lineage alone? |
|---|---|---|
| stream/instruction sequence, master time | producer sequence and timing | No; no capture identity |
| PC/address/value/flags, CPU, width/domain | runtime observation payload | No; payload does not prove a window |
| reserved/auxiliary | format-specific extra values | No |
| run/cycle in filename and run receipt | experiment and wave context | No, without segment partition |
| exact run audit rows | Worker/capture ID, byte count, record count, stream bounds, SHA-256 | Yes when bound to the run receipt and exactly partitions the W3 bytes |

The W3 record layout omits capture/segment identity. The producer emits one
wave per round, appending Worker exports in ascending Worker order. Its run
receipt binds the segment-audit JSONL SHA-256. The bridge reconstructs a wave
only when all expected Workers are present exactly once, ordered record counts
cover the raw bytes exactly, each contiguous window hash matches its audited
FLOW segment, and stream/instruction bounds match. It does not use timestamps,
PC proximity, filename similarity, prefixes, or partial overlaps.

The exact historical instrumentation-script hash was not persisted, so that
item remains `UNVERIFIED`. The accepted assignment is tied to the persisted run
receipt, audit hash, native artifact identity, exact W3 hash, and byte-for-byte
chunk hashes; the missing script identity is retained as an explicit limit.

## First-wave result

- Raw W3 SHA-256:
  `502b2b370abd7c113604e63ac7070b04a1c1375295fad1f0d98903c451d1c235`;
  62,592 bytes / 1,304 records.
- Run `1790206461`, wave/cycle 15, four distinct Worker windows. Byte offsets
  are 0, 15,696, 31,392, and 46,992; lengths are 15,696, 15,696, 15,600,
  and 15,600 bytes. Ambiguous and unmapped records: zero.
- Raw-free accounting: 329 merged + 975 already known + 0 unresolved +
  0 rejected = 1,304 input. Unaccounted: zero.
- Canonical lineage retains 474 runtime occurrences (329 direct + 145
  derived) across eight CPU-specific paths. Four M68K and four Z80 windows
  remain separate; ordered multiplicity is preserved.
- Exact ROM linkage passed for 570/570 M68K instruction occurrences, with
  four terminal facts and no unresolved or unsupported instructions. The
  independent ROM audit found no identity conflicts or opcode mismatch.
- Canonical pipeline passed and added 1,215 evidence references, with no
  graph nodes, claims, or relations added. Idempotent reimport passed. The map
  hash changed from
  `799f763c8194f13d6ef74c569637c63841b6076f75ac9de45ed2546d06602998` to
  `bc4a15f15cdb75e268ddd8c5ce8c72725a81be747ca5f77feb154de8adbea36c`;
  structure and emission hashes stayed unchanged. `SOURCE_OWNED` remained
  1,487,672 bytes (delta 0).
- The exact receipt-listed cleanup removed 12 files / 3,877,900 bytes: the
  raw W3 and 11 temporary staging artifacts. The [post-delete check](m14-7b-closure-receipts/w3-wave-000015/post-delete-self-check.json)
  confirms all 12 absent, same map generation/hash, SQLite integrity `ok`,
  zero FK violations, and unchanged `SOURCE_OWNED`.

## Future captures and validation

Future runs publish a versioned `.lineage.json` sidecar after the run closes.
It binds the W3 SHA-256 and segment-audit SHA-256 to exact run/cycle/Worker
identities, offsets, lengths, counts, stream bounds, and per-window hashes.
W3 bytes and legacy readers remain unchanged.

```text
STATUS = PASS_MERGED
W3_RECORDS = 1304
W3_RECORDS_ACCOUNTED = 1304
W3_WINDOWS_PROVEN = 4
W3_AMBIGUOUS_RECORDS = 0
W3_UNMAPPED_RECORDS = 0
FULL_PATH_LINEAGE_SAFE = true
STANDALONE_OCCURRENCE_INGEST_SAFE = true
EXPERIMENT_CLOSED = true
RAW_DISPOSABLE = true
CANONICAL_MAP_CHANGED = true (evidence/index only)
SOURCE_OWNED_DELTA = 0
BATCH_RESUME_ALLOWED = true
```

Debug and Release builds passed. Full CTest passed 223/223 in each
configuration, including the 500-line source limit. `git diff --check` passed.
Linux smoke and fresh-worktree acceptance were not run for this checkpoint.
Other corpus blobs remain unclassified until their own per-blob accounting and
canonical checks pass.

## Unique-blob batch checkpoint

The first batch is complete: all ten first-queue W3 blobs have closed receipts.
The second blob (`2117996d92a43712c5928af0798658dca8e726d5f3236515dcc99ba219988cb6`)
proved 16 windows and accounted for 1,422 records (125 accepted, 1,297 known
duplicates, zero unresolved/rejected). It added 23 evidence references; the
map structure and emissions stayed fixed and `SOURCE_OWNED` delta was zero.
Its receipt is [w3-wave-000008](m14-7b-closure-receipts/w3-wave-000008/w3-wave-000008.experiment.json).
Cleanup removed that exact raw and 15 listed temporary artifacts. The copied
The third blob (`88ffcc2879666a944091f5e0dc85419d4daa8ecd4b9e37179866081874c408a4`)
proved all 16 windows from the receipt-bound audit and accounted for 1,447
records (129 accepted, 1,318 duplicates, zero unresolved/rejected). Exact ROM
linkage passed for 485 instruction occurrences, 23 ranges, and 16 terminal
facts. The canonical import added 394 evidence references; structure,
emissions, and `SOURCE_OWNED` stayed unchanged. Its receipt is
[w3-wave-000073](m14-7b-closure-receipts/w3-wave-000073/w3-wave-000073.experiment.json).
The receipt-bound cleanup removed that raw and 18 temporary files. Refresh the
The third through tenth captures are detailed in
[the batch 1 report](THOR_M14_7B_BATCH_1_REPORT.md). All ten are `MERGED`,
with zero unresolved/rejected/unaccounted events. Batch 1 cleanup and
post-delete checks passed for every blob; the refreshed queue contains 1,008
unique blobs. The next 25-blob batch may proceed.
