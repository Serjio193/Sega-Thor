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
│   │   ├── controlled_screen.cpp Synthetic M11.18 screen update and rasterizer
│   │   ├── controlled_screen.hpp Controlled-screen state and fixture API
│   │   ├── render/
│   │   │   ├── framebuffer.cpp Software framebuffer storage
│   │   │   └── framebuffer.hpp Packed software framebuffer API
│   │   ├── graphics_decompress.cpp Native translation of original 0x3820 routine
│   │   ├── graphics_decompress.hpp Decompressor result/API
│   │   ├── genesis_graphics.cpp Pure 4bpp tile + CRAM palette decoding
│   │   ├── genesis_graphics.hpp Graphics decoder data types/API
│   │   ├── resource_loader.cpp Verified canonical-ROM resource extraction
│   │   ├── resource_loader.hpp Bounded resource ID 3 contract/API
│   │   ├── resource_diagnostic.cpp Native VRAM transfer and tile-atlas renderer
│   │   ├── resource_diagnostic.hpp Diagnostic resource visualization API
│   │   ├── translated_routines.cpp Initial translated compatibility routines
│   │   ├── translated_routines.hpp Their public declarations
│   │   ├── player/
│   │   │   ├── player.cpp     Portable input, state and terrain-gated movement
│   │   │   └── player.hpp     Player movement API and ROM evidence constants
│   │   ├── entities/
│   │   │   ├── entity_pool.cpp  Raw entity-pool bounds and active-record view
│   │   │   └── entity_pool.hpp  Entity-pool descriptors and raw field offsets
│   │   ├── scripts/
│   │   │   ├── event_router.cpp  Raw event producer and handler-range mapping
│   │   │   └── event_router.hpp  Event RAM addresses and raw router API
│   │   ├── spirits/
│   │   │   ├── spirit_slots.cpp  Evidence-backed slot, target and dispatch trace
│   │   │   └── spirit_slots.hpp  Spirit slot/target/dispatch constants and API
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
    ├── graphics_decompress_reference.cpp ROM-backed differential oracle verifier
    ├── resource_id3_reference.cpp Local USA-ROM resource/VRAM/atlas oracle verifier
    ├── genesis_graphics_test.cpp         Synthetic tile/palette conversion tests
    ├── rom_identity_test.cpp             Synthetic ROM/hash/header tests
    ├── player_test.cpp                   Deterministic input and movement tests
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
