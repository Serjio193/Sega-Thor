# M14.7B closed-only cleanup — 2026-09-26

Ingestion was stopped for this cleanup. No build, CTest, or new ingestion was
run.

`EXPERIMENT_CLOSED=TRUE → cleanup --closed-only` inventory found 82 closed
experiment receipts. All 1,147 receipt-approved raw and temporary paths
(420,481,285 bytes) were already absent under the exact audited root, so no
additional raw files were deleted in this cleanup. The plan includes the
receipt identities and exact absent targets.

The task-generated staging leftovers were checked against closed receipts by
raw SHA, lossless lineage/envelope identity, canonical import status, and
matching generation/map hash. The 80 closed staging directories contained
1,043 files / 306,039,589 bytes. They were outside the accepted canonical-map
root and ROM path. Their exact file manifests were revalidated before unlink.
The staging directory for unclosed blob 47 was explicitly excluded and left
untouched.

```text
FREE_SPACE_BEFORE = 95,613,534,208
FREE_SPACE_AFTER = 95,920,328,704
BYTES_FREED = 306,794,496

CLOSED_EXPERIMENTS_FOUND = 82
RAW_FILES_DELETED = 0
RAW_BYTES_DELETED = 0
STAGING_FILES_DELETED = 1043
STAGING_BYTES_DELETED = 306039589

OPEN_RAW_FILES_TOUCHED = 0
CANONICAL_MAP_TOUCHED = NO
ROM_TOUCHED = NO

MAP_HASH_BEFORE = ed285391323a56549cbb203258f78af720a9a61830a9011debeed185ed9333e1
MAP_HASH_AFTER = ed285391323a56549cbb203258f78af720a9a61830a9011debeed185ed9333e1
CANONICAL_MAP_SELF_CHECK = PASS

BLOB_47_EXISTS = YES
BLOB_47_SHA_MATCH = YES

NEXT_SAFE_ACTION = resume from blob 47
```

The post-cleanup self-check recomputed the canonical logical hashes from the
read-only current knowledge database, validated the accepted pointer and
generation hashes, and found `integrity_check=ok` with zero foreign-key
violations in both canonical SQLite databases. `SOURCE_OWNED` remained
1,487,672 bytes. ROM SHA-256 remained
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`. Blob 47
remains at its queued source path, 81,024 bytes, SHA-256
`23f38bb2eb75704459ab308e98404b61d30ead5e3e677cf4b626578b0cfa4ff9`.

One historical receipt (`real-capture.experiment.json`) has no separate
post-delete-self-check JSON. Its receipt targets were all absent; the current
canonical map self-check above was performed after this cleanup.

The machine had approximately 95.6 GB free immediately before this cleanup;
the earlier low-space stop had already changed by the time cleanup began. The
measured free-space delta is based on the volume snapshots; exact staging file
payload removed was 306,039,589 bytes. The full receipt and staging file manifests are in
`THOR_M14_7B_CLOSED_CLEANUP_PLAN.json`.
