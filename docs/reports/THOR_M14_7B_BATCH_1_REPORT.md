# M14.7B — unique-blob batch 1 report

Status: `PASS_BATCH_10_CLOSED`; all ten unique SHA-256 blobs closed as `MERGED`. The milestone remains `IN_PROGRESS` because the remaining corpus is not yet ingested.

Every W3 blob had lossless record accounting, hash-verified capture windows, exact ROM linkage with zero unresolved/unsupported instructions, a passing atomic canonical pipeline, idempotent reimport, and an exact cleanup receipt. `SOURCE_OWNED` remained 1,487,672 bytes throughout. No graph structure or ROM emission changed; imports added provenance evidence only. One legacy run had no standalone run receipt; its `runtime log + segment audit` uniquely bound all 16 windows, and the original campaign `RESULT=FAIL` at a later cycle is retained as a limitation rather than represented as a successful campaign.

| # | SHA-256 | W3 records | Accepted | Known duplicates | Windows | ROM instructions | Evidence refs | Generation after | Closure |
|---:|---|---:|---:|---:|---:|---:|---:|---|---|
| 1 | `502b2b370abd…` | 1,304 | 329 | 975 | 0 | 570 | 1,215 | `gen-9d10ca77e8b40c7c-9fa7ac7c` | MERGED |
| 2 | `2117996d92a4…` | 1,422 | 125 | 1,297 | 16 | 430 | 23 | `gen-5d7c3f5fbf0834ef-a682f274` | MERGED |
| 3 | `88ffcc287966…` | 1,447 | 129 | 1,318 | 16 | 485 | 394 | `gen-922c3bef9b58bc5a-0507b157` | MERGED |
| 4 | `d7e4fb5b702f…` | 1,459 | 131 | 1,328 | 16 | 485 | 396 | `gen-be998debbabaeed9-21f1c253` | MERGED |
| 5 | `1362b2582e62…` | 1,475 | 130 | 1,345 | 16 | 494 | 400 | `gen-ab1a4e5395fb040e-a3dddbd6` | MERGED |
| 6 | `93dba68f359b…` | 1,485 | 129 | 1,356 | 16 | 490 | 398 | `gen-3161f4e9bb1ccc2a-3d01c439` | MERGED |
| 7 | `82cd6b274b1d…` | 1,486 | 129 | 1,357 | 16 | 494 | 399 | `gen-cb3daf1fa913a20f-161b65dd` | MERGED |
| 8 | `797b0beb05bb…` | 1,487 | 128 | 1,359 | 16 | 490 | 397 | `gen-20eb5da3dc994b1f-d14cc276` | MERGED |
| 9 | `3d6a69d789b1…` | 1,489 | 131 | 1,358 | 16 | 485 | 396 | `gen-3f41905596db1493-35944f64` | MERGED |
| 10 | `9b9a8ed3952e…` | 1,495 | 132 | 1,363 | 16 | 485 | 397 | `gen-4cdadcafda3f91ba-83038f5d` | MERGED |

Batch totals:
- 14,549 records = 1,493 accepted + 13,056 known duplicates; zero unresolved, rejected, or unaccounted.
- 144 proven windows and 4,908 exact ROM instruction occurrences; 4,415 evidence references added.
- Exact receipt cleanup removed 141 files / 40,741,189 bytes, including 698,352 bytes of unique raw captures.
- Canonical map: `gen-d90e9284ac4647cf-d7b37ed6` → `gen-4cdadcafda3f91ba-83038f5d`; final map SHA-256 `4a38ead9d790c4044cd7fd25f7effdceeb36a36a9fbe6c5fa40eb0e6cb936375`; `SOURCE_OWNED` delta 0.
- Every post-delete self-check confirms raw/temp absence, unchanged accepted generation, SQLite `integrity_check=ok`, and zero foreign-key violations.
- Incrementally refreshed remaining unique queue: 1,008 blobs; 25-blob batch may proceed.

Closure receipts and post-delete checks are stored under `docs/reports/m14-7b-closure-receipts/`. The refreshed queue is `build/thor-evidence/m14-7b-real-capture/corpus-dry-run-post-batch-1.json`; it reconciles the previous read-only scan against the ten exact closed receipts and removed unique files.
