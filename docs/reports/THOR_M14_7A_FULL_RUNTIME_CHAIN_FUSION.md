# M14.7A — FULL_RUNTIME_CHAIN_FUSION

Base: `fb489969e764223e0318ec74339e6cf4a21182d0`.
Branch: `codex/m14-7a-full-runtime-chain-fusion`.

## Contract and boundaries

`same start_pc != same execution`; `same start_pc + same next_pc != same
execution chain`; `same prefix != same full chain`.

Canonical ROM objects retain ROM/range/type and exact-byte validation.
MAP-1 structural nodes use kind/key/scope; edges use source/target/relation/scope.
Ordered path identity includes **every** retained event, CPU/domain, PC, opcode,
next address and outcome flags, including repetitions and the terminal address
fact. An edge set alone cannot identify a DBcc loop or an ordered trajectory.
Runtime identity remains the existing capture/run, epoch, CPU, domain, native
sequence, instruction sequence and event kind. Worker/capture/generation/segment
windows are witnesses: overlapping windows do not multiply native occurrences.

The accepted unit is a validated capture window, not the whole unobserved game
run. Paths do not bridge capture gaps. A terminal next_pc is an address fact,
not proof that its instruction executed. Prefix/path queries are derived views;
no trie, additional canonical database or ownership authority was introduced.
Runtime execution stays OBSERVED in MAP-1 and OBSERVED_RUNTIME in canonical
knowledge. AUTO67's pre-existing PROVEN reaching-definition contract is unchanged;
it is not static control-flow truth and repetition does not promote anything.

## Read-only identity audit (completed before branch creation/edits)

| Site | Identity / collapse | Lineage and alternative-tail assessment |
|---|---|---|
| auto65_chain._candidate | Hash of complete represented nodes and edges; candidate hash is truncated, classify also compares full structure | Different represented tails differ; no start-PC suppression. Source report rows are retained externally; candidate.source is only a representative. Not a full execution trace adapter. |
| auto65_chain.derive_candidates | setdefault by candidate ID for writer/value/address, static caller/target, target activity | Repeated report facts collapse; next_pc/trajectory fields are not an input contract. Cannot claim unrepresented continuations were analyzed. A hash collision is a theoretical legacy risk; no observed start-only collision. |
| auto65_chain.classify / cluster | Full structure equality; full prefix hash used for scheduling groups | KNOWN_NEW_INSTANCE requires full represented equality. Shared prefixes create NEW_BRANCH, not rejection. Groups retain each candidate ID. |
| auto65_campaign.capture_fingerprint | Scenario, reset, inputs, watched nodes, obligations, knowledge lineage | Capture reuse descriptor, not complete path identity. It does not skip candidate processing. Never use it to deduplicate full trajectories. |
| auto65_campaign.process | classify result; capture ID is scenario/report-name + scenario file hash | Existing report campaign replay bookkeeping, not native occurrence authority. Known candidates skip repeated static investigation; original reports remain required. |
| auto66_campaign.scenario_metadata / planner | Scenario file/start/inputs/targets/watch fingerprint | Avoids rescheduling same scenario definition; does not assert repeated gameplay would be identical. process_scenario still classifies all represented candidates. |
| auto66_campaign.process_scenario | Full candidate classify, prefix grouping; scenario-derived capture label | Different represented tails survive. Legacy scenario captures are not native run identities. |
| auto67_live._ensure_occurrence_identity | epoch/seq and window_item_id; investigation also includes window item | Exact stored item is leased once; no branch fingerprint or active-known collision gate. Added session UUID provenance to distinguish restarted dispatchers downstream. |
| auto67_live._choose_current | dispatch_state on window object, worker mailbox/lease | Existing identical-branch-occurrence test remains intact. Capsule exhaustion is capacity, not equivalence; current-window sampling does not prove full capture coverage. |
| auto67_cartographer._stable_bundle | Sorted unique producer/consumer/register dependencies | Targets survive; this is a dependency set, not ordered execution. DEFECT: exact occurrences disappeared from exported lineage. Fixed by separate local occurrence/step witness lineage; stable structural hash unchanged. |
| auto67_persistence.LiveMapSink | One bounded queue; stable dependency import_ref | Cartographer still merges lineage even if import_ref exists. No known-chain early return. Queue drops/errors remain explicit and are not successful fusion. |
| Cartographer._node_id / _edge_id | kind/key/scope; source/target/relation/scope | Canonical structural reuse; different targets have different IDs. Full lineage set union is idempotent. |
| Cartographer.merge / export_bundle | import_ref tracks admission, not skip condition; nodes/edges/frontiers exported | Graph body contradictions become CONFLICT. Historical export deliberately omits auxiliary tables; this caused exact occurrence loss through map_merge. |
| map_merge.merge_session_map | Structural session hash import_ref; temp copy + atomic replace | DEFECT: exact event table absent from master after merge. Fixed by streaming its native-ID union before publication. Conflicting same-ID event fails without replacing master. |
| live_forward_cartographer._admit | All ordered semantic rows; source+target EXECUTED_NEXT; segment lineage | Both outgoing targets survive; no seed/prefix rejection. Native events retained separately, overlap unions windows. Metadata defect: kind_flags_or read opcode; corrected to flags and tested. |
| live_forward_cartographer._record_runtime_occurrence | Existing runtime_occurrence_id; core payload comparison | Different executions retained, same native event across windows deduplicated, contradictory identity fails closed. |
| live_forward_archivist | Closed session, ROM, graph hash, source artifact hash | Reuses atomic merge. New sessions also seal exact occurrence rows independently; legacy sessions remain readable. |
| live_forward_rom_link.stage / decode | run/worker/capture/generation; decoder cache by PC | Duplicate capture rejected, not treated as known start. Canonical bytes cache is safe for immutable ROM. Every captured instruction still gets lineage. |
| live_forward_rom_link.project | ROM SHA/start/end/bytes; source+range edges, source+terminal address edges | Different terminal next_pc preserved. Raw per-capture lineage remains available; overlap counts there are window observations, not unique native-event counts. |
| rom_knowledge_map schema / runtime_occurrence_id | Existing v2 range/object, relation and scoped evidence_ref keys | Emission remains sole ownership authority; no new truth classes. |
| rom_knowledge_live_delta | Imports exact source-session events; one evidence_ref per event and subject | Already preserves canonical occurrence lineage independently of MAP-1 master defect. New end-to-end divergent-tail/replay test covers it. |
| rom_knowledge_fusion (M14.2B) | Exact accepted artifacts and earliest event per selected DMA-emitter PC | Explicit selected-evidence adapter, NOT full-chain importer. No claim that omitted corpus events were fused. Full validated sessions use live_delta. |

No start_pc-only **execution dedup gate** was found. Two lineage-loss defects
were reproduced and repaired, plus one outcome-mask metadata defect. This is
not a claim that historical AUTO65 reports or sparse AUTO67 captures contain
complete game executions. No address-specific exceptions were added.

## Before / after evidence

A two-instruction session held 3 native event rows (two instructions and one
transition). Before: master had no live_forward_runtime_occurrence table.
After: all 3 rows survive; replay adds zero rows and zero structural nodes/edges.

Before: two AUTO67 local chains with distinct occurrence IDs produced byte-for-byte
identical bundles. After: dependency hashes/nodes/edges still share identity,
but witness lineage distinguishes the occurrences. Tests replace the obsolete
bundle-equality expectation with equal structure AND unequal witness assertions.
No assertion of structural deduplication was removed.

A 100 x 3-instruction fixture produces 3 structural nodes, 2 structural edges,
300 instruction occurrences and 200 transition occurrences. It has 100 capture
paths, one structural path signature, and replay-stable accounting. Measured
fixture master size: 901120 bytes; creation/merge/path read: about 0.24 seconds
on this machine (synthetic, not a production throughput benchmark).

## Storage, complexity and retention

Reuse the optional session occurrence table in the existing MAP-1 master.
No canonical knowledge schema or MAP-1 core table changes; no new version is
required for additive optional-table preservation. New sessions add an optional
occurrence SHA-256 metadata seal. Existing graph hashes retain their meaning;
exact evidence has a separate hash and archived source-file identity.

The merge streams rows: O(N log M) primary-key lookups plus per-event window
union, O(one event + its witness list) additional Python memory. The existing
Cartographer export and graph hashing still materialize structural rows and
lineage: O(V+E+L) memory; this change does not claim to remove that existing
scaling limit. Total exact SQLite storage is O(unique native events + witness
windows), and shared structural prefixes are stored once. AUTO67 edge lineage
also grows with retained witnesses; its JSON union has quadratic worst-case
replay cost per hot edge. It is suitable for bounded sessions, not unlimited
per-instruction production tracing. The output queue remains capacity 16384.

Required lookup index is the existing occurrence_id primary key. Prefix views
scan retained evidence using SQLite JSON expansion and sort by run/epoch/CPU/
window/native sequence; they use O(max_window_events) Python memory and may
use SQLite sort memory or temporary disk. The default guard is 65536 events per
window; overflow fails explicitly rather than truncating or marking a partial
path complete. No global all-path dictionary or exponentially expanded trie is
stored. Worst-case structural fanout is the number of distinct observed targets,
up to address-space limits; every valid target remains represented. A future UI
can page this iterator or build disposable indexes in the same store.

Raw evidence may be archived only with source hash, exact events, capture bounds
and unique-branch witnesses preserved and independently replayable. No automatic
aging/deletion was added. Aggregate window counts must not be summed as unique
execution counts: use distinct native occurrence IDs. Overlapping capture views
may show the same execution more than once and explicitly retain identical IDs.

## Query / visualization boundary

runtime_path_view.iter_runtime_paths accepts either the existing MAP-1 connection
or canonical knowledge.sqlite and optionally a PC prefix. It returns the whole
retained ordered window, full structural signature, run/epoch/CPU, exact native
IDs/sequences, window provenance and terminal next address. It never joins paths
merely because their prefix matches. Loops retain ordered multiplicity.

Existing global_object_view supplies bytes/claims, observed incoming/outgoing
relations, static relations, evidence and conflicts. Instruction semantics only
exist where separately decoded; scenario names not present in source evidence
remain unknown. Frequency and unique-run counts derive from native IDs, not
truth promotion. Missing continuations remain unresolved. No new 3D UI was
implemented; these APIs provide evidence to the existing visualization layer.

## Validation

The new focused suite covers the ten requested cases, DBcc multiplicity,
RTS and RTE, overlapping windows, bounded-view failure, same-ID conflict atomicity,
occurrence-hash tampering, AUTO65 full-structure equality, AUTO67 100-witness
retention and shared-prefix divergence, and canonical bridge emission invariance.
Existing test_identical_branch_occurrences_are_not_suppressed is preserved.

Validation results and publication identity are recorded in WORKLOG.md and the
final task response. No gameplay capture, authoritative ownership update or
Stage7 promotion is part of this change.

Final local acceptance: PASS_CHAIN_FUSION_REWORK. New suite 18/18; Debug
218/218; Release 218/218; fresh snapshot smoke 21/21; Linux link and smoke 3/3.
Source limit and diff checks passed. No ownership or accepted branch changed.
