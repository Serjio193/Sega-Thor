# M12-CARVER-5 — Whole-ROM Format & Container Reconstruction

## Scope and baseline

This pass starts from `origin/main` / `fa99ca0e3c2287d7d38335b2f1d4aaf2f91abe1c`,
the post-AUTO60 canonical manifest. Carver-3 runtime capture and Carver-4
static-consumer recovery are consumed as recorded evidence; neither sweep is
repeated here. The pass is developer-only and does not add ROMs, extracted
assets, M13 work, ASM-to-C++ migration, or new production dependencies.

The canonical interval is exactly `[0x000000,0x300000)`: 3,145,728 bytes,
zero gaps, zero overlaps. Baseline SOURCE_OWNED is 1,427,873 bytes
(45.3908602397%). The final ownership is identical:

| measure | baseline | final | delta |
| --- | ---: | ---: | ---: |
| SOURCE_OWNED bytes | 1,427,873 | 1,427,873 | 0 |
| SOURCE_OWNED percent | 45.3908602397% | 45.3908602397% | 0 |
| UNKNOWN ranges | 758 | 758 | 0 |
| UNKNOWN bytes | 1,717,855 | 1,717,855 | 0 |

No structural observation independently closed a reusable grammar, exact
physical representation, and code/semantic exclusion contract. Therefore no
candidate was promoted. Existing exact promotion contracts remain
authoritative.

## Whole-ROM structural acquisition

All 758 UNKNOWN ranges were fingerprinted globally. Fingerprints contain only
metadata and hashes, never source-byte dumps. The pass recorded alignment,
bank crossing, zero/FF runs, entropy for prioritization, repeated fixed-size
blocks, word frequency, pointer-like values, monotonic offset shapes, tail
sentinels, neighboring manifest classes, and exact range hashes.

The existing BO graphics grammar was scanned at every even candidate start in
every UNKNOWN range. This is a bounded parser mirror of
`oasis::game::decompress_graphics`; it returns source consumption and output
size, but a decoder hit remains CANDIDATE evidence. It is not a replacement
for an existing promoter.

| structural result | count |
| --- | ---: |
| even candidate starts checked | 858,249 |
| valid BO grammar hits | 1,454 |
| retained aggregated decoder spans | 722 |
| count × stride candidates | 31 |
| monotonic pointer-family candidates | 32 |
| sentinel candidates | 48 |
| uniform alignment candidates | 182 |
| exact duplicate provenance observations | 128 |
| promoted ranges / bytes | 0 / 0 |

Decoder hits and format candidates are represented as typed evidence in the
IntervalDB. Equivalent decoder observations are keyed by typed span, source
end, output size, and grammar mode; no millions of single-read events are
emitted.

## Formats, tables, containers, and extraction

The global candidate grammar census found 31 count/stride hypotheses, 32
monotonic pointer-family hypotheses, 48 sentinel hypotheses, and 182 uniform
alignment/padding hypotheses. These are prioritization and review records only.
No candidate was promoted because no candidate had a confirmed reusable
grammar sibling plus an exact source representation contract.

- Compression validity and decoder EOS were not treated as ownership.
- Boundaries alone did not invent a semantic resource type.
- Duplicate bytes produced provenance candidates but no typed source artifact.
- No arbitrary UNKNOWN `dc.b` representation was created.
- No executable bytes were claimed by format analysis.

The pass generated no committed payload extraction. Local ignored output may
contain hashes and metadata for the canonical user-supplied ROM only.

## Blockers and family result

The complete remaining map retains the Carver-4 blocker census:

| blocker | ranges | bytes | structural interpretation |
| --- | ---: | ---: | --- |
| B | 113 | 623,036 | known consumer context but no closed container/grammar |
| F | 56 | 58,789 | candidate overlap requires an exact split/view contract |
| G | 589 | 1,036,030 | no available exact consumer/boundary closure |
| **total** | **758** | **1,717,855** | **no promotion contract closed** |

The largest family remains the static consumer family rooted at
`c2288d60080e03d3`, spanning 521 ranges / 458,543 bytes. Other large unresolved
families are the 3-range / 404,266-byte family rooted at
`b5f7b8fd6228615b`, and the 1-range / 118,960-byte family rooted at
`4635581e9d3deee0`. These are family-level structural blockers, not isolated
manual gap investigations.

Remaining UNKNOWN ranges are labelled `FORMAT_HEURISTIC_ONLY` when a
structural candidate exists, `NO_CLOSED_GRAMMAR` when none exists,
`KNOWN_CONSUMER_UNKNOWN_CONTAINER` for unresolved B consumer context, and
`CANDIDATE_OVERLAP_REQUIRES_SPLIT` for F. None of these labels is ownership.

## Carver state and deterministic identity

The fixed point has an empty expansion queue, zero range splits, zero range
reclassifications, and zero new provenance edges in the reconciliation pass.
The output IntervalDB contains 4,494 evidence records, 7,083 provenance
nodes, 10,148 provenance edges, and zero conflicts.

Ignored machine-readable output:
`build/m12-carver-m12c5-format-reconstruction-c/interval_db.json` and
`build/m12-carver-m12c5-format-reconstruction-c/format_reconstruction_report.json`.

| hash | value |
| --- | --- |
| canonical ROM SHA-256 | `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263` |
| IntervalDB SHA-256 | `9f0e249954ffff4d31c20b99ca5f42737d0947fdcc98b9cc1ea772e7f4daf612` |
| evidence-state SHA-256 | `bdba33a930d61407b36702de26e3ac9a9f08897ea3734cd6093da516e0253d05` |
| report SHA-256 | `2b7fa8117521f813e8b1c6adb6b4fff12eddfd19e17fd194f1a5336f12c3f818` |

Canonical ROM identity remains byte-exact relative to the M12 baseline. The
implementation/final SHA and exact final-SHA CI run are recorded after
publication.

## Validation and stop gate

Helper tests and Python compilation pass. The required repository
Debug/Release, CTest, source-size, `git diff --check`, and GNU-equivalent
checks are recorded in the final publication entry below. A pre-existing
MSVC Debug failure in `src/core/ram_flag_routine.cpp` remains unrelated and is
not changed by this pass.

Final implementation SHA: `TO_BE_RECORDED`

Exact final-SHA CI: `TO_BE_RECORDED`

M12-CARVER-5 stops at this fixed point. No Stage 2 detector expansion,
semantic interpretation, M13, or ASM-to-C++ migration starts automatically.
