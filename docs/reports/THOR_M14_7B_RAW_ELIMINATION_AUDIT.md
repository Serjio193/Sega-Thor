# M14.7B — Raw elimination audit, first pass

Date: 2026-09-26
Base: `4f8b17837e48e2d9b960631b319f4968855aca33`
Branch: `codex/m14-7b-raw-elimination-evidence-map`

## Decision

No current runtime artifact is proven safe to delete. Existing `PASS` receipts
prove their named processing stage; they do not prove that every source event
and every format field has a durable replacement or that the full session can
be replayed without raw input. This pass adds inventory and seal policy only.
It does not rewrite capture, MAP-1, canonical knowledge, or cleanup behavior.

## Format and pipeline audit

| Artifact | Producer / format | Consumer and retained replacement | Unique information and current deletion reason |
|---|---|---|---|
| `flow-v1-records.bin`, `live-forward-wave-records-pass*.bin` | Live Forward Worker; fixed 48-byte FLOW_V1 records with stream/instruction sequence, master time, PC, address, value, kind flags, CPU, width, domain, reserved and auxiliary fields | FLOW byte auditor, ROM linker, Cartographer and post-run analyzers; MAP-1 stores structural nodes/edges plus a scoped occurrence projection | The occurrence projection does not retain every raw field (`master_time`, reserved/auxiliary metadata and some event payload distinctions are absent); faulted/untyped records can fail admission without an event-level rejection ledger. `KEEP_REASON=RAW_HAS_UNEXTRACTED_FIELDS; PROCESSING_INCOMPLETE` |
| `flow-v1-segments.jsonl`, `segment-audits.jsonl` | Worker host auditors; segment bounds, hashes, capture/window identity and audited register/lifecycle metadata | FLOW linker, Cartographer, runtime path view, receipts | These indexes establish raw offsets, exact windows and entry/exit state. Current session/map artifacts do not independently replace every index field. `KEEP_REASON=ONLY_COPY_OF_CAPTURE_WINDOW_METADATA; REPLAY_REQUIRES_RAW_INDEX` |
| `live-discovery-wave-*.bin` | W6 discovery worker; W3 V2 fixed 48-byte records | W3 decoder/discovery analyzer; some outputs are aggregate discovery reports | W3 V2 can decode all physical fields, but no sealed normalized event envelope and full accounting/replay receipt currently exists. `KEEP_REASON=LOSSLESS_NORMALIZED_EVIDENCE_NOT_PROVEN` |
| AUTO67 `capsule-*.bin` (O67C v1 / O67V v2) | Focused capsule collector; 20-byte/24-byte header and 16-byte/20-byte BUS/PC records | `auto67_capsule_codec.py`, AUTO67 worker and dependency witness adapter | Bounded selected observations only; capsule-specific observations and lease/frame identity must survive as local witnesses. No capsules matching the known filename were found in the scanned roots. `KEEP_REASON=ONLY_COPY_OF_CAPSULE_OBSERVATION` until a witness seal exists |
| Normalized event / session SQLite | FLOW normalizer, in-memory Cartographer, Archivist/MAP-1 merge | MAP-1 path view and canonical live delta import | Preserves accepted native occurrence identity and alternative tails, but does not account for all raw events or carry all raw semantic fields. `KEEP_REASON=PROCESSING_INCOMPLETE` |
| MAP-1 / `knowledge.sqlite` | Cartographer/Archivist and canonical knowledge importer | 3D/map queries, evidence lookup and path view | Existing truth labels, canonical ROM emission and ownership remain authoritative. These stores do not yet independently reconstruct all accepted, unresolved, rejected and duplicate source events. No map/schema or ownership change is proposed. |

`runtime_path_view.py` retains ordered per-window identity and does not join
across gaps. M14.7A therefore covers trajectory structure and lineage, but it
does not by itself prove source-format completeness. Unique branch, indirect
target, RTS/RTE, DBcc multiplicity and unresolved-event witnesses need explicit
retained inventories and replay comparisons in the next implementation step.

## Read-only inventory

The new auditor scanned 109,936 filesystem entries beneath
`C:\Github\Sega-Thor\build` and SHA-256 hashed 1,335 matching raw/support
artifacts. `C:\Dev\SegaThorTools` had 25,004 entries and no files matching the
runtime raw formats/patterns. The table counts discovered content; byte sizes
are exact and include duplicate copies at every pathname.

| Classification | Files | Bytes | Current disposition |
|---|---:|---:|---|
| FLOW_V1 binaries | 104 | 23,139,010,416 | `NORMALIZATION_REQUIRED` |
| W3 V2 discovery chunks | 1,161 | 8,576,681,184 | `NORMALIZATION_REQUIRED` |
| FLOW segment/audit indexes | 70 | 118,066,739 | `PROCESSING_REQUIRED` |
| Proven delete-safe raw binaries | 0 | 0 | None |
| Unknown large binary files selected by the scan | 0 | 0 | None |

Audited raw binary total: **31,715,691,600 bytes** across 1,265 files. Including
indexes, audited input/support total is **31,833,758,339 bytes**. The inventory
found 139 duplicate SHA-256 groups, 247 redundant same-byte path copies and
17,200,841,040 bytes of apparent duplicate storage. Those paths remain intact:
content equality alone does not authorize unlinking campaign evidence or its
lineage.

This is a runtime-artifact inventory, not a total disk-usage report for every
repository/tool/build file under `C:\Github` or `C:\Dev`. The scanner scopes
raw candidates by known FLOW/W3/AUTO67 names and large `.bin` files; new raw
formats or differently named small artifacts remain an explicit coverage
limitation until the format inventory is expanded.

## Completeness and seal status

`INPUT_EVENTS`, accepted/unresolved/rejected/duplicate counts and the graph,
path and occurrence replay hashes are `NOT_COMPUTED` for this cross-campaign
inventory. Existing per-campaign reports do not expose one compatible
event-accounting contract. Therefore `UNACCOUNTED_EVENTS` cannot be claimed as
zero. For this report:

```text
RAW_BINARY_DELETE_SAFE_FILES=0
RAW_BINARY_DELETE_SAFE_BYTES=0
NORMALIZED_EVIDENCE_DELETE_SAFE_FILES=0
NORMALIZED_EVIDENCE_DELETE_SAFE_BYTES=0
KEEP_OR_NORMALIZE_FILES=1265
KEEP_OR_NORMALIZE_BYTES=31715691600
PROCESSING_DEPENDENCY_FILES=70
PROCESSING_DEPENDENCY_BYTES=118066739
UNKNOWN_FILES=0 (within the selected large-binary/name scan only)
INPUT_EVENTS=NOT_COMPUTED
ACCOUNTED_EVENTS=NOT_COMPUTED
UNACCOUNTED_EVENTS=NOT_COMPUTED
AUTO_DELETE_PERFORMED=NO
```

Tool output is saved outside Git at
`build/thor-evidence/m14-7b-raw-audit.json` and
`build/thor-evidence/m14-7b-devtools-audit.json`. No raw, normalized, SQLite,
ROM, or map files were modified or deleted.

## Remaining acceptance work

The envelope retains all 12 fields and exact 48-byte source records for
FLOW_V1/W3 V2 and separately preserves O67C/O67V capsule headers,
lease/frame identity and observation records. It accounts
accepted/unresolved/rejected/duplicate records and reproduces raw SHA-256 from
compressed evidence without opening raw. This is not yet integrated into the
complete MAP-1/canonical pipeline and does not independently reproduce graph,
path and occurrence hashes. The existing 31.7 GB was inventoried but not
normalized; production receipts have not been upgraded to the new format or
replayed from retained sessions. The measured existing captures therefore
remain KEEP. No deletion command is part of M14.7B.

## Continuation — one canonical map / consumable captures

The existing `rom_knowledge_pipeline.py` is the sole map publisher. It stages
the child master and knowledge DB, imports and independently audits the
session, writes an immutable generation directory, and only then atomically
replaces `current.json`. The continuation adds
`experiment_lifecycle.py`: compact closure receipts bind raw path/hash/size,
format/producer/schema, exhaustive event counts, map generation and logical
map hash before/after, self-check result, new-evidence counts and unchanged
`SOURCE_OWNED`. `MERGED` must change the map generation/hash; `NO_NEW_KNOWLEDGE`
and `INVALID` must not. Invalid captures require a reason.

`cleanup --closed-only` is a separate explicit command. It prints an exact
plan, verifies every listed receipt and raw hash before deleting anything,
requires the receipt's exact audited root, rejects path escapes and duplicate
targets, and has no wildcard deletion path. Receipts can name hashed temporary
envelopes/session files for the same deletion transaction. Default mode is plan-only; pass
`--execute` to unlink. It also refuses inconsistent status/accounting/map
identity and ownership evidence. The receipts are transaction records, not a
new knowledge database.

The acceptance fixture closed and removed a synthetic raw capture, then queried
the retained MAP-1 occurrence store and recovered both divergent paths
`A-B-C-D` and `A-B-C-E`. Focused tests also cover `NO_NEW_KNOWLEDGE`, `INVALID`,
incomplete accounting, map-transition inconsistencies, ownership mutation,
open/tampered receipts and unchanged raw on refusal. This proves the lifecycle
primitive and post-deletion query behavior in a fixture; it does not yet prove
that FLOW/W3 raw events are semantically ingested into canonical MAP-1 for
production captures.

Fresh read-only inventory on 2026-09-26 reproduced the baseline exactly:
1,265 raw binaries / 31,715,691,600 bytes, 70 support indexes /
118,066,739 bytes, 139 duplicate-hash groups / 247 redundant paths /
17,200,841,040 duplicate bytes. The historical corpus remains untouched and
all 1,265 files remain open pending per-experiment ingestion receipts. No
production raw bytes were deleted.

## Continuation acceptance snapshot

```text
STATUS=IN_PROGRESS
BASE_COMMIT_SHA=4f8b17837e48e2d9b960631b319f4968855aca33
COMMIT_SHA=NOT_CREATED
BRANCH=codex/m14-7b-raw-elimination-evidence-map
CANONICAL_MAPS=1 (existing publisher)
SECONDARY_LONG_TERM_EVIDENCE_STORES=0 (policy; not yet corpus-verified)
RAW_FILES_BASELINE=1265
RAW_BYTES_BASELINE=31715691600
EXPERIMENTS_DISCOVERED=NOT_IDENTIFIED (1265 raw artifacts; experiment grouping not reconciled)
EXPERIMENTS_MERGED=0
EXPERIMENTS_NO_NEW_KNOWLEDGE=0
EXPERIMENTS_INVALID=0
EXPERIMENTS_OPEN=1265 raw artifacts pending ingestion (not experiment count)
RAW_DISPOSABLE_FILES=0
RAW_DISPOSABLE_BYTES=0
RAW_REMAINING_FILES=1265
RAW_REMAINING_BYTES=31715691600
BYTE_DUPLICATE_BYTES_FOUND=17200841040
INPUT_EVENTS=NOT_COMPUTED
ACCOUNTED_EVENTS=NOT_COMPUTED
UNACCOUNTED_EVENTS=NOT_COMPUTED
NEW_OCCURRENCES=fixture 6; historical NOT_COMPUTED
NEW_PATHS=fixture 2; historical NOT_COMPUTED
NEW_ALTERNATE_TAILS=fixture 1; historical NOT_COMPUTED
NEW_UNRESOLVED=NOT_COMPUTED
RARE_PATHS_PRESERVED=fixture PASS; historical NOT_COMPUTED
INDIRECT_TARGETS_PRESERVED=NOT_COMPUTED
RETURN_TARGETS_PRESERVED=NOT_COMPUTED
FULL_CHAIN_LINEAGE_PRESERVED=fixture PASS; historical NOT_COMPUTED
MAP_SELF_CHECK_RECEIPT_VALIDATION=fixture PASS; actual production map checks NOT_RUN
MAP_TRANSACTION_ATOMICITY=existing publisher stages and swaps current pointer; receipt integration NOT_PROVEN
OPEN_EXPERIMENT_CLEANUP_REFUSAL=fixture PASS
SOURCE_OWNED_BEFORE=0 (fixture)
SOURCE_OWNED_AFTER=0 (fixture)
SOURCE_OWNED_DELTA=0 (fixture)
NEW_TRUTH_CLASSES=0
OWNERSHIP_AUTHORITY_CHANGED=NO
RAW_ACTUALLY_DELETED=0
RAW_BYTES_ACTUALLY_DELETED=0
FOCUSED_TESTS=53/53
DEBUG_CTEST=PASS 219/219; updated lifecycle test PASS 1/1
RELEASE_CTEST=PASS 219/219; updated lifecycle test PASS 1/1
FRESH_WORKTREE_ACCEPTANCE=NOT_RUN
LINUX_SMOKE=PASS 2/2 (lifecycle and CFG link/smoke)
PUSH_PERFORMED=NO
REMOTE_HEAD_SHA=NOT_READ
```

Counts above keep fixture proof separate from historical corpus facts. The
milestone is not eligible for commit/push acceptance while production ingestion,
historical classification, fresh-worktree acceptance or full test results are
missing.
