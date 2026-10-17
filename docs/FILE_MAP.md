# M12 Controlled Entity / Player Provenance V1

`docs/reports/THOR_ACCEPTED_ARTIFACT_BACKLOG_20260928.md` inventories existing
accepted evidence that could contribute ROM properties through the separate
`ACCEPTED_ARTIFACT` layer. It records exact W5 resource results and separates
ROM-backed graphics candidates from runtime-only SAT/VDP evidence.

## M14 canonical full-ROM address map V1

`src/tools/thor_evidence/canonical_full_rom_map.py` is the semantic-neutral
full-ROM interval map API. It imports the existing emission partition, enforces
canonical ROM identity and exact ownership accounting, supports byte lookup,
boundaries, deterministic map operations, promotion gates, map deltas, and
unknown-range ranking.

`tests/canonical_full_rom_map_test.py` covers lookup, coverage-preserving
closure operations, promotion rejection, and fail-closed baseline import.

`src/tools/thor_evidence/m14_reconcile_ownership.py` locates an accepted
post-Stage7 generation, verifies the exact ownership transition and byte
roundtrips, and regenerates the canonical report from authoritative SQLite
emission rows. It rejects stale Stage7/full-ROM receipts and ownership
accounting mismatches.

`src/tools/thor_evidence/m14_runtime_validation.py` validates sealed M14.1
runtime receipts, computing ownership and coverage from the authoritative
SQLite emission table.

`src/tools/thor_evidence/knowledge_generation.py` records deterministic
derivations, proposal operations, and generation lineage in the canonical
knowledge SQLite schema.

## M12 generic recursive ROM closure V1

- `src/tools/thor_evidence/generic_recursive_closure.py` — normalized
  post-run, subsystem-neutral provenance graph, register/memory backtrace,
  call/table discovery, fail-closed queue, exact range round-trip promotion,
  and deterministic artifact emission.
- `tests/generic_recursive_closure_test.py` — synthetic generic regression for
  RAM selector → indexed ROM table → ROM value → A0 → renderer/SAT tail and
  unresolved-evidence queue behavior.

## M13 generic recursive closure live integration V1

- `tools/bizhawk-native-ring/generic_flow_normalizer.py` — converts native
  FLOW V2 records into versioned, subsystem-neutral instruction, memory,
  ROM-read, control-flow, call, return and indirect-target collections while
  reporting evidence fields absent from the native ABI.
- `src/tools/thor_evidence/rom_read_consumer_index.py` — imports canonical-ROM
  receipt-bound FLOW V2 bus reads into a deterministic per-read JSONL corpus
  and reader-to-ROM-range index; caller ancestry remains explicitly unknown.
- `tests/rom_read_consumer_index_test.py` — synthetic FLOW import, physical
  mapping and duplicate event regression.
- `tools/bizhawk-native-ring/live_forward_generic_closure_stage.py` — seals a
  normalized FLOW corpus, checks canonical ROM identity, runs closure, emits
  the M13 receipt and deterministic capture-gap ranking.
- `tools/bizhawk-native-ring/generic_closure_handoff.py` — coordinator handoff
  that continues to ASM closure on PASS/NO_DELTA/SKIPPED results and stops on
  integrity failure.
- `tools/bizhawk-native-ring/pipeline_outcomes.py` — typed stage result and
  scheduling-effect policy; preserves semantic unresolved reasons separately
  from fatal failures.
- `tools/bizhawk-native-ring/semantic_stage_scheduler.py` — executes ordered
  semantic stages, continues recognized unresolved outcomes into generic
  closure, and returns fatal stops without downstream scheduling.
- `tools/bizhawk-native-ring/live_forward_cleanup_acceptance.py` — verifies
  generic-closure deterministic replay and gap-ranking receipts before raw-FLOW
  reclaim is allowed.
- `tests/live_forward_generic_closure_stage_test.py` — sealed-corpus identity,
  missing-corpus and zero-promotion gap-plan regressions.
- `tests/generic_flow_normalizer_test.py` — FLOW V2 identity, CPU/domain,
  opcode verification, memory linkage, control-flow and explicit missing
  register-snapshot coverage.
- `docs/reports/THOR_M13_3_GENERIC_EVIDENCE_CONTRACT_V2_V1.md` — live V2
  coverage, closure metrics, retained evidence identity, and unresolved
  acceptance gaps.
- `tests/live_forward_controlled_entity_stage_test.py` — semantic unresolved
  continuation through generic/ASM/full audit/cleanup and fatal-stop boundary.
- `tests/live_forward_postrun_progress_test.py` — distinct unresolved terminal
  UI state and truthful final pipeline labeling.
- `tests/live_forward_absorption_cleanup_test.py` — requires validated generic
  closure, ranked-gap and deterministic-replay artifacts before deletion.
- `docs/reports/THOR_M13_2_GENERIC_CLOSURE_GAP_DRIVEN_IMPROVEMENT_V1.md` —
  artifact-derived baseline, replay metrics, blocker ranking, and next-capture
  plus raw-retention contract.
- `docs/reports/THOR_PER_READ_ROM_CONSUMER_EVIDENCE_20261001.md` — restored
  receipt-bound per-read ROM consumer evidence, hashes, target intersection,
  and capture limitations.
- `docs/reports/carver-family-index-v1.json` — fail-closed exact Carver family
  membership index and reader intersections; unresolved families retain no
  synthesized intervals.
- `docs/reports/carver-family-index-v2.json` — audit of historical Carver
  family-ID derivation and blocked independent re-derivation, with provenance
  and a separate reader-driven pivot candidate.
- `docs/reports/THOR_002888_CONSUMER_CONTRACT_20261001.md` — static and
  runtime audit of the reusable one-word queue consumer at `0x002888`, kept
  separate from any UNKNOWN-family parser or classification claim.

## Structural consumer-family evidence plane

- `src/tools/thor_evidence/static_family_direct_root.py` — versioned removable
  structural ledger for exact direct-literal ROM consumer roots.
- `src/tools/thor_evidence/gfx3820_3c07c_structural_closure.py` and
  `tests/gfx3820_3c07c_structural_closure_test.py` — two-run machine-code
  source-chain verification, exact `0x3820` boundary accounting, direct-root
  contribution and removal replay.
- `docs/reports/THOR_GFX3820_3C07C_STRUCTURAL_CLOSURE_20261004.md` and
  `docs/reports/THOR_GFX3820_3C07C_STRUCTURAL_20261004/` — accepted 8,564-byte
  structural contribution, A/B source-chain artifacts, before/after/removal
  views, and closure receipt.

- `docs/reports/THOR_STRUCTURAL_FAMILY_RANKING_AFTER_FIXED1208_REJECTION_20261004.md` —
  pre-closure ranking snapshot against the 1,240,094-byte AUTO52 view; records
  FIXED_1208 physical-mapping rejection and later GFX3820_3C07C closure.

- `docs/reports/THOR_STRUCTURAL_FAMILY_RANKING_POST_AUTO51_20261002.md` — exact
  candidate unions re-intersected with the accepted 1,207,195-byte view;
  selects AUTO8 by unique delta per estimated proof cost.

- `src/tools/thor_evidence/gfx_static_stream_pointers_auto51_closure.py` —
  closes the finite AUTO51 pointer inventory with exact ROM references,
  independent A/B enumeration, decoder ends, overlap, and removal replay.
- `tests/gfx_static_stream_pointers_auto51_closure_test.py` — AUTO51 census,
  pointer membership, ledger integrity, and removal acceptance checks.
- `docs/reports/THOR_GFX_STATIC_STREAM_POINTERS_AUTO51_20261002/` and
  `docs/reports/THOR_GFX_STATIC_STREAM_POINTERS_AUTO51_20261002.md` —
  machine census, accepted contribution, structural views, and closure report.

- `src/tools/thor_evidence/m68k_auto50_multi_resource_closure.py` — closes
  AUTO50's three descriptor/table/direct-loader mechanisms with deterministic
  A/B enumeration, Python/native decoder agreement, exact overlap, and
  independently removable contributions.
- `tests/m68k_auto50_multi_resource_closure_test.py` — AUTO50 census,
  accounting, source evidence, and full/independent removal acceptance tests.
- `docs/reports/THOR_M68K_AUTO50_STRUCTURAL_20261002/` and
  `docs/reports/THOR_M68K_AUTO50_STRUCTURAL_20261002.md` — machine censuses,
  accepted subfamily contributions, structural views, and closure report.

- `src/tools/thor_evidence/static_family_descriptor_stream.py` — v3 removable
  ledger schema for exact descriptor records plus variable-length decoder
  streams and explicit provenance edges.
- `src/tools/thor_evidence/gfx_descriptor_streams_auto49_closure.py` — fresh
  A/B AUTO49 descriptor census, independent parser boundary checks, exact
  overlap accounting, and removal replay.
- `tests/gfx_descriptor_streams_auto49_closure_test.py` — acceptance and
  fail-closed schema regressions for the AUTO49 family.
- `docs/reports/THOR_GFX_DESCRIPTOR_STREAMS_AUTO49_20261002/` — machine
  enumeration, accepted contribution, structural views, and closure receipt.

- `docs/reports/THOR_STRUCTURAL_FAMILY_RANKING_POST_1M_20261002.md` — historical
  pre-AUTO49 ranking against the then-current 1,030,892-byte view; retained for
  chronology and reproduction, not a current ranking authority.
- `docs/reports/THOR_STRUCTURAL_FAMILY_RANKING_POST_AUTO49_20261002.md` — exact
  remaining-family intervals re-intersected with the 1,103,176-byte view;
  selects AUTO50 by unique delta per estimated proof cost.
- `docs/reports/THOR_STRUCTURAL_FAMILY_RANKING_POST_AUTO50_20261002.md` — exact
  candidate unions re-intersected with the accepted 1,179,195-byte view;
  selects AUTO51 by unique delta per estimated proof cost.
- `docs/reports/THOR_STRUCTURAL_FAMILY_RANKING_POST_AUTO8_20261002.md` — exact
  remaining-family intervals re-intersected with the 1,228,211-byte view;
  selected AUTO52, now accepted.
- `docs/reports/THOR_STRUCTURAL_FAMILY_RANKING_POST_AUTO52_20261003.md` — exact
  remaining-family intervals re-intersected with the accepted 1,240,094-byte
  view; ranking and stale-baseline audit. Its FIXED_1208 candidate was later
  rejected by the source-backing audit.
- `src/tools/thor_evidence/fixed_1208_source_audit.py` — independent raw-root
  and decoded-instruction census for the eight 1,208-byte candidate slots;
  fails closed when logical address backing is not physical ROM.
- `tests/fixed_1208_source_audit_test.py` — checks A/B equality, exact
  candidate arithmetic, and rejection of unresolved SRAM-overlaid mapping.
- `docs/reports/THOR_FIXED_1208_SOURCE_AUDIT_20261004/` and
  `docs/reports/THOR_FIXED_1208_SOURCE_AUDIT_20261004.md` — machine A/B rows,
  source hashes, exact candidate ranges, SRAM mapping blocker, and closure
  outcome; no contribution was accepted.

- `docs/reports/THOR_LARGE_STRUCTURAL_FAMILY_CANDIDATES_20261001.md` — ranked
  exact structural-family candidates through post-AUDIO_MODE1 re-ranking;
  Family #4 `GFX_LEVEL_RESOURCE_FIELD1` was the latest TOP-1 and is now closed.

- `src/tools/thor_evidence/static_family_evidence.py` — provenance-bound,
  independently removable `STATICALLY_ENUMERATED_CONSUMER_SOURCE` ledger and
  derived range view; never inputs the runtime property map, `TOTAL_PROVEN`,
  canonical ownership, or `SOURCE_OWNED`.
- `tests/static_family_evidence_test.py` — deterministic generation, exact
  physical union, fail-closed contract validation, source hash, mixed-ROM,
  authority-separation, and removal/rebuild regressions.
- `src/tools/thor_evidence/static_family_variable.py` — versioned variable-
  length family contribution, exact range union, provenance graph validation,
  source verification, derived view, and independent removal support.
- `src/tools/thor_evidence/gfx_screen_roots_structural_closure.py` —
  deterministic A/B census verifier and `GFX_SCREEN_ROOTS` family closure.
- `tests/static_family_variable_test.py` — exact interval, deterministic hash,
  ambiguous descriptor, boundary, and root-domain regression cases.
- `docs/reports/THOR_GFX_SCREEN_ROOTS_20261001/` — Debug/Release census
  inputs, per-stream machine enumeration, accepted contribution, before/after
  and removal views, and closure acceptance report.
- `docs/reports/THOR_GFX_SCREEN_ROOTS_STRUCTURAL_CLOSURE_20261001.md` — human
  summary of exact family accounting, provenance, and validation.
- `src/tools/thor_evidence/static_family_table_variable.py` — v3 table-rooted
  variable-length contribution validator and removable structural view.
- `src/tools/thor_evidence/audio_mode0_structural_closure.py` — independent
  descriptor-census regeneration, exact Mode-0 boundary proof, overlap
  accounting, and removal replay.
- `tests/static_family_table_variable_test.py` — selector completeness,
  exact length boundary, descriptor uniqueness, provenance hash, and removal
  regressions.
- `docs/reports/THOR_AUDIO_MODE0_STRUCTURAL_20261002/` — dual parser census,
  67-resource enumeration, accepted contribution, structural views, and
  machine acceptance report.
- `docs/reports/THOR_LARGE_STRUCTURAL_FAMILY_2_20261002.md` — candidate rank,
  proof-cost ratio, exact contract, and closure result for family #2.
- `src/tools/thor_evidence/static_family_pointer_table.py` — v4 absolute
  pointer-table ledger for variable-length parser-terminated streams; validates
  exact table census, target mapping, boundary evidence, provenance, source
  hashes, and removable structural views alongside v1/v2/v3 contributions.
- `src/tools/thor_evidence/gfx3820_indexed_pointer_closure.py` — canonical
  ROM table and `0x00D3B2` opcode-contract validation, 0x3820 exact
  source-consumed A/B enumeration, overlap accounting, source verification,
  and byte-exact removal replay for the 107-stream family.
- `tests/gfx3820_indexed_pointer_closure_test.py` — exact census, structural
  accounting/removal, missing-boundary, pointer/provenance, and hash-drift
  fail-closed cases.
- `docs/reports/THOR_GFX3820_INDEXED_STRUCTURAL_20261002/` — two matching
  complete censuses, 107-resource machine enumeration, accepted contribution,
  before/after/removal structural views, and closure receipt.
- `docs/reports/THOR_GFX3820_FILE_BACKEND_ASSESSMENT_20261004.{md,json}` —
  discovery-only assessment of the accepted 107-ID graphics family as a
  future file-backed native resource-loader target; stores hashes and ranges,
  not extracted assets.
- `docs/reports/THOR_CROSS_POOL_OBJECT_TARGET_QUERY_CANDIDATE_20261004.md` —
  raw `0xB922` cross-pool query contract, native replacement boundary, and
  differential-oracle limitations.
- `docs/reports/THOR_B922_ORACLE_ATTEMPT_AND_ANIMATION_PIVOT_20261004.md` —
  bounded full-oracle attempt result, static CFG evidence, animation pivot
  limits, and unchanged authority totals.
- `build/b922-oracle-attempt/fixed-count-frontier.json` — ignored summary of
  the accepted fixed-count record family's exact ID/alias and parser contract
  as the next externalization candidate.
- `build/b922-oracle-attempt/` — ignored bounded M68K slice plus fresh
  selector/descriptor and relative-table analysis artifacts for the single
  `0xB922` oracle attempt and subsequent animation-discovery pivot.
- `src/tools/generate_gfx_screen_roots_manifest.py` — validates the accepted
  root/descriptor/stream census and emits the provenance-bound screen-root
  manifest draft.
- `tests/gfx_screen_resource_reference.cpp` — finalizes decoder hashes with
  the native 0x3820 decoder and verifies all ROM fallbacks, exact-copy file
  overrides, and invalid-override fallback.
- `docs/reports/THOR_GFX_SCREEN_ROOTS_FILE_BACKEND_20261004.md` — accepted
  stream-ID contract, alias accounting, portability acceptance, authority
  limits, and next direction.
- `docs/reports/THOR_LARGE_STRUCTURAL_FAMILY_3_20261002.md` — exact acceptance
  accounting and provenance summary for the indexed graphics family.
- `src/tools/thor_evidence/static_family_gfx_level_field1.py` — validates the
  exact 99-row field1 table census and the removable 35-stream accepted ledger.
- `src/tools/thor_evidence/gfx_level_resource_structural_closure.py` — runs
  independent native decoder censuses, checks field1 target/alias mapping,
  pins parser boundaries and source hashes, and emits exact overlap and removal
  replay views for Family #4.
- `tests/gfx_level_resource_structural_closure_test.py` — table pointer drift,
  parser boundary disagreement, alias accounting, and claim-boundary regressions.
- `docs/reports/THOR_GFX_LEVEL_RESOURCE_FIELD1_20261002/` — identical A/B
  machine censuses, 35-row enumeration, accepted evidence contribution,
  before/after/removal views, and machine closure receipt.
- `docs/reports/THOR_LARGE_STRUCTURAL_FAMILY_4_20261002.md` — Family #4 exact
  boundary, overlap, deterministic regeneration, and removal acceptance report.
- `src/tools/thor_evidence/static_family_fixed_count.py` — validates the
  removable structural evidence ledger for the 99-entry count-bounded M68K
  record family without entering the runtime property plane.
- `src/tools/thor_evidence/fixed_count_record_structural_closure.py` — performs
  independent AUTO11/raw-table enumerations, pins source hashes, recomputes
  structural overlap, and verifies removal replay for
  `M68K_FIXED_COUNT_RECORD_FIELD0`.
- `tests/fixed_count_record_structural_closure_test.py` — validates the
  emitted census/contribution contract and fail-closed mutations.
- `tests/fixed_count_resource_reference.cpp` — canonical-ROM acceptance for
  96 row resolutions, 78 stream fallbacks, exact copies, aliases, and rejection
  fallback cases.
- `docs/reports/THOR_M68K_FIXED_COUNT_FILE_BACKEND_20261004.md` — portable
  loader contract, exact-copy acceptance, and authority separation.
- `docs/reports/THOR_MAIN_POOL_SELECTOR_FIXED_COUNT_LINK_20261004.md` — static
  main-pool selector to fixed-count stream/consumer-field linkage.
- `docs/reports/THOR_M68K_FIXED_COUNT_FIELD0_20261002/` — A/B censuses,
  machine enumeration, accepted structural contribution, structural views,
  and closure receipt for the fixed-count family.
- `docs/reports/THOR_M68K_FIXED_COUNT_FIELD0_20261002.md` — exact family
  accounting, boundary contract, and authority-separation summary.
- `src/tools/thor_evidence/gfx_direct_static_structural_closure.py` — validates
  three static source roots, A0-preserving `0x3820` call chains, independent
  decoder boundaries, family accounting, and removal replay for the direct
  graphics stream family.
- `tests/gfx_direct_static_structural_closure_test.py` — checks identical
  independent censuses, exact provenance edges, structural delta, source hashes,
  and byte-exact removal receipts.
- `docs/reports/THOR_GFX_DIRECT_STATIC_CONSUMER_STREAMS_20261002/` — A/B
  machine censuses, exact enumeration, accepted structural contribution,
  resulting/removal views, and closure receipt.
- `docs/reports/THOR_GFX_DIRECT_STATIC_CONSUMER_STREAMS_20261002.md` — exact
  direct-root/continuation contract, instance boundaries, and claim limits.

`src/tools/m12_dynamic_sat_shadow_discovery.py` is the bounded, payload-free
M12 analyzer for distinguishing fixed/template writes to the accepted
`0x00FF13CC` SAT shadow from post-template dynamic patches and for inventorying
alternative SAT DMA groups. It emits the five `postrun_*` JSON artifacts under
ignored `build/thor-evidence/` output directories.
`tests/m12_dynamic_sat_shadow_discovery_test.py` covers writer-contract and
callback-alias fail-closed behavior. The result report is
`docs/reports/THOR_M12_DYNAMIC_SAT_SHADOW_MUTATION_DISCOVERY_V1.md`.

`docs/reports/THOR_M12_UPSTREAM_CONTROLLED_OBJECT_DISCOVERY_V1.md` records the
fail-closed upstream classification of `0xFF13CC` as a SAT shadow buffer. It
separates the exact fixed ROM render-record producer and DMA/SAT relation from
the still-unproven gameplay RAM object, X/Y source, and input provenance.

`src/tools/re_bizhawk_m12_targeted_controlled_entity.lua` is the bounded,
developer-only real BizHawk observer for the targeted controller/entity capture;
it records deliberate input transitions, frame-end events, PC-window register
snapshots, and selected bus events while keeping Worker capture disabled.
`tools/bizhawk-native-ring/run_targeted_controlled_entity_capture.py` launches
it in an isolated install, and
`tools/bizhawk-native-ring/analyze_targeted_controlled_entity_capture.py`
performs the fail-closed post-run acceptance audit. Raw capture artifacts remain
under ignored `build/thor-evidence/` directories.

`tools/bizhawk-native-ring/live_forward_controlled_entity_stage.py` is the
developer-only fail-closed stage after `GAMEPLAY RAM / ENTITY CANDIDATES` and
before `ASM CLOSURE`. It consumes accepted gameplay candidate/SAT artifacts and
preserved FLOW records, identifies exact controller-port and input-RAM events,
requires same-control-flow input consumers before accepting an entity update,
recovers only evidence-backed fields, joins proven RAM→SAT→DMA/hardware pieces,
and gates `CONTROLLED_ENTITY`/`PLAYER` labels. It emits
`postrun_controlled_entity_analysis.json`, `postrun_controlled_entity_fields.json`,
`postrun_input_entity_chains.json`, `postrun_controlled_entity_sat_chain.json`,
and `postrun_controlled_entity_receipt.json`; it never changes SOURCE_OWNED.
`tests/live_forward_controlled_entity_stage_test.py` covers event identity,
exact input consumers, field/SAT continuation, slot-reuse fail-closed behavior,
the PLAYER gate, deterministic analysis, zero ownership delta, and automatic
stage ordering.

# M12 Post-Run Gameplay RAM / Entity Candidates V1

`tools/bizhawk-native-ring/live_forward_gameplay_stage.py` consumes preserved
FLOW V2 M68K RAM bus evidence plus accepted Sprite/SAT provenance to discover
fail-closed RAM structure candidates, exact or hypothesized strides, runtime
slot lifetimes, update-loop witnesses, RAM-to-SAT field chains, and
candidate-to-hardware-sprite relations. It emits six deterministic post-run
artifacts and keeps SOURCE_OWNED unchanged.
`tests/live_forward_gameplay_stage_test.py` covers stride proof, slot iteration,
lifetime/reuse, exact and partial RAM-to-SAT provenance, one-to-many ownership,
overlap deduplication, label exclusion, deterministic receipts, zero ownership
delta, and automatic coordinator ordering.

# M12 Post-Run Sprite / SAT Analysis Stage V1

`tools/bizhawk-native-ring/live_forward_sprite_stage.py` implements the repository-native post-run sprite and SAT analysis stage (placed between Stage 8 `VDP / DMA ANALYSIS` and Stage 10 `ASM CLOSURE`), consuming preserved W3 V2 FLOW records, decoding Genesis Sprite Attribute Table (SAT) entries and link lists, handling H40/H32 mode base addresses, tracking scanline raster limits (20 sprites / 320 pixels per line, X=0 masking, clipping), tracking exact DMA-to-SAT provenance, verifying regression against contract and durable real oracle `sprite_oracle_v1_f779` (frame 779) with 0 conflicts, enforcing zero SOURCE_OWNED delta, and emitting the 6 authoritative artifacts (`postrun_sprite_analysis.json`, `postrun_sat_state.json`, `postrun_sprite_frames.json`, `postrun_sprite_provenance.json`, `postrun_sprite_candidates.json`, `postrun_sprite_receipt.json`).
`tools/bizhawk-native-ring/capture_real_sprite_oracle.py` provides standalone capture and verification tooling for the durable real sprite oracle, extracting hardware VRAM/CRAM/VSRAM/SAT state from BizHawk and generating oracle manifests and receipts.
`tests/live_forward_sprite_stage_test.py` covers 19 comprehensive unit tests (Tests A–S) verifying exact SAT base decode, entry decode, link traversal, termination, cycle detection, partial SAT handling, direct vs persisted provenance, DMA-to-SAT linkage, worker deduplication, sprite geometry, flip, palette, clipping, scanline raster limits, no logical entity naming, deterministic receipts, zero source-owned delta, automatic pipeline progression, and durable real sprite oracle regression.

# M12 Post-Run VDP / DMA Analysis Stage V1

`tools/bizhawk-native-ring/live_forward_vdp_stage.py` implements the repository-native post-run VDP and DMA analysis stage (placed between Stage 7 `AUDIO ANALYSIS` and Stage 9 `ASM CLOSURE`), consuming preserved W3 V2 FLOW records, decoding Genesis VDP control commands, register state, DMA operations (68K_BUS, FILL, COPY), memory-to-VDP producer provenance, candidate ROM graphics ranges, verifying S8 frame-779 regression with zero conflicts, and emitting the 6 authoritative artifacts (`postrun_vdp_analysis.json`, `postrun_vdp_registers.json`, `postrun_vdp_dma.json`, `postrun_vdp_provenance.json`, `postrun_vdp_candidates.json`, `postrun_vdp_receipt.json`).
`tests/live_forward_vdp_stage_test.py` covers 14 comprehensive tests (Tests A–N) verifying W3 V2 record consumption, complete two-word VDP command decoding, incomplete command fail-closed handling, register write decoding, VRAM/CRAM/VSRAM target distinction, exact DMA parameter derivation, non-exact DMA classification, equality-only causality rejection, frame/segment identity enforcement, S8 frame-779 regression with 0 conflicts, candidate ROM range clustering without promotion, source-owned invariance, idempotent deterministic receipt emission, automatic pipeline progression, and the 500-line limit across all governed files.

# M12 Post-Run Audio Analysis Stage V1

`tools/bizhawk-native-ring/live_forward_audio_stage.py` implements the repository-native post-run audio analysis stage (placed between Stage 6 `CONTROL PROVENANCE` and Stage 8 `ASM CLOSURE`), consuming preserved W3 V2 FLOW records, performing exact resource recognition and W5 roundtrip decode/encode verification for accepted resources (Resource 1 `0x0BD540..0x0BF768` and Resource 2 `0x0BC95C..0x0BD540`), verifying Level 3 strict causal chains with provenance tags, clustering newly discovered banked ROM reads into non-promoted candidate ranges, and emitting the 4 authoritative artifacts (`postrun_audio_analysis.json`, `postrun_audio_candidates.json`, `postrun_audio_provenance.json`, `postrun_audio_receipt.json`).
`tests/live_forward_audio_stage_test.py` covers 11 comprehensive tests (Tests A–K) verifying clean execution, exact resource recognition, byte-exact W5 roundtrip, Level 3 strict causal provenance, candidate clustering without promotion, source-owned invariance, zero fallback defaults, deterministic receipts generation, pipeline progression, schema validity, and the 500-line limit across all governed files.

# M12 Post-Run Pipeline Modernization V1 receipts architecture

`tools/bizhawk-native-ring/live_forward_postrun_receipts.py` generates all eight authoritative post-run receipts (`postrun_map_refresh_receipt.json`, `postrun_control_provenance_receipt.json`, `postrun_audio_receipt.json`, `postrun_vdp_receipt.json`, `postrun_asm_closure_receipt.json`, `postrun_full_rom_audit.json`, `postrun_cleanup_receipt.json`, `postrun_final_receipt.json`) deterministically from pipeline artifacts without fallback defaults, authoritatively deriving Stage 7 promotion deltas and pipeline deltas.
`tests/live_forward_postrun_receipts_test.py` covers unit tests verifying deterministic generation, schema conformance, fail-closed validation, non-zero Stage 7 promotion derivation, and synthetic zero-promotion fail-closed semantics.

# M12 W5d functional audio validation and listening test V1

`src/tools/thor_evidence/w5_audio_functional.py` implements the functional audio validation and listening pipeline, including raw PCM and WAV preview export, timing analysis, checksum-conserved semantic mutation within the early-boot witness window, emulator execution, standalone prediction vs hardware DAC verification, and report generation.
`tests/w5_audio_functional_test.py` covers 11 focused functional tests (Tests A–K) verifying header rejection, exact stream boundaries, deterministic PCM SHA, isolation, WAV format fidelity, timing classification, semantic mutation encoding, temp ROM checksum conservation, modified emulator DAC stream prediction with 0 mismatches, and canonical ROM immutability.

# M12 W5c exact audio resource ownership and canonical promotion V1

`src/tools/thor_evidence/w5_audio_promote.py` implements the canonical promotion engine for reconstructed audio resources `AUDIO_RESOURCE_FORMAT_A_0001` and `AUDIO_RESOURCE_FORMAT_A_0002` into the canonical ROM knowledge map, handling set-theoretic overlap reconciliation, parent emission tri-split, KnowledgeStore insertion, idempotence validation, and map/report re-export.
`src/tools/thor_evidence/w5_audio_ownership_receipt.py` generates the 7 required W5c acceptance artifacts in `build/m12-w5-acceptance/`.
`tests/w5_audio_ownership_test.py` covers 20 unit and integration tests (Tests A–T) verifying ownership eligibility, overlap reconciliation, partition split, dry-run invariants, canonical tooling execution, idempotence, diff accuracy, fail-closed boundaries, and ADR compliance.

# M12 W5 exact audio resource format and round-trip V1

`src/tools/thor_evidence/w5_audio_format.py` provides the data model, delta lookup tables, bank base mapping, descriptor parsing, and canonical Intermediate Representation (IR) for Beyond Oasis non-linear delta-PCM audio (Format A).
`src/tools/thor_evidence/w5_audio_decode.py` implements pure deterministic decoding of compressed audio data into canonical IR tokens and PCM sample streams for both Mode 0 (standard) and Mode 1 (hold/interpolate).
`src/tools/thor_evidence/w5_audio_encode.py` implements pure deterministic encoding from canonical IR tokens back to byte-identical original ROM binary streams without reading original ROM bytes.
`src/tools/thor_evidence/w5_audio_pipeline.py` orchestrates bank descriptor discovery, resource selection, decode, encode, byte-identical round-trip verification, witness alignment, and emission of the 9 acceptance artifacts in `build/m12-w5-acceptance/`.
`tests/w5_audio_format_test.py` covers 18 unit and integration tests (Tests A–R) verifying format semantics, bank parsing, boundary proof, witness alignment, byte-identical round-trip, multi-bank generality, and gate audit.

# M12 W4 active audio resource analysis V1

`src/tools/thor_evidence/w4_audio_sink.py` tracks hardware audio ports (YM2612 ports 0x4000-0x4003, PSG port 0x7F11), enforces port-local address latch pairing (0x4000->0x4001, 0x4002->0x4003) without cross-port leakage, and identifies DAC data (reg 0x2A) and enable (reg 0x2B) registers.
`src/tools/thor_evidence/w4_z80_dataflow.py` tracks exact Z80 register, stack, and memory dataflow provenance, supporting instruction transfers with conservative termination on unsupported opcodes.
`src/tools/thor_evidence/w4_audio_provenance.py` implements the 4 causal levels (Level 0 Temporal, Level 1 Memory last-writer, Level 2 Dataflow, Level 3 Strict Causal Chain), driver hotspot analysis, ROM clustering, candidate resource classifications, and directed provenance graph generation.
`src/tools/thor_evidence/w4_audio_pipeline.py` provides the end-to-end pipeline generating the 9 canonical acceptance artifacts in `build/m12-w4-acceptance/`.
`tests/w4_audio_analysis_test.py` covers unit tests for port-local pairing, DAC/PSG identification, last-writer verification, Z80 dataflow provenance, and artifact generation.

# M12 W3 Z80 co-capture + cross-CPU timeline

`tools/bizhawk-native-ring/live_forward_z80_trace.c` implements native Z80 instruction
capture and Z80 bus event sidebands (RAM, banked ROM, YM2612, PSG, bank register).
`tools/bizhawk-native-ring/genesis-plus-gx-z80-cocapture-v1.patch` and
`tools/bizhawk-native-ring/bizhawk-2.11.1-z80-cocapture-v1.patch` contain the external
GPGX and BizHawk patch chains for Z80 execution loop, memory, and bank hooks.
`tests/live_forward_z80_test.cpp` provides the 22 required C tests A–V for Z80 capture,
banked ROM physical resolution, cross-CPU master_time ordering, and M68K regression.
`src/tools/thor_evidence/w3_z80_evidence.py` provides lossless V2 48-byte record decoding,
stream iteration, and Genesis Z80 bank register tracking.
`src/tools/thor_evidence/w3_cross_cpu_timeline.py` builds the cross-CPU common master_time
timeline and validates exact causal interactions across 68K and Z80.
`src/tools/thor_evidence/w3_z80_classification.py` classifies low-level primitive facts
(Z80 RAM, Banked ROM, YM2612, PSG, 68K Z80-window writes) without song/instrument semantics.
`tests/w3_z80_evidence_test.py` validates the Python analysis layer with 13 unit tests.

# Runtime Z80 ROM execution-origin tracking

- `tools/bizhawk-native-ring/graphics_caller_census.c/.h` — bounded,
  non-authoritative runtime event buffer for attempted `0x3820` caller census.
  The current JSON integration is blocked; see the 2026-09-30 census report.

# Ephemeral graphics resource transaction

`tools/bizhawk-native-ring/graphics_transaction_bridge.c/.h` provide bounded
per-byte decoder-output origin tracking, overwrite invalidation and partial
RAM-to-VDP publication records. JSON receipt serialization is isolated in
`graphics_transaction_bridge_json.c`; `graphics_transaction_sha256.c/.h`
provide the local hash primitive used for decoder output and published spans.
`graphics_publication_path_diag.c/.h` hold a bounded recent-event diagnostic
for CPU VDP writes and DMA path families; it is observational only and does not
promote map properties. `tools/bizhawk-native-ring/export_graphics_transaction_tiles.py`
validates captured decoder/transfer hashes and exports only byte ranges that
were actually published into a grayscale tile diagnostic sheet.
`graphics_transaction_runtime.c` owns the emulator-facing bounded collector
instance and its narrow lifecycle/VDP API. Its caller-scoped observer records
only a `0x03C07C` call into `0x3820`, checking the immediate JSR handoff, saved
register/stack context, returned boundary and physical ROM mapping; a missing
caller hit remains unclassified. `apply_runtime_rom_properties_v1.py` installs
the required bridge sources and compile define into the diagnostic checkout.
`tests/graphics_transaction_bridge_test.c` validates deterministic hashes,
partial transfer linkage and byte-specific invalidation.

`src/tools/thor_evidence/graphics_resource_transaction.py` validates and
deterministically assembles one bounded 0x3820 -> RAM -> VDP -> SAT -> visible
sprite receipt. It fails closed on identity, ordering, range, transfer-content,
frame, publication-coverage, and visibility gaps; it does not mutate any map or
property authority. `tests/graphics_resource_transaction_test.py` covers
successful replay and core rejection conditions. Runtime evidence collection
has a host-tested collector. `runtime-build-inputs/graphics-transaction-lifecycle-v1/`
freezes the lifecycle collector sources and original overlay inputs.
`runtime-build-inputs/graphics-publication-path-diag-v1/` contains the bounded
publication-path diagnostic overlay and exact source snapshots. The current
collector retains completed decoder output across a bounded 120-frame
publication window, clearing origin on overwrite, run/reset/loadstate changes,
or timeout. Cross-frame matching is an intermediate diagnostic, not yet a
closed graphics resource receipt.

`tools/bizhawk-native-ring/rom_properties_z80_origin.c/.h` owns the bounded
Z80 RAM origin table, lifecycle invalidation, exact fetched-byte validation,
and completed-instruction origin batch. `rom_properties_runtime.c` applies
those validated offsets to the runtime property map; metrics remain in
`rom_properties_z80_metrics.c/.h`. The pinned GPGX patcher includes these
modules in the Waterbox build. The bounded live acceptance report and all
runtime-only checkpoints/contributions remain under ignored
`build/z80-origin-live-acceptance/`.

`tools/bizhawk-native-ring/live_forward_v3_abi.py` independently audits the
frozen Runtime V3 32-byte FLOW ABI against
`flow_schemas/runtime-v3-flow32.json`; historical 48-byte auditing remains in
`live_forward_scaling_audit.py`. `live_forward_v3_acceptance.lua` supplies the
V3 bounded runtime capture with ABI-correct API field indices.
`tests/live_forward_v3_abi_test.py` covers the known byte fixture, layout and
identity rejection, malformed lengths, capabilities and segment validation.
The format-audit report is `docs/reports/RUNTIME_V3_FLOW_FORMAT_AUDIT_20260929.md`.
The follow-up V3 runtime-gate receipt is
`docs/reports/RUNTIME_V3_ACCEPTANCE_CLOSE_20260929.md`; it records replay,
checkpoint, removable contribution, throughput and same-run audio blockers.

# M12 S8 proven logical VDP frame

`src/tools/re_bizhawk_m12_vdp_frame.lua` is a read-only BizHawk capture for
one coherent frame's ordered VDP register writes and complete VRAM/CRAM/VSRAM.
`src/tools/m12_vdp_frame_artifact.py` reuses the S1 tile/attribute decoder,
renders Plane A, Plane B, Window and backdrop, and composes the S7 sprite
raster with per-pixel provenance. `tests/m12_vdp_frame_artifact_test.py`
covers the A–U S8 contract. Generated capture and frame JSON remain ignored
runtime artifacts under `build/m12-gfx-runtime`.

# M12 S7 proven sprite scanline raster

`src/tools/m12_sprite_scanline_raster.py` reads the unchanged S6 frame and
observed VDP mode evidence, derives H32/H40 limits, evaluates each scanline
in SAT-link order, and emits `hardware-sprite-raster-v1` with logical and
hardware-filtered matrices plus per-pixel suppression provenance.
`tests/m12_sprite_scanline_raster_test.py` covers synthetic count, dot-budget,
transparency, offscreen, masking, flip, mode, determinism, and fail-closed
boundaries. The generated raster and mode-derived evidence remain ignored
runtime artifacts under `build/m12-gfx-runtime`.

# M12 S6 proven hardware sprite frame

`src/tools/m12_sprite_frame_artifact.py` consumes the S5 piece catalog,
validates a complete SAT-link traversal, and emits the deterministic
`hardware-sprite-frame-v1` JSON proof with exact tile/CRAM records, sprite-only
composition matrices, and pixel-level provenance. `tests/m12_sprite_frame_artifact_test.py`
covers the A–N fail-closed frame contract. The generated frame artifact is an
ignored runtime file under `build/m12-gfx-runtime`.

# M12 S5 SAT persistence and causality

`src/tools/m12_sprite_piece_catalog.py` now distinguishes
`DIRECT_PUBLICATION` from `PERSISTED_FROM_PUBLICATION`, proves SAT and shadow
persistence only under complete supported mutation coverage, and retains
entry-specific causal status. `tests/m12_sprite_persistence_test.py` covers
the A–L persistence contract. S5 mutation receipts are emitted by the
developer-only BizHawk capture and remain under `build/m12-gfx-runtime`.

# M12 S4 sprite evidence gap closure

`src/tools/m12_sprite_evidence_gap.py` produces the exact reason histogram,
frame/SAT-entry and shape distributions, before/after classification counts,
and measured capture-cost report. `tests/m12_sprite_evidence_gap_test.py`
covers the A–L gap-closure contract. The expanded S3/S4 capture records
bounded SAT-window DMA segments with source offsets and event sequences.

# M12 S3 hardware sprite piece catalog

`src/tools/m12_sprite_piece_catalog.py` classifies each `(run_id, frame,
SAT-entry)` observation as PROVEN, INCOMPLETE, OBSERVED_LINKAGE_ONLY, or
CONFLICT, preserving exact multi-tile/CRAM evidence and deterministic semantic
fingerprints. `tests/m12_sprite_piece_catalog_test.py` covers the A–L
fail-closed catalog contract. The generated catalog and capture remain ignored
runtime artifacts under `build/m12-gfx-runtime`.

# M12 S2 hardware sprite piece

`src/tools/re_bizhawk_m12_sprite_piece.lua` is a developer-only, read-only
BizHawk witness capture for one same-frame producer → shadow SAT → VRAM SAT →
VRAM tile → CRAM chain. `src/tools/m12_sprite_piece_artifact.py` validates the
capture and emits the compact `hardware-sprite-piece-v1` JSON proof using the
S1 Genesis decoder. `tests/m12_sprite_piece_artifact_test.py` covers the A–K
fail-closed contract.

# THOR M12 AUTO64.1 invariant repair

# THOR M12 AUTO65 multi-chain campaign

`src/tools/thor_evidence/auto65_chain.py` owns normalized chain construction,
structural equivalence, novelty classes, prefix reuse, conflict retention and
shared-prefix clustering. `auto65_campaign.py` owns fixed-point scenario
selection, anti-repeat fingerprints, static-before-runtime scheduling,
investigation persistence and replay accounting. The regression contract is
`tests/thor_evidence_auto65_test.py`, registered in `cmake/m12_auto2.cmake`.
The real campaign receipt and ignored DAG are under
`build/thor-evidence/auto65`; the published result is
`docs/reports/THOR_M12_AUTO65_MULTI_CHAIN_CAMPAIGN.md`.

# THOR M12 AUTO66 multi-scenario campaign

`src/tools/thor_evidence/auto66_campaign.py` owns the repository scenario
planner, equivalent-scenario rejection, bounded BizHawk discovery dispatch,
multi-scenario receipts and cumulative campaign metrics while reusing AUTO65's
chain engine. Its regression is
`tests/thor_evidence_auto66_test.py`, registered in
`cmake/m12_auto2.cmake`; the result is recorded in
`docs/reports/THOR_M12_AUTO66_MULTI_SCENARIO_CAMPAIGN.md` and ignored runtime
evidence is under `build/thor-evidence/auto66`.

# THOR M12 AUTO67 live opportunistic RE

`src/tools/thor_evidence/auto67_live.py` launches the canonical ROM with the
developer-only live sampler, owns the atomic Dispatcher, bounded rolling-window
accounting, free worker pool, claims/merges, and snapshot-backed operator view.
`src/tools/thor_evidence/auto67_window.py` renders that replaceable snapshot in
the parallel native operator window; it has no raw-event backlog or Dispatcher
access. `auto67_status.py` retains file publication and explicit browser
compatibility mode.
`auto67_runner.py` contains launcher/process orchestration so the Dispatcher
remains below the source-file limit; `auto67_profile.py` keeps bounded T0-T8
dispatch and claim-lock timing samples. The live capsule Lua path records
bounded frame spikes and filtered-hook installation timing.
`capture/auto67_hook_metrics.lua` adds bounded hook registration, callback
self-time, per-frame, and callback-budget diagnostics for AUTO67.2.
`src/tools/thor_evidence/capture/live_opportunistic.lua` performs only minimal
frame/bus sampling and fixed-ring updates inside emulator callbacks. The view
is served from the same launcher and reads `*.view.json`; it has no raw event
backlog and cannot claim or process work. `tests/thor_evidence_auto67_test.py`
covers claims, known/active collision handling, worker return, bounded overwrite,
and 1/8/16 worker configuration. The validation and evidence are recorded in
`docs/reports/THOR_M12_AUTO67_LIVE_OPPORTUNISTIC_RE.md`.
The dispatch-only timing proof is recorded in
`docs/reports/THOR_M12_AUTO67_DISPATCH_PROFILE.md` and its bounded JSON
counterpart; raw runtime receipts remain under `build/`.
AUTO67.2 hook scaling and hunt evidence are recorded in
`docs/reports/THOR_M12_AUTO67_2_HOOK_DIAGNOSIS.md` and its JSON proof.
`auto67_yield.py` is retained as historical AUTO67.2 code but is no longer
imported by the AUTO67 acceptance path. Its session-only yield classifications
and unresolved-context rejection are intentionally disabled. The native
operator window does not show those classifications; regression coverage for
the chain-store-first boundary remains in `tests/thor_evidence_auto67_1_test.py`.
The measured AUTO67.2 yield proof is
`docs/reports/THOR_M12_AUTO67_2_KNOWLEDGE_YIELD.md` and its bounded JSON
counterpart; the full ignored runtime artifact is under `build/`.

`auto67_persistence.py` contains `LiveMapSink`, the AUTO67-only single bounded
local-chain queue and RAM SQLite Cartographer writer. `map_merge.py` performs
the post-shutdown deterministic session-to-GLOBAL merge with an atomic replace.
The historical descriptor/Store
bridge is isolated in `auto67_legacy_persistence.py`; its consumers are
`tests/thor_evidence_auto67_3_test.py`,
`tests/thor_evidence_auto67_3_identity_audit.py` and
`tests/thor_evidence_auto67_4_test.py`. The live runner never imports that
module. The authoritative AUTO67.3 checkpoint is
  `docs/reports/THOR_M12_AUTO67_3_FINAL_CANONICAL_CHAIN_IDENTITY_CHECKPOINT.md`
  and its JSON counterpart; earlier AUTO67 iteration reports are historical.

`src/tools/thor_evidence/auto67_status.py` handles bounded, lossy status
publication and HTTP serving for the existing launcher. The AUTO67 regression
also exercises slow-disk isolation, nonblocking snapshot contention, and
bounded/detached history. `THOR_M12_AUTO67_LIVE_OPERATOR_TEST.md` and its JSON
under `docs/reports/` distinguish failed manual responsiveness from the
subsequent sparse-capture performance experiment and its coverage limitation.

# M12 AUTO67 live-forward RAM Cartographer 2A

`src/tools/thor_evidence/live_forward_cartographer.py` adapts only completed,
host-validated FLOW_V1 segments into a fresh RAM MAP-1 observed execution graph
and saves a closed SQLite session by backup. `live_forward_archivist.py`
validates and atomically seeds or merges those saved sessions using
`map_merge.py`. `tools/bizhawk-native-ring/live_forward_cartographer_runtime.py`
coordinates two isolated EmuHawk sessions and invokes the Archivist only after
each process exits; the optional callback in `live_forward_scaling_runtime.py`
is downstream of full segment validation and immediately before ACK.
`tests/live_forward_cartographer_test.py` covers observed edges, overlap,
branches, lineage, session round-trip, status preservation, merge idempotency
and fail-closed archive behavior. `tools/bizhawk-native-ring/live_forward_cartographer_audit.py`
independently reconciles every segment receipt against both session databases
and the merged master. Evidence and real BizHawk receipts belong in the ignored
`build/thor-evidence/live-forward-worker-2a/` campaign directory.

# M12 FLOW_V1 exact ROM range linkage 2B

`src/tools/re_rom_range_decode.cpp` is a developer-only batch adapter over the
existing bounded M68K decoder and explicit memory-address classifier.
`src/tools/thor_evidence/live_forward_rom_link.py` stages already-audited
FLOW_V1 bytes and, after EmuHawk exits, adds stable `ROM_INSTRUCTION_RANGE`
nodes, occurrence-lineaged `EXECUTED_FROM_ROM` claims and terminal
`OBSERVED_NEXT_PC` address facts to the 2A MAP-1 session. The one-run driver is
`tools/bizhawk-native-ring/live_forward_rom_link_runtime.py`; its independent
saved-result verifier checks every instruction occurrence and terminal fact
against raw segments, a fresh decode and canonical ROM bytes. Deterministic
fixtures are in `tests/live_forward_rom_link_test.py`, registered by
`cmake/m12_auto2.cmake`. Interval output and raw FLOW_V1 evidence belong only
under the ignored `build/thor-evidence/live-forward-rom-link-2b/` directory.
Implementation, runtime evidence and final audit are recorded in
`docs/reports/THOR_M12_ROM_RANGE_LINKAGE_2B.md` and its compact JSON receipt.

`live_forward_rom_link_runtime.py` gates post-run analysis on a sealed
interactive stop outcome; `tests/live_forward_rom_link_runtime_test.py` keeps
preflight rejection from opening a misleading analyzer failure window.

`docs/reports/THOR_M12_RUNTIME_CONTROL_PROVENANCE_2C.md` and its compact JSON
receipt record the Phase 0 classification of the saved 2B FLOW_V1 evidence.
The bounded result is `STOP_NO_RUNTIME_PROVENANCE_GAP`; no native runtime
sensor or per-memory-access trace was added.

# M12 canonical ROM knowledge map 2D

`src/tools/thor_evidence/rom_knowledge_map.py` owns the deterministic SQLite
schema, stable ROM-range/object identities, queries, metrics, hashes and
compact export. `rom_knowledge_sources.py` imports the accepted AUTO61
manifest, reconciles its ownership history, preserves Carver candidates as
hypotheses and imports qualified 2B facts. `rom_knowledge_import.py` builds
the ignored `build/thor-evidence/canonical-rom-knowledge-2d/knowledge.sqlite`
and the tracked JSON/Markdown checkpoint reports. The independent read-only
auditor is split between `rom_knowledge_audit.py` and
`rom_knowledge_audit_records.py`; focused deterministic query/idempotence tests
are in `tests/rom_knowledge_map_test.py`, registered by
`cmake/m12_auto2.cmake`. Runtime sessions, raw FLOW and the generated SQLite
map remain local under ignored `build/`.

`src/tools/thor_evidence/auto67_capsule.py` owns the fixed sixteen-slot,
128 KiB capsule pool, bounded command publication, capture freeze/reuse
metrics, and capsule status snapshots. `src/tools/thor_evidence/capture/live_capsule.lua`
owns the discovery ring, targeted hooks, per-frame callback budget, and frozen
capsule binary output. `tests/thor_evidence_auto67_1_test.py` covers exact pool
shape, the four-capture limit, freeze/reuse, dispatcher release, and bounded
control state. The experiment report is
`docs/reports/THOR_M12_AUTO67_1_128K_CAPSULE.md` with its bounded JSON proof.

AUTO67.4 adds `auto67_capsule_codec.py`, a bounded O67V v2 decoder with
legacy O67C compatibility and lease/capsule validation, and
`auto67_materializer.py`, the worker-side conversion of decoded runtime
observations into explicitly unresolved evidence fields. `auto67_capsule.py`
exposes frozen capsule metadata and rejects stale status snapshots; new
materialized results are persisted as `MATERIALIZED_CHAIN` while historical
seed-only rows remain `SEED_ONLY`. Regression coverage is in
`tests/thor_evidence_auto67_4_test.py`; the real short proof is recorded in
  `docs/reports/THOR_M12_AUTO67_4_FROZEN_CAPSULE_REAL_WORKER_CHAIN.md` and its
  bounded JSON counterpart.

AUTO67.5 extends `auto67_materializer.py` with schema-2 observed/causal
separation and a bounded generic M68K MOVE memory-write decoder. Its integrity
tests and real proof are recorded in
`tests/thor_evidence_auto67_4_test.py` and
`docs/reports/THOR_M12_AUTO67_5_CAUSAL_INTEGRITY_GATE.md` with the JSON proof.

AUTO67.6R2 extends `auto67_predecessor.py` with the O67P v2 bounded global
prehistory decoder and fail-closed reaching-definition resolver.
`capture/predecessor_capture.lua` owns one continuously active 4096-entry
execution ring and exact BUS_WRITE joins; `capture/live_capsule.lua` exposes
frozen slice metadata through the existing bounded status path.
`tests/thor_evidence_auto67_6_test.py` covers exact occurrence identity, latest
reaching writes, ROM-PC opcode recovery, generic indexed/PC operands, gaps,
truncation, overwrites, epoch mismatch, unsupported writers and generic
register selection. The R2 checkpoint is
`docs/reports/THOR_M12_AUTO67_6R2_CONTINUOUS_PREHISTORY.md` with its JSON
counterpart. The proof is positive for register evidence but records the
measured global-hook frame-time limitation; it does not claim smooth gameplay.

AUTO67.6R2's `auto67_predecessor.py` also validates continuous, complete,
audited FLOW_V1 intervals for runtime control provenance. The bounded
post-run replay is in `auto67_control_provenance.py`; exact 68000 EA/register
semantics are isolated in `auto67_control_provenance_decode.py`. The
independent occurrence/source/target auditor and BizHawk campaign driver are
`tools/bizhawk-native-ring/control_provenance_audit.py` and
`control_provenance_campaign.py`. `tests/thor_evidence_control_provenance_test.py`
covers exact pointer, offset, copy, selected-entry, generic-register and
fail-closed cases; CTest runs it. The checkpoint distinguishes the emulator
capability proof from the canonical-game witness in
`docs/reports/THOR_M12_RUNTIME_CONTROL_PROVENANCE_2E.md` and its JSON receipt.
Raw captures and per-segment outputs remain under ignored `build/`.

`src/tools/re_full_split_run.py` owns the canonical generated-layout alias
filter and deterministic full-layout writer; `src/tools/re_auto_promote.py`
delegates to it. `tests/re_full_split_test.py` and
`tests/re_auto_promote_test.py` cover equivalent/conflicting aliases and
deterministic layout output. `tests/check_file_limits.cmake` checks the
governed Git inventory under `cmake/`, `src/`, `tests/`, and root
`CMakeLists.txt` without walking ignored runtime/build trees. Evidence and
validation are recorded in
`docs/reports/THOR_M12_AUTO64_1_INVARIANT_REPAIR.md`.

# THOR Evidence Engine design

# AUTO64 Knowledge Coverage and scheduler

`src/tools/thor_evidence/auto64_knowledge_coverage.py` imports existing AUTO62,
AUTO63, runtime and bounded static artifacts into multidimensional fail-closed
coverage without changing ownership. `auto64_provenance.py` persists the
dependency graph and deterministic request, `auto64_provenance_analyze.py`
updates it from the focused capture, and `auto64_static_queue.py` consumes the
remaining bounded static obligations. Their focused developer-only capture is
`capture/auto64_caller_provenance.lua` with its PowerShell runner. Tests are
`tests/thor_evidence_auto64*_test.py`; ignored evidence is under
`build/thor-evidence/auto64-provenance`. The result is recorded in
`docs/reports/THOR_M12_AUTO64_KNOWLEDGE_COVERAGE.md`.

`docs/reports/THOR_EVIDENCE_ENGINE_ARCHITECTURE.md` specifies the proposed
developer-only M12 capture/provenance sidecar, relational schema, temporal
identity, proof obligations and non-owning Carver integration.
`docs/reports/THOR_EVIDENCE_ENGINE_STAGES.md` defines bounded V0–V9 goals,
acceptance tests, performance budgets, STOP gates and transition evidence.
The listed future source modules are a plan; no engine implementation is
introduced by these reports. See ADR-0044.

# THOR Evidence Engine V0 foundation

`src/tools/thor_evidence/identity.py` defines full-hash environment/location/
temporal identities. `events.py` validates the sealed JSONL transport,
`normalize.py` converts one bounded local BizHawk observation into that format,
and `report.py` emits the evidence-backed capability matrix and source receipts.
`receipt.py` creates/finalizes launch identity; `capture/run_v01.ps1` binds the
existing launcher to that receipt for the bounded repeat. The V0.1 collector
raw envelope is rejected before normalization when completion or identity is
wrong.
`store.py` and `schema.sql` provide the non-owning SQLite receipt, epoch, event,
value-version and UNKNOWN relation store. The one bounded collector is
`src/tools/thor_evidence/capture/capabilities.lua`; its raw output remains local
under ignored `build/thor-evidence/v0`. `tests/thor_evidence_v0_test.py` covers
the storage and fail-closed transport contract; CTest registration is in
`cmake/m12_auto2.cmake`.

# THOR Evidence Engine V1-GATE local FF13CC closure

`src/tools/thor_evidence/v1_gate.py` validates the bounded A372 static form,
selected ordered runtime pairing, four-byte local versions, writer-coverage
certificates and interruption certificates. It intentionally stops before a
provenance graph or general last-writer engine. `tests/thor_evidence_v1_gate_test.py`
and `tests/fixtures/thor_evidence_v1_gate/negative_cases.json` cover the
adversarial fail-closed cases; `cmake/m12_auto2.cmake` registers the test.
The gate result is recorded in
`docs/reports/THOR_EVIDENCE_ENGINE_V1_GATE.md`.

# THOR Evidence Engine V1-GATE-COVERAGE dense local closure

`src/tools/thor_evidence/capture/dense.lua` and `run_dense.ps1` use the
existing BizHawk receipt/raw envelope to capture the minimal A370..A374
execution interval through `event.on_bus_exec_any`. `src/tools/thor_evidence/dense_gate.py`
validates ROM-bound static decoding, concrete effective write ranges,
interruption continuity and the machine-readable coverage certificate.
`tests/thor_evidence_dense_gate_test.py` and
`tests/fixtures/thor_evidence_v1_gate_coverage/negative_cases.json` cover
incomplete, reordered, forged, duplicate-PC, loop and interruption cases.
The result is recorded in
`docs/reports/THOR_EVIDENCE_ENGINE_V1_GATE_COVERAGE.md`.

# Independent BizHawk infrastructure diagnosis

`docs/reports/BIZHAWK_CONTROLLED_HARNESS_DIAGNOSIS.md` records the verified
BizHawk 2.11.1 CLI/Lua controlled-state workflow, input API correction,
process coexistence and lag-frame oracle limitation. Local diagnostic scripts
and evidence remain ignored under `build/bizhawk-controlled-harness`.
`docs/reports/THOR_M12_CONTROLLED_RUNTIME_ROOT_CAPTURE.md` records the bounded
Right-versus-neutral register/root control and the re-ranked M12 forks.

# M12-GFX-MAX closes the screen graphics root table

`src/tools/re_m12_gfx_max_closure.py` and
`tests/re_m12_gfx_max_closure_test.py` validate the complete known screen
graphics root: the 21-entry group-pointer table, 167 descriptor uses and 159
exact decompressor streams. The transaction output remains ignored/local.

`src/tools/re_m12_gfx_loader_census.py` and
`tests/re_m12_gfx_loader_census_test.py` build the exact direct-xref census for
the `0x00D406` graphics loader and split screen-descriptor call sites from
unmatched direct calls without promoting bytes. The census also records bounded
forward-adjacency candidates, verifies the 15 known small 68000 continuation
forms, and records descriptor-shaped unmatched-call candidates; the generated
census remains ignored/local.

`src/tools/re_m12_gfx_37d2_census.py` and
`tests/re_m12_gfx_37d2_census_test.py` census the seven direct `0x37D2`
graphics-wrapper calls. Five exact ROM-source setups are checked against their
Ancient boundaries and existing manifest ownership; two inherited/post-state
sites remain blocked. The generated census is ignored/local and never promotes
bytes by itself.

`src/tools/re_m12_gfx_d3b2_census.py` and
`tests/re_m12_gfx_d3b2_census_test.py` close the exact indexed `0x00D3B2`
graphics-loader family: seven direct callers, the 108-entry root table, and
all 107 finite child streams. The generated census is ignored/local and
records already-owned bytes without promoting them.

`src/tools/re_m12_gfx_36d4_census.py` and
`tests/re_m12_gfx_36d4_census_test.py` census the six direct callers of the
`0x0036D4` wrapper, verify its bounded `0x36D4..0x372A` body and
`0x3820 -> 0x2CBC` edges, and retain its inherited-A0 source as an explicit
non-owning blocker.

`src/tools/re_m12_gfx_sibling_census.py` and
`tests/re_m12_gfx_sibling_census_test.py` classify the complete direct-xref
sets for the proven `0x00D950`, `0x002CBC`, and `0x002E1E` sibling helpers.
They verify each bounded body fingerprint and record the non-owning VDP/RAM
helper classification; generated output is ignored/local.

`src/tools/re_m12_gfx_d406_residual_census.py` and
`tests/re_m12_gfx_d406_residual_census_test.py` classify the five remaining
non-screen direct `0x00D406` callers after screen continuations and descriptor
families are removed. They verify exact bounded caller fingerprints and retain
inherited/zero-source blockers without promoting bytes; output is ignored/local.

`src/tools/re_bizhawk_m12_gfx_provenance.lua` captures only the ten unresolved
`0x3820` caller PCs and decoder entry under an existing frozen natural scenario.
`src/tools/re_m12_gfx_runtime_provenance.py` validates canonical identity,
byte-identical replays, ROM `A0`, exact Ancient ends, and manifest ownership;
runtime JSON and the provenance report remain ignored/local.

# M12-AUTO56 adds the overlapping PC-relative word-table promoter

`src/tools/re_m12_pc_word_overlap_promote.py` and
`tests/re_m12_pc_word_overlap_test.py` record the exact 16-word table at
`0x062DA8`, promoting only its unknown 24-byte prefix because the final four
words overlap the existing AUTO28 table. Generated transaction output remains
ignored/local.

# M12-AUTO55 adds the bounded 0x03B8DE table promoter

`src/tools/re_m12_table_03b8de_promote.py` and
`tests/re_m12_table_03b8de_test.py` record the exact eight-record,
16-byte-stride descriptor-table contract. Generated transaction output remains
ignored/local.

# M12-AUTO52 adds the exact direct-loader graphics-stream promoter

`src/tools/re_m12_direct_loader_stream_promote.py` and
`tests/re_m12_direct_loader_stream_test.py` record three direct-loader pointer
contracts and their exact decompressor boundaries. Generated transaction output
remains ignored/local.

# M12-AUTO51 adds the exact static-stream-pointer promoter

`src/tools/re_m12_static_stream_pointer_promote.py` and
`tests/re_m12_static_stream_pointer_test.py` record nine closed static pointer
stream contracts. Generated transaction output remains ignored/local.

# M12-AUTO50 adds the exact multi-resource-family promoter

`src/tools/re_m12_multi_resource_family_promote.py` and
`tests/re_m12_multi_resource_family_test.py` record repeated descriptor/table
families and direct loader streams. Generated transaction output remains
ignored/local.

# M12-AUTO49 adds the exact descriptor-stream promoter

`src/tools/re_m12_descriptor_stream_promote.py` and
`tests/re_m12_descriptor_stream_promote_test.py` record the ten closed
descriptor-backed graphics streams. Generated transaction output remains
ignored/local.

# M12-AUTO48..46 add exact dispatch promoters

`src/tools/re_m12_pc_relative_dispatch_promote.py`,
`src/tools/re_m12_multi_dispatch_targets_promote.py`, and
`src/tools/re_m12_static_dispatch_targets_promote.py`, with their paired tests,
record the closed dispatch/table contracts. Generated transaction output
remains ignored/local.

# M12-AUTO45 adds the exact indexed offset-table promoter

`src/tools/re_m12_indexed_offset_table_promote.py` and its deterministic
`tests/re_m12_indexed_offset_table_promote_test.py` record the closed
`0x00AD56..0x00AD76` table contract. Generated transaction output remains
ignored/local.

# M12-AUTO44 adds the runtime record-region promoter

`src/tools/re_m12_runtime_record_regions_promote.py` and its deterministic
`tests/re_m12_runtime_record_regions_promote_test.py` record the new exact
count-bounded regions selected by readers `0x00AF16` and `0x00ABB4`.
Generated transaction output remains ignored/local; already-owned AUTO11
regions are explicitly excluded from the new byte count.

# M12-AUTO43 adds the small caller-backed runtime-code promoter

`src/tools/re_m12_runtime_code_small_caller_promote.py` and its deterministic
`tests/re_m12_runtime_code_small_caller_promote_test.py` record only the
exact routine `[0x0083F8,0x00846C)`. Generated evidence and transaction
output remain ignored/local.

# M12-AUTO42 adds the caller-backed runtime-code promoter

`src/tools/re_m12_runtime_code_caller_promote.py` and its deterministic
`tests/re_m12_runtime_code_caller_promote_test.py` record only the exact
caller-backed routine `[0x03B358,0x03B486)`. Generated evidence and
transaction output remain ignored/local.

# M12-AUTO41 adds the developer-only bounded runtime-code promoter

`src/tools/re_m12_runtime_code_range_promote.py` and its deterministic
`tests/re_m12_runtime_code_range_promote_test.py` record only the exact
runtime-observed routine `[0x03AE74,0x03B092)`. Generated evidence and
transaction output remain ignored/local.

# M12-AUTO40 adds the developer-only runtime-code promoter

`src/tools/re_m12_runtime_code_promote.py` and its deterministic
`tests/re_m12_runtime_code_promote_test.py` record only the runtime-observed,
byte-exact routine `[0x00B79A,0x00B852)`. Generated evidence and transaction
output remain ignored/local.

# M12-AUTO39 adds the developer-only relative-selector promoter

`src/tools/re_m12_relative_selector_table_promote.py` and its deterministic
`tests/re_m12_relative_selector_table_promote_test.py` record only the exact
`0x15A9A6..0x15A9B0` selector table. Generated transaction output remains
ignored; selected payloads remain outside ownership.

# M12-AUTO38 adds the developer-only menu record-stream promoter

`src/tools/re_m12_menu_record_stream_promote.py` and its deterministic
`tests/re_m12_menu_record_stream_promote_test.py` record only the exact
`0x15B9D4..0x15BAC2` menu offset table and count-bounded streams. Generated
transaction output remains ignored.

# M12-AUTO37 adds the developer-only enum-lookup promoter

`src/tools/re_m12_enum_lookup_promote.py` and its deterministic
`tests/re_m12_enum_lookup_promote_test.py` record only the exact
`0x05CE56..0x05CE96` table. Generated transaction output remains ignored.

# M12-AUTO36 adds the developer-only menu-graphics stream promoter
`src/tools/re_m12_menu_graphics_promote.py` and its deterministic regression
`tests/re_m12_menu_graphics_promote_test.py`. It records only the three exact
`0x15BAC2..0x15CEA0` stream intervals; the `0x15CA9B` separator and generated
transaction output remain outside ownership.

# M12-AUTO35 adds the developer-only bit-7 lookup promoter
`src/tools/re_m12_bit7_lookup_promote.py` and its deterministic regression
`tests/re_m12_bit7_lookup_promote_test.py`. It records only the exact
`0x05CE16..0x05CE56` table; generated transaction output remains ignored.

# M12-AUTO34 adds the developer-only resource-pointer promoter
`src/tools/re_m12_resource_pointer_table_promote.py` and its deterministic
regression `tests/re_m12_resource_pointer_table_promote_test.py`. It records
only the exact `0x05CE96..0x05D046` pointer table; generated transaction
output remains ignored.

# M12-AUTO33 adds the developer-only item-label promoter
`src/tools/re_m12_item_label_table_promote.py` and its deterministic
regression `tests/re_m12_item_label_table_promote_test.py`. It records only
the exact `0x05CC16..0x05CE16` 64-record table; generated transaction output
remains ignored.

# M12-AUTO32 adds the developer-only menu-label promoter
`src/tools/re_m12_label_table_promote.py` and its deterministic regression
`tests/re_m12_label_table_promote_test.py`. It records only the exact
`0x05CBA6..0x05CBD6` fixed-width label table; generated transaction output
remains ignored.

# M12-AUTO31 adds the developer-only threshold-table promoter
`src/tools/re_m12_threshold_table_promote.py` and its deterministic
regression `tests/re_m12_threshold_table_promote_test.py`. It records only
the exact `0x05D906..0x05D918` sentinel table; generated transaction output
remains ignored.

# M12-AUTO30 adds the developer-only dispatch pointer-table promoter
`src/tools/re_m12_dispatch_pointer_table_promote.py` and its deterministic
regression `tests/re_m12_dispatch_pointer_table_promote_test.py`. It records
only the exact `0x00DF54..0x00E0B8` pointer table; pointed-to handler code and
generated transaction output remain outside this source change.

# M12-AUTO29 adds the preserved candidate-map converter
`src/tools/re_candidate_map_to_ghidra.py` reconstructs the bounded
developer-only Ghidra map used to test the preserved code-census provenance
path. `tests/re_candidate_map_to_ghidra_test.py` covers deterministic
conversion; generated maps and transaction output remain ignored.

# M12-AUTO28 adds the developer-only nibble-lookup promoter
`src/tools/re_m12_nibble_lookup_promote.py` and its deterministic regression
`tests/re_m12_nibble_lookup_promote_test.py`. It records only the exact
`0x062DC0..0x062DE0` lookup; generated transaction output remains ignored.

# M12-AUTO27 adds the developer-only event dispatch promoter
`src/tools/re_m12_event_dispatch_promote.py` and its deterministic regression
`tests/re_m12_event_dispatch_promote_test.py`. It records only the exact
`0x00532C..0x005378` signed-relative event table; generated transaction output
remains ignored.

# M12-AUTO26 adds the developer-only field-86 callback promoter
`src/tools/re_m12_callback_promote.py` and its deterministic regression
`tests/re_m12_callback_promote_test.py`. It materializes only the exact
`0x10000` callback and `0x30000` RTS entrypoint; transaction output remains
ignored.

# M12-AUTO25 adds the developer-only PC-relative table promoter
`src/tools/re_m12_pc_tables_promote.py` and its deterministic regression
`tests/re_m12_pc_tables_promote_test.py`. It records only closed lookup,
fixed-record, copied-record, and VDP initialization tables; generated output
remains ignored.

# M12-AUTO24 adds the developer-only exact static-code promoter
`src/tools/re_m12_static_code_promote.py` and deterministic regression
`tests/re_m12_static_code_promote_test.py`. It records only the caller-backed
`[0x0167BE,0x01685A)` island; generated transaction output remains ignored.

# M12-AUTO23 adds the developer-only save-slot promoter
`src/tools/re_m12_save_slots_promote.py` and deterministic regression
`tests/re_m12_save_slots_promote_test.py`. It records only the exact primary
save slot and independently bounded secondary prefix; generated transaction
output remains ignored and no C++ migration or ROM asset is tracked.

# M12-AUTO20 extends the developer-only graphics promoter
`src/tools/re_m12_table_graphics_promote.py` and its deterministic regression
`tests/re_m12_table_graphics_promote_test.py` promote only wholly UNKNOWN
graphics streams selected by exact field1 pointers in the 99-row resource
table and independently bounded by the graphics census.

M12-AUTO19 extends the developer-only graphics promoter
`src/tools/re_m12_sound_data_promote.py` and its deterministic regression
`tests/re_m12_sound_data_promote_test.py` promote only the exact bounded
Ancient Music Driver sound-data archive after table and terminal-boundary
validation.

`src/tools/re_m12_runtime_graphics_promote.py` and its deterministic
regression `tests/re_m12_runtime_graphics_promote_test.py` with exact static
`LEA → D9A4 → 37D2 → 0x3820` consumers. It records two decoder-bound streams;
generated transaction output remains ignored.

# M12-AUTO17 adds the developer-only runtime-correlated graphics promoter
`src/tools/re_m12_runtime_graphics_promote.py` and its deterministic
regression `tests/re_m12_runtime_graphics_promote_test.py`. It records only
two exact decoder-bound streams with canonical runtime reader and static
pointer evidence; generated transaction output remains ignored.

# M12-AUTO9 adds the developer-only erased-alignment padding promoter
`src/tools/re_m12_padding_promote.py` and deterministic regression
`tests/re_m12_padding_promote_test.py`. It records only complete UNKNOWN
`0xFF` runs ending on 4 KiB ROM boundaries; generated transaction output
remains ignored and no C++ migration or ROM asset is tracked.

M12-AUTO8 adds the developer-only sequential direct-graphics-chain promoter
`src/tools/re_m12_direct_graphics_chain_promote.py` and its deterministic
regression `tests/re_m12_direct_graphics_chain_promote_test.py`. It records
five new decoder-verified continuation streams while revalidating three
already-owned anchors; generated transaction output remains ignored.

The separate structural closure is implemented by
`src/tools/thor_evidence/m12_auto8_direct_graphics_chain_closure.py`, tested in
`tests/m12_auto8_direct_graphics_chain_closure_test.py`, and documented in
`docs/reports/THOR_M12_AUTO8_DIRECT_GRAPHICS_CHAIN_20261002.md`. Its machine
census and removable contribution live under
`docs/reports/THOR_M12_AUTO8_STRUCTURAL_20261002/`; generated evidence remains
local and does not alter runtime or source-ownership authorities.
Its variable-range union/intersection and ledger validation are isolated in
`src/tools/thor_evidence/m12_auto8_structural_ledger.py`.

M12-AUTO52's structural closure is implemented by
`src/tools/thor_evidence/m12_auto52_direct_loader_closure.py`, with its
wrapper-specific contribution validation in
`src/tools/thor_evidence/m12_auto52_direct_loader_ledger.py` and focused
regression in `tests/m12_auto52_direct_loader_closure_test.py`. The closure
report is `docs/reports/THOR_M12_AUTO52_DIRECT_LOADER_20261003.md`; its
machine census, independently removable 37D2/36D4 ledgers, and derived views
are local under `docs/reports/THOR_M12_AUTO52_STRUCTURAL_20261003/`.

M12.1 adds the developer-only transactional promotion orchestrator
`src/tools/re_m12_1_promote.py`, its helper regression
`tests/re_m12_1_test.py`, exact status-register decoding coverage in
`src/tools/re_slice_exact.cpp`, and the M12.1 report
`docs/reports/ASM_PROMOTION_06042A_0611F4_M12_1.md`. The transaction promotes
only six exact source-owned slices; no production/core file changed.

M12.0 adds the roadmap rebase, exact ASM completion census report
docs/reports/ASM_COMPLETION_CENSUS_M12_0.md and the M12.0 state/task/ledger
updates. The report contains the machine-readable 136-range blob inventory.
No source or production/core file changed.

M11.64 adds the documentation-only bounded G0 ledger
`docs/RE_LEDGER.md`, reusable method index `docs/RE_METHOD_CATALOG.md`, and
closure report `docs/reports/G0_PORTABILITY_BOUNDARY_M11_64.md`. No source or
production/core file changed.

M11.63 adds `src/tools/hybrid/callee_623ac_observer.hpp/.cpp` and runtime
wiring for the bounded 0x0623AC natural callee contract. The deterministic
regression is `tests/hybrid_callee_623ac_test.cpp`; the report is
`docs/reports/CALLEE_0623AC_CONTRACT_M11_63.md`. No `oasis_core` file changed.

M11.62 adds `src/tools/hybrid/callee_61934_observer.hpp/.cpp` and runtime
wiring for the bounded 0x061934 natural callee contract. The deterministic
regression is `tests/hybrid_callee_61934_test.cpp`; the report is
`docs/reports/CALLEE_061934_CONTRACT_M11_62.md`. No oasis_core file changed.
# M12-AUTO6 adds the developer-only fixed-stride table promoter
`src/tools/re_m12_fixed_stride_table_promote.py` and its deterministic
regression `tests/re_m12_fixed_stride_table_promote_test.py`. It records only
the bounded 50-record table `[0x5D046,0x5D686)` selected by four exact 68000
consumers; generated transaction output remains ignored and no C++ migration
or ROM asset is tracked.

M12-AUTO7 adds the developer-only direct-graphics promoter
`src/tools/re_m12_direct_graphics_promote.py` and its deterministic regression
`tests/re_m12_direct_graphics_promote_test.py`. It records seven exact
decoder-consumed streams selected by direct `0x3820` consumers and sequential
`A0` continuation; generated transaction output remains ignored.

# M12-AUTO16 adds the developer-only exact runtime-correlated probe-slice
promoter src/tools/re_m12_exact_probe_slices_promote.py and deterministic
regression tests/re_m12_exact_probe_slices_promote_test.py. It records nine
bounded canonical-equal ASM slices totalling 1,430 bytes; generated transaction
output and local probe evidence remain ignored.

# M12-AUTO15 adds the developer-only constant-D0 CC-B0 selected-slot promoter
`src/tools/re_m12_ccb0_selected_slots_promote.py` and its deterministic
regression `tests/re_m12_ccb0_selected_slots_promote_test.py`. It records only
15 unique 16-bit relative-pointer slots selected by exact constant-D0 callers;
dynamic callers and nested table extents remain UNKNOWN.

# M12-AUTO14 adds the developer-only CC-B0 group pointer-table promoter
`src/tools/re_m12_ccb0_group_table_promote.py` and its deterministic regression
`tests/re_m12_ccb0_group_table_promote_test.py`. It records only the exact
32-entry longword table `[0x04371E,0x04379E)`; nested target subtables remain
UNKNOWN and generated transaction output remains ignored.

# File Map

M12-AUTO22 adds the developer-only exact small-table promoter
`src/tools/re_m12_exact_small_tables_promote.py` and deterministic regression
`tests/re_m12_exact_small_tables_promote_test.py`. It records only three
fixed consumer-bounded tables; generated transaction output remains ignored.

M12-AUTO21 adds the developer-only CC-B0 nested target-table promoter
`src/tools/re_m12_ccb0_target_tables_promote.py` and deterministic regression
`tests/re_m12_ccb0_target_tables_promote_test.py`. It records only exact
256-slot target windows; generated transaction output remains ignored.

M12-AUTO13 extends the developer-only record-stream promoter with the exact
BCEA field3 sentinel-list edge. It records only the five merged ranges derived
from 30 field3 bases and 100 terminated list views; generated transaction
output remains ignored and no ROM or C++ gameplay/runtime source is tracked.

M12-AUTO11 adds the developer-only count-bounded record-stream promoter
`src/tools/re_m12_record_stream_promote.py` and deterministic regression
`tests/re_m12_record_stream_promote_test.py`. It records the exact 99-record
table `[0x3F306,0x3FF66)` and the merged count-bounded six-byte stream family;
generated transaction output remains ignored and no ROM or C++ gameplay/runtime
source is tracked.

M12-AUTO5 adds the developer-only exact lookup-table promoter
`src/tools/re_m12_lookup_table_promote.py` and its deterministic regression
`tests/re_m12_lookup_table_promote_test.py`. It records the bounded
`0x5D686..0x5D706` word table and `0x5D706..0x5D906` byte table from their
68000 consumers; generated transaction output remains ignored.

M12-AUTO4 adds the developer-only nested level-table promoter
`src/tools/re_m12_level_table_promote.py` and its deterministic regression
`tests/re_m12_level_table_promote_test.py`. It records the exact
`0x5D918..0x5D958` outer table, contiguous count-bounded nested groups, and
only the 185 pointer-backed NUL-terminated record spans through `0x5E1A0`;
the generated transaction remains ignored build evidence and no C++ migration
or ROM asset is tracked.

M12-AUTO2 adds the developer-only consumer-backed graphics census
`src/tools/re_graphics_stream_census.cpp`, pointer consumer scanner
`src/tools/re_m12_pointer_resource_scan.cpp`, and transactional promoter
`src/tools/re_m12_consumer_promote.py` with helper regression
`tests/re_m12_consumer_promote_test.py`. The bounded transaction preserves
the canonical ROM while recording exact 68000 graphics consumers, the Z80
upload, screen descriptors, and fixed 1208-byte parser records. Census and
materialization outputs stay in ignored build evidence directories; the
isolated target/test definitions live in `cmake/m12_auto2.cmake`; no ROM or
extracted commercial asset is tracked.

M12-AUTO3 adds the developer-only indexed script-table promoter
`src/tools/re_m12_script_promote.py` and its deterministic regression
`tests/re_m12_script_promote_test.py`. It records the exact `0x51514` table,
the `0x00C2EC` index loop, and the NUL-terminated streams without beginning
production C++ migration; generated transaction output remains ignored.

`src/tools/thor_evidence/indexed_script_family_audit.py` independently audits
the 98-entry signed-relative script table and fail-closes structural admission
when an alternate dynamic selector bypasses the bulk-loop exclusions.
`tests/indexed_script_family_audit_test.py` covers independent offset/NUL
enumeration and the open-selector rejection for the full-table family. Its
machine census, no-op structural views, and closure report are under
`docs/reports/THOR_M68K_INDEXED_SCRIPT_FAMILY_20261004/`; the separately
scoped bulk-loop subfamily is recorded below.

`src/tools/thor_evidence/indexed_script_bulk_family.py` regenerates and accepts
the path-scoped 95-entry bulk-loop subfamily. Its versioned validator is
`src/tools/thor_evidence/static_family_indexed_strings.py`; focused ledger and
removal tests are in `tests/indexed_script_bulk_family_test.py`. The accepted
census, contribution ledger, before/after/removal structural views, and report
are in `docs/reports/THOR_M68K_INDEXED_SCRIPT_FAMILY_20261004/bulk-loop-95/`.

M12-AUTO adds the developer-only combined promoter
`src/tools/re_m12_auto_promote.py`, the compressed-resource boundary scanner
`src/tools/re_resource_boundary_scan.cpp`, their helper regression
`tests/re_m12_auto_test.py`, the M12.5 bounded report
`docs/reports/ASM_PROMOTION_003B3E_004A92_M12_5.md`, and the autonomous final
report `docs/reports/ASM_AUTONOMOUS_TO_90_PERCENT_M12_AUTO.md`. Generated
ROM/source/assets remain local ignored build outputs; no ROM or extracted
commercial asset is tracked.
M12.4 adds the developer-only ROM-start transaction
`src/tools/re_m12_4_promote.py`, its deterministic helper regression
`tests/re_m12_4_test.py`, and the report
`docs/reports/ASM_PROMOTION_000000_0007C4_M12_4.md`. The exact reassembler now
retains vasm-safe raw encoding for `MOVE USP`; full-split metrics and
materialization recognize explicit vector/header/structured ASM ownership.

M12.3 adds the developer-only transactional promoter
`src/tools/re_m12_3_promote.py`, its deterministic helper regression
`tests/re_m12_3_test.py`, the exact decoder/reassembler regression extensions,
and the bounded report
`docs/reports/ASM_PROMOTION_006516_0083D4_M12_3.md`. No production/core
runtime file or ROM/asset was added; M12.3 materialization remains under the
ignored build evidence directory.

M12.2 adds the developer-only transactional promoter
`src/tools/re_m12_2_promote.py`, its deterministic helper test
`tests/re_m12_2_test.py`, the bounded promotion report
`docs/reports/ASM_PROMOTION_00DE00_00E338_M12_2.md`, and exact decoder/
reassembler coverage for the target's ADDX, MULU, EOR/CMP and indirect-call
forms. No production/core runtime file or ROM/asset was added.

M11.61 adds `src/tools/hybrid/callee_62ae0_observer.hpp/.cpp` and the
developer-only runtime wiring for natural entry/return, A5 equality, paths,
and memory-effect evidence. `tests/hybrid_callee_62ae0_test.cpp` locks the
minimum deterministic contract. The report is
`docs/reports/CALLEE_062AE0_CONTRACT_M11_61.md`; no `oasis_core` file changed.

M11.60 adds `src/tools/hybrid/a5_lifetime_observer.hpp/.cpp` and the small
`a5_lifetime_runtime.*` bridge for opt-in natural G0 provenance, plus
`runner_support.*` to keep the developer runner below the source-size limit.
`tests/hybrid_a5_lifetime_test.cpp` locks deterministic materialization,
consumer and parent-restore evidence. The milestone report is
`docs/reports/A5_CONSUMER_LIFETIME_M11_60.md`. These are developer-only
artifacts; no `oasis_core` or production behavior changed.

M11.59 adds `docs/reports/RAW_DATA_OWNERSHIP_M11_59.md`, the all-ROM raw-data
ownership/alias/lifetime census and typed-gate result. It also adds
`tests/raw_data_provenance_test.cpp`, a decoder-only address-mode regression,
and its CTest registration in `CMakeLists.txt`. No production/core runtime
module or ROM/GPGX adapter changed.

M11.58 adds `docs/reports/PORTABLE_BEHAVIOR_CLUSTER_M11_58.md`, the raw
footprint/ownership census and cluster gate result. No source files or runtime
modules were added; the existing `src/core/parent_suffix.*` composition remains
the minimal behavior-cluster boundary and `src/tools/hybrid/candidate_parent_suffix.*`
continues to own ROM/GPGX adaptation and proof.


M11.57 additions: `src/core/parent_suffix.hpp/.cpp` owns the minimal portable
parent-owned suffix contract; `src/tools/hybrid/candidate_parent_suffix.hpp/.cpp`
owns ROM/GPGX adapter and shadow/native proof; `tests/parent_suffix_test.cpp`
covers adverse entries and every represented boundary; report:
`docs/reports/PARENT_SUFFIX_HANDOFF_M11_57.md`.

M11.56 additions: src/tools/hybrid/caller_continuation.hpp/.cpp owns the
bounded EMULATED-only parent/entry/exit and data-hook evidence observer;
src/tools/hybrid/validate_caller_continuation.py checks paired local traces,
canonical bytes, saved-frame ownership and the exact memory journal;
tests/hybrid_caller_continuation_test.cpp tests capture boundaries and negative
paths; docs/reports/RAMFLAG_CALLER_ROUTINE_M11_56.md records the exact internal
tail, shared epilogue and negative third-routine promotion result.

M11.55 additions: src/tools/hybrid/caller_attribution.hpp/.cpp owns the
developer-only natural RamFlag caller/return/register/cycle attribution schema;
src/tools/hybrid/external_library.hpp owns the shared dynamic-library wrapper;
tests/hybrid_caller_attribution_test.cpp covers deterministic attribution JSON;
docs/reports/RAMFLAG_CALLER_DATA_CLOSURE_M11_55.md records the caller CFG,
shared-data, hardware-boundary and STOP-gate evidence.
M11.54 additions: docs/reports/NATIVE_ROUTINE_CLUSTER_M11_54.md records the
bounded caller/data cluster, provenance, metrics and proposed M11.55.
M11.53 additions: src/core/ram_flag_routine.cpp/.hpp owns the portable
structured RAM flag/output routine contract; src/tools/hybrid/candidate_604bc
owns the ROM-specific per-instruction adapter; tests/ram_flag_routine_test.cpp
and tests/hybrid_candidate_604bc_test.cpp provide standalone and bridge
regressions; docs/reports/SECOND_PORTABLE_NATIVE_ROUTINE_M11_53.md records the
candidate inventory and dual promotion evidence.

This document is the canonical map of repository structure. Update it whenever structure or responsibilities change.

```text
/
├── AGENTS.md                  Mandatory rules for AI agents and contributors
├── AI_DEVELOPMENT_CONTRACT.md Operational contract for evidence/focus/completion
├── CMakeLists.txt             Top-level build configuration and CTest registration
├── PROJECT_STATE.md           Current milestone/task/status for context recovery
├── README.md                  Public project overview and build entry point
├── TASK.md                    Current task, evidence, acceptance criteria, next action
├── .github/
│   └── workflows/
│       ├── ci.yml                    Build + ordinary CTest verification
│       └── (ROM-backed probes run locally; the commercial ROM is never fetched by CI)
├── assets/gfx3820/manifest.tsv  Hash-pinned exact ResourceID 1..107 compressed/output manifest (no payloads)
├── assets/gfx3820_screen_roots/manifest.tsv  Hash-pinned root/descriptor/stream graph and unique-stream hashes (no payloads)
├── assets/fixed_count_records/manifest.tsv  Row-preserving 96-to-78 raw stream portability manifest (no payloads)
├── docs/
│   ├── ARCHITECTURE.md        Layering, dependencies, translation strategy
│   ├── DECISIONS.md           Architecture decision records (ADR-style)
│   ├── DEVELOPMENT_RULES.md   Coding, testing and documentation rules
│   ├── FILE_MAP.md            This canonical repository map
│   ├── PORTING.md             Portability boundary/target notes
│   ├── PROJECT_VISION.md      Goal, scope, non-goals and end-state
│   ├── RE_TOOLCHAIN_GUIDE.md  Historical Mega Drive SDK/toolchain evidence and fingerprinting rules
│   ├── REASSEMBLY_POC.md     M11.9/M11.10 bounded reassembly reproduction and measured results
│   ├── FULL_ROM_SPLIT_BASELINE.md M11.11 full-ROM split manifest and exact baseline report
│   ├── AUTO_BLOB_PROMOTION.md M11.12 transactional blob-to-source promotion report
│   ├── AUTO_PROMOTION_SCALE.md M11.13 100-attempt promotion scale report
│   ├── AUTO_PROMOTION_BATCH2.md M11.14 large batch II and saturation report
│   ├── EVIDENCE_INTEGRITY_AUDIT.md M11.15 exactness/trust classification audit
│   ├── TARGETED_DYNAMIC_CONFIRMATION.md M11.16 bounded natural evidence report
│   ├── STRUCTURED_DATA_CLASSIFICATION.md M11.17 bounded data classification report
│   ├── NATIVE_VERTICAL_SLICE.md M11.18 native controlled-screen boundary and status
│   ├── reports/
│   │   └── ASM_COMPLETION_CENSUS_M12_0.md M12.0 full-ROM ASM completion census
│   │   └── NATIVE_ROM_RESOURCE_ID3.md M11.25 verified native resource baseline
│   │   └── NATIVE_ROM_RESOURCE_ID3_VISUAL_ROLE.md M11.26 bounded visual-role report
│   │   └── HOT_PATH_MULTI_BLOCK_COVERAGE_M11_39.md M11.39 bounded hot-path coverage report
│   │   └── REMAINING_INTERPRETER_ATTRIBUTION_M11_40.md M11.40 baseline-blocked attribution report
│   │   └── CHECKPOINT_IDENTITY_M11_41.md M11.41 checkpoint provenance/repair report
│   │   └── REMAINING_INTERPRETER_ATTRIBUTION_M11_42.md M11.42 restart-gate blocker report
│   │   └── CHECKPOINT_CANONICALIZATION_M11_43.md M11.43 exact GPGX state-layout and identity proof
│   │   └── REMAINING_INTERPRETER_ATTRIBUTION_M11_44.md M11.44 exhaustive pre-promotion interpreter ledger
│   │   └── REMAINING_INTERPRETER_ATTRIBUTION_FINAL_M11_44.md M11.44 final post-promotion interpreter ledger/Pareto
│   │   └── SEMANTIC_CLOSURE_M11_45.md M11.45 candidate selection and shadow/native proof
│   │   └── REMAINING_INTERPRETER_ATTRIBUTION_M11_45.md M11.45 exhaustive final interpreter ledger
│   │   └── RUNTIME_ADDRESS_PROVENANCE_M11_46.md M11.46 bounded runtime address and memory-class evidence
│   │   └── SAFE_MEMORY_SEMANTIC_CLOSURE_M11_47.md M11.47 safe-memory semantic and primitive proof
│   │   └── REMAINING_INTERPRETER_ATTRIBUTION_M11_47.md M11.47 exhaustive post-promotion interpreter ledger
│   │   └── NATIVE_MECHANICAL_PRIMITIVE_M11_48.md M11.48 resumable native mechanical primitive proof
│   │   └── MECHANICAL_PRIMITIVE_FAMILY_M11_49.md M11.49 mechanical primitive family closure proof
│   │   └── FIRST_PORTABLE_NATIVE_ROUTINE_M11_51.md M11.51 routine extraction and replacement gate
│   │   └── NATIVE_ROUTINE_MISMATCH_M11_52.md M11.52 continuation mismatch closure and promotion proof
│   │   └── SECOND_PORTABLE_NATIVE_ROUTINE_M11_53.md M11.53 second routine contract and dual promotion proof
│   │   └── RAMFLAG_CALLER_DATA_CLOSURE_M11_55.md M11.55 caller/data/hardware closure evidence
│   │   └── PARENT_SUFFIX_HANDOFF_M11_57.md M11.57 parent-owned helper and handoff proof
│   ├── M11_19_TEST_A.md       M11.19 external Genesis-Plus-GX live-coverage report
│   ├── REVERSE_ENGINEERING.md Address/routine/ROM/data research ledger
│   ├── ROADMAP.md             Ordered milestones and current active milestone
│   ├── TASK_TEMPLATE.md       Mandatory task/session handoff template
│   ├── VDP_MODEL.md           Narrow portable video-state model and non-goals
│   └── WORKLOG.md             Chronological record of development actions
├── src/
│   ├── main.cpp               Canonical ROM gate and native runtime entry point
│   ├── core/
│   │   ├── rom.cpp            ROM file loading/basic title access
│   │   ├── rom.hpp            ROM byte-container API
│   │   ├── rom_identity.cpp   Header/checksum/hash/known-revision identification
│   │   ├── rom_identity.hpp   ROM identity models and public API
│   │   ├── mechanical_primitive.cpp Portable proven copy/clear/DBF executor
│   │   ├── mechanical_primitive.hpp Portable primitive contract and continuation API
│   │   ├── table_copy_routine.cpp Portable structured table-copy routine executor
│   │   ├── table_copy_routine.hpp Routine contract, machine interface and continuation API
│   │   ├── ram_flag_routine.cpp Portable structured RAM flag/output routine executor
│   │   ├── ram_flag_routine.hpp RAM flag/output contract and continuation API
│   │   ├── runtime.cpp        Explicit deterministic frame stepping
│   │   └── runtime.hpp        Portable controller/input/frame runtime API
│   ├── genesis/
│   │   ├── memory_bus.cpp     Minimal Mega Drive address-space compatibility
│   │   ├── memory_bus.hpp     Memory bus interface
│   │   ├── vdp.cpp            Bounded VRAM/CRAM/VSRAM storage/access
│   │   ├── vdp.hpp            Narrow VDP state API
│   │   └── vdp_types.hpp      Tile/plane/sprite raw attribute decoding
│   ├── game/
│   │   ├── controlled_screen.cpp Native screen controller supporting synthetic fixture, ROM screens, transitions, combat collision, loot drop spawning, pickup collection, and spirit summoning
│   │   ├── controlled_screen.hpp Controlled-screen state, ROM loader, transition API, and spirit slot integration
│   │   ├── screen_instance.cpp ScreenInstance loader, decompressor, and view rebind implementation
│   │   ├── screen_instance.hpp ScreenInstance runtime model for ROM-backed screens
│   │   ├── render/
│   │   │   ├── debug_overlay.cpp Bitmap font diagnostic overlay text rasterizer
│   │   │   ├── debug_overlay.hpp Diagnostic text overlay API
│   │   │   ├── framebuffer.cpp Software framebuffer storage
│   │   │   └── framebuffer.hpp Packed software framebuffer API
│   │   ├── graphics_decompress.cpp Native translation of original 0x3820 routine
│   │   ├── graphics_decompress.hpp Decompressor result/API
│   │   ├── audio_format_a.cpp Native Format-A selector parser and bounded Mode 0/1 PCM decoder candidate
│   │   ├── audio_format_a.hpp Format-A descriptor, selector ID and decode result API
│   │   ├── genesis_graphics.cpp Pure 4bpp tile + CRAM palette decoding
│   │   ├── genesis_graphics.hpp Graphics decoder data types/API
│   │   ├── resource_loader.cpp Hash-verified ResourceID 1..107 ROM/file graphics loader
│   │   ├── resource_loader.hpp GFX3820 ResourceID loader and migration status API
│   │   ├── screen_resource_loader.cpp Root-path to stable stream-ID loader with exact file override and ROM fallback
│   │   ├── screen_resource_loader.hpp Screen-root manifest/alias and backend result API
│   │   ├── fixed_count_resource_loader.cpp Row-to-stream resolver, exact raw override validation and ROM fallback
│   │   ├── fixed_count_resource_loader.hpp Neutral fixed-count record API and backend result
│   │   ├── resource_diagnostic.cpp Native VRAM transfer and tile-atlas renderer
│   │   ├── resource_diagnostic.hpp Diagnostic resource visualization API
│   │   ├── translated_routines.cpp Initial translated compatibility routines
│   │   ├── translated_routines.hpp Their public declarations
│   │   ├── player/
│   │   │   ├── player.cpp     Portable input, state, terrain-gated movement, hurt/knockback physics, melee attack state machine, damage pipeline, and item pickup healing
│   │   │   └── player.hpp     Player movement API, damage state, melee attack state/hitbox API, item pickup API, combat routines, and ROM evidence constants
│   │   ├── entities/
│   │   │   ├── entity_pool.cpp  Raw entity-pool bounds and active-record view
│   │   │   ├── entity_pool.hpp  Entity-pool descriptors and raw field offsets
│   │   │   ├── screen_entity.cpp ROM-backed entity descriptors, frame piece decoding, animation player, interaction logic, moving actor state machine, attack hitboxes, damage reception, death lifecycle, and loot drop entity spawning
│   │   │   └── screen_entity.hpp ScreenEntity, EntityDescriptor, AnimationState, ActorRuntime (with HP/hurt/death), EntityHitbox, directional attack hitbox, loot drop spawning, EntityInteractionState, and SpritePiece public API
│   │   ├── scripts/
│   │   │   ├── event_router.cpp  Raw event producer and handler-range mapping
│   │   │   └── event_router.hpp  Event RAM addresses and raw router API
│   │   ├── spirits/
│   │   │   ├── spirit_slots.cpp  Evidence-backed slot, active spirit state machine, Dytto summon, follow tracking, active command decoding, and water bubble projectile pipeline
│   │   │   └── spirit_slots.hpp  Spirit slot/target/dispatch constants, ActiveSpirit model, active command API, and public declarations
│   │   └── world/
│   │       ├── byte_grid.cpp          8-pixel world grid and footprint aggregation
│   │       ├── byte_grid.hpp          Bounded world-grid view API
│   │       ├── screen_descriptor.cpp Screen descriptor table reader
│   │       ├── screen_descriptor.hpp Screen descriptor data types/API
│   │       ├── terrain_collision.cpp  Terrain-state and movement gate semantics
│   │       └── terrain_collision.hpp  Terrain gate API
│   ├── platform/
│   │   ├── window.cpp Minimal Win32 window/input/presentation adapter; non-Windows stub
│   │   └── window.hpp Platform boundary for controller polling and framebuffer presentation
│   └── tools/
│       ├── hybrid/
│       │   ├── caller_attribution.hpp/.cpp Developer-only natural RamFlag caller attribution and JSON evidence
│       │   ├── external_library.hpp Developer-only cross-platform external-library loading wrapper
│       │   ├── generated_block_runtime.cpp Helpers used by generated blocks
│       │   ├── generated_block_runtime.hpp Generated-block helper boundary
│       │   ├── basic_block_reference.cpp Generic prediction adapter for generated blocks
│       │   ├── basic_block_reference.hpp Generic generated-block prediction API
│       │   ├── generated_blocks.cpp Generated historical M11.33 bodies
│       │   ├── generated_blocks_m1137.cpp Generated M11.37 single-instruction bodies
│       │   ├── generated_blocks_m1138.cpp Generated M11.38 multi-instruction bodies
│       │   ├── generated_blocks_m1139.cpp Generated M11.39 hot-path bodies
│       │   ├── generated_blocks_m1144.cpp Generated M11.44 0x03A7AE block body
│       │   ├── generated_blocks_m1145_*.cpp Generated M11.45 semantic candidate bodies and registry fragments
│       │   ├── generated_blocks_m1147.cpp Generated M11.47 safe-memory candidate bodies
│       │   ├── generated_blocks_m1147_registry.cpp Generated M11.47 registry fragment
│       │   ├── mechanical_primitive.hpp/.cpp Hybrid ROM registry, canonical adapter and evidence glue
│       │   ├── runner_report.hpp/.cpp Developer-only hybrid summary detail serializer
│       │   ├── generated_block_registry.cpp Generated registry metadata/glue
│       │   ├── generated_blocks.hpp Generated block declarations and registry API
│       │   ├── basic_block.cpp Handwritten generic shadow/native boundary glue
│       │   ├── basic_block.hpp BlockExit contract, registry state and metrics
│       │   ├── interpreter_profile.cpp Ranked interpreter fallback profile writer
│       │   ├── interpreter_profile.hpp Interpreter profile writer API
│       │   ├── interpreter_ledger.cpp Exhaustive decoder-backed interpreter attribution CLI
│       │   ├── address_provenance.hpp/.cpp Developer-only runtime address observer and memory classification
│       │   ├── recomp_generator.cpp Decoder-to-C++ generator with fail-closed forms
│       │   ├── recomp_generator.hpp Generator model/emitter API
│       │   ├── checkpoint_evidence.cpp Raw checkpoint evidence and canonical identity adapter
│       │   ├── checkpoint_evidence.hpp Checkpoint identity/evidence API
│       │   ├── gpgx_checkpoint_layout.cpp Pinned GPGX ABI layout and representation span table
│       │   ├── gpgx_checkpoint_layout.hpp Checkpoint layout contract API
│       │   ├── runner.cpp Developer-only GPGX hybrid scenario runner
│       │   └── recomp_generator_report.cpp Generator CLI
│       ├── asset_inspector.cpp Local-only ROM graphics inspection CLI
│       ├── w5_audio_native_reference.cpp Canonical-ROM native Format-A selector/decode hash reporter
│       ├── generate_gfx3820_resource_manifest.py Regenerates the pinned externalization manifest from accepted census/closure data
│       ├── fixed_count_file_manifest.py Generates the row-preserving fixed-count portability catalog from accepted enumeration plus canonical ROM
│       ├── m12_gfx_ancient.py Independent Ancient graphics parser/classifier
│       ├── m12_gfx_sweep.py Developer-only whole-ROM M12-GFX-1 campaign/report
│       ├── m12_gfx_caller_closure.py Developer-only M12-GFX-2 0x3820 caller/source closure and promoter
│       ├── re_m12_gfx_loader_census.py Developer-only exact 0x00D406 direct-xref census
│       ├── re_m12_gfx_37d2_census.py Developer-only exact 0x37D2 wrapper-family census
│       ├── re_m12_gfx_d3b2_census.py Developer-only exact 0xD3B2 indexed-loader census
│       ├── re_m12_gfx_36d4_census.py Developer-only exact 0x36D4 wrapper census
│       ├── re_m12_gfx_sibling_census.py Developer-only proven sibling-helper census
│       ├── re_m12_gfx_d406_residual_census.py Developer-only residual 0x00D406 census
│       ├── re_m12_gfx_runtime_provenance.py Developer-only runtime source-provenance validator
│       ├── re_resource_boundary_scan.cpp Developer-only pointer-table resource boundary scanner
│       ├── ghidra/OasisGhidraMap.java Developer-only Ghidra map exporter; never production-linked
│       ├── re_slice_decoder.cpp Developer-only bounded 68000 evidence decoder/reporter
│       ├── re_slice_decoder.hpp Decoder data types and report API
│       ├── re_slice_exact.cpp Bounded exact instruction normalization over decoder-owned operands
│       ├── re_assemble.hpp/.cpp Developer-only ASM emission and first-byte comparator
│       ├── re_assemble_format.cpp Raw-word and typed-operand JSON output
│       ├── re_assemble_report.cpp Canonical-ROM 25-slice emit/verify CLI and bounded split manifest
│       ├── re_assemble_run.py Local vasm orchestration, gap extraction and exact comparison
│       ├── re_full_split_run.py Full-ROM local-blob split, manifest metrics and exact comparison
│       ├── re_m12_auto_promote.py Combined exact-island/resource transactional promoter
│       ├── re_assemble_range_report.cpp Generic bounded decoder/ASM emitter for candidate slices
│       ├── re_auto_promote.py Evidence-ranked transactional UNKNOWN→ASM promotion runner
│       ├── re_auto_promote_helpers.py Promotion failure metadata, clustering and trend helpers
│       ├── re_dynamic_confirm.py Fail-closed bounded natural evidence confirmation
│       ├── re_structured_data.py Exact bounded ROM data structure classifier
│       ├── re_slice_format.cpp Deterministic JSON/human report formatting
│       ├── re_slice_report.cpp Local-only bounded ROM slice report CLI
│       ├── gpgx_coverage_report.cpp Developer-only NEW executed-PC bitmap decoder/report
│       ├── gpgx_import_gpgx_coverage.cpp Developer-only address-level GPGX trust evidence importer
│       ├── gpgx_import_io.cpp Structural JSON range/evidence import helpers
│       ├── gpgx_import_io.hpp GPGX importer model and provenance helper API
│       ├── gpgx_json.cpp Minimal dependency-free structural JSON parser
│       ├── gpgx_json.hpp Structural JSON value/parser declarations
│       ├── gpgx_unknown_priority.py Deterministic ranking and bounded-slice report for runtime-unknown PCs
│       ├── oasis_gpgx_rom_reader_correlation.py Deterministic analysis ROM-read to reader-PC correlation
│       ├── re_program.hpp Multi-function bounded RE aggregation types/API
│       ├── re_program.cpp Conservative function boundaries/call and memory bindings
│       ├── re_program_format.cpp Deterministic multi-slice JSON/human formatting
│       ├── re_program_report.cpp Local-only representative multi-slice CLI
│       ├── re_trace.hpp Bounded dynamic trace data model/API
│       ├── re_trace.cpp Isolated scenario interpreter and static/dynamic comparison
│       ├── re_trace_format.cpp Deterministic trace JSON/human formatting
│       ├── re_trace_report.cpp Local-only bounded dynamic trace CLI
│       ├── re_diff.hpp Retail/beta bounded correspondence types/API
│       ├── re_diff.cpp Conservative normalized signature and analogue matching
│       ├── re_diff_detail.cpp Bounded changed-block CFG/instruction detail
│       ├── re_diff_format.cpp Deterministic revision-diff JSON/human formatting
│       ├── re_diff_report.cpp Local-only retail/beta differential CLI
│       ├── re_atlas.hpp Typed bounded ROM Atlas model/query API
│       ├── re_atlas.cpp Atlas manifest, evidence aggregation and conflict detection
│       ├── re_atlas_format.cpp Deterministic Atlas JSON/human formatting
│       ├── re_atlas_report.cpp Local-only bounded Atlas CLI
│       ├── re_atlas_ranking.hpp Atlas unresolved-evidence ranking API
│       ├── re_atlas_ranking.cpp Deterministic ranking aggregation and priority groups
│       ├── re_atlas_ranking_report.cpp Local-only Atlas ranking CLI
│       ├── re_resolution.hpp Bounded address-displacement resolution API
│       ├── re_resolution.cpp Conservative register propagation and CFG merge handling
│       ├── re_resolution_format.cpp Deterministic resolution JSON/text formatting
│       ├── re_resolution_report.cpp Local-only bounded resolution CLI
│       ├── re_cfg_audit.hpp Bounded unreachable-CFG audit data model/API
│       ├── re_cfg_audit.cpp Audit classifications, islands and bounded CFG context
│       ├── re_cfg_audit_format.cpp Deterministic CFG-audit JSON/text formatting
│       ├── re_cfg_audit_report.cpp Local-only USA CFG-audit CLI
│       ├── re_reachable_closure.hpp Bounded reachable unresolved closure API
│       ├── re_reachable_closure.cpp Backward register provenance and reason audit
│       ├── re_reachable_closure_format.cpp Deterministic closure JSON/text formatting
│       ├── re_reachable_stack.hpp Narrow MOVEA postincrement stack API
│       ├── re_reachable_stack.cpp Bounded push/pop value provenance only
│       ├── re_reachable_closure_report.cpp Local-only USA closure CLI
│       ├── re_callee_effect.hpp Bounded direct-callee register/stack effect API
│       ├── re_callee_effect.cpp Conservative callee return-effect audit
│       ├── re_callee_effect_format.cpp Deterministic callee-effect JSON/text formatting
│       ├── re_callee_effect_report.cpp Local-only USA callee-effect CLI
│       ├── re_caller_stack.hpp Bounded caller pre-BSR stack provenance API
│       ├── re_caller_stack.cpp Symbolic A7 paths and conservative stack effects
│       ├── re_caller_stack_format.cpp Deterministic caller-stack JSON/text formatting
│       ├── re_caller_stack_report.cpp Local-only USA caller-stack CLI
│       ├── re_emulator_trace.hpp Neutral external emulator capture model/API
│       ├── re_emulator_trace.cpp Capture parser, normalization and Atlas comparison
│       ├── re_emulator_trace_format.cpp Deterministic emulator-trace JSON/text formatting
│       ├── re_emulator_trace_report.cpp External capture import CLI; no emulator backend
│       ├── re_scenario.hpp              Frozen natural emulator-scenario data/API
│       ├── re_scenario.cpp              Frozen natural emulator-scenario parser/JSON
│       ├── re_candidate_map.hpp         Ghidra/Atlas normalized candidate model/API
│       ├── re_candidate_map.cpp         Conservative evidence merge/classification/ranking
│       ├── re_candidate_map_parse.cpp   Strict parser for the external Ghidra JSON schema
│       ├── re_candidate_map_format.cpp  Deterministic candidate JSON/top-report formatting
│       ├── re_candidate_map_report.cpp  Local-only Ghidra-to-Atlas candidate CLI
│       ├── re_mass_verify.hpp            Batch structural-verification data/API
│       ├── re_mass_verify.cpp            Bounded decode, overlap, classification and clustering
│       ├── re_mass_verify_format.cpp     Deterministic mass JSON/text report formatting
│       ├── re_mass_verify_report.cpp     Local-only mass verification CLI
│       ├── re_explore.hpp                Recursive explorer model, states, edges and frontier API
│       ├── re_explore.cpp                Tiered deterministic guarded structural worklist
│       ├── re_explore_format.cpp         Deterministic explorer JSON/human formatting
│       ├── re_explore_report.cpp         Local-only bounded/ROM-wide explorer CLI
│       ├── re_explore_dynamic.cpp        Dynamic-edge validation for natural ant evidence
│       ├── re_ant.hpp                     Single-ant job/result/merge model and API
│       ├── re_ant.cpp                     Deterministic ant identity, parsing and merge rules
│       ├── re_ant_format.cpp              Deterministic ant JSON/text formatting
│       ├── re_ant_report.cpp              Local-only single-ant job/merge CLI
│       ├── re_ant_queue.hpp               Frozen sequential ant queue model and lifecycle API
│       ├── re_ant_queue.cpp                Deterministic queue selection, lifecycle and parsing
│       ├── re_ant_queue_format.cpp         Deterministic queue JSON/text formatting
│       ├── re_ant_queue_report.cpp         Local-only queue make/claim/finalize/merge CLI
│       ├── re_static_translation.hpp        Minimal PoC state/memory/comparison API
│       ├── re_static_translation.cpp        Developer-only bounded static C++ PoC
│       ├── re_static_translation_report.cpp Local ROM-backed PoC report CLI
│       ├── re_bizhawk_boot_trace.lua Developer-only BizHawk boot trace and bus-write probe
│       ├── re_bizhawk_natural_reach.lua Developer-only bounded natural-input target/caller/state probe
│       ├── re_bizhawk_m12_gfx_provenance.lua Developer-only targeted runtime source-register probe for unresolved 0x3820 callers
│       ├── re_bizhawk_m12_capabilities.lua Developer-only one-frame BizHawk observable/API capability probe
│       ├── re_bizhawk_m12_sat_provenance.lua Developer-only live VDP/SAT/DMA/source-RAM provenance probe
│       ├── re_bizhawk_m12_sprite_context.lua Developer-only bounded sprite-table caller/producer context probe
│       ├── re_bizhawk_m12_ali_transition.lua Developer-only one-shot F1 Ali transition provenance probe
│       ├── re_bizhawk_m12_targeted_reads.lua Developer-only exact-address ROM-read provenance probe
│       ├── re_bizhawk_m12_b730_calls.lua Developer-only bounded table-to-B730 register-context probe
│       ├── m12_sprite_context_catalog.py Developer-only payload-free table/live provenance catalog validator
│       ├── m12_static_sprite_dispatch_catalog.py Developer-only bounded static dispatch-table consumer catalog
│       ├── m12_sprite_reconstruction.py Developer-only SAT/tile/CRAM/piece reconstruction and provenance validator
│       ├── re_bizhawk_ant.lua            Developer-only one-frontier natural ant worker
│       ├── re_bizhawk_stack_provenance.lua Developer-only bounded runtime stack-value/writer probe
│       ├── re_bizhawk_natural_scenario.txt Frozen neutral-input reachability scenario
│       ├── re_bizhawk_m11_8_natural_scenario.txt M11.8 natural input/target/RAM scenario
│       ├── re_mame_boot_trace.cmd Developer-only MAME fixed-instruction debugger trace
│       ├── re_mame_writer_probe.cmd Developer-only MAME RAM writer watchpoint probe
│       └── re_mame_trace_normalize.ps1 Normalize MAME/BizHawk raw events to neutral trace
└── tests/
    ├── check_file_limits.cmake           Enforces <=500-line rule through CTest
    ├── check_core_boundary.cmake         Enforces core source/link dependency boundary
    ├── mechanical_primitive_test.cpp     Standalone oasis_core primitive semantics and continuation tests
    ├── table_copy_routine_test.cpp       Standalone portable routine oracle and yield/resume tests
    ├── table_copy_routine_contract_test.cpp Synthetic 0x2D66 CFG/entry/exit contract test
    ├── byte_grid_test.cpp                Synthetic world-grid/footprint tests
    ├── graphics_decompress_test.cpp      Synthetic decompressor behavior tests
    ├── m12_gfx_sweep_test.py             Independent Ancient parser safety/classifier tests
    ├── m12_sprite_reconstruction_test.py Synthetic hardware-sprite reconstruction and provenance tests
    ├── m12_sprite_context_catalog_test.py Ordered live selector/producer catalog validation tests
    ├── m12_static_sprite_dispatch_catalog_test.py Static M12 dispatch-table overlap and bounds tests
    ├── m12_gfx_caller_closure_test.py    M12-GFX-2 caller/source closure regression tests
    ├── re_m12_gfx_loader_census_test.py  Exact 0x00D406 direct-xref census regression tests
    ├── re_m12_gfx_37d2_census_test.py    Exact 0x37D2 wrapper-family census regression tests
    ├── re_m12_gfx_d3b2_census_test.py    Exact 0xD3B2 indexed-loader census regression tests
    ├── re_m12_gfx_36d4_census_test.py    Exact 0x36D4 wrapper census regression tests
    ├── re_m12_gfx_sibling_census_test.py Exact sibling-helper census regression tests
    ├── re_m12_gfx_d406_residual_census_test.py Exact residual 0x00D406 census regression tests
    ├── re_m12_gfx_runtime_provenance_test.py Static runtime-probe contract regression test
    ├── re_m12_gfx_runtime_provenance_validator_test.py Fail-closed runtime-source validator test
    ├── graphics_decompress_reference.cpp ROM-backed differential oracle verifier
    ├── resource_id3_reference.cpp Local USA-ROM resource/VRAM/atlas oracle verifier
    ├── gfx3820_file_override_reference.cpp Manual all-ID ROM fallback and exact-file override verifier
    ├── audio_format_a_test.cpp Synthetic Mode 0/1 accumulator and repeat-code decoder tests
    ├── w5_audio_native_differential.py Whole-catalog C++ vs W5 Python differential and evidence receipt
    ├── genesis_graphics_test.cpp         Synthetic tile/palette conversion tests
    ├── rom_identity_test.cpp             Synthetic ROM/hash/header tests
    ├── player_test.cpp                   Deterministic input and movement tests
    ├── spirit_slots_test.cpp             Deterministic spirit summoning, slot allocation, SP drain, follow movement, and transition persistence tests
    ├── environment_interaction_test.cpp  Deterministic Dytto Water Bubble -> Flame environment interaction and persistence tests
    ├── native_vertical_slice_test.cpp    M11.18 fixture, replay and framebuffer oracle
    ├── player_reference.cpp              Local USA-ROM oracle for player vectors
    ├── entity_pool_test.cpp              Synthetic raw entity-pool/active-record tests
    ├── entity_pool_reference.cpp         Local USA-ROM oracle for entity pool loops
    ├── event_router_test.cpp              Synthetic raw event producer/router tests
    ├── event_router_reference.cpp         Local USA-ROM oracle for event boundaries
    ├── re_slice_decoder_test.cpp           Synthetic bounded decoder/report tests
    ├── re_assemble_test.cpp                Exact operands, golden ASM, encoding widths and byte-difference tests
    ├── re_full_split_test.py               Synthetic deterministic full-split manifest/helper tests
    ├── re_auto_promote_test.py             Synthetic ranking/promotion/rollback helper tests
    ├── re_dynamic_confirm_test.py           Synthetic range-linkage/coverage/trust tests
    ├── re_structured_data_test.py            Synthetic conservative data classifier tests
    ├── re_slice_reference.cpp              Local USA-ROM oracle for the 0x60004 slice
    ├── re_program_test.cpp                 Synthetic multi-function RE aggregation tests
    ├── re_program_reference.cpp            Local USA-ROM oracle for representative RE targets
    ├── re_trace_test.cpp                   Synthetic dynamic trace/report tests
    ├── re_trace_reference.cpp              Local USA-ROM oracle for bounded dynamic scenario
    ├── re_diff_test.cpp                    Synthetic exact/structural/changed matching tests
    ├── re_diff_reference.cpp               Local retail/beta fingerprint/correspondence oracle
    ├── re_atlas_test.cpp                   Synthetic Atlas model/query/conflict tests
    ├── re_atlas_reference.cpp              Local USA/Beta Atlas oracle
    ├── re_atlas_ranking_test.cpp            Synthetic ranking/grouping tests
    ├── re_resolution_test.cpp               Synthetic bounded propagation/merge tests
    ├── re_resolution_reference.cpp          Local USA-ROM resolution oracle
    ├── re_cfg_audit_test.cpp                Synthetic CFG-audit classification/island tests
    ├── re_cfg_audit_reference.cpp           Local USA-ROM CFG-audit oracle
    ├── re_reachable_closure_test.cpp        Synthetic backward closure/stack/merge tests
    ├── re_reachable_closure_reference.cpp   Local USA-ROM closure oracle
    ├── re_callee_effect_test.cpp             Synthetic bounded callee-effect tests
    ├── re_callee_effect_reference.cpp        Local USA-ROM callee-effect oracle
    ├── re_caller_stack_test.cpp              Synthetic bounded caller-stack tests
    ├── re_caller_stack_reference.cpp         Local USA-ROM caller-stack oracle
    ├── re_emulator_trace_test.cpp            Synthetic external capture/import tests
    ├── re_scenario_test.cpp                  Synthetic scenario parser/serialization tests
    ├── re_candidate_map_test.cpp              Synthetic Ghidra/Atlas merge/ranking tests
    ├── re_mass_verify_test.cpp                Synthetic batch classification/clustering tests
    ├── re_explore_test.cpp                    Synthetic recursive explorer/control/frontier tests
    ├── re_ant_test.cpp                         Synthetic single-ant contract/merge tests
    ├── re_ant_queue_test.cpp                    Synthetic queue selection/lifecycle tests
    ├── re_static_translation_test.cpp            Synthetic static translation differential tests
    ├── gpgx_persistent_coverage_test.py           Synthetic persistent bitmap merge contract test
    ├── gpgx_rom_reader_correlation_test.py        Synthetic reader correlation/provenance tests
    ├── gpgx_bounded_classification_test.py        Bounded decode/runtime completeness tests
    ├── re_natural_reference.cpp              Local USA-ROM natural reachability oracle
    ├── runtime_test.cpp                  Deterministic frame/input sequence tests
    ├── screen_descriptor_test.cpp        Synthetic screen descriptor tests
    ├── terrain_collision_test.cpp        Synthetic terrain-gate tests
    ├── smoke.cpp                         Minimal build/runtime smoke test
    └── vdp_test.cpp                      VDP storage/bounds/attribute tests
```

`docs/reports/GPGX_RUNTIME_EXECUTION_TRUST.md` is the retained human-readable
report for the canonical manual-realtime evidence import. The bounded M11.21
region investigation is retained in
`docs/reports/RUNTIME_REGION_060BB6.md`; its M11.22 bounded classification is
retained in `docs/reports/RUNTIME_REGION_060BB6_CLASSIFICATION.md`.
The analysis ROM-read to reader-PC correlation report is retained in
`docs/reports/GPGX_ROM_READER_CORRELATION.md`.
The bounded M11.24 resource contract is retained in
`docs/reports/RESOURCE_CONTRACT_ID3.md`.

## M11.28 hybrid experiment files
M11.28 adds the developer-only `src/tools/hybrid/` directory:

- `CMakeLists.txt`: separate contract/tests and opt-in external GPGX frontend.
- `contract.hpp/.cpp`: 0x3820 bounded effect prediction/comparison and explicit
  unsupported override gate, reusing native/mechanical decompressors.
- `dispatch.hpp/.cpp`: natural entry/return capture, footprint checks and ISR
  isolation through the existing emulator hook.
- `candidate_2d66.hpp/.cpp`: the M11.29/M11.52 target adapter for the bounded
  `0x2D66` leaf, including exact GPGX instruction-continuation reconstruction.
- `runner.cpp`: deterministic neutral libretro frontend, local hash evidence.
- `grid_query_capture.cpp`: opt-in direct-register PRE/POST capture around the
  M68K `0x9C40` entry and its stack-derived RTS continuation.
- `grid_query_micro_oracle.cpp/.hpp`: independent MOVEQ/MOVE/RTS/sentinel test
  for HOOK_CPU timing, stack byte order, register writes, and read-value
  propagation.
- `gpgx_capture_bridge.c`: minimal HOOK_CPU register, memory-peek, cycle, and
  callback exports for that capture runner; excluded from production targets.
- `gpgx_bridge.c`: read-only bridge compiled only inside the external emulator.
- `prepare_gpgx.py`: reproducible bridge installation, coverage opt-out guard
  and M11.36 post-instruction comparison hook.
- `tests/hybrid_contract_test.cpp`, `tests/hybrid_dispatch_test.cpp`: independent
  synthetic state/output/stack, corruption, footprint and override-gate tests.
- `tests/hybrid_candidate_2d66_test.cpp`: synthetic exact MOVEM-stack shadow,
  instruction bridge and M11.51 refresh-blind-spot regression for the selected
  candidate.
- `docs/reports/HYBRID_NATIVE_MIGRATION_POC.md`: evidence and exact blockers.
- `docs/reports/HYBRID_NATIVE_OVERRIDE_MINIMAL_POC.md`: M11.29 target selection,
  identity and shadow/override equivalence evidence.
- `src/tools/hybrid/replacement.hpp/.cpp`: developer-only explicit routine registry and metrics.
- `src/tools/hybrid/candidate_604bc.hpp/.cpp`: M11.30 bounded RAM flag routine contract.
- `src/tools/hybrid/candidate_61032.hpp/.cpp`: M11.30 bounded table/RAM routine contract.
- `tests/hybrid_replacement_test.cpp`: synthetic registry routing regression.
- `docs/reports/HYBRID_NATIVE_OVERRIDE_BATCH_POC.md`: M11.30 shadow evidence and override blocker.
- `basic_block.hpp/.cpp`: M11.32 developer-only bounded block registry and
  generic M11.38 `BlockExit`/yield continuation glue; generated execution
  remains separate from this handwritten registry.
- `tests/hybrid_semantic_core_test.cpp`: M11.34 independent reference vectors,
  M11.35 newly required semantic vectors, exact decode/length checks and
  exact-IR assertions.
- `tests/hybrid_m1145_semantic_test.cpp`: M11.45 independent exact vectors for
  the bounded generated semantic helpers.
- `tests/hybrid_m1147_semantic_test.cpp`: M11.47 exact safe-memory vectors,
  overlap/address-wrap checks and generator provenance regression.
- `tests/hybrid_generated_provenance_test.cpp`: generated M11.33/M11.35 block
  extension-word consumption and direct-successor PC/provenance regression.
- `tests/hybrid_basic_block_test.cpp`: post-instruction boundary regression
  proving shadow closes before a scheduler/frame rebase.
- `tests/hybrid_generated_block_test.cpp`: generated semantics and
  instruction-boundary yield/resume regression for a frozen M11.38 range.
- `tests/hybrid_checkpoint_layout_test.cpp`: GPGX layout, representation-span,
  version-guard and cross-process canonicalization regression.
- `tests/hybrid_address_provenance_test.cpp`: address-class and observer
  aggregation regression for the developer-only runtime provenance path.
- `src/core/mechanical_primitive.*`: M11.50 portable primitive contract,
  executor and continuation state; contains no ROM candidate/opcode metadata.
- `src/core/table_copy_routine.*`: M11.51 tokenized structured table-copy
  routine contract and resumable executor; contains no ROM PC constants.
- `tests/table_copy_routine_test.cpp` and
  `tests/table_copy_routine_contract_test.cpp`: standalone semantic,
  continuation and synthetic CFG proof.
- `tests/hybrid_mechanical_primitive_test.cpp`: hybrid registry metadata and
  portable-contract conversion regression.
- `tests/check_core_boundary.cmake`: source scan for hybrid/GPGX/libretro and
  candidate-specific constants, paired with the CMake no-link assertion.
- `docs/reports/PORTABLE_MECHANICAL_PRIMITIVE_LAYER_M11_50.md`: exact before /
  after ownership, dependency audit and extraction proof.
- `docs/reports/BASIC_BLOCK_RECOMPILATION_TIMING_M11_32.md`: M11.32 identity,
  shadow, native override and state/video equivalence evidence.
- `docs/reports/DEMAND_DRIVEN_BLOCK_PROMOTION_M11_35.md`: bounded discovery,
  semantic/generation gate and first runtime shadow blocker; no promotion claim.
- `docs/reports/GPGX_TIMING_REFRESH_BRIDGE_M11_36.md`: counter ownership,
  frame epoch, bounded A–G boundary evidence and final three-candidate gate.
- `docs/reports/INTERRUPT_SAFE_MULTI_INSTRUCTION_BLOCKS_M11_38.md`: frozen
  candidate ledger, generic yield contract, per-boundary shadow and interrupted
  continuation evidence.
- `docs/reports/REMAINING_INTERPRETER_ATTRIBUTION_M11_40.md`: M11.40 baseline
  reproduction and checkpoint identity mismatch; no attribution or promotion.
- `docs/reports/CHECKPOINT_CANONICALIZATION_M11_43.md`: exact pinned GPGX
  wholesale-state layout, canonicalization contract and cross-process proof.
- `docs/reports/NATIVE_ROUTINE_MISMATCH_M11_52.md`: exact four-byte mismatch,
  temporal root cause, continuation repair and paired authoritative proof.

## M11.31 comparative method transfer
- `docs/reports/COMPARATIVE_DISASSEMBLY_METHOD_TRANSFER_M11_31.md`: pinned
  public Streets of Rage 2/3 project inventory, bounded similarity matrix,
  transferable workflow, Beyond Oasis dry-run and separated conclusions.

## Planned directories
Create these only when their milestone begins and evidence justifies the structure:

```text
src/game/entities/  Enemy/NPC/entity systems
src/game/scripts/   Event/script interpreter or translated semantics
src/audio/          Audio runtime/compatibility layer
src/platform/       Window/input/render/audio platform integration
```

## Rules
- Do not create a generic `utils` dumping ground. Name modules by responsibility.
- Do not create planned directories early merely to make the tree look complete.
- New top-level directories require an architecture decision.
- Keep executable/source/build-code files at or below 500 lines; prose/reference documentation is exempt from the numeric limit.
- When a source/build file approaches 400 lines, consider splitting before adding major functionality.
# M12-AUTO54 adds the PC-island table promoter

`src/tools/re_m12_pc_island_tables_promote.py` and
`tests/re_m12_pc_island_tables_test.py` record 15 exact PC-relative table and
text-literal contracts. Generated transaction output remains ignored/local.

# M12-AUTO53 adds the PC-relative lookup-family promoter

`src/tools/re_m12_pc_lookup_family_promote.py` and
`tests/re_m12_pc_lookup_family_test.py` record five fixed lookup-table
contracts. Generated transaction output remains ignored/local.
# M12-AUTO57 adds the bounded word-transform promoter

`src/tools/re_m12_bounded_word_transform_promote.py` materializes the exact
128-byte `0x03BD86` word table closed by routine `0x03B7C0` and its four
callers. `tests/re_m12_bounded_word_transform_test.py` checks the contract,
closed word count and manifest split; CTest registration is in
`cmake/m12_auto2.cmake`.
# M12-AUTO59 adds bounded word-copy provenance tooling

`src/tools/re_m12_bounded_word_copy_promote.py` materializes the two exact
word-copy tables closed by `0x002D66`; its regression test is
`tests/re_m12_bounded_word_copy_test.py`.

# M12-AUTO58 adds an exact static ASM island

`src/tools/re_m12_static_003260.asm` is the human-maintained exact source for
`0x003260..0x0032E8`; `re_m12_static_003260_roundtrip.asm` supplies isolated
external labels for vasm verification, and
`re_m12_static_003260_promote.py` performs the transaction. The regression test
is `tests/re_m12_static_003260_test.py`.
# M12-AUTO60 adds the contiguous static-island promoter

`src/tools/re_m12_contiguous_islands_promote.py` consumes the positive results
of the generated direct-caller-to-RTS audit in
`build/m12-auto60-island-audit/`; its regression test is
`tests/re_m12_contiguous_islands_test.py`. CTest registration is in
`cmake/m12_auto2.cmake`.

# M12-CARVER-1 adds Stage 1 evidence/gap orchestration

`src/tools/m12_carver.py` owns the deterministic half-open IntervalDB,
typed evidence, provenance graph, conflicts, fixed-point reconciliation, gap
analysis and campaign ranking. `src/tools/m12_carver_adapters.py` adapts only
the existing deterministic M12 evidence producer schemas. The command-line
entry point is `src/tools/re_m12_carver.py`; its generated DB/report stay in
ignored build output because they may reference an external user ROM and local
evidence. `tests/re_m12_carver_test.py` covers manifest coverage, ownership
preservation, conflict blocking, provenance cycles and deterministic output.

# M12-CARVER-2 adds graph-guided evidence expansion

`src/tools/m12_carver_expansion.py` and
`src/tools/re_m12_carver_expand.py` add a deterministic, analysis-only pass
over the confirmed 99-row field1 table and existing graphics census. It emits
typed pointer/resource graph records and non-owning raw-size hypotheses while
leaving existing promoters and the manifest unchanged. The expansion report
is documented in `docs/reports/THOR_ROM_CARVER_M12_STAGE2.md`; regression
coverage remains in `tests/re_m12_carver_test.py`.
Expansion helper contracts are covered by
`tests/re_m12_carver_expansion_test.py`.

# M12-CARVER-3 adds global runtime-evidence sweep orchestration

`src/tools/m12_carver_global_sweep.py` and
`src/tools/re_m12_carver_global_sweep.py` discover canonical-ROM-compatible
runtime, checkpoint, census and existing M12 promotion artifacts under an
input root, normalize global ROM spans, and emit a whole-ROM blocker census
without changing manifest ownership. `tests/re_m12_carver_global_sweep_test.py`
covers deterministic span merging and exhaustive unobserved-gap classification.

# M12-CARVER-4 adds global static consumer recovery

`src/tools/m12_carver_static_recovery.py` and
`src/tools/re_m12_carver_static_recovery.py` consume existing static reverse-
engineering artifacts globally. They emit typed ROM-reference census edges,
parser-contract observations, consumer families, container candidates and a
complete blocker map without writing manifest ownership or repeating runtime
capture. Regression coverage is in
`tests/re_m12_carver_static_recovery_test.py`; the result is documented in
`docs/reports/THOR_ROM_CARVER_M12_STATIC_CONSUMER_RECOVERY.md`.

# M12-CARVER-5 adds whole-ROM format/container reconstruction

`src/tools/m12_carver_format_reconstruction.py` and
`src/tools/re_m12_carver_format_reconstruction.py` fingerprint every UNKNOWN
range, solve bounded structural candidates, scan the existing BO grammar, and
attach non-owning typed evidence to the Carver IntervalDB. The regression test
is `tests/re_m12_carver_format_reconstruction_test.py`. Machine-readable
outputs remain ignored under `build/m12-carver-m12c5-format-reconstruction-c/`;
the result is documented in
`docs/reports/THOR_ROM_CARVER_M12_FORMAT_CONTAINER_RECONSTRUCTION.md`.

`src/tools/m12_selector_descriptor_grammar.py` emits the payload-free M12
selector/descriptor/dispatch/child-table grammar catalog and validates the
canonical ROM contracts without changing ownership. Its regression coverage is
`tests/m12_selector_descriptor_grammar_test.py`; CTest registration is in
`cmake/m12_auto2.cmake`. The generated JSON under `build/m12-gfx-runtime/` is
local ignored evidence and is not repository data.

# M12-AUTO62 adds bounded live-discovery comparison

`src/tools/thor_evidence/live_discovery_plan.py` creates a ROM-bound watch plan
from the current ownership manifest, Evidence Engine SQLite sidecars and prior
GPGX execution evidence. The developer-only Lua collector is
`src/tools/thor_evidence/capture/live_discovery.lua`; its launcher is
`src/tools/thor_evidence/capture/run_live_discovery.ps1`. The analyzer
`src/tools/thor_evidence/live_discovery_analyze.py` persists novelty classes,
deduplicated edges and fail-closed investigation records without changing
ownership. Regression coverage is in
`tests/thor_evidence_live_discovery_test.py`; the campaign result is documented
in `docs/reports/THOR_M12_AUTO62_LIVE_DISCOVERY.md`. Raw captures and reports
remain ignored under `build/thor-evidence/live-discovery/`.

# M12-AUTO63 adds focused register-slice investigation

`src/tools/thor_evidence/live_discovery_followup.py` ranks persisted AUTO62
novelties and emits one narrow static/request plan. The developer-only
`src/tools/thor_evidence/capture/focused_register_slice.lua` and
`run_focused_register_slice.ps1` capture only the selected register dependency
chain and target ROM reads. `src/tools/thor_evidence/live_discovery_followup_analyze.py`
requires source/consumer ordering, active-count and exact stride evidence; it
fails closed when any frontier is missing. `src/tools/re_m12_auto63_promote.py`
reuses the existing record-stream materializer and cannot run on a blocked
report. Regression coverage is in `tests/thor_evidence_followup_test.py` and
the result is documented in `docs/reports/THOR_M12_AUTO63_INVESTIGATION.md`.
Generated capture, static and promotion outputs remain ignored under
`build/thor-evidence/auto63-followup/`.

`src/tools/re_m12_child_tables_promote.py` applies the exact table promotion
contract to the finite selector child-table streams at `0x03B95E..0x03BA46`,
preserving the two-byte descriptor alias prefix. Its regression test is
`tests/re_m12_child_tables_promote_test.py`; generated transaction output
remains ignored/local.

`src/tools/m12_selector_control_analysis.py` proves the reset/main-dispatch
source of the `0x03A748` selector handler. `src/tools/m12_relative_table_analysis.py`
proves the bounded `0x03BDA6` relative-pair consumer grammar and fails closed
at its unresolved extent edge. Regression coverage is in
`tests/m12_selector_control_analysis_test.py` and
`tests/m12_relative_table_analysis_test.py`; local JSON outputs remain ignored
ROM-derived evidence. The combined result is documented in
`docs/reports/THOR_M12_SELECTOR_RELATIVE_CLOSURE.md`.

`src/tools/m12_b092_tail_dispatch.py` validates the finite second-level
dispatch rooted at `0x03B092`/`0x03B0A8`, its four-entry table at `0x03B0AA`,
all four RTS-closed targets, static I/O, and interpreter negative evidence.
Its regression test is `tests/m12_b092_tail_dispatch_test.py`; CTest
registration is in `cmake/m12_auto2.cmake`. Local JSON output is ignored
ROM-derived evidence. The result is documented in
`docs/reports/THOR_M12_SELECTOR_RELATIVE_CLOSURE.md`.

`src/tools/m12_indirect_body_dispatch.py` validates the two exact
selector-masked indirect body calls at `0x03AA28` and `0x03AAA8`, enumerates
their 15 unique targets, records static target I/O/call provenance, and
closes 14 RTS routine boundaries while classifying `0x03BA46` as a data alias.
Its regression test is `tests/m12_indirect_body_dispatch_test.py`; the CTest
registration is in `cmake/m12_auto2.cmake`. Local JSON output is ignored
ROM-derived evidence. The bounded result is documented in
`docs/reports/THOR_M12_SELECTOR_RELATIVE_CLOSURE.md`.

`src/tools/m12_a372_shadow_sat_producer.py` validates the bounded static
`0xA342..0xA438` producer, its two 9x8 root tables, D2/A5 dataflow, and the
`0xFF1858` root selector without emitting payload. Its regression test is
`tests/m12_a372_shadow_sat_producer_test.py`; CTest registration is in
`cmake/m12_auto2.cmake`. The result is documented in
`docs/reports/THOR_M12_A372_SHADOW_SAT_PRODUCER.md`.

`src/tools/m12_a372_caller_join.py` validates the static scheduler-to-A372
caller join, the `FF1858` writer context, and the bounded negative neighbor
result without emitting payload. Its regression test is
`tests/m12_a372_caller_join_test.py`; CTest registration is in
`cmake/m12_auto2.cmake`. The result is documented in
`docs/reports/THOR_M12_A372_CALLER_JOIN.md`.

`src/tools/re_bizhawk_m12_a372_runtime.lua` is a developer-only bounded
BizHawk register/root probe for the `A196 -> A342 -> A372` path. Its
payload-free validator is `src/tools/m12_a372_runtime_report.py`, tested by
`tests/m12_a372_runtime_report_test.py`; CTest registration is in
`cmake/m12_auto2.cmake`. The bounded negative result is documented in
`docs/reports/THOR_M12_A372_RUNTIME_CAPTURE.md`.

The independent non-UI BizHawk launcher and Lua baseline live in the ignored
`build/bizhawk-controlled-harness` directory. Its verified policy and exact
CLI/Lua sequence are documented in
`docs/reports/BIZHAWK_CONTROLLED_HARNESS_DIAGNOSIS.md`; the controlled
Right-versus-neutral register/root result and M12 fork ranking are documented
in `docs/reports/THOR_M12_CONTROLLED_RUNTIME_ROOT_CAPTURE.md`.

`src/tools/m12_shadow_sat_access_graph.py` is a payload-free direct-access
census for `FF1858`, `FF188A` and `FF188C`; its regression test is
`tests/m12_shadow_sat_access_graph_test.py`. The two-pass closure and writer
accounting are documented in
`docs/reports/THOR_M12_SHADOW_SAT_ACCESS_CLOSURE.md`.
# THOR Evidence Engine V1 FF13CC canary

`src/tools/thor_evidence/canary_engine.py` derives and validates the bounded
engine-derived provenance certificate from checked static JSON and sealed
dynamic events. `src/tools/thor_evidence/capture/canary.lua` and
`run_canary.ps1` are developer-only capture helpers. The existing
`schema.sql`/`store.py` sidecar now has separate operation-instance and
provenance-dependency tables. Regression and adversarial coverage is in
`tests/thor_evidence_v1_canary_test.py` and
`tests/fixtures/thor_evidence_v1_canary/negative_cases.json`; CTest registration
is in `cmake/m12_auto2.cmake`. The bounded result is documented in
`docs/reports/THOR_EVIDENCE_ENGINE_V1_FF13CC_CANARY.md` and governed by
ADR-0048.
# THOR Evidence Engine V2 RAM provenance

`src/tools/thor_evidence/ram_versions.py` is the reusable byte-version,
overlap, epoch and coverage-aware last-writer primitive. The existing
`schema.sql`/`store.py` sidecar persists RAM versions, write operations,
operation outputs and coverage certificates. `tests/thor_evidence_v2_ram_test.py`
and `tests/fixtures/thor_evidence_v2_ram/negative_cases.json` cover the
fail-closed V2 contract; CTest registration is in `cmake/m12_auto2.cmake`.
The bounded result is documented in
`docs/reports/THOR_EVIDENCE_ENGINE_V2_RAM_PROVENANCE.md` and governed by
ADR-0049.
`docs/reports/THOR_M12_AUTO67_6_REGISTER_PROVENANCE_GATE.md` and its JSON
counterpart record the negative register-version evidence gate: the current
O67V v2 capsules lack register values and bounded predecessor execution needed
to prove an A4/A5 reaching definition. No runtime code change was made.

`docs/reports/THOR_M12_AUTO67_6R3_LOW_OVERHEAD_PREHISTORY.md` and its JSON
counterpart record the negative low-overhead source audit: BizHawk TraceLogger
has no bounded machine-readable stream, the global Lua hook fails the real
performance gate, and the existing external GPGX block hook is not connected
to authoritative BizHawk QuickSave1 execution.

`src/tools/thor_evidence/capture/predecessor_burst.lua` is the developer-only
AUTO67.6R3B targeted producer-triggered, 64-record execution burst source;
`predecessor_disabled.lua` is its no-hook matrix control. The existing
`live_capsule.lua` selector, `live_capsule_status.lua` bounded status output,
and `auto67_runner.py` launcher expose the mode without changing the
authoritative runtime architecture. Regression coverage is in
`tests/thor_evidence_auto67_6r3b_test.py`; the real gate is documented in
`docs/reports/THOR_M12_AUTO67_6R3B_TARGETED_BURST_FEASIBILITY.md` and JSON.

`auto67_predecessor.py` now also exposes the reusable static
`register_writer_candidate_report()` used to generate generic A4/A5 writer
targets. `tests/thor_evidence_auto67_6r3c_test.py` covers its candidate and
runtime-configuration gate. The negative generic result is documented in
`docs/reports/THOR_M12_AUTO67_6R3C_GENERIC_REGISTER_WRITER_TARGETING.md` and
JSON.

`src/tools/thor_evidence/cartographer.py` is the MAP-1 persistent generic
provenance graph; `map_sources.py` imports bounded existing JSON evidence and
`m12_map1.py` runs the deterministic proof gate. Focused regression coverage is
`tests/thor_evidence_map1_test.py`. The authoritative checkpoint report is
`docs/reports/THOR_M12_MAP_1_GLOBAL_PROVENANCE_CARTOGRAPHER.md` with its JSON
counterpart; SQLite proof artifacts remain under ignored `build/`.

`src/tools/thor_evidence/capture/walker_two_window.lua` is the developer-only
WALKER-1 transport experiment: one bounded W1 window, a guarded global-hook-off
straight-line body, and one bounded W2 window. `src/tools/thor_evidence/walker1.py`
validates runtime identity, fail-closed joins, and stable Cartographer replay;
`tests/thor_evidence_walker1_test.py` covers the focused gate. The result is
documented in `docs/reports/THOR_M12_WALKER_1_TWO_WINDOW_SPARSE_ADVANCEMENT.md`
and its JSON counterpart; runtime SQLite and emulator artifacts remain under
ignored `build/`.

`src/tools/thor_evidence/auto67_live.py` owns occurrence-only Dispatcher
leasing: branch fingerprints are diagnostic, exact window items use
`dispatch_state`, and runtime occurrence identity is carried into worker
investigations and leases. `src/tools/thor_evidence/capture/live_opportunistic.lua`
publishes the source `epoch + seq` identity; `auto67_persistence.py` retains it
in worker provenance. Regression coverage is in
`tests/thor_evidence_dispatcher1_test.py` plus the updated
`tests/thor_evidence_auto67_test.py`. The checkpoint is documented in
`docs/reports/THOR_M12_DISPATCHER_1_OCCURRENCE_ONLY.md` and its JSON
counterpart.

`src/tools/thor_evidence/auto67_snapshot_admission.py` is a developer-only
1:1 snapshot admission probe. It freezes immutable occurrence-scoped snapshot
metadata into a fixed 16-slot pool, observes the existing Dispatcher/Worker
flow, and exposes occupancy, ownership and fail-visible pool-full metrics
without switching production AUTO67 or adding a queue. Its contract is tested
by `tests/thor_evidence_auto67_snapshot_admission_test.py` and documented in
`docs/reports/THOR_M12_AUTO67_NATIVE_RING_BRIDGE_1A_ADMISSION_1X.md`.

`src/tools/thor_evidence/auto67_native_snapshot.py` freezes and validates
actual experimental GPGX native-ring records at the existing occurrence
boundary, leases the exact immutable tuple through the fixed snapshot pool,
adapts it to the existing predecessor resolver input, and releases its slot
after the matching Worker returns. `capture/native_trace_snapshot.lua` invokes
the test-only BizHawk API synchronously; `tools/bizhawk-native-ring/bizhawk-native-snapshot-freeze.patch`
adds that API to the isolated managed host build. The experiment is opt-in in
`auto67_runner.py` and does not change production scheduling. Its native record,
identity, resolver and slot-lifecycle checks are in
`tests/thor_evidence_auto67_native_snapshot_test.py`; the runtime receipt is in
`docs/reports/THOR_M12_AUTO67_BIZHAWK_NATIVE_RING_BRIDGE_1B_FREEZE_WORKER_1.md`.

`src/tools/thor_evidence/auto67_worker_input_trace.py` provides a bounded,
opt-in equality trace for native-snapshot `BUS_WRITE_PC` events at Dispatcher
mailbox write, Worker mailbox read, and immediately before materialization. It
hashes the complete normalized event and recomputes the immutable frozen-record
SHA-256 at every point. The 1C regression in
`tests/thor_evidence_auto67_native_snapshot_test.py` exercises the real
Dispatcher/Worker mailbox path and reports per-field equality. The runtime
receipt is in
`docs/reports/THOR_M12_AUTO67_BIZHAWK_NATIVE_RING_BRIDGE_1C_WORKER_INPUT_TRACE.md`.

`tools/bizhawk-native-ring/live_forward_trace.[ch]` records one shared native
execution stream; its W1 sideband API preserves the fixed record ABI for bus
and frame events. The isolated GPGX checkout adds
`core/debug/live_forward_bus.c` for memory-map domain resolution and hooks
top-level 68K data accesses in `core/m68k/m68kcpu.h`; `cinterface.c` marks
authoritative frame advancement. `tests/live_forward_sideband_test.cpp`
covers ordered native event encoding. `live_forward_pool.c` owns checked dynamic pool allocation,
identity tables and memory plans; `live_forward_metrics.c` exports aggregate
and per-slot transition counters; `live_forward_worker.c` owns the Worker lifecycle,
bounded memory/retention termination, exact-range sealing, and immutable ACK
contract; `live_forward_flow.c` classifies M68K control flow. `live_forward.cmake`
builds developer-only native tests, including a 100-cycle-per-slot regression;
`live_forward_scaling.lua` and `live_forward_scaling_runtime.py` run progressive
scaling rounds and require two identical binary audits, per-slot transition
counters and exact ACK before slot reuse. `live_forward_scaling_stream.py`
tails the replaceable status stream without retaining cycle history, while
`live_forward_scaling_campaign.py` owns progressive count selection and its
CLI. `live_forward_scaling_audit.py`
checks IDs, stream sequence, depth, bounds and immutable copies.
`live_forward_runtime.lua` and `live_forward_runtime.py` run and verify the
single-Worker 1A proof. The
exact BizHawk/GPGX source patches and final runtime/boundary evidence are in
`tools/bizhawk-native-ring/*live-forward-worker-1a.patch` and
`docs/reports/THOR_M12_AUTO67_LIVE_FORWARD_WORKER_1A.md` plus JSON. Incremental
BizHawk and GPGX patches are in
`tools/bizhawk-native-ring/*live-forward-worker-1b.patch`; the 1B runtime gate
and runtime stop are recorded in
`docs/reports/THOR_M12_AUTO67_LIVE_FORWARD_WORKER_1B_SCALING.md` and its JSON
receipt. The BizHawk runtime campaign proves natural counts 1-64 and forced
counts 2-128 with 100 cycles and N*100 audited segments at each PASS count;
natural 128 and forced 256 stop before immutable reread when CPU execution
stalls. Per-count logs and audit JSONL files remain under the ignored
`build/thor-evidence/live-forward-worker-1b/` directory. This path does not
alter the live AUTO67 runner or Cartographer.

W1 native bus-event acceptance extends that developer-only path with the
canonical `*native-bus-event-sideband-v1.patch` pair and the
`*native-bus-event-sideband-v1-observability.patch` pair. The isolated GPGX
source owns bus hooks, map-domain resolution, event schema and frame emission;
the managed patch exposes only the frame-boundary witness API. The
`live_forward_sideband_probe.lua` and `run_sideband_probe.py` files run the
real micro probe; `run_counted_w1_acceptance.py` owns the exact final 128-Worker
record-count receipt. Exact source, build, 128-Worker, control-flow A/B,
witness and measured-cost evidence is recorded in
`docs/reports/THOR_M12_NATIVE_BUS_EVENT_SIDEBAND_V1.md`; raw receipts remain
under the ignored `build/m12-w1-acceptance/` tree.

W2 active-resource classification is worker-side only. `w2_resource_classification.py`
preserves copied W1 event fields, classifies only mapped primitive bus facts,
groups events by exact instruction sequence, emits bounded deterministic
candidates and conservative shadow-SAT/Z80 reports. `w2_vdp_decoder.py`
decodes observed VDP register/control writes and exact CPU DMA programming
fields without claiming internal DMA bytes. `run_w2_active_resource_classification.py`
reuses the audited scaling callback for a fresh short real-runtime receipt;
`tests/w2_active_resource_classification_test.py` covers the A–Q W2 contract.
The report is `docs/reports/THOR_M12_WORKER_ACTIVE_RESOURCE_CLASSIFICATION_V1.md`
with ignored raw output under `build/m12-w2-acceptance/`. This path does not
modify W1 capture, Worker scheduling, S1–S8 artifacts, or `SOURCE_OWNED`.

W2.1 frame-coherent evidence adds only segment-level `entry_frame` and
`exit_frame` snapshots to the accepted GPGX result metadata. The incremental
source-of-truth patches are
`tools/bizhawk-native-ring/*worker-frame-coherent-v1.patch`; the authoritative
counter remains W1's `frame_number`. `w2_frame_coherence.py` partitions only
when contained FRAME_BOUNDARY markers prove the transition, otherwise it emits
`MULTI_FRAME_UNRESOLVED`. `run_w2_frame_coherent_evidence.py` records exact
`(run_id, epoch, frame)` identity in W2 facts and keeps S1–S8 bridging
read-only. Its focused tests are in
`tests/w2_frame_coherent_evidence_test.py`; the acceptance report is
`docs/reports/THOR_M12_WORKER_FRAME_COHERENT_EVIDENCE_V1.md` with ignored raw
output under `build/m12-w2-1-acceptance/`. This adds no graphics semantics or
Z80 implementation and keeps `SOURCE_OWNED` unchanged.

# M12 AUTO67 Worker-clean boundary (2026-09-15)

`src/tools/thor_evidence/auto67_live.py` owns occurrence-only leasing and a
bounded factual Worker history (`recent_investigations`, 16 entries). It has no
unbounded investigation map or Worker novelty classifier. `auto67_capsule.py`
owns temporary 16-slot evidence resources and id-based release without semantic
flags. `auto67_materializer.py` keeps static causal facts and bounded
register/capture diagnostics; live Worker calls omit prescriptive frontiers.
`auto67_persistence.py` remains a downstream optional writer and preserves
occurrence provenance. `tests/thor_evidence_auto67_worker_clean_test.py`
covers the A-G Worker-clean and Dispatcher regression contract. The published
checkpoint is `docs/reports/THOR_M12_AUTO67_WORKER_CLEAN_1.md` plus JSON.
# M12 AUTO67 current-event ring boundary (2026-09-15)

`src/tools/thor_evidence/auto67_live.py::RollingWindow` is the bounded Python
current-event window for Dispatcher leasing. It owns only `capacity`,
`items` and `overwrites`; the exact item still carries `dispatch_state=LEASED`.
`src/tools/thor_evidence/capture/live_opportunistic.lua` remains the separate
bounded transport ring across the status poll boundary and preserves event
order and occurrence identity. The two rings are not merged and neither is a
backlog. `tests/thor_evidence_auto67_ring_clean_test.py` is the deterministic
Python/Lua-contract fixture for this checkpoint.

`src/tools/thor_evidence/auto67_transport.py` owns the bounded pre-dispatch
status cursor. It deduplicates replaceable Lua snapshots by epoch/sequence/
occurrence identity and passes current records directly to the Dispatcher;
`tests/thor_evidence_auto67_predispatch_transport_clean_test.py` covers epoch
rollover, duplicate/stale rejection, legacy key compatibility, and the
no-backlog boundary.

R1 keeps final-snapshot consumption on the same cursor: the launcher consumes
`lua_final` after the emulator exits, ingests those records, then stops the
Dispatcher. The live-opportunistic runner has no state option or
`OASIS_LIVE_STATE` export; savestate-aware capture scripts are separate.

# M12 AUTO67 minimal mailbox boundary (2026-09-15)

`auto67_live.py::Dispatcher` sends one detached event message per worker. The
message carries one occurrence-derived `investigation_id` and one `lease_id`
through CapsulePool and persistence; branch/context, seed, duplicate
occurrence, worker and capsule-lease mailbox fields are removed. Worker
history remains bounded and factual. The contract is covered by
`tests/thor_evidence_auto67_mailbox_clean_test.py` and the checkpoint report
`docs/reports/THOR_M12_AUTO67_MAILBOX_CLEAN_1.md` plus JSON.

# M12 AUTO67 Cartographer seam (2026-09-15)

`auto67_cartographer.py` converts Worker local-chain results into a stable
MAP-1 bundle only for the predecessor-produced
`REGISTER_REACHING_DEFINITION` proof contract. The existing Cartographer owns
PROVEN, duplicate, conflict and component decisions. `LivePersistenceSink`
feeds the adapter through its single bounded queue and retains the temporary
`live_chain` compatibility write. `--map-db` supports map-only mode; combining
it with `--chain-db` supports compatibility mode. Regression coverage is in
the writer-thread ownership, all-step canonicalization and drop metrics in
`tests/thor_evidence_auto67_cartographer_seam_test.py` and the report
`docs/reports/THOR_M12_AUTO67_CARTOGRAPHER_SEAM_1.md` plus JSON.
The follow-up removes live-chain output and is recorded in
`docs/reports/THOR_M12_AUTO67_LEGACY_CHAIN_CLEAN_1.md` plus JSON.

# M12 Archivist → Canonical Knowledge Pipeline 2G

`src/tools/thor_evidence/live_forward_archivist.py` emits a compact
content-addressed merge receipt. `rom_knowledge_live_import.py` validates the
MAP-1/Archivist chain and relation vocabulary; `rom_knowledge_live_delta.py`
translates one accepted session into stable 2D objects, claims, relations and
evidence. `rom_knowledge_pipeline.py` builds an ignored versioned pair of
master/knowledge SQLite databases, independently audits it, then atomically
selects the pair through `current.json`. `rom_knowledge_pipeline_audit.py`
recomputes graph and canonical evidence without using importer calculations.
The A–O contract plus unsupported-fact stop are in
`tests/rom_knowledge_pipeline_test.py`, registered in
`cmake/m12_auto2.cmake`; the canonical result is in
`docs/reports/THOR_M12_ARCHIVIST_KNOWLEDGE_PIPELINE_2G.md` and its compact JSON
receipt. Runtime databases and raw evidence remain ignored under `build/`.

# M12 Live Worker Control Window 2H

`tools/bizhawk-native-ring/live_forward_worker_control_model.py` validates
unbounded positive next-run values, shares the launcher's resource-budget
arithmetic, and decodes bounded Worker/status records. The host-only
`live_forward_worker_control.py` publishes a replaceable 4 Hz snapshot and
measured evidence growth; `live_forward_worker_control_window.py` renders it
in a separate Tk process and writes only next-run configuration. The Lua
status read uses the native read-only `oasis_lf_worker_status_get` API declared
in `live_forward_trace.h` and implemented in `live_forward_metrics.c`.
Regression coverage is in `tests/live_worker_control_test.py` and the native
status assertions in `tests/live_forward_scaling_test.cpp`. Incremental
BizHawk/GPGX integration patches are
`tools/bizhawk-native-ring/bizhawk-2.11.1-live-worker-control-2h.patch` and
`tools/bizhawk-native-ring/genesis-plus-gx-live-forward-worker-control-2h.patch`.
The runtime proof and compact receipt are
`docs/reports/THOR_M12_LIVE_WORKER_CONTROL_WINDOW_2H.md` and its JSON
counterpart. Runtime/session databases and segment evidence remain ignored
under `build/thor-evidence/`. The optional desktop `--until-closed` mode keeps
the same Worker/1B lifecycle and bounded 2H window active until EmuHawk exits;
`live_forward_segment_spool.py` appends individually host-audited FLOW_V1 bytes
and segment indexes with constant-memory bookkeeping. It stops cleanly at a
1 GiB free-disk reserve, labeled separately from RAM in the status window, and
does not run Cartographer/Archivist or claim a bounded campaign PASS. The
normal bounded 2B proof path remains unchanged.
`live_forward_scaling_stream.py` tails the append-only Lua status stream and
preserves SEG, LIFECYCLE, and round-ACK ordering during host validation so a
single read spanning adjacent rounds cannot validate an older lifecycle receipt
against already-advanced counters.
The 2H Worker rows show per-Worker start, completion, ACK, host-audited capture,
and latest/cumulative FLOW_V1 record counts; the host publisher copies these
summaries only for the currently visible status page.
Its operating limits and verification record are in
`docs/reports/THOR_M12_LIVE_WORKER_INTERACTIVE_MODE.md`.
# M12 2I post-run orchestration

The developer-only live-forward tools now include `live_forward_rolling_master.py`
for compact, hash-bound FLOW_V1 rolling generations and
`live_forward_postrun_window.py` for the post-run stage/result window. The
existing Worker Control window publishes a single idempotent `END GAME`
request; Lua consumes it only between complete rounds and closes EmuHawk
through the supported exit path.

`live_forward_progress.py` owns the atomic post-run heartbeat/status contract;
`live_forward_postrun_window.py` renders determinate or indeterminate stage
progress, heartbeat age, partial/full terminal state, and a separate DETAILS
view. `live_forward_rolling_master.py` publishes snapshots while auditing and
compacting the sealed FLOW run.
# M12 end-game post-run stages 5–9 (2I.2)

`tools/bizhawk-native-ring/live_forward_complete_pipeline.py` is the single
post-run coordinator after rolling-master compaction. It builds a closed
session only from sealed ordered FLOW, performs an atomic canonical generation
refresh when accepted inputs exist, checks ordered control provenance, and
stops before unsupported reconstruction or destructive cleanup. It reuses the
2I.1 `live_forward_progress.py` status contract. Regression coverage is in
`tests/live_forward_complete_pipeline_test.py`.
# M12 2I.2a terminal post-run state

`tools/bizhawk-native-ring/live_forward_progress.py` owns the single terminal
finalizer and status invariants. It distinguishes RUNNING/UNRESPONSIVE from
terminal COMPLETE, PARTIAL_COMPLETE, STOPPED and FAILED states, and records
BLOCKED stages after a mandatory STOP/ERROR. The post-run window maps terminal
states to backend-complete or backend-stopped text without a false heartbeat
warning. Deterministic coverage is in
`tests/live_forward_postrun_progress_test.py`.
### M12 Stage 5 refresh additions

`tools/bizhawk-native-ring/live_forward_complete_pipeline.py` owns the
run-scoped compact FLOW session, exact ROM-range linkage, canonical ownership
audit, and atomic post-run generation publication. The Stage 5 regression
coverage is in `tests/live_forward_stage5_test.py`.
* `src/tools/thor_evidence/map_driven_asm_closure.py` — pure 2F/2I.3 closure
  gates and deterministic ownership partition rules.
* `src/tools/thor_evidence/map_driven_asm_stage7.py` — current canonical
  generation Stage 7 coordinator; Stage 8 is invoked by the post-run
  coordinator after the atomic Stage 7 terminal receipt.
* `src/tools/thor_evidence/stage7_subprocess.py` — bounded Stage 7/8 external
  tool runner with heartbeat-safe output tails and separate fail-closed
  Stage 7/Stage 8 timeout codes.
* `tests/map_driven_executed_asm_closure_test.py` — historical synthetic 2F
  proof gates; `tests/map_driven_stage7_test.py` — Stage 7 artifact split
  regression; `tests/stage7_subprocess_test.py` — event-driven wait, pipe
  draining, timeout, crash and kill-fallback regressions.

* `tools/bizhawk-native-ring/live_forward_control_provenance_stage.py` — receipt-bound Stage 6 ordered-FLOW scan and structured lifecycle result.
* `tests/live_forward_control_provenance_stage_test.py` — Stage 6 NO_DELTA and fail-closed consumer contract tests.

* `src/tools/thor_evidence/map_driven_stage7_preflight.py` — lineage-bound
  Stage 7 canonical map versus emission-manifest authority resolution and
  diagnostic receipt generation.
* `tests/map_driven_stage7_preflight_test.py` — matching and fail-closed
  emission-partition preflight regressions.
* `tests/live_stage7_stage8_stage9_contract_test.py` — structural handoff
  contract ensuring Stage 7 returns before Stage 8 and Stage 9 cleanup.

* `tools/bizhawk-native-ring/live_forward_control_provenance_stage.py` —
  Stage 6 diagnostic receipt, integrity-stop classification, and unresolved
  consumer NO_DELTA contract.
* `tools/bizhawk-native-ring/live_forward_postrun_window.py` — exact STOP/ERROR
  code rendering in the current-stage and stage-list UI.
* `tests/live_forward_control_provenance_stage_test.py` — unresolved-consumer
  NO_DELTA and FLOW-corruption STOP regressions.

# M12 Stage 9 absorbed raw permanent reclaim 2I.5

`tools/bizhawk-native-ring/live_forward_absorption_cleanup.py` owns the
fail-closed absorption proof, semantic-equivalence audit, atomic delete
manifest/receipt publication, bounded WinError 32 retry and direct permanent
deletion of only run-scoped redundant artifacts. The accepted rolling master,
canonical map, Stage 5–8 receipts, final report/status and ROM remain protected.
`tests/live_forward_absorption_cleanup_test.py` covers lineage/stage gates,
semantic failure, permanent deletion, lock retry/stop, replay idempotence,
protected paths and post-delete integrity. Cleanup UI details are rendered by
`tools/bizhawk-native-ring/live_forward_postrun_window.py`.

## M12 RAM migration R1 shadow representation

`tools/bizhawk-native-ring/master_v2_shadow.py` is a read-only shadow encoder
for the accepted rolling master and canonical map. It produces a deterministic
self-contained container with streamed sections for rolling facts, canonical
map, canonical knowledge/ownership/emission and normalized Stage 5–9 outcomes.
The footer verifies section hashes and the overall logical content hash. It
never writes `current.json`, deletes legacy artifacts, or changes runtime
capture and cleanup authority. `tests/master_v2_shadow_test.py` covers
round-trip equality, deterministic bytes, corruption/missing-section rejection
and legacy pointer preservation.

## M12 RAM migration R2 — MASTER V2 canonical read authority

- `tools/bizhawk-native-ring/master_canonical_view.py` — single canonical
  access layer backed by verified MASTER V2 sections; provides map objects,
  ranges, instruction facts, runtime relations, ownership/emission partition,
  lineage and legacy-pointer shadow verification, plus stage-local SQLite
  materialization.
- `tests/master_canonical_view_test.py` — API, relation, ownership,
  materialization and legacy-shadow contract tests.
- `build/thor-evidence/master-v2-shadow-r2/` — ignored R2 shadow and current
  pointer evidence; legacy canonical files remain the comparison authority.

## M12 RAM migration R3 — provenance and outcomes authority

- `tools/bizhawk-native-ring/master_outcome_view.py` — verified read API for
  MASTER V2 provenance, Stage 5–9 outcomes and absorbed-run history; missing or
  corrupt sections fail closed without a legacy fallback.
- `tests/master_outcome_view_test.py` — projection, corruption and authority
  contract tests.
- `build/thor-evidence/master-v2-shadow-r3/` — ignored R3 shadow container,
  deterministic acceptance report and authority pointer; legacy receipts stay
  present for shadow comparison.

## M12 RAM migration R4 — MASTER V2 startup authority

- `tools/bizhawk-native-ring/master_startup_authority.py` — fail-closed
  MASTER V2 plus canonical ROM startup validator, explicit legacy shadow mode,
  startup IO metrics and deterministic N+1 candidate boundary.
- `tools/bizhawk-native-ring/live_forward_rom_link_runtime.py` — optional
  `--master-startup` preflight used by the desktop launcher before BizHawk.
- `tests/master_startup_authority_test.py` — MASTER-only startup, integrity,
  no-legacy-read and continuation regressions.
- `build/thor-evidence/master-v2/current.json` — ignored atomic startup pointer;
  it references the accepted R3 MASTER without replacing legacy artifacts.

## M12 R5 in-process post-run

- `tools/bizhawk-native-ring/live_forward_postrun_coordinator.py` — explicit
  Stage 1–9 in-process coordinator, context/result objects, and event sink.
- `tests/live_forward_postrun_coordinator_test.py` — event, background lifetime,
  STOP/ERROR, and diagnostic snapshot contracts.
- `tools/bizhawk-native-ring/live_forward_rom_link_runtime.py` — selects the
  in-process coordinator for MASTER-authorized interactive runs while retaining
  the old subprocess path.

## M12 V2 run contribution reconciliation

- `tools/bizhawk-native-ring/master_v2_contribution_boundary.py` — run-scoped
  contribution hashes, apply-once/NO_DELTA/conflict handling, VERIFY_ONLY,
  forensic no-write replay, and streaming publication of the hashed
  `run_contributions` MASTER V2 section.
- `tests/master_v2_contribution_boundary_test.py` — occurrence-safe idempotent
  replay, conflict, forensic, absorption-gate and SOURCE_OWNED regressions.
- `build/thor-evidence/historical-run-contribution-ledger.json` — ignored
  forensic ledger for all historical entries; raw evidence remains external.

## M12 V2 runtime authority

- `tools/bizhawk-native-ring/master_v2_runtime_bridge.py` — materializes a
  campaign-local rolling scratch from V2 and atomically promotes a sealed
  post-run contribution back into the current MASTER V2.
- `build/thor-evidence/master-v2/current.json` and its resolved binary — the
  single persistent canonical/provenance/outcome read and write authority.

## M12 RAM migration R6 — Stage 5 in-memory Cartographer

- `tools/bizhawk-native-ring/stage5_session_memory.py` — production Stage 5
  RAM session, exact FLOW aggregation, semantic witnesses, and ephemeral exact
  decoder adapter.
- `tools/bizhawk-native-ring/stage5_runtime_occurrences.py` — raw-record-bound
  occurrence witnesses persisted into the in-memory Stage 5 MAP-1 session.
- `tools/bizhawk-native-ring/stage5_disk_legacy.py` — retained disk-backed
  Stage 5 implementation for explicit shadow/regression comparison only.
- `tools/bizhawk-native-ring/live_forward_complete_pipeline.py` — selects the
  RAM Stage 5 path while keeping canonical candidate/master publication on
  disk and releasing the session before Stage 6.

## M12 RAM migration R7 — in-memory FLOW handoff

- `tools/bizhawk-native-ring/flow_stream.py` — bounded ordered FLOW chunks,
  backpressure, hashes and zero-disk accounting.
- `tools/bizhawk-native-ring/stage5_stream.py` — incremental Stage 5 RAM
  consumer that builds the existing `Stage5SessionMemory` contract.
- `tools/bizhawk-native-ring/flow_handoff_runtime.py` — runtime owner and
  consumer thread that fans chunks to Stage 5 and control provenance.
- `tools/bizhawk-native-ring/live_forward_control_provenance_stage.py` —
  incremental control-provenance consumer with the legacy disk comparator
  retained.
- `tests/flow_stream_test.py` — queue ordering, backpressure, hash and failure
  propagation regressions.

## M12 RAM migration R8 — analysis performance

- `src/tools/thor_evidence/stage7_decode.py` — bounded parallel external
  candidate decoder with stable ordinal results and fail-closed error capture.
- `src/tools/thor_evidence/map_driven_asm_stage7.py` — uses the bounded decode
  batch while retaining ordered proof, promotion and merge.
- `tools/bizhawk-native-ring/live_forward_postrun_coordinator.py` — publishes
  post-run timing, throughput, queue/RAM peaks and R7 invariant counters.
- `tests/stage7_decode_test.py` — stable ordering and decode-error regressions.
- `tests/stage7_decoder_contract_test.py` — Stage 7 range-tool capability,
  fail-closed classification, and canonical-ROM decode fixtures.
- `tests/live_forward_postrun_coordinator_test.py` — performance report and
  invariant accounting regression.

## M12 R9 — measured decode deduplication and Stage 7 snapshot reuse

- `src/tools/thor_evidence/stage7_decode.py` — complete decode request-key telemetry, duplicate accounting, bounded pool metrics and cross-iteration metric summarization; no persistent cache.
- `src/tools/thor_evidence/map_driven_asm_stage7.py` — reuses unchanged SQLite snapshots while preserving fresh reads after promotion.
- `tests/stage7_decode_test.py` — key distinction, duplicate measurement, cross-iteration summary and stable failure/order regressions.

## M12 Worker W6 — sustained live game discovery run and analysis

- `src/tools/thor_evidence/w6_live_discovery.py` — long-run live discovery session orchestrator and wave chunk manager for 128 Workers at depth 512.
- `src/tools/thor_evidence/w6_discovery_analysis.py` — post-run discovery and novelty analysis engine classifying execution, memory, I/O, VDP, and audio activity against accepted Thor Brain with zero SOURCE_OWNED promotion.
- `tests/w6_live_discovery_test.py` — unit and integration test suite covering canonical constants, ROM verification, chunk scanning, interval merging, novelty classification, schema verification, and SOURCE_OWNED conservation.
- `build/m12-w6-live-discovery/` — directory containing 106 raw wave chunks and all 13 canonical discovery artifacts.

## Desktop Worker Control launcher update (W6/W3 coherent runtime)

- `tools/bizhawk-native-ring/live_forward_worker_control_launcher.py` — dedicated desktop launcher script running with zero mandatory arguments, defaulting to 128 workers, depth 512, 512 KiB memory, verified BizHawk install, canonical ROM, and scaling Lua script.
- `tools/bizhawk-native-ring/live_forward_worker_control.py` — host-side status publisher with updated 512 KiB memory configuration and direct CLI launcher entry point.
- `tools/bizhawk-native-ring/live_forward_worker_control_model.py` — resource budget and next-run configuration model with defaults updated to 128 workers and depth 512.
- `tools/bizhawk-native-ring/live_forward_rom_link_runtime.py` — runtime coordinator with modular `build_parser()`, legacy disassembler prerequisites removed/optional, and 48-byte segment spooling for interactive sessions.
- `tools/bizhawk-native-ring/thor_worker_control_desktop.ps1` — repository PowerShell launcher script backing the Windows desktop shortcut for W6/W3 coherent runtime.
- `tests/live_forward_worker_control_launcher_test.py` — regression tests for parser defaults, custom parameter overrides, fallback configuration, and launcher callability.

## M12 VDP / DMA Analysis and Durable Real VDP Oracle V2

- `tools/bizhawk-native-ring/live_forward_vdp_stage.py` — fail-closed VDP and DMA analysis stage for M12 post-run pipeline, decoding control commands, registers, DMA operations, and verifying against Durable Real VDP Oracle V2.
- `tools/bizhawk-native-ring/capture_real_vdp_oracle.py` — capture script using real BizHawk runtime to freeze Durable Real VDP Oracle V2 (`vdp_oracle_v2_f200`) with raw event capture, logical frame artifact, manifest, and receipt.
- `docs/reports/THOR_M12_VDP_ORACLE_V2_RECEIPT.json` — non-copyrighted checked-in receipt for Durable Real VDP Oracle V2 with SHA256 hashes, metadata, and zero SOURCE_OWNED delta.
- `tests/live_forward_vdp_stage_test.py` — unit and regression test suite verifying VDP/DMA decoding, register matching, destination domains, DMA classification, and Durable Real VDP Oracle V2 validation.
# M12 targeted dynamic sprite capture

- `src/tools/re_bizhawk_m12_targeted_dynamic_sprite.lua` — developer-only
  natural-input BizHawk collector; snapshots the SAT window and accepted
  `0xFF13CC` shadow range every frame and records shadow write PCs.
- `src/tools/m12_targeted_dynamic_sat_capture.py` — fail-closed entry-level
  SAT/shadow diff and writer-window analyzer; does not promote ownership or
  gameplay semantics.
- `tests/m12_targeted_dynamic_sat_capture_test.py` — focused analyzer
  regression tests.
# M12 dynamic SAT producer backtrace

- `src/tools/re_bizhawk_m12_dynamic_sat_backtrace.lua` — developer-only
  runtime register/source capture for the bounded `0xB74E..0xB78C` producer.
- `src/tools/m12_dynamic_sat_producer_backtrace.py` — static decode, exact
  ROM-source transform, multi-frame validation, and four-artifact receipt.
- `tests/m12_dynamic_sat_producer_backtrace_test.py` — writer mapping and
  transform regression tests.
- `docs/reports/THOR_M12_DYNAMIC_SAT_PRODUCER_BACKTRACE_V1.md` — checkpoint
  report and evidence boundary.

# M12 dynamic SAT ROM index / A0 provenance

- `src/tools/m12_dynamic_sat_rom_index_provenance.py` — fail-closed analyzer
  for A0 reaching definitions, ROM-index decomposition, caller candidates,
  and multi-frame provenance receipt.
- `docs/reports/THOR_M12_DYNAMIC_SAT_ROM_INDEX_A0_PROVENANCE_V1.md` — STOP
  report preserving the unresolved B730-entry caller boundary.

# M12 dynamic SAT caller-side A0 provenance

- `src/tools/m12_dynamic_sat_caller_a0_provenance.py` — caller-side A0
  backtrace analyzer and multi-frame receipt.
- `docs/reports/THOR_M12_DYNAMIC_SAT_CALLER_A0_PROVENANCE_V1.md` — accepted
  caller/control-transfer and A0 reaching-definition report.

# M12 dynamic SAT selector / A6 provenance

- `src/tools/m12_dynamic_sat_selector_a6_provenance.py` — A6 field writer,
  selector-to-ROM mapping, and multi-frame provenance analyzer.
- `docs/reports/THOR_M12_DYNAMIC_SAT_SELECTOR_A6_PROVENANCE_V1.md` — accepted
  selector/A6 provenance report.

# M14.7B normalized-generic v2 adapter

- `src/tools/thor_evidence/normalized_v2_canonical_adapter.py` streams only
  producer-defined primary `records[]`, maps exact existing M68K instructions,
  preserves per-ordinal evidence provenance, and accounts unresolved/rejected
  records without skipping.
- `src/tools/thor_evidence/rom_knowledge_hash_stream.py` computes existing
  canonical logical hashes directly from ordered SQLite rows with bounded
  memory.
- `src/tools/thor_evidence/rom_knowledge_fusion_stream.py` provides the shared
  large-JSON reader and bounded structural skipping used by the normalized-v2
  adapter.
- `tests/normalized_v2_canonical_adapter_test.py` covers primary-array
  accounting, exact instruction linking, unresolved/rejected preservation,
  replay idempotence, hash compatibility, and staged canonical publisher
  integration.
# Runtime ROM property map v1

- `src/tools/thor_evidence/runtime_rom_properties.hpp/.cpp` implement the
  dense observed-property map, dirty pages, range conversion, identity checks,
  versioned checkpoint codec, atomic checkpoint persistence, and rebuildable
  compatible-run unions.
- `src/tools/thor_evidence/runtime_rom_observation.hpp/.cpp` validate supported
  M68K decoder spans, exact physical-offset data reads, captured Z80 byte
  spans, exact-copy RAM origins, and consumed VDP source bytes.
- `src/tools/thor_evidence/runtime_rom_properties_contract.json` is the pinned
  bit-level proof and rejection contract. Its exact SHA-256 is embedded in the
  checkpoint schema.
- `src/tools/thor_evidence/runtime_rom_properties_tool.cpp` exports complete
  `[start,end)` property ranges and merges per-run checkpoint contributions
  with identity checks and a source-file checksum manifest.
- `src/tools/thor_evidence/runtime_rom_map_integration.py` validates range
  exports and overlays their property masks on canonical boundaries while
  preserving canonical ownership.
- `tools/bizhawk-native-ring/rom_properties_core.c/.h` provide the native
  property bitmap, counters, dirty pages, and exact-span collector used by
  instrumented GPGX.
- `tools/bizhawk-native-ring/rom_properties_runtime.c/.h` adapt pinned M68K,
  Z80, direct VDP DMA, completed same-instruction ROM `MOVE.W` data-port
  writes, and exact Format-A decoder reads to fail-closed physical ROM offsets.
- `tools/bizhawk-native-ring/apply_runtime_rom_properties_v1.py` applies the
  pinned BizHawk/GPGX integration; `GPGX.LiveRomProperties.cs` and
  `runtime_rom_properties_capture.lua` expose bounded post-run map export.
  `runtime_rom_properties_parity.lua` records per-frame M68K/Z80 registers,
  deterministic RAM markers, and RAM hashes; `runtime_rom_properties_acceptance.py`
  runs matched parity and performance samples while restoring the corrected
  core afterward. `runtime_rom_properties_scenario_capture.lua` runs a bounded,
  deterministic input schedule and exports an independently identified map.
  `runtime_rom_properties_parity.lua` records bounded per-frame M68K/Z80
  registers, deterministic RAM markers, and RAM hashes for parity runs.
- `cmake/runtime_rom_properties_core.cmake` and
  `tests/runtime_rom_properties_core_test.c` build and test the native C map.
- `cmake/project_tests.cmake` holds test and test-tool target declarations
  separated from the root build file to keep each CMake source below 500 lines.
- `tests/runtime_rom_properties_tool_test.py` also tests strict live hex import
  into a ROM/core-identified checkpoint.
- `tests/runtime_rom_properties_z80_arg16_test.py` guards two operand-byte
  reads, exact Z80 PC advancement by two, and following-opcode alignment.
- `tests/runtime_rom_properties_vdp_hook_test.py` guards the Mode 5 CPU data
  port hook and its same-instruction source gate.
- `tests/runtime_rom_properties_audio_hook_test.py` guards the existing Z80
  banked-read hook, exact decoder PCs, and Format-A source bounds.
- `tests/runtime_rom_properties_graphics_hook_test.py` guards the completed
  `0x3820` input-span contract, exact ROM continuity checks, and compressed
  graphics property declaration.
- `tests/runtime_rom_properties_z80_arg16_test.py` guards the patched Z80
  two-byte operand read, exact PC advance, and following-opcode alignment.
- `tests/runtime_rom_properties_test.cpp` and
  `tests/runtime_rom_map_integration_test.py` cover maps, checkpoint integrity,
  merge algebra, supported instruction spans, origins, range coverage, and
  canonical ownership invariance.
- `tests/runtime_rom_properties_fixture.cpp` and
  `tests/runtime_rom_properties_tool_test.py` exercise checkpoint merge,
  source manifest checksums, CLI range export, duplicate-run rejection, and
  incompatibility rejection end-to-end.
- `docs/ROM_PROPERTY_MAP.md` defines proof contracts and implementation
  boundaries.
- `docs/reports/THOR_RUNTIME_ROM_PROPERTIES_RESEAL_20260927.md` records the
  corrected runtime manifest/checkpoint identity, exact replay results,
  bounded parity, repeated A/B/C performance measurements, and M2–M9 status.
- `docs/reports/THOR_RUNTIME_ROM_PROPERTIES_EXPANSION_20260927.md` records the
  longer corrected-runtime capture, exact range replay and read-only canonical
  overlay, expanded direct-class coverage, and remaining proof gaps.
- `docs/reports/THOR_ROM_PROPERTY_CONSUMER_CLASSIFICATION_20260927.md` records
  the bounded direct-VDP and audio consumer iterations, fresh audio checkpoint
  replay, classified-coverage delta, parity, and benchmark limitations.
- `docs/reports/THOR_ROM_PROPERTY_GRAPHICS_CONSUMER_20260927.md` records the
  live `0x3820` compressed-source property, sealed checkpoint, exact replay,
  bounded parity, and map-off/map-on timing.
- `tools/bizhawk-native-ring/rom_coverage_gui.py` displays sealed property
  checkpoints in a read-only address-ordered Defraggler-style grid, with
  separate observed-only and proven-class coverage, baseline-gated coverage
  and class deltas, responsive legend wrapping, and exact hover details;
  `rom_coverage_model.py` validates checkpoint/overlay identities and computes
  byte-accurate cell states, coverage summaries, and filters.
- `tests/rom_coverage_model_test.py` covers checkpoint integrity, overlay
  pairing, exact cell partitioning, overlapping properties, coverage deltas,
  and property filters.
- `tools/bizhawk-native-ring/rolling_master_map_gui.py` opens run-local rolling
  MASTER SQLite snapshots in a read-only ROM address grid, with occurrence heat
  and click-through 1 KiB, 32-byte, and byte detail. It explicitly separates
  runtime PCs from byte ownership and asset proof; `rolling_master_map_model.py`
  validates schema/ROM identity and performs bounded cell and instruction reads.
- `tools/bizhawk-native-ring/live_forward_postrun_map_panel.py` asynchronously
  follows a run-local MASTER pointer and renders the verified runtime-address
  overview in the post-run progress window; it never writes evidence.
- `tools/bizhawk-native-ring/live_forward_postrun_window.py` keeps the stage
  monitor responsive and provides the post-run runtime-map tab after live worker
  capture ends.
- `tests/rolling_master_map_model_test.py` covers read-only access, ROM/schema
  identity, range validation, cell aggregation, and instruction lookup for the
  rolling MASTER viewer.
- `src/tools/thor_evidence/consumer_coverage_backlog.py` validates the current
  property checkpoint against the local ROM and contract hash, verifies
  accepted W4/W5 receipt links, and writes a read-only ranked backlog in JSON
  and Markdown. Candidate overlap is an upper bound; it never modifies the
  checkpoint, property masks, or canonical ownership.
- `tests/consumer_coverage_backlog_test.py` covers interval union,
  observed-only intersection, fail-closed bounds, accepted evidence identity,
  and current backlog metrics. The generated report is
  `docs/reports/CONSUMER_COVERAGE_BACKLOG_20260928.md` with a JSON companion.
- `src/tools/thor_evidence/m14_consumer_evidence_import.py` validates accepted
  W5/W4 evidence and emits a separate sparse `ACCEPTED_ARTIFACT` contribution
  plus invariant report; it never changes the runtime checkpoint or canonical
  SQLite. `tests/m14_consumer_evidence_import_test.py` covers W5 secondary
  resource acceptance, identity/schema/proof/boundary rejection, exact replay,
  deterministic replay, removal, and authority invariance. Outputs are written
  under ignored `build/m14-consumer-evidence-import/`.
- `src/tools/thor_evidence/m14_gfx_accepted_artifact_import.py` reconstructs
  the accepted M12-GFX-2 closure for `[0x18A4BE,0x18CC6E)` from pinned Git
  evidence, replays the exact independent decoder boundary, and emits a
  removable `COMPRESSED_GRAPHICS_SOURCE` artifact contribution. Its focused
  tests are in `tests/m14_gfx_accepted_artifact_import_test.py`; generated
  artifact, contribution, and machine report remain under ignored
  `build/m14-consumer-evidence-import/`. The checked human report is
  `docs/reports/THOR_M14_GFX_ARTIFACT_IMPORT_18A4BE_18CC6E_20260928.md`.
- `src/tools/thor_evidence/m14_gfx_accepted_artifact_batch.py` reconstructs
  and imports the accepted TOP-20 exact M12-GFX source ranges through the same
  proof contract, with a separate removable ledger per range. Its tests are in
  `tests/m14_gfx_accepted_artifact_batch_test.py`; batch outputs remain under
  ignored `build/m14-consumer-evidence-import/`. The dry-run inventory and
  final acceptance report are under `docs/reports/`.
- `src/tools/thor_evidence/contracts/m14_z80_driver_image_upload_v1.json` and
  `m14_z80_driver_image_contract.py` bind accepted AUTO2 upload, W5 destination,
  and W4 execution/sink sources to one exact Z80 driver image range. The
  verifier reports overlap and potential gain without importing a property;
  its focused tests are `tests/m14_z80_driver_image_contract_test.py`. The
  ranked five-family inventory is recorded in
  `docs/reports/THOR_CONSUMER_FAMILY_BACKLOG_20260928.md` and `.json`; the
  machine proof receipt stays under ignored `build/m14-consumer-evidence-import/`.
- `src/tools/thor_evidence/m14_z80_driver_image_import.py` imports the exact
  accepted Z80 image as `Z80_DRIVER_IMAGE_SOURCE` in a typed artifact-only
  contribution schema, separate from runtime property masks. It revalidates
  the source contract and existing W5/GFX ledgers, measures the union coverage,
  and proves removal/rebuild and deterministic replay. Its regression test is
  `tests/m14_z80_driver_image_import_test.py`; the resulting removable ledger
  and report are generated under ignored `build/m14-consumer-evidence-import/`.
- `docs/reports/THOR_AUDIO_MODE0_CONSUMER_BACKLOG_20260930.md` and `.json`
  rank the next audio payload consumer family after low-yield graphics linkage.
  The report keeps all 65 unaccepted Mode-0 resources candidate-only and names
  the 9,698-byte top range; it does not extend the runtime property contract or
  import a contribution.
- `docs/reports/THOR_AUDIO_FORMAT_A_MODE0_ENTRY4_CLOSURE_20260930.md` audits
  accepted W4/W5/W6 consumer evidence for bank `0x17` entry `4`. It remains
  `NEEDS_LIVE_USE` because no accepted exact consumer-to-DAC chain covers it.
- `docs/reports/THOR_AUDIO_FORMAT_A_MODE0_ENTRY4_DISCOVERY_20260930.json`
  hash-binds 12 historical audio-scenario FLOW replays used only to locate a
  productive future trigger; they are not accepted contribution evidence.
- `docs/reports/RUNTIME_ORIGIN_V2_SOURCE_BUNDLE_PREFLIGHT_20260929.md` and
  `.json` record the frozen runtime source-bundle audit, expected and observed
  source hashes, missing baseline patch inputs, and the stop gate before clean
  dual builds. No incomplete bundle is presented as accepted runtime input.


## Runtime V3 build inputs and preflight

- `runtime-build-inputs/runtime-v3/` — candidate frozen runtime source/patch/generator inputs plus per-file SHA-256 manifest; not accepted until clean A/B builds and runtime gates pass.
- `docs/reports/RUNTIME_V3_BUILD_PREFLIGHT_20260929.*` — machine-readable and human-readable status of the V3 integration preflight.

- `docs/reports/RUNTIME_V3_GRAPHICS_BRIDGE_LIVE_20260929.md` and `.json` record capability-scoped V3 build identity, the bounded graphics transaction live result, isolated collector benchmark, and the frozen runtime lifecycle blocker. No core rebuild or evidence-map promotion was made.
- `runtime-build-inputs/runtime-v3-patch-stack-v2/` — exact ordered BizHawk/GPGX V3 patches, frozen lifecycle generator/source inputs, and SHA-256 manifest; clean-applied and built reproducibly, but graphics live acceptance remains blocked on VDP transfer observation.
- `docs/reports/RUNTIME_V3_PATCH_STACK_V2_20260929.md` and `.json` — exact pinned identities, file/patch/build hashes, clean apply, smoke, and bounded graphics live result.
# V3 Z80 audio consumer discovery

- `src/tools/thor_evidence/z80_audio_discovery.py` — bounded, identity-checked
  reducer for Mode-0 resource reads, Format-A decoder hits, and explicitly
  proven strict DAC causal-chain events. It is discovery-only and emits no
  ROM property.
- `src/tools/thor_evidence/z80_audio_discovery_cli.py` — JSONL-to-compact-JSON
  receipt command for the reducer.
- `tests/z80_audio_discovery_test.py` — catalog size, unique-read accounting,
  run identity, and causal-vs-temporal regression coverage.
- `docs/reports/THOR_V3_Z80_AUDIO_CONSUMER_DISCOVERY_20260930.md` and `.json` —
  current V3 instrumentation boundary, reducer status, and live-capture blocker.
- `runtime-build-inputs/runtime-v3-z80-audio-diag-v1/` — frozen, bounded
  V3 Z80 diagnostic overlay sources and manifest; separate from the accepted
  V3 runtime base and does not change property semantics.
- `tools/bizhawk-native-ring/z80_audio_diag_capture.lua` and
  `run_z80_audio_diag_capture.py` — bounded natural-run capture and receipt
  generation; `z80_audio_diag_perf.lua` and `run_z80_audio_diag_perf.py`
  measure collector-disabled/enabled throughput.
- `tests/z80_audio_diag_core_test.c` — bounded-ring, exact-ROM-resolution,
  and event-accounting synthetic tests.
- `docs/reports/THOR_V3_Z80_AUDIO_DIAGNOSTIC_OVERLAY_20260930.md` and `.json` —
  live discovery and isolated three-run performance results. The old consumer
  discovery report above remains the pre-overlay historical status.


## V3 Z80 audio strict-dataflow prototype (2026-09-30)

- `runtime-build-inputs/runtime-v3-z80-audio-diag-v2/`: frozen bounded observer/provenance source bundle and manifest for the experimental W5 strict dataflow overlay.
- `tools/bizhawk-native-ring/run_z80_audio_diag_capture.py`: bounded run launcher and host reducer entry point for this overlay.
- `tests/z80_audio_diag_core_test.c`, `tests/z80_audio_discovery_test.py`: synthetic provenance and strict-reducer tests.
- `docs/reports/THOR_V3_Z80_AUDIO_STRICT_DATAFLOW_20260930.*`: machine receipt and human-readable outcome.


## V3 bounded Z80 audio reject diagnostics (2026-09-30)

- `runtime-build-inputs/runtime-v3-z80-audio-diag-v2/sources/z80_audio_provenance.[ch]` — one-resource Format-A state shadow, strict sample/DAC linkage, up to 64 exact merged root ranges per pending sample, and first-eight reject diagnostics.
- `runtime-build-inputs/runtime-v3-z80-audio-diag-v2/sources/z80_audio_diag.c` — event serialization for bounded reject details, strict range roots (schema v3), and per-subreason totals.
- `src/tools/thor_evidence/z80_audio_discovery.py` — fail-closed v2 offset-list and v3 root-range validation, strict-root coverage accounting, and rejection-reason counters.
- `src/tools/thor_evidence/m14_audio_strict_import.py` — validates and imports exact sparse strict ROM→Format-A→DAC intervals as independently removable `AUDIO_STRICT_CAUSAL_SOURCE` artifact contributions, separate from the runtime property contract.
- `src/tools/thor_evidence/m14_audio_strict_baseline.py` — reconstructs total-proven coverage from the validated runtime checkpoint and accepted artifact ledgers, preserving zero-mask partition spans as unknown.
- `src/tools/thor_evidence/m14_audio_strict_sources.py` — verifies frozen collector inputs, raw capture hashes and receipts, and matching GPGX WBX/ZST identities for strict audio contributions.
- `tests/m14_audio_strict_import_test.py` — checks Entry 7 sparse complement, Entry 4/batch controlled identity, and the separation from runtime property masks.
- `tools/bizhawk-native-ring/run_z80_audio_diag_capture.py` — bounded capture, first-N reject extraction, strict coverage and rejection aggregates.
- `docs/reports/THOR_V3_Z80_AUDIO_REJECT4_RESOLUTION_20260930.*` — root cause, deterministic replay, and machine receipt.
- `docs/reports/THOR_V3_Z80_AUDIO_ENTRY7_PROVENANCE_SCALING_20260930.*` — exact Entry 7 range coverage, second-segment witness, deterministic replay hashes, and the fail-closed root-cap frontier.
- `docs/reports/THOR_V3_Z80_AUDIO_ENTRY7_GAP_CLOSURE_20260930.*` — byte-exact inventory for Entry 7's remaining strict-root gaps, frame-identity witness rejects, range-capacity analysis, and deterministic V3 replay hashes.
- `runtime-build-inputs/runtime-v3-z80-audio-diag-v3/` — catalog-scoped strict audio diagnostic overlay inputs for the second resource; its manifest pins every source and generator hash.
- `docs/reports/THOR_V3_Z80_AUDIO_SECOND_RESOURCE_20260930.*` — controlled command-to-entry-4 strict consumer result, replay comparison, and unproven payload remainder.

## V3 Mode-0 batch consumer harvest (2026-09-30)

- `runtime-build-inputs/runtime-v3-z80-audio-diag-v3/` — hash-verified catalog-scoped diagnostic source/generator bundle used for the controlled second-resource and batch runs.
- `docs/reports/THOR_V3_Z80_AUDIO_SECOND_RESOURCE_20260930.*` — strict partial consumer proof and deterministic replay for bank 0x17 entry 4.
- `docs/reports/THOR_V3_Z80_AUDIO_MODE0_BATCH_HARVEST_20260930.*` — two-run exact-range receipts and aggregate partial strict-root coverage for the remaining 64 Mode-0 candidates.
- `docs/reports/THOR_AUDIO_MODE0_STRICT_RESIDUAL_CLOSURE_20261001.md` and `.json` — byte-exact residual ranges across all 65 resources, read/decoder observations, strict reducer replay, fail-closed cause split and unchanged authority totals.
- `docs/reports/THOR_WHOLE_ROM_FAMILY_CLOSURE_20260930.md` — joins current Mode-0 residual replay, seven blocked `0x3820` caller audits, reusable known graphics/audio contracts, Carver structural UNKNOWN families, and bounded family stop/rank decisions; non-classifying.
- `docs/reports/THOR_CALLER_SCOPED_03C07C_OBSERVER_20260930.md` — records the narrow runtime observer contract and the no-hit bounded runs; no range classification or ownership promotion.
# `src/tools/thor_evidence/m14_graphics_vram_causal_import.py`

- Validates deterministic stage-1 graphics publication receipts and emits
  independently removable `GRAPHICS_VRAM_CAUSAL_SOURCE` artifact ledgers.
- Keeps graphics publication causality separate from runtime property masks,
  `COMPRESSED_GRAPHICS_SOURCE`, and canonical ROM ownership.

# `tests/m14_graphics_vram_causal_import_test.py`

- Exercises exact graphics transaction replay, sparse range accounting,
  contribution removal, and property namespace separation.
# `tools/bizhawk-native-ring/graphics_transaction_scenario_capture.lua`

- Runs reproducible bounded graphics bridge captures with scenario-specific
  controller input profiles while preserving the pinned runtime collector.

# `src/tools/thor_evidence/m14_graphics_vram_scenario_harvest.py`

- Replays bounded A/B scenario receipts, ranks exact graphics causal ranges by
  unique total-proven gain, and stores proof-depth receipts separately when no
  new coverage is available.

# `tests/m14_graphics_vram_scenario_harvest_test.py`

- Checks exact sparse overlap accounting, half-open range rejection, and
  replay identity matching for graphics publication candidates.

# `src/tools/thor_evidence/static_family_sentinel_list.py`

- Validates a separately versioned structural contribution for exact
  sentinel-terminated list members, their physical union, source hashes, and
  structural-only claim boundaries.

# `src/tools/thor_evidence/auto13_bcea_structural_closure.py`

- Independently regenerates the AUTO13 field-3 relative-pointer census,
  validates the `0x00BCEA` 44-byte/negative-key parser contract, computes exact
  overlap, and writes the removable evidence ledger and before/after/removal
  structural views.

# `tests/auto13_bcea_structural_closure_test.py`

- Covers independent census agreement, exact sentinel boundary behavior,
  machine-contract tamper rejection, structural removal, and evidence-plane
  separation.

# `docs/reports/THOR_AUTO13_BCEA_STRUCTURAL_20261004/`

- Stores A/B census JSON, full selector-slot audit, accepted contribution,
  current structural views, and closure receipt for the AUTO13 sentinel-list
  family.

# `docs/reports/THOR_AUTO13_BCEA_STRUCTURAL_CLOSURE_20261004.md`

- Records the exact signed-relative source chain, sentinel parser contract,
  family accounting, and remaining evidence limitations.

# Native caller-census receipt export

- `tools/bizhawk-native-ring/graphics_caller_census.c/.h` — bounded discovery
  event buffer, decoder completion fields, and JSON serializer.
- `tools/bizhawk-native-ring/graphics_caller_census_api.c/.h` — combines the
  existing native graphics transaction JSON with census rows and export
  diagnostics for the existing C ABI.
- `tests/graphics_caller_census_host/` — synthetic native→managed→Lua-visible
  receipt contract test with two callers and three ranges.
- `docs/reports/THOR_3820_DYNAMIC_CALLER_CENSUS_20260930.md` — export-path
  verification and the outstanding live-run timeout.

## Dynamic `0x3820` caller identity proof (2026-10-01)

- `tools/bizhawk-native-ring/graphics_call_decode.[ch]` — bounded 68000 JSR/BSR decode, exact predecessor handoff, target/return validation and `LSB_FIRST` stack lane normalization.
- `tools/bizhawk-native-ring/graphics_caller_census.[ch]` — v2 diagnostic receipt fields for proven caller/callsite, predecessor encoding and return/transaction status.
- `tools/bizhawk-native-ring/rom_properties_runtime_abort.c` — small M68K abort bridge split from the 500-line runtime module.
- `tests/graphics_caller_census_host/` — host-level census receipt and supported call-form contract tests.
- `docs/reports/THOR_3820_CALLER_IDENTITY_20261001.md` — live caller identities, exact ranges, current-union overlap, return mismatch root cause and remaining deterministic replay gap.
- `src/tools/thor_evidence/audio_mode1_structural_closure.py` — ROM-bound
  Mode-1 descriptor census, Z80 opcode contract checks, deterministic A/B
  enumeration, exact overlap, removable structural contribution, and removal
  replay.
- `tests/audio_mode1_structural_closure_test.py` — Mode-1 descriptor range,
  ROM opcode contract, A/B, authority-separation, and artifact-hash regressions.
- `docs/reports/THOR_AUDIO_MODE1_STRUCTURAL_20261002/` — dual census, 29-row
  enumeration, accepted contribution, before/after/removal views, and closure
  receipt.
- `docs/reports/THOR_AUDIO_MODE1_STRUCTURAL_20261002/THOR_AUDIO_MODE1_STRUCTURAL_CLOSURE_20261002.md` — exact Mode-1 loop contract, family accounting, and evidence limits.

# `docs/reports/THOR_M68K_SCRIPT_CONSUMER_XREFS_20261004.*`

- Records a whole-ROM discovery census of 47 decoder-recognized absolute-long
  calls to the indexed-script parser, 44 immediate D0 candidates, one finite
  local selector, two unresolved dynamic contexts, and explicit limits. This
  is not a structural or runtime contribution.
- `docs/reports/THOR_8DA4_MAIN_POOL_RESOURCE_BINDING_GRAPH_20261004.md` — direct-caller census, conditional selector-to-stream/GFX dispatch graph, field-lifecycle limits, and authority-neutral checkpoint.
- `docs/reports/THOR_MAIN_POOL_FIELD26_INDEXED_LOOKUP_20261004.md` — main-pool field census, selector-row `+0x26` pointer to indexed update lookup, and explicit unresolved extent/semantics.
- `docs/reports/THOR_MAIN_POOL_FIELD26_BOUNDARY_ATTEMPT_20261004.md` — bounded index/address audit, exact row-pointer alias scan, rejected table extent, and pivot to the adjacent `+0x22` field.

- `docs/reports/THOR_MAIN_POOL_FIELD22_CALLBACK_DISPATCH_20261004.md` — static `A6+0x22` main-pool callback sink and unresolved selector-2 instance/target join.

- `docs/reports/THOR_MAIN_POOL_FIELD1E_BOUNDED_LOOKUP_20261004.md` — bounded 16-entry secondary word lookup from descriptor row `+0x10` and its same-path relation to the fixed-count stream.

- `docs/reports/THOR_MAIN_POOL_CALLBACK_TARGET_CENSUS_20261005.md` — 99-row descriptor callback-value census and bounded target-decode results; code family/ABI unresolved.
- `docs/reports/THOR_MAIN_POOL_CALLBACK_TARGET_REGION_CLASSIFICATION_20261005.md` — runtime/direct-transfer/exception discriminator for the 30 unique `+0x18` targets; slot retained, whole-region classification unresolved.
- `docs/reports/THOR_ROOM_MAP_FRONTIER_MAIN_POOL_STREAM_LINK_20261005.md` — room/map frontier assessment and exact main-pool selector `0x00A6` to accepted `STREAM_2ECC04` edge; no RoomID semantics inferred.
- `docs/reports/THOR_D406_WORLD_GRID_CONSUMER_CLOSURE_20261005.md` — reuses accepted M7 evidence to connect the four D406 RAM structures to bounded world-grid and collision consumers; no RoomID/source-payload boundary inferred.
- `docs/reports/THOR_SCREEN_SETUP_0009_GRID_GRAPH_20261005.md` — joins ScreenID `0x0009` to D406 grid initialization and exact GFX ResourceIDs 3/4, while preserving the unresolved grid-backing source edge.
- `docs/reports/THOR_SCREEN_SETUP_DESCRIPTOR_FAMILY_20261005.md` — bounded census of three screen setup IDs sharing the 26-byte descriptor grammar and D406 setup mechanism; resource/grid limits remain explicit.
- `docs/reports/THOR_SCREEN_SETUP_FIELD_ROLE_CENSUS_20261005.md` — field-to-consumer census for the 26-byte setup descriptor, including the reusable four-byte D406 resource-selector vector.

# `docs/reports/THOR_SCREEN_SETUP_SELECTOR_VECTORS_20261005.md`

- Records exact selector bytes reconstructed from ROM-hash-verified descriptor segments for setup IDs 0x0009, 0x000C, and 0x0704; joins all twelve D406 slots to the accepted GFX ResourceID/stream manifest while retaining grid, main-pool, transition, and runtime unknowns. Machine receipt: `build/room-map-screen-setup-selector-vectors.json`.

# `docs/reports/THOR_SCREEN_SETUP_GROUP07_RESOURCE_REUSE_20261005.md`

- Records six exact group-7 screen setup mappings, two repeated four-ID vectors, and six exact descriptor `+0x04` joins to file-backed-capable screen-root streams, preserving the unresolved global selector domain and gameplay links. Machine receipt: `build/room-map-screen-setup-group07-reuse.json`.


## Screen setup group-1 family checkpoint

- `docs/reports/THOR_SCREEN_SETUP_GROUP01_FAMILY_20261005.md` — nine exact group-1 descriptor/resource mappings confirming the same path-specific screen-root plus four-slot indexed GFX vector and D406 handoff observed in group 7. Receipt: `build/room-map-screen-setup-group01-family.json`.


## Screen setup 0x0100 world-grid join

- `docs/reports/THOR_SCREEN_SETUP_0100_D406_GRID_STATE_20261005.md` — path-specific setup/resource → D406 selector → byte-grid consumer and movement-gate graph, with exact continuation stores and unresolved grid source. Receipt: `build/room-map-screen-setup-0100-grid-state.json`.

# Screen setup 0x0100 byte-grid asset

- `docs/reports/THOR_SCREEN_SETUP_0100_BYTE_GRID_ASSET_20261006.md` — exact decoded grid subrange within `STREAM_211F7A`, D406 pointer/dataflow join, 64×64 byte extent, consumer chain, externalization limits, and unchanged authorities. Machine receipt: `build/room-map-screen-setup-0100-byte-grid-asset.json`.

# Screen-root decoded layout

- `docs/reports/THOR_SCREEN_ROOT_DECODED_LAYOUT_20261006.md` — compares two exact setup-root outputs, establishes the repeated 64×64 byte-grid decoded subview, inventories proven versus unknown decoded ranges, and records the decoder-level exact-copy probe and production-loader limitation. Machine receipt: `build/room-map-screen-root-decoded-layout.json`.

## ByteGridView typed decoded subviews

- `src/game/world/byte_grid.*` — non-owning bounded grid queries, explicit width/height/stride/row-shift metadata, and a checked zero-copy factory for subranges of decoded parent buffers. The class remains a bounded native view; it does not claim ROM routine equivalence by itself.
- `tests/byte_grid_test.cpp` — synthetic boundary, dimension, pointer-identity, and distinct-parent-payload cases.

# ByteGridView and 0x9C40 candidate

- `docs/reports/THOR_BYTE_GRID_VIEW_9C40_CANDIDATE_20261006.md` — explicit zero-copy view shape and bounds, tests on both accepted decoded screen-root outputs, bounded M68K input/result contract, and why the missing stateful differential oracle keeps `0x9C40` at candidate status. Receipt: `build/byte-grid-9c40-native-candidate.json`.

# 0x9C40 natural caller capture attempt

- `src/tools/hybrid/grid_query_natural_capture.cpp` — developer-only HOOK_CPU runner that executes ordinary libretro frames, captures only an adjacent `0xE376` → `0x9C40` transfer, and records state/events through caller continuation. It never seeds guest CPU or RAM.
- `docs/reports/THOR_9C40_NATURAL_E376_CAPTURE_ATTEMPT_20261011.md` — bounded reachability result, caller liveness evidence, remaining stack/oracle blocker, and unchanged replacement status. Receipts are under ignored `build/9c40-state-capture/`.

# M68K grid footprint capture tool

- `src/tools/hybrid/grid_footprint_capture.cpp` — developer-only HOOK_CPU runner for the bounded `0x9BF2` footprint query. Validates canonical ROM identity and two exact screen-root/grid hashes, then records synthetic PRE/RTS/sentinel states, grid reads, stack writes, and ByteGridView D4/D5 aggregate comparisons. It does not alter production dispatch or evidence authorities.
- Receipt/report: `build/9bf2-contract-scout.json` and `docs/reports/THOR_9BF2_FOOTPRINT_ORACLE_20261007.md`.

# M68K grid column helper oracle

- `src/game/world/byte_grid.*` — adds a bounded vertical-column aggregation view operation (`aggregate_cell_column_world_y`) for the exact D4/D5 outputs of ROM helper `0x94A2`.
- `src/tools/hybrid/grid_leaf_capture.cpp` — developer-only HOOK_CPU runner; checks canonical identity, captures exact `0x94A2` PRE/RTS/sentinel state and cell reads, then compares the native column aggregate.
- `tests/byte_grid_test.cpp` — column aggregation and bounds regression cases.
- Report: `docs/reports/THOR_M68K_94A2_GRID_COLUMN_ORACLE_20261008.md`; machine census/receipts are under ignored `build/` artifacts.

- `src/game/entities/raw_record_initializer.*` — neutral bounded raw-record window translation for ROM helper `0x8EF8`; no gameplay semantics assigned to offsets.
- `tests/raw_record_initializer_test.cpp` — span-size and byte-layout checks for the translated initializer.
- `src/tools/hybrid/raw_record_capture.cpp` — developer HOOK_CPU PRE/RTS/sentinel runner comparing canonical ROM `0x8EF8` writes with the native helper.
- Report: `docs/reports/THOR_M68K_GAMEPLAY_HELPER_COHORT_20261008.md`.
- Census: `build/gameplay-helper-candidate-census.json`; acceptance: `build/raw-record-8ef8-acceptance.json`.
- `docs/reports/THOR_M68K_HYBRID_DISPATCH_DESIGN_20261008.md` — design-only proposal for reversible, caller-gated dispatch of the three verified path-specific helpers; no runtime implementation.
- `src/game/entities/sparse_word_store.*` — bounds-checked neutral four-word sparse store equivalent for ROM helper `0x16A46`, retaining exact register/CCR output data.
- `tests/sparse_word_store_test.cpp` — write offsets, big-endian payloads, result state, and bounds rejection checks for the sparse word-store helper.
- `src/tools/hybrid/sparse_word_store_capture.cpp` — developer-only HOOK_CPU canonical-ROM PRE/RTS/sentinel differential runner for helper `0x16A46`.
- Report: `docs/reports/THOR_M68K_16A46_SPARSE_WORD_STORE_20261008.md`; instruction census and receipts are under ignored `build/` artifacts.

# M68K transfer-source provenance

- `src/tools/hybrid/transfer_provenance_capture.cpp` — developer-only HOOK_CPU runner testing exact source-PC provenance for BSR/JSR edges, rejecting non-call transfers, and validating six known direct ROM call edges.
- `docs/reports/THOR_M68K_TRANSFER_SOURCE_PROVENANCE_20261008.md` — hook timing, provenance method, micro-test and real-call results, interrupt-safety limitation, and unchanged authority/status scope.
- Machine receipt: ignored `build/m68k-transfer-provenance.json`.

# M68K `0x8EF8` opt-in live route

- `src/tools/hybrid/raw_record_live_dispatch.cpp` — developer-only opt-in HOOK_CPU runner for a single path-specific native route, paired ROM/native direct-call probes, and unlisted-caller ROM fallback.
- Report: `docs/reports/THOR_M68K_8EF8_OPT_IN_LIVE_ROUTE_20261009.md`.
- Machine receipts are ignored under `build/raw-record-live-dispatch-*.json`.

# W6 M68K-to-Z80 ready handoff

- `docs/reports/THOR_M12_Z80_READY_HANDOFF_20261009.md` — bounded static/runtime join for the `0x60310` copy-and-ready path and Z80 poll at `0x04D0`; unresolved payload source and conditional continuation remain explicit.
- Machine metadata receipt is ignored at `build/m12-w6-z80-ready-handoff.json`.
- `docs/reports/THOR_M12_Z80_HANDOFF_SENTINEL_PATH_20261009.md` — one hash-bound W6 path closes RAM-staged `0xFF` bytes through M68K copy, ready bit, Z80 branch, and parser terminator; Mode 0/1 and other payload variants stay unresolved.
- Its payload-free machine receipt is ignored at `build/m12-w6-z80-sentinel-path.json`.
# W6 audio pointer/resource join

`docs/reports/THOR_M12_W6_91740_FORMAT_A_RESOURCE_JOIN_20261010.md` records a
hash-bound W6 Z80 sample-read join to the existing Format-A bank-18 descriptor
resource, including the Mode-1 descriptor alias and unresolved live selector
provenance. Its metadata-only machine receipt is emitted under ignored `build/`.

# Word-grid upload consumer scout

- `docs/reports/THOR_WORD_GRID_UPLOAD_SCOUT_20261010.md` — bounded static consumer shape for `0xDB84` / `0xDC26`; exact backing extent remains unresolved.
- Machine receipt: ignored `build/word-grid-upload-scout-20261010.json`.


# Controller-selected setup graph

- `docs/reports/THOR_MAIN_POOL_SCREEN_SETUP_001B_GRAPH_20261010.md` — static controller → setup descriptor → GFX/main-pool binding graph, with unresolved same-invocation edge explicit.
- `build/main-pool-screen-setup-001b-graph.json` — ignored machine receipt with ROM, manifest, and slice hashes.

- `docs/reports/THOR_MAIN_POOL_SCREEN_SETUP_001B_SAME_INVOCATION_AUDIT_20261010.md` — bounded same-invocation closure attempt; dynamic indirect target, FF0DC4 value, and row-20 consumer provenance remain unresolved.
- `build/main-pool-screen-setup-001b-same-invocation-audit.json` — ignored machine receipt pinning the closure attempt.

- `docs/reports/THOR_SETUP_MAIN_POOL_CANDIDATE_SCOUT_20261010.md` — alternate setup 0x000C candidate, conditional main-pool row-20 binding, and indirect-dispatch blocker/pivot.
- `build/setup-main-pool-candidate-scout-20261010.json` — ignored machine receipt pinning manifests, ROM, slices, and runtime novelty artifact.

- `docs/reports/THOR_RUNTIME_ANIMATION_SELECTOR_SCOUT_20261010.md` — bounded
  ranking and evidence limits for the runtime-covered selector-to-graphics/
  SAT path; its machine receipt is `build/m12-animation-selector-scout-20261010.json`.

- `docs/reports/THOR_SELECTOR_CAUSAL_CAPTURE_ATTEMPT_20261011.md` — bounded
  audit of available runtime evidence for the selector→descriptor→decoder→SAT
  chain and the resulting renderer-side pivot; machine receipt is under `build/`.

- `docs/reports/THOR_A342_SAT_SHADOW_PRODUCER_ANALYSIS_20261011.md` — bounded neutral producer contract for M68K `0xA342/0xA372`, with caller and runtime-capture limits; receipt is under ignored `build/`.
- `docs/reports/THOR_HELPER5_RUNTIME_CANDIDATE_SCOUT_20261011.md` — ranked runtime-covered helper scout; A342 full-call oracle gap and A196 stateful/indirect blocker recorded.
- `src/tools/thor_evidence/helper_candidate_index.py` — builds a SHA-256-bound callable-helper index from retained receipts/censuses, preserving UNKNOWN and distinguishing runtime PC-set membership from hit counts; records direct-caller PC-set intersections separately.
- `tests/helper_candidate_index_test.py` — schema, provenance-hash, mission-exclusion, and fail-closed strong-intersection checks for the helper index.
- `docs/reports/THOR_HELPER_CANDIDATE_INDEX_20261011.md` — top candidates, evidence intersection, blockers, and instrumentation recommendation.

- `src/tools/re_bizhawk_m12_b730_full_call.lua` — targeted full-call capture for the direct `0x003B448 -> 0xB730` path, with complete register snapshots and bounded operand/write observations.
- `src/game/render/sat_shadow_records.hpp/.cpp` — bounded native SAT shadow record effects translated from M68K `0xB730`; no runtime dispatch.
- `src/tools/m12_b730_native_differential.cpp` — ROM-bound differential runner for saved natural `0xB730` captures.
- `src/tools/hybrid/b730_branch_capture.cpp` — developer-only HOOK_CPU synthetic branch oracle for `0xB730` coordinate-cull and D2-limit paths.
- `tests/sat_shadow_records_test.cpp` — bounded validation and synthetic record-output checks.
- `docs/reports/THOR_B730_FULL_CALL_CAPTURE_20261011.md` — exact `0xB730` static and runtime call contract, native comparison, limits, and path-scoped status; machine artifacts are under ignored `build/`.

## Deterministic gameplay session capture/replay

- `src/tools/hybrid/gameplay_session.hpp/.cpp` — versioned effective input poll
  and ordered controller-query journal with strict capture/replay sequencing.
- `src/tools/hybrid/gameplay_session_identity.hpp/.cpp` — SHA-256 identities,
  deterministic configuration identity, and session manifest IO.
- `src/tools/hybrid/gameplay_session_receipts.hpp/.cpp` — stable replay-result
  receipt serialization.
- `src/tools/hybrid/grid_query_native_model.hpp/.cpp` — bounded, non-mutating
  `ByteGridView` model for M68K `0x9C40`, used by the developer replay oracle.
- `src/tools/hybrid/gameplay_session_runner.cpp` — developer-only libretro
  capture/replay executable, M68K PC/frame landmarks, and exact `0xE376 →
  0x9C40` full-call watch.
- `tests/gameplay_session_test.cpp` — poll/query roundtrip, fail-closed journal,
  manifest, identity, transfer census, and bounded full-call capture-limit tests.
- `src/tools/hybrid/m68k_call_census.hpp/.cpp` — replay-time exact transfer
  attribution for direct BSR/JSR forms and bounded natural full-call receipts.
- `src/tools/hybrid/e268_state_transform.hpp/.cpp` — native state-word effect
  model for the path-specific M68K routine at `0xE268`.
- `tests/hybrid_e268_state_transform_test.cpp` — 24 captured ROM input/output
  vectors and native effect checks for caller `0x00359A`.
- `docs/reports/THOR_REPLAY_HELPER6_E268_20261011.md` — replay census ranking,
  selected caller contract, differential result, and evidence scope.
- `docs/reports/THOR_DETERMINISTIC_GAMEPLAY_SESSION_CAPTURE_20261011.md` —
  validated capture/replay receipt and evidence limits; machine data is under
  ignored `build/gameplay-session-validation-20261011-v2/`.
- `docs/reports/THOR_9C40_E376_PATH_SPECIFIC_VERIFICATION_20261011.md` —
  24-call natural differential, caller liveness, stack-scratch scope, and
  synthetic edge-case oracle; machine receipts remain under ignored `build/`.
- `docs/reports/THOR_REPLAY_HELPER7_SCOUT_8B8E_8DD4_20261011.md` — bounded analysis of the two top replay candidates; freezes the zero-only observed paths and records unresolved state branches.
- `src/tools/hybrid/coordinate_bound_model.hpp/.cpp` — typed 16-bit model for
  the path-specific M68K bound helper at `0x9D4E`.
- `src/tools/hybrid/coordinate_bound_oracle.cpp` — developer-only canonical
  ROM HOOK_CPU oracle for bounded `0x9D4E` cases.
- `tests/coordinate_bound_model_test.cpp` — natural-capture vectors and
  arithmetic/branch tests for the coordinate-bound model.
- `docs/reports/THOR_REPLAY_HELPER7_9D4E_20261011.md` — exact boundary, caller
  ABI, natural read patterns, native differential, and limits.

- `src/tools/hybrid/gameplay_session_input_sources.{hpp,cpp}` — isolated
  fallback/scripted/physical input selection used by the session runner.
- `src/tools/hybrid/gameplay_session_landmarks.{hpp,cpp}` — deterministic
  per-frame landmark serialization and strict replay comparison.
- `src/tools/hybrid/gameplay_session_replay_diagnostic.{hpp,cpp}` — opt-in
  continued replay landmark capture with a machine receipt for the first
  mismatch; the one-shot native diagnostic falls back to ROM afterward.
- `tests/gameplay_session_replay_diagnostic_test.cpp` — matching/mismatching
  landmark capture and first-difference receipt regression test.
- `src/tools/hybrid/native_island_runtime.{hpp,cpp}` — replay-only, default-off
  route gate and typed staged adapter for `0x9D70` caller `0x009144`; no
  production dispatch.
- `tests/native_island_runtime_test.cpp` and
  `tests/gameplay_session_input_sources_test.cpp` — gate/fallback/effect and
  input-source checks.
- `docs/reports/THOR_REPLAY_NATIVE_ISLAND_ATTEMPT_20261012.{md,json}` —
  ROM-only replay pass, one-helper divergence, metrics, and unchanged statuses.

## Six-helper instruction-step replay island

- `src/tools/hybrid/m68k_9c40_instruction_step.{hpp,cpp}` — replay-only,
  caller-gated 0x9C40 instruction stepper that preserves MOVEM stack bus effects.
- `docs/reports/THOR_SIX_HELPER_INSTRUCTION_STEP_ISLAND_20261013.{md,json}` —
  exact 0x9C40 path timing/stack receipt and six-helper replay equivalence.
- Per-call replay evidence remains in ignored
  `build/multi-helper-step-9c40-20261013/` directories.
- `docs/reports/THOR_REPLAY_NATIVE_9D70_DIVERGENCE_DIAGNOSTIC_20261012.{md,json}` —
  matched one-call state comparison and strict PC-landmark divergence diagnosis.
- `src/tools/hybrid/gameplay_session_cpu.{hpp,cpp}` — shared HOOK_CPU register,
  memory, and serialized-state snapshot helpers used by replay captures.
- `src/tools/hybrid/replay_equivalence_trace.{hpp,cpp}` and
  `src/tools/hybrid/compare_normalized_replay.py` — normalized helper-event,
  guest-state, scheduler, and external-event comparison for the single
  `0x9D70` replay route.
- `docs/reports/THOR_REPLAY_NATIVE_9D70_TIMING_STATE_DIAGNOSTIC_20261012.{md,json}` —
  exact entry/return cycle and Z80 RAM mismatch receipt; route equivalence remains NO.

- `src/tools/hybrid/negative_bound_model.hpp/.cpp` — bounded native scalar model for the path-specific M68K helper at `0x9DB4`.
- `tests/negative_bound_model_test.cpp` — 24 captured replay vectors and branch/CCR-X arithmetic checks for `0x9DB4`.
- `docs/reports/THOR_REPLAY_HELPER8_9DB4_20261011.md` — candidate ranking, path-specific ABI/liveness, oracle results, and evidence limits.
- `docs/reports/THOR_REPLAY_HELPER8_9DB4_ACCEPTANCE_20261011.json` — machine-readable acceptance receipt.
- `docs/reports/THOR_REPLAY_HELPER_CANDIDATE_INDEX_20261011.json` — updated replay helper index and remaining candidate ranking.

- `src/tools/hybrid/positive_bound_model.hpp/.cpp` — typed 16-bit bound model for the path-specific M68K helper at `0x9D92`.
- `tests/positive_bound_model_test.cpp` — 24 replay vectors and clamp/flag/wrap checks for `0x9D92`.
- `docs/reports/THOR_REPLAY_HELPER9_9D92_20261011.md` — `0xA7E6` bounded rejection and `0x9D92` verification evidence.
- `docs/reports/THOR_REPLAY_HELPER9_9D92_ACCEPTANCE_20261011.json` — machine-readable path-specific acceptance receipt.
- `docs/reports/THOR_NATIVE_ISLAND_DESIGN_8_HELPERS_20261011.md/.json` — design-only registry, adapter, gate, fallback, replay-validation and metrics contract for eight verified helpers.


- `docs/reports/THOR_REPLAY_HELPER10_99D6_BOUNDARY_20261012.md/.json` — exact `0x99D6` read arithmetic, caller evidence, and unresolved extent disposition.
- `docs/reports/THOR_NATIVE_ISLAND_REGISTRY_PREP_9_HELPERS_20261012.md/.json` — nine verified path-scoped helper contracts grouped for future dispatch design; no runtime wiring.

- `src/tools/hybrid/grid_query_micro_oracle.{hpp,cpp}` — deterministic HOOK_CPU RTS harness fixture used by developer oracles; `src/tools/hybrid/subtraction_bound_model.{hpp,cpp}` — typed native model for M68K `0x9D70` word subtraction/clamp and exact flag/write effects; `tests/subtraction_bound_model_test.cpp` — 22 replay vectors plus branch vectors; `src/tools/hybrid/subtraction_bound_oracle.cpp` — developer-only canonical-ROM HOOK_CPU differential oracle. Evidence: `docs/reports/THOR_REPLAY_HELPER10_9D70_20261012.{md,json}`.

\n
# 2026-10-13 scheduler fixture additions

- `src/tools/hybrid/prepare_scheduler_fixture.py` — creates a dedicated,
  isolated external GPGX copy with scheduler/IRQ instrumentation and
  `HOOK_CPU=1`; does not modify the normal capture core.
- `src/tools/hybrid/scheduler_fixture_runner.cpp` — developer-only paired
  ROM/instruction-step micro-case runner using GPGX's frame scheduler.
- `src/tools/hybrid/scheduler_fixture_acceptance.py` — validates canonical ROM
  and hook configuration, compares ordered traces and final states, and emits
  a machine receipt.
- `docs/reports/THOR_SCHEDULER_FIXTURE_20261013.md/.json` — scope,
  instrumentation, limitations, build identities, and acceptance result.
- Full paired traces and JSON receipts are generated under ignored
  `build/scheduler-fixture-20261013/`.
- `src/tools/hybrid/m68k_9d70_instruction_step.{hpp,cpp}` — developer-only,
  caller-gated, resumable instruction-step substitute for the exact `0x9D70`
  body; production dispatch is not connected.
- `src/tools/hybrid/gameplay_session_call_capture.{hpp,cpp}` — isolated full
  call-capture serialization used by the replay runner.
- `src/tools/hybrid/replay_equivalence_trace.{hpp,cpp}` — per-call and
  per-frame CPU/scheduler/observable-memory trace for the selected `0x9D70`
  replay experiment.
- `docs/reports/THOR_9D70_INSTRUCTION_STEP_REPLAY_20261013.{md,json}` —
  natural-call timing, full-replay comparison, and disclosed serializer-hash
  instability.
- Paired replay traces are generated under ignored
  `build/9d70-step-diagnostic-20261013/post-marker3/`.

# 2026-10-13 three-helper instruction-stepped replay island

- `src/tools/hybrid/m68k_gameplay_step_set.{hpp,cpp}` — caller-gated,
  scheduler-backed instruction-step adapters for the accepted paths at
  `0x9D70`, `0x9D4E`, `0x9DB4`, and `0x9D92`; developer/replay-only.
- `src/tools/hybrid/gameplay_session_replay_options.{hpp,cpp}` — replay option
  parsing for selecting a step-helper set.
- `src/tools/hybrid/gameplay_session_step_trace.{hpp,cpp}` — filtered
  instruction-step trace collection and route markers.
- `src/tools/hybrid/normalize_step_helper_trace.py` — creates normalized helper
  events, path-specific cycle census including opcode decoding for `0x9D92`,
  and native/fallback route metrics.
- `docs/reports/THOR_MULTI_HELPER_INSTRUCTION_STEP_ISLAND_20261013.{md,json}`
  — baseline, single, pair, and triple replay acceptance for the pinned
  development replay; production dispatch remains inactive.
- `docs/reports/THOR_FOUR_HELPER_INSTRUCTION_STEP_ISLAND_20261013.{md,json}`
  — `0x9D92` single-helper and four-helper replay acceptance for the same
  pinned development session; production dispatch remains inactive.
- Raw replay runs and intermediate captures are under ignored
  `build/multi-helper-step-20261013/`.

# 2026-10-13 five-helper instruction-stepped replay island

- `src/tools/hybrid/m68k_94a2_instruction_step.{hpp,cpp}` — bounded, replay-only
  instruction-step adapter for 0x94A2, gated to direct caller 0x9468.
- `src/tools/hybrid/m68k_gameplay_step_flags.hpp` — shared condition-code
  helpers factored from the existing stepper module to keep it below 500 lines.
- `docs/reports/THOR_FIVE_HELPER_INSTRUCTION_STEP_ISLAND_20261013.{md,json}` —
  0x94A2 timing, read-bound, single-helper and five-helper replay receipt.
- Raw replay captures and reconstructed read/timing census are under ignored
  `build/multi-helper-step-94a2-20261013/`.

# 2026-10-13 six-helper instruction-stepped replay island

- `src/tools/hybrid/m68k_9c40_instruction_step.{hpp,cpp}` — replay-only,
  caller-gated 0x9C40 stepper preserving MOVEM stack bus effects.
- `docs/reports/THOR_SIX_HELPER_INSTRUCTION_STEP_ISLAND_20261013.{md,json}` —
  exact 0x9C40 path timing/stack receipt and six-helper replay equivalence.
- Per-call replay evidence is under ignored
  `build/multi-helper-step-9c40-20261013/`.

# 2026-10-13 seven-helper instruction-step replay island

- `src/tools/hybrid/m68k_8ef8_instruction_step.{hpp,cpp}` — exact ROM-checked, caller-gated stepper for the previously accepted 0x8EF8 BSR.W callers.
- `src/tools/hybrid/gameplay_session_replay_diagnostic.{hpp,cpp}` and `tests/gameplay_session_replay_diagnostic_test.cpp` — report actual replay landmark divergence independently from diagnostic-only status.
- `src/tools/hybrid/normalize_step_helper_trace.py` — 0x8EF8 opcode/cycle census and per-helper native/fallback counts for seven-helper runs.
- `docs/reports/THOR_SEVEN_HELPER_INSTRUCTION_STEP_ISLAND_20261013.{md,json}` — candidate comparison, single-helper and seven-helper acceptance receipt.
- Final validated replay artifacts are under ignored `build/multi-helper-8ef8-20261013/final-validated/`.

# 2026-10-13 eight-helper instruction-stepped replay island

- `src/tools/hybrid/m68k_e268_instruction_step.{hpp,cpp}` — ROM-checked,
  caller-gated instruction stepper for `0x00359A → 0xE268` only.
- `src/tools/hybrid/gameplay_session_metrics.{hpp,cpp}` — focused native-island
  and E268 counters shared by the developer replay runner.
- `docs/reports/THOR_EIGHT_HELPER_INSTRUCTION_STEP_ISLAND_20261013.{md,json}` —
  path contract, timing/stack evidence, replay comparisons, and machine receipt.
- Final v5 replay artifacts are under ignored
  `build/multi-helper-e268-20261013/`.


# 2026-10-13 frozen eight-helper replay baseline

- `docs/reports/THOR_FROZEN_EIGHT_HELPER_REPLAY_BASELINE_20261013.md` — human-readable freeze scope, repeat-run result, gates, limits, performance, and next-direction recommendation.
- `docs/reports/THOR_FROZEN_EIGHT_HELPER_REPLAY_BASELINE_20261013.json` — machine-readable pinned identities, eight caller contracts, repeated replay metrics, build/test status, counter coverage limits, and authority status.
- Repeated replay artifacts and comparison summary remain under ignored `build/multi-helper-e268-20261013/frozen-repro/`.

# 2026-10-13 frozen island timing telemetry V2

- `src/tools/hybrid/build_island_telemetry_v2.py` — fail-closed offline join of
  frozen helper step traces, E268 runtime counters, replay identities, and the
  clean-build dependency closure. Missing counters remain `UNINSTRUMENTED`.
- `tests/build_island_telemetry_v2_test.py` — unit checks for unknown-value
  preservation and path/cycle aggregation.
- `docs/reports/THOR_REPLAY_NATIVE_ISLAND_EIGHT_V2_20261013.{md,json}` — V2
  telemetry, coverage matrix, fresh replay comparison, and build dependency set;
  V1 remains unchanged.
- The fresh replay output and matching call traces are under ignored
  `build/multi-helper-e268-20261013/frozen-repro/debug-telemetry-v2/`.

# Secondary weapon selection and use pipeline (Bow)

- `src/game/player/secondary_weapon.hpp` — secondary weapon inventory model (`WeaponInventorySlot`, `WeaponInventory`), `ArrowProjectile` entity struct, and pure lifecycle function declarations.
- `src/game/player/secondary_weapon.cpp` — authentic secondary weapon cycle logic (`cycle_secondary_weapon`), secondary attack state starter (`try_start_secondary_attack`), State 14 animation/fire/recovery progression (`update_secondary_attack`), and arrow projectile physics/terrain/actor damage collisions (`update_arrow_projectile`).
- `tests/secondary_weapon_test.cpp` — 10 deterministic unit tests covering weapon selection, skipping unowned slots, rejecting no-ammo firing, use input state entry, frame 12 arrow spawning, facing direction transforms, single ammo decrement, enemy damage and knockback, projectile wall/lifetime removal, and frame 16 return to normal.

# M15 vertical slice connected recompilation audit and cluster selection

- `docs/reports/THOR_M15_GAMEPLAY_SLICE_CONNECTED_AUDIT_20261014.md` — comprehensive audit report documenting the 12-stage deterministic slice trajectory, execution ownership model, M68K PC coverage from 3,600-frame baseline replay, native-to-ROM boundary graph, top 10 bottlenecks evaluation, connected native regions analysis, selection rationale for Player State & Action Dispatcher (`0x004A20..0x004B80`), and full translation plan.

# Execution-driven hybrid recompilation registry and first-divergence oracle

- `src/tools/hybrid/hybrid_registry.hpp/.cpp` — central native replacement registry (`HybridExecutionRegistry`) managing `NativeRegion` descriptors, active/inactive region status, supported precondition gating, fail-closed fallback recording, and execution metrics.
- `src/tools/hybrid/player_dispatch_region.hpp/.cpp` — native region definition and handler for player dispatcher (`0x004A20` marked inactive StaleStaticCandidate; canonical `0x00557A` active), routing States 0, 10, 14, and 24 to verified native logic and eliminating child provenance rejects.
- `src/tools/hybrid/main_loop_tick_region.hpp/.cpp` — native region definition for Main Loop Frame Tick / Delay Coordinator (`0x0033D2..0x003424`), registered in instruction-stepped timing mode.
- `src/tools/hybrid/entity_oscillation_region.hpp/.cpp` — bounded native region definition and handler for Entity Oscillation / Bobbing Displacement Coordinator (`0x0613B2..0x0613F6`), displacing 26,233 ROM fallbacks across callers `0x061AC2` and `0x062576`.
- `src/tools/hybrid/m68k_613b2_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for `0x0613B2`, verified in dual canonical 3,600-frame replay (Run A: 26,233 ROM executions; Run B: 26,233 native executions, 0 divergences).
- `src/tools/hybrid/m68k_61e48_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Sprite Tile Allocation / Sub-Sprite Animation Cycling Coordinator (`0x061E48..0x061EDE`), verified in dual canonical 3,600-frame replay (Run A: 16,693 ROM executions; Run B: 16,693 native executions, 0 divergences).
- `src/tools/hybrid/m68k_61934_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Display List Priority Coordinator & Sprite Tier Driver (`0x061934..0x061F7E`), verified in dual canonical 3,600-frame replay (Run A: 20,916 ROM executions; Run B: 20,916 native executions, 0 divergences).
- `src/tools/hybrid/m68k_61934_step_moves.cpp` — dedicated modular move instruction stepping operations for `0x061934`.
- `src/tools/hybrid/m68k_61934_step_ops.hpp/.cpp` — ALU, branch, bitwise, memory manipulation, and CCR update stepping operations for `0x061934`.
- `src/tools/hybrid/m68k_61934_step_tail.hpp/.cpp` — tail instructions (`0x061C80..0x061F7E`), piece emission, and loop blocks for `0x061934`.
- `src/tools/hybrid/m68k_61934_step_tail_ops.hpp/.cpp` — auxiliary ALU, memory, and piece queuing helpers for the `0x061934` tail.
- `src/tools/hybrid/first_divergence_oracle.hpp/.cpp` — differential execution validator comparing D0-D7, A0-A7, SR/CCR/X, PC, stack top, and player entity record RAM.
- `tests/hybrid_registry_test.cpp` — unit test suite verifying canonical player ABI, stale region accounting, active region dispatch, main loop tick coordinator registration, fail-closed fallback recording, differential divergence detection, and E268 instruction step caller provenance (0x00359A, 0x0033E2, 0x00340E).
- `tests/m68k_61e48_instruction_step_test.cpp` — unit test suite verifying entry caller contract, early exit branches, mode 4/5 animation cycling, LCG random generator arithmetic, display output piece queueing, and D7 restoration for `0x061E48`.
- `tests/m68k_61934_instruction_step_test.cpp` — unit test suite verifying caller gating (6 proven callers `0x0601EE`, `0x0601FA`, `0x060206`, `0x060212`, `0x06021E`, `0x060260`), early exit flows, bitwise operations, piece emission, and child call routing to `0x061E48`.
- `src/tools/hybrid/m68k_6147e_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Entity Coordinate Screen Projection & Auxiliary Component Driver (`0x06147E..0x061586`), verified in dual canonical 3,600-frame replay (Run A: 16,673 ROM executions; Run B: 16,673 native executions, 0 divergences, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_6147e_step_ops.hpp/.cpp` — core instruction-stepping primitives, bit operations, branch/jump handling, and register/RAM checks for `0x06147E`.
- `src/tools/hybrid/m68k_6147e_step_moves.cpp` — move, movem, and lea instruction-stepping operations for `0x06147E` with cycle-accurate bus refresh order and dummy postincrement reads.
- `src/tools/hybrid/m68k_6147e_step_math.cpp` — arithmetic, logic, shift, and rotate instruction-stepping operations for `0x06147E`.
- `src/tools/hybrid/m68k_6147e_table1.cpp` — jump table 1 layer merging handlers (`0x06159E..0x06163C`) for overlapping component layers.
- `src/tools/hybrid/m68k_6147e_component_step.cpp` — component timer countdown, direction sign negation, accumulator stepping, and base coordinate lookup (`0x06163E..0x0616B2`).
- `src/tools/hybrid/m68k_6147e_table2.cpp` — jump table 2 screen projection handlers (`0x0616CA..0x061932`) for directional coordinate scaling and clamping.
- `tests/m68k_6147e_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x061AC6`), component step dispatch, and exit at RTS for `0x06147E`.
- `src/tools/hybrid/m68k_623ac_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Entity Script Interpreter & Sprite Animator (`0x0623AC..0x062892`), verified in dual canonical 3,600-frame replay (Run A: 13,944 ROM executions; Run B: 13,944 native executions, 0 divergences, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_623ac_step_ops.hpp/.cpp` — core instruction-stepping primitives, branch handling, and register/RAM/PSG access checks for `0x0623AC`.
- `src/tools/hybrid/m68k_623ac_step_moves.cpp` — move, movem, and lea instruction-stepping operations for `0x0623AC` with interleaved extension word fetch / memory reads and cycle-accurate bus refresh order.
- `src/tools/hybrid/m68k_623ac_step_math.cpp` — arithmetic operations (add, sub, mulu, cmp) with dynamic bit-count cycle models.
- `src/tools/hybrid/m68k_623ac_step_logic.cpp` — bitwise logic and shift/rotate operations with dynamic shift cycle addition before `SKIP_BUS_REFRESH()`.
- `src/tools/hybrid/m68k_623ac_block1.cpp` — modular instruction dispatch block 1 (`0x0623AC..0x0624BE`) for entity script command parsing and channel initialization.
- `src/tools/hybrid/m68k_623ac_block2.cpp` — modular instruction dispatch block 2 (`0x0624C0..0x06263A`) for opcode handlers and animation step timers.
- `src/tools/hybrid/m68k_623ac_block3.cpp` — modular instruction dispatch block 3 (`0x06263C..0x062776`) for coordinate updates and SN76489 PSG sound command generation.
- `src/tools/hybrid/m68k_623ac_block4.cpp` — modular instruction dispatch block 4 (`0x062778..0x062892`) for jump tables and terminal interpreter dispatch.
- `tests/m68k_623ac_instruction_step_test.cpp` — unit test suite verifying caller gating (4 callers: `0x060234`, `0x060242`, `0x060250`, `0x060276`), SN76489 PSG audio command dispatch, shift/movem dynamic cycles, and exit at RTS.
- `tests/hybrid_callee_623ac_test.cpp` — test fixture verifying `0x0623AC` callee contract and register preservation.
- `src/tools/hybrid/m68k_6273a_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for SN76489 PSG Channel Attenuation / Volume Synthesizer (`0x06273A..0x062788`), verified in dual canonical 3,600-frame replay (Run A: 11,303 ROM executions; Run B: 11,303 native executions, 0 fallbacks, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_6273a_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x062862`, `0x062620`), fail-closed registers, mute paths, distance attenuation arithmetic and clamping, and dual RTS exits for `0x06273A`.
- `src/tools/hybrid/m68k_62788_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for ADSR Sound Envelope Generator & Tremolo Modulator (`0x062788..0x06286E`), verified in dual canonical 3,600-frame replay (Run A: 9,287 ROM executions; Run B: 9,287 native executions, 0 fallbacks, 248,168 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_62788_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x062556`), fail-closed registers, ADSR attack/decay/sustain/release transitions, volume multiplication (`mulu.w`), tremolo modulation, connected call to child `0x06273A`, and exit at RTS.
- `src/tools/hybrid/m68k_60000_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Frame Audio/Graphics Coordinator (`0x060000..0x060284`), verified in dual canonical 3,600-frame replay (Run A: 3,486 ROM executions; Run B: 3,486 native executions, 0 fallbacks, 186,101 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_60000_step_ops.hpp/.cpp` — core instruction-stepping primitives, branch/call handling, and SR/MOVEM save/restore operations for `0x060000`.
- `src/tools/hybrid/m68k_60000_step_mem.hpp/.cpp` — memory read/write, immediate moves, and flag clearing operations for `0x060000`.
- `src/tools/hybrid/m68k_60000_step_alu.hpp/.cpp` — arithmetic, logical, and shift/rotate operations for `0x060000`.
- `src/tools/hybrid/m68k_60000_block1.hpp/.cpp` — modular instruction dispatch block 1 (`0x060000..0x060180`) for coordinator preamble, sound trigger parsing, and channel setup.
- `src/tools/hybrid/m68k_60000_block2.hpp/.cpp` — modular instruction dispatch block 2 (`0x060182..0x060284`) for coordinator orchestration of child subsystems `0x062AE0`, `0x061934`, `0x0623AC`, `0x060286` and epilogue.
- `tests/m68k_60000_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x002380`), frame-level pass branches, child call routing, stack preservation, and exit at RTS.
- `src/tools/hybrid/m68k_60286_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Ancient Music Driver Z80 Sound Command Commit & Bus Controller (`0x060286..0x060352`), verified in dual canonical 3,600-frame replay (Run A: 3,486 ROM executions; Run B: 3,486 native executions, 0 fallbacks, 445,912 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_60286_step_ops.hpp/.cpp` — instruction-stepping primitives, Z80 bus request/release protocol operations, memory copy loops (`move.b (a0)+, (a1)+` / `dbf d0`), and bit operations (`btst`/`bset`) for `0x060286`.
- `tests/m68k_60286_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x06027A`), bus request and wait semantics, BGM request flag transfer, voice activity clearing, buffer 0 and buffer 1 transfers, double buffer overflow path, and 3 RTS exits.
- `src/tools/hybrid/m68k_62ae0_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Ancient Music Driver FM Voice Parameter Processor / Channel 6 Sequencer (`0x062AE0..0x062D82`), verified in dual canonical 3,600-frame replay (Run A: 3,486 ROM executions; Run B: 3,486 native executions, 0 fallbacks, 27,085 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_62ae0_step_ops.hpp/.cpp` — instruction-stepping primitives for `0x062AE0`, covering post-increment byte moves (`move.b <ea>, (a4)+`), displacement byte test/moves (`btst #b, d(an)`, `move.b d(an), (a4)+`), and byte-store immediate.
- `tests/m68k_62ae0_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x0601E2`), Channel 6 context invariants (`A6 = 0x00FF05B2`, `D7 = 6`), BGM trigger parameter packet generation, muted channel skip, disabled channel buffer termination, and unobserved channel-enabled fail-closed fallback to ROM.
- `src/tools/hybrid/m68k_626bc_instruction_step.hpp/.cpp` — exact ROM-checked, dual-caller-gated instruction-stepped helper for Ancient Music Driver Sound Script Bytecode Lookahead & Loop Marker Detector (`0x0626BC..0x062722`), verified in dual canonical 3,600-frame replay (Run A: 2,970 ROM executions; Run B: 2,970 native executions, 0 fallbacks, 11,880 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_626bc_instruction_step_test.cpp` — unit test suite verifying caller gating (both `0x061ABA` and `0x062544`), immediate 0xFD sequence termination (`bset #1, 0(a6)`, `a0 += 1`), non-script skip, F4 sub-command lookahead patterns, and 7 RTS exits.
- `src/tools/hybrid/m68k_0027fc_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for VDP DMA Queue Record Consumer & Direct Hardware Committer (`0x0027FC..0x002898`), verified in dual canonical 3,600-frame replay (Run A: 2,806 ROM executions; Run B: 2,806 native executions, 0 fallbacks, 123,464 stepped instructions, 2,806 DMA transfers committed, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0027fc_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x002156` and `0x002678`), Z80 BUSREQ assert/grant/release protocol, Mode 2 DMA enable/disable shadow toggles, VDP register programming sequence, initial word port write, and stack frame restoration at RTS.
- `src/tools/hybrid/m68k_002992_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Mega Drive Hardware Gamepad Reader & Debouncer (`0x002992..0x002AE2`), verified in dual canonical 3,600-frame replay (Run A: 2,714 ROM executions; Run B: 2,714 native executions, 0 fallbacks, 279,482 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_002992_step_ops.hpp/.cpp` — instruction-stepping primitives for `0x002992`, covering Z80 BUSREQ assert/release, NOP timing delays, dynamic ASL timing (+28 master cycles), TH pin toggles, edge-triggered button state debouncing, and 6-button/3-button detection paths.
- `tests/m68k_002992_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x002188`), helper op logic (NOP, ASL dynamic cycles, ADDQ, ANDI), 6-button path, 3-button path, invalid handshake fallback path, and stack preservation at RTS.
- `src/tools/hybrid/m68k_6138e_instruction_step.hpp/.cpp` — exact ROM-checked, dual-caller-gated instruction-stepped helper for Sound Channel Envelope Parameter Staging & Modulation Init (`0x06138E..0x0613B2`), verified in dual canonical 3,600-frame replay (Run A: 2,609 ROM executions; Run B: 2,609 native executions, 0 fallbacks, 18,263 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_6138e_instruction_step_test.cpp` — unit test suite verifying caller gating (both `0x061A12` and `0x0624A0`), Musashi-exact memory access ordering (`fetch` -> `read` -> `fetch` -> `write`), dynamic shift cycle attribution (+14 master cycles on `lsr.w #1, d6`), channel envelope parameter copy/staging, and handoff to native RTS.
- `src/tools/hybrid/m68k_00e682_instruction_step.hpp/.cpp` — exact ROM-checked, 6-caller-gated instruction-stepped helper for Entity Action/Animation Freeze Timer & Damage Stun Gatekeeper (`0x00E682..0x00E6BA`), verified in dual canonical 3,600-frame replay (Run A: 2,104 ROM executions; Run B: 2,104 native executions, 0 fallbacks, 10,859 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00e682_instruction_step_test.cpp` — unit test suite verifying caller gating (6 callers: `0x0057D0`, `0x01A54E`, `0x01BF84`, `0x01BBF8`, `0x01C4DC`, `0x01B7F2`), normal path 1, freeze timer countdown path 2a, freeze timer active caller stack bypass (`addq.l #4, sp` + RTS to grandparent) path 2b, stun hurt flash path 3a/3b, and fail-closed fallback.
- `src/tools/hybrid/m68k_0613f8_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Sound Channel Dual Auxiliary Voice / Pitch-Modulation Slot Initializer (`0x0613F8..0x06147E`), verified in dual canonical 3,600-frame replay (Run A: 1,687 ROM executions; Run B: 1,687 native executions, 0 fallbacks, 13,496 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0613f8_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x061A16`), MOVEM save/restore cycle timing (168 master cycles each), post-increment dummy read, Slot 1 / Slot 2 activation, default pitch path, table lookup pitch path, and stack preservation at RTS.
- `src/tools/hybrid/m68k_016ebc_instruction_step.hpp/.cpp` — exact ROM-checked, dual-caller-gated instruction-stepped helper for Interactive Entity / World Object Update & Initialization Handler (`0x016EBC..0x016F0E`), verified in dual canonical 3,600-frame replay (Run A: 1,310 ROM executions; Run B: 1,310 native executions, 0 fallbacks, 5,260 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_016ebc_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x00DF1A`, `0x00FF88`), memory-to-memory move displacement ordering (`fetch` -> `read` -> `fetch` -> `write`), initialization path (`0x9E == 0`, `0x9E == 1`, `0x9E == 2`), update idle path (`0x66 == 0`), trigger path fallback, and stack preservation at RTS.
- `src/tools/hybrid/m68k_009ad6_instruction_step.hpp/.cpp` — exact ROM-checked, 8-caller-gated instruction-stepped helper for Terrain Ground Height Calculation & Slope Elevation Interpolation (`0x009AD6..0x009BF2`), verified in dual canonical 3,600-frame replay (Run A: 1,283 ROM executions; Run B: 1,283 native executions, 0 fallbacks, 31,624 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_009ad6_step_ops.hpp` — register push/pop, CCR flag calculation, and memory helper functions for `0x009AD6` instruction-stepped execution.
- `src/tools/hybrid/m68k_009ad6_step_slope.hpp/.cpp` — terrain slope interpolation calculation routine (`0x009BA6..0x009BEA`) with bit-exact Musashi `MULU.W` dynamic cycle formula and `DIVU.W` iterative non-restoring division cycle loop.
- `tests/m68k_009ad6_instruction_step_test.cpp` — unit test suite verifying caller gating (8 callers), register preservation (`D0-D3, D5-D7, A0, A3-A7`), MOVEM save/restore bus cycles (168 master cycles each), flat terrain calculation, void handling, slope height interpolation arithmetic, iterative division cycles, and stack preservation at RTS.
- `src/tools/hybrid/m68k_009bf2_instruction_step.hpp/.cpp` — exact ROM-checked, 8-caller-gated instruction-stepped helper for Entity Bounding-Box Terrain Grid Footprint Bitwise OR/AND Aggregator (`0x009BF2..0x009C40`), verified in dual canonical 3,600-frame replay (Run A: 1,283 ROM executions; Run B: 1,283 native executions, 0 fallbacks, 124,041 stepped instructions, 18,816 inner cell reads, 4,817 outer row steps, bit-identical canonical six-stream hashes across all 3,600 frames).
- `tests/m68k_009bf2_instruction_step_test.cpp` — unit test suite verifying caller gating (8 callers: `0x0090E8`, `0x00918A`, `0x0091D4`, `0x009270`, `0x009312`, `0x00935C`, `0x0098D4`, `0x009D26`), 2D cell footprint OR/AND aggregation, upper-word preservation, 16-bit predecrement `movem.l` writes and post-increment `movem.l` 32-bit reads + 16-bit dummy read, dynamic shift/DBF cycle timing, boundary coordinates, and fail-closed fallback.
- `src/tools/hybrid/m68k_0027a6_instruction_step.hpp/.cpp` — exact ROM-checked, dual-caller-gated instruction-stepped helper for Register-Parameterized Fixed-Buffer VDP Hardware DMA Committer (`0x0027A6..0x0027FC`), verified in dual canonical 3,600-frame replay (Run A: 1,171 ROM executions; Run B: 1,171 native executions, 0 fallbacks, 26,933 stepped instructions, 1,171 DMA transfers, bit-identical canonical six-stream hashes across all 3,600 frames).
- `tests/m68k_0027a6_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x001FC4` SAT DMA and `0x002252` CRAM DMA), Z80 BUSREQ assertion/wait/release protocol, VDP Mode 2 shadow bit-4 toggle, inline ROM source register table reads via `(A6)+` and `(A6)`, stack-staged DMA trigger write, first-word VDP Data Port rewrite (`(A5) -> $C00000`), and fail-closed fallback.
- `src/tools/hybrid/m68k_00b15e_instruction_step.hpp/.cpp` — exact ROM-checked, dual-caller-gated instruction-stepped helper for Entity Multi-Piece Sprite Compositor, VDP SAT Shadow Buffer Builder & Dynamic VRAM Tile DMA Request Stager (`0x00B15E..0x00B28C`), verified in dual canonical 3,600-frame replay (Run A: 1,165 ROM executions; Run B: 1,165 native executions, 0 fallbacks, 130,936 stepped instructions, 1,729 SAT records written, 396 DMA descriptors staged, 479 offscreen clipped pieces, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_00b15e_step_ops.hpp/.cpp` — instruction-stepping primitives, CCR status flag calculations, 5-register MOVEM save/restore bus cycles (280 master cycles each) with postincrement dummy read, and branch execution for `0x00B15E`.
- `src/tools/hybrid/m68k_00b15e_step_setup.cpp` — frame initialization and master entity table indexing phase (`0x00B15E..0x00B1DE`) for `0x00B15E`, covering entity orientation calculation, frame change detection, animation tick updates, dynamic shift cycle adjustments, and `bset #31, d5` timing (+14 master cycles).
- `src/tools/hybrid/m68k_00b15e_step_piece.cpp` — multi-piece loop, SAT entry generation, link indexing, and screen coordinate clipping phase (`0x00B1E0..0x00B23E`) for `0x00B15E`.
- `src/tools/hybrid/m68k_00b15e_step_dma.cpp` — dynamic VRAM tile DMA descriptor queue staging (`0x00FF10B4..`), per-frame tile budget decrement (`0x00FF1652`), and piece loop footer / epilogue phase (`0x00B240..0x00B288`) for `0x00B15E`.
- `tests/m68k_00b15e_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x00AB7A` and `0x00AC74`), multi-piece SAT shadow buffer construction, offscreen piece clipping, VRAM tile DMA descriptor staging, per-frame budget tracking, and fail-closed fallback to ROM.
- `src/tools/hybrid/m68k_00a8da_instruction_step.hpp/.cpp` — exact ROM-checked, 5-caller-gated instruction-stepped helper for Direct VDP Sprite Attribute Table Entry Appender & HUD Shadow Record Builder (`0x00A8DA..0x00A8F0`), verified in dual canonical 3,600-frame replay (Run A: 1,082 ROM executions; Run B: 1,082 native executions, 0 fallbacks, 9,738 stepped instructions, 1,082 SAT entries written, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00a8da_instruction_step_test.cpp` — unit test suite verifying caller gating (5 callers: `0x00A81E`, `0x00A82A`, `0x00A836`, `0x00A886`, `0x00A896`), SAT entry generation and word-stride writes, sprite count increment, 80-sprite limit check and termination branch, upper word register preservation, and fail-closed fallback to ROM.
- `src/tools/hybrid/m68k_017d12_instruction_step.hpp/.cpp` — exact ROM-checked, dual-caller-gated instruction-stepped helper for Collectible Dropped Item & Chest Entity Handler (`0x017D12..0x017E28`, Entity Type `0x1E` / 30), verified in dual canonical 3,600-frame replay (Run A: 985 ROM executions; Run B: 985 native executions, 0 fallbacks, 10,025 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_017d12_step_ops.hpp/.cpp` — instruction-stepping primitives, CCR status flag calculations, memory helpers, and child call delegation protocol (`step_jsr_abs` and `step_bsr_w`) under `namespace oasis::hybrid::step_017d12`.
- `src/tools/hybrid/m68k_017d12_step_init.cpp` — spawn initialization (`0x017D12..0x017D3C`), air falling / gravity integration, ground clamping, single bounce, and bounce sound dispatch (`0x017D3E..0x017D82`) for `0x017D12`.
- `src/tools/hybrid/m68k_017d12_step_update.cpp` — player contact detection, pickup state transition, sound dispatch, collection timer handling, inventory grant, persistent event registration, and chest open phase (`0x017D84..0x017E26`) for `0x017D12`.
- `tests/m68k_017d12_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x00DF1A` and `0x00FF88`), spawn initialization with bit 7 clear/set, idle update, collection timer countdown, in-air gravity velocity integration, chest open deactivation, contact sound dispatch with child yield, and inventory item collection.
- `src/tools/hybrid/m68k_6281a_step_tail.hpp/.cpp` — shared release phase & attenuation tail dispatch (`0x06281A..0x062866`) under `namespace oasis::hybrid::step_6281a`, reused by both `0x062788` and `0x06281A` to eliminate conflicting instruction ownership and duplicated logic.
- `src/tools/hybrid/m68k_6281a_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Sound Channel Note-Off / Release Phase Attenuation Committer (`0x06281A..0x06286C`, caller `0x06243C`), verified in dual canonical 3,600-frame replay (Run A: 985 ROM executions; Run B: 985 native executions, 0 fallbacks, 19,773 stepped instructions, 985 child calls to `0x06273A`, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_6281a_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x06243C`), invalid register rejection, normal release decrement, underflow clamp to 0, master volume multiplication, optional tremolo modulation delta application, attenuation inversion, child call to hardware attenuation committer `0x06273A`, envelope caching, and RTS completion.
- `src/tools/hybrid/m68k_6282e_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Sound Channel Attenuation Calculator & Hardware Committer Sub-Tail Entry (`0x06282E..0x06286C`, caller `0x0624C8`), verified in dual canonical 3,600-frame replay (Run A: 922 ROM executions; Run B: 922 native executions, 0 fallbacks, 11,986 stepped instructions, 922 child calls to `0x06273A`, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_6282e_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x0624C8`), invalid register rejection, master volume multiplication (factor 1..16), tremolo modulation branch, attenuation inversion (`eori.b #15, d0`), child call to hardware attenuation committer `0x06273A`, envelope caching, and RTS completion.
- `src/tools/hybrid/m68k_01ac90_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Entity Type 0x38 Lifecycle State Machine & Jump Table Dispatcher (`0x01AC90..0x01ACA2`, callers `0x00DF1A`, `0x00FD7A`, `0x00FDFE`), verified in dual canonical 3,600-frame replay (Run A: 914 ROM executions; Run B: 914 native executions, 0 fallbacks, 5,484 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_01ac90_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x00DF1A`, `0x00FD7A`, `0x00FDFE`), invalid register rejection, state index doubling, jump table offset indexing, jump dispatch across states, condition codes, and fail-closed rejection of bad callers, invalid SP, wrong entity type, and unsupported states.
- `src/tools/hybrid/m68k_01b102_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Entity Type 0x38 Damage & Hit Processing Handler (`0x01B102..0x01B1BE`, callers `0x01B7F8`, `0x01BBFE`, `0x01BF8A`, `0x01C4E2`), verified in dual canonical 3,600-frame replay (Run A: 890 ROM executions; Run B: 890 native executions, 0 fallbacks, 2,121 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_01b102_step_hit.hpp/.cpp` — instruction-stepping primitives, CCR status flag calculations, damage formula, recoil/death state transitions, direction table lookup (`0x01074A`), and sound/animation child call protocol under `namespace oasis::hybrid::step_01b102`.
- `tests/m68k_01b102_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x01B7F8`, `0x01BBFE`, `0x01BF8A`, `0x01C4E2`), no-hit fast path, non-fatal recoil path, fatal damage with death hook (`0x00E4D0`), SFX child call (`0x00E7E8`), and fail-closed rejection of unexpected callers, bad return PC, and invalid SP.
- `src/tools/hybrid/m68k_00350e_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Title Screen "PRESS START BUTTON" Blinking String Dispatcher (`0x00350E..0x003534`, caller `0x0033FE`), verified in dual canonical 3,600-frame replay (Run A: 866 ROM executions; Run B: 866 native executions, 0 fallbacks, 7,139 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00350e_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x0033FE`), visible string path (`0x00003534`), blank spaces path (`0x00003547`), D6 upper word preservation, CCR flags, and fail-closed rejection of wrong callers, invalid SP, and corrupt ROM.
- `src/tools/hybrid/m68k_008e22_instruction_step.hpp/.cpp` — exact ROM-checked, dual-caller-gated instruction-stepped helper for Hardware Palette Cycling & Animated CRAM Engine (`0x008E22..0x008E90`, callers `0x0007B8` and `0x008B3E`), verified in dual canonical 3,600-frame replay (Run A: 842 ROM executions; Run B: 842 native executions, 0 fallbacks, 47,465 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_008e22_step_cycle.hpp/.cpp` — instruction-stepping primitives, record loop traversal, frame countdown, keyframe advance, loop wrap, and CRAM shadow buffer write logic under `namespace oasis::hybrid::step_008e22`.
- `tests/m68k_008e22_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x0007B8` and `0x008B3E`), disabled fast exit, countdown without advance, keyframe advance with CRAM buffer update and DMA request flag, keyframe loop wrap, and fail-closed rejections.
- `src/tools/hybrid/m68k_00a7e6_instruction_step.hpp/.cpp` — exact ROM-checked, dual-caller-gated instruction-stepped helper for HUD Status Overlay Sprite Generator & VRAM DMA Transfer Queuer (`0x00A7E6..0x00A8DA`, callers `0x00A336` and `0x00A694`), verified in dual canonical 3,600-frame replay (Run A: 811 ROM executions; Run B: 811 native executions, 0 fallbacks, 12,017 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_00a7e6_step_mode_a.hpp/.cpp` — Mode A sprite attributes, tile offsets, child `0x00A8DA` BSR calls, and DMA transfer queueing logic under `namespace oasis::hybrid::step_00a7e6`.
- `src/tools/hybrid/m68k_00a7e6_step_mode_b.hpp/.cpp` — Mode B sprite attributes, tile offsets, child `0x00A8DA` BSR calls, and DMA transfer queueing logic under `namespace oasis::hybrid::step_00a7e6`.
- `tests/m68k_00a7e6_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x00A336` and `0x00A694`), disabled fast exit, mode A with and without DMA, mode B with DMA, child helper invocation, and fail-closed rejections.
- `src/tools/hybrid/m68k_008e90_instruction_step.hpp/.cpp` — exact ROM-checked, dual-caller-gated instruction-stepped helper for Active Entity Pool Scan & Master Physics/Animation Update Loop (`0x008E90..0x008EF6`, callers `0x008B42` and `0x00CD24`), verified in dual canonical 3,600-frame replay (Run A: 775 ROM executions; Run B: 775 native executions, 0 fallbacks, 430,427 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_008e90_step_ops.hpp/.cpp` — instruction-stepping primitives, CCR status flag calculations, sign-extended `movem.w` popping, dynamic shift cycles (`CYC_SHIFT`), and stack operations under `namespace oasis::hybrid::step_008e90`.
- `src/tools/hybrid/m68k_008e90_step_scan.hpp/.cpp` — triple-pool scan loops (`0xFF1770`, `0xFF19E8`, `0xFF2D8C`), active entity checks, and pool traversal logic under `namespace oasis::hybrid::step_008e90`.
- `src/tools/hybrid/m68k_008e90_step_anim.hpp/.cpp` — entity animation update, direction flags, and animation frame sequencing under `namespace oasis::hybrid::step_008e90`.
- `src/tools/hybrid/m68k_008e90_step_move_x.hpp/.cpp` — horizontal entity physics, velocity accumulation, terrain collision checks, and child call dispatch under `namespace oasis::hybrid::step_008e90`.
- `src/tools/hybrid/m68k_008e90_step_move_y.hpp/.cpp` — vertical entity physics, gravity accumulation, terrain collision checks, and child call dispatch under `namespace oasis::hybrid::step_008e90`.
- `src/tools/hybrid/m68k_008e90_step_move_alt.hpp/.cpp` — alternate movement path physics, collision responses, and coordinate updates under `namespace oasis::hybrid::step_008e90`.
- `tests/m68k_008e90_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x008B42`, `0x00CD24`), inactive entity pool scanning, child helper dispatch, fail-closed rejections, and register preservation.
- `src/tools/hybrid/m68k_00f788_instruction_step.hpp/.cpp` — exact ROM-checked, dual-caller-gated instruction-stepped helper for Event Script & Cutscene / Camera Scroll Sequencer (`0x00F788..0x00F920`, callers `0x008B2A` and `0x00CD02`), verified in dual canonical 3,600-frame replay (Run A: 775 ROM executions; Run B: 775 native executions, 0 fallbacks, 11,189 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_00f788_step_ops.hpp/.cpp` — instruction-stepping primitives, absolute memory moves, long moves, byte logic flags, swap/neg, and stack operations under `namespace oasis::hybrid::step_00f788`.
- `src/tools/hybrid/m68k_00f788_step_seq.hpp/.cpp` — event script inhibit checks, countdown timer decrements, camera pan direction dispatch, script sequencer byte loop, and early child exit handling under `namespace oasis::hybrid::step_00f788`.
- `tests/m68k_00f788_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x008B2A`, `0x00CD02`), inhibit path, timer path, script command loop, early child exits, and fail-closed rejections.
- `src/tools/hybrid/m68k_00a196_instruction_step.hpp/.cpp` — exact ROM-checked, dual-caller-gated instruction-stepped helper for Sprite Table Committer & Multi-Pool Display Dispatcher (`0x00A196..0x00A33E`, callers `0x008B86` and `0x00CD28`), verified in dual canonical 3,600-frame replay (Run A: 773 ROM executions; Run B: 773 native executions, 0 fallbacks, 882,033 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_00a196_step_ops.hpp/.cpp` — instruction-stepping primitives, bit manipulation (`bset` with upper-word cycle scaling), indexing, stack push/pop, and RTS primitives under `namespace oasis::hybrid::step_00a196`.
- `src/tools/hybrid/m68k_00a196_step_seq.hpp/.cpp` — multi-pool scan loops ($FF2954, $FF19C8, $FF19E8, $FF2ABC, $FF2D8C), indirect jump dispatch, HUD generator triggers, SAT finalization, and DMA staging under `namespace oasis::hybrid::step_00a196`.
- `tests/m68k_00a196_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x008B86`, `0x00CD28`), base sprite building, HUD triggers, SAT finalization, child return handling, and fail-closed rejections.
- `src/tools/hybrid/m68k_00a342_instruction_step.hpp/.cpp` — exact ROM-checked, dual-caller-gated instruction-stepped helper for Base Sprite Builder & VDP DMA Queue Stager (`0x00A342..0x00A436`, callers `0x00A19C` and `0x00A6A0`), verified in dual canonical 3,600-frame replay (Run A: 773 ROM executions; Run B: 773 native executions, 0 fallbacks, 55,011 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_00a342_step_ops.hpp/.cpp` — instruction-stepping primitives, shadow SAT pointer displacement, sprite attribute generation, DBF loop stepping, and VDP DMA queue staging under `namespace oasis::hybrid::step_00a342`.
- `src/tools/hybrid/m68k_00a342_step_seq.hpp/.cpp` — sprite table template selection (tables 0x00A438 and 0x00A480), sprite linking loops, cursor sprite generation, and DMA queue staging under `namespace oasis::hybrid::step_00a342`.
- `tests/m68k_00a342_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x00A19C`, `0x00A6A0`), sprite table building, DBF loop iteration, DMA queue staging, and fail-closed rejections.
- `src/tools/hybrid/m68k_00b34c_instruction_step.hpp/.cpp` — exact ROM-checked, dual-caller-gated instruction-stepped helper for Entity Display Mode & Visual Effects Dispatcher (`0x00B34C..0x00B3B0`, callers `0x00AD42` and `0x00B012`), verified in dual canonical 3,600-frame replay (Run A: 773 ROM executions; Run B: 773 native executions, 0 fallbacks, 12,368 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_00b34c_step_ops.hpp/.cpp` — instruction-stepping primitives, absolute byte testing, memory bit test & clear (`bclr #2, 57(%fp)`), and RTS under `namespace oasis::hybrid::step_00b34c`.
- `src/tools/hybrid/m68k_00b34c_step_seq.hpp/.cpp` — global pause/freeze check (`$FF1927`), entity display mode flag bit-testing (`$39(%fp)`), and child delegation (`0x00B3D2`, `0x00B56E`, `0x00B45C`, `0x00B4FE`, `0x00B610`) under `namespace oasis::hybrid::step_00b34c`.
- `tests/m68k_00b34c_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x00AD42`, `0x00B012`), freeze early exit, primary child dispatch, background child dispatch, overlay child dispatch with memory bit clear, and fail-closed rejections.
- `src/tools/hybrid/m68k_00c980_instruction_step.hpp/.cpp` — exact ROM-checked, multi-caller-gated instruction-stepped helper for Per-Frame Map Script Vector 0 Dispatcher (`0x00C980..0x00C996`, callers `0x008B60`, `0x00CD20`, and `0x006A3E`), verified in dual canonical 3,600-frame replay (Run A: 773 ROM executions; Run B: 773 native executions, 0 fallbacks, 3,805 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_00c980_step_ops.hpp/.cpp` — instruction-stepping primitives, absolute bit testing (`btst #1, $FF17B8`), address register loading (`movea.l $FF16EC, %a1`), indexed addition (`adda.w (%a1), %a1`), indirect tail jumping (`jmp (%a1)`), and RTS under `namespace oasis::hybrid::step_00c980`.
- `tests/m68k_00c980_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x008B60`, `0x00CD20`, `0x006A3E`), corrupt ROM rejection, script inhibit early exit, vector 0 dispatch path, and interruption/reentrancy protection.
- `src/tools/hybrid/m68k_00c998_instruction_step.hpp/.cpp` — exact ROM-checked, single-caller-gated instruction-stepped helper for Per-Frame Map Script Vector 1 Dispatcher (`0x00C998..0x00C9B0`, caller `0x008B52`), verified in dual canonical 3,600-frame replay (Run A: 772 ROM executions; Run B: 772 native executions, 0 fallbacks, 4,542 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_00c998_step_ops.hpp/.cpp` — instruction-stepping primitives, address register quick-addition (`addq.l #2, %a1`) under `namespace oasis::hybrid::step_00c998`.
- `tests/m68k_00c998_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x008B52`), corrupt ROM rejection, script inhibit early exit, vector 1 dispatch path, and interruption/reentrancy protection.
- `src/tools/hybrid/m68k_008dd4_instruction_step.hpp/.cpp` — exact ROM-checked, single-caller-gated instruction-stepped helper for HUD / Screen Spirit / Special Counter Step Handler (`0x008DD4..0x008E20`, caller `0x008B46`), verified in dual canonical 3,600-frame replay (Run A: 772 ROM executions; Run B: 772 native executions, 0 fallbacks, 4,632 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_008dd4_step_ops.hpp/.cpp` — instruction-stepping primitives, absolute word testing (`tst.w $FF1AA4`) and immediate comparison (`cmpi.w #$0014, $FF199E`) under `namespace oasis::hybrid::step_008dd4`.
- `tests/m68k_008dd4_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x008B46`), corrupt ROM rejection, active increment path, inactive zero-exit path, and interruption/reentrancy protection.
- `src/tools/hybrid/m68k_00557a_instruction_step.hpp/.cpp` — exact ROM-checked, single-caller-gated instruction-stepped helper for Player Per-Frame State, Footstep Audio & Shadow Coordinator (`0x00557A..0x00566E`, caller `0x008B2E`), verified in dual canonical 3,600-frame replay (Run A: 772 ROM executions; Run B: 772 native executions, 0 fallbacks, 24,968 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_00557a_step_ops.hpp/.cpp` — instruction-stepping primitives, absolute flag clear (`sf $FF1986`), displacement register operations, bit testing (`btst #0, 23(%a6)`), register swapping (`exg %d1, %d2`), immediate comparison, arithmetic, sound event dispatch, and shadow coordinate updates under `namespace oasis::hybrid::step_00557a`.
- `src/tools/hybrid/m68k_00557a_step_block1.hpp/.cpp` — block 1 instruction dispatch sequence for `0x00557A..0x0055DC`.
- `src/tools/hybrid/m68k_00557a_step_block2.hpp/.cpp` — block 2 instruction dispatch sequence for `0x0055E2..0x00566E`.
- `tests/m68k_00557a_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x008B2E`), corrupt ROM rejection, shadow skip path, shadow update path, and interruption/reentrancy protection.
