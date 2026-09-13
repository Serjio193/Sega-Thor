# THOR M12 AUTO64 — Knowledge Coverage and scheduler result

Date: 2026-09-13. Baseline: `d9d080a3a0c152d5b131f246a1ab2a3f9a435178`.

AUTO64 adds the smallest machine-readable coverage and queue layer demanded by
the AUTO63 frontier. It does not create a second Evidence Engine, change game
runtime architecture, or modify ownership. The ignored state is under
`build/thor-evidence/auto64-provenance/`.

## Coverage contract

`src/tools/thor_evidence/auto64_knowledge_coverage.py` imports AUTO62's 620
persisted investigations, AUTO63's selected range and unresolved frontier, the
AUTO63 raw capture identity, and the bounded static report. Each entity has
independent dimensions for observation, execution, readers, writers, consumer,
producer, pointer source, caller, control, selector, domain, boundary, format,
structure, terminal root, ASM representation and SOURCE_OWNED. Coverage never
increments SOURCE_OWNED; the imported ownership remains `1,475,600 /
3,145,728 = 46.9080607096%`.

The canary is derived from `AUTO63.unresolved_frontiers`, not from a scheduler
constant: `A6_INHERITED_AT_ENTRY` creates `INV-AUTO64-A6`. The imported entity
`ROM:167DD8-167E48` is observed with a proven consumer, partial boundary and
structure, and SOURCE_OWNED=`NO`; this is semantic coverage, not byte ownership.

## DAG and queue execution

The persistent graph records parent, entity, question, why-open, known and
missing facts, required evidence class, status, children, raw witnesses,
derived relations, resolution, block reason, creation source and a repeat
fingerprint. The observed chain was:

`AUTO63 unresolved frontier -> INV-AUTO64-A6 -> static phase ->
INV-AUTO64-A6-CALLER -> focused runtime -> A6 version candidate
INV-AUTO64-A6-DEFINITION-00A22C -> bounded static slice`.

The scheduler then selected D3 and both gap obligations from the graph and ran
bounded static analysis. They are `BOUNDED_UNRESOLVED`; the six-byte theory is
not promoted. The final statuses are:

- `INV-AUTO64-A6`: `BLOCKED`; A6 source remains unresolved and no terminal root
  is claimed.
- `INV-AUTO64-A6-CALLER`: `BLOCKED`; no `AF00` entry plus return-compatible
  dynamic caller edge was observed.
- `INV-AUTO64-A6-DEFINITION-00A22C`: `EXHAUSTED`; its bounded slice reads
  `(A6)` but contains no A6 destination, so the observation site is not called
  a definition.
- D3 and both `+8` investigations: `BOUNDED_UNRESOLVED`; static candidates do
  not establish a semantic count, stride, alignment or record format.

No terminal root was promoted. An unresolved frontier remains a blocked proof
obligation, not a ROM_DATA/ROM_CODE_CONSTANT/RESET/HARDWARE/INPUT root.

## Runtime request and evidence

The before-run request recorded investigation, known facts, missing caller/A6
version evidence, watch plan, why AUTO63 was insufficient and expected new
information. The corrected focused BizHawk run used the exact ROM/state and 20
frames after the validated three-frame settle. It produced 180 target contexts,
including AF02/AF06/AF20/AF22, and 10 same-frame bounded A6-version candidates
at observation PC `0xA22C`; it did not observe AF00 or a proven caller edge.
AF20 contexts showed the known address families, but no read/source pairing was
used to explain D3=512 or either +8 gap.

The first instrumentation attempt was not treated as evidence: it serialized
every A6 change, produced 1,436,161 raw bytes and hit the event cap. The second
run changed the watch scope to retain only bounded in-memory A6 versions and
target contexts, produced 829,449 raw bytes and completed with `result=PASS`.
Launch-to-result filesystem timestamps measured approximately 21,752 ms for 20
frames; callback count was 215,277 and raw event records were 184. This is an
instrumentation cost measurement, not a game timing claim.

## Ownership and validation state

SOURCE_OWNED delta is `0`. No ROM/assets were added. Full-layout ASM and
canonical byte identity remain governed by the existing M12 gate; AUTO64 made
no promotion and therefore cannot claim a new byte-exact assembly result.
The canonical identity remains size `3,145,728`, CRC32 `C4728225`, SHA256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

Next scheduler state is empty for the current high-value frontier because all
current candidates reached justified fixed points. A future new game scenario
is required before another runtime run; replaying the same QuickSave1 window is
anti-repeat prohibited.
