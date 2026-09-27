# M14.7B — unique-blob batch 25 report

Status: `PASS_BATCH_25_CLOSED`; all 25 selected unique W3 SHA-256 blobs are closed as `MERGED`. M14.7B remains `IN_PROGRESS` for the rest of the corpus.

Each blob had a complete hash-bound lineage bridge, lossless event envelope, exact ROM linkage, passing canonical pipeline/idempotent reimport, and a separate closed-only receipt before cleanup. Every raw capture and exact temporary artifact listed by that receipt was removed and checked after deletion.

| # | SHA-256 | Records | Accepted | Known duplicates | Windows | ROM instructions | Evidence refs | Generation after | Status |
|---:|---|---:|---:|---:|---:|---:|---:|---|---|
| 1 | `10a36d780e2e…` | 1,499 | 131 | 1,368 | 16 | 485 | 396 | `gen-0411ceb70bde6c9d-f898ffef` | MERGED |
| 2 | `c8d47aca72a1…` | 1,499 | 131 | 1,368 | 16 | 490 | 400 | `gen-09023160f89740de-9a0e2e8c` | MERGED |
| 3 | `f906298ddcb5…` | 1,499 | 131 | 1,368 | 16 | 490 | 400 | `gen-d2ed2c50344f506b-f4c8df1e` | MERGED |
| 4 | `1512a3d9d1b8…` | 1,500 | 131 | 1,369 | 16 | 494 | 401 | `gen-d83a7d75a93fcb97-59c01909` | MERGED |
| 5 | `787873d4d1ff…` | 1,507 | 132 | 1,375 | 16 | 494 | 402 | `gen-e850a016eaf65aa2-1a52f02c` | MERGED |
| 6 | `23787eb9f6fd…` | 1,507 | 132 | 1,375 | 16 | 494 | 402 | `gen-7e8f8a0fc84063b3-25ed1a0c` | MERGED |
| 7 | `737c6af437c6…` | 1,508 | 131 | 1,377 | 16 | 490 | 400 | `gen-81ff2af16d58be1b-3f01b2e4` | MERGED |
| 8 | `75fbe78fc41f…` | 1,509 | 131 | 1,378 | 16 | 494 | 401 | `gen-ecf990bf4884c5a9-8b9a267a` | MERGED |
| 9 | `14daad128a24…` | 1,510 | 130 | 1,380 | 16 | 490 | 399 | `gen-59516b1d7597a740-31ecb9ca` | MERGED |
| 10 | `7c9294a76a45…` | 1,515 | 132 | 1,383 | 16 | 490 | 401 | `gen-a544386400fb8dab-7fb7cc36` | MERGED |
| 11 | `6a8d47f2477d…` | 1,515 | 132 | 1,383 | 16 | 490 | 401 | `gen-a469cd2767596ab6-302a8ce6` | MERGED |
| 12 | `3352245f088e…` | 1,516 | 132 | 1,384 | 16 | 494 | 402 | `gen-590f54d149bedc63-8725df0b` | MERGED |
| 13 | `99764dbab276…` | 1,517 | 131 | 1,386 | 16 | 490 | 400 | `gen-092fde3a94b09bf0-ffcfb907` | MERGED |
| 14 | `d0a2b475bd36…` | 1,519 | 130 | 1,389 | 16 | 490 | 399 | `gen-1bd21f1e35225946-a1f87bec` | MERGED |
| 15 | `3bc811d18d6f…` | 1,523 | 133 | 1,390 | 16 | 494 | 403 | `gen-9cddee0a5a06cfa3-499e8911` | MERGED |
| 16 | `44a6a5990f23…` | 1,525 | 132 | 1,393 | 16 | 494 | 402 | `gen-bc5a5decc9e27912-c78166e2` | MERGED |
| 17 | `b91f9bd8cd96…` | 1,527 | 134 | 1,393 | 16 | 485 | 399 | `gen-b5b81df9f363f783-a7463050` | MERGED |
| 18 | `1791a42dd142…` | 1,539 | 136 | 1,403 | 16 | 485 | 401 | `gen-ee1093e12bbaecb3-d660fd48` | MERGED |
| 19 | `c4c2995f173a…` | 1,539 | 134 | 1,405 | 16 | 494 | 404 | `gen-ccfbfaac23683314-1e7ccf1a` | MERGED |
| 20 | `caec1f4d8198…` | 1,542 | 132 | 1,410 | 16 | 490 | 401 | `gen-afaf3f2e07d1ce04-fcfcff14` | MERGED |
| 21 | `3db1d497b966…` | 1,548 | 134 | 1,414 | 16 | 494 | 404 | `gen-bf315d9427a54c6e-af427143` | MERGED |
| 22 | `4a75dc71880a…` | 1,550 | 133 | 1,417 | 16 | 494 | 403 | `gen-d84e7c2995fecf9c-d5ea2af7` | MERGED |
| 23 | `6c477997498a…` | 1,551 | 132 | 1,419 | 16 | 490 | 401 | `gen-f100dd061ebc4b3e-b829bdaa` | MERGED |
| 24 | `fcc343bef54d…` | 1,552 | 129 | 1,423 | 16 | 502 | 397 | `gen-7be27b580157f723-fdd4daba` | MERGED |
| 25 | `e64756e6e3c1…` | 1,555 | 135 | 1,420 | 16 | 494 | 405 | `gen-6cceb7a8183ee315-4544b2c7` | MERGED |

Batch totals:
- 38,071 records = 3,301 accepted + 34,770 known duplicates; zero unresolved/rejected/unaccounted.
- 400 windows; 12,291 exact ROM instruction occurrences; 10,024 provenance refs added.
- Receipt cleanup removed 350 files / 95,395,382 bytes, including 1,827,408 bytes of unique raw W3 data.
- Canonical map advanced from `gen-4cdadcafda3f91ba-83038f5d` to `gen-6cceb7a8183ee315-4544b2c7`; final map SHA `974a6da015c9b7cc665890aab017c02b7db30fa63eb8752d893e2b565163c92c`. Every import preserved structure/emissions and `SOURCE_OWNED` (delta 0).
- All post-delete checks show absent receipt targets, unchanged accepted generation, `integrity_check=ok`, and zero foreign-key violations.
- Incrementally reconciled unique queue: 983 remaining blobs / 1,229 raw candidates.

Receipts and post-delete checks are under `docs/reports/m14-7b-closure-receipts/`. The updated queue is `build/thor-evidence/m14-7b-real-capture/corpus-dry-run-post-batch-25.json`; it applies the exact 25 receipt-confirmed deletions to the preceding read-only census.
