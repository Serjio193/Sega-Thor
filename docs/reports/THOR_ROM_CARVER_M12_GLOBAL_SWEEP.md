# M12-CARVER-3 — Global Sweep

Status: complete global sweep and one Carver fixed-point pass. No M13 or
ASM-to-C++ migration started.

## Identity and ownership

| Metric | Result |
| --- | ---: |
| Canonical ROM bytes | 3,145,728 (`0x300000`) |
| Baseline SOURCE_OWNED | 1,427,873 (45.39086023966471%) |
| Final SOURCE_OWNED | 1,427,873 (45.39086023966471%) |
| Gain | 0 |
| Coverage gaps / overlaps | 0 / 0 |
| Canonical CRC32 | `C4728225` |
| Canonical SHA-1 | `2944910c07c02eace98c17d78d07bef7859d386a` |
| Canonical SHA-256 | `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263` |

The baseline is the pushed AUTO60 materialized manifest. Carver preserved its
confirmed byte count exactly.

## Global inputs and runtime aggregation

The sweep discovered 859 canonical-ROM-compatible JSON artifacts and ingested
784 unique payloads. It retained repeated artifacts in coverage accounting and
deduplicated identical payload content for IntervalDB ingestion. The set
contains 13 runtime scenario variants and 94 stored hybrid checkpoint summaries
(25 from `oasis.hybrid-poc.v1`, 69 from `oasis.hybrid-poc.v2`).

The normalized runtime set contains 25,037 raw observations and 16,367 typed
spans after deterministic adjacent/repeated-read aggregation. The union covers
117,000 ROM bytes; 9,764 bytes intersect UNKNOWN manifest ranges and 270
candidate bytes were observed. Each span records half-open `start`/`end`,
reader PC, width/type, order/frame where available, scenario ID, repetition
count, parser/decoder and destination fields, and explicit nullable
`src_before`, `src_after`, `consumed_bytes`, and caller ancestry fields. The
stored captures supplied no exact decoder `src_before`/`src_after` boundary.

External BizHawk and MAME executables were checked on the current host and were
not available. No emulator was installed and no runtime capture was fabricated.
The retained local deterministic inputs are listed in
`build/m12-carver-m12c3-global-sweep-d/global_sweep_report.json`.

Scenario union summary:

| Scenario family | Artifacts | Spans | Observed ROM bytes | UNKNOWN observed |
| --- | ---: | ---: | ---: | ---: |
| `boot_initial` | 13 | 32 | 32 | 0 |
| GPGX reader correlation (+ repeat) | 2 | 1,444 | 102,262 | 2,261 each |
| GPGX execution evidence | 1 | 14,732 | 14,732 | 7,497 |
| `natural_idle_to_6121a_v1` variants | 15 | 144 | 144 | 1–14 |
| isolated `re-trace` variants | 3 | 15 | 15 | 5 |

## Global graph and boundary pass

All existing recognized Carver adapters were run through the global input set:

| Adapter | Records |
| --- | ---: |
| exact 68000 code census | 572 |
| graphics/resource scan | 175 |
| pointer/resource table | 17 |
| runtime PC/read | 11 |
| screen descriptor | 3 |
| structured record | 3 |
| Z80 upload proof | 2 |
| padding/alignment | 1 |

Runtime clusters are available by reader PC, parser, consumer and destination
domain. Evidence-side clusters are available by parser, consumer, structural
format and destination. No runtime destination domain was identified by the
stored captures; all 16,367 runtime spans therefore remain
`destination=UNKNOWN`. No exact boundary recovery was available, so no global
candidate satisfied the exact boundary plus confirmed consumer contract.

## Promotion and conflicts

The fixed-point result is:

```text
reached=true
new_evidence=0
range_splits=0
range_reclassifications=0
provenance_edges_added=0
expansion_queue=0
```

No new range was promoted. 18,004 candidate records were rejected as
non-owning because the global evidence did not provide a new existing exact
promotion transaction. 168 blocking conflicts remain; 0 were resolved by this
pass. Runtime read or decoder observation alone never changes ownership.

## Complete UNKNOWN blocker census

The machine-readable report contains one blocker record for every one of the
758 remaining UNKNOWN manifest ranges. The dominant blocker is selected by the
following deterministic order: conflict overlap, code/data conflict, observed
without consumer, observed with consumer but unknown boundary, known boundary
with unresolved semantics, static-only evidence, then never observed.

| Blocker | Meaning | Ranges | Bytes |
| --- | --- | ---: | ---: |
| B | observed at runtime, consumer/parser unknown | 113 | 623,036 |
| F | candidate overlaps confirmed range / blocking conflict | 56 | 58,789 |
| G | static evidence exists, no available scenario observed it | 589 | 1,036,030 |

Largest remaining families by blocker are therefore G (1,036,030 bytes), B
(623,036 bytes), and F (58,789 bytes). There are no remaining UNKNOWN ranges
outside this census.

## Deterministic artifacts

Output directory: `build/m12-carver-m12c3-global-sweep-d/`

| Artifact | SHA-256 |
| --- | --- |
| `interval_db.json` | `7d3fc90fa03e7f61b4fae272f1c2a67e420ab9079ee0efa315efd0328f7629d4` |
| `global_sweep_report.json` | `6847ccac3712ab01fe3877b21534bad0b1154ef8474b8ecf61d2bbd284f6300c` |
| canonical deterministic DB | `a5d11a451fc49c4afb1a64eb1bf3109397b14a58f48e28ea311e26e36e0b687d` |

The output report and interval database are ignored build artifacts because
they reference the external user-supplied ROM and local evidence. No
copyrighted ROM or extracted asset was committed.

## Validation and final SHA

The global-sweep regression test, Python compilation, source-size review, and
`git diff --check` are required final gates. Existing Release/GNU-equivalent
CI gates remain authoritative; the final commit and exact CI run are recorded
in the completion message and must match this report’s final SHA.
