# M15.258 Safe Computational Islands, Debug Harness Repair, and Measurable Native Speedup

- `tests/m15_258_computational_islands_safety_test.cpp` — dedicated safety gate test suite validating disqualification of `0x00D406` subregions and top Musashi hotspots (12/12 passing under ASan/UBSan).
- `tools/dump_checkpoint_manifest.py` — tool for inspecting and verifying all 37 checkpoints across the 6 gameplay scenarios.
- `tools/benchmark_m15_258_worker.py` / `tools/benchmark_m15_258_real.py` / `data/thor-evidence/benchmark_m15_258_measured_summary.json` — 180-process real OS process wall-clock benchmark harness and evidence report.
- `tools/generate_m15_258_receipt.py` / `data/thor-evidence/combined-clusters-qualification-m15-258-20261111.json` — certified qualification receipt verified by `ReceiptVerifier` (0 rejections).
- `docs/handoffs/STANDALONE_M15_258_HANDOFF.md` — comprehensive handoff document detailing debug harness triage, computational island discovery, and qualification evidence.

# M15.257 Heavy-Kernel Forensics, Low-Overhead CPU Profiling, and Controlled ROM Recompilation

- `src/tools/hybrid/host_cpu_profiler.hpp` — calibrated invariant TSC profiler hooks with kernel-specific attribution (`0x00C8AA`, `0x00D406`).
- `tools/verify_m15_257_phase0.py` — Phase 0 baseline reproduction script verifying SHA256 hashes, production source hash across 535 files, and baseline fixtures.
- `tools/analyze_m15_257_heavy_kernels.py` — automated disassembly and static inspection tool evaluating instruction bounds, memory access regions, VDP/DMA coupling, and Phase 4 Safety Gate compliance.
- `tools/run_m15_257_host_cpu_profile.py` / `data/thor-evidence/m15_257_host_cpu_profile_summary.json` — 6-scenario TSC profiling harness and summary proving reduction of profiler overhead from +27.33% to -0.73%.
- `tests/m15_257_heavy_kernel_safety_test.cpp` — dedicated safety gate test suite validating quarantine of `0x00C8AA` and `0x00D406`, boundary checks, and hardware trap handling (11/11 passing under ASan/UBSan).
- `tools/run_m15_257_qualification.py` / `data/thor-evidence/m15_257_qualification_summary.json` — 13-configuration 6-scenario gameplay replay qualification harness and summary (78 sessions, 624/624 streams bit-exact, 37 checkpoints bit-exact).
- `tools/benchmark_m15_257_worker.py` / `tools/benchmark_m15_257_real.py` / `data/thor-evidence/benchmark_m15_257_measured_summary.json` — 180-process real OS process wall-clock benchmark harness and evidence report.
- `tools/generate_m15_257_receipt.py` / `data/thor-evidence/combined-clusters-qualification-m15-257-20261111.json` — certified qualification receipt verified by `ReceiptVerifier` (0 rejections).
- `docs/handoffs/STANDALONE_M15_257_HEAVY_KERNEL_HANDOFF.md` — comprehensive handoff document detailing heavy-kernel forensics, Phase 4 Safety Gate quarantine rationale, and roadmap to milestone M15.258.

# M15.256 Host CPU Profiling, Musashi Hotspot Attribution, and Profile-Guided Portable Coverage Expansion

- `src/tools/hybrid/host_cpu_profiler.hpp` — high-resolution host CPU execution time profiler instrumenting 9 categories of engine workload.
- `tools/run_m15_256_host_cpu_profile.py` / `data/thor-evidence/m15_256_host_cpu_profile_summary.json` — 6-scenario host CPU profiling runner and evidence summary.
- `tools/thor_recompiler/decoder_ext_m15_256.py` / `tools/thor_recompiler/insn_lowering_m15_256.py` / `tools/thor_recompiler/cpp_emitter_m15_256.py` — compiler extensions for M15.256 instructions (`ADDA.W (A0)+, A1` pattern decoding, IR lowering, partitioned C++20 emission).
- `tools/generate_entity_dispatch_m15_256.py` — compilation driver emitting Thor-IR, source map, IR manifest, and partitioned headers for 5 candidate leaf routines.
- `src/tools/thor_ir/m15_256_callee_ir.json` / `src/tools/thor_ir/m15_256_source_map.json` / `data/thor-fixtures/m15_256_ir_manifest.json` — Thor-IR document, 1:1 PC source map, and manifest (73 insns, 34 blocks).
- `src/tools/thor_ir/generated_m15_256/` — 11 partitioned C++20 headers implementing the 5 hotpath leaf routines (all $\le 260$ lines).
- `src/tools/thor_ir/exp256_dispatch_hybrid_adapter.hpp` — hybrid dispatch bridge adapting the M15.256 routines with held helper slot 214 preservation.
- `tools/generate_m15_256_fixtures.py` / `data/thor-fixtures/m15_256_callee_fixtures.hpp` / `data/thor-fixtures/m15_256_callee_fixtures.json` — 146 pinned GPGX reference fixtures (5 new for M15.256).
- `tests/thor_ir_m15_256_diff_test.cpp` — 3-way differential test (Musashi vs Mode A vs Mode B) and 12 mutation safety contracts (146/146 passing 100% under ASan/UBSan).
- `tools/run_m15_256_qualification.py` / `data/thor-evidence/m15_256_qualification_summary.json` — 12-configuration qualification runner and summary (72 sessions, 576 streams bit-exact, 31 checkpoints bit-exact).
- `tools/benchmark_m15_256_real.py` / `data/thor-evidence/benchmark_m15_256_measured_summary.json` — 10-trial measured OS process wall-clock benchmark across RUN_G, RUN_I, RUN_J, RUN_K, RUN_L.
- `tools/generate_m15_256_receipt.py` / `data/thor-evidence/combined-clusters-qualification-m15-256-20261111.json` — certified qualification receipt verified by `ReceiptVerifier` (0 rejections).
- `docs/handoffs/STANDALONE_M15_256_HOTSPOT_HANDOFF.md` — comprehensive handoff document detailing host profiling breakdown, unowned candidate landscape, and roadmap to milestone M15.257.

# M15.255 Virtual Bus Bottleneck Forensics, Assert-Enabled Regression Recovery, and Safe Direct-RAM Access Experiment

- `src/tools/thor_ir/exp250_direct_ram_adapter.hpp` — direct Work RAM fast-path bus adapter (`HybridM68kDirectRamBus250` / `step_m68k_portable_m15250_direct_ram`) with byte-interleaving awareness and fail-closed fallback.
- `tests/m15_255_direct_ram_differential_test.cpp` — dedicated direct-RAM differential, mirror, boundary wrap, fail-closed, and microbenchmark test suite (4/4 passing under ASan/UBSan).
- `tools/run_m15_255_direct_ram_ab.py` / `data/thor-evidence/m15_255_direct_ram_ab_summary.json` — 11-configuration qualification runner and dynamic memory profiling evidence summary across 6 scenarios.
- `tools/benchmark_m15_255_real.py` / `data/thor-evidence/benchmark_m15_255_measured_summary.json` — 10-trial measured OS process benchmark report across RUN_G, RUN_I, RUN_J, RUN_K.
- `tools/generate_m15_255_receipt.py` / `data/thor-evidence/combined-clusters-qualification-m15-255-20261111.json` — certified qualification receipt verified by `ReceiptVerifier` (0 rejections).
- `docs/handoffs/STANDALONE_M15_255_DIRECT_RAM_HANDOFF.md` — standalone track handoff for direct Work RAM access, memory layout, and bottleneck findings.

# M15.254 Real Benchmark Evidence, CTest Regression Triage, and Controlled Multi-Block Fusion Prototype

- `tests/m15_254_multiblock_adversarial_test.cpp` — dedicated adversarial multi-block fusion safety test suite (6/6 passing under ASan/UBSan).
- `tools/benchmark_m15_254_real.py` / `data/thor-evidence/benchmark_m15_254_measured_summary.json` — multi-trial measured OS benchmark report and dispatch attribution forensics across 6 scenarios.
- `tools/run_m15_254_multiblock_ab.py` / `data/thor-evidence/m15_254_multiblock_ab_summary.json` — 10-configuration qualification runner and evidence summary across 6 scenarios.
- `tools/generate_m15_254_receipt.py` / `data/thor-evidence/combined-clusters-qualification-m15-254-20261111.json` — certified qualification receipt verified by `ReceiptVerifier` (0 rejections).
- `docs/handoffs/STANDALONE_M15_254_MULTIBLOCK_FUSION_HANDOFF.md` — standalone track handoff for multi-block fusion and native memory optimization.

# M15.253 Fused Execution Performance Proof, Scheduler Boundary Validation, and Full Regression Recovery

- `tests/m15_253_adversarial_timeslice_test.cpp` — dedicated adversarial timeslice and interrupt boundary test suite (6/6 passing under ASan/UBSan).
- `tools/benchmark_m15_253_integrated.py` / `data/thor-evidence/benchmark_m15_253_integrated_report.json` — multi-trial integrated performance report and dispatch attribution forensics across 6 scenarios.
- `tools/generate_m15_253_receipt.py` / `data/thor-evidence/combined-clusters-qualification-m15-253-20261111.json` — certified qualification receipt verified by `ReceiptVerifier` (0 rejections).
- `docs/handoffs/STANDALONE_M15_253_FUSED_PERFORMANCE_HANDOFF.md` — standalone track handoff for bounded basic-block and routine-level fusion.

# M15.252 Bounded Basic-Block Fusion, Dispatch Amortization, and Measured Gameplay Speedup

- `src/tools/thor_ir/exp250_fused_hybrid_adapter.hpp` — bounded basic-block execution adapter (`HybridM68kFusedBus250` / `step_m68k_portable_m15250_fused`) implementing `slice_yield_` scheduler boundary yielding.
- `tools/run_m15_252_fused_ab.py` — nine-configuration (`RUN_A`..`RUN_I`) six-scenario qualification harness.
- `data/thor-evidence/m15_252_fused_ab_summary.json` — qualification evidence report confirming 100% bit-exact stream matching across 432 stream files and 31 checkpoints.
- `tools/benchmark_m15_252.py` / `data/thor-evidence/benchmark_m15_252_report.json` — microbenchmark and dispatch amortization attribution (1.40x isolated speedup, -58.82% dispatch entries).
- `tools/generate_m15_252_receipt.py` / `data/thor-evidence/combined-clusters-qualification-m15-252-20261111.json` — certified qualification receipt verified by `ReceiptVerifier` (0 rejections).
- `docs/handoffs/STANDALONE_M15_252_BOUNDED_BASIC_BLOCK_FUSION_HANDOFF.md` — standalone track handoff for basic-block and routine-level fusion.

# M15.251 Real Gameplay Speedup, Native Coverage Accounting, and Hybrid Dispatch Cost Forensics

- `docs/handoffs/STANDALONE_M15_251_DISPATCH_FORENSICS_HANDOFF.md` — standalone track handoff detailing verified ROM findings, calling conventions, and dispatch overhead forensics.

# M15.250 Residual CPU Bottleneck Elimination, Connected Hotpath Expansion, and Integrated Gameplay Performance

- `tools/audit_m15_250_phase1.py` / `data/thor-evidence/native_coverage_accounting_m15_250_phase1.json` — Phase 1 baseline coverage accounting across 6 scenarios (712,783 baseline steps).
- `data/thor-evidence/residual_top30_hotspot_manifest_m15_250.json` — residual hotspot profile and unowned candidate ranking.
- `tools/thor_recompiler/decoder_ext_m15_250.py` / `tools/thor_recompiler/insn_lowering_m15_250.py` / `tools/thor_recompiler/cpp_emitter_m15_250.py` — compiler extensions for M15.250 instructions (exact DBRA cycle adjustment, BSET register cycle calibration, block dispatch fusion).
- `tools/generate_entity_dispatch_m15_250.py` — compiler driver generating IR, source map, and partitioned C++20 headers.
- `src/tools/thor_ir/m15_250_callee_ir.json` / `src/tools/thor_ir/m15_250_source_map.json` — portable Thor IR document and 1:1 PC source map (181 insns, 71 blocks, 175 edges).
- `src/tools/thor_ir/generated_m15_250/` — 18 partitioned C++20 headers implementing the 8 hotpath routines (all <= 260 lines).
- `src/tools/thor_ir/exp250_dispatch_hybrid_adapter.hpp` — hybrid bridge adapting the M15.250 routines with held helper slot 214 preservation.
- `tools/generate_m15_250_fixtures.py` / `data/thor-fixtures/m15_250_callee_fixtures.hpp` / `data/thor-fixtures/m15_250_callee_fixtures.json` — 141 pinned GPGX reference fixtures (18 authentic + 123 controlled).
- `tools/audit_m15_250_cfg_coverage.py` / `data/thor-evidence/cfg_edge_coverage_manifest_m15_250.json` — CFG edge coverage manifest reaching 133/151 internal edges (88.1%) and 150/151 combined (99.3%).
- `tests/thor_ir_m15_250_diff_test.cpp` — 3-way differential test (Musashi vs Mode A vs Mode B) + 12 mutation tests.
- `tools/build_m15_250_runner.py` / `tools/run_m15_250_portable_ab.py` — build script and eight-configuration runtime qualification harness (`RUN_A..H`).
- `data/thor-evidence/m15_250_ab_summary.json` — summary of 8-config qualification across all 6 scenarios (219,573 new steps, 100% bit-exact).
- `tools/audit_m15_250_accounting.py` / `data/thor-evidence/native_coverage_accounting_m15_250.json` — canonical single-owner coverage accounting V5 (932,356 cumulative portable steps).
- `tools/benchmark_m15_250.py` / `data/thor-evidence/benchmark_m15_250_report.json` — isolated microbenchmark comparing Mode A vs Mode B (1.39x speedup).
- `tools/generate_m15_250_receipt.py` / `data/thor-evidence/combined-clusters-qualification-m15-250-20261110.json` — certified qualification receipt verified by `ReceiptVerifier`.
- `docs/handoffs/STANDALONE_M15_250_CAMERA_AND_AI_SUBSTATES_HANDOFF.md` — standalone track handoff for the camera tracker, priority sort, enemy AI substates, and dungeon stream routines.

# M15.249 Dynamic Displacement Scale-Up, Hardware-Polling Boundaries, and Connected Callee Recompilation

- `tools/audit_m15_249_phase1_2.py` — audit tool for Phase 1 inventory and Phase 2 baseline accounting.
- `data/thor-evidence/m15_249_phase1_inventory.json` — inventory audit covering all 15 M15.248 roots.
- `data/thor-evidence/native_coverage_accounting_m15_249_phase2.json` — baseline canonical accounting for 691,019 cumulative portable steps.
- `tools/build_m15_249_residual_manifest.py` / `data/thor-evidence/residual_top30_hotspot_manifest_m15_249.json` — residual hotspot profile and unowned candidate ranking.
- `tools/audit_hardware_polling_boundaries_m15_249.py` / `data/thor-evidence/hardware_polling_boundary_audit_m15_249.json` — formal audit of hardware polling loop boundaries (`$FF164D`).
- `tools/thor_recompiler/decoder_ext_m15_249.py` / `tools/thor_recompiler/insn_lowering_m15_249.py` / `tools/thor_recompiler/cpp_emitter_m15_249.py` — compiler extensions for M15.249 instructions (`pc_index`, `disp_ax`, cycle calibration).
- `tools/generate_entity_dispatch_m15_249.py` — compiler driver generating IR, source map, and partitioned C++20 headers.
- `src/tools/thor_ir/m15_249_callee_ir.json` / `src/tools/thor_ir/m15_249_source_map.json` — portable Thor IR document and 1:1 PC source map.
- `src/tools/thor_ir/generated_m15_249/` — 12 partitioned C++20 headers implementing the 7 hotpath routines (all <= 260 lines).
- `src/tools/thor_ir/exp249_dispatch_hybrid_adapter.hpp` — hybrid bridge adapting the M15.249 routines with polling loop quarantine.
- `tools/generate_m15_249_fixtures.py` / `data/thor-fixtures/m15_249_callee_fixtures.hpp` / `data/thor-fixtures/m15_249_callee_fixtures.json` — 130 pinned GPGX reference fixtures.
- `tools/audit_m15_249_cfg_coverage.py` / `data/thor-evidence/cfg_edge_coverage_manifest_m15_249.json` — CFG edge coverage manifest reaching 133/151 internal edges (88.1%) and 150/151 combined (99.3%).
- `tests/thor_ir_m15_249_diff_test.cpp` — 3-way differential test (Musashi vs Mode A vs Mode B) + 12 mutation tests.
- `tools/build_m15_249_runner.py` / `tools/run_m15_249_portable_ab.py` — build script and seven-configuration runtime qualification harness (`RUN_A..G`).
- `data/thor-evidence/m15_249_ab_summary.json` — summary of 7-config qualification across all 6 scenarios.
- `data/thor-evidence/native_coverage_accounting_m15_249.json` — canonical single-owner coverage accounting V4 (712,783 cumulative portable steps).
- `tools/benchmark_m15_249.py` / `data/thor-evidence/benchmark_m15_249_report.json` — isolated microbenchmark comparing Mode A vs Mode B (1.21x speedup).
- `tools/generate_m15_249_receipt.py` / `data/thor-evidence/combined-clusters-qualification-m15-249-20261110.json` — certified qualification receipt verified by `ReceiptVerifier`.
- `docs/handoffs/STANDALONE_M15_249_HOTPATH_HANDOFF.md` — standalone track handoff for the Master Enemy Controller and spatial math routines.

# M15.248 Profile-Guided Hotpath Recompilation & Cycle-Exact Hybrid Expansion

- `data/thor-evidence/m15_248_hotspot_profile_manifest.json` — profile manifest across all 6 scenarios.
- `data/thor-evidence/ranked_callee_candidates_m15_248.json` — unowned candidate ranking and selection table.
- `data/thor-evidence/longrun_diversity_report_m15_248.json` — long-run execution diversity report.
- `tools/generate_m15_248_combat_journal.py` — authentic gameplay combat journal generator for `longrun_combat_1800f`.
- `data/thor-evidence/longrun_combat_capture_manifest_m15_248.json` — capture manifest for the 1,800-frame combat session.
- `src/tools/thor_ir/m15_248_callee_ir.json` — portable Thor IR document for the 15 selected hotpath routines.
- `src/tools/thor_ir/m15_248_source_map.json` — 1:1 mapping from original M68000 instruction PCs to basic blocks and generated C++ headers.
- `src/tools/thor_ir/generated_m15_248/` — 19 auto-split C++20 headers implementing the 15 routines (all <= 260 lines, <= 350 limit).
- `src/tools/thor_ir/exp248_dispatch_hybrid_adapter.hpp` — hybrid bridge adapting the M15.248 routines with DMA polling quarantine.
- `tools/thor_recompiler/insn_lowering_m15_248.py` / `tools/thor_recompiler/cpp_emitter_m15_248.py` — instruction lowering and C++ code generator for M15.248.
- `tools/generate_entity_dispatch_m15_248.py` — compiler driver generating IR, CFG, source map, and C++ headers.
- `data/thor-fixtures/m15_248_callee_fixtures.hpp` / `data/thor-fixtures/m15_248_callee_fixtures.json` — 112 pinned GPGX reference fixtures.
- `data/thor-evidence/cfg_edge_coverage_manifest_m15_248.json` — CFG edge coverage manifest reaching 123/151 internal edges (81.5%).
- `tests/thor_ir_m15_248_diff_test.cpp` — 3-way differential test (Musashi vs Mode A vs Mode B) + 12 mutation tests.
- `tools/run_m15_248_portable_ab.py` — six-configuration runtime qualification harness (`RUN_A..F`).
- `data/thor-evidence/m15_248_ab_summary.json` — summary of 6-config qualification across all 6 scenarios.
- `tools/audit_m15_248_accounting.py` / `data/thor-evidence/native_coverage_accounting_m15_248.json` — canonical single-owner coverage accounting V3.
- `tools/benchmark_m15_248.py` / `data/thor-evidence/benchmark_m15_248_report.json` — isolated microbenchmark comparing Mode A vs Mode B.
- `docs/handoffs/STANDALONE_M15_248_TOWN_SUB_SYSTEM_HANDOFF.md` — standalone track handoff for the hotpath and town subsystem.
- `tools/generate_m15_248_receipt.py` / `data/thor-evidence/combined-clusters-qualification-m15-248-20261110.json` — certified qualification receipt verified by `ReceiptVerifier`.

# M15.247 Authentic CFG Edge Coverage & Town Action Handlers Recompilation

- `data/thor-evidence/cfg_edge_coverage_manifest_m15_247.json` — complete inventory and edge classification of all 183 CFG edges across M15.246.
- `data/thor-fixtures/m15_247_callee_fixtures.hpp` / `data/thor-fixtures/m15_247_callee_fixtures.json` — 70 pinned GPGX reference fixtures reaching 104/151 internal edges (68.9% >= 100 target).
- `tests/thor_ir_m15_247_diff_test.cpp` — 3-way differential test (GPGX vs Mode A vs Mode B) + 12 negative and mutation test contracts in Debug & Release.
- `tests/m68k_semantic_stress_test.cpp` — standalone semantic stress test exercising 19 M68000 instruction classes with zero failures.
- `data/thor-evidence/longrun_diversity_report_m15_247.json` — 300-frame execution diversity audit over all 12 windows of the 3,600f journal.
- `tools/audit_coverage_and_displacement_m15_247.py` — canonical single-owner coverage accounting V2 script with 4 negative mutation tests.
- `data/thor-evidence/native_coverage_accounting_m15_247.json` — single-owner coverage accounting evidence across C, A, B, longrun_3600f, and longrun_alt_1800f.
- `tools/audit_m15_247_candidates.py` — candidate hotspot profiler and unowned candidate ranking.
- `data/thor-evidence/ranked_callee_candidates_m15_247.json` — candidate classification and selection manifest.
- `src/tools/thor_ir/m15_247_callee_ir.json` / `data/thor-fixtures/m15_247_ir_manifest.json` — portable Thor IR document for the 17 Town entity action handlers.
- `data/thor-evidence/callee_cfg_source_map_m15_247.json` — 1:1 mapping from original M68000 instruction PCs to basic blocks and generated C++ headers.
- `src/tools/thor_ir/generated_m15_247/` — 14 auto-split C++20 headers implementing the 17 entity handlers (all <= 274 lines, <= 350 limit).
- `src/tools/thor_ir/exp247_dispatch_hybrid_adapter.hpp` — hybrid bridge adapting the 17 action handlers into `oasis_gameplay_session`.
- `tools/generate_entity_dispatch_m15_247.py` — compiler driver generating IR, CFG, source map, and C++ headers.
- `tools/thor_recompiler/insn_lowering_m15_247.py` / `tools/thor_recompiler/cpp_emitter_m15_247.py` — instruction lowering and C++ code generator for M15.247.
- `tools/generate_m15_247_longrun_journal.py` — journal generator capturing Town/Village gameplay (`longrun_alt_1800f`).
- `data/thor-evidence/longrun_alt_1800f_capture_manifest_m15_247.json` — capture manifest with initial state, input polls, and video/audio hashes.
- `tools/run_m15_247_portable_ab.py` — five-configuration runtime qualification harness (`RUN_A..E`).
- `build/ab_portable_m15_247/m15_247_ab_summary.json` — summary of 5-config qualification across all 5 scenarios.
- `tools/benchmark_m15_247.py` / `data/thor-evidence/benchmark_m15_247_report.json` — isolated microbenchmark comparing Mode A vs Mode B.
- `docs/handoffs/STANDALONE_M15_247_TOWN_SUB_SYSTEM_HANDOFF.md` — standalone track handoff for the Town entity action handlers subsystem.
- `tools/generate_m15_247_receipt.py` / `data/thor-evidence/combined-clusters-qualification-m15-247-20261110.json` — certified qualification receipt verified by `ReceiptVerifier`.

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
  layer switcher ("PROPERTY VIEW" vs "EVIDENCE / RECOMPILATION VIEW"),
  evidence category mapping, runtime heat overlay, and deep inspection;
  `rom_coverage_model.py` validates checkpoint/overlay identities and computes
  byte-accurate cell states, coverage summaries, and filters.
- `tools/bizhawk-native-ring/evidence_overlay_model.py` provides read-only visual
  categorization, runtime heat intensity calculation, RAW_HOT_RANK, and RECOMP_CANDIDATE_RANK
  filtering out active native, verified, and non-code routines.
- `tools/bizhawk-native-ring/evidence_inspector_panel.py` provides the read-only
  Evidence Inspector panel formatting 18 verified fields, cell drawing with heat overlay,
  and dynamic legend generation.
- `tools/bizhawk-native-ring/evidence_knowledge_bridge.py` provides the read-only bridge
  joining knowledge.sqlite, machine receipts, canonical census, and C++ steppers.
- `tools/bizhawk-native-ring/generate_verification_receipts.py` generates the machine-readable
  verification receipt index with canonical timestamps and stream hashes.
- `tests/rom_coverage_model_test.py` covers checkpoint integrity, overlay
  pairing, exact cell partitioning, overlapping properties, coverage deltas,
  and property filters.
- `tests/evidence_knowledge_bridge_test.py`, `tests/evidence_overlay_model_test.py`,
  and `tests/rom_coverage_gui_evidence_test.py` cover machine receipt verification,
  activation provenance, candidate ranking, layer switching, heat overlay, and inspector UI.
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
-src/tools/hybrid/m68k_code_map.hpp/.cpp — execution-driven accumulative M68000
  instruction code map, coverage tracker, and ASM/JSON exporter.
- `src/tools/hybrid/m68k_code_formatter.hpp/.cpp` — complete, modular Motorola 68000
  instruction formatter with operand completeness, standard mnemonics, and assembler roundtrip.
- `tests/m68k_code_map_test.cpp` — unit verification for instruction observation,
  coverage calculation, and artifact emission.
- `tests/m68k_code_map_asm_test.cpp` — C++ regression verification for M68000 assembly
  operand formatting and branch target resolution.
- `tools/smoke_code_map_ab.py` — single-variable A/B smoke verification verifying
  zero behavioral divergence and bit-for-bit equivalence with trace enabled.
- `tools/run_attract_code_map.py` — full attract_3600 replay execution for ROM code
  reconstruction and performance profiling.
- `tools/merge_code_maps.py` — idempotent multi-scenario accumulative merge engine,
  bytecode consistency validator, and global coverage generator.
- `tools/run_game_coverage_campaign.py` — multi-scenario replay campaign orchestrator
  covering attract_3600, canonical_3600, beach_6000, menu_3600, fileselect_3600.
- `tools/verify_asm_roundtrip_classes.py` — independent M68000 reverse assembly verification
  across 15 instruction classes using GNU as, distinguishing exact matches from unverified.
- `tools/map_native_coverage.py` — native helper coverage mapper correlating global executed
  code maps with ownership indices and verification receipts.
- `tools/analyze_hot_ranges.py` — gameplay subsystem hot code range auditor analyzing
  callers, children, RAM accesses, and hardware dependencies.
- `tools/test_roundtrip_samples.py` — GNU as roundtrip assembler verification across
  distinct M68000 instruction mnemonics.
- `tools/verify_all_3708_quality.py` — automated batch ASM quality verification across
  executed instruction sets.
- `tools/audit_steppers.py` — systematic instruction stepper audit tool and semantic validation gate validating opcode consumption, operand fetch parity, and contract invariants across all 185 native helpers.
- `docs/reports/00638c-m15-187g-stepper-repair-20261103.json` — machine-readable verification receipt, 0x00638C repair data, 0 proven defects remaining across 185 helpers, 1,592-frame three-way continuation parity, and M15.188 readiness for M15.187G.
- `docs/reports/00e398-m15-187f-stepper-repair-20261103.json` — machine-readable verification receipt, 0x00E398 repair data, 1,592-frame checkpoint parity, and full 185-baseline 0x00663E reachability for M15.187F.
- `docs/reports/01ad14-m15-187e-stepper-correctness-audit-20261103.json` — machine-readable stepper correctness audit, 0x01AD14 repair receipt, 185-helper flaw census, and 668-frame continuation parity data for M15.187E.
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
- `src/tools/hybrid/m68k_005670_instruction_step.hpp/.cpp` — exact ROM-checked, single-caller-gated instruction-stepped helper for Player Pre-Action Input, Timers & Action Dispatcher (`0x005670..0x0059B8` + `0x005B3C..0x005B9E`, caller `0x005586`), verified in dual canonical 3,600-frame replay (Run A: 772 ROM executions; Run B: 772 native executions, 0 fallbacks, 47,655 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_005670_step_ops.hpp`, `src/tools/hybrid/m68k_005670_step_ops1.cpp`, `src/tools/hybrid/m68k_005670_step_ops2.cpp` — instruction-stepping primitives, input/timer evaluations, table lookups, arithmetic, and hardware bus refresh cycles under `namespace oasis::hybrid::step_005670`.
- `src/tools/hybrid/m68k_005670_step_block1.hpp/.cpp` — block 1 instruction dispatch sequence for `0x005670..0x0057D0`.
- `src/tools/hybrid/m68k_005670_step_block2.hpp/.cpp` — block 2 instruction dispatch sequence for `0x0057D6..0x0059B8`.
- `src/tools/hybrid/m68k_005670_step_block3.hpp/.cpp` — block 3 instruction dispatch sequence for `0x005B3C..0x005B9E`.
- `tests/m68k_005670_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x005586`), corrupt ROM rejection, odd SP rejection, action dispatch path, and interruption/reentrancy protection.
- `src/tools/hybrid/m68k_008b8e_instruction_step.hpp/.cpp` — exact ROM-checked, single-caller-gated instruction-stepped helper for Per-Frame State Gate & Mode Transition Dispatcher (`0x008B8E..0x008BAC`, caller `0x008B26`), verified in dual canonical 3,600-frame replay (Run A: 772 ROM executions; Run B: 772 native executions, 0 fallbacks, 2,316 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_008b8e_step_ops.hpp/.cpp` — instruction-stepping primitives, absolute bit clear (`bclr #7, $00FF1864`) and mode tail branches under `namespace oasis::hybrid::step_008b8e`.
- `tests/m68k_008b8e_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x008B26`), corrupt ROM rejection, odd SP rejection, zero-exit path, bra 0x530c path, bra 0x7cc6 path, and interruption/reentrancy handling.
- `src/tools/hybrid/m68k_008bac_instruction_step.hpp/.cpp` — exact ROM-checked, single-caller-gated instruction-stepped helper for Player Menu, Pause & Inventory Coordinator (`0x008BAC..0x008C5A`, caller `0x008B22`), verified in dual canonical 3,600-frame replay (Run A: 772 ROM executions; Run B: 772 native executions, 0 fallbacks, 17,262 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_008bac_step_ops.hpp/.cpp` — instruction-stepping primitives, controller state reads, inhibit checks, and menu jump handlers under `namespace oasis::hybrid::step_008bac`.
- `tests/m68k_008bac_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x008B22`), corrupt ROM rejection, odd SP rejection, clean RTS path, inhibit RTS path, and menu jump path.
- `src/tools/hybrid/m68k_00deec_instruction_step.hpp/.cpp` — exact ROM-checked, single-caller-gated instruction-stepped helper for Master Active Entity Tick & Isometric Projection Loop (`0x00DEEC..0x00DF52`, caller `0x008B32`), verified in dual canonical 3,600-frame replay (Run A: 772 ROM executions; Run B: 772 native executions, 0 fallbacks, 161,708 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_00deec_step_ops.hpp/.cpp` — instruction-stepping primitives, entity loop state evaluation, movem stack operations, isometric velocity projection, and dbf cycle adjustments under `namespace oasis::hybrid::step_00deec`.
- `tests/m68k_00deec_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x008B32`), corrupt ROM rejection, odd SP rejection, inhibit early exit path, full entity loop path, and reentrancy/state handling.
- `src/tools/hybrid/m68k_00e0ba_instruction_step.hpp/.cpp` — exact ROM-checked, single-caller-gated instruction-stepped helper for Secondary / Auxiliary Entity Slot Dispatch Loop (`0x00E0BA..0x00E0F4`, caller `0x008B36`), verified in dual canonical 3,600-frame replay (Run A: 772 ROM executions; Run B: 772 native executions, 0 fallbacks, 21,608 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_00e0ba_step_ops.hpp/.cpp` — instruction-stepping primitives, auxiliary entity slot active evaluation, movem stack operations, and dbf cycle adjustments under `namespace oasis::hybrid::step_00e0ba`.
- `tests/m68k_00e0ba_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x008B36`), corrupt ROM rejection, odd SP rejection, inhibit early exit path, empty slots loop path, and active child slot dispatch path.
- `src/tools/hybrid/m68k_00e0f4_instruction_step.hpp/.cpp` — exact ROM-checked, single-caller-gated instruction-stepped helper for Tertiary / Interactive Entity Slot Dispatch & Table Jump Loop (`0x00E0F4..0x00E140`, caller `0x008B3A`), verified in dual canonical 3,600-frame replay (Run A: 772 ROM executions; Run B: 772 native executions, 0 fallbacks, 32,010 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_00e0f4_step_ops.hpp/.cpp` — instruction-stepping primitives, countdown timer decrement, tertiary entity slot evaluation, jump table offset indexing, movem stack operations, and dbf cycle adjustments under `namespace oasis::hybrid::step_00e0f4`.
- `tests/m68k_00e0f4_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x008B3A`), corrupt ROM rejection, odd SP rejection, timer countdown & inhibit early exit path, empty slots loop path, and active child slot dispatch path.
- `src/tools/hybrid/m68k_00b3b2_instruction_step.hpp/.cpp` — exact ROM-checked, single-caller-gated instruction-stepped helper for Player / Entity Pause Freeze Gate & Flag Dispatch Subroutine (`0x00B3B2..0x00B3D2`, caller `0x00AD4E`), verified in dual canonical 3,600-frame replay (Run A: 759 ROM executions; Run B: 759 native executions, 0 fallbacks, 4,554 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_00b3b2_step_ops.hpp/.cpp` — instruction-stepping primitives, global pause test, entity attribute inspection, and child routine dispatch under `namespace oasis::hybrid::step_00b3b2`.
- `tests/m68k_00b3b2_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x00AD4E`), corrupt ROM rejection, odd SP rejection, pause exit path, bit 1 clear exit path, and bit 1 set child bsr path.
- `src/tools/hybrid/m68k_camera_scroll_step_ops.hpp/.cpp` and `m68k_camera_scroll_step_ops_alu.cpp` — modular instruction-stepping primitives, branch adjustments, ALU operations, shifts, and memory reads/writes for the Camera Viewport Scroll subsystem under `namespace oasis::hybrid::step_camera`.
- `src/tools/hybrid/m68k_00ceea_instruction_step.hpp/.cpp` and `m68k_00ceea_line_buffer.hpp/.cpp` — instruction-stepped native helper for Camera Viewport Boundary Clamping & VDP Scroll Line Buffer Pipeline (`0x00CEEA..0x00D0C4`, callers `0x02E604`, `0x02E6F6`, `0x02E87C`).
- `src/tools/hybrid/m68k_00d188_instruction_step.hpp/.cpp` — instruction-stepped native helper for Camera Sub-pixel Velocity & Fractional Position Accumulator (`0x00D188..0x00D266`, caller `0x00D0BC`).
- `src/tools/hybrid/m68k_00d344_instruction_step.hpp/.cpp` — instruction-stepped native helper for Camera VDP Scroll Plane Table Committal & Inter-frame Delta Step (`0x00D344..0x00D398`, caller `0x00D2B4`).
- `src/tools/hybrid/m68k_00d398_instruction_step.hpp/.cpp` — instruction-stepped native helper for Camera Plane A/B Offset Sync & VDP Queue Trigger (`0x00D398..0x00D404`, callers `0x02E60A`, `0x02E6FC`, `0x02E882`).
- `tests/m68k_camera_scroll_instruction_step_test.cpp` — unit test suite verifying all 4 camera scroll routines, caller gatings, ROM checks, and state transitions.
- `src/tools/hybrid/m68k_terrain_collision_step_ops.hpp/.cpp` and `m68k_terrain_collision_step_ops_alu.cpp` — modular instruction-stepping primitives, coordinate delta math, elevation lookups, bit operations with hardware writeback cycle calibration, and bounds checking for the Terrain Heightmap Collision subsystem under `namespace oasis::hybrid::step_terrain`.
- `src/tools/hybrid/m68k_009d4e_instruction_step.hpp/.cpp` — instruction-stepped native helper for Direction 0 (Right) Terrain Elevation Delta Step (`0x009D4E..0x009D70`).
- `src/tools/hybrid/m68k_009d70_instruction_step.hpp/.cpp` — instruction-stepped native helper for Direction 1 (Left) Terrain Elevation Delta Step (`0x009D70..0x009D92`).
- `src/tools/hybrid/m68k_009d92_instruction_step.hpp/.cpp` — instruction-stepped native helper for Direction 2 (Down) Terrain Elevation Delta Step (`0x009D92..0x009DB4`).
- `src/tools/hybrid/m68k_009db4_instruction_step.hpp/.cpp` — instruction-stepped native helper for Direction 3 (Up) Terrain Elevation Delta Step (`0x009DB4..0x009DD6`).
- `src/tools/hybrid/m68k_009dd6_instruction_step.hpp/.cpp` — instruction-stepped native helper for Direction 0 (Right) Entity Bounding Box Collision Loop (`0x009DD6..0x009EBA`).
- `src/tools/hybrid/m68k_009eba_instruction_step.hpp/.cpp` — instruction-stepped native helper for Direction 1 (Left) Entity Bounding Box Collision Loop (`0x009EBA..0x009FA0`).
- `src/tools/hybrid/m68k_009fa0_instruction_step.hpp/.cpp` — instruction-stepped native helper for Direction 2 (Down) Entity Bounding Box Collision Loop (`0x009FA0..0x00A09A`).
- `src/tools/hybrid/m68k_00a09a_instruction_step.hpp/.cpp` — instruction-stepped native helper for Direction 3 (Up) Entity Bounding Box Collision Loop (`0x00A09A..0x00A196`).
- `tests/m68k_terrain_collision_instruction_step_test.cpp` — comprehensive unit test suite covering all 8 terrain heightmap collision routines, coordinate delta math, bit 31 set/clear branches, and boundary loops.
- `src/tools/hybrid/m68k_player_weapon_step_ops.hpp/.cpp` and `m68k_player_weapon_step_ops_alu.cpp` — modular instruction-stepping primitives, ALU operations, MOVEM 168-cycle stack operations, and dynamic shift timing for the Player Weapon Sprite Assembly subsystem under `namespace oasis::hybrid::step_player_weapon`.
- `src/tools/hybrid/m68k_00b3d2_instruction_step.hpp/.cpp` — instruction-stepped native helper for Player Primary Weapon Sprite Piece Layout Step (`0x00B3D2..0x00B42C`, caller `0x00B3CA`).
- `src/tools/hybrid/m68k_00b56e_instruction_step.hpp/.cpp` — instruction-stepped native helper for Player Secondary Weapon / Bomb Sprite Piece Step (`0x00B56E..0x00B5C8`, caller `0x00B3CE`).
- `src/tools/hybrid/m68k_00b6a6_instruction_step.hpp/.cpp` — instruction-stepped native helper for Weapon Sprite SAT Attribute Assembly & Screen Clipping Dispatch (`0x00B6A6..0x00B730`, callers `0x00B422`, `0x00B5BE`).
- `tests/m68k_player_weapon_instruction_step_test.cpp` — unit test suite covering all 3 player weapon sprite routines, negative-piece terminal exits, zero-frame exits, and sprite emission.
- `src/tools/hybrid/m68k_00c9b2_step_ops.hpp/.cpp` — instruction-stepping primitives, absolute byte testing (`tst.b ($FFxxxx)`), and register updates under `namespace oasis::hybrid::step_00c9b2`.
- `src/tools/hybrid/m68k_00c9b2_instruction_step.hpp/.cpp` — exact ROM-checked, multi-caller-gated instruction-stepped helper for Room Transition Script Abort & Mode Gatekeeper (`0x00C9B2..0x00C9EC`, callers `0x02E5B0`, `0x02E64E`, `0x02E774`), verified in dual canonical 3,600-frame replay (Run A: 742 ROM executions; Run B: 742 native executions, 0 fallbacks, 9,646 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00c9b2_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x02E5B0`, `0x02E64E`, `0x02E774`), corrupt ROM rejection, all 6 flag abort return branches (`ADDQ.L #4, SP; RTS`), normal exit path (`RTS`), and interrupt/reentrancy protection.
- `data/thor-evidence/c9b2-replay-differential-20261018.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00C9B2` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_00c1ce_step_ops.hpp/.cpp` — instruction-stepping primitives, 8-register MOVEM stack push/pop, dynamic shift timing, player position subtraction, and CCR bit manipulation under `namespace oasis::hybrid::step_00c1ce`.
- `src/tools/hybrid/m68k_00c1ce_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Entity Proximity AABB Intersect Collision Test (`0x00C1CE..0x00C222`, caller `0x02E796`), verified in dual canonical 3,600-frame replay (Run A: 463 ROM executions; Run B: 463 native executions, 0 fallbacks, 13,427 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00c1ce_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x02E796`), corrupt ROM rejection, collision hit path (`ORI #1, CCR`, C=1), collision miss path (`ANDI #$FE, CCR`, C=0), and MOVEM register preservation.
- `data/thor-evidence/c1ce-replay-differential-20261018.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00C1CE` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_00e0b8_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Entity Null Subroutine & Idle Slot Return Stub (`0x00E0B8..0x00E0BA`, callers `0x00DF1A`, `0x00FF88`), completing the native `ENTITY_LIFECYCLE` dispatch group and eliminating inter-domain exits from `0x00DEEC`, verified in dual canonical 3,600-frame replay (Run A: 464 ROM executions; Run B: 464 native executions, 0 fallbacks, 464 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00e0b8_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x00DF1A`, `0x00FF88`), corrupt ROM rejection, SP popping, return state prefetch alignment, and cycle accounting.
- `data/thor-evidence/e0b8-replay-differential-20261018.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00E0B8` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_00b856_step_ops.hpp/.cpp` — instruction-stepping primitives, 8-register MOVEM stack/memory push/pop, 10-cycle `lsr.l #1` shift timing compensation, bounding box subtraction/comparisons, and CCR bit manipulation under `namespace oasis::hybrid::step_00b856`.
- `src/tools/hybrid/m68k_00b856_instruction_step.hpp/.cpp` — exact ROM-checked, multi-entry instruction-stepped helper for Entity Overlap AABB Query Generator & Iteration Engine (`0x00B852..0x00B922`, entry `0x00B856`, prefix `0x00B852`, resume `0x00B912`, callers `0x00B7CA`, `0x00B848`, `0x008332`), verified in dual canonical 3,600-frame replay (Run A: 154 target entries, 41 resume entries; Run B: 154 native target calls, 41 resume calls, 195 total completed, 0 fallbacks, 39,325 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00b856_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x00B7CA`, `0x00B848`, `0x008332`), corrupt ROM rejection, AABB query hit path (`ANDI #$FE, CCR`), miss path (`ORI #1, CCR`), and resume iterator loop.
- `data/thor-evidence/b856-replay-differential-20261018.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00B856` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_00b79a_step_ops.hpp/.cpp` — instruction-stepping primitives, directional facing vector setup, displacement comparisons, child BSR invocation, and CCR arithmetic/logic manipulation under `namespace oasis::hybrid::step_00b79a`.
- `src/tools/hybrid/m68k_00b79a_instruction_step.hpp/.cpp` — exact ROM-checked, multi-caller-gated instruction-stepped helper for Player Combat Hitbox Directional Scan & Reaction Dispatcher (`0x00B79A..0x00B852`, callers `0x005D46`, `0x00E53C`), verified in dual canonical 3,600-frame replay (Run A: 139 ROM executions; Run B: 139 native executions, 0 fallbacks, 2,844 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00b79a_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x005D46`, `0x00E53C`), corrupt ROM rejection, direction-based search bounds adjustment, child BSR handling (`0x00B856`), and RTS exit.
- `data/thor-evidence/b79a-replay-differential-20261018.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00B79A` displacement across all 8 canonical streams.
- `data/thor-evidence/combat-hitbox-replay-differential-20261018.json` — composite machine-readable verification receipt proving bit-for-bit replay equivalence of the complete `PLAYER_ACTION_COMBAT_HITBOX` cluster (`0x00B79A` + `0x00B856`) across all 8 canonical streams.
- `src/tools/hybrid/m68k_061ef4_step_ops.hpp/.cpp` — instruction-stepping primitives, bit test, dynamic shift/rotate cycle compensation, memory operand reads, register updates, and CCR bit manipulation under `namespace oasis::hybrid::step_061ef4`.
- `src/tools/hybrid/m68k_061ef4_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for YM2612 4-Operator FM Envelope Parameter Formatter & Sound Queue Appender (`0x061EF4..0x061F56`, caller `0x061EE4`), verified in dual canonical 3,600-frame replay (Run A: 731 ROM calls; Run B: 731 native calls, 0 fallbacks, 34,155 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_061ef4_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x061EE4`), corrupt ROM rejection, early exit branch path, and 4-operator loop execution path.
- `src/tools/hybrid/m68k_061cd2_step_ops.hpp/.cpp` — instruction-stepping primitives, register/accumulator increment/decrement, DBF loop branch compensation, sound queue post-increment memory writes, and CCR arithmetic manipulation under `namespace oasis::hybrid::step_061cd2`.
- `src/tools/hybrid/m68k_061cd2_instruction_step.hpp/.cpp` — exact ROM-checked, multi-caller-gated instruction-stepped helper for YM2612 4-Operator Register Index Sequence Step & Queue Allocator (`0x061CD2..0x061CEE`, callers `0x061D28`, `0x061DB6`, `0x06219A`), verified in dual canonical 3,600-frame replay (Run A: 121 ROM calls; Run B: 121 native calls, 0 fallbacks, 3,025 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_061cd2_instruction_step_test.cpp` — unit test suite verifying multi-caller gating (`0x061D28`, `0x061DB6`, `0x06219A`), corrupt ROM rejection, 4-operator register index step calculation, and sound queue write verification.
- `data/thor-evidence/audio-envelope-replay-differential-20261018.json` — composite machine-readable verification receipt proving bit-for-bit replay equivalence of the complete `YM2612_ENVELOPE_FORMATTER` cluster (`0x061EF4` + `0x061CD2`) across all 8 canonical streams.
- `src/tools/hybrid/m68k_01b580_step_ops.hpp/.cpp` — instruction-stepping primitives, dynamic shift cycle compensation (`lsr.w #1`), stack memory operations (`step_movem_to_stack_d0`), branch adjustments, and CCR flag manipulation under `namespace oasis::hybrid::step_01b580`.
- `src/tools/hybrid/m68k_01b580_instruction_step.hpp/.cpp` — exact ROM-checked, 7-caller-gated instruction-stepped helper for Entity-to-Player Front/Rear Dual-Sector Proximity & Detection Scanner (`0x01B580..0x01B5DA`, callers `0x01B558`, `0x01B89A`, `0x01B95C`, `0x01BC40`, `0x01BCBE`, `0x01BFD4`, `0x01C04A`), verified in dual canonical 3,600-frame replay (Run A: 406 ROM calls; Run B: 406 native calls, 0 fallbacks, 6,484 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_01b580_instruction_step_test.cpp` — unit test suite verifying 7-caller gating, corrupt ROM rejection, inhibit exit path, front detect hit path, and rear detect jump path.
- `data/thor-evidence/01b580-replay-differential-20261018.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x01B580` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_0064c4_step_ops.hpp/.cpp` — instruction-stepping primitives, dynamic arithmetic shift right cycle compensation (`asr.l #1`), facing direction bitmasking, and CCR arithmetic manipulation under `namespace oasis::hybrid::step_0064c4`.
- `src/tools/hybrid/m68k_0064c4_instruction_step.hpp/.cpp` — exact ROM-checked, 5-caller-gated instruction-stepped helper for Player Isometric Diagonal & Surface Velocity Scaler (`0x0064C4..0x006516`, callers `0x0062FC`, `0x00653A`, `0x006692`, `0x006720`, `0x0067D0`), verified in dual canonical 3,600-frame replay (Run A: 269 ROM calls; Run B: 269 native calls, 0 fallbacks, 3,131 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0064c4_instruction_step_test.cpp` — unit test suite verifying 5-caller gating, corrupt ROM rejection, orthogonal facing preserve path, diagonal facing scale path, slowdown mode halving path, and running lock skip path.
- `data/thor-evidence/0064c4-replay-differential-20261018.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0064C4` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_010930_step_ops.hpp/.cpp` — instruction-stepping primitives, Z-delta height arithmetic, direction table indexing, and Carry flag manipulation under `namespace oasis::hybrid::step_010930`.
- `src/tools/hybrid/m68k_010930_instruction_step.hpp/.cpp` — exact ROM-checked, 4-entry-point instruction-stepped helper for Multi-Sector Player Bounding Box Hitbox & Z-Height Proximity Test (`0x010916..0x010A74`, entry points `0x010916`, `0x010920`, `0x010926`, `0x010930`), closing the native-to-ROM hole from `0x01B580`, verified in dual canonical 3,600-frame replay (Run A: 669 ROM calls; Run B: 669 native calls, 0 fallbacks, 8,232 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_010930_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, Z-reject miss exit, each cardinal direction hit and miss paths, and all 4 entry points.
- `data/thor-evidence/010930-replay-differential-20261018.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x010930` cluster displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_02e704_step_ops.hpp/.cpp` — instruction-stepping primitives, entity slot loop pointer math, elevation addition, and dbra loop timing under `namespace oasis::hybrid::step_02e704`.
- `src/tools/hybrid/m68k_02e704_instruction_step.hpp/.cpp` — exact ROM-checked, 2-caller-gated instruction-stepped helper for Entity Elevation & Shadow Layer Delta Adjustment Loop (`0x02E704..0x02E748`, callers `0x02E64A` and `0x02E770`), verified in dual canonical 3,600-frame replay (Run A: 653 ROM calls; Run B: 653 native calls, 0 fallbacks, 76,968 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_02e704_instruction_step_test.cpp` — unit test suite verifying disabled mode, unsupported caller rejection, corrupt ROM rejection, all inactive slots, active slot elevation update, and filter condition matrix.
- `data/thor-evidence/02e704-replay-differential-20261018.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x02E704` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_0178ea_step_ops.hpp/.cpp` — instruction-stepping primitives, entity substate testing, animation doubling, and vertical velocity kick under `namespace oasis::hybrid::step_0178ea`.
- `src/tools/hybrid/m68k_0178ea_instruction_step.hpp/.cpp` — exact ROM-checked, 2-caller-gated instruction-stepped helper for Entity Substate-0 First-Tick Initializer & Z-Velocity Kick (`0x0178EA..0x017908`, callers `0x00DF1A` and `0x00FF88`), verified in dual canonical 3,600-frame replay (Run A: 541 ROM calls; Run B: 541 native calls, 0 fallbacks, 1,633 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0178ea_instruction_step_test.cpp` — unit test suite verifying disabled mode, unsupported caller rejection, corrupt ROM rejection, substate skip path, and substate init paths for both callers.
- `data/thor-evidence/0178ea-replay-differential-20261018.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0178EA` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_00baac_step_ops.hpp/.cpp` — instruction-stepping primitives, entity 3D bounding box checks, flag tests, and cycle-accurate MOVEM.W dummy-read/timing operations under `namespace oasis::hybrid::step_00baac`.
- `src/tools/hybrid/m68k_00baac_instruction_step.hpp/.cpp` — exact ROM-checked, 9-caller-gated instruction-stepped helper for Entity Bounding Box Collision Check against Player (`0x00BAAC..0x00BBA8`), verified in dual canonical 3,600-frame replay (Run A: 428 ROM calls; Run B: 428 native calls, 0 fallbacks, 25,259 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00baac_instruction_step_test.cpp` — unit test suite verifying disabled mode, corrupt ROM rejection, unsupported caller rejection, hit player path, miss player path, and inactive player path.
- `data/thor-evidence/00baac-replay-differential-20261018.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00BAAC` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_00f91c_step_ops.hpp/.cpp` — instruction-stepping primitives, stack unwinding (`ADDQ.L #4, SP` without CCR alteration), RTS outer caller return, and metrics tracking under `namespace oasis::hybrid::step_00f91c`.
- `src/tools/hybrid/m68k_00f91c_instruction_step.hpp/.cpp` — exact ROM-checked instruction-stepped helper for Cutscene/Event Script Sequencer Command 00 — Script Terminate / Stack-Unwind Return (`0x00F91C..0x00F920`, caller `0x00F7C6`), verified in dual canonical 3,600-frame replay (Run A: 630 ROM calls; Run B: 632 native calls, 0 fallbacks, 1,264 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00f91c_instruction_step_test.cpp` — unit test suite verifying disabled mode, corrupt ROM rejection, unsupported caller rejection, outer return 0x008B2E path, outer return 0x00CD06 path, and bad outer return PC rejection.
- `data/thor-evidence/00f91c-replay-differential-20261018.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00F91C` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_0107aa_step_ops.hpp/.cpp` — instruction-stepping primitives, Z-height comparisons, cardinal jump-table dispatch, bounding box calculations, and CCR Carry set/clear operations under `namespace oasis::hybrid::step_0107aa`.
- `src/tools/hybrid/m68k_0107aa_instruction_step.hpp/.cpp` — exact ROM-checked, 8-caller-gated instruction-stepped helper for Spatial Detection Sector Hitbox & Range Scanner (`0x0107AA..0x010916`), verified in dual canonical 3,600-frame replay (Run A: 337 ROM calls; Run B: 337 native calls, 0 fallbacks, 5,772 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0107aa_instruction_step_test.cpp` — unit test suite verifying disabled mode, corrupt ROM rejection, unsupported caller rejection, Z reject path, Sector 0..3 hit and miss paths, and exact CCR preservation/modification.
- `data/thor-evidence/0107aa-replay-differential-20261018.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0107AA` displacement across all 8 canonical streams.
- `scratch/run_ab_0107aa.py` — automated dual-run A/B replay harness verifying 3,600-frame state equivalence and generating verification receipts for `0x0107AA`.
- `src/tools/hybrid/m68k_00e192_step_ops.hpp/.cpp` — instruction-stepping primitives, bounce math, weapon slot pointer logic, flicker manipulation, and cycle-accurate condition evaluation under `namespace oasis::hybrid::step_00e192`.
- `src/tools/hybrid/m68k_00e192_instruction_step.hpp/.cpp` — exact ROM-checked, 2-caller-gated instruction-stepped helper for Entity Secondary State Transition & Animation Sequencer (`0x00E192..0x00E20A`, loop caller `0x00E130` and indirect dispatcher `0x00DF1A`), verified in dual canonical 3,600-frame replay (Run A: 404 ROM calls; Run B: 468 native calls, 0 fallbacks, 5,543 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00e192_instruction_step_test.cpp` — unit test suite verifying disabled mode, corrupt ROM rejection, unsupported caller rejection, normal bounce active, bounce clamp to zero, weapon slot despawn, flicker phase, and timeout despawn paths.
- `data/thor-evidence/00e192-replay-differential-20261018.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00E192` displacement across all 8 canonical streams.
- `scratch/run_ab_00e192.py` — automated dual-run A/B replay harness verifying 3,600-frame state equivalence and generating verification receipts for `0x00E192`.
- `src/tools/hybrid/m68k_00e502_step_ops.hpp/.cpp` — instruction-stepping primitives, coordinate math, velocity integration, sector classification, and MOVEM push/pop cycle adjustments under `namespace oasis::hybrid::step_00e502`.
- `src/tools/hybrid/m68k_00e502_instruction_step.hpp/.cpp` — exact ROM-checked, 2-caller-gated instruction-stepped helper for Entity Coordinate Integration & Physics Position Step (`0x00E502..0x00E5AE`, callers `0x01B1E8` and `0x01A95A`), verified in dual canonical 3,600-frame replay (Run A: 329 ROM calls; Run B: 329 native calls, 0 fallbacks, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00e502_instruction_step_test.cpp` — unit test suite verifying disabled mode, corrupt ROM rejection, unsupported caller rejection, coordinate advancement, and MOVEM register preservation.
- `data/thor-evidence/00e502-replay-differential-20261018.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00E502` displacement across all 8 canonical streams.
- `scratch/run_ab_00e502.py` — automated dual-run A/B replay harness verifying 3,600-frame state equivalence and generating verification receipts for `0x00E502`.
- `src/tools/hybrid/m68k_00e5ae_step_ops.hpp/.cpp` — instruction-stepping primitives, camera coordinate loading, bit testing, signed screen projection math, margin bounding box calculations, and CCR Carry flag manipulation under `namespace oasis::hybrid::step_00e5ae`.
- `src/tools/hybrid/m68k_00e5ae_instruction_step.hpp/.cpp` — exact ROM-checked, 3-caller-gated instruction-stepped helper for Entity 3D Camera Viewport Frustum Culling & Visibility Test (`0x00E5AE..0x00E610`, callers `0x00EC02`, `0x01C1E4`, `0x01C5F0`), verified in dual canonical 3,600-frame replay (Run A: 215 ROM calls; Run B: 215 native calls, 0 fallbacks, 4,007 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00e5ae_instruction_step_test.cpp` — unit test suite verifying Plane 0 and Plane 1 camera selection, in-frustum visible return (Carry=0), left/top/right/bottom frustum culling (Carry=1), disabled mode, and corrupt ROM rejection.
- `data/thor-evidence/00e5ae-replay-differential-20261019.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00E5AE` displacement across all 8 canonical streams.
- `scratch/run_ab_00e5ae.py` — automated dual-run A/B replay harness verifying 3,600-frame state equivalence and generating verification receipts for `0x00E5AE`.
- `src/tools/hybrid/m68k_0096ba_step_ops.hpp/.cpp` — instruction-stepping primitives, coordinate math, tilemap address calculation, shift timing, and horizontal span collision accumulation loop under `namespace oasis::hybrid::step_0096ba`.
- `src/tools/hybrid/m68k_0096ba_instruction_step.hpp/.cpp` — exact ROM-checked, 2-caller-gated instruction-stepped helper for Terrain Tilemap Collision Span Evaluator (`0x0096BA..0x0096E8`, callers `0x0094F2` and `0x009680`), verified in dual canonical 3,600-frame replay (Run A: 226 ROM calls; Run B: 226 native calls, 0 fallbacks, 6,940 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0096ba_instruction_step_test.cpp` — unit test suite verifying multi-tile loop accumulation, 1-tile loop, boundary coordinates, corrupt ROM rejection, and disabled mode.
- `data/thor-evidence/0096ba-replay-differential-20261019.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0096BA` displacement across all 8 canonical streams.
- `scratch/run_ab_0096ba.py` — automated dual-run A/B replay harness verifying 3,600-frame state equivalence and generating verification receipts for `0x0096BA`.
- `src/tools/hybrid/m68k_0101ec_step_ops.hpp/.cpp` — instruction-stepping primitives, entity table scanning, active state predicate evaluation, and DBF loop timing under `namespace oasis::hybrid::step_0101ec`.
- `src/tools/hybrid/m68k_0101ec_instruction_step.hpp/.cpp` — exact ROM-checked, 4-caller-gated instruction-stepped helper for Active Entity Slot Counter (`0x0101EC..0x010214`, callers `0x00F9A6`, `0x00FCCE`, `0x00FD8C`, `0x017988`), verified in dual canonical 3,600-frame replay (Run A: 194 ROM calls; Run B: 194 native calls, 0 fallbacks, 16,557 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0101ec_instruction_step_test.cpp` — unit test suite verifying all 17 slots, zero active slots, flag 58 rejection, DBF paths, upper-word preservation, corrupt ROM rejection, and disabled mode.
- `data/thor-evidence/0101ec-replay-differential-20261019.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0101EC` displacement across all 8 canonical streams.
- `scratch/run_ab_0101ec.py` — automated dual-run A/B replay harness verifying 3,600-frame state equivalence and generating verification receipts for `0x0101EC`.
- `src/tools/hybrid/m68k_0094d2_step_ops.hpp/.cpp` — instruction-stepping primitives, 4-register MOVEM push/pop, child BSR invocation, elevation delta comparison, Work RAM height table lookup, and CCR Carry manipulation under `namespace oasis::hybrid::step_0094d2`.
- `src/tools/hybrid/m68k_0094d2_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Terrain Collision Coordinate Normalizer (`0x0094D2..0x0095AA`, caller `0x0092F0`), verified in dual canonical 3,600-frame replay (Run A: 194 ROM calls; Run B: 194 native calls, 0 fallbacks, 4,958 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0094d2_instruction_step_test.cpp` — unit test suite verifying disabled gate, same level free move, wall block (delta >= 2), elevation delta table lookup (delta <= -2), corrupt ROM rejection, and invalid child return handling.
- `data/thor-evidence/0094d2-replay-differential-20261019.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0094D2` displacement across all 8 canonical streams.
- `scratch/run_ab_0094d2.py` — automated dual-run A/B replay harness verifying 3,600-frame state equivalence and generating verification receipts for `0x0094D2`.
- `src/tools/hybrid/m68k_010a74_step_ops.hpp/.cpp` — instruction-stepping primitives, 4-way PC-relative jump table dispatch, Z-elevation comparison, 2D lateral span and reach test, and CCR Carry flag manipulation under `namespace oasis::hybrid::step_010a74`.
- `src/tools/hybrid/m68k_010a74_instruction_step.hpp/.cpp` — exact ROM-checked, 2-caller-gated instruction-stepped helper for Entity Line-of-Sight Range Test (`0x010A74..0x010BF0`, callers `0x01BC36`, `0x01BCB4`), verified in dual canonical 3,600-frame replay (Run A: 193 ROM calls; Run B: 193 native calls, 0 fallbacks, 965 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_010a74_instruction_step_test.cpp` — unit test suite verifying disabled mode, caller validation, player inactive miss, and hit paths for UP and DOWN cardinal directions.
- `data/thor-evidence/010a74-replay-differential-20261019.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x010A74` displacement across all 8 canonical streams.
- `scratch/run_ab_010a74.py` — automated dual-run A/B replay harness verifying 3,600-frame state equivalence and generating verification receipts for `0x010A74`.
- `src/tools/hybrid/m68k_01c1d6_step_ops.hpp/.cpp` — instruction-stepping primitives, bit test, child JSR calls, 160 px outer radius / 64 px inner deadzone threshold tests, child slot allocation, and CCR Carry manipulation under `namespace oasis::hybrid::step_01c1d6`.
- `src/tools/hybrid/m68k_01c1d6_instruction_step.hpp/.cpp` — exact ROM-checked, 3-caller-gated instruction-stepped helper for Entity Annulus Proximity Detection & Child Spawn Trigger (`0x01C1D6..0x01C23E`, callers `0x01BF1A`, `0x01BFCC`, `0x01C042`), verified in dual canonical 3,600-frame replay (Run A: 191 ROM calls; Run B: 191 native calls, 0 fallbacks, 1,552 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_01c1d6_instruction_step_test.cpp` — unit test suite verifying disabled mode, caller gating, bit 3 set miss, frustum culling, outer range miss, inner deadzone miss, hit path, and corrupt ROM rejection.
- `data/thor-evidence/01c1d6-replay-differential-20261019.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x01C1D6` displacement across all 8 canonical streams.
- `scratch/run_ab_01c1d6.py` — automated dual-run A/B replay harness verifying 3,600-frame state equivalence and generating verification receipts for `0x01C1D6`.
- `src/tools/hybrid/m68k_00e144_step_ops.hpp/.cpp` — instruction-stepping primitives, indexed LEA calculation from PC-relative table base 0x00E140, signed word table addition, and tail JMP (An) dispatch under `namespace oasis::hybrid::step_00e144`.
- `src/tools/hybrid/m68k_00e144_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Entity Movement Sub-Mode Dispatcher Stub (`0x00E144..0x00E150`, caller `0x00DF1A`), verified in dual canonical 3,600-frame replay (Run A: 106 ROM calls; Run B: 106 native calls, 0 fallbacks, 424 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00e144_instruction_step_test.cpp` — unit test suite verifying disabled mode, corrupt ROM, invalid caller gating, dispatch index 0, dispatch index 2, stack preservation, and metrics reporting.
- `data/thor-evidence/00e144-replay-differential-20261019.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00E144` displacement across all 8 canonical streams.
- `scratch/run_ab_00e144.py` — automated dual-run A/B replay harness verifying 3,600-frame state equivalence and generating verification receipts for `0x00E144`.
- `src/tools/hybrid/m68k_0083d4_step_ops.hpp/.cpp` — instruction-stepping primitives, displacement read from A6, byte subtraction, short branch, word branch, moveq, and RTS under `namespace oasis::hybrid::step_0083d4`.
- `src/tools/hybrid/m68k_0083d4_instruction_step.hpp/.cpp` — exact ROM-checked, 3-caller-gated instruction-stepped helper for Entity 4-Way Compass Direction Code Normalization Helper (`0x0083D4..0x0083F8`, callers `0x005B70`, `0x006346`, `0x006524`), verified in dual canonical 3,600-frame replay (Run A: 94 ROM calls; Run B: 94 native calls, 0 fallbacks, 476 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0083d4_instruction_step_test.cpp` — unit test suite verifying disabled mode, corrupt ROM, invalid caller gating, direction mappings 0->0, 1->3, 2->1, 3->2, and caller attribution.
- `data/thor-evidence/0083d4-replay-differential-20261019.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0083D4` displacement across all 8 canonical streams.
- `scratch/run_ab_0083d4.py` — automated dual-run A/B replay harness verifying 3,600-frame state equivalence and generating verification receipts for `0x0083D4`.
- `src/tools/hybrid/m68k_01a244_step_ops.hpp/.cpp` — instruction-stepping primitives, bit test on displacement from A6, conditional short branch, word displacement write from D0 and immediate to A6, immediate word addition to D0, and RTS under `namespace oasis::hybrid::step_01a244`.
- `src/tools/hybrid/m68k_01a244_instruction_step.hpp/.cpp` — exact ROM-checked, 6-caller-gated instruction-stepped helper for Entity Animation Facing Direction Slot & Frame Offset Dispatcher (`0x01A244..0x01A288`, callers `0x01A4E2`, `0x01A51C`, `0x01A5D2`, `0x01A606`, `0x01A96E`, `0x01A9DE`), verified in dual canonical 3,600-frame replay (Run A: 90 ROM calls; Run B: 90 native calls, 0 fallbacks, 717 stepped instructions, 100% path coverage, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_01a244_instruction_step_test.cpp` — unit test suite verifying disabled mode, corrupt ROM, invalid caller gating, all 4 orientation paths, and all 6 caller attributions.
- `data/thor-evidence/01a244-replay-differential-20261019.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x01A244` displacement across all 8 canonical streams.
- `scratch/run_ab_01a244.py` — automated dual-run A/B replay harness verifying 3,600-frame state equivalence and generating verification receipts for `0x01A244`.
- `src/tools/hybrid/m68k_010660_step_ops.hpp/.cpp` — instruction-stepping primitives, word register test, word conditional branch, immediate address load to A0, immediate long shift (112 master cycles), unsigned word division (with Musashi `calc_divu_cycles` and `skip_bus_refresh`), word register clear, post-increment word compare, word unsigned branch lower-or-same, immediate word addition to D0, short unconditional branch, and RTS under `namespace oasis::hybrid::step_010660`.
- `src/tools/hybrid/m68k_010660_instruction_step.hpp/.cpp` — exact ROM-checked, 4-caller-gated instruction-stepped helper for Vector Angle & Fixed-Point Tangent Octant Boundary Search Routine (`0x010660..0x010684`, parent caller `0x010594` via callsites `0x0105B8`, `0x0105DA`, `0x0105AE`, `0x0105CA`), verified in dual canonical 3,600-frame replay (Run A: 68 ROM calls; Run B: 68 native calls, 0 fallbacks, 2,124 stepped instructions, 446 table loop iterations, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_010660_instruction_step_test.cpp` — unit test suite verifying disabled mode, corrupt ROM, invalid caller gating, all 4 callers, D0=0 early exit, all 9 table buckets, exact 24 captured canonical call vectors, and DIVU cycle calculation (16 test cases).
- `data/thor-evidence/010660-replay-differential-20261019.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x010660` displacement across all 8 canonical streams.
- `scratch/run_ab_010660.py` — automated dual-run A/B replay harness verifying 3,600-frame state equivalence and generating verification receipts for `0x010660`.
- `src/tools/hybrid/m68k_010594_step_ops.hpp/.cpp` & `m68k_010594_step_math.cpp` — instruction-stepping primitives, coordinate difference calculations, quadrant sign evaluation, child BSR call invocation, angle quadrant transformation, ROM sine/cosine table lookup, and speed vector multiplications under `namespace oasis::hybrid::step_010594`.
- `src/tools/hybrid/m68k_010594_instruction_step.hpp/.cpp` — exact ROM-checked, 3-caller-gated instruction-stepped helper for Vector Angle & Velocity Component Calculator (`0x010594..0x010660`, callers `0x01B9A2`, `0x01BD04`, `0x01C090`), verified in dual canonical 3,600-frame replay (Run A: 68 ROM calls; Run B: 68 native calls, 0 fallbacks, 3,691 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_010594_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, re-entry protection, all 4 quadrants, child calls to `0x010660`, and memory write assertions (11 test cases).
- `data/thor-evidence/010594-replay-differential-20261019.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x010594` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_00e150_instruction_step.hpp/.cpp` — exact ROM-checked, 2-caller-gated instruction-stepped helper for Entity Airborne Gravity & Landing Handler (`0x00E150..0x00E192`, callers `0x00E130` and `0x00DF1A` via tail dispatcher `0x00E144`), verified in dual canonical 3,600-frame replay (Run A: 63 ROM calls; Run B: 105 native entries, 0 fallbacks, 1,041 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00e150_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, airborne path, ground landing clamp path, and sub-mode transitions (7 test cases).
- `data/thor-evidence/00e150-replay-differential-20261019.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00E150` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_00e25c_step_ops.hpp/.cpp` — instruction-stepping primitives, word subtraction with immediate 1 on entity lifetime counter, conditional branch, and entity slot clear under `namespace oasis::hybrid::step_00e25c`.
- `src/tools/hybrid/m68k_00e25c_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Entity Lifecycle Timer Decrement & Despawn Handler (`0x00E25C..0x00E268`, caller `0x00DF1A`), verified in dual canonical 3,600-frame replay (Run A: 40 ROM calls; Run B: 40 native calls, 0 fallbacks, 120 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00e25c_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, decrement non-zero branch path, and zero despawn path (5 test cases).
- `data/thor-evidence/00e25c-replay-differential-20261019.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00E25C` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_0094a2_step_ops.hpp/.cpp` — instruction-stepping primitives, coordinate word subtraction, bitwise AND mask, table pointer indexing, tile height comparison, and RTS under `namespace oasis::hybrid::step_0094a2`.
- `src/tools/hybrid/m68k_0094a2_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Vertical Terrain Span Evaluator (`0x0094A2..0x0094D2`, caller `0x00947A`), verified in dual canonical 3,600-frame replay (Run A: 66 ROM calls; Run B: 66 native calls, 0 fallbacks, 792 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0094a2_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, span calculation paths, and bounds checks (6 test cases).
- `data/thor-evidence/0094a2-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0094A2` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_009460_step_ops.hpp/.cpp` — instruction-stepping primitives, elevation scan loop, child BSR call invocation to native `0x0094A2`, step-delta accumulation, and RTS under `namespace oasis::hybrid::step_009460`.
- `src/tools/hybrid/m68k_009460_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Surface Elevation Scanner / Vertical Raycast (`0x009460..0x0094A2`, caller `0x0093A6`), verified in dual canonical 3,600-frame replay (Run A: 66 ROM calls; Run B: 66 native calls, 0 fallbacks, 528 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_009460_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, child invocation to native 0x0094A2, and scan loop bounds (7 test cases).
- `data/thor-evidence/009460-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x009460` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_00938e_step_ops.hpp/.cpp` — instruction-stepping primitives, MOVEM push/pop register bus timing, word branch not-taken adjustment, elevation delta comparison, collision obstacle flag bit setting, and RTS under `namespace oasis::hybrid::step_00938e`.
- `src/tools/hybrid/m68k_00938e_instruction_step.hpp/.cpp` — exact ROM-checked, 2-caller-gated instruction-stepped helper for Terrain Bounding Box Evaluator (`0x00938E..0x009460`, callers `0x0090C6` and `0x009168`), verified in dual canonical 3,600-frame replay (Run A: 66 ROM calls; Run B: 66 native calls, 0 fallbacks, 1,384 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00938e_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, obstacle detection paths, carry flag contract, and cycle timing (10 test cases).
- `data/thor-evidence/00938e-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00938E` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_0099d6_step_ops.hpp/.cpp` — instruction-stepping primitives, coordinate tile division, pitch shift exponent multiplication, linear tile offset computation, tile byte read from A1, and RTS under `namespace oasis::hybrid::step_0099d6`.
- `src/tools/hybrid/m68k_0099d6_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Map Tile Fetcher & Grid Coordinate Evaluator (`0x0099D6..0x0099F0`, caller `0x009932`), verified in dual canonical 3,600-frame replay (Run A: 30 ROM calls; Run B: 30 native calls, 0 fallbacks, 300 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0099d6_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, tile index calculation, and byte fetch (5 test cases).
- `data/thor-evidence/0099d6-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0099D6` displacement across all 8 canonical streams.

- `src/tools/hybrid/m68k_009678_step_ops.hpp/.cpp` — instruction-stepping primitives, half-width/depth probe calculations, child BSR call invocation to native `0x0096BA`, elevation obstruction normalization, and RTS under `namespace oasis::hybrid::step_009678`.
- `src/tools/hybrid/m68k_009678_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Surface Raycast Scanner (`0x009678..0x0096BA`, caller `0x0095C8`), verified in dual canonical 3,600-frame replay (Run A: 32 ROM calls; Run B: 32 native calls, 0 fallbacks, 416 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_009678_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, probe bounds, and child call to 0x0096BA (7 test cases).
- `data/thor-evidence/009678-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x009678` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_0095aa_instruction_step.hpp/.cpp` — exact ROM-checked, 2-caller-gated instruction-stepped helper for Diagonal Terrain Bounding Box Evaluator (`0x0095AA..0x009678`, callers `0x009540` and `0x00956E`), verified in dual canonical 3,600-frame replay (Run A: 32 ROM calls; Run B: 32 native calls, 0 fallbacks, 1,328 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0095aa_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, diagonal collision branches, obstacle flags, and child call to 0x009678 (8 test cases).
- `data/thor-evidence/0095aa-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0095AA` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_009ad0_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Terrain Height Interpolation Alternate Entry (`0x009AD0..0x009AD6`, caller `0x0098FA`), verified in dual canonical 3,600-frame replay (Run A: 30 ROM calls; Run B: 30 native calls, 0 fallbacks, 60 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_009ad0_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, coordinate loading, and fallthrough to 0x009AD6 (5 test cases).
- `data/thor-evidence/009ad0-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x009AD0` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_009d00_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Tile Attribute / Height Descriptor Query (`0x009D00..0x009D4E`, multiple callers), verified in dual canonical 3,600-frame replay (Run A: 42 ROM calls; Run B: 42 native calls, 0 fallbacks, 714 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_009d00_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, MOVEM push/pop framing, descriptor selection, and child calls to 0x009BF2 and 0x009AD6 (6 test cases).
- `data/thor-evidence/009d00-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x009D00` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_0103b0_instruction_step.hpp/.cpp` — exact ROM-checked, 2-caller-gated instruction-stepped helper for Entity Offset / Coordinate Setter Leaf (`0x0103B0..0x0103E0`, callers `0x010260` and `0x010372`), verified in dual canonical 3,600-frame replay (Run A: 11 ROM calls; Run B: 11 native calls, 0 fallbacks, 132 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0103b0_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and offset calculations (5 test cases).
- `data/thor-evidence/0103b0-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0103B0` displacement across all 8 canonical streams (receipt #86).
- `src/tools/hybrid/m68k_010382_instruction_step.hpp/.cpp` — exact ROM-checked, 2-caller-gated instruction-stepped helper for Entity ID Proximity Search Leaf (`0x010382..0x0103B0`, callers `0x01034A` and `0x014F96`), verified in dual canonical 3,600-frame replay (Run A: 10 ROM calls; Run B: 10 native calls, 0 fallbacks, 350 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_010382_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, pool scanning, and match conditions (6 test cases).
- `data/thor-evidence/010382-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x010382` displacement across all 8 canonical streams (receipt #87).
- `src/tools/hybrid/m68k_00e398_instruction_step.hpp/.cpp` — exact ROM-checked, 2-caller-gated instruction-stepped helper for Entity-to-Player Vector / Quadrant Direction Leaf (`0x00E398..0x00E3E6`, callers `0x01A2D0` and `0x01B91C`), verified in dual canonical 3,600-frame replay (Run A: 20 ROM calls; Run B: 20 native calls, 0 fallbacks, 359 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00e398_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, delta vector math, and 4-quadrant cardinal classification (8 test cases).
- `data/thor-evidence/00e398-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00E398` displacement across all 8 canonical streams (receipt #88).
- `src/tools/hybrid/m68k_01ad14_instruction_step.hpp/.cpp` — exact ROM-checked, 8-caller-gated instruction-stepped helper for Entity Path / Trajectory Data Loader Leaf (`0x01AD14..0x01AD38`, 8 callers), verified in dual canonical 3,600-frame replay (Run A: 11 ROM calls; Run B: 11 native calls, 0 fallbacks, 99 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_01ad14_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, ROM table offsets, shift cycle timing, and trajectory loading (5 test cases).
- `data/thor-evidence/01ad14-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x01AD14` displacement across all 8 canonical streams (receipt #89).
- `src/tools/hybrid/m68k_00e2d4_instruction_step.hpp/.cpp` — exact ROM-checked, 2-caller-gated instruction-stepped helper for 8-Direction Velocity / Delta-Position Transform Leaf (`0x00E2D4..0x00E338`, callers `0x00C722` and `0x00DB7A`), verified in dual canonical 3,600-frame replay (Run A: 37 ROM calls; Run B: 37 native calls, 0 fallbacks, 625 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00e2d4_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, 8-direction velocity decomposition, and speed scaling (7 test cases).
- `data/thor-evidence/00e2d4-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00E2D4` displacement across all 8 canonical streams (receipt #90).
- `src/tools/hybrid/m68k_06231e_instruction_step.hpp/.cpp` — exact ROM-checked, 2-caller-gated instruction-stepped helper for Parameter Scaler Leaf (`0x06231E..0x06232C`, callers `0x062092` and `0x062300`), verified in dual canonical 3,600-frame replay (Run A: 40 ROM calls; Run B: 40 native calls, 0 fallbacks, 120 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_06231e_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, threshold branching, signed multiplication, and arithmetic shift (6 test cases).
- `data/thor-evidence/06231e-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x06231E` displacement across all 8 canonical streams (receipt #91).
- `src/tools/hybrid/m68k_0622d0_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Entity Animation Attribute Loader Leaf (`0x0622D0..0x0622DC`, caller `0x0622A8`), verified in dual canonical 3,600-frame replay (Run A: 30 ROM calls; Run B: 30 native calls, 0 fallbacks, 120 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0622d0_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, stream fetching, and dual entity field stores (4 test cases).
- `data/thor-evidence/0622d0-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0622D0` displacement across all 8 canonical streams (receipt #92).
- `src/tools/hybrid/m68k_0622dc_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Entity Animation Auxiliary Attribute Loader Leaf (`0x0622DC..0x0622E8`, caller `0x0622AC`), verified in dual canonical 3,600-frame replay (Run A: 30 ROM calls; Run B: 30 native calls, 0 fallbacks, 120 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0622dc_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, stream fetching, and dual auxiliary entity field stores (4 test cases).
- `data/thor-evidence/0622dc-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0622DC` displacement across all 8 canonical streams (receipt #93).
- `src/tools/hybrid/m68k_06230e_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Entity Animation Step Parameter Scaler Leaf (`0x06230E..0x06231E`, caller `0x0622B4`), verified in dual canonical 3,600-frame replay (Run A: 30 ROM calls; Run B: 30 native calls, 0 fallbacks, 180 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_06230e_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, zero-extension, shift timing, and dual parameter stores (4 test cases).
- `data/thor-evidence/06230e-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x06230E` displacement across all 8 canonical streams (receipt #94).
- `src/tools/hybrid/m68k_0622f4_instruction_step.hpp/.cpp` — exact ROM-checked, 2-caller-gated instruction-stepped helper for Entity Animation Little-Endian Word Parameter Reader & Scaler (`0x0622F4..0x06230E`, callers `0x0622B0` and `0x0622E8`), verified in dual canonical 3,600-frame replay (Run A: 30 ROM calls; Run B: 30 native calls, 0 fallbacks, 270 stepped instructions, 30 child native calls to `0x06231E`, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0622f4_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, little-endian word assembly, child native call integration, and scaled parameter stores (4 test cases).
- `data/thor-evidence/0622f4-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0622F4` displacement across all 8 canonical streams (receipt #95).
- `src/tools/hybrid/m68k_00129a_instruction_step.hpp/.cpp` — exact ROM-checked, 3-caller-gated instruction-stepped helper for SRAM Header Signature & Checksum Validator (`0x00129A..0x0012BC`), verified in dual canonical 3,600-frame replay (Run A: 16 ROM calls; Run B: 16 native calls, 0 fallbacks, 144 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00129a_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, magic bytes validation, and checksum calculation (4 test cases).
- `data/thor-evidence/00129a-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00129A` displacement across all 8 canonical streams (receipt #96).
- `src/tools/hybrid/m68k_0005ec_instruction_step.hpp/.cpp` — exact ROM-checked, 5-caller-gated instruction-stepped helper for Decompressed Graphics Asset Buffer Alignment Leaf (`0x0005EC..0x0005F8`), verified in dual canonical 3,600-frame replay (Run A: 10 ROM calls; Run B: 10 native calls, 0 fallbacks, 50 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0005ec_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and pointer alignment calculation (4 test cases).
- `data/thor-evidence/0005ec-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0005EC` displacement across all 8 canonical streams (receipt #97).
- `src/tools/hybrid/m68k_00d9be_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Entity Dynamic Slot Allocation Table Scanner (`0x00D9BE..0x00D9F0`), verified in dual canonical 3,600-frame replay (Run A: 9 ROM calls; Run B: 9 native calls, 0 fallbacks, 96 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00d9be_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, table traversal, and slot indexing (4 test cases).
- `data/thor-evidence/00d9be-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00D9BE` displacement across all 8 canonical streams (receipt #98).
- `src/tools/hybrid/m68k_008ef8_instruction_step.hpp/.cpp` — exact ROM-checked, 14-caller-gated instruction-stepped helper for Entity Animation Action State Reset Leaf (`0x008EF8..0x008F10`), verified in dual canonical 3,600-frame replay (Run A: 14 ROM calls; Run B: 14 native calls, 0 fallbacks, 84 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_008ef8_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, direct call and tail-jump entries, and field stores (4 test cases).
- `data/thor-evidence/008ef8-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x008EF8` displacement across all 8 canonical streams (receipt #99).
- `src/tools/hybrid/m68k_008cac_instruction_step.hpp/.cpp` — exact ROM-checked, 3-caller-gated instruction-stepped helper for Entity Record Block Clearer (`0x008CAC..0x008CCE`), verified in dual canonical 3,600-frame replay (Run A: 9 ROM calls; Run B: 9 native calls, 0 fallbacks, 441 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_008cac_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, bounded stride loop, and record field initialization (4 test cases).
- `data/thor-evidence/008cac-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x008CAC` displacement across all 8 canonical streams (receipt #100).
- `src/tools/hybrid/m68k_008cd0_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Entity Secondary Record Clearer Leaf (`0x008CD0..0x008CDA`), verified in dual canonical 3,600-frame replay (Run A: 3 ROM calls; Run B: 3 native calls, 0 fallbacks, 39 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_008cd0_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, stride stepping, and record clearing (4 test cases).
- `data/thor-evidence/008cd0-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x008CD0` displacement across all 8 canonical streams (receipt #101).
- `src/tools/hybrid/m68k_00d7b0_instruction_step.hpp/.cpp` — exact ROM-checked, 2-caller-gated instruction-stepped helper for Camera / Map Chunk Stride Bit-Shift Exponent Calculator (`0x00D7B0..0x00D7C0`), verified in dual canonical 3,600-frame replay (Run A: 6 ROM calls; Run B: 6 native calls, 0 fallbacks, 114 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00d7b0_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, bit-shift loop execution, and shift cycle timing parity (3 test cases).
- `data/thor-evidence/00d7b0-replay-differential-20261021.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00D7B0` displacement across all 8 canonical streams (receipt #102).
- `src/tools/hybrid/m68k_0101e4_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Active Entity Header Buffer Pointer Getter (`0x0101E4..0x0101EC`), verified in dual canonical 3,600-frame replay (Run A: 2 ROM calls; Run B: 2 native calls, 0 fallbacks, 4 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0101e4_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and buffer address loading (3 test cases).
- `data/thor-evidence/0101e4-replay-differential-20261021.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0101E4` displacement across all 8 canonical streams (receipt #103).
- `src/tools/hybrid/m68k_007d8a_instruction_step.hpp/.cpp` — exact ROM-checked, 5-caller-gated instruction-stepped helper for Controller Input History Buffer Bitmask Accumulator (`0x007D8A..0x007DC8`), verified in dual canonical 3,600-frame replay (Run A: 8 ROM calls; Run B: 8 native calls, 0 fallbacks, 376 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_007d8a_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, buffer traversal, wrapping, and bitmask accumulation (4 test cases).
- `data/thor-evidence/007d8a-replay-differential-20261021.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x007D8A` displacement across all 8 canonical streams (receipt #104).
- `src/tools/hybrid/m68k_0085e2_instruction_step.hpp/.cpp` — exact ROM-checked, 4-caller-gated instruction-stepped helper for Directional Velocity and Facing Resolver (`0x0085E2..0x0086B4`), verified in dual canonical 3,600-frame replay (Run A: 283 ROM calls; Run B: 283 native calls, 0 fallbacks, 3,254 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0085e2_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and vector calculation.
- `data/thor-evidence/0085e2-replay-differential-20261021.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0085E2` displacement across all 8 canonical streams (receipt #105).
- `src/tools/hybrid/m68k_0604bc_instruction_step.hpp/.cpp` — exact ROM-checked, 2-caller-gated instruction-stepped helper for Sound Effect Reset & Sub-Track Muter (`0x0604BC..0x0604E6`), verified in dual canonical 3,600-frame replay (Run A: 10 ROM calls; Run B: 10 native calls, 0 fallbacks, 100 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0604bc_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and audio RAM clears.
- `data/thor-evidence/0604bc-replay-differential-20261021.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0604BC` displacement across all 8 canonical streams (receipt #106).
- `src/tools/hybrid/m68k_01a288_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Facing / Animation Sub-State Lookup (`0x01A288..0x01A29C`), verified in dual canonical 3,600-frame replay (Run A: 5 ROM calls; Run B: 5 native calls, 0 fallbacks, 30 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_01a288_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and sub-state table lookup.
- `data/thor-evidence/01a288-replay-differential-20261021.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x01A288` displacement across all 8 canonical streams (receipt #107).
- `src/tools/hybrid/m68k_0062cc_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Actor Motion State and Timer Clearer (`0x0062CC..0x0062E4`), reclassified as PARTIAL_NATIVE_INTERCEPT and disabled from the active native helpers baseline due to 145 tail-fallthrough entries from ~30 callers.
- `tests/m68k_0062cc_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and timer field clearing.
- `data/thor-evidence/0062cc-replay-differential-20261021.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0062CC` displacement across all 8 canonical streams (receipt #108).
- `src/tools/hybrid/m68k_0101d0_instruction_step.hpp/.cpp` — exact ROM-checked, 2-caller-gated instruction-stepped helper for Secondary Entity Header Pointer Evaluator (`0x0101D0..0x0101E4`), verified in dual canonical 3,600-frame replay (Run A: 2 ROM calls; Run B: 2 native calls, 0 fallbacks, 16 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0101d0_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and address pointer evaluation.
- `data/thor-evidence/0101d0-replay-differential-20261021.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0101D0` displacement across all 8 canonical streams (receipt #109).
- `src/tools/hybrid/m68k_01b024_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Actor Animation Sub-State Dispatcher D (`0x01B024..0x01B046`), verified in dual canonical 3,600-frame replay (Run A: 4 ROM calls; Run B: 4 native calls, 0 fallbacks, 44 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_01b024_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and actor animation sub-state dispatching.
- `data/thor-evidence/01b024-replay-differential-20261021.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x01B024` displacement across all 8 canonical streams (receipt #110).
- `src/tools/hybrid/m68k_0085c4_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Player Extended Motion Flag Setter (`0x0085C4..0x0085E2`), verified in dual canonical 3,600-frame replay (Run A: 1 ROM call; Run B: 1 native call, 0 fallbacks, 6 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0085c4_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, threshold comparison, and motion flag mutation.
- `data/thor-evidence/0085c4-replay-differential-20261021.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0085C4` displacement across all 8 canonical streams (receipt #111).
- `src/tools/hybrid/m68k_01a21e_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Actor Animation Sub-State Dispatcher A (`0x01A21E..0x01A23C`), verified in dual canonical 3,600-frame replay (Run A: 1 ROM call; Run B: 1 native call, 0 fallbacks, 8 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_01a21e_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and sub-state table lookup.
- `data/thor-evidence/01a21e-replay-differential-20261021.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x01A21E` displacement across all 8 canonical streams (receipt #112).
- `src/tools/hybrid/m68k_01ae7e_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Actor Animation Sub-State Dispatcher B (`0x01AE7E..0x01AE9C`), verified in dual canonical 3,600-frame replay (Run A: 1 ROM call; Run B: 1 native call, 0 fallbacks, 8 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_01ae7e_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and sub-state table lookup.
- `data/thor-evidence/01ae7e-replay-differential-20261021.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x01AE7E` displacement across all 8 canonical streams (receipt #113).
- `src/tools/hybrid/m68k_01af3a_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Actor Animation Sub-State Dispatcher C (`0x01AF3A..0x01AF58`), verified in dual canonical 3,600-frame replay (Run A: 1 ROM call; Run B: 1 native call, 0 fallbacks, 8 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_01af3a_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and sub-state table lookup.
- `data/thor-evidence/01af3a-replay-differential-20261021.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x01AF3A` displacement across all 8 canonical streams (receipt #114).

- `src/tools/hybrid/m68k_00c90e_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Actor Parameter Table Lookup and Displacement Resolver (`0x00C90E..0x00C924`), verified in dual canonical 3,600-frame replay (Run A: 3 ROM calls; Run B: 3 native calls, 0 fallbacks, 33 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00c90e_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and actor parameter table lookup.
- `data/thor-evidence/00c90e-replay-differential-20261021.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00C90E` displacement across all 8 canonical streams (receipt #115).
- `src/tools/hybrid/m68k_00cecc_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Actor Collision / Boundary Coordinate Delta Resolver (`0x00CECC..0x00CEFA`), verified in dual canonical 3,600-frame replay (Run A: 3 ROM calls; Run B: 3 native calls, 0 fallbacks, 210 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00cecc_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and actor bounding box coordinate calculation.
- `data/thor-evidence/00cecc-replay-differential-20261021.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00CECC` displacement across all 8 canonical streams (receipt #116).
- `src/tools/hybrid/m68k_010086_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Entity State Reset and Attribute Register Clear (`0x010086..0x0100AC`), verified in dual canonical 3,600-frame replay (Run A: 2 ROM calls; Run B: 2 native calls, 0 fallbacks, 50 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_010086_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and entity struct clearing.
- `data/thor-evidence/010086-replay-differential-20261021.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x010086` displacement across all 8 canonical streams (receipt #117).
- `src/tools/hybrid/m68k_00f920_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Entity Coordinate Range Clamping & Direction Evaluator (`0x00F920..0x00F94C`), verified in dual canonical 3,600-frame replay (Run A: 2 ROM calls; Run B: 2 native calls, 0 fallbacks, 86 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00f920_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, coordinate delta calculation, and compass direction determination.
- `data/thor-evidence/00f920-replay-differential-20261021.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00F920` displacement across all 8 canonical streams (receipt #118).
- `src/tools/hybrid/m68k_002d66_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for VDP Sprite Attribute Buffer Pack and Transfer Subroutine (`0x002D66..0x002D88`), verified in dual canonical 3,600-frame replay (Run A: 6 ROM calls; Run B: 6 native calls, 0 fallbacks, 458 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_002d66_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and sprite attribute block copying.
- `data/thor-evidence/002d66-replay-differential-20261021.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x002D66` displacement across all 8 canonical streams (receipt #119).
- `src/tools/hybrid/m68k_061032_instruction_step.hpp/.cpp` — exact ROM-checked, 2-caller-gated instruction-stepped helper for Level Transition Entity Parser (`0x061032..0x06108C`), verified in dual canonical 3,600-frame replay (Run A: 27 ROM calls; Run B: 27 native calls, 0 fallbacks, 1,215 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_061032_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and transition entity record parsing.
- `data/thor-evidence/061032-replay-differential-20261021.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x061032` displacement across all 8 canonical streams (receipt #120).
- `src/tools/hybrid/m68k_0086b4_instruction_step.hpp/.cpp`, `src/tools/hybrid/m68k_0086b4_step_ops.hpp/.cpp` — exact ROM-checked, 4-caller-gated modular instruction-stepped helper for Player Direction Vector & Angle Range Mapper (`0x0086B4..0x008778`), verified in dual canonical 3,600-frame replay (Run A: 8 ROM calls; Run B: 8 native calls, 0 fallbacks, 1,926 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0086b4_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and player vector/angle mapping.
- `data/thor-evidence/0086b4-replay-differential-20261021.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0086B4` displacement across all 8 canonical streams (receipt #121).
- `src/tools/hybrid/m68k_00f740_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Entity Lifecycle Flag Evaluator (`0x00F740..0x00F756`), verified in dual canonical 3,600-frame replay (Run A: 7 ROM calls; Run B: 7 native calls, 0 fallbacks, 71 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00f740_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and lifecycle flag condition code evaluations.
- `data/thor-evidence/00f740-replay-differential-20261021.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00F740` displacement across all 8 canonical streams (receipt #122).
- `src/tools/hybrid/m68k_008950_instruction_step.hpp/.cpp` — exact ROM-checked, 4-caller-gated instruction-stepped helper for Player Animation Motion Buffer Matcher (`0x008950..0x00898E`), verified in dual canonical 3,600-frame replay (Run A: 10 ROM calls; Run B: 10 native calls, 0 fallbacks, 1,409 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_008950_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and animation buffer matching logic.
- `data/thor-evidence/008950-replay-differential-20261021.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x008950` displacement across all 8 canonical streams (receipt #123).
- `src/tools/hybrid/m68k_00c012_instruction_step.hpp/.cpp`, `src/tools/hybrid/m68k_00c012_step_ops.hpp/.cpp` — instruction-stepped helper for Collision Box Overlap & Distance Comparator (`0x00C012..0x00C0DE`). Unit tested in `tests/m68k_00c012_instruction_step_test.cpp`. Evaluated in dual 3,600-frame canonical replay; showed instruction timing divergence at frame 2053. Classified as `RESEARCHED_NOT_DISPLACED` / `REJECTED_FULL_DISPLACEMENT_DISABLED`, disabled in runtime island.
- `data/thor-evidence/00c012-replay-differential-20261021.json` — machine-readable verification receipt recording rejection of `0x00C012` full displacement due to timing divergence.
- `src/tools/hybrid/m68k_0610c8_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for FM Sound Channel Track Context Initializer (`0x0610C8..0x06112C`), verified in dual canonical 3,600-frame replay (Run A: 9 ROM calls; Run B: 9 native calls, 0 fallbacks, 243 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0610c8_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and FM channel context initialization logic.
- `data/thor-evidence/0610c8-replay-differential-20261022.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0610C8` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_0008a2_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Framebuffer RAM Block Clear Loop (`0x0008A2..0x0008B4`), verified in dual canonical 3,600-frame replay (Run A: 2 ROM calls; Run B: 2 native calls, 0 fallbacks, 3,848 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0008a2_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and RAM clear loop execution.
- `data/thor-evidence/0008a2-replay-differential-20261022.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0008A2` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_0008b6_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Screen Region Setup and Dimension Pre-calculator (`0x0008B6..0x0008E8`), verified in dual canonical 3,600-frame replay (Run A: 2 ROM calls; Run B: 2 native calls, 0 fallbacks, 620 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0008b6_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and nested DBRA screen dimension loops.
- `data/thor-evidence/0008b6-replay-differential-20261022.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0008B6` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_008910_instruction_step.hpp/.cpp` - exact ROM-checked, 4-caller-gated instruction-stepped helper for Motion Sequence Buffer Bit-Test (`0x008910..0x008950`), verified in dual canonical 3,600-frame replay (Run A: 17 ROM calls; Run B: 17 native calls, 0 fallbacks, 1,488 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_008910_instruction_step_test.cpp` - unit test suite verifying caller gating, corrupt ROM rejection, and motion sequence bit-test loop.
- `data/thor-evidence/008910-replay-differential-20261022.json` - machine-readable verification receipt proving bit-for-bit replay equivalence of `0x008910` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_061054_instruction_step.hpp/.cpp` - exact ROM-checked, 1-caller-gated instruction-stepped helper for Sound Channel State Reset & Track Initializer (`0x061054..0x0610C8`), verified in dual canonical 3,600-frame replay (Run A: 7 ROM calls; Run B: 7 native calls, 0 fallbacks, 189 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_061054_instruction_step_test.cpp` - unit test suite verifying caller gating, corrupt ROM rejection, and channel state reset initialization.
- `data/thor-evidence/061054-replay-differential-20261022.json` - machine-readable verification receipt proving bit-for-bit replay equivalence of `0x061054` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_06121a_instruction_step.hpp/.cpp` - exact ROM-checked, 2-caller-gated instruction-stepped helper for PSG 4-Channel Mute & Attenuation Setup (`0x06121A..0x061232`), verified in dual canonical 3,600-frame replay (Run A: 11 ROM calls; Run B: 11 native calls, 0 fallbacks, 66 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_06121a_instruction_step_test.cpp` - unit test suite verifying caller gating, corrupt ROM rejection, and PSG port attenuation writes.
- `data/thor-evidence/06121a-replay-differential-20261022.json` - machine-readable verification receipt proving bit-for-bit replay equivalence of `0x06121A` displacement across all 8 canonical streams.
- `data/thor-evidence/00e3e8-replay-differential-20261022.json` - machine-readable verification receipt recording rejection of `0x00E3E8` full displacement due to timing divergence on complex EA displacement calculation.
- `src/tools/hybrid/m68k_017ff2_instruction_step.hpp/.cpp` — instruction-stepped helper for Sprite Table Index Adjuster (`0x017FF2..0x018002`). Unit tested in `tests/m68k_017ff2_instruction_step_test.cpp`. Evaluated in dual 3,600-frame canonical replay; showed raster/interrupt timing divergence at frame 2181 (`first_different_frame=2181`, `different_frame_count=937`). Classified as `RESEARCHED_NOT_DISPLACED` / `REJECTED_FULL_DISPLACEMENT_DISABLED`, disabled in runtime island.
- `data/thor-evidence/017ff2-timing-rejection-20261022.json` — machine-readable verification receipt recording rejection of `0x017FF2` full displacement due to timing divergence.
- `data/thor-evidence/aggregate-136-to-139-20261022.json` — aggregate canonical 3,600-frame replay verification receipt advancing baseline from 136 to 139 active native helpers.
- `src/tools/hybrid/m68k_060fb2_instruction_step.hpp/.cpp` - exact ROM-checked, 1-caller-gated instruction-stepped helper for Sound Channel Context Initializer (`0x060FB2..0x061032`), verified in dual canonical 3,600-frame replay (Run A: 90 ROM calls; Run B: 90 native calls, 0 fallbacks, 2,700 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_060fb2_instruction_step_test.cpp` - unit test suite verifying caller gating, corrupt ROM rejection, and channel context setup.
- `data/thor-evidence/060fb2-replay-differential-20261022.json` - machine-readable verification receipt proving bit-for-bit replay equivalence of `0x060FB2` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_002d58_instruction_step.hpp/.cpp` - exact ROM-checked, 1-caller-gated instruction-stepped helper for Screen Transition Flag Setter & Child Invoker (`0x002D58..0x002D66`), verified in dual canonical 3,600-frame replay (Run A: 4 ROM calls; Run B: 4 native calls, 0 fallbacks, 12 stepped instructions, 4 connected native child calls, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_002d58_instruction_step_test.cpp` - unit test suite verifying caller gating, corrupt ROM rejection, and child invoker execution.
- `data/thor-evidence/002d58-replay-differential-20261022.json` - machine-readable verification receipt proving bit-for-bit replay equivalence of `0x002D58` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_0063da_instruction_step.hpp/.cpp` - exact ROM-checked, 1-caller-gated instruction-stepped helper for Entity Record Dispatch & State Updater (`0x0063DA..0x006464`), verified in dual canonical 3,600-frame replay (Run A: 1 ROM call; Run B: 1 native call, 0 fallbacks, 21 stepped instructions, bit-identical stream hashes across all 3,600 frames). Repaired in M15.187C for opcode prefetch consumption and 4-byte displacement decoding.
- `tests/m68k_0063da_instruction_step_test.cpp` - unit test suite verifying caller gating, corrupt ROM rejection, entity record dispatch, and un-elidable full instruction stepping assertions.
- `data/thor-evidence/0063da-replay-differential-20261022.json` - machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0063DA` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_00638c_instruction_step.hpp/.cpp` - exact ROM-checked, 1-caller-gated instruction-stepped helper for Entity Record Validation & Branch Routine (`0x00638C..0x0063DA`), verified in dual canonical 3,600-frame replay (Run A: 1 ROM call; Run B: 1 native call, 0 fallbacks, 4 stepped instructions along proven path `bne.w 0x63d8 -> rts`, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00638c_instruction_step_test.cpp` - unit test suite verifying caller gating, corrupt ROM rejection, and branch path execution.
- `data/thor-evidence/00638c-replay-differential-20261022.json` - machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00638C` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_00297c_instruction_step.hpp/.cpp` - exact ROM-checked, 1-caller-gated instruction-stepped helper for Genesis Controller Port Direction Initializer (`0x00297C..0x002992`), verified in dual canonical 3,600-frame replay (Run A: 1 ROM call; Run B: 1 native call, 0 fallbacks, 5 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00297c_instruction_step_test.cpp` - unit test suite verifying caller gating, corrupt ROM rejection, and controller port IO writes.
- `data/thor-evidence/00297c-replay-differential-20261022.json` - machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00297C` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_gameplay_step_metrics.cpp` - helper source separating metrics emission to preserve strict <= 500 lines limit in step set.
- `data/thor-evidence/aggregate-142-to-147-20261022.json` — aggregate canonical 3,600-frame replay verification receipt advancing baseline from 142 to 147 active native helpers.
- `src/tools/hybrid/m68k_gameplay_step_registry.hpp/.cpp` — modular step registration helper separating legacy step slots `0..100` from `m68k_gameplay_step_set.cpp` (slots `101..162`) to keep both files well below the 500-line limit with zero runtime/semantic changes.
- `src/tools/hybrid/m68k_01a2ac_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Entity Player-Proximity & Quadrant Evaluator (`0x01A2AC..0x01A322`, caller `0x01A4CA`), verified in dual canonical 3,600-frame replay (Run A: 10 ROM calls; Run B: 10 native calls, 0 fallbacks, 150 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_01a2ac_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, Z-height threshold, and X/Y proximity evaluation.
- `data/thor-evidence/01a2ac-replay-differential-20261023.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x01A2AC` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_00e4d0_instruction_step.hpp/.cpp` — exact ROM-checked, 2-caller-gated instruction-stepped helper for Entity Knockback Velocity Initializer (`0x00E4D0..0x00E502`, callers `0x01A5B6` and `0x01B170`), verified in dual canonical 3,600-frame replay (Run A: 7 ROM calls; Run B: 7 native calls, 0 fallbacks, 77 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00e4d0_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, knockback state initialization, and child `0x00E2D4` velocity transform.
- `data/thor-evidence/00e4d0-replay-differential-20261023.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00E4D0` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_0102be_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Entity Spawn ID Table Range Scanner — Offset +2 (`0x0102BE..0x010300`, caller `0x00FDA2`), verified in dual canonical 3,600-frame replay (Run A: 5 ROM calls; Run B: 5 native calls, 0 fallbacks, 115 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0102be_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, active entity table scan at offset `+2`, and carry flag outputs.
- `data/thor-evidence/0102be-replay-differential-20261023.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0102BE` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_01027c_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Entity Spawn ID Table Range Scanner — Offset +16 (`0x01027C..0x0102BE`, caller `0x00FCE4`), verified in dual canonical 3,600-frame replay (Run A: 2 ROM calls; Run B: 2 native calls, 0 fallbacks, 73 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_01027c_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, active entity table scan at offset `+16`, and carry flag outputs.
- `data/thor-evidence/01027c-replay-differential-20261023.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x01027C` displacement across all 8 canonical streams.
- `data/thor-evidence/aggregate-147-to-151-20261023.json` — aggregate canonical 3,600-frame replay verification receipt advancing baseline from 147 to 151 active native helpers.
- `src/tools/hybrid/m68k_008cdc_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Entity Parameter Reset Loop (`0x008CDC..0x008D06`, caller `0x008C8A`), verified in dual canonical 3,600-frame replay (Run A: 3 ROM calls; Run B: 3 native calls, 0 fallbacks, 516 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_008cdc_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, loop body execution, and parameter reset.
- `data/thor-evidence/008cdc-replay-differential-20261023.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x008CDC` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_062156_instruction_step.hpp/.cpp` — exact ROM-checked, 3-caller-gated instruction-stepped helper for Audio Driver SFX Dispatcher (`0x062156..0x062186`, callers `0x061032`, `0x061054`, `0x0610C8`), verified in dual canonical 3,600-frame replay (Run A: 20 ROM calls; Run B: 20 native calls, 0 fallbacks, 400 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_062156_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, effect code masking, pointer table indexing, and slot committal.
- `data/thor-evidence/062156-replay-differential-20261023.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x062156` displacement across all 8 canonical streams.
- `tools/bizhawk-native-ring/evidence_hold_directives.py` — modular hold and deferral directives dictionary extracted from `evidence_knowledge_bridge.py` to maintain strict <= 500 line size limit.
- `data/thor-evidence/aggregate-151-to-153-20261023.json` — aggregate canonical 3,600-frame replay verification receipt advancing baseline from 151 to 153 active native helpers.
- `src/tools/hybrid/m68k_003064_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for CRAM RGB Channel Linear Interpolator Leaf (`0x003064..0x00307A`, caller `0x00303C`), verified in expanded scenario `beach_combat_transition_6000` (Run A: 3,168 ROM calls; Run B: 3,168 native calls, 0 fallbacks, 34,848 stepped instructions, 0 diff frames).
- `tests/m68k_003064_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, RGB channel math, and blend accumulation.
- `data/thor-evidence/003064-replay-differential-20261026.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x003064` displacement.
- `src/tools/hybrid/m68k_02edca_instruction_step.hpp/.cpp` — exact ROM-checked, 4-caller-gated instruction-stepped helper for Entity Child Slot Relative Coordinate Updater (`0x02EDCA..0x02EDE6`, callers `0x02EDAA`, `0x02EDB2`, `0x02EDBA`, `0x02EDC2`), verified in expanded scenario `beach_combat_transition_6000` (Run A: 2,020 ROM calls; Run B: 2,020 native calls, 0 fallbacks, 14,140 stepped instructions, 0 diff frames).
- `tests/m68k_02edca_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, coordinate offsets, and slot memory writes.
- `data/thor-evidence/02edca-replay-differential-20261026.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x02EDCA` displacement.
- `src/tools/hybrid/m68k_02eea0_instruction_step.hpp/.cpp` — exact ROM-checked, 4-caller-gated instruction-stepped helper for Entity Child Slot 3D Position & Offset Subtractor (`0x02EEA0..0x02EEC4`, callers `0x02EE82`, `0x02EE8A`, `0x02EE92`, `0x02EE9A`), verified in expanded scenario `beach_combat_transition_6000` (Run A: 2,020 ROM calls; Run B: 2,020 native calls, 0 fallbacks, 22,220 stepped instructions, 0 diff frames).
- `tests/m68k_02eea0_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, 3D subtraction logic, and memory updates.
- `data/thor-evidence/02eea0-replay-differential-20261026.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x02EEA0` displacement.
- `src/tools/hybrid/m68k_001d30_instruction_step.hpp/.cpp` — exact ROM-checked, 3-caller-gated instruction-stepped helper for Menu Text / Sprite Tile Descriptor Writer Leaf (`0x001D30..0x001D5A`, callers `0x001C64`, `0x001C6C`, `0x001C74`), verified in expanded scenario `file_select_name_entry_3600` (Run A: 117 ROM calls; Run B: 117 native calls, 0 fallbacks, 1,197 stepped instructions, 0 diff frames).
- `tests/m68k_001d30_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, character mapping, and sprite attribute writes.
- `data/thor-evidence/001d30-replay-differential-20261026.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x001D30` displacement.
- `src/tools/hybrid/m68k_03ac92_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Title / Intro Scroll Dual Fixed-Point Accumulator Stepper (`0x03AC92..0x03ACA8`, caller `0x03AAA8`), verified in expanded scenario `attract_title_story_demo_3600` (Run A: 89 ROM calls; Run B: 89 native calls, 0 fallbacks, 267 stepped instructions, 0 diff frames).
- `tests/m68k_03ac92_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and dual accumulator updates.
- `data/thor-evidence/03ac92-replay-differential-20261026.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x03AC92` displacement.
- `data/thor-evidence/aggregate-153-to-158-20261026.json` — aggregate canonical 3,600-frame replay verification receipt advancing baseline from 153 to 158 active native helpers with exact 8/8 canonical stream match and 0 diff frames.
- `src/tools/hybrid/m68k_02e30e_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Name Entry Cursor Position Clamper Leaf (`0x02E30E..0x02E3A8`, caller `0x02E290`), verified in expanded scenario `file_select_name_entry_3600` (125 native calls, 0 fallbacks, 1,562 stepped instructions, 0 diff frames).
- `tests/m68k_02e30e_instruction_step_test.cpp` — unit test suite for `0x02E30E` verifying caller gating, all 4 branch paths, bounds clamping, and flag manipulation.
- `data/thor-evidence/02e30e-replay-differential-20261027.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x02E30E`.
- `src/tools/hybrid/m68k_03c584_instruction_step.hpp/.cpp` — exact ROM-checked, 4-caller-gated instruction-stepped helper for Menu Entity Slot Coordinator Leaf (`0x03C584..0x03C5B6`, callers `0x03C34A`, `0x03C3CC`, `0x03C402`, `0x03C432`), verified in expanded scenario `menu_pause_inventory_3600` (3 native calls, 0 fallbacks, 192 stepped instructions, 0 diff frames).
- `tests/m68k_03c584_instruction_step_test.cpp` — unit test suite for `0x03C584` verifying caller gating, entity slot iteration, and threshold clearing.
- `data/thor-evidence/03c584-replay-differential-20261027.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x03C584`.
- `data/thor-evidence/aggregate-167-to-169-20261027.json` — aggregate canonical 3,600-frame replay verification receipt advancing baseline from 167 to 169 active native helpers with exact 8/8 canonical stream matches and 0 diff frames.
- `src/tools/hybrid/m68k_00b730_instruction_step.hpp/.cpp` — exact ROM-checked, 4-caller-gated instruction-stepped helper for Sprite Attribute Table (SAT) Builder Loop (`0x00B730..0x00B79A`, callers `0x000EFE`, `0x004B48`, `0x03B448`, `0x03CE86`), verified in expanded scenario `attract_title_story_demo_3600` (1,396 native calls, 0 fallbacks, 171,475 stepped instructions, 0 diff frames).
- `tests/m68k_00b730_instruction_step_test.cpp` — unit test suite for `0x00B730` verifying caller gating, corrupt ROM rejection, and sprite piece clamping.
- `data/thor-evidence/00b730-replay-differential-20261027.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00B730`.
- `data/thor-evidence/aggregate-169-to-170-20261027.json` — aggregate canonical 3,600-frame replay verification receipt advancing baseline from 169 to 170 active native helpers with exact 8/8 canonical stream matches and 0 diff frames.
- `src/tools/hybrid/m68k_03b832_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped prototype for Title/Intro Palette Cross-Fader Leaf (`0x03B832..0x03B8A6`, caller `0x03AB8C`), slot 191 registered in step set, classified as TIMING_MODEL_BUG / DEFERRED pending `muls.w`/`divs.w` hardware cycle calibration.
- `tests/m68k_03b832_instruction_step_test.cpp` — unit test suite for `0x03B832` verifying caller gating, corrupt ROM rejection, and palette interpolation.
- `src/tools/hybrid/m68k_002b8a_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for VDP H-Scroll Table Reset Leaf (`0x002B8A..0x002BA0`, caller `0x002B40`), verified in scenarios `canonical_3600` and `attract_3600` (10 / 13 native calls, 0 fallbacks, 39 stepped instructions, 0 diff frames).
- `tests/m68k_002b8a_instruction_step_test.cpp` — unit test suite for `0x002B8A` verifying caller gating, corrupt ROM rejection, and VDP port writes.
- `data/thor-evidence/002b8a-replay-differential-20261028.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x002B8A`.
- `src/tools/hybrid/m68k_002ba0_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for VDP Plane Table Clear Loop Leaf (`0x002BA0..0x002BBC`, caller `0x002B44`), verified in scenarios `canonical_3600` and `attract_3600` (10 / 13 native calls, 0 fallbacks, 1,105 stepped instructions, 0 diff frames).
- `tests/m68k_002ba0_instruction_step_test.cpp` — unit test suite for `0x002BA0` verifying caller gating, corrupt ROM rejection, and DBRA plane zeroing.
- `data/thor-evidence/002ba0-replay-differential-20261028.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x002BA0`.
- `src/tools/hybrid/m68k_002b6e_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for VDP V-Scroll Table Clear Leaf (`0x002B6E..0x002B8A`, caller `0x002B5E`), verified in scenarios `canonical_3600` and `attract_3600` (10 / 13 native calls, 0 fallbacks, 52 stepped instructions, 0 diff frames).
- `tests/m68k_002b6e_instruction_step_test.cpp` — unit test suite for `0x002B6E` verifying caller gating, corrupt ROM rejection, and VSRAM table clear.
- `data/thor-evidence/002b6e-replay-differential-20261028.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x002B6E`.
- `src/tools/hybrid/m68k_03c956_instruction_step.hpp/.cpp` — exact ROM-checked, multi-caller-gated instruction-stepped helper for Display/Window Critical Mode Transition Leaf (`0x03C956..0x03C98A`, 23 ROM callers), verified in scenarios `canonical_3600` and `attract_3600` (170 native calls, 0 fallbacks, 1,530 stepped instructions, 0 diff frames).
- `tests/m68k_03c956_instruction_step_test.cpp` — unit test suite for `0x03C956` verifying caller gating, corrupt ROM rejection, SR interrupt masking, and RAM flag updates.
- `data/thor-evidence/03c956-replay-differential-20261028.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x03C956`.
- `src/tools/hybrid/m68k_002cbc_instruction_step.hpp/.cpp` — exact ROM-checked, 6-caller-gated instruction-stepped helper for VDP Plane Tile Rectangle Block Writer Leaf (`0x002CBC..0x002CE4`), verified across all 5 scenarios (2 native calls, 0 fallbacks, 2,642 stepped instructions, 0 diff frames).
- `tests/m68k_002cbc_instruction_step_test.cpp` — unit test suite for `0x002CBC` verifying caller gating, corrupt ROM rejection, and VDP tile block copy loop.
- `data/thor-evidence/002cbc-replay-differential-20261028.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x002CBC`.
- `src/tools/hybrid/m68k_shared_tail_contract.hpp` — reusable Shared Tail Execution Contract providing caller/branch validation, transfer provenance extraction, stack frame validation, and register/CCR safety for shared tail routines.
- `src/tools/hybrid/m68k_0062cc_instruction_step.hpp/.cpp` — shared-tail contract instruction stepper for Stop Entity / Reset Movement Velocity (`0x0062CC..0x0062E4`), supporting both direct calls (BSR/JSR) and tail branches (BRA/Bcc/JMP), verified across canonical_3600 (148 entries) and beach_6000 (372 entries) with 0 fallbacks and 0 diff frames.
- `tests/m68k_0062cc_instruction_step_test.cpp` — 12-case unit test suite for `0x0062CC` shared tail execution contract verifying direct BSR, BRA, Bcc, RTS, SP balance, CCR flags, X preservation, RAM writes, invalid entry, and stack integrity.
- `data/thor-evidence/0062cc-replay-differential-20261101.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0062CC` under shared tail contract (Milestone M15.184).
- `src/tools/hybrid/m68k_00663e_instruction_step.hpp/.cpp` — exact ROM-checked instruction stepper for Entity Movement Friction Deceleration & Subpixel Drift Applicator (`0x00663E..0x0066AA`, Action 6 in Player Dispatcher `0x005670`), supporting arithmetic friction deceleration, zero clamping, subpixel coordinate accumulation, child call dispatch (`0x0064C4`, `0x060004`), tail call to `0x0062CC`, and RTS return, verified across canonical_3600 (13 entries) and beach_6000 (52 entries) with 0 fallbacks and 0 diff frames.
- `tests/m68k_00663e_instruction_step_test.cpp` — 6-case unit test suite for `0x00663E` verifying normal completion, tail call to `0x0062CC`, negative velocity damping, SFX trigger path, unhandled caller rejection, and ROM byte mismatch.
- `data/thor-evidence/00663e-replay-differential-20261102.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00663E` displacement across all 8 canonical and beach streams (Milestone M15.186).
- `src/tools/hybrid/m68k_0062e4_instruction_step.hpp/.cpp` — exact ROM-checked instruction stepper for Entity Velocity / Subpixel Movement Applicator (`0x0062E4..0x006336`, Action 2 in Player Dispatcher `0x005670`), supporting subpixel coordinate accumulation, child call dispatch (`0x007DC8`, `0x0085E2`, `0x0064C4`, `0x060004`), tail call to `0x0062CC`, and stack unwind abort handling, verified across canonical_3600 (84 entries) and beach_6000 (296 entries) with 0 fallbacks and 0 diff frames.
- `tests/m68k_0062e4_instruction_step_test.cpp` — 9-case unit test suite for `0x0062E4` verifying normal completion, tail call to `0x0062CC`, stack abort early exit, unhandled caller rejection, ROM byte mismatch, external IRQ handling, SFX trigger path, and subpixel math accumulation.
- `data/thor-evidence/0062e4-replay-differential-20261102.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0062E4` displacement across all 8 canonical and beach streams (Milestone M15.185).
- `src/tools/hybrid/m68k_006516_instruction_step.hpp/.cpp` and `m68k_006516_step_block1.hpp/.cpp` — exact ROM-checked instruction stepper partitioned across two <= 500 line translation units for Secondary Locomotion / Diagonal Movement Integration (`0x006516..0x00663E`, Action 2 in Player Action Dispatcher `0x005670`), supporting coordinate accumulation, child call dispatch (`0x007DC8`, `0x0083D4`, `0x0064C4`, `0x008EF8`, `0x060004`), tail call to `0x0062CC`, direct fallthrough into active helper `0x00663E`, and stack unwind abort handling, verified across 1,592 gameplay continuation frames from checkpoint `canonical_frame2008.state` (97 entries, 0 fallbacks, 3,049 stepped instructions, 0 diff frames).
- `tests/m68k_006516_instruction_step_test.cpp` — 6-case unit test suite for `0x006516` using non-elidable `TEST_ASSERT` macros verifying entry validation, child returns, branch conditions, sound trigger, unhandled caller rejection, ROM byte mismatch, and direct fallthrough to `0x00663E`.
- `data/thor-evidence/006516-gameplay-checkpoint-ab-20261103.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x006516` displacement across all 8 output streams under the full production baseline (Milestone M15.188).
- `src/tools/hybrid/m68k_007074_instruction_step.hpp/.cpp` and `m68k_007074_step_ops.hpp/.cpp` — modular ROM-checked instruction stepper partitioned across dedicated <= 500 line translation units for Player Locomotion Jump Gravity & Landing Collision Check (`0x007074..0x0070F8`, called from Action 0x72 `0x006FE4` and Action 0x74 `0x00701A`), supporting floor collision probe `0x00BCC4`, sound dispatch `0x060004`, landing state `0x00E268`, and animation update `0x01AD14`. Repaired operand inversion in three `ADD.L` instructions (`ADD.L Dn, d16(An)`), verified across 1,592 continuation frames with 42 native calls, 0 fallbacks, 600 stepped instructions, and 8/8 stream bit-for-bit equivalence; active helper #187 (Milestone M15.190).
- `tests/m68k_007074_instruction_step_test.cpp` — unit test suite for `0x007074` using non-elidable `TEST_ASSERT` macros verifying RAM writes to altitude (`16(a6)`), X-pos (`114(a6)`), Y-pos (`118(a6)`), velocity register preservation, fallthrough to `0x00BCC4`, ground branch with sound trigger `0x060004`, and ground collision active path with Carry set.
- `data/thor-evidence/007074-timing-rejection-20261103.json` — machine-readable rejection receipt recording 5/8 stream matches, 42/42 completed calls, 0 contract faults, and frame 771 DRAM refresh timing shift details (Milestone M15.189).
- `data/thor-evidence/007074-gameplay-checkpoint-ab-20261104.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x007074` across all 8 canonical output streams over 1,592 continuation frames from checkpoint `canonical_frame2008.state` under production baseline 187 (Milestone M15.190).
- `src/tools/hybrid/m68k_006fe4_instruction_step.hpp/.cpp` and `m68k_006fe4_step_ops.hpp/.cpp` — modular ROM-checked instruction stepper partitioned across dedicated <= 500 line translation units for Player Action 0x72 Jump Handler (`0x006FE4..0x00701A`, called from Action Table Jump via 0x00558A loop), supporting child physics call `0x007074`, input read `0x007D8A`, sound dispatch `0x060004`, carry-clear early exit, and transition branch to `0x007066`. Verified across 1,592 continuation frames with 24 native calls, 0 fallbacks, 87 stepped instructions, and 8/8 stream bit-for-bit equivalence; active helper #188 (Milestone M15.191).
- `tests/m68k_006fe4_instruction_step_test.cpp` — unit test suite for `0x006FE4` using non-elidable `TEST_ASSERT` macros verifying carry-clear airborne return, landing branch to `0x007066`, sound trigger dispatch `0x060004`, unhandled caller rejection, and corrupt ROM rejection.
- `data/thor-evidence/006fe4-gameplay-checkpoint-ab-20261104.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x006FE4` across all 8 canonical output streams over 1,592 continuation frames from checkpoint `canonical_frame2008.state` under production baseline 188 (Milestone M15.191).
- `src/tools/hybrid/m68k_03ace4_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Title / Intro Sequence Entity Initializer (`0x03ACE4..0x03AD0C`, proven caller `0x03AAA8`), verified in scenarios `attract_3600` and `canonical_3600` (158 native calls, 0 fallbacks, 474 stepped instructions, 0 diff frames).
- `tests/m68k_03ace4_instruction_step_test.cpp` — unit test suite for `0x03ACE4` verifying caller gating, corrupt ROM rejection, child call dispatch, and memory word writes.
- `data/thor-evidence/03ace4-replay-differential-20261101.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x03ACE4` (Milestone M15.183).
- `tests/thor_auto_migrator_phase18_test.py` — regression test suite for Milestone M15.183 production integration, step set slot 205, receipt index 173 entries, and baseline 182 integrity.
- `src/tools/hybrid/m68k_00e36e_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Entity Collision / Object Bound Updater (`0x00E36E..0x00E398`, proven caller `0x00EBC4`), verified in scenarios `canonical_3600` and `beach_6000` (48 native calls, 0 fallbacks, 336 stepped instructions, 0 diff frames).
- `tests/m68k_00e36e_instruction_step_test.cpp` — unit test suite for `0x00E36E` verifying caller gating, corrupt ROM rejection, child call dispatch, and bit flag operations.
- `data/thor-evidence/00e36e-replay-differential-20261101.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00E36E` (Milestone M15.182).
- `tests/thor_auto_migrator_codegen_extension_test.py` — unit test suite for generalized M68000 instruction codegen extensions (displacement byte transfer, bit ops, immediate arithmetic).
- `tests/thor_auto_migrator_phase17_test.py` — regression test suite for Milestone M15.182 production integration, step set slot 204, receipt index 172 entries, and baseline 181 integrity.
- `src/tools/hybrid/m68k_03abda_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Title / Intro Sequence Stepper (`0x03ABDA..0x03AC16`, proven caller `0x03AAA8`), verified in scenarios `attract_3600` and `canonical_3600` (119 native calls, 0 fallbacks, 526 stepped instructions, 0 diff frames).
- `tests/m68k_03abda_instruction_step_test.cpp` — unit test suite for `0x03ABDA` verifying caller gating, corrupt ROM rejection, child call dispatch, and multi-frame branch stepping.
- `data/thor-evidence/03abda-replay-differential-20261101.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x03ABDA` (Milestone M15.181).
- `tests/thor_auto_migrator_phase16_test.py` — regression test suite for Milestone M15.181 production integration, step set slot 203, receipt index 171 entries, and baseline 180 integrity.
- `src/tools/hybrid/m68k_03ab98_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Title / Intro Entity Initialization Sequence Leaf (`0x03AB98..0x03ABDA`, proven caller `0x03AA28`), verified in scenarios `attract_3600` and `canonical_3600` (1 native call, 0 fallbacks, 10 stepped instructions, 0 diff frames).
- `tests/m68k_03ab98_instruction_step_test.cpp` — unit test suite for `0x03AB98` verifying caller gating, corrupt ROM rejection, child call dispatch, and entity RAM flag updates.
- `data/thor-evidence/03ab98-replay-differential-20261101.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x03AB98` (Milestone M15.180).
- `tests/thor_auto_migrator_phase15_test.py` — regression test suite for Milestone M15.180 production integration, step set slot 202, receipt index 170 entries, and baseline 179 integrity.
- `src/tools/hybrid/m68k_03ac16_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Title / Intro Entity Initialization Sequence Leaf (`0x03AC16..0x03AC68`, proven caller `0x03AA28`), verified in scenarios `attract_3600` and `canonical_3600` (1 native call, 0 fallbacks, 12 stepped instructions, 0 diff frames).
- `tests/m68k_03ac16_instruction_step_test.cpp` — unit test suite for `0x03AC16` verifying caller gating, corrupt ROM rejection, child call dispatch, and entity RAM flag updates.
- `data/thor-evidence/03ac16-replay-differential-20261031.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x03AC16` (Milestone M15.179).
- `tests/thor_auto_migrator_phase14_test.py` — regression test suite for Milestone M15.179 production integration, step set slot 201, receipt index 169 entries, and baseline 178 integrity.
- `src/tools/hybrid/m68k_03ac6e_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Title / Intro Entity Initialization Sequence Leaf (`0x03AC6E..0x03AC92`, proven caller `0x03AA28`), verified in scenarios `attract_3600` and `canonical_3600` (1 native call, 0 fallbacks, 6 stepped instructions, 0 diff frames).
- `tests/m68k_03ac6e_instruction_step_test.cpp` — unit test suite for `0x03AC6E` verifying caller gating, corrupt ROM rejection, child call dispatch, and entity RAM flag updates.
- `data/thor-evidence/03ac6e-replay-differential-20261031.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x03AC6E` (Milestone M15.178).
- `tests/thor_auto_migrator_phase13_test.py` — regression test suite for Milestone M15.178 production integration, step set slot 200, receipt index 168 entries, and baseline 177 integrity.
- `src/tools/hybrid/m68k_00d962_instruction_step.hpp/.cpp` — exact ROM-checked, 3-caller-gated instruction-stepped helper for VDP Plane Clear/Fill Rectangle Loop Leaf (`0x00D962..0x00D9A4`), verified across all 5 scenarios (2 canonical / 2 attract calls, 0 fallbacks, 5,016 stepped instructions, 0 diff frames).
- `tests/m68k_00d962_instruction_step_test.cpp` — unit test suite for `0x00D962` verifying caller gating, corrupt ROM rejection, and VDP rectangular fill pattern loop.
- `data/thor-evidence/00d962-replay-differential-20261028.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00D962`.
- `src/tools/hybrid/m68k_0007c4_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for VDP DMA/Command FIFO Enqueue Leaf (`0x0007C4..0x0007E2`), registered slot 196 marked deferred pending interrupt mask / SR latency calibration.
- `tests/m68k_0007c4_instruction_step_test.cpp` — unit test suite for `0x0007C4` verifying caller gating, corrupt ROM rejection, and FIFO queue updates.
- `src/tools/hybrid/m68k_007198_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Entity Friction & Inertia Velocity Scaler Leaf (`0x007198..0x0071DA`), registered slot 199 marked deferred pending scheduler cycle timing calibration (confirming historical `ADR-M15-HOLD-007198`).
- `tests/m68k_007198_instruction_step_test.cpp` — unit test suite for `0x007198` verifying caller gating, corrupt ROM rejection, and velocity scaling calculations.

## Automated Routine Migration Tooling (tools/thor_auto_migrator/)
- thor-auto-migrator.py — Top-level CLI launcher for the automated migrator.
- tools/thor_auto_migrator/__init__.py — Package initialization and versioning.
- tools/thor_auto_migrator/config.py — Migration limits, canonical ROM SHA256, and known hold/deferral tables.
- tools/thor_auto_migrator/state_machine.py — 3-dimensional state machine (historical, discovery, migration), attempt tracking, and persistent JSON database.
- tools/thor_auto_migrator/status_indexer.py — Comprehensive historical exclusions aggregator generating data/thor-auto-migrator/historical-status-index.json.
- tools/thor_auto_migrator/status_adapter.py — Authoritative status ledger adapter with active supersession and regression verification.
- tools/thor_auto_migrator/doctor.py — Environment and toolchain capability diagnostics (ROM, active set, censuses, hybrid cores, WSL, strict oracles).
- tools/thor_auto_migrator/discovery.py — Two-stage candidate discovery engine across all 5 runtime censuses with zero contamination guarantee.
- tools/thor_auto_migrator/m68k_opcodes.py — M68K instruction decoder and auto-safe opcode whitelist.
- tools/thor_auto_migrator/ir.py — Instruction and RoutineIR representation with branch/exit detection.
- tools/thor_auto_migrator/precheck.py — Conservative safety validator enforcing authoritative exclusions and hardware polling checks.
- tools/thor_auto_migrator/codegen.py — Native C++ instruction step generator with self-checking.
- tools/thor_auto_migrator/testgen.py — Deterministic unit test generator enforcing Rule 7 non-elidable TEST_ASSERT assertions.
- tools/thor_auto_migrator/build_gate.py — Isolated build and unit test execution gate.
- tools/thor_auto_migrator/ownership.py — Native C++ routine ownership index classifier (FULL_ROUTINE, PARTIAL_ROUTINE, EMBEDDED_BLOCK, SHARED_TAIL, INTERNAL_ENTRY, UNKNOWN).
- tools/thor_auto_migrator/runner_build.py — Isolated Linux runner build adapter compiling/linking staged overlays into build/auto-migrator/<target>/linux-runner/.
- tools/thor_auto_migrator/ab_gate.py — Strict differential A/B replay runner and 8-stream hash verifier.
- tools/thor_auto_migrator/cache.py — Evidence and execution cache manager with 8 authoritative keys preventing redundant analysis.
- tools/thor_auto_migrator/transaction.py — Isolated transaction directory manager (build/auto-migrator/<target>/).
- tools/thor_auto_migrator/fetch_contract_audit.py — Static pattern validator auditing opcode fetch and PC advancement contracts across hybrid step helpers.
- tools/thor_auto_migrator/run_03ac6e_verification.py — Target-specific reachability and strict differential A/B verification runner for prototype targets.
- tools/thor_auto_migrator/cli.py — CLI commands implementation (doctor, candidates, explain, scan, inspect, dry-run, migrate, status).
- data/thor-auto-migrator/historical-status-index.json — Machine-readable index of all authoritative project exclusions and active statuses.
- data/thor-auto-migrator/native-ownership-index.json — Machine-readable native C++ code coverage and routine ownership index.
- data/thor-auto-migrator/state.json — Persistent state database recording all evaluated targets and 3-dimensional status dimensions.
- docs/AUTO_MIGRATOR.md — Documentation guide for thor-auto-migrator operation, doctor diagnostics, and failure recovery.
- tests/thor_auto_migrator_test.py — Unit test suite covering StatusAdapter, supersession, doctor, candidate discovery, and state machine invariants.
- tests/thor_auto_migrator_phase11_test.py — Regression test suite for dual-run full-baseline A/B gate, native ownership, and cache invalidation.
- tests/thor_auto_migrator_baseline_regression_test.py — Regression test suite for baseline crash classification, contract fault repair verification, and A/B readiness.

- `src/tools/hybrid/m68k_00701a_instruction_step.hpp/.cpp` — Native C++ instruction stepper helper for Player Action 0x74 Fall Handler, Actions 0x76/0x78 & Landing Transition (`0x00701A..0x007074`, 90 ROM bytes, 21 instructions). Handles primary entry `0x00701A`, secondary landing entry `0x007066`, and external secondary entries `0x007050` / `0x00705A` natively with zero ROM fallbacks.
- `tests/m68k_00701a_instruction_step_test.cpp` — Dedicated non-elidable unit test suite covering carry-clear airborne path, landing branch, SFX dispatch path, tail transfer to `0x0062CC`, caller rejection, and corrupt ROM rejection.
- `data/thor-evidence/00701a-gameplay-checkpoint-ab-20261104.json` — Machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00701A` across 1,592 continuation frames from `canonical_frame2008.state` (8/8 streams match, 48 native calls, 0 ROM fallbacks).
- `src/tools/hybrid/m68k_007198_instruction_step.hpp/.cpp` — Native C++ instruction stepper helper for Player Inertia & Velocity Damping Leaf Subroutine (`0x007198..0x0071DA`, 66 ROM bytes, 27 instructions). Implements shift, clamp, and conditional branch damping logic with instruction-exact Musashi cycle accounting; promoted to Active Native Helper #190 in Milestone M15.194.
- `src/tools/hybrid/m68k_0064c4_step_ops.hpp/.cpp` — Shared M68000 arithmetic and branch step operations, including cycle-exact `step_asr_l_1` (+14 master clock dynamic adjustment) and `step_bcc_s` (-14 master clock not-taken branch adjustment).
- `tests/m68k_007198_instruction_step_test.cpp` — Dedicated unit test suite covering velocity damping, positive/negative clamping, zero thresholds, branch cycles, and Frame 245 regression fixture with non-elidable `TEST_ASSERT` assertions.
- `data/thor-evidence/007198-timing-rejection-20261104.json` — Machine-readable verification receipt documenting initial differential A/B evaluation of candidate `0x007198`, Frame 245 28-cycle drift isolation, and preserved Baseline 189 historical hold (M15.193).
- `data/thor-evidence/007198-gameplay-checkpoint-ab-20261104.json` — Machine-readable verification receipt proving bit-for-bit replay equivalence of `0x007198` across 1,592 continuation frames from `canonical_frame2008.state` (8/8 streams match, 49 native calls, 0 ROM fallbacks, 921 stepped instructions) and baseline promotion to 190 active helpers (M15.194).
- `src/tools/hybrid/m68k_0070f8_step_ops.hpp/.cpp` — Specialized M68000 instruction step operations for `0x0070F8`, implementing `step_tst_l_disp`, `step_cmpi_w_disp`, and `step_andi_w_disp` with exact operand direction and CCR calculation.
- `src/tools/hybrid/m68k_0070f8_instruction_step.hpp/.cpp` — Native C++ instruction stepper helper for Player Jump Collision Interaction Callback (`0x0070F8..0x007112`, 26 ROM bytes, 7 instructions). Displaces callback registered into `$FF1940` by `0x007074` and invoked at `0x00BE5A`, extending the contiguous native jump/fall pipeline `[0x006FE4, 0x007112)` (302 contiguous ROM bytes); promoted to Active Native Helper #191 in Milestone M15.195.
- `tests/m68k_0070f8_instruction_step_test.cpp` — Dedicated non-elidable unit test suite covering caller gating, child call dispatch to `0x00BE98`, altitude active branch, action 0x66 branch, and ground contact mask application with `TEST_ASSERT` assertions.
- `data/thor-evidence/0070f8-gameplay-checkpoint-ab-20261104.json` — Machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0070F8` across 1,592 continuation frames from `canonical_frame2008.state` (8/8 streams match, 3 native calls, 0 ROM fallbacks, 21 stepped instructions) and baseline promotion to 191 active helpers (M15.195).
- `src/tools/hybrid/m68k_00e2a2_step_ops.hpp/.cpp` — Specialized M68000 instruction step operations for `0x00E2A2`, implementing `step_tst_w_reg`, `step_move_l_reg_predec_sp`, `step_move_w_reg_reg`, `step_mulu_w_reg_reg`, `step_swap_reg`, `step_move_l_postinc_sp_reg`, and `step_rts` with cycle-exact unsigned MULU dynamics.
- `src/tools/hybrid/m68k_00e2a2_instruction_step.hpp/.cpp` — Native C++ instruction stepper helper for Bounded Pseudo-Random Number Scaling Subroutine `GetRandomBounded(D0)` (`0x00E2A2..0x00E2BA`, 24 ROM bytes, 12 words, 10 instructions). Gated across 5 proven continuation callers (`0x01A4FC`, `0x01B242`, `0x01B7D6`, `0x01BBDC`, `0x01BF68`), handles child LFSR PRNG call to `0x00E268`, and provides uniform bounded random integer scaling; promoted to Active Native Helper #192 in Milestone M15.196.
- `tests/m68k_00e2a2_instruction_step_test.cpp` — Dedicated non-elidable unit test suite covering caller set gating, zero-bound early exit, and normal scaling with child return (including MSB-set `0xC000` test vector) with `TEST_ASSERT` assertions.
- `data/thor-evidence/00e2a2-gameplay-checkpoint-ab-20261104.json` — Machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00E2A2` across 1,592 continuation frames from `canonical_frame2008.state` (8/8 streams match, 14 native calls, 0 ROM fallbacks, 140 stepped instructions) and baseline promotion to 192 active helpers (M15.196).

- `src/tools/hybrid/m68k_00e268_step_ops.hpp/.cpp` — Specialized M68000 instruction step operations for `0x00E268`, implementing `step_move_w_reg_predec_sp`, `step_move_w_abs_reg`, `step_move_w_reg_reg`, `step_rol_w_imm_reg`, `step_eor_w_reg_reg`, `step_lsr_w_imm_reg`, `step_addx_w_reg_reg`, `step_move_w_reg_abs`, `step_move_w_postinc_sp_reg`, and `step_rts` with exact Musashi cycle and CCR calculation.
- `src/tools/hybrid/m68k_00e268_instruction_step.hpp/.cpp` — Native C++ instruction stepper helper for Core 16-bit Galois/Fibonacci LFSR Pseudo-Random Number Generator `GetRandom16()` (`0x00E268..0x00E2A2`, 58 ROM bytes, 29 words, 25 instructions). Manages state seed at `$00FF1928.W`, preserves D1.L across stack, and provides uniform 16-bit random word in D0.W; promoted to Active Native Helper #193 in Milestone M15.197.
- `src/tools/hybrid/CMakeLists_captures.cmake` — Sub-CMake module containing extracted test capture and oracle tool target definitions to maintain `src/tools/hybrid/CMakeLists.txt` strictly <= 500 lines.
- `tests/m68k_multiply_semantics_regression_test.cpp` — Dedicated test suite verifying M68000 unsigned MULU vs signed MULS operations, 32-bit register replacement, condition code flags, and dynamic cycle calculation against Musashi reference.
- `tests/m68k_00e268_instruction_step_test.cpp` — Dedicated non-elidable unit test suite covering 0x00E268 alignment checks, corrupt ROM rejection, stack preservation, and full 25-instruction execution cycle with non-elidable `TEST_ASSERT` assertions.
- `data/thor-evidence/00e268-gameplay-checkpoint-ab-20261104.json` — Machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00E268` across 1,592 continuation frames from `canonical_frame2008.state` (8/8 streams match, 61 native calls, 0 ROM fallbacks, 1,525 stepped instructions) and baseline promotion to 193 active helpers (M15.197).

- `src/tools/hybrid/m68k_0083f8_step_ops.hpp/.cpp` — Specialized M68000 instruction step operations for `0x0083F8`, implementing indexed memory reading `step_move_w_indexed_disp_reg`, bit testing/setting `step_btst_imm_reg` / `step_bset_imm_reg`, and conditional loop branch operations with Musashi cycle calculations.
- `src/tools/hybrid/m68k_0083f8_instruction_step.hpp/.cpp` — Native C++ instruction stepper helper for Entity Collision Ring Buffer Scanner `ScanEntityCollisionRing` (`0x0083F8..0x00846C`, 116 ROM bytes, 58 words, 33 instructions). Recompiled and evaluated in Milestone M15.198; quarantined in `HISTORICAL_HOLD` due to accumulated micro-cycle drift shifting frame boundary.
- `tests/m68k_0083f8_instruction_step_test.cpp` — Dedicated non-elidable unit test suite covering 0x0083F8 match exit condition, ring exhaustion, sequential calls, register preservation, and memory read assertions with non-elidable `TEST_ASSERT` assertions.
- `data/thor-evidence/0083f8-timing-rejection-20261104.json` — Machine-readable rejection receipt recording cycle-timing divergence forensics of `0x0083F8` across 1,592 continuation frames from `canonical_frame2008.state` (28 calls, 3,933 instructions displaced, 0 contract faults, 28-cycle boundary drift) and quarantine into `HISTORICAL_HOLD` (M15.198).
- `tests/hybrid_registry_test.cpp` — Focused unit test suite verifying `HybridExecutionRegistry` player ABI canon, state tracking, main loop tick, divergence oracle, and entity oscillation region dispatch.
- `tools/candidate_selection_gate.py` — Fail-closed 7-step candidate selection gate preventing accidental promotion or migration of historical failed, held, active, or overlapping routines without approved ADR supersession tokens (M15.199).
- `tests/candidate_selection_gate_test.py` — Dedicated test suite verifying `CandidateSelectionGate` rejection of `0x0083F8`, `0x007DC8`, `0x00BCC4`, `0x00C012`, active routines, overlapping ranges, high execution count invariance, and authorized supersession (M15.199).
- `data/thor-evidence/canonical-beach-frame3600-checkpoint-20261105.json` — Machine-readable checkpoint verification receipt for `canonical_beach_frame3600.state` establishing 100% bit-for-bit deterministic resume proof across 600 continuation frames, 26 new ROM procedures, and confirmed combat gameplay mechanics (M15.199).

- `src/tools/hybrid/m68k_00b610_step_ops.hpp/.cpp` — Specialized M68000 instruction step operations for `0x00B610`, implementing arithmetic shift right `step_asr_w_imm_reg` (`0xE448`, count=2 with `count * 14` cycle calculation), address addition `step_adda_w_reg_reg`, register-to-stack / stack-to-register moves `step_move_w_reg_predec_sp` / `step_move_w_postinc_sp_reg`, and sub-piece table lookups.
- `src/tools/hybrid/m68k_00b610_instruction_step.hpp/.cpp` — Native C++ instruction stepper helper for Player Weapon Dynamic Collision & Sub-Piece Animation Sequencer (`0x00B610..0x00B66A`, 90 ROM bytes, 26 instructions + 10-byte lookup table). Displaces caller `0x00B3A4`, coordinates child `0x00B6A6`, correctly applies `ASR.W #2, D0` and cycle-adjusted `movem.l` operations; promoted to Active Native Helper #194 in Milestone M15.200.
- `tests/m68k_00b610_instruction_step_test.cpp` — Dedicated non-elidable unit test suite covering 0x00B610 instruction stepping, arithmetic shift right bit-exact behavior, child call dispatch, register preservation, and stack alignment with non-elidable `TEST_ASSERT` assertions.
- `data/thor-evidence/00b610-dual-scenario-ab-20261105.json` — Machine-readable dual-scenario verification receipt proving bit-for-bit replay equivalence across BOTH authentic scenarios: Scenario A (`canonical_frame2008.state`, 1,592 frames, 8/8 streams match) and Scenario B (`canonical_beach_frame3600.state`, 600 frames, 8/8 streams match, 5 native calls, 120 stepped instructions, 0 ROM fallbacks), establishing baseline promotion to 194 active helpers (`ACTIVE_SET_SHA256 = 89c33e7e9946e15ddf58b2611fbcc8c928897b9fa01c10dc7d50b69a978ec43f`).


- `src/tools/hybrid/m68k_0082ae_instruction_step.hpp/.cpp` — Native C++ instruction stepper helper for Player Interaction / Item Contact Scanner & Landing Dispatcher (`0x0082AE..0x0082F8`, 74 ROM bytes, 37 ROM words, 21 instructions + 16 extension words). Displaces caller `0x007FE8`, coordinates child `0x00B9EC` (registered return PC `0x0082BE`), and executes landing dispatch logic; promoted to Active Native Helper #195 in Milestone M15.201.
- `tests/m68k_0082ae_instruction_step_test.cpp` — Dedicated non-elidable unit test suite covering 0x0082AE instruction stepping, child call dispatch to `0x00B9EC`, stack preservation, and landing dispatch with non-elidable `TEST_ASSERT` assertions.
- `tests/m68k_00ceea_cmpa_regression_test.cpp` — Dedicated regression test suite verifying `CMPA.L #imm, A0` vs `A1` condition code register differences across line buffer sites `0x00D2EC`, `0x00D316`, and `0x00D336` preventing M15.200 register mismatch regressions.
- `data/thor-evidence/0082ae-dual-scenario-ab-20261105.json` — Machine-readable dual-scenario verification receipt proving bit-for-bit replay equivalence across BOTH authentic scenarios: Scenario A (`canonical_frame2008.state`, 1,592 frames, 8/8 streams match) and Scenario B (`canonical_beach_frame3600.state`, 600 frames, 8/8 streams match, 12 dynamic entries, 156 stepped instructions, 0 ROM fallbacks), establishing baseline promotion to 195 active helpers (`ACTIVE_SET_SHA256 = efa86699a2d3714e23f51b163de3f067e0da2bd59eea996dec77035afb377ffb`).

- `src/tools/hybrid/m68k_00b66a_step_ops.hpp/.cpp` — Specialized M68000 instruction step operations for `0x00B66A`, implementing address subtraction `step_suba_w_reg_areg` (`SUBA.W D0, A4`) and immediate addition `step_addq_w_imm_reg` (`ADDQ.W #1, D0`).
- `src/tools/hybrid/m68k_00b66a_instruction_step.hpp/.cpp` — Native C++ instruction stepper helper for Player Weapon Dynamic Collision & Sub-Piece Animation Sequencer #2 (`0x00B66A..0x00B6A6`, 60 ROM bytes, 30 ROM words, 19 instructions). Displaces caller `0x00B3CA` (displacement `0x029E`), coordinates child `0x00B6A6`, and steps animation sequencing logic; promoted to Active Native Helper #196 in Milestone M15.203.
- `tests/m68k_00b66a_instruction_step_test.cpp` — Dedicated non-elidable unit test suite covering 0x00B66A instruction stepping, caller validation, displacement calculation, branch taken paths, child call dispatch, and register preservation with non-elidable `TEST_ASSERT` assertions.
- `tests/m68k_00b856_instruction_step_test.cpp` — Dedicated non-elidable unit test suite covering 0x00B856 composite instruction stepping, prefix entry verification at `0x00B852`, miss paths, and resume paths with non-elidable `TEST_ASSERT` assertions.
- `data/thor-evidence/00b66a-dual-scenario-ab-20261105.json` — Machine-readable dual-scenario verification receipt proving bit-for-bit replay equivalence across BOTH authentic scenarios: Scenario A (`canonical_frame2008.state`, 1,592 frames, 8/8 streams match) and Scenario B (`canonical_beach_frame3600.state`, 600 frames, 8/8 streams match, 5 native calls, 90 stepped instructions, 0 ROM fallbacks), establishing baseline promotion to 196 active helpers (`ACTIVE_SET_SHA256 = 17396fdaf9c566d36ca8317d40bb334f0f3467c73f1aebe0e26a36f8d4951867`).

- `tools/thor_evidence/receipt_verifier.py` — Deterministic machine receipt verification pipeline validating 64-hex SHA-256 formats, computing physical disk byte hashes, verifying runner SHA formats and paths, detecting duplicate Z80 RAM digests, inspecting and recalculating SHA-256 of all 48 stream files directly from disk across 3 scenarios x 2 runs A/B x 8 streams (`STREAM_ARTIFACT_COUNT = 48`, `STREAM_ARTIFACTS_VALID = 48`), and differentiating arbitrary hash mismatches from truncated logger prefix hallucinations (`HALLUCINATED_SHA256_SUFFIX`) with zero tolerance for LLM-transcribed cryptographic claims (M15.204..M15.210).
- `tests/receipt_verifier_test.py` — Dedicated unit test suite verifying `ReceiptVerifier` functionality with 17 comprehensive test cases covering 1-byte corruptions, missing files, omitted streams, duplicate Z80 digests, fabricated SHA suffixes, byte size mismatches, invalid runner SHA formats, stream artifact count mismatches, and physical verification of superseded, active, and qualification receipt files (M15.204..M15.210).
- `data/thor-evidence/canonical-beach-frame3600-checkpoint-20261105.m15_199_superseded.json` — Immutable byte-exact copy of the original superseded M15.199 receipt with authentic LF newlines preserving historical evidence integrity (`OLD_RECEIPT_CONTENT_SHA256 = 310645d2bd4502ea1a0e6aaf1433aa35fbed482c0dcaff57c4ed81e15ed346e2`).
- `data/thor-evidence/supersession-beach-frame3600-20261105.json` — Cryptographic supersession record linking the superseded M15.199 receipt and updated M15.204 active receipt with verified physical disk SHA-256 values.
- `tools/audit_steppers.py` — AST-based static instruction stepper semantic analysis tool auditing all 196 active production helpers for multi-word operand ordering, branch synchronization, elidable test assertions, operand directionality, MULU/MULS signedness, and CMPA target registers. Updated in M15.205 with `verify_production_stepper_inventory()`, 20 validation gate fixtures, and complete 196-helper inventory enforcement.
- `src/tools/hybrid/m68k_00b45c_step_ops.hpp/.cpp` — Specialized M68000 instruction step operations for `0x00B45C`, implementing memory test `step_tst_b_disp_reg` (`TST.B d16(An)`), branch lower or same `step_bls_s` (`BLS.S`), and sub-piece parameter table lookups.
- `src/tools/hybrid/m68k_00b45c_instruction_step.hpp/.cpp` — Native C++ instruction stepper helper for Player Weapon Sub-Piece Animation Sequencer #1 (`0x00B45C..0x00B4C8`, 108 ROM bytes, 38 instructions + 54-byte lookup tables). Displaces caller `0x00B394` (displacement `0x00C6`), coordinates child `0x00B6A6`, and steps animation sequencing logic. Repaired in M15.205 for authentic `DC 40` `ADD.W D0, D6` ($3 \times i$ index) and caller displacement `0x00C6`. Registered disabled in slot 217 awaiting authentic scenario reachability.
- `tests/m68k_00b45c_instruction_step_test.cpp` — Dedicated non-elidable unit test suite covering 0x00B45C instruction stepping, caller validation against authentic ROM displacement, variant lookups, table 1 and table 2 dispatch paths with $3 \times i$ index verification, child call dispatch to `0x00B6A6`, and register preservation with non-elidable `TEST_ASSERT` assertions (4/4 PASS).
- `tools/thor_auto_migrator/production_inventory.py` — Dynamic production inventory discovery and validation module for the auto-migrator pipeline (M15.206). Eliminates manual versioned list imports, parses active ranges from `native-ownership-index.json`, correlates with C++ selectable/disabled wiring in `m68k_gameplay_step_registry.cpp` and `m68k_gameplay_step_set.cpp`, and verifies existence of physical C++ source files on disk.
- `tests/production_inventory_test.py` — Comprehensive regression test suite for `production_inventory.py` implementing 7 negative validation fixtures (active omission, disabled inclusion, duplicate registration, invalid range, missing source, historical hold enabled, adapter disagree) and verifying 100% active set consistency (7/7 PASS).
- `src/tools/hybrid/m68k_00302e_instruction_step.hpp/.cpp` — Native C++ instruction stepper helper for Palette Blender & 3-Channel RGB Interpolator (`0x00302E..0x003064`, 54 ROM bytes, 15 instructions). Recompiles register save/restore (`MOVEM.L`), dual source palette reading `(A1)+` and `(A2)+`, child invocation of color channel blender #171 `0x003064` (3 calls per color word), destination writing `(A0)+`, palette line loop `DBRA D1`, and VDP dirty flag setting `BSET.B #0, ($00FF164D)`. Registered in slot 218.
- `tests/m68k_00302e_instruction_step_test.cpp` — Dedicated non-elidable unit test suite for `0x00302E` covering 16-color loop execution, output color blending, register restoration, dirty flag assertion, non-matching PC rejection, disabled bypass mode, and 15 authentic ROM caller validations using `TEST_ASSERT` (4/4 PASS).
- `src/tools/hybrid/m68k_00f0d2_instruction_step.hpp/.cpp` — Native C++ instruction stepper helper for Euclidean 2D Distance Calculator (`0x00F0D2..0x00F0EC`, 26 ROM bytes, 13 words, 10 instructions). Recompiles register save/restore (`MOVEM.L D3-D4`), coordinate subtractions (`SUB.W`), signed multiplications (`MULS.W`) using bit-exact Musashi cycle formula $38 \times 7 + 14 \times \text{bits}$, sum of squares accumulation (`ADD.L`), child integer square root dispatch (`BSR.W 0x00E6BA`), stack restore with postincrement dummy read, and return (`RTS`). Registered in slot 219 (M15.212).
- `tests/m68k_00f0d2_instruction_step_test.cpp` — Dedicated non-elidable unit test suite for `0x00F0D2` covering Euclidean distance vector arithmetic ($30^2 + 40^2 \rightarrow 50$), signed multiplication limits, negative numbers, zero handling, MOVEM stack pointer pre-decrement/post-increment (-8/+8 bytes), +112 master clocks extra, dummy read at new SP, stack sentinels (`0xCAFEBABE`, `0xDEADBEEF`), register preservation, child `0x00E6BA` invocation, caller validation against authentic ROM caller `0x02581C`, and metrics recording (M15.213).
- `tools/run_00f0d2_three_scenario_ab.py` — Automated three-scenario differential A/B replay runner for `0x00F0D2` across Scenario A (1,592 frames), Scenario B (600 frames), and Scenario C (1,800 frames), verifying 24/24 streams bit-for-bit exact match and 0 ROM fallbacks (M15.213).
- `data/thor-evidence/00f0d2-three-scenario-qualification-20261107.json` — Machine-readable three-scenario qualification receipt documenting bit-exact single-variable differential A/B equivalence across Scenario A (1,592 frames, 8/8 streams match), Scenario B (600 frames, 8/8 streams match), and Scenario C (1,800 frames, 8/8 streams match, 1,202 native calls, 12,020 instructions stepped, 1,202 child calls, 0 ROM fallbacks), verified by `ReceiptVerifier` across 52 physical disk files (M15.213).
- `src/tools/hybrid/m68k_gameplay_step_metrics.cpp` — Global gameplay step metrics serialization implementation split from `m68k_gameplay_step_set.cpp` to maintain source files $\le 500$ lines. Writes JSON metrics for all registered gameplay helper steppers (including slots 218 `0x00302E` and 219 `0x00F0D2`).
- `scratch/run_00302e_dual_scenario.py` — Dual-scenario differential replay test script executing single-variable A/B runs across Scenario A (1592 frames) and Scenario B (600 frames) on WSL, verifying bit-for-bit identity across all 8 stream artifacts.
- `tools/prepare_target_active_session.py` — Generates authentic target-active verification scenario `target_active_beach_1800` (1,800 frames resuming from `canonical_beach_frame3600.state`, covering original timeline frames 3601..5400) by slicing and renumbering input polls and frame landmarks from `beach_combat_transition_6000` (M15.207).
- `tools/run_target_active_ab.py` — Single-variable differential A/B replay runner for `target_active_beach_1800` on WSL, comparing 196 baseline vs 196 + 00302E, verifying native execution telemetry (66 calls, 11,880 stepped instructions, 3,168 child calls), and evaluating raw byte-exact equivalence across all 8 streams (M15.207).
- `data/thor-evidence/00302e-three-scenario-qualification-20261106.json` — Machine-readable three-scenario qualification receipt documenting bit-exact single-variable differential A/B equivalence across Scenario A (1,592 frames, 8/8 streams match), Scenario B (600 frames, 8/8 streams match), and Scenario C (1,800 frames, 8/8 streams match, 66 native calls, 11,880 instructions stepped, 3,168 child calls, 0 ROM fallbacks), verified by `ReceiptVerifier` (M15.209).
- `src/tools/hybrid/replay_finalization.hpp/.cpp` — Explicit runner finalization and replay lifecycle coordinator (`finalize_diagnostic_replay()`) ensuring all diagnostic traces, journals, external event streams, and manifests are durably flushed, verified, and closed before calling `retro_unload_game()` and `retro_deinit()`. Completely eliminates unsafe `std::quick_exit(0)` and prevents buffered file truncations (M15.214).
- `tests/gameplay_session_termination_test.py` — Dedicated pytest suite verifying clean return codes (exit 0), `SAFE_CLEANUP_SUCCESS` teardown status, full JSONL record counts, and fail-closed termination under journal truncation, missing checkpoints, corrupted states, and manifest mismatches (M15.214).
- `tests/m68k_0099d6_timing_regression_test.cpp` — Cycle-timing regression test verifying M68000 bus-refresh cycle suppression and shift timing order for helper `0x0099D6` (`LSL.W D3, D4`), proving that adding shift cycles before invoking `skip_bus_refresh()` correctly suppresses refresh when crossing the 896-cycle boundary (M15.214).
- `data/thor-evidence/00f0d2-three-scenario-qualification-20261107.json` — Machine-readable three-scenario qualification receipt documenting bit-exact single-variable differential A/B equivalence across Scenario A (1,592 frames, 8/8 streams match), Scenario B (600 frames, 8/8 streams match), and Scenario C (1,800 frames, 8/8 streams match, 1,202 native calls, 12,020 instructions stepped, 1,202 child calls, 0 ROM fallbacks), verified by `ReceiptVerifier` across 56 distinct physical disk files (57 total checks: 48 streams + 2 checkpoints + 3 inputs + 1 ROM + 1 Core + 1 Runner) (M15.214).
- `src/tools/hybrid/m68k_00d168_instruction_step.hpp/.cpp` — Native C++ instruction stepper helper for Camera Viewport Scroll Offset Wrapper / Coordinate Delta Accumulator (`0x00D168..0x00D266`, 254 ROM bytes, 73 instructions). Evaluates camera tracking active flag (`btst #0, 39(%a0)`), conditionally branches to RTS (`0x00D264`), accumulates coordinate deltas (`add.w (%a0), %d1` and `add.w 4(%a0), %d2`), and branches to viewport clamping logic (`0x00D188..0x00D264`) via `step_camera::step_camera_scroll_op()`. Registered in slot 220 (M15.215).
- `tests/m68k_00d168_instruction_step_test.cpp` — Dedicated non-elidable unit test suite for `0x00D168` covering tracking-enabled branch (delta accumulation and clamping branch), tracking-disabled branch (immediate RTS bypass), caller verification (`0x02EB24`), ROM opcode prefix verification, and register preservation using `TEST_ASSERT` (M15.215).
- `tools/run_00d168_three_scenario_ab.py` — Automated three-scenario differential A/B replay runner for `0x00D168` across Scenario A (1,592 frames), Scenario B (600 frames), and Scenario C (1,800 frames), verifying 24/24 streams bit-for-bit exact match, 114 native calls, 5,965 stepped instructions, 0 fallbacks, 0 contract faults, and generating authoritative evidence receipt (M15.215).
- `data/thor-evidence/00d168-three-scenario-qualification-20261107.json` — Machine-readable three-scenario qualification receipt documenting bit-exact single-variable differential A/B equivalence across Scenario A (1,592 frames, 8/8 streams match), Scenario B (600 frames, 8/8 streams match), and Scenario C (1,800 frames, 8/8 streams match, 114 native calls, 5,965 instructions stepped, 0 ROM fallbacks, 0 contract faults), verified by `ReceiptVerifier` across 56 distinct physical disk files (57 total checks: 48 streams + 2 checkpoints + 3 inputs + 1 ROM + 1 Core + 1 Runner) (M15.215, ownership updated in M15.216).
- `tests/m68k_00d188_semantic_regression_test.cpp` — Dedicated semantic regression test suite for `0x00D188` and `0x00D168` covering caller isolation, mutual caller rejection, tracking flag branch, coordinate delta accumulation, viewport boundary clamping, RTS return state, and register preservation using non-elidable `TEST_ASSERT` (M15.216).
- `data/thor-evidence/00d168-ownership-supplement-20261107.json` — Machine-readable ownership supplement receipt documenting the dynamic step histogram for candidate `0x00D168` across 114 native calls (570 prefix steps + 5,395 shared-tail steps = 5,965 total native steps, 0 early RTS branches), resolving M15.216 evidence accounting gaps without repeating full replays (M15.217).
- `src/tools/hybrid/m68k_00dbca_instruction_step.hpp/.cpp` — Native C++ instruction stepper helper for Camera Viewport Buffer 2D Block Copy Math Leaf (`0x00DBCA..0x00DC26`, 92 ROM bytes, 28 instructions). Simulates tile buffer 2D row/column block copy, coordinate pitch indexing, register shifts, and loop control via `dbra`. Enforces exact Musashi bus refresh timing bridge and caller whitelist (`0x00DB64`, `0x010130`). Registered in slot 221 (M15.217).
- `tests/m68k_00dbca_instruction_step_test.cpp` — Dedicated unit test suite for `0x00DBCA` covering caller whitelisting, ROM byte guard verification, empirical Scenario C execution fixture (62 steps, 2D tile block copy memory and register parity), and non-elidable `TEST_ASSERT` validation in Debug and Release (M15.217).
- `tools/run_00dbca_three_scenario_ab.py` — Automated three-scenario differential A/B replay runner for `0x00DBCA` across Scenario A (1,592 frames), Scenario B (600 frames), and Scenario C (1,800 frames), verifying 24/24 streams bit-for-bit exact match, 2 native calls, 235 stepped instructions, 0 fallbacks, 0 contract faults, and generating authoritative evidence receipt (M15.217).
- `data/thor-evidence/00dbca-three-scenario-qualification-20261107.json` — Machine-readable three-scenario qualification receipt documenting bit-exact single-variable differential A/B equivalence across Scenario A (1,592 frames, 8/8 streams match), Scenario B (600 frames, 8/8 streams match), and Scenario C (1,800 frames, 8/8 streams match, 2 native calls, 235 instructions stepped, 0 ROM fallbacks, 0 contract faults), verified by `ReceiptVerifier` across 56 distinct physical disk files (57 total checks: 48 streams + 2 checkpoints + 3 inputs + 1 ROM + 1 Core + 1 Runner) (M15.217).

- `src/tools/hybrid/m68k_entity_cluster_types.hpp` — Common type definitions, transfer descriptors, execution outcomes, and constant definitions for the `0x0254EA` entity state engine execution cluster (M15.219).
- `src/tools/hybrid/m68k_entity_cluster_context.hpp/.cpp` — Shared explicit `GuestExecutionContext` wrapping register state, guest memory reads/writes, CCR flags, and cycle tracking without altering guest semantics (M15.219).
- `src/tools/hybrid/m68k_entity_cluster_dispatch.hpp/.cpp` — Position-independent jump table calculation engine (`0x02575C..0x025766`) reconstructing index validation, relative table arithmetic, and fail-closed dispatch to 13 targets (M15.219).
- `src/tools/hybrid/m68k_entity_cluster_state0.hpp/.cpp` — State 0 timer decrement handler (`0x025896`) managing timer exhaustion, loopback transfers, and original return points (M15.219).
- `src/tools/hybrid/m68k_entity_cluster_target10.hpp/.cpp` — Target 10 proximity check & coordinate math handler (`0x0257EE`) coordinating child call `0x00F0D2` and common return joins (M15.219).
- `src/tools/hybrid/m68k_entity_cluster_target11_12.hpp/.cpp` — Target 11 elevation math (`0x02583E`) and Target 12 stun loopback (`0x025880`) handlers (M15.219).
- `src/tools/hybrid/m68k_entity_cluster_coordinator.hpp/.cpp` — Execution cluster coordinator orchestrating multi-module control transfers between native dispatcher and native state handlers with zero synthetic stack pushes or duplicate instructions (M15.219).
- `src/tools/hybrid/m68k_entity_cluster_metrics.hpp/.cpp` — Cluster telemetry and instruction step accounting tracking authentic ROM instruction displacement across all cluster modules (M15.219).
- `tests/m68k_entity_cluster_test.cpp` — Dedicated test suite with 7 test fixtures verifying jump table calculation, dispatcher stepping, State 0, Target 10, Targets 11/12, and coordinator chaining across Debug and Release (M15.219).
- `scratch/extract_cluster_cfg.py` — Automated CFG and basic block extraction script analyzing 1,608 bytes of ROM at `[0x0254EA, 0x025B32)` across 111 basic blocks and authentic Scenario C trace counts (M15.219).
- `scratch/rank_handlers.py` — Dynamic workload ranking script computing authentic Scenario C entries and instruction step displacement across all 13 jump table targets (M15.219).
- `src/tools/hybrid/m68k_02575c_instruction_step.hpp/.cpp` — Native C++ hybrid instruction stepper bridging the entity engine cluster (`0x02575C..0x02589E`) to the live M68K runner. Implements fail-closed unsupported state decline, child call coordination, State 0, State 7, Target 12, routing, and metrics serialization (M15.220, M15.221, M15.222).
- `src/tools/hybrid/m68k_02575c_step_ops.hpp` — Shared internal step declarations, CCR update helpers (`do_bset_b`, `do_bclr_b`, etc.), and instruction address boundaries for the modular `0x02575C` entity cluster stepper (M15.221, M15.222).
- `src/tools/hybrid/m68k_02575c_target10_step.cpp` — Dedicated native stepper module for Target 10 (`0x0257EE..0x02583C`), implementing proximity checks, coordinate loading, ROM child call `0x00F0D2`, and return continuation at `0x025822` (1,202 calls, 15,629 native steps) (M15.221, M15.222).
- `src/tools/hybrid/m68k_02575c_target11_step.cpp` — Dedicated native stepper module for Target 11 (`0x02583E..0x02587A`), implementing elevation/gravity integration, velocity updates, and intentional tail continuation to `0x00E7E8` (27 calls, 166 native steps) (M15.221, M15.222).
- `src/tools/hybrid/m68k_02575c_state1_step.cpp` — Dedicated native stepper module for State 1 (`0x0258A0..0x025940`), implementing attack/movement processing and child call `0x00BCD8` (229 calls, 3,196 steps) (M15.222).
- `src/tools/hybrid/m68k_02575c_state2_3_4_step.cpp` — Dedicated native stepper module for States 2, 3, and 4 (`0x025940..0x0259CE`), implementing deceleration, AI alert mode with child `0x00E502` and tail jump `0x00E7E8`, and reaction checks (M15.222).
- `src/tools/hybrid/m68k_02575c_state5_step.cpp` — Dedicated native stepper module for State 5 (`0x0259CE..0x025A72`), implementing entity lookup and displacement math (M15.222).
- `src/tools/hybrid/m68k_02575c_state6_step.cpp` — Dedicated native stepper module for State 6 (`0x025A72..0x025B22`), implementing bounding box checks and child calls `0x00B852` and `0x00E7E8` (32 calls, 618 steps) (M15.222).
- `tools/build_02575c_runner.py` — Build orchestration script generating the Linux gameplay runner `oasis_gameplay_session` (SHA256: `cdd7f58d0215fe9418b8fed5b1f84bd3c84a913c3a92e960f4c1707ed85819c8`) with slot 222 wired to `step_m68k_02575c` (M15.220, M15.221, M15.222).
- `tools/run_02575c_three_scenario_ab.py` — Automated three-scenario differential A/B replay runner for `0x02575C` across Scenario A (1,592 frames), Scenario B (600 frames), and Scenario C (1,800 frames), verifying 24/24 streams bit-for-bit exact match (M15.220, M15.221, M15.222).
- `data/thor-evidence/02575c-three-scenario-qualification-20261108.json` — Machine-readable three-scenario qualification evidence receipt for State 0, validated via `ReceiptVerifier` across 55 checked items (M15.220).
- `data/thor-evidence/02575c-three-scenario-qualification-m15-221-20261108.json` — Machine-readable three-scenario qualification evidence receipt for the expanded multi-handler cluster (State 0, Target 10, Target 11, Target 12), validated via `ReceiptVerifier` across all 56 distinct physical files (58 check operations) with status `VERIFIED` (M15.221).
- `data/thor-evidence/02575c-three-scenario-qualification-m15-222-20261108.json` — Machine-readable three-scenario qualification evidence receipt for M15.222 (States 0..4, 6..7, Targets 10..12), validated via `ReceiptVerifier` across all 56 distinct physical files (58 check operations) with status `VERIFIED` (M15.222).
- `tools/generate_02575c_m15_223_receipt.py` — Automated receipt generator for M15.223, capturing all 48 stream artifacts across Scenarios A, B, and C with exact byte counts and SHA256 hashes (M15.223).
- `data/thor-evidence/02575c-three-scenario-qualification-m15-223-20261108.json` — Machine-readable three-scenario qualification evidence receipt for M15.223 (subsystem completion, root CFG integration, transfer ledger, 24/24 streams), validated via `ReceiptVerifier` across 58 check operations with status `VERIFIED` (M15.223).
- `src/tools/hybrid/m68k_02575c_root_step.hpp/.cpp` — Dedicated native stepper module for Root Entry `0x0254EA`, implementing Mode 0 path into dispatcher `0x02575C`, child motion helper `0x00E682` coordination, `134(a6) == 0` check, preflight decline of complex paths (Modes 1..5, motion stack pop, non-zero 134), and metrics tracking (3,192 calls, 15,960 native steps) (M15.224).
- `tools/generate_02575c_m15_224_receipt.py` — Automated receipt generator for M15.224, capturing all 48 stream artifacts across Scenarios A, B, and C with exact byte counts and SHA256 hashes (M15.224).
- `data/thor-evidence/02575c-three-scenario-qualification-m15-224-20261108.json` — Machine-readable three-scenario qualification evidence receipt for M15.224 (State 5 safe paths, Root Entry `0x0254EA` execution, 53,508 stepped instructions, 24/24 streams), validated via `ReceiptVerifier` across 58 check operations with status `VERIFIED` (M15.224).
- `src/tools/hybrid/m68k_00ad76_step_ops.hpp/.cpp` — Shared memory, CCR status helpers, and 68000 branching operations for Sprite Engine Mode 0 Cluster (M15.225).
- `src/tools/hybrid/m68k_00ad76_instruction_step.hpp/.cpp` — Main instruction router and stage coordinator for `0x00AD76` cluster (M15.225).
- `src/tools/hybrid/m68k_00ad76_ingress_step.hpp/.cpp` — Ingress attribute checks and fast-path hidden RTS evaluation (`0x00AD76..0x00AD84`) (M15.225).
- `src/tools/hybrid/m68k_00ad76_attrib_step.hpp/.cpp` — Sprite orientation, flip, and palette bank parsing (`0x00ADDE..0x00AE3E`) (M15.225).
- `src/tools/hybrid/m68k_00ad76_camera_step.hpp/.cpp` — Camera coordinate transformation, screen-space clipping, dynamic tile budget management (`0x00AE40..0x00AECA`) (M15.225).
- `src/tools/hybrid/m68k_00ad76_piece_step.hpp/.cpp` — Sprite descriptor table resolution, multi-piece loop setup, piece coordinate parsing (`0x00AECC..0x00AF16`) (M15.225).
- `src/tools/hybrid/m68k_00ad76_render_step.hpp/.cpp` — SAT entry generation, tile attribute packing, piece loop iteration via `dbf d5` (`0x00AF18..0x00AF74`) (M15.225).
- `src/tools/hybrid/m68k_00ad76_shadow_step.hpp/.cpp` — Shadow sprite rendering path and shadow SAT attributes (`0x00AF76..0x00AFF8`) (M15.225).
- `src/tools/hybrid/m68k_00ad76_commit_step.hpp/.cpp` — SAT buffer subtraction, count commit, stack pop and RTS (`0x00AFFA..0x00B010`) (M15.225).
- `src/tools/hybrid/m68k_00ad76_metrics.cpp` — Dynamic telemetry and call metrics for `0x00AD76` cluster (M15.225).
- `tests/m68k_00ad76_cluster_test.cpp` — Comprehensive unit test suite for the `0x00AD76` cluster modules (M15.225).
- `tools/build_00ad76_runner.py` — Isolated Linux gameplay runner build orchestrator for `0x00AD76` (M15.225).
- `tools/run_00ad76_three_scenario_ab.py` — Three-scenario differential A/B qualification runner for `0x00AD76` (M15.225).
- `data/thor-evidence/00ad76-three-scenario-qualification-m15-225-20261108.json` — Machine-readable three-scenario qualification evidence receipt for `0x00AD76` (1,305,350 stepped instructions, 24/24 streams), validated via `ReceiptVerifier` (M15.225).
- `data/thor-evidence/cluster_00ad76/` — Machine-readable cluster configuration, dynamic execution profile, external transfers, and ownership ledgers for `0x00AD76` (M15.225).
- `src/game/render/sat_pipeline_contract.hpp/.cpp` — Standalone, reusable pure C++ Mega Drive VDP Sprite Attribute Table (SAT) pipeline contract; entry bitfield packing, coordinate transformation, screen-space clipping, link chain traversal and termination, 80-sprite capacity limit, and shadow emission; decoupled from CPU emulation (M15.226).
- `tests/sat_pipeline_contract_test.cpp` — Comprehensive unit test suite for standalone SAT pipeline contract (6/6 passing tests) (M15.226).
- `tools/run_combined_clusters_ab.py` — Combined dual-cluster qualification test orchestrator running P0, P1, P2, P3 across Scenarios C, A, and B (24/24 bit-exact streams) (M15.226).
- `tools/generate_m15_226_receipt.py` — Machine evidence receipt generator and ReceiptVerifier runner for M15.226 (M15.226).
- `data/thor-evidence/combined-clusters-qualification-m15-226-20261108.json` — Machine-readable evidence receipt for combined qualification of Entity Engine Cluster (Slot 222) and Sprite Engine Mode 0 Cluster (Slot 223), displacing 6,032,753 native instructions in Scenario C (34.223%) and 12,588,815 across 3 scenarios (3,992 frames) with 24/24 bit-exact stream matches, validated via `ReceiptVerifier` (M15.226).
- `src/tools/hybrid/m68k_00ad76_weapon_step.hpp/.cpp` — Modular native stepper for weapon and attached-sprite branch paths (`0x00B012..0x00B020`), invoking child helpers `0x00B34C` and `0x00B3B2`, resuming piece and shadow loops (M15.227).
- `tools/generate_sat_oracle_fixtures.py` — Authentic SAT oracle fixtures generator from original Sega Genesis VDP specifications and ROM table `0x00ACFE` (M15.227).
- `data/thor-fixtures/sat_oracle/` — Authentic SAT oracle fixtures and manifest covering normal player sprite, weapon attachment, horizontal flip, multi-piece descriptors, shadow table `0x00ACFE`, and capacity/clipping boundary (M15.227).
- `tools/generate_m15_227_receipt.py` — Machine evidence receipt generator and ReceiptVerifier runner for M15.227 (M15.227).
- `data/thor-evidence/combined-clusters-qualification-m15-227-20261108.json` — Machine-readable evidence receipt for Sprite Engine Weapon Path Expansion (Slot 223) + Entity Engine Cluster (Slot 222), displacing 6,053,007 native instructions in Scenario C (34.338%) and 1,331,652 sprite steps across 3 scenarios with 24/24 bit-exact stream matches, validated via `ReceiptVerifier` (M15.227).
- `src/tools/hybrid/m68k_00a9a0_step_ops.hpp/.cpp` — Core M68000 ALU, cycle-exact timing, CCR tracking, and BSR stack frame management for `0x00A9A0` (M15.228).
- `src/tools/hybrid/m68k_00a9a0_instruction_step.hpp/.cpp` — Main instruction router, ROM signature verification (`btst.b #1, 54(a6)`), and execution boundary tracking for Sprite Engine Mode 1 `0x00A9A0` (M15.228).
- `src/tools/hybrid/m68k_00a9a0_ingress_step.hpp/.cpp` — Fast-path hidden/paused entity exit, status word checks, flip/palette unpack (`0x00A9A0..0x00AA02`) (M15.228).
- `src/tools/hybrid/m68k_00a9a0_attrib_step.hpp/.cpp` — Sprite orientation, flip, palette bank and tile attributes (`0x00AA04..0x00AA74`) (M15.228).
- `src/tools/hybrid/m68k_00a9a0_camera_step.hpp/.cpp` — Camera coordinate translation, camera subtraction, relative vs absolute coordinates (`0x00AA76..0x00AAFC`) (M15.228).
- `src/tools/hybrid/m68k_00a9a0_coords_step.hpp/.cpp` — Coordinate offsets, camera calculations, and conditional X/Y adjustment (`0x00AB00..0x00AB66`) (M15.228).
- `src/tools/hybrid/m68k_00a9a0_piece_step.hpp/.cpp` — Multi-piece loop setup, piece coordinate parsing, child call `0x00B15E` integration (`0x00AB68..0x00ABDC`) (M15.228).
- `src/tools/hybrid/m68k_00a9a0_pieceloop_step.hpp/.cpp` — Piece loop iteration and bounding box updates (`0x00ABDE..0x00AC14`) (M15.228).
- `src/tools/hybrid/m68k_00a9a0_render_step.hpp/.cpp` — 8-byte SAT record emission, link chaining, viewport clipping (`0x00AC16..0x00AC5A`) (M15.228).
- `src/tools/hybrid/m68k_00a9a0_shadow_step.hpp/.cpp` — Attached entity shadow rendering loop (`0x00ACA0..0x00ACDE`) (M15.228).
- `src/tools/hybrid/m68k_00a9a0_commit_step.hpp/.cpp` — SAT pointer updates (`$FF1892`, `$FF188C`, `$FF188A`), stack register restore, RTS (`0x00AC5E..0x00ACFC`, `0x00AD1E..0x00AD40`) (M15.228).
- `src/tools/hybrid/m68k_00a9a0_weapon_step.hpp/.cpp` — Modular stepper for weapon hook branches to `0x00B34C` and `0x00B3B2` (M15.228).
- `src/tools/hybrid/m68k_00a9a0_metrics.cpp` — Telemetry and dynamic counters for `0x00A9A0` cluster (M15.228).
- `src/game/render/sprite_compositor.hpp/.cpp` — Standalone pure C++20 domain interfaces (`ISpriteCompositor`, `SpritePiece`, `SATRecord`) and concrete compositor decoupled from CPU emulation (M15.228).
- `tests/sprite_compositor_test.cpp` — Unit test suite for `ISpriteCompositor` and `SpriteCompositor` covering Mode 0, Mode 1 (absolute & relative), and clipping/shadows (M15.228).
- `tests/m68k_00a9a0_instruction_step_test.cpp` — Deterministic unit tests for Mode 1 instruction step dispatcher and fast-path hidden exits (M15.228).
- `tests/m68k_00a9a0_cluster_test.cpp` — Comprehensive unit test suite for 0x00A9A0 Mode 1 cluster verifying fast paths, full piece loops, and weapon hook routing (M15.228).
- `tools/build_m15_228_runner.py` — Linux gameplay runner build orchestrator for M15.228 triple-cluster runtime (Slots 222 + 223 + 224) (M15.228).
- `tools/run_m15_228_combined_ab.py` — Combined 3-scenario differential A/B qualification runner for M15.228 (M15.228).
- `tools/generate_m15_228_receipt.py` — Machine evidence receipt generator and `ReceiptVerifier` runner for M15.228 (M15.228).
- `data/thor-evidence/combined-clusters-qualification-m15-228-20261108.json` — Machine-readable evidence receipt for Sprite Dispatch Mode 1 (Slot 224) + Mode 0 (Slot 223) + Entity Engine (Slot 222), displacing 6,348,983 native instructions in Scenario C (36.017%) and 1,128,499 Mode 1 steps across 3 scenarios with 24/24 bit-exact stream matches, validated via `ReceiptVerifier` (M15.228).
- `src/tools/hybrid/m68k_008f22_step_ops.hpp/.cpp` — Core M68000 ALU, cycle-exact timing, CCR condition codes, and BSR stack frame management for `0x008F22` (M15.229).
- `src/tools/hybrid/m68k_008f22_instruction_step.hpp/.cpp` — Main instruction router, ROM signature verification (`tst.w 42(a6)`), and execution boundary tracking for `0x008F22` (M15.229).
- `src/tools/hybrid/m68k_008f22_ingress_step.hpp/.cpp` — Ingress checks, fast-path paused entity skip, status word evaluation (`0x008F22..0x008F8E`) (M15.229).
- `src/tools/hybrid/m68k_008f22_anim_step.hpp/.cpp` — Animation timer advance, direction vector lookup, frame indexing (`0x008F90..0x00904E`) (M15.229).
- `src/tools/hybrid/m68k_008f22_xright_step.hpp/.cpp` — Positive horizontal motion vector accumulation and bounds checking (`0x009050..0x0090AA`) (M15.229).
- `src/tools/hybrid/m68k_008f22_xloop_step.hpp/.cpp` — Rightward velocity stepping and obstacle collision checks (`0x0090AC..0x00911E`) (M15.229).
- `src/tools/hybrid/m68k_008f22_xleft_step.hpp/.cpp` — Negative horizontal motion vector accumulation (`0x009120..0x00915A`) (M15.229).
- `src/tools/hybrid/m68k_008f22_xleftloop_step.hpp/.cpp` — Leftward velocity stepping and terrain wall collision (`0x00915C..0x0091F4`) (M15.229).
- `src/tools/hybrid/m68k_008f22_ydown_step.hpp/.cpp` — Downward vertical motion vector accumulation (`0x0091F6..0x00923C`) (M15.229).
- `src/tools/hybrid/m68k_008f22_ydownloop_step.hpp/.cpp` — Downward velocity stepping and slope calculation (`0x00923E..0x0092AE`) (M15.229).
- `src/tools/hybrid/m68k_008f22_yup_step.hpp/.cpp` — Upward vertical motion vector accumulation and trajectory setup (`0x0092B0..0x0092FC`) (M15.229).
- `src/tools/hybrid/m68k_008f22_yuploop_step.hpp/.cpp` — Upward velocity stepping and ceiling collision (`0x0092FE..0x00938C`) (M15.229).
- `src/tools/hybrid/m68k_008f22_metrics.hpp/.cpp` — Telemetry and dynamic counters for `0x008F22` cluster (M15.229).
- `tests/m68k_008f22_cluster_test.cpp` — Comprehensive unit test suite for `0x008F22` cluster verifying disabled paths, paused exits, and motion transitions (M15.229).
- `tests/m68k_008f22_instruction_step_test.cpp` — Deterministic unit tests for `0x008F22` instruction router and ROM validation (M15.229).
- `tools/build_m15_229_runner.py` — Linux gameplay runner build orchestrator for M15.229 quad-cluster runtime (Slots 222 + 223 + 224 + 225) (M15.229).
- `tools/run_m15_229_combined_ab.py` — Combined 3-scenario differential A/B qualification runner for M15.229 (M15.229).
- `tools/build_global_hotspots_m15_229.py` — 14-column Top 20 unmigrated hotspots profiler for Scenario C (M15.229).
- `build/scenario_c_top20_hotspots_m15_229.md` — Generated markdown table for Top 20 unmigrated hotspots with 14 detailed columns (M15.229).
- `build/scenario_c_top20_hotspots_m15_229.json` — Machine-readable JSON profiling database for Top 20 unmigrated hotspots (M15.229).
- `tools/generate_m15_229_receipt.py` — Machine evidence receipt generator and `ReceiptVerifier` runner for M15.229 (M15.229).
- `data/thor-evidence/combined-clusters-qualification-m15-229-20261108.json` — Machine-readable evidence receipt for Quad Subsystem Recompiled Island Execution (Slots 222 + 223 + 224 + 225), displacing 6,348,983 native instructions in Scenario C (36.017%) with 24/24 bit-exact stream matches, validated via `ReceiptVerifier` (M15.229).
- `src/tools/hybrid/m68k_002fda_metrics.hpp/.cpp` — Telemetry and dynamic counters for `0x002FDA` Palette Scaling Engine (M15.230).
- `src/tools/hybrid/m68k_002fda_step_ops.hpp/.cpp` — Core M68000 ALU, cycle-exact timing, CCR condition codes, data-dependent `calc_mulu_cycles` and `calc_divu_cycles` formulas, and RTS return management for `0x002FDA` (M15.230).
- `src/tools/hybrid/m68k_002fda_instruction_step.hpp/.cpp` — Instruction stepper and loop router for `0x002FDA..0x00302C` palette color scaling engine (M15.230).
- `src/game/render/palette_scaler.hpp/.cpp` — Standalone pure C++20 domain interfaces (`PaletteScalingContract`, `scale_genesis_color`, `scale_palette`) (M15.230).
- `tests/m68k_002fda_cluster_test.cpp` — Comprehensive unit test suite for `0x002FDA` palette scaling engine across multiple intensity factors (0, 3, 7) (M15.230).
- `tools/build_m15_230_runner.py` — Linux gameplay runner build orchestrator for M15.230 penta-cluster runtime (Slots 222 + 223 + 224 + 225 + 226) (M15.230).
- `tools/run_m15_230_combined_ab.py` — Combined 3-scenario differential A/B qualification runner for M15.230 (M15.230).
- `tools/generate_m15_230_receipt.py` — Machine evidence receipt generator and `ReceiptVerifier` runner for M15.230 (M15.230).
- `data/thor-evidence/combined-clusters-qualification-m15-230-20261108.json` — Machine-readable evidence receipt for Penta Subsystem Recompiled Island Execution (Slots 222 + 223 + 224 + 225 + 226), displacing 6,413,735 native instructions in Scenario C (36.384% raw, 81.211% non-idle) with 24/24 bit-exact stream matches, validated via `ReceiptVerifier` (M15.230).
- `src/game/render/tilemap_plane_upload.hpp/.cpp` — Standalone pure C++20 domain interfaces (`TilemapPlaneUploadContract`, `calculate_plane_row_params`, `simulate_plane_row_upload`) for VDP plane row VRAM updates (M15.231).
- `tests/tilemap_plane_upload_test.cpp` — Standalone unit test suite for tilemap plane row upload contract and parameter calculations (M15.231).
- `src/tools/hybrid/m68k_002512_step_ops.hpp/.cpp` — Core M68000 ALU, cycle-exact timing, CCR condition codes, and dynamic shift timing for `0x002512` (M15.231).
- `src/tools/hybrid/m68k_002512_calc_step.hpp/.cpp` — Direction branch and horizontal plane upload address calculation (`0x002512..0x002568`, `0x00257E..0x0025C4`) (M15.231).
- `src/tools/hybrid/m68k_002512_calc_down_step.hpp/.cpp` — Downward plane upload calculation, table indexing, VDP address setup (`0x00256A..0x00257C`, `0x0025C6..0x0025E6`) (M15.231).
- `src/tools/hybrid/m68k_002512_upload_step.hpp/.cpp` — Unrolled 40-word tilemap buffer transfer loop into VDP data port and RTS cleanup (`0x0025E8..0x002630`) (M15.231).
- `src/tools/hybrid/m68k_002512_metrics.hpp/.cpp` — Dynamic telemetry and entry/step counters for `0x002512` cluster (M15.231).
- `src/tools/hybrid/m68k_002512_instruction_step.hpp/.cpp` — Modular instruction router and boundary stepper for `0x002512..0x002630` Tilemap Plane Row Upload Engine (M15.231).
- `tests/m68k_002512_cluster_test.cpp` — Comprehensive unit test suite for `0x002512` cluster verifying all 4 directional paths and 40-word unrolled transfers (M15.231).
- `tools/build_m15_231_runner.py` — Linux gameplay runner build orchestrator for M15.231 hexa-cluster runtime (Slots 222 + 223 + 224 + 225 + 226 + 227) (M15.231).
- `tools/run_m15_231_combined_ab.py` — Combined 3-scenario differential A/B qualification runner for M15.231 (M15.231).
- `tools/generate_002512_exact_step.py` — Tooling script generating exact cycle and flag accurate step dispatchers from ROM opcode map (M15.231).
- `data/thor-evidence/combined-clusters-qualification-m15-231-20261108.json` — Machine-readable evidence receipt for Hexa Subsystem Recompiled Island Execution (Slots 222 + 223 + 224 + 225 + 226 + 227), displacing 6,551,996 native instructions in Scenario C (37.168% raw, 82.961% non-idle) with 24/24 bit-exact stream matches, validated via `ReceiptVerifier` (M15.231).
- `src/tools/hybrid/m68k_0023cc_step_ops.hpp/.cpp` — Core M68000 ALU, cycle-exact timing, CCR condition codes, and step orchestration for `0x0023CC` Tilemap Plane Column Upload Engine (M15.232).
- `src/tools/hybrid/m68k_0023cc_calc_step.hpp/.cpp` — Scroll offset and VDP address calculation for plane column upload (`0x0023CC..0x00245A`) (M15.232).
- `src/tools/hybrid/m68k_0023cc_calc_left_step.hpp/.cpp` — Leftward scroll offset, wrap-around bounds checking, and VDP command setup (`0x00245E..0x0024AA`) (M15.232).
- `src/tools/hybrid/m68k_0023cc_upload_step.hpp/.cpp` — 28-row column tile buffer DMA upload loop, auto-increment toggle ($8F80 -> $8F02), and RTS return (`0x0024AE..0x002510`) (M15.232).
- `src/tools/hybrid/m68k_0023cc_metrics.hpp/.cpp` — Dynamic telemetry and entry/step counters for `0x0023CC` cluster (M15.232).
- `src/tools/hybrid/m68k_0023cc_instruction_step.hpp/.cpp` — Modular instruction router and boundary stepper for `0x0023CC..0x002510` Tilemap Plane Column Upload Engine (M15.232).
- `tests/m68k_0023cc_cluster_test.cpp` — Comprehensive unit test suite for `0x0023CC` cluster verifying left/right scroll paths and column transfers (M15.232).
- `tools/build_m15_232_runner.py` — Linux gameplay runner build orchestrator for M15.232 heptad-cluster runtime (Slots 222 + 223 + 224 + 225 + 226 + 227 + 228) (M15.232).
- `tools/run_m15_232_combined_ab.py` — Combined 3-scenario differential A/B qualification runner for M15.232 (M15.232).
- `tools/gen_0023cc_calc.py` — Modular generator submodule for 0x0023CC plane column upload calculation step C++ implementation (M15.232).
- `tools/gen_0023cc_left.py` — Modular generator submodule for 0x0023CC plane column upload left-scroll calculation step C++ implementation (M15.232).
- `tools/gen_0023cc_upload.py` — Modular generator submodule for 0x0023CC plane column tile buffer DMA upload loop C++ implementation (M15.232).
- `tools/generate_0023cc_exact_step.py` — Orchestrator tooling script generating exact cycle and flag accurate step dispatchers for column upload from ROM opcode map (M15.232).
- `tools/generate_m15_232_receipt.py` — Machine evidence receipt generator and `ReceiptVerifier` runner for M15.232 (M15.232).
- `data/thor-evidence/combined-clusters-qualification-m15-232-20261108.json` — Machine-readable evidence receipt for Heptad Subsystem Recompiled Island Execution (Slots 222 + 223 + 224 + 225 + 226 + 227 + 228), displacing 6,583,206 native instructions in Scenario C (37.345% raw, 83.357% non-idle) with 24/24 bit-exact stream matches, validated via `ReceiptVerifier` (M15.232).
- `data/thor-fixtures/tilemap_upload/` — Directory containing immutable oracle fixtures for tilemap plane row and column upload parameter contracts (M15.232).
- `tools/thor_recompiler/decoder.py` — M68000 instruction decoder for recompiler prototype (M15.233).
- `tools/thor_recompiler/cfg.py` — Basic block discovery and CFG builder for recompiler prototype (M15.233).
- `tools/thor_recompiler/codegen_cpp.py` — C++20 code generator with basic block switch dispatch, cycle accounting, and bus refresh (M15.233).
- `tools/thor_recompiler/main.py` — Automated CFG partitioning CLI tool enforcing Rule 3 (<= 500 lines) (M15.233).
- `src/tools/hybrid/generated/m68k_recompiled_002512*` — Machine-generated C++20 recompiled units for `0x002512..0x002630` (M15.233).
- `tests/m68k_recompiler_differential_test.cpp` — Differential qualification test comparing recompiled C++ against handwritten stepper (M15.233).
- `tools/build_recompiler_test.py` — Build and execution harness for recompiler differential tests (M15.233).
- 	ools/thor_recompiler/supported_opcodes.json — Machine-readable supported opcode family manifest with bit patterns and cycle rules (M15.234).
- src/tools/hybrid/generated/m68k_recompiled_0023cc* — Machine-generated C++20 recompiled units for 0x0023CC..0x002510 (M15.234).
- src/tools/hybrid/generated/m68k_recompiled_00d168* — Machine-generated C++20 recompiled units for 0x00D168..0x00D264 (M15.234).
- src/tools/hybrid/generated/manifests/* — Recompiler manifests (decoded, CFG, source_pc_map, unsupported_opcodes, codegen_manifest) (M15.234).
- 	ests/m68k_target_c_differential_test.cpp — Differential and mutation verification test for Target C (0x00D168) against reference stepper (M15.234).
- 	ools/build_target_c_test.py — Build and execution harness for Target C differential test (M15.234).
- 	ools/build_m15_234_runner.py — Isolated Linux runner build harness with Slot 229 integration (M15.234).
- 	ools/run_m15_234_combined_ab.py — 3-scenario combined cluster A/B qualification runner for M15.234 (M15.234).
- 	ools/generate_m15_234_receipt.py — Authoritative receipt generator and verifier for M15.234 (M15.234).
- data/thor-evidence/combined-clusters-qualification-m15-234-20261109.json — Verified machine evidence receipt for M15.234 (M15.234).
- data/thor-fixtures/batch_recompiler_candidates_m15_235.json — Frozen candidate manifest for M15.235 batch recompilation containing 9 candidate subroutines (M15.235).
- src/tools/hybrid/generated/m68k_recompiled_062610* — Machine-generated C++20 recompiled units for 0x062610..0x062692 (M15.235).
- src/tools/hybrid/generated/m68k_recompiled_005660* — Machine-generated C++20 recompiled units for 0x005660..0x00566E (M15.235).
- src/tools/hybrid/generated/m68k_recompiled_062986* — Machine-generated C++20 recompiled units for 0x062986..0x0629B8 (M15.235).
- src/tools/hybrid/generated/m68k_recompiled_06293a* — Machine-generated C++20 recompiled units for 0x06293A..0x062984 (M15.235).
- src/tools/hybrid/generated/m68k_recompiled_062900* — Machine-generated C++20 recompiled units for 0x062900..0x062938 (M15.235).
- src/tools/hybrid/generated/m68k_recompiled_0626a6* — Machine-generated C++20 recompiled units for 0x0626A6..0x0626BA (M15.235).
- src/tools/hybrid/generated/m68k_recompiled_062a24* — Machine-generated C++20 recompiled units for 0x062A24..0x062A76 (M15.235).
- src/tools/hybrid/generated/m68k_recompiled_062694* — Machine-generated C++20 recompiled units for 0x062694..0x0626A4 (M15.235).
- src/tools/hybrid/generated/m68k_recompiled_0629ee* — Machine-generated C++20 recompiled units for 0x0629EE..0x062A0A (M15.235).
- src/tools/hybrid/m68k_batch_step.hpp — Batch stepper declarations and metrics for experimental Slots 230–238 (M15.235).
- src/tools/hybrid/m68k_batch_step.cpp — Batch stepper implementation and dispatch wiring (M15.235).
- tests/m68k_batch_differential_test.cpp — Independent differential and mutation verification test suite for batch candidates (M15.235).
- tools/build_batch_differential_test.py — Build and execution script for batch differential tests (M15.235).
- tools/build_m15_235_runner.py — Linux qualification runner build harness for M15.235 (M15.235).
- tools/run_m15_235_combined_ab.py — 3-scenario combined cluster A/B qualification runner for M15.235 (M15.235).
- tools/generate_m15_235_receipt.py — Authoritative receipt generator and verifier for M15.235 (M15.235).
- data/thor-evidence/combined-clusters-qualification-m15-235-20261109.json — Verified machine evidence receipt for M15.235 (M15.235).
- tools/thor_recompiler/codegen_step.py — Instruction-level C++20 scheduler-safe stepper generator with cooperative GPGX scheduler yielding and dynamic shift cycle accounting (M15.236).
- src/tools/hybrid/generated/m68k_step_helpers.hpp — Common scheduler-safe step helpers (read_be, branch_to, prepare_step, complete_step, step_rts, CCR updates) (M15.236).
- data/thor-fixtures/batch_recompiler_candidates_m15_236.json — Manifest of evaluated and qualified scheduler-safe recompiler candidate subroutines (M15.236).
- src/tools/hybrid/generated/m68k_recompiled_00d266* — Scheduler-safe recompiled camera world coordinate normalizer routine for 0x00D266..0x00D2B2 (Slot 230) (M15.236).
- src/tools/hybrid/generated/m68k_recompiled_062732* — Scheduler-safe recompiled sound modulation direct strobe for 0x062732..0x062738 (Slot 231) (M15.236).
- src/tools/hybrid/generated/m68k_recompiled_062722* — Scheduler-safe recompiled sound modulation threshold clamp for 0x062722..0x062730 (Slot 232) (M15.236).
- tests/m68k_scheduler_safe_differential_test.cpp — Independent differential test suite for scheduler-safe instruction-stepped routines (M15.236).
- tools/build_scheduler_safe_differential_test.py — Build and execution harness for scheduler-safe differential tests (M15.236).
- tools/build_m15_236_runner.py — Linux qualification runner build harness for M15.236 (M15.236).
- tools/run_m15_236_combined_ab.py — 3-scenario combined cluster A/B qualification runner for M15.236 (M15.236).
- tools/generate_m15_236_receipt.py — Authoritative receipt generator and verifier for M15.236 (M15.236).
- data/thor-evidence/combined-clusters-qualification-m15-236-20261109.json — Verified machine evidence receipt for M15.236 (M15.236).
- data/thor-fixtures/batch_recompiler_candidates_m15_237.json — Manifest of evaluated scheduler-safe leaf candidates from Scenario C profile (M15.237).
- src/tools/hybrid/generated/m68k_recompiled_005660* — Machine-generated scheduler-safe recompiled routine for 0x005660..0x00566E (Slot 233, shadowed by 00557a) (M15.237).
- src/tools/hybrid/generated/m68k_recompiled_006e7a* — Machine-generated scheduler-safe recompiled Event/System Flag Query Engine for 0x006E7A..0x006E84 (Slot 234, productive +153 steps) (M15.237).
- src/tools/hybrid/generated/m68k_recompiled_062694* — Machine-generated scheduler-safe recompiled Sound Driver Attenuation Normalizer for 0x062694..0x0626BA (Slot 235, shadowed by 623ac) (M15.237).
- src/tools/hybrid/generated/m68k_recompiled_062a78* — Machine-generated scheduler-safe recompiled Sound Driver Channel Pan Update for 0x062A78..0x062A96 (Slot 236, shadowed by 623ac) (M15.237).
- tools/audit_m15_237_candidates.py — Candidate discovery, ownership screening, and Scenario C step profiler for M15.237 (M15.237).
- tools/build_m15_237_runner.py — Linux qualification runner build harness for M15.237 (M15.237).
- tools/run_m15_237_combined_ab.py — 3-scenario combined cluster A/B qualification runner for M15.237 (M15.237).
- tools/generate_m15_237_receipt.py — Authoritative receipt generator and verifier for M15.237 (M15.237).
- data/thor-evidence/combined-clusters-qualification-m15-237-20261109.json — Verified machine evidence receipt for M15.237 (M15.237).
- data/thor-evidence/scenario_c_disjoint_execution_ledger_m15_238.json — Verified mathematical disjoint execution ledger for Scenario C guest execution (M15.238).
- tools/audit_m15_238_residual_ledger.py — Verification script for mathematical disjoint execution ledger (M15.238).
- data/thor-fixtures/indirect_target_manifest_m15_238.json — Versioned runtime manifest for 0x0059B8 dynamic jump table targets (M15.238).
- src/tools/hybrid/generated/m68k_recompiled_00d2b4* — Machine-generated non-leaf recompiled Camera Viewport Scroll Step Coordinator for 0x00D2B4..0x00D344 (Slot 237, productive +3,124 steps) (M15.238).
- src/tools/hybrid/generated/m68k_recompiled_0059a8* — Machine-generated dynamic jump table recompiled Entity Type Action Dispatcher for 0x0059A8..0x0059BA (Slot 238, productive +25 steps) (M15.238).
- tools/build_m15_238_runner.py — Linux qualification runner build harness for M15.238 with Slots 222..238 (M15.238).
- tools/run_m15_238_combined_ab.py — 3-scenario combined cluster A/B qualification runner for M15.238 (M15.238).
- tools/generate_m15_238_receipt.py — Authoritative receipt generator and verifier for M15.238 (M15.238).
- data/thor-evidence/combined-clusters-qualification-m15-238-20261109.json — Verified machine evidence receipt for M15.238 (M15.238).
- src/tools/hybrid/m68k_residual_profiler.hpp/.cpp — Residual-only runtime execution profiler instrumenting ROM interpreter steps to build exact PC-level histograms without native interference (M15.239).
- src/tools/thor_ir/ir_schema.hpp — Emulator-independent intermediate representation schema with typed operations, operands, basic blocks, and abstract guest environment interface for portable C++20 and verification backends (M15.239).
- tools/thor_recompiler/partitioner.py — Deterministic automatic partitioning pipeline decomposing M68000 CFGs into bounded C++20 units (<= 8 instructions per file, <= 105 lines) with coordination dispatchers and manifest generation (M15.239).
- src/tools/hybrid/generated/m68k_recompiled_00e682* — Partitioned machine-generated scheduler-safe recompiled units and dispatcher for Camera Viewport State/Flags Coordinator at 0x00E682..0x00E6A8 (Slot 239, productive +19,183 steps) (M15.239).
- src/tools/hybrid/generated/m68k_recompiled_015ae2* — Partitioned machine-generated scheduler-safe recompiled units and dispatcher for Entity Movement / AI Branch Selector at 0x015AE2..0x015AFC (Slot 240, productive +16,324 steps) (M15.239).
- src/tools/hybrid/generated/m68k_recompiled_00c9b2* — Partitioned machine-generated scheduler-safe recompiled units and dispatcher for System Status Gatekeeper Routine at 0x00C9B2..0x00C9EA (Slot 241, productive +5,340 steps) (M15.239).
- src/tools/hybrid/generated/manifests/m68k_recompiled_*_manifest.json — Machine-generated candidate manifests containing PC-to-source maps, source hashes, and CFG structures (M15.239).
- tests/m68k_mixed_abi_and_irq_test.cpp — Comprehensive test suite for mixed call/return ABI stack preservation, NotHandled idempotence, interrupt preemption and continuation across scheduler boundaries, and dynamic jump table safety (M15.239).
- tests/m68k_candidate_diff_m15_239_test.cpp — Differential and mutation verification tests for Candidates 1–3 against pinned CPU reference logic with fault injection (M15.239).
- tests/thor_ir_mode_differential_test.cpp — Differential test verifying bit-exact execution parity between Mode A (verification stepper backend) and Mode B (compiled portable native backend) using the emulator-independent IR (M15.239).
- tools/build_m15_239_tests.py — Test build and execution runner for M15.239 differential, ABI, and IR test suites (M15.239).
- tools/build_m15_239_runner.py — Linux qualification runner build harness for M15.239 with Slots 222..241 (M15.239).
- tools/run_m15_239_profile.py — Execution harness for runtime residual profiling across Scenario C (M15.239).
- tools/generate_m15_239_candidates.py — Recompilation orchestrator generating partitioned C++20 code, dispatchers, and manifests for Candidates 1–3 (M15.239).
- tools/run_m15_239_combined_ab.py — 3-scenario combined cluster A/B qualification runner for M15.239 (M15.239).
- tools/benchmark_m15_239.py — Performance benchmark script measuring replay time and frame rates for P0 vs P_COMBINED (M15.239).
- tools/generate_m15_239_receipt.py — Machine evidence receipt generator and ReceiptVerifier runner for M15.239 (M15.239).
- data/thor-fixtures/residual_hotspots_m15_239.json — Frozen ranked residual interpreter hotspots excluding native-owned ranges and quarantined decompressor (M15.239).
- data/thor-evidence/combined-clusters-qualification-m15-239-20261109.json — Verified machine evidence receipt for M15.239 qualifying Slots 222..241, displacing 6,635,264 native instructions in Scenario C (+40,847 new native steps) with 24/24 bit-exact stream matches, validated via ReceiptVerifier (M15.239).
- src/tools/thor_ir/portable_guest_env.hpp — Scoped, emulator-independent guest environment interface implementing memory decoding, byte-swapped reads/writes, condition code arithmetic, and zero runtime dependencies on GPGX or Musashi (M15.240).
- src/tools/thor_ir/pilot_00e682_ir.json — Direct ROM-extracted Thor IR JSON representation for Camera Viewport pilot routine 0x00E682 (M15.240).
- src/tools/hybrid/m68k_jump_exception_types.hpp/.cpp — Formal dynamic jump target classification differentiating original M68000 architectural exceptions (Vector 2 BusError, Vector 3 AddressError), MMIO UnsupportedBusMapping, KnownCompiledTarget, and ValidUncompiledTarget (M15.240).
- src/tools/hybrid/generated/m68k_recompiled_00e6ba_part1.cpp — Partitioned recompiled unit for Integer Square Root subroutine at 0x00E6BA..0x00E6FC (quarantined/registered-disabled due to data-dependent DIVU cycle timing) (M15.240).
- src/tools/hybrid/generated/m68k_recompiled_00f0d2_part1.cpp — Partitioned recompiled unit for 2D Euclidean Distance Math at 0x00F0D2..0x00F110 (quarantined/registered-disabled due to Booth MULS cycle timing) (M15.240).
- src/tools/hybrid/generated/m68k_recompiled_016294_part1.cpp — Partitioned recompiled unit for Entity AI Patrol State Timer & Selector at 0x016294..0x0162CE (Slot 242, qualified productive execution with +1,976 Scenario C steps) (M15.240).
- tests/thor_ir_three_way_diff_test.cpp — Three-way differential verification test achieving bit-exact parity across Pinned CPU, Mode A Verification Step backend, and Mode B Portable C++ backend across 5 authentic state paths + mutation tests (M15.240).
- tests/m68k_jump_exception_semantics_test.cpp — Comprehensive unit test verifying dynamic jump classification, address error (Vector 3), bus error (Vector 2), MMIO execution, known compiled, and uncompiled ROM/RAM targets (M15.240).
- tests/m68k_candidate_diff_m15_240_test.cpp — Differential tests for M15.240 candidate routines against reference M68000 state mutations (M15.240).
- tests/m68k_mixed_stress_m15_240_test.cpp — Stress test validating nested call stacks, stack push/pop balance, interrupt preemption and resumption idempotence, and MMIO access ordering (M15.240).
- tools/thor_recompiler/ir_builder.py — Authentic ROM-to-Thor-IR extraction pipeline parsing instruction streams into structured IR JSON models (M15.240).
- tools/generate_m15_240_candidates.py — Orchestrator compiling candidate routines 0x00E6BA, 0x00F0D2, and 0x016294 into scheduler-safe C++20 units (M15.240).
- tools/generate_m15_240_receipt.py — Machine evidence receipt generator and ReceiptVerifier executor for M15.240 (M15.240).
- tools/benchmark_m15_240.py — Replay performance benchmark measuring P0 baseline vs P_COMBINED execution throughput and speedup (M15.240).
- tools/run_m15_240_combined_ab.py — Three-scenario differential qualification runner executing Scenarios C, A, and B across 24 differential streams (M15.240).
- data/thor-evidence/combined-clusters-qualification-m15-240-20261109.json — Certified machine evidence receipt for M15.240 qualifying Slots 222..242, displacing 6,637,240 native instructions in Scenario C (+1,976 new native steps) with 24/24 bit-exact stream matches, validated via ReceiptVerifier (M15.240).
- src/tools/thor_ir/recompiled_016294_ir.json — Direct ROM-extracted Thor IR JSON representation for Entity AI Patrol State Timer routine 0x016294 (M15.241).
- src/tools/thor_ir/recompiled_006e7a_ir.json — Direct ROM-extracted Thor IR JSON representation for Event Flag Bit-Test routine 0x006E7A (M15.241).
- src/tools/thor_ir/generated/portable_routines.hpp — Generated C++20 routines for 0x00E682, 0x016294, and 0x006E7A implementing Mode A (Verification Stepper) and Mode B (Portable Native C++) execution backends (M15.241).
- data/thor-fixtures/m15_241_cpu_reference_fixtures.json — Authoritative frozen CPU hardware reference execution fixtures generated directly from pinned GPGX core ca4be304... across 9 diverse execution paths (M15.241).
- data/thor-fixtures/m15_241_cpu_reference_fixtures.hpp — C++ header embedding the 9 frozen reference execution fixtures for fast, standalone test execution (M15.241).
- data/thor-fixtures/m15_241_ir_manifest.json — Metadata manifest indexing all generated Thor IR modules, checksums, entry PCs, and instruction inventories (M15.241).
- data/thor-evidence/idle_loop_audit_m15_241.json — Disjoint classification audit evidence reconciling attract sync loop (0x008ACA..0x008AD0, 5,327,384 steps) and gameplay sync loop (0x008B72..0x008B7A, 4,402,749 steps), totaling exact 9,730,133 idle wait steps (M15.241).
- data/thor-evidence/combined-clusters-qualification-m15-241-20261109.json — Certified machine evidence receipt for M15.241 qualifying canonical 3 MiB memory model, reference core independence, multi-routine IR recompilation, and 24/24 bit-exact stream matches, validated via ReceiptVerifier (M15.241).
- tests/portable_memory_map_test.cpp — Comprehensive unit test verifying canonical 3 MiB ROM boundary access (0x000000, 0x1FFFFE, 0x200000, 0x2FFFFE, 0x300000), read-only protection, RAM mirroring, alignment exceptions, and MMIO callbacks (M15.241).
- tests/portable_continuation_contract_test.cpp — Resumable continuation contract test verifying dual SSP/USP stack swapping, stack push/pop, 24-bit address wraparound, and instruction-boundary yield/resume hooks (M15.241).
- tests/m68k_variable_cycle_arithmetic_test.cpp — Variable-cycle arithmetic timing test modeling Booth multiplier (38..70 cycles), non-restoring DIVU (108..140 cycles), and characterizing sub-cycle timing drift in 0x00E6BA (M15.241).
- tests/m68k_decompressor_resumption_test.cpp — Decompressor 0x003820 resumption forensics test proving state preservation across VBlank Level 6 interrupts and confirming quarantine requirement (M15.241).
- tools/generate_m15_241_fixtures.py — Fixture generator sampling authentic reference execution directly from pinned core ca4be304... (M15.241).
- tools/audit_idle_loop_classification.py — Forensic audit script analyzing disjoint idle polling loops in Beyond Oasis (M15.241).
- tools/benchmark_m15_241.py — Performance benchmark measuring Mode A verification stepper vs Mode B portable C++ execution throughput and speedup (M15.241).
- tools/generate_m15_241_receipt.py — Machine evidence receipt generator and ReceiptVerifier executor for M15.241 (M15.241).
- src/tools/thor_ir/connected_00d2b4_ir.json — Direct ROM-extracted Thor IR JSON representation for Raster Scroll Coordinator caller routine 0x00D2B4 (M15.242).
- src/tools/thor_ir/connected_00d344_ir.json — Direct ROM-extracted Thor IR JSON representation for Plane A Scroll Updater callee routine 0x00D344 (M15.242).
- src/tools/thor_ir/generated/connected_portable_routines.hpp — Generated C++20 routines for connected subsystem 0x00D2B4 -> BSR 0x00D344 implementing Mode A and Mode B execution backends (M15.242).
- data/thor-fixtures/m15_242_connected_fixtures.json — Authoritative frozen CPU hardware reference execution fixtures generated for connected subsystem across 4 execution paths (M15.242).
- data/thor-fixtures/m15_242_connected_fixtures.hpp — C++ header embedding the 4 frozen connected reference execution fixtures for fast, standalone test execution (M15.242).
- data/thor-fixtures/m15_242_ir_manifest.json — Metadata manifest indexing all generated Thor IR modules including connected routines (M15.242).
- data/thor-evidence/reference_independence_audit_m15_242.json — Audit report validating reference CPU fixture provenance against pinned core ca4be304... and canonical ROM eb19bda4... (M15.242).
- data/thor-evidence/combined-clusters-qualification-m15-242-20261110.json — Certified machine evidence receipt for M15.242 qualifying connected real-ROM recompilation and 24/24 bit-exact stream matches, validated via ReceiptVerifier (M15.242).
- tests/portable_mixed_continuation_contract_test.cpp — Comprehensive unit test verifying safe mixed continuation contract, native->native, native->uncompiled, uncompiled->native, nested returns, interrupt preemption/RTE, and CPU exceptions (M15.242).
- tests/thor_ir_connected_diff_test.cpp — Three-way differential verification test achieving 100% bit-exact parity across CPU Reference, Mode A, and Mode B across 4 connected paths + 3 mutation tests (M15.242).
- tests/m68k_variable_cycle_integration_test.cpp — Integration test validating variable-cycle arithmetic formulas for Booth MULS (38..70 cycles) and non-restoring DIVU (108..140 cycles) across edge operands (M15.242).
- tools/audit_reference_independence_m15_242.py — Script auditing reference CPU fixture provenance against pinned core and canonical ROM (M15.242).
- tools/generate_m15_242_connected_fixtures.py — Generator sampling authentic connected execution fixtures from pinned core ca4be304... (M15.242).
- tools/benchmark_m15_242.py — Performance benchmark measuring Mode A vs Mode B on connected raster scroll workload across 100,000 iterations (M15.242).
- tools/generate_m15_242_receipt.py — Machine evidence receipt generator and ReceiptVerifier executor for M15.242 (M15.242).
- tools/generate_rom_00e682_semantic_audit.py — Cross-track semantic audit script analyzing 0x00E682 ROM bytes, 44 callers, A6 entity pointer, and ADDQ.L #4, A7 frame pop yield semantics (M15.243).
- data/thor-evidence/rom_00e682_semantic_audit_m15_243.json — Authoritative semantic reconciliation report identifying 0x00E682 as entity_timer_tick_and_yield (M15.243).
- tools/generate_scroll_subsystem_audit.py — Instruction coverage and scroll RAM layout audit script for M15.242 raster scroll subsystem 0x00D2B4 & 0x00D344 (M15.243).
- data/thor-evidence/scroll_subsystem_instruction_audit_m15_243.json — Exact 60-instruction 1:1 IR coverage audit report and scroll RAM layout reconciliation (M15.243).
- tools/generate_entity_dispatch_ir.py — Automatic Thor IR and C++20 code generator for the connected entity state-dispatch subsystem 0x0059A8 (80 instructions, 6 CFG blocks) (M15.243).
- src/tools/thor_ir/entity_dispatch_subsystem_ir.json — Direct ROM-extracted Thor IR JSON representation for connected entity state-dispatch subsystem (0x0059A8, 0x005C82, States 12-17) (M15.243).
- src/tools/thor_ir/generated/entity_dispatch_portable_routines.hpp — Generated C++20 routines for connected entity state-dispatch subsystem implementing Mode A and Mode B backends (M15.243).
- data/thor-fixtures/m15_243_ir_manifest.json — Metadata manifest indexing generated Thor IR module and blocks for entity state-dispatch subsystem (M15.243).
- tools/generate_m15_243_fixtures.py — Generator sampling 6 authentic execution paths (including ValidUncompiledPc and Address Error exception) from pinned core ca4be304... (M15.243).
- data/thor-fixtures/m15_243_entity_dispatch_fixtures.json — Authoritative frozen CPU hardware reference execution fixtures across 6 entity dispatch paths (M15.243).
- data/thor-fixtures/m15_243_entity_dispatch_fixtures.hpp — C++ header embedding the 6 frozen entity dispatch reference fixtures for standalone test execution (M15.243).
- tests/thor_ir_entity_dispatch_diff_test.cpp — Three-way differential verification test achieving 100% bit-exact parity across CPU Reference, Mode A, and Mode B across 6 paths + 3 mutation tests (M15.243).
- tools/benchmark_m15_243.py — Performance benchmark measuring Mode A vs Mode B on connected entity state-dispatch subsystem across 100,000 iterations (2.06x speedup) (M15.243).
- data/thor-evidence/combined-clusters-qualification-m15-243-20261110.json — Certified machine evidence receipt for M15.243 qualifying semantic audits, portable entity state dispatch, and 24/24 bit-exact stream matches, validated via ReceiptVerifier (M15.243).
- tools/generate_m15_243_receipt.py — Machine evidence receipt generator and ReceiptVerifier executor for M15.243 (M15.243).
- tools/audit_entity_dispatch_table_m15_244.py — Exhaustive 59-entry entity jump-table audit (`0x0059BA..0x005A30`), ownership, and gameplay watch-count aggregator (264 lines <= 500) (M15.244).
- data/thor-evidence/entity_dispatch_table_manifest_m15_244.json — Authoritative 59-entry table manifest, boundary proof (`0x005A30`), and observed hit counts across Scenarios C/A/B (M15.244).
- data/thor-evidence/entity_dispatch_watch_counts_m15_244.json — Committed compact summary of live Scenario C/A/B `PC_WATCH` counts at `0x0059B8` (M15.244).
- tools/thor_recompiler/cfg_reconstructor.py — BFS reachable CFG basic-block splitter and typed edge classifier for M68000 subsystems (237 lines <= 500) (M15.244).
- tools/thor_recompiler/cpp_emitter.py — Auto-splitting templated C++20 emitter (`<= 350` lines/file) with Musashi-calibrated hardware cycle counts and Mode B block dispatch (488 lines <= 500) (M15.244).
- tools/generate_entity_dispatch_m15_244.py — Deterministic IR + C++20 + CFG source-map generator for the 7-root entity state-dispatch subsystem (161 lines <= 500) (M15.244).
- src/tools/thor_ir/generated/entity_dispatch_common.hpp — Auto-generated shared bus/operand/ALU helpers for entity state dispatch (104 lines <= 500) (M15.244).
- src/tools/thor_ir/generated/entity_dispatch_part0.hpp — Auto-generated entity dispatch per-instruction steppers partition 0 (253 lines <= 500) (M15.244).
- src/tools/thor_ir/generated/entity_dispatch_part1.hpp — Auto-generated entity dispatch per-instruction steppers partition 1 (252 lines <= 500) (M15.244).
- src/tools/thor_ir/generated/entity_dispatch_part2.hpp — Auto-generated entity dispatch per-instruction steppers partition 2 (258 lines <= 500) (M15.244).
- src/tools/thor_ir/generated/entity_dispatch_part3.hpp — Auto-generated entity dispatch per-instruction steppers partition 3 (250 lines <= 500) (M15.244).
- src/tools/thor_ir/generated/entity_dispatch_part4.hpp — Auto-generated entity dispatch per-instruction steppers partition 4 (157 lines <= 500) (M15.244).
- src/tools/thor_ir/generated/entity_dispatch_router.hpp — Auto-generated PC ownership predicate and instruction dispatcher across all 130 entity-dispatch ops (163 lines <= 500) (M15.244).
- src/tools/thor_ir/generated/entity_dispatch_mode_b.hpp — Auto-generated fused CFG basic-block Mode B executor for entity state dispatch (234 lines <= 500) (M15.244).
- src/tools/thor_ir/generated/entity_dispatch_portable_routines.hpp — Umbrella portable header defining `run_entity_dispatch_mode_a` / `run_entity_dispatch_mode_b` (20 lines <= 500) (M15.244).
- data/thor-evidence/entity_dispatch_cfg_source_map_m15_244.json — Complete 130-row traceability map linking ROM PC, ROM bytes, disassembly, IR op IDs, and generated C++ locations (M15.244).
- data/thor-fixtures/m15_244_ir_manifest.json — Subsystem IR build manifest for M15.244 (`130` instructions, `43` blocks, `123` edges) (M15.244).
- tools/m15_244_gpgx_oracle.cpp — Standalone pinned-GPGX oracle replayer and isolated CPU prober (420 lines <= 500) (M15.244).
- tools/generate_m15_244_fixtures.py — Authentic + controlled GPGX oracle fixture generator and `0x00E682` stack proof extractor (248 lines <= 500) (M15.244).
- data/thor-evidence/rom_00e682_stack_proof_m15_244.json — Proof of `49` live nonlocal returns via `0x00E6A6 ADDQ.L #4, A7 + 0x00E6A8 RTS` across Scenarios C/A/B (M15.244).
- data/thor-fixtures/m15_244_entity_dispatch_fixtures.json — 20 frozen GPGX hardware reference fixtures (`10 AUTHENTIC_GAMEPLAY_ENTRY` + `10 CONTROLLED_CPU_ENTRY`) (M15.244).
- data/thor-fixtures/m15_244_entity_dispatch_fixtures.hpp — C++ header embedding the 20 M15.244 GPGX reference fixtures (36 lines <= 500) (M15.244).
- tests/portable_entity_interrupt_safety_test.cpp — Portable VBlank Level 6 & HBlank Level 4 preemption, masking, and RTE resumption test suite (112 lines <= 500) (M15.244).
- src/tools/thor_ir/entity_dispatch_hybrid_adapter.hpp — Hybrid runner step adapter executing generated portable C++20 `step_one_entity_dispatch_ir` as `0059a8p` (158 lines <= 500) (M15.244).
- tools/build_m15_244_runner.py — Standalone WSL build script compiling isolated `oasis_gameplay_session` with portable `0059a8p` support (149 lines <= 500) (M15.244).
- tools/run_m15_244_portable_ab.py — Three-scenario Single-Variable A/B qualification harness comparing 196 prod vs `196+0059a8p` vs `196+comb+0059a8p` (143 lines <= 500) (M15.244).
- tools/benchmark_m15_244.py — Reproducible 10-trial Mode A vs Mode B Release benchmark and binary/source size collector (222 lines <= 500) (M15.244).
- data/thor-evidence/benchmark_m15_244_report.json — Verified M15.244 Mode A vs Mode B benchmark measurements (`1.121x` speedup across 10 trials) (M15.244).
- tools/generate_m15_244_receipt.py — Receipt generator & `ReceiptVerifier` runner for M15.244 (`57` checked disk files) (205 lines <= 500) (M15.244).
- data/thor-evidence/combined-clusters-qualification-m15-244-20261110.json — Certified M15.244 qualification receipt verified by `ReceiptVerifier` (`overall_valid=True`, `48/48` stream artifacts) (M15.244).
- docs/handoffs/STANDALONE_M15_244_ENTITY_DISPATCH_HANDOFF.md — Read-only standalone engineering handoff for entity state dispatch (`0x0059A8`) and nonlocal sub-action abort (`0x00E682`) (M15.244).
- tools/m15_245_gpgx_profiler.cpp — Live pinned GPGX profiler capturing full Scenario C/A/B PC counts, entity table callers/exits, and `0x00E682` unwind statistics (443 lines <= 500) (M15.245).
- tools/audit_m15_245_candidates.py — Exact `196`-helper ownership audit and boundary analyzer producing `entity_handler_hotspot_manifest_m15_245.json` and `rom_005670_and_00e682_continuation_proof_m15_245.json` (198 lines <= 500) (M15.245).
- data/thor-evidence/entity_handler_hotspot_manifest_m15_245.json — Ranked 25-handler candidate ownership/boundary audit excluding all production overlaps (`0x006516`, `0x0062E4`, etc.) (M15.245).
- data/thor-evidence/rom_005670_and_00e682_continuation_proof_m15_245.json — Caller topology (`0x005586 -> 0x005670 -> 0x0059B8`) and `0x00E682` live nonlocal return proof (M15.245).
- tools/thor_recompiler/decoder_ext.py — M68000 decoder extensions for `BTST #imm, disp_ax`, `JMP abs.l`, and `OR`/`AND`/`CMP` addressing forms (107 lines <= 500) (M15.245).
- tools/thor_recompiler/insn_lowering_m15_245.py — Musashi-calibrated M68000 instruction cycle computation (`DBRA` `12, taken ? -1 : 1`, `ASR.L #1` `8, 1`) and C++20 lowering for M15.245 (334 lines <= 500) (M15.245).
- tools/thor_recompiler/cpp_emitter_m15_245.py — Auto-splitting C++20 emitter (`<= 259` lines/file) for `m15245p` steppers, chunked router, and Mode B block executor (221 lines <= 500) (M15.245).
- tools/generate_entity_dispatch_m15_245.py — Deterministic IR + 29-header C++20 + 508-row CFG source-map generator for the 16 unowned M15.245 entity handler roots (183 lines <= 500) (M15.245).
- src/tools/thor_ir/entity_dispatch_expanded_ir.json — Thor IR JSON for the `508` new M68000 instructions across `230` CFG basic blocks and `397` edges (M15.245).
- src/tools/thor_ir/generated_m15_245/exp_dispatch_common.hpp — Umbrella common include reusing M15.244 `PortableIrBus` / `entity_dispatch_common.hpp` (`2` lines <= 350) (M15.245).
- src/tools/thor_ir/generated_m15_245/exp_dispatch_part0.hpp .. exp_dispatch_part16.hpp — 17 auto-generated instruction-stepper header partitions (`121..259` lines `<= 350`) covering all `508` M15.245 ops (M15.245).
- src/tools/thor_ir/generated_m15_245/exp_dispatch_router_p0.hpp .. exp_dispatch_router_p2.hpp, exp_dispatch_router.hpp — Partitioned PC predicate and per-instruction dispatcher headers (`81..204` lines `<= 350`) (M15.245).
- src/tools/thor_ir/generated_m15_245/exp_dispatch_mode_b_p0.hpp .. exp_dispatch_mode_b_p4.hpp, exp_dispatch_mode_b.hpp — Partitioned fused CFG basic-block Mode B executor headers (`19..248` lines `<= 350`) (M15.245).
- src/tools/thor_ir/generated_m15_245/exp_dispatch_portable_routines.hpp — Umbrella M15.245 portable C++20 API header (`20` lines <= 350) (M15.245).
- data/thor-evidence/entity_dispatch_cfg_source_map_m15_245.json — Complete 508-row PC/opcode/IR/C++ traceability source map with `0` overlap and `0` omitted ops (M15.245).
- data/thor-fixtures/m15_245_ir_manifest.json — Build manifest for M15.245 expanded entity handler IR (`508` new / `638` combined instructions) (M15.245).
- tools/m15_245_gpgx_oracle.cpp — Standalone pinned-GPGX oracle replayer and isolated CPU probe harness for M15.245 fixtures (331 lines <= 500) (M15.245).
- tools/generate_m15_245_fixtures.py — Authentic (`18`) + controlled (`10`) GPGX fixture generator emitting JSON and C++ header (247 lines <= 500) (M15.245).
- data/thor-fixtures/m15_245_entity_dispatch_fixtures.json — 28 frozen pinned-GPGX reference fixtures (`18 AUTHENTIC_GAMEPLAY_ENTRY` + `10 CONTROLLED_CPU_ENTRY`) (M15.245).
- data/thor-fixtures/m15_245_entity_dispatch_fixtures.hpp — C++ header embedding the 28 M15.245 GPGX reference fixtures (36 lines <= 500) (M15.245).
- tests/thor_ir_m15_245_diff_test.cpp — Three-way bit/cycle-exact differential (`28/28` Oracle vs Mode A vs fused Mode B), Phase 5 nonlocal unwind, and `15/15` negative/mutation contract suite (209 lines <= 500) (M15.245).
- src/tools/thor_ir/exp_dispatch_hybrid_adapter.hpp — Hybrid runner step adapter registering `m15245p` disjoint entity handler execution (102 lines <= 500) (M15.245).
- tools/build_m15_245_runner.py — Isolated WSL builder compiling `oasis_gameplay_session` with `0059a8p` and `m15245p` enabled (154 lines <= 500) (M15.245).
- tools/run_m15_245_portable_ab.py — Three-configuration (`196` vs `196+0059a8p` vs `196+0059a8p+m15245p`) A/B/C stream parity harness across Scenarios C, A, B (145 lines <= 500) (M15.245).
- tools/benchmark_m15_245.py — 10-trial Release Mode A vs fused Mode B benchmark and integrated wall-clock reporter (238 lines <= 500) (M15.245).
- data/thor-evidence/benchmark_m15_245_report.json — Verified M15.245 Mode A (`15.402 ms`) vs Mode B (`12.649 ms`, `1.218x`) and integrated timing report (M15.245).
- tools/generate_m15_245_receipt.py — Qualification receipt generator & `ReceiptVerifier` runner for M15.245 (211 lines <= 500) (M15.245).
- data/thor-evidence/combined-clusters-qualification-m15-245-20261110.json — Certified M15.245 qualification receipt (`ReceiptVerifier overall_valid=True, checked_files=57, rejections=[]`) (M15.245).
- docs/handoffs/STANDALONE_M15_245_ENTITY_HANDLERS_HANDOFF.md — Read-only standalone handoff covering M15.245 entity handlers, `$FF1668` input ring scan, caller continuation, and trace hashes (M15.245).
- tools/audit_coverage_and_displacement_m15_246.py — Coverage forensics reconciler & strictly disjoint single-owner execution auditor for C/A/B & `longrun_3600f` (M15.246).
- data/thor-evidence/native_coverage_accounting_m15_246.json — Canonical coverage accounting contract (`oasis.coverage-accounting-contract.v1`), reconciliation, and single-owner histograms (M15.246).
- tools/generate_m15_246_longrun_journal.py — Deterministic 3,600f input journal (`Scenario C 0..1799` + `INJECTED_SCRIPTED 1800..3599`) & manifest generator (M15.246).
- tools/m15_246_longrun_capture.cpp — Pinned-GPGX 3,600f capture profiler collecting checkpoints and video/audio/CPU trace hashes (M15.246).
- data/thor-evidence/longrun_3600f_capture_manifest_m15_246.json — Verified 3,600f gameplay capture manifest (`37,831,953` steps, `12` checkpoints) (M15.246).
- tools/audit_m15_246_candidates.py — Callee hotspot ranker, candidate ownership verifier (`0x0085E2`/`0x00BCBA`/`0x007A28`), and CFG boundary auditor (M15.246).
- data/thor-evidence/callee_hotspot_manifest_m15_246.json — Ranked callee ownership & CFG boundary audit manifest (M15.246).
- tools/thor_recompiler/decoder_ext_m15_246.py — M68000 decoder extensions for `DIVU`/`DIVS #imm`, `EXT`, `SF/ST Dn`, `LEA disp_ax`, `MOVEM.W`, `EXG`, `CMPA`, `JSR (An)` (M15.246).
- tools/thor_recompiler/insn_lowering_m15_246.py — M15.246 C++20 lowering & Musashi cycle calibration (`MULS`, `DIVU`/`DIVS`, `MOVEM.W` vs `.L`, `ASL`/`LSL`/`LSR`, `ADD.L <ea>, Dn`) (M15.246).
- tools/thor_recompiler/cpp_emitter_m15_246.py — Auto-splitting M15.246 C++20 emitter producing `<= 259`-line partitioned headers (M15.246).
- tools/generate_entity_dispatch_m15_246.py — Deterministic IR + 23-header C++20 + 375-row CFG source-map generator for the 7 M15.246 roots (M15.246).
- src/tools/thor_ir/m15_246_callee_ir.json — Thor IR JSON for the `375` M15.246 instructions across `104` blocks and `183` edges (M15.246).
- data/thor-fixtures/m15_246_ir_manifest.json — Build manifest for M15.246 connected high-frequency callee IR (M15.246).
- data/thor-evidence/callee_cfg_source_map_m15_246.json — 375-row PC/IR/C++ source map with `0` overlap and `0` omitted ops (M15.246).
- src/tools/thor_ir/generated_m15_246/exp246_dispatch_*.hpp — Auto-generated C++20 common, stepper (`part0..part12`), router (`router_p0..p2, router`), Mode B (`mode_b_p0..p2, mode_b`), and umbrella headers (`<= 259` lines) (M15.246).
- tools/m15_246_gpgx_oracle.cpp — Pinned-GPGX oracle replayer and isolated CPU probe harness with full pre-entry RAM snapshot & Vector 5 trapping (M15.246).
- tools/generate_m15_246_fixtures.py — Authentic (`18`) + controlled (`12`) GPGX fixture generator emitting JSON and C++ header (M15.246).
- data/thor-fixtures/m15_246_callee_fixtures.json — 30 frozen pinned-GPGX reference fixtures (`18 AUTHENTIC + 12 CONTROLLED`) (M15.246).
- data/thor-fixtures/m15_246_callee_fixtures.hpp — C++ header embedding the 30 M15.246 GPGX reference fixtures (M15.246).
- tests/thor_ir_m15_246_diff_test.cpp — Three-way bit/cycle-exact differential (`30/30`), cross-module M15.244/245/246 continuations, and `12/12` mutation contracts suite (M15.246).
- src/tools/thor_ir/exp246_dispatch_hybrid_adapter.hpp — Hybrid runner step adapter registering default-disabled `m15246p` (M15.246).
- tools/build_m15_246_runner.py — Isolated WSL builder compiling `oasis_gameplay_session` with `0059a8p`, `m15245p`, and `m15246p` (M15.246).
- tools/run_m15_246_portable_ab.py — Four-configuration (`RUN_A..D`) A/B/C/D + `longrun_3600f` parity harness across `32/32` streams and `12/12` 300f checkpoints (M15.246).
- tools/benchmark_m15_246.py — 10-trial Release Mode A vs Mode B benchmark and integrated timing reporter (M15.246).
- data/thor-evidence/benchmark_m15_246_report.json — Verified M15.246 Mode A (`24.655 ms`) vs Mode B (`18.122 ms`, `1.361x`) and integrated timing report (M15.246).
- tools/generate_m15_246_receipt.py — Qualification receipt generator & `ReceiptVerifier` runner for M15.246 (`139` checked files, `0` rejections) (M15.246).
- data/thor-evidence/combined-clusters-qualification-m15-246-20261110.json — Certified M15.246 qualification receipt verified by `ReceiptVerifier` (`overall_valid=True`) (M15.246).
- docs/handoffs/STANDALONE_M15_246_CALLEES_AND_MATH_HANDOFF.md — Read-only standalone handoff covering M15.246 callees, math/shift/MOVEM/JSR primitives, and long-run capture hashes (M15.246).
