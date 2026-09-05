# File Map

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
│   ├── REVERSE_ENGINEERING.md Address/routine/ROM/data research ledger
│   ├── ROADMAP.md             Ordered milestones and current active milestone
│   ├── TASK_TEMPLATE.md       Mandatory task/session handoff template
│   ├── VDP_MODEL.md           Narrow portable video-state model and non-goals
│   └── WORKLOG.md             Chronological record of development actions
├── src/
│   ├── main.cpp               ROM identity/report entry point
│   ├── core/
│   │   ├── rom.cpp            ROM file loading/basic title access
│   │   ├── rom.hpp            ROM byte-container API
│   │   ├── rom_identity.cpp   Header/checksum/hash/known-revision identification
│   │   ├── rom_identity.hpp   ROM identity models and public API
│   │   ├── runtime.cpp        Explicit deterministic frame stepping
│   │   └── runtime.hpp        Portable controller/input/frame runtime API
│   ├── genesis/
│   │   ├── memory_bus.cpp     Minimal Mega Drive address-space compatibility
│   │   ├── memory_bus.hpp     Memory bus interface
│   │   ├── vdp.cpp            Bounded VRAM/CRAM/VSRAM storage/access
│   │   ├── vdp.hpp            Narrow VDP state API
│   │   └── vdp_types.hpp      Tile/plane/sprite raw attribute decoding
│   ├── game/
│   │   ├── graphics_decompress.cpp Native translation of original 0x3820 routine
│   │   ├── graphics_decompress.hpp Decompressor result/API
│   │   ├── genesis_graphics.cpp Pure 4bpp tile + CRAM palette decoding
│   │   ├── genesis_graphics.hpp Graphics decoder data types/API
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
│   └── tools/
│       ├── asset_inspector.cpp Local-only ROM graphics inspection CLI
│       ├── ghidra/OasisGhidraMap.java Developer-only Ghidra map exporter; never production-linked
│       ├── re_slice_decoder.cpp Developer-only bounded 68000 evidence decoder/reporter
│       ├── re_slice_decoder.hpp Decoder data types and report API
│       ├── re_slice_format.cpp Deterministic JSON/human report formatting
│       ├── re_slice_report.cpp Local-only bounded ROM slice report CLI
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
│       ├── re_bizhawk_boot_trace.lua Developer-only BizHawk boot trace and bus-write probe
│       ├── re_bizhawk_natural_reach.lua Developer-only bounded natural-input target/caller probe
│       ├── re_bizhawk_ant.lua            Developer-only one-frontier natural ant worker
│       ├── re_bizhawk_stack_provenance.lua Developer-only bounded runtime stack-value/writer probe
│       ├── re_bizhawk_natural_scenario.txt Frozen neutral-input reachability scenario
│       ├── re_mame_boot_trace.cmd Developer-only MAME fixed-instruction debugger trace
│       ├── re_mame_writer_probe.cmd Developer-only MAME RAM writer watchpoint probe
│       └── re_mame_trace_normalize.ps1 Normalize MAME/BizHawk raw events to neutral trace
└── tests/
    ├── check_file_limits.cmake           Enforces <=500-line rule through CTest
    ├── byte_grid_test.cpp                Synthetic world-grid/footprint tests
    ├── graphics_decompress_test.cpp      Synthetic decompressor behavior tests
    ├── graphics_decompress_reference.cpp ROM-backed differential oracle verifier
    ├── genesis_graphics_test.cpp         Synthetic tile/palette conversion tests
    ├── rom_identity_test.cpp             Synthetic ROM/hash/header tests
    ├── player_test.cpp                   Deterministic input and movement tests
    ├── player_reference.cpp              Local USA-ROM oracle for player vectors
    ├── entity_pool_test.cpp              Synthetic raw entity-pool/active-record tests
    ├── entity_pool_reference.cpp         Local USA-ROM oracle for entity pool loops
    ├── event_router_test.cpp              Synthetic raw event producer/router tests
    ├── event_router_reference.cpp         Local USA-ROM oracle for event boundaries
    ├── re_slice_decoder_test.cpp           Synthetic bounded decoder/report tests
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
    ├── re_natural_reference.cpp              Local USA-ROM natural reachability oracle
    ├── runtime_test.cpp                  Deterministic frame/input sequence tests
    ├── screen_descriptor_test.cpp        Synthetic screen descriptor tests
    ├── terrain_collision_test.cpp        Synthetic terrain-gate tests
    ├── smoke.cpp                         Minimal build/runtime smoke test
    └── vdp_test.cpp                      VDP storage/bounds/attribute tests
```

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
