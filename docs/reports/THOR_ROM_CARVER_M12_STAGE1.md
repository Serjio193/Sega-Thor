# Thor ROM Carver M12 Stage 1

Status: COMPLETE / STOPPED after the Stage 1 report.

## Scope

M12-CARVER-1 adds a developer-only orchestration layer over the existing M12
manifest and promotion evidence. It does not replace promoters, create new
`SOURCE_OWNED` bytes, emit unknown `dc.b` dumps, start M13, migrate ASM to C++,
or use a classifier or ML model.

The baseline was the pushed AUTO60 checkpoint
`8fa79225584cf0fbac76356d81c0b1c9adacfb85`, whose exact CI run `34600920230`
passed Configure, Build, and Test. The canonical external ROM remains
3,145,728 bytes with CRC32 `C4728225`, SHA1
`2944910c07c02eace98c17d78d07bef7859d386a`, and SHA256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

## Deterministic model

`src/tools/m12_carver.py` imports `oasis.full-rom-split.v1` into the exact
half-open interval `[0x000000,0x300000)`. Every manifest entry becomes one
range with the required identity, classification, confidence, evidence,
conflicts, parser/consumer/destination, discovery, provenance parent/child,
promotion transaction, and original manifest-entry fields. Import rejects
wrong bounds, gaps, overlaps, and a changed manifest SOURCE_OWNED metric.

`SOURCE_OWNED` is confirmed-only: an UNKNOWN kind or PROBABLE/CANDIDATE/
UNVERIFIED confidence cannot become confirmed. Evidence can attach to a range
or point into one, but Stage 1 never splits or reclassifies the imported map.
Candidate evidence overlapping a confirmed range creates a blocking conflict.
Candidate derivation and expansion require a confirmed manifest parent; a child
never inherits confirmed status.

Evidence and producer, consumer, parser, table, resource, code, runtime-reader
and range nodes are connected by typed directed edges. The graph is explicitly
allowed to contain cycles. Duplicate IDs with different content, invalid
candidate parentage, candidate/confirmed overlap, and expansion violations are
blocking conflict records.

## Existing evidence adapters

`m12_carver_adapters.py` has explicit deterministic adapters for:

- exact 68000/code census;
- Z80 upload proof;
- graphics decoder/resource scans;
- screen descriptors;
- pointer/resource tables;
- structured-record promoters;
- runtime PC/read provenance;
- padding/alignment promoters.

An adapter consumes only the evidence records already present in a producer
payload. It does not run a new detector or make a promotion decision.

## Fixed point and report

`fixed_point()` drains the expansion queue and reports whether it reached an
empty queue, new evidence count, range splits/reclassifications, and graph
edge count. Stage 1 is at a fixed point when no evidence, range change, or
provenance edge remains to be added and the queue is empty.

`gap_report()` emits every remaining UNKNOWN manifest range with size, immediate
neighbors, pointers/xrefs into the range, runtime reads/executions, detector
hits, known consumers, evidence, and conflicts. Neighboring UNKNOWN ranges are
unioned deterministically when they share consumer/parser, pointer-table
ancestry, runtime reader, structural format, resource-block context, or
contiguous resource context. Campaigns are ranked by deterministic size and
evidence-presence weights, and conflicts mark a campaign as promotion-blocked.

The CLI is:

```text
python src/tools/re_m12_carver.py --manifest <AUTO60 manifest> \
  --rom <external canonical ROM> --evidence <existing report> \
  --output <ignored build output>
```

It writes `interval_db.json` and `gap_report.json` with sorted, stable JSON.
Generated outputs remain ignored because they reference the external ROM and
local evidence.

## Stage 1 result

The AUTO60 import contained 2,448 ranges: 1,690 confirmed ranges and 758
UNKNOWN ranges. Coverage was exactly 3,145,728 bytes with zero gaps and zero
overlaps. SOURCE_OWNED remained exactly 1,427,873 bytes. Ingestion of the
existing AUTO60, graphics-census, and runtime evidence reports produced 23,113
evidence records, 25,565 provenance nodes, 46,341 directed edges, and zero
conflicts. The report had 758 campaigns and recommended the analysis-only
interval `[0x0C0000,0x11F360)` (389,984 bytes). This recommendation is not a
promotion authorization.

Regression coverage includes manifest rejection, exact ownership preservation,
candidate/conflict blocking, confirmed-parent enforcement, cyclic provenance,
and byte-identical serialization. Stage 2 detector expansion is intentionally
not started.
