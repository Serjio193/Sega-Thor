# THOR M12 AUTO67.5 — causal integrity gate

Status: PASS — bounded semantic correction and first generic dependency edge.

Baseline: `02791979f004dccafb6aad410bb730e7ec62c718`

AUTO67.4's capsule-to-worker-to-SQLite path was retained. The correction is
semantic only: new materialized payloads use schema version 2 and separate
`runtime_observations`, `observed_facts`, `causal_facts`, `chain_steps`, and
`unresolved_frontier`. Historical AUTO67.4 rows and its proof database were
opened read-only and were not migrated or rewritten.

## Integrity audit

The ten deterministic AUTO67.4 `MATERIALIZED_CHAIN` samples are listed in the
JSON report. Every old `causal_fact` was a direct capsule tuple of the form
`PC X was observed writing address Y`; each is classified
`OBSERVATION_ONLY`, with supporting capsule lease and record examples. Sample
totals:

- sampled records: 10;
- sampled old causal facts: 28;
- `OBSERVATION_ONLY`: 28;
- `CAUSAL_PROVEN`: 0;
- unsupported: 0.

Therefore AUTO67.4's old field name `causal_facts` was not semantically valid
for those runtime-only entries. They remain readable as historical payloads.

## Generic derivation

The worker now decodes the canonical ROM opcode for a `BUS_WRITE_PC` seed using
bounded generic M68K MOVE memory-write semantics. It emits no dependency from
time proximity, same-address writers, or observation order.

For the real canary:

```text
seed:          BUS_WRITE_PC pc=0x0027EC address=0xC00004
ROM opcode:    0x3955, extension 0xFFFC
decoded write: MOVE.W (A5),-4(A4)
causal facts:  INSTRUCTION_SOURCE_MEMORY (A5)
               ADDRESS_DEPENDENCY (A4)
chain_steps:   []
frontier:      REGISTER_PROVENANCE
```

This proves the instruction's source/destination operand semantics only. It
does not claim the origin of A5/A4, a RAM version producer, FF13CC, DMA/SAT
causality, or a complete chain.

The unrelated real seed `BUS_WRITE_PC pc=0x06009A address=0xFF0B82` decodes to
opcode `0x4A39`, which is not a supported memory-writing MOVE. It therefore
has zero causal facts and an explicit `STATIC_DECODE` unresolved frontier.

## Real short BizHawk proof

Artifact: `build/auto67-5-short-proof2.json`.

- 720-frame session, 16 workers, 336 leases, 320 returns;
- materialized records: 147;
- observed facts: 606;
- causal facts: 30;
- chain steps: 0;
- unresolved: 147;
- rooted: 0;
- unsupported causal facts: 0;
- capsule decode errors: 0;
- queue drops: 0;
- DB errors: 0;
- raw backlog: `NONEXISTENT`.

The live worker counters before SQLite deduplication were 304 materializations,
801 observed facts, and 38 causal facts; the figures above are the unique
persisted-record totals.

Five actual stored canonical payloads are preserved in the JSON report under
`five_actual_materialized_payloads`. The report also contains the canary and
generic payloads with their real chain hashes and all separated evidence
fields.

## Validation

- AUTO67.4 tests: 8 passed;
- AUTO67.3 tests: 3 passed;
- AUTO67 tests: 9 passed;
- Python compilation: passed;
- source-size gate: passed, 643 governed files at or below 500 lines;
- real BizHawk proof: passed.

No graph merging, AUTO68 work, SOURCE_OWNED change, or ownership promotion was
performed.
