# M13.2 — Generic Closure Gap-Driven Improvement V1

## Result

`PASS_GENERIC_CLOSURE_GAP_DRIVEN_IMPROVEMENT_V1 = PASS`

The M13.1 normalized corpus was sufficient for deterministic replay; its deleted
raw FLOW/index were not accessed or reconstructed. The canonical ROM at
`local-roms/Beyond Oasis (USA).md` was verified at 3,145,728 bytes with SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

## M13.1 baseline artifact audit

The M13.1 `postrun_recursive_closure.json` reports a trivial fixpoint after one
iteration: zero graph nodes/edges, zero new facts, zero CFG edges, zero ROM
tables, zero RAM structures, zero indirect targets, zero promotions, and empty
engine unresolved/queue arrays. No candidate or exact ROM ranges were supplied.
The stage-level receipt separately reports one unresolved initial/final item;
that is its exact-ROM-roundtrip capture-gap placeholder, not a closure-engine
fact. Its ranking's zero blocked-byte/edge values were placeholders, not
measured absence, and are not treated as evidence.

The preserved normalized input contains 3,862,913 rows but has empty
`instructions`, `memory`, and `rom_ranges`. The adapter copied rows into
`records` without translating them into closure facts. Therefore the engine
had no input facts to propagate and no exact source/canonical byte-range
contract to promote. Zero delta is an input/normalization and round-trip
boundary gap, not a negative claim about all observed execution.

## Selected generic capability

The engine now imports a unique observed PC→next-PC edge from a normalized
instruction event only if that event's source opcode exactly matches canonical
ROM bytes. Edge truth is `OBSERVED_NEXT_PC`; it does not assert that the edge is
a static branch/call, prove the target instruction, define a code/data extent,
or authorize ownership. Identity failures are aggregated as unresolved facts.

Selection rationale: this uses preserved evidence generically and converted a
large set of previously ignored instruction events into auditable CFG
navigation facts without adding any PC/address/subsystem special case. Expected
ROM-byte gain is unmeasurable and this capability alone does not increase
SOURCE_OWNED.

## Deterministic replay metrics

| Metric | M13.1 baseline | M13.2 replay |
|---|---:|---:|
| FIXPOINT_REACHED | true | true |
| ITERATIONS | 1 | 2 |
| TOTAL_FACTS | 0 | 1,208 |
| NEW_FACTS | 0 | 1,208 |
| CFG_EDGES | 0 | 1,208 observed edges |
| ROM_TABLES | 0 | 0 |
| RAM_STRUCTURES | 0 | 0 |
| INDIRECT_TARGETS | 0 | 0 |
| CANDIDATE_ROM_RANGES | 0 supplied | 0 supplied |
| EXACT_ROM_RANGES_CLOSED | 0 | 0 |
| UNRESOLVED_INITIAL / FINAL | engine 0 / 0; stage gap 1 / 1 | 1 / 1 |
| SOURCE_OWNED_BEFORE | 1,487,672 | 1,487,672 |
| SOURCE_OWNED_AFTER / DELTA | 1,487,672 / 0 | 1,487,672 / 0 |

The replay read 3,862,913 preserved rows: 2,424,594 instruction-tagged rows
and 1,438,319 bus-tagged rows. Of the instruction-tagged rows, 962,333 did not
match canonical ROM opcode identity and remain unresolved; the old normalized
schema omitted CPU/domain, so this count cannot safely be split into CPU-space
differences versus identity faults. There were 1,254 unique observed execution
PCs in the rows and 1,208 unique accepted PC→next-PC transitions.

Both independent fresh engine runs produced identical canonical result bytes,
SHA-256 `f4c824145094474679fd87e67fc2e5cf7be209952e4bfcf46f1a5925953a9bfb`.
Replay status and gap-ranking status are PASS. Exact ranges remain zero; no
positive SOURCE_OWNED result is claimed.

## Deterministic blocker ranking

Every blocker has `blocked_ranges`, `blocked_bytes`, and `blocked_cfg_edges`
unknown where no exact range relation exists. Expected ROM gain is
`UNMEASURABLE_FROM_PRESERVED_FACTS` for every row. The deterministic scheduling
tie-break uses affected-reference counts; rows overlap and are not additive.

| Rank | Blocker | Affected references | Required evidence/capability | Expected ROM gain |
|---:|---|---:|---|---|
| 1 | `MISSING_REGISTER_SNAPSHOT_OR_DECODED_OPERANDS` | 2,424,594 | Register snapshots or verified instruction operand normalization | Unmeasurable |
| 2 | `MISSING_MEMORY_WIDTH_DOMAIN_AND_CAUSAL_IDENTITY` | 1,438,319 | Bus width, domain, value, and ordered instruction identity | Unmeasurable |
| 3 | `INSTRUCTION_IDENTITY_OR_CPU_DOMAIN_UNRESOLVED` | 962,333 | Preserve CPU/domain identity and validate against that address space | Unmeasurable |
| 4 | `EXACT_ROM_ROUNDTRIP_CLOSURE_GAP` | Unknown | Exact candidate boundaries plus byte-identical round trip | Unmeasurable |

The former unconsumed-flow blocker was improved: 1,208 verified CFG facts are
now present. No exact ownership boundary can be derived from those facts alone.

## Minimal next-capture contract

- `REQUIRED_NATIVE_FIELDS`: retain stream sequence, instruction sequence,
  master time, run/epoch/worker/capture/generation/segment identity, CPU ID and
  address-space/domain, PC, next-PC, opcode and instruction bytes/length;
  bus direction, address, width, value, domain, and auxiliary fields.
- `REQUIRED_REGISTER_WINDOWS`: pre/post D0–D7, A0–A7 and SR snapshots at
  instruction boundaries, with exact instruction/sequence identity; ensure
  call/return boundaries retain caller and callee register states.
- `REQUIRED_CONTROL_FLOW`: ordered instruction transitions, explicit branch,
  call, return and indirect-transfer classifications, with direct targets and
  observed next-PC kept as separate evidence types.
- `REQUIRED_MEMORY_EVENTS`: preserve ordered read/write events, width, value,
  address space, and exact causing instruction identity; distinguish RAM from
  canonical/banked ROM without inferring from numeric address alone.
- `RETENTION_REQUIREMENTS`: keep immutable raw FLOW and segment index until
  generic closure, blocker ranking, and deterministic replay/acceptance have
  all passed and their hashes/receipts are durable. Only then may normal
  cleanup reclaim them. The already-deleted M13.1 files were not modified.

## Artifacts and verification

Replay artifacts are under
`build/thor-evidence/live-worker-control/campaign-desktop-20260923-213728-376/post-run-analysis/m13.2-generic-closure-improvement/`.
The pipeline now runs closure twice and cleanup verifies the deterministic
replay and hashed ranking before deletion. Tests cover generic observed-edge
identity, unresolved preservation, existing selector/call/effective-address
fixtures, and cleanup fail-closed behavior.

`COMMIT_CREATED = NO`; `PUSH_PERFORMED = NO`.
