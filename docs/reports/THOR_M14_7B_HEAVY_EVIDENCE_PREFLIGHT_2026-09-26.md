# M14.7B Heavy Evidence Consumption — Preflight

Status: `BLOCKED_SYSTEMATIC_ACCOUNTING_GAP`. This was a read-only assessment.
No normalized corpus, session DB, raw capture, generation, ROM, or canonical map
file was deleted or modified. No ingestion was attempted.

## Post-build-cleanup audit

The user-run build cleanup result reports `COMPLETE`: 51,653 entries and
17,973,234,988 logical bytes deleted, with zero skips. A stream audit of its
JSONL confirms 51,653 deleted records, the same byte sum, zero skipped records,
and zero listed paths still present. Recorded free space increased from
121,562,128,384 to 139,570,184,192 bytes.

The current pointer still selects
`gen-6f8777084fd64470-ece7acab`, map hash
`2a9dd405b6670404df7b93f955f18e741fb3edb3a8c10507afa3e7d4f6c40e91`.
Both database SHA-256 values and the ROM SHA-256 match the cleanup result.
Read-only `knowledge_generation_gc.py --plan` returned `PASS`, its current
self-check returned `PASS`, and it found exactly one full generation. The six
unsealed staging directories recorded in the batch-50 audit were not touched.
`SOURCE_OWNED` remains 1,487,672 bytes; delta is zero because no write occurred.

## Largest normalized artifacts

All three have historical sealed generic-closure receipts with deterministic
replay `PASS`, unresolved count zero, and `source_owned_delta=0`. Those receipts
also report new facts, so `NO_DELTA` does not establish that the current M14.7B
map already contains the resulting knowledge. They do not provide the required
per-record `MERGED / ALREADY_KNOWN / UNRESOLVED / REJECTED` accounting for the
current destination.

| Size (bytes) | Run identity | Format / records | File SHA-256 | Historical closure |
| ---: | --- | --- | --- | --- |
| 13,160,429,958 | `1790240077`; corpus `run-1790240077-d43ab1c62e3c3ab3`; source raw SHA `d43ab1c62e3c3ab3b1312bca7c5522cf831adfcbc6f48a2f0d286ad3aee76527` | normalized-generic-corpus v2, `instructions[]`; 19,186,672 records | `33413d0cfeb2a5e5baa019276ccf1daa9a61e75fa5724023f3d04a7e2cdff447` | `NO_DELTA`; 9,739 new facts; 3,109 CFG edges |
| 4,031,598,922 | `1790239022`; corpus `run-1790239022-85359eb0db65825f`; full source-raw SHA unavailable | normalized-generic-corpus v2, legacy `calls[]`; 3,798,288 records | `3ba6f51adb57ea3b1a427ce06840fab6f9a2a0b6e372cdcac651aa0cc651cc30` | `NO_DELTA`; 2,820 new facts; 1,039 CFG edges |
| 2,395,075,150 | `1790246422`; corpus `run-1790246422-1952eeb1ade6c3ff`; source raw SHA `1952eeb1ade6c3ff4287b930afd0e62b9d25082b037f8ba5552bb8c32c61cf3d` | normalized-generic-corpus v2, `instructions[]`; 3,514,546 records | `9221ca104fbe4a20d4c01a475c2b5f7b66db5b3e9d07785b03d9b7b3a6726ca8` | `NO_DELTA`; 3,899 new facts; 1,616 CFG edges |

Combined size is 19,587,104,030 bytes and the historical receipt record count
is 26,499,506. The first file's exact SHA is present in the current map's
`source_artifact` table, but only one evidence reference points to it; that is
not sufficient proof that its 19,186,672 records are represented. The other
two file SHAs are absent from the current map's source-artifact table.

## Stop reason and disposition

The current fusion importer only imports selected target-instruction
occurrences. It does not provide a lossless, bounded-memory import contract
with per-record accounting for these complete M13 normalized-generic corpora;
the legacy `calls[]` file also lacks the expected corpus checksum fields. The
historical closure totals cannot substitute for destination accounting or
prove that ordered occurrences, paths, and alternatives are queryable in the
current canonical map. This is a systematic accounting gap, so the phase stops
here as instructed.

| Measure | Result |
| --- | ---: |
| Artifacts assessed | 3 |
| Artifacts closed for current map | 0 |
| Merged / proven no-new-knowledge | 0 / 0 |
| Artifacts left OPEN | 3 |
| Current-destination records accounted / unaccounted | 0 / 26,499,506 |
| Normalized / session / raw / old-generation bytes deleted | 0 / 0 / 0 / 0 |
| Current map growth / `SOURCE_OWNED` delta | 0 / 0 |
| Current canonical map self-check | `PASS` |

Next safe step: define and test a streaming M13 normalized-generic importer
through the existing canonical publisher, including full record accounting,
stable event identity, and complete provenance preservation. Only after its
receipt proves closure should any of these three inputs become disposable.
