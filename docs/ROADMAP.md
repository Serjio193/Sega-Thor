# Development Roadmap

The roadmap is ordered. Do not skip ahead unless a blocking dependency is documented.

## Status legend
- `ACTIVE` — current milestone being worked on
- `NEXT` — immediate queued milestone when useful to distinguish it from general TODO work
- `TODO` — planned
- `BLOCKED` — cannot proceed until documented dependency is resolved
- `DONE` — acceptance criteria met

## M11.37 — Controlled dynamic coverage expansion — DONE
Starting from the M11.36 bridge proof, freeze exact before metrics for the
unchanged canonical 600-frame scenario, then use only natural execution to
build a bounded queue of approximately 10–25 blocks outside the existing six.
Every selected block must pass exact decode, independent semantic verification,
mechanical generation/provenance and authoritative GPGX shadow certification
before bounded native promotion. No runtime JIT or whole-ROM coverage claim.

Gate result: `CONTROLLED_DYNAMIC_COVERAGE_EXPANSION_PROVEN`. Sixteen new
natural single-instruction blocks were promoted beside the six existing
entries. Shadow completed 388,308/388,308 with zero divergence; native
promotion completed the unchanged 600-frame scenario with exact checkpoint
and video hashes, 388,314 translated guest instructions, 12 interpreter
fallback entries, zero starts inside translated blocks and a 5.9844%
translated-instruction share. Four multi-instruction candidates were rejected
fail-closed after observed interrupt interleaving; one hardware-visible
candidate was rejected before promotion. No runtime JIT or ROM-byte coverage
claim was added.

## M11.36 — GPGX timing / refresh bridge contract — DONE
Resolve the M11.35 cycle/refresh observer boundary without expanding the
candidate set. GPGX source evidence establishes frame-relative accumulated
`m68k.cycles`/`refresh_cycles`, frame rebasing through `mcycles_vdp`, normal
interrupt polling at `m68k_run` entry, and the post-instruction comparison
boundary before the next scheduler transition.

Gate result: `GPGX_TIMING_REFRESH_BRIDGE_PROVEN` and
`DEMAND_DRIVEN_BLOCK_PROMOTION_PROVEN`. The existing six-entry registry passed
185,975/185,975 shadow comparisons with zero divergence; native promotion then
passed the unchanged 600-frame scenario with exact checkpoint/video hashes,
185,975 native entries, zero original-body starts and zero observed interrupts.
The M11.35 negative result remains historical evidence.

## M11.35 — Demand-driven block promotion pilot — DONE (initial gate blocked)
Run one bounded 600-frame natural trace, select only a small set of additional
guest block entries, decode them through the existing exact IR, independently
verify newly required forms, generate provenance-bound C++ and require exact
GPGX shadow equivalence before any native promotion. Keep failed candidates on
the interpreter fallback and do not implement a runtime JIT or automatic trust.

Gate result: `DEMAND_DRIVEN_PROMOTION_RUNTIME_BLOCKED`. Discovery recorded 2,188
unique PCs and the bounded shortlist was `0x3A85E`, `0x3A8BA`, `0x3A88C`.
Independent TST vectors and generator/provenance checks passed. Shadow stopped
at the first candidate with `actual_cycles=74 expected_cycles=896114` and
`actual_refresh=228 expected_refresh=896268`; no M11.35 candidate was promoted.
The first blocker is the GPGX cycle/refresh and interrupt-boundary contract, not
a semantic pass or a whole-game coverage result.

## M11.34 — M68K semantic core — DONE
Extract and independently verify common instruction semantics, beginning with
CCR/SR and the instruction families emitted by M11.33. Keep the oracle and
generated hybrid boundary separate until the semantic helpers have their own
tests and machine-derived contracts.

Gate result: `M68K_SEMANTIC_CORE_INDEPENDENTLY_VERIFIED`. The exact seven
M11.33 emitted combinations passed 23 deterministic independent-reference
vectors, decode/length/provenance checks, and the unchanged M11.33 600-frame
GPGX shadow/native regression. No broader instruction or production surface was added.

## M11.33 — Recomp generator v1 — DONE
Generate the three M11.32 basic blocks from the shared decoder/exact IR and
execute the generated instruction sequence through developer-only hybrid
helpers. Generated output retains guest provenance and unsupported forms fail
closed. The existing 600-frame GPGX proof remains the behavioral gate.

Gate result: `MECHANICAL_BLOCK_GENERATION_PROVEN`. The canonical USA ROM
regenerated the three blocks byte-for-input deterministically; GPGX shadow and
native runs both completed 600 frames with 14/14 calls and matching state/video.

## M11.32 — Basic-block recompilation timing proof — DONE
Three naturally executed blocks (`0x2D66`, `0x604BC`, `0x61032`) now share a
minimal developer-only GPGX block boundary. Shadow comparisons are 14/14 with
zero divergence; native override completes the 600-frame neutral scenario with
matching serialized state/video and zero original-body instruction starts.
Keep this migration infrastructure separate from production runtime. Do not
expand coverage, repair `0x3820`, investigate ID3 or add AI generation here.

## M0 — Repository bootstrap — DONE
C++20/CMake bootstrap, ROM loader, minimal memory/VDP scaffolding and smoke test established.

## M1 — Project governance and documentation — DONE
Mandatory AI workflow, architecture/file map, 500-line rule, decision/research/work logs and task handoff are enforced.

## M2 — Identify supported ROM revision — DONE
Canonical engineering reference is USA retail `Beyond Oasis`; final C++ model remains region-independent.

Confirmed USA fingerprint:
- size `3145728`;
- CRC32 `C4728225`;
- SHA-1 `2944910c07c02eace98c17d78d07bef7859d386a`;
- SHA-256 `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

## M3 — Graphics decompression `0x00003820` — DONE
Verified native C++ translation of original routine `[0x3820, 0x3B3E)`.

Reference vectors:
- format A `0x16943C`: `1217 -> 3072`, SHA-256 `65e99e74020fedbdcb97c8249a5ccfe540aca5bb5d29bfb260352cd6f388c31a`;
- format B `0x1894EA`: `112 -> 128`, SHA-256 `167d4e5409f6b075b3b6f2bc61dbb747e8d8c857e8699745184ddf48d83bcda9`.

## M4 — Local asset inspection tool — DONE
`oasis_inspect` performs supported-ROM validation, native decompression, Genesis 4bpp tile decoding and local PGM/PPM export. Generated commercial data remains local/ignored.

## M5 — VDP data model and rendering primitives — DONE
Verified 64 KiB VRAM, 128-byte CRAM, 80-byte VSRAM, bounded byte/word access, standard tile pattern-name decoding, minimal plane/sprite data, explicit unsupported emulator semantics and green tests.

## M6 — Runtime frame/input skeleton — DONE
Portable controller snapshots, explicit integer frame stepping and deterministic replay-style tests are established without wall-clock or platform dependencies.

## M7 — World/map/collision foundations — DONE
Goal: decode room/map structures and collision semantics before translating player behavior.

Verified:
- screen dispatcher `0xC8F0` uses high/low bytes of the screen ID with group table `0xC92C` and resolves 26-byte descriptors;
- multiple USA ROM screen IDs resolve to fixed descriptor addresses and are covered by ROM-backed native tests;
- auxiliary byte-grid descriptors at `0xFF1766` / `0xFF1770` expose backing pointer, row stride and row shift;
- world coordinates map to 8-pixel cells; `0x9C40` is the confirmed single-cell reader;
- `0x9BF2` aggregates OR/AND values across an entity footprint and is reproduced by `ByteGridView`;
- low-nibble terrain codes are mapped by tables `0x96E8` / `0x96F8` into movement/height classes;
- `0x938E` is a confirmed directional terrain movement gate: carry set blocks, carry clear allows;
- portable byte-grid, footprint and terrain-gate tests pass;
- final build-test, reference-ROM workflow and M7 probe are green.

## M8 — Player system — DONE
Goal: translate player movement/state from verified binary behavior before attacks and presentation layers.

Tasks:
- identify player entity/update entry and call path;
- trace controller bits into movement intent;
- connect player movement to the M7 terrain gate;
- identify exact position/delta/facing/movement-state fields used by the path;
- translate the smallest portable movement slice with integer/fixed-point semantics;
- add deterministic movement and blocked-movement tests;
- add ROM-backed behavior evidence where practical;
- only then expand into animation and attacks.

Acceptance criteria:
- player movement/update path documented with addresses and confidence labels;
- portable movement state/update exists without platform/render dependencies;
- collision uses the single M7 world model;
- deterministic tests cover free and blocked movement;
- reverse-engineering ledger/worklog updated;
- files remain <= 500 lines;
- CI green.

Delivered on `main`: the portable movement/state slice, M7 terrain-gated integration,
ROM-backed evidence, raw velocity context boundary, local Debug/Release verification
and successful GitHub Actions CI. The commercial ROM remains local-only.

## M9 — Entities/enemies/NPCs — DONE
Goal: translate the common entity framework and one evidence-backed representative
non-player callback-dispatch path.

Delivered on `main`: raw pool descriptors and bounded record views, the shared
movement field-access contract, a representative `FF2954` dispatch gate, local
Debug/Release verification and successful GitHub Actions CI. AI, attacks,
animation and spawn semantics remain explicitly outside this milestone.

## M10 — Spirits — DONE
Goal: reproduce one evidence-backed spirit lifecycle slice: summon/slot state,
target selection and one ability/interaction contract. The first accepted slice
now includes the caller-gated raw summon-entry seed at `0x7A10 -> 0x846C`,
four event-backed slot bits, raw dispatch and first-match target selection.
Rendering, audio and unproven spirit semantics remain outside the milestone.
The milestone acceptance slice is complete with local Debug/Release verification
and successful GitHub Actions CI; remaining unknown relationships stay documented
as unknown and may be revisited when later evidence requires them.

## M11 — Scripts/events/dialogue — DONE
Goal: reproduce game progression and event semantics. The first task is to
prove one bounded event/script router operation from its ROM data source.

Current slice: the type-8 producer at `0x82AE` transfers raw fields to
`FF1976/FF1978/FF197A`, and `0x7A28` maps the raw event byte to bounded handler
addresses. Event, progression and dialogue meanings remain unknown.

The evidence-backed bounded M11 slice is complete with native tests, local
USA-ROM verification and CI. The post-M11 RE-acceleration checkpoints are also
complete: separate bounded decoder/report tooling covers `0x60004` and the
representative routines `0x3820`, `0x8E90`, `0xA6A4` and `0xD3B2`, plus a
bounded dynamic trace scenario for `0xA7D4..0xA7E4`, without adding gameplay
behavior. A fourth bounded comparison tool fingerprints the user-supplied USA
Beta 1994-11-01 ROM and compares only the requested `0x3820`, `0x60004`,
`0x82AE`, `0x7A28` and `0xA6A4` targets using normalized opcode/CFG signatures.
Unknown dialogue, progression, driver and cross-revision semantics remain
explicitly unimplemented. Full-game tracing, broader similarity search,
whole-ROM discovery, recompilation and M12 remain out of scope until explicitly
authorized.

Post-M11.5 evidence checkpoint: M11.6 verified static translation PoC is DONE.
It verified three small developer-only compiled C++ slices (`0x3820`, `0xA8DA`
and `0x62CC`) with bounded differential fixtures and no runtime interpreter.
The result is `STATIC_TRANSLATION_POC_NEEDS_FIXUPS`; mechanical translation is
retained only as a verification aid. No next-step implementation is authorized
by that checkpoint.

Post-M11.6.1 evidence checkpoint: M11.7 single-target reachability root cause
is DONE. Static incoming analysis and watch-only BizHawk observations show
`CALLER_NOT_REACHED` for `0x62CC` under both existing scenarios: none of its
33 direct incoming PCs executed. The next recommendation is one separately
authorized minimal natural scenario; no scenario is created here.

Post-M11.7 evidence checkpoint: M11.8 is `ROOT_CAUSE_ADVANCED`. A new
hardware-reset natural scenario with title/start, movement, attack/use and
interaction/room input phases ran 1800 frames with exact hooks for all 33
incoming sites, player/event owners and the `0x3820` positive control. The
positive control remained live, while the target and all incoming sites stayed
unreached. The probe now records input schedule, frame PCs and selected RAM
state. The next bounded action is a timing/hold-input sweep around the
startup-to-gameplay transition; production C++ and M12 remain out of scope.

Any later follow-up remains evidence-gathering only while its implementation-
slice confidence remains below 90%; production C++ must not expand until
caller/data or downstream-handler evidence raises that slice to >=90%.

## M11.5 — Recursive structural exploration — DONE
Goal: automate bounded, provable 68000 structural exploration while retaining
explicit uncertainty frontiers and a reusable ROM address map.

Delivered: developer-only oasis_re_explore reuses re_slice_decoder, re_atlas
and re_candidate_map; it applies tiered provenance seeds, a deterministic
priority worklist, guarded recursive control-flow traversal, explicit
analysis states, compact map ranges and stable blocker identities. Bounded
validation runs before the optional single ROM-wide measurement.

The fresh USA Ghidra 12.1.3 raw export was repeated A/B with identical
390972-byte SHA-256
613A3AA6DEB8D2DCF994C82ADC6A6939B7D5F27AF67A51D05C16F090D60A5315.
The bounded corpus passed: 15 entries and all six current direct-edge
anchors were recovered. The gated ROM-wide evidence run processed 537 entries,
decoded 19623 instructions and emitted 148 frontiers; 451 entries were
analyzed, 25 stopped on indirect flow and 28 on unsupported instructions.
The result is structural evidence only; no Atlas classification or production
runtime behavior was changed. The dynamic ant PoC remains the next separately
authorized checkpoint and is not implemented here.

## M11.5 — Single-ant closed-loop evidence bridge — DONE
Goal: resolve exactly one current indirect-flow frontier through one bounded
natural emulator job, preserve its provenance, and measure one static rerun.

Delivered: deterministic `oasis.m68k.re-ant-job.v1` and
`oasis.m68k.re-ant-result.v1` tooling, one BizHawk natural-reset worker,
natural-only A/B validation and a provenance-tagged dynamic explorer edge.
Frontier `0x020E:0x045A` resolved to `0x307A` twice with result hash
`0x21238399`; the rerun closed one frontier and added 480 instruction bytes.
The worker is a developer-only bridge; production runtime and architecture are
unchanged. No scheduler, swarm, second frontier job or M12 work is authorized
by this checkpoint.

## M11.5 — Single-worker sequential ant queue PoC — DONE
Goal: process a small bounded frontier batch through one emulator worker while
preserving deterministic identity, lifecycle, natural-only provenance and
honest failure states.

Delivered: frozen `oasis.m68k.re-ant-queue.v1` queue of exactly five existing
ant jobs, one-claim lifecycle with stale recovery and resolved-duplicate
suppression, deterministic A/B comparison, sequential BizHawk process restarts,
and one batch merge into `oasis_re_explore`. Three jobs produced natural
dynamic edges (`0x307A`, `0x6211A`, `0x62900`); two bounded fallback jobs were
`FAILED_FINAL`/`NOT_REACHED`. Explorer frontiers changed `148 -> 145`, with
three provenance-tagged dynamic edges. This checkpoint ends before parallel
workers, a scheduler, a larger queue, or M12.

## M11.5 — Ant reachability diagnostic — DONE
Delivered: a bounded reachability-only extension to the existing natural
BizHawk probe, a deterministic ten-context sample, two existing scenario
checks, static source validation and a fresh positive control. All sampled
contexts remained `NOT_REACHED`; the environment was healthy, so the result is
`SCENARIO_COVERAGE_INSUFFICIENT`. The next recommendation is a separately
authorized small set of natural gameplay scenarios. No queue expansion,
parallelism, checkpoint farm or production runtime work was added.

## M11.9 — Reassemblable disassembly pipeline PoC — DONE
Post-M11 tooling checkpoint M11.9 is DONE: five selected slices (330 instructions,
882 bytes) round-trip exactly through decoder-owned typed IR and vasm. The local
bounded split also matches. Result: `REASSEMBLABLE_DISASM_POC_HIGH_VALUE`.
Next recommendation is A, expand to 25–50 verified routines; not implemented.
M12 remains TODO.

## M11.10 — Diverse reassembly coverage expansion — DONE
The bounded corpus now contains exactly 25 mass/explorer-selected routines,
including the three M11.9 controls. Shared decoder metadata and ASM emission
cover 105 observed practical forms with 105/105 exact round trips; the expanded
split and legacy M11.9 mixed split both match. No routine-specific encoding
patches or raw opcode directives were used. Runtime meaning remains unknown.
Decision: `DIVERSE_REASSEMBLY_HIGH_VALUE`. The one queued recommendation is A,
establish a full-ROM split baseline; it is deferred and not implemented here.

## M11.11 — Full-ROM split reassembly baseline — DONE
The trusted M11.10 ranges now anchor a deterministic manifest spanning the
canonical ROM. The full 3,145,728-byte output matches byte-for-byte; uncertain
regions remain local-ROM-backed blobs, with zero gaps and overlaps. Decision:
`FULL_ROM_SPLIT_EXACT`. Recommendation A — replace verified blob regions with
ASM/data — is deferred and not implemented here.

## M11.12 — Automated blob-to-source promotion PoC — DONE
Existing candidate-map and mass-verification evidence now drives deterministic
bounded promotions. Twenty-one of 25 attempted candidates were accepted after
exact slice and full-ROM checks; ASM coverage increased from 1,846 to 2,632
bytes and the canonical hashes remained exact. Decision:
`AUTO_PROMOTION_HIGH_VALUE`. Recommendation A — increase the automatic batch to
100 candidates — is deferred and not implemented here.

## M11.13 — Automated promotion scale pass — DONE
The post-M11.12 manifest was carried forward without repeating accepted
candidates. One hundred deterministic candidates were attempted and 84 accepted
after exact slice and full-ROM checks. ASM coverage increased from 2,632 to
6,462 bytes; canonical full-ROM hashes remained exact. A generic
immediate-to-CCR emitter syntax fix (.b for CCR) was regression-tested and
accepted both prior CCR-blocked candidates. Decision: AUTO_PROMOTION_SCALE_HIGH_VALUE.
Recommendation A — continue automatic code promotion with another large batch —
is deferred.

## M11.14 — Automatic promotion large batch II — DONE
The post-M11.13 manifest excluded prior attempts and carried two old slice
mismatch retries. Ninety-one attempts accepted 73 (80.22%); acceptance windows
remained stable and ASM grew to 13,550 bytes. Full-ROM hashes stayed exact.
Rejects are clustered by systemic form; unsupported and nonrepresentable vasm
forms remain explicit. Decision: AUTO_PROMOTION_BATCH2_HIGH_VALUE.
Recommendation A — run another automatic code batch — is deferred.

## M11.15 — Evidence integrity audit and classification trust repair — DONE
All 203 M11.14 ranges were audited with per-artifact round-trip checks, ROM
identity, entry/range provenance, concrete incoming xrefs and caller trust.
The resulting ladder is 197 `ASM_ROUNDTRIP_EXACT`, 5
`CODE_STATIC_SUPPORTED`, 1 `CODE_EXECUTED` and 0 `BEHAVIOR_VERIFIED`; the full
ROM remains exact. One known Ghidra boundary mismatch at `0x3820` remains a
bounded fixup. Decision: `EVIDENCE_TRUST_NEEDS_FIXUPS`. Recommendation B —
targeted dynamic confirmation of critical code — is deferred.

## M11.16 — Targeted dynamic code confirmation — DONE
Five critical ranges were selected before any run and checked against retained
natural reports. None had a verifiable target-hit artifact; the existing
`0x6121A` natural positive control passed exact ROM/scenario/hash checks. No
promotion, emulator run, or scenario expansion was performed. Decision:
`TARGETED_DYNAMIC_REACHABILITY_LIMITED`. The single deferred recommendation is
D — one bounded timing/hold-input sweep around the M11.8 startup transition.

## M11.17 — Structured data classification PoC — DONE
The bounded proof accepted nine exact structured-data ranges and one weaker
header region, covering 1,164 bytes without changing the blob-backed full-ROM
representation. Invalid pointer-like data and code/data overlap are rejected
or recorded as conflicts, and the future code-promotion gate vetoes only
trusted data classifications. Decision: `STRUCTURED_DATA_HIGH_VALUE`. The
single deferred recommendation is A — return to the native vertical slice.

## M11.18 — Native controlled screen vertical slice — DONE
Goal: prove the first interactive native path with explicit synthetic geometry:
input, deterministic logical update, player movement, terrain collision, software
framebuffer and visible window.

Delivered: `ControlledScreen` reuses the M6/M7/M8 runtime, player and byte-grid
contracts; a 32x28 test fixture renders free terrain, walls, a corner and an
isolated obstacle; `oasis_platform` provides a minimal Windows Win32 input/window
adapter with focus-loss clearing and scaled DIB presentation. A 600-frame input
replay compares state after every frame and is invariant across presentation
intervals. The framebuffer has a fixed SHA-256 oracle, and canonical ROM identity
is enforced before opening the window.

Validation closure: the canonical-ROM Win32 executable was manually checked for
window creation, cardinal and diagonal movement, release, focus-loss clearing,
focus restoration, visible fixture-wall collision and responsiveness. The
fixture is synthetic and does not claim an original room. MSVC, Linux,
deterministic replay/hash and ROM identity checks passed locally. GitHub Actions
run `34052574735` passed for the closure commit; the final documentation-only
commit `4118867a487227275d18246c93b8ebbe7980d8f2` also passed exact-SHA CI in
run `34052627403`. The non-Windows
GUI adapter remains intentionally unavailable and is recorded as the next
recommendation rather than an M11.18 Win32 runtime blocker. Decision:
`NATIVE_VERTICAL_SLICE_PLAYABLE`. The single recommendation is D — platform
portability.

Post-M11.25 native resource checkpoint: resource ID 3 is now loaded from the
verified canonical-ROM table contract, decompressed by the existing native
routine, transferred to native VRAM `0x4000..0x4FFF` and rendered as a
diagnostic 128-tile atlas. The result is
`NATIVE_ROM_RESOURCE_ID3_HIGH_VALUE`; no room, background, player or sprite
semantic role was promoted. The single deferred recommendation is A — identify
the resource's palette/visual role.

## M11.28 — Hybrid native migration PoC — DONE (shadow); override BLOCKED
M11.27 manual ID3 runtime hunting is stopped by explicit user direction.
The developer-only `0x3820` hook observes six natural calls in the existing
600-frame neutral startup scenario. Three shadow runs provide 18 clean bounded
comparisons and identical emulator checkpoints/video to EMULATED. Result:
`HYBRID_SHADOW_PROVEN_OVERRIDE_BLOCKED`. CCR.X, interrupt-aware timing and
prefetch/IR continuation remain unproven, so override is rejected before
execution. No production emulator dependency is introduced. STOP; no follow-up
routine, gameplay search or tooling expansion is active.

## M11.29 — Minimal safe native override target — DONE
The naturally executed `0x2D66` leaf was selected from the existing 600-frame
neutral PC bitmap: ten decoded instructions, direct `RTS`, one local `DBF`, no
hardware access, no nested call and bounded full-SR/RAM/stack effects. One
SHADOW_NATIVE call compared cleanly with zero divergences. One NATIVE_OVERRIDE
call skipped all original target-body instruction starts, completed 600 frames,
and matched EMULATED checkpoint and video sequences exactly. Result:
`HYBRID_NATIVE_OVERRIDE_MINIMAL_PROVEN`. The adapter remains developer-only;
`0x3820` timing/CCR.X/prefetch work, ID3, manual gameplay, coverage expansion
and AI generation remain out of scope.

## M11.30 — Hybrid migration small batch — DONE (shadow); override not repeatable
The shared developer-only registry covers `0x2D66`, `0x604BC` and `0x61032`.
All 14 natural calls shadow cleanly with zero divergences and no interrupts.
The simultaneous native run skips every original body and completes the
scenario, but serialized VDP/sound state diverges from EMULATED because the
minimal boundary lacks the exact per-instruction bus-refresh/hardware phase
contract. Result: `HYBRID_OVERRIDE_NOT_YET_REPEATABLE`. Stop here; do not expand
timing work or production dependencies.

## M11.31 — Comparative disassembly method transfer — DONE
Pinned public Streets of Rage 2/3 projects were compared with existing Beyond
Oasis evidence. The RuiNelson disassembly/recompilation workflow is retained as
`PARTIAL_TRANSFERABILITY`; IDA assembly and Pancakes are `SPECIALIZED_ONLY`.
Programmer-style similarity is `NON_DISCRIMINATING`, binary structure is only
`PARTIAL`, and sound-driver lineage is `SPECIALIZED_ONLY`/unresolved. The
`0x2D66` dry-run confirms workflow compatibility without importing code or
changing runtime dependencies. No high-confidence template was found.
Possible M11.32: one bounded Beyond Oasis evidence-led pass using the adapted
sequence in the report. Do not broaden coverage or repair hybrid timing here.

## M12 — Inventory/UI/save — TODO
Goal: menus, inventory, item behavior and compatible save semantics.

## M13 — Audio — TODO
Goal: faithful music/SFX playback with the narrowest viable compatibility strategy. Audio architecture requires an ADR before implementation.

## M14 — Full-game parity — TODO
Goal: complete game from start to credits with regression coverage.

## M15 — Portability and packaging — TODO
Initial targets: Windows, Linux and macOS. Additional platforms are later decisions.

## M16 — Optional enhancements — TODO
Only after base parity: scaling/resolution, controller UX, widescreen experiments, QoL and rendering enhancements. Faithful mode remains available.

## Deferred
**The Story of Thor 2 / The Legend of Oasis** is explicitly deferred until the first project reaches a mature parity milestone.
