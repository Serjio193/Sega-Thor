# Architecture Decision Log

Use this file for decisions that can redirect architecture, dependencies, scope, or reverse-engineering strategy.

## ADR-0025 — Promote only the canonical 0x03A7AE generated block
**Status:** Accepted for M11.44 developer-only hybrid tooling
**Date:** 2026-09-09

**Context:** After M11.43 restored the authoritative checkpoint identity, the
M11.42 restart gate reproduced the frozen 661,916 interpreter remainder. The
hottest pair, 0x03A7AE `TST.W ($00FF1654).L` and 0x03A7B4 `BNE.W`, retained the
M11.35 IR/prefetch rejection (`actual 0x4E73`, expected `0x4A79`).

**Decision:** Re-run the exact decoder-owned pair through the current generic
timing/refresh, instruction-boundary, checkpoint-canonicalization and GPGX
shadow contracts. Promote it only as generator output with no address-specific
semantic body or timing constant. Keep all other remaining forms fail-closed,
retain 0x060BA4 as hardware-visible fallback, and close the complete remainder
with a per-PC ledger rather than using an `other` bucket.

**Consequences:** The historical rejection is preserved but classified obsolete.
The pair passed 100,948 per-instruction shadow comparisons with zero divergence;
the unchanged 600-frame native proof preserves checkpoint/video/CPU/boundary
identity and removes 100,948 interpreter executions. Final translated share is
91.3548%, below the 95% gate, so the result is
`REMAINING_INTERPRETER_ATTRIBUTION_PROVEN_SEMANTICS_BLOCKED`.

## ADR-0024 — Canonicalize only the proven pinned-GPGX representation layout
**Status:** Accepted for M11.43 developer-only hybrid tooling
**Date:** 2026-09-09

**Context:** M11.42 found that the M11.41 adapter left a YM2612 host pointer at
serialized offset `140734`. The pinned DLL's raw state also showed its Z80
`daisy` pointer at `144576`. The available external `state.c` source and stale
build objects were not byte-for-byte synchronized with that DLL: source-only
arithmetic predicts an earlier Z80 position, while raw cart mapping and pointer
evidence prove the pinned binary position.

**Decision:** Model the raw-saved YM2612/Z80 structs with machine-checked x64
`sizeof`/`offsetof` assertions and construct one non-overlapping representation
span table from those fields. Guard `STATE_SIZE=0xfd000` and version
`GENPLUS-GX 1.7.6`; reject unknown values. Canonicalization copies the raw
buffer and clears only the table's 55 host-pointer, one function-pointer and
111 ABI-padding spans. It does not clear semantic bytes or broaden hardware
serialization. The active USA cart path is separately serialized field-by-field;
optional SVP wholesale state is out of scope for this active baseline.

**Consequences:** The complete contract produces the new authoritative aggregate
`251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d` across five
current proofs and current/exact-historical/M11.41 raw replay. M11.41's
`c9236218...` remains historical and is superseded because its adapter erased
semantic bytes. M11.42 PHASE 2 remains unstarted.

## ADR-0023 — Keep M11.42 closed on checkpoint identity mismatch
**Status:** Accepted for M11.42 gate governance
**Date:** 2026-09-09

**Context:** The M11.41 repair was expected to reproduce authoritative
checkpoint aggregate `c9236218...` from commit `5c19e22`. Two fresh runs matched
all execution and video metrics but produced `d5de401c...`. Raw evidence shows
the M11.41 canonicalization leaves a host-pointer byte at serialized offset
`140734`.

**Decision:** Treat the restart as blocked. Do not classify the remaining
interpreter executions, broaden semantic forms, generate candidates, run shadow
promotion, broaden hardware behavior or weaken the checkpoint identity gate.
Repair and independently re-prove the developer-only identity contract in a
separate bounded task before resuming M11.42.

**Consequences:** M11.39, M11.40 and M11.41 evidence remain preserved; no
coverage or attribution result is claimed from this run. Production/native
architecture is unchanged.

## ADR-0022 — Deterministic developer-only checkpoint identity
**Status:** Accepted for M11.41 developer-only hybrid tooling
**Date:** 2026-09-09

**Context:** M11.40 matched ROM, GPGX, video, instruction and boundary metrics
but could not reproduce the historical checkpoint aggregate. Byte evidence
showed that GPGX v1.7.6 wholesale-serialized host pointers and ABI padding in
the YM2612 and Z80 contexts. The first mismatch was frame 60, state offset
`140654`, inside `FM_SLOT.DT`.

**Decision:** Preserve the complete raw `retro_serialize()` buffer in ignored
developer evidence, but compute the authoritative checkpoint identity from a
copy with only the proven pointer/padding spans cleared for the recognized
`STATE_SIZE=0xfd000` format. Reject unknown state sizes. Keep the existing
frame cadence and aggregate order; retain all semantic state bytes and record
both raw and authoritative per-record hashes.

**Consequences:** Checkpoint identity is deterministic across the current and
exact historical checkout: the repaired 600-frame aggregate is
`c9236218f55fb18f7f1d5095e4970b25f228de588bd03bd7cbbffccc2e225fd1`. Raw
evidence remains available for future audits but is not tracked. This does not
start M11.40 PHASE 2 and does not add a production emulator dependency.

## ADR-0021 — Bounded hot-path profile and exact multi-block promotion
**Status:** Accepted for M11.39 developer-only hybrid tooling
**Date:** 2026-09-09

**Context:** M11.38 proved generic instruction-boundary continuation, but
661,916 interpreter executions remained after the exact M11.38 registry. The
next coverage step needed a dynamic-payoff ranking without becoming an
indefinite discovery pass or weakening the exactness gate.

**Decision:** Emit a deterministic interpreter-PC profile from the existing
M11.38 registry run, rank by dynamic instruction executions, and consider a
bounded maximum of 40 decoder-owned ranges. Promote only candidates whose exact
forms pass the independent semantic harness, whose bodies are generated from
canonical ROM provenance, and whose every instruction boundary passes the
existing GPGX shadow contract. Keep indirect control unresolved and hardware
visible candidates in fallback unless an existing bridge already proves them.

The M11.39 set contains `[0x000380,0x0003A0)` and `[0x03A864,0x03A868)`.
The first required one new exact semantic form, `ADD.W (An)+,Dn`; the second
reused the verified direct Bcc form. Generated code and metadata remain
separate from handwritten helper/registry glue, and promotion remains offline.

**Consequences:** Native translated dynamic share rose from 63.5261% to
89.7991% with exact 600-frame checkpoint/video, CPU, interrupt and boundary
evidence. The remaining profile is reported conservatively; no latent function
boundaries or indirect target sets are claimed. M11.40 remains a separate
milestone and is not implemented here.

## ADR-0020 — Generic instruction-boundary yields for rejected multi-instruction blocks
**Status:** Accepted for M11.38 developer-only hybrid tooling
**Date:** 2026-09-08

**Context:** M11.37 rejected four mechanically generated two-instruction ranges
because natural execution showed interrupt interleaving inside their ranges.
GPGX owns interrupt service, trace handling, cycle/refresh state and the VDP/Z80
schedule; treating the ranges as atomic would change observable ordering.

**Decision:** Make generated execution instruction-granular at the bridge
boundary. After each guest instruction, the generated body asks one generic
GPGX-owned boundary callback and returns `BlockExit` with the exact next PC,
reason and executed count when continuation is required. The shadow adapter
compares registers, SR, PC, prefetch, RAM/bus effects, cycle/refresh and
interrupt-visible fields at every boundary. Native continuation re-enters the
same generated block at the exact instruction entry PC; GPGX performs any
pending interrupt before the continuation. Do not add candidate-specific
timing, a second scheduler, atomic blocks, runtime JIT or production linkage.

**Consequences:** The four frozen M11.37 ranges pass the per-boundary shadow
gate and native 600-frame equivalence; 274 naturally observed continuations
resumed after actual GPGX interrupt service. The capability is conservative and
instruction-granular, and further coverage still requires a separate milestone.

## ADR-0019 — Controlled dynamic block promotion remains offline and generic
**Status:** Accepted for M11.37 developer-only hybrid tooling
**Date:** 2026-09-08

**Context:** M11.36 made the GPGX timing/refresh boundary authoritative. M11.37
needed to expand coverage beyond six entries without turning discovery into a
runtime JIT or trusting generated code by construction.

**Decision:** Use one bounded natural trace, an explicit candidate queue,
independent semantic verification of the used Bcc/DBcc/TST forms, existing
decoder-owned mechanical generation, and the same GPGX shadow gate before
promotion. Store block boundaries and instruction counts as generated metadata;
the registry dispatches through that metadata rather than a candidate-specific
hard-coded list. Reject a candidate if its block crosses an observed interrupt
or hardware-visible boundary. Keep all discovery, generated bodies and GPGX
bridge code in developer-only tooling; do not add runtime JIT, whole-ROM
translation or ROM-byte coverage claims.

**Consequences:** Sixteen new single-instruction natural blocks passed the
full shadow/native gate. Four multi-instruction candidates remain unpromoted
because the trace showed interrupt interleaving; a hardware-visible candidate
was rejected fail-closed. Further expansion requires a new bounded milestone.

## ADR-0018 — GPGX post-instruction bridge comparison boundary
**Status:** Accepted for M11.36 developer-only hybrid tooling
**Date:** 2026-09-08

**Context:** M11.35 compared a prediction sampled before a direct TST entry with
an execution event observed at the next instruction boundary. GPGX rebases its
frame-relative `m68k.cycles` and `refresh_cycles` counters at frame end, so the
first mismatch was `896114/896268` versus `74/228` even though the instruction
effect itself was exact.

**Decision:** Keep GPGX as the timing, refresh, hardware and interrupt oracle.
Expose one generic post-instruction hook from the normal interpreter after
semantic and cycle/refresh advancement, before the next scheduler/frame
transition. Close shadow comparison there. Keep the existing entry hook for
block dispatch. A translated block remains valid only for bounded code whose
accesses cannot expose a GPGX-observable boundary; interrupt polling and trace
handling remain outside generated semantics and are never crossed atomically.

**Consequences:** No baseline subtraction, magic constant, candidate-address
patch or parallel timing engine is needed. The old M11.35 negative result stays
historical. The existing M11.33 plus three M11.35 blocks pass the full shadow
and native 600-frame gate; future blocks still require the same exact contract.

## ADR-0017 — Fail-closed demand-driven block promotion gate
**Status:** Accepted for M11.35 developer-only pilot
**Date:** 2026-09-08

**Context:** M11.33 generated three proven blocks and M11.34 independently
verified their used instruction semantics. The next bounded experiment needs a
repeatable way to select naturally executed blocks without turning the project
into a whole-ROM recompiler or trusting generated output by construction.

**Decision:** Permit only one bounded natural trace, exact decoder-owned block
IR, independent vectors for newly required forms, provenance-bound generated
bodies and full GPGX shadow comparison before promotion. Keep candidate bodies
and discovery evidence in developer-only tooling. If semantic or runtime shadow
evidence fails, do not promote the candidate and retain interpreter fallback.
Do not add runtime JIT, automatic trust, or whole-game ranking in this pilot.

**Consequences:** M11.35 may end in a negative gate result while preserving
useful discovery, semantic and generator evidence. The first runtime blocker is
recorded rather than hidden by running native execution. Any future promotion
must establish the missing cycle/refresh and interrupt-boundary contract in
a separately bounded task.

**Result:** `DEMAND_DRIVEN_PROMOTION_RUNTIME_BLOCKED` at `0x3A85E`, with
`actual_cycles=74 expected_cycles=896114 actual_refresh=228
expected_refresh=896268`.

## ADR-0016 — Test-only independent M68K semantic oracle
**Status:** Accepted for M11.34 verification tooling
**Date:** 2026-09-08

**Context:** M11.33 established decoder-owned generated execution, but shared
decoder, emitter and helper assumptions could agree while encoding an incorrect
68000 rule.

**Decision:** Verify only the seven currently emitted M11.33 combinations with
a small deterministic reference model transcribed from the Motorola/NXP 68000
Programmer's Reference Manual. Keep it in tests/developer tooling, separate
from the decoder, emitter and production targets; do not broaden the instruction
surface to make the verification table larger.

**Consequences:** A mismatch must remain visible as a first-mismatch vector and
be fixed only with sufficient evidence. Passing this checkpoint establishes
independent verification of the bounded used subset, not a general CPU core or
whole-ROM semantic claim.

**Reference:** `https://www.nxp.com/docs/en/reference-manual/M68000PRM.pdf`

## ADR-0015 — Decoder-owned generated basic-block bodies
**Status:** Accepted for developer-only migration tooling
**Date:** 2026-09-08

**Context:** M11.32 proved three manually written basic-block bodies, while
the next milestone must remove routine-specific instruction authoring without
changing the proven GPGX timing and hardware boundary.

**Decision:** M11.33 uses the existing decoder/exact IR to generate C++ calls
to a small hybrid instruction-helper boundary. Generated functions retain the
guest PC, opcode and decoded assembly as provenance. The initial generated set
is exactly the three M11.32 blocks; unsupported forms fail closed. The oracle
prediction path remains separate until M11.34 verifies common semantics.

**Consequences:** Adding an ordinary supported block can use generator output
rather than a handwritten body, while the hybrid bridge, GPGX state and
generated artifact remain developer-only. This does not create a production
CPU emulator or authorize semantic promotion.

**Affected files/milestones:** `src/tools/hybrid/`, `CMakeLists.txt`, M11.33.

## ADR-0014 — Developer-only GPGX basic-block replacement boundary
**Status:** Accepted for migration experiments only
**Date:** 2026-09-08

**Context:** The M11.30 atomic override skipped GPGX instruction-body timing
and diverged in serialized VDP/sound state. A bounded proof needs native
mechanical execution while preserving GPGX's own fetch, memory bus, prefetch,
cycle and refresh behavior.

**Decision:** Add one developer-only block callback immediately before GPGX's
opcode dispatch. A registered block may perform its exact bounded operations
through helper functions implemented inside GPGX, then return control to the
normal CPU loop. The initial registry contains only `0x2D66`, `0x604BC` and
`0x61032`, each with an explicit state/effect contract. This is not a generic
M68K replacement engine and is not linked by production Sega-Thor targets.

**Consequences:** The three-block 600-frame proof can reuse GPGX hardware and
timing semantics and has passed with exact state/video equivalence. Every new
block still requires an independent shadow proof; uncertain side effects must
fail closed. The bridge, ROM PCs, emulated memory and GPGX remain confined to
developer tooling. See `docs/reports/BASIC_BLOCK_RECOMPILATION_TIMING_M11_32.md`.

## ADR-0001 — Native C++ reimplementation, not a general emulator
**Status:** Accepted

**Context:** The goal is to make Beyond Oasis run natively on modern systems while preserving original behavior.

**Decision:** Translate game routines and implement only the Mega Drive hardware semantics required by the game. Do not build a general 68000/Mega Drive emulator as the main architecture.

**Consequences:**
- more reverse-engineering work up front;
- clearer native game code long term;
- hardware compatibility layer must remain narrow;
- address/routine mappings must be preserved for traceability.

## ADR-0002 — User-supplied ROM owns commercial data
**Status:** Accepted

**Decision:** The repository never contains the original ROM or extracted commercial assets. Runtime/tools read a locally supplied ROM.

**Consequences:**
- Git repository remains source-only;
- CI tests use synthetic/non-copyrighted fixtures;
- local developer tools may export ignored files for inspection.

## ADR-0003 — Fidelity before enhancements
**Status:** Accepted

**Decision:** Reproduce original gameplay and rendering semantics before widescreen, HD, QoL, remaster behavior or Story of Thor 2 work.

## ADR-0004 — 500-line hard file limit
**Status:** Accepted

**Decision:** Source and project documentation files must not exceed 500 lines.

**Reason:** Keep modules understandable for humans and AI agents, discourage monoliths, make review and context retrieval reliable.

## ADR-0005 — USA reference binary, region-independent reconstructed game
**Status:** Accepted
**Date:** 2026-09-03

**Context:** The project aims to reconstruct the complete game so the native C++ runtime can target desktop, mobile, and future consoles. Regional retail binaries differ, while existing public reverse-engineering work and known addresses are based on the USA `Beyond Oasis` release.

**Decision:**
- Use the clean USA retail `Beyond Oasis` binary as the canonical engineering reference for addresses, traces, and differential verification.
- The reconstructed game model must be region-independent and must not embed USA ROM addresses in gameplay code.
- Europe and Japan are secondary evidence sources and future data profiles, not separate game implementations.
- ROM-specific offsets belong only in extraction/reverse-engineering metadata.
- Runtime gameplay code consumes normalized game data structures rather than raw ROM addresses.


**Alternatives considered:**
- Europe as primary reference: rejected because current public address knowledge targets USA.
- Supporting all regions equally from the beginning: rejected because it multiplies binary-diff work before core behavior is understood.
- Building a USA-only final runtime: rejected because it conflicts with the portable reconstruction goal.

**Consequences:**
- address annotations default to USA reference addresses;
- later regional support maps region-specific data to the same C++ game model;
- regional differences are documented rather than forked into separate engines;
- reference identity must be established before translating substantial 68000 routines.

**Affected files/milestones:** M2 onward, all reverse-engineering documentation and extraction code.

## ADR-0006 — Separate reassembly exactness from execution trust
**Status:** Accepted
**Date:** 2026-09-06

**Context:** M11.14 `CODE_VERIFIED` proved exact decoder/assembler bytes but did
not prove that each reconstructed range is executed code. Treating those facts
as one classification could make weak structural evidence appear trusted.

**Decision:** Keep the existing full-ROM ownership and ASM artifacts, while
reporting an evidence ladder: `ASM_ROUNDTRIP_EXACT`, `CODE_STATIC_SUPPORTED`,
`CODE_EXECUTED` and `BEHAVIOR_VERIFIED`. Static support requires an independent
anchor, vector/startup provenance, dynamic evidence, or an exact incoming xref
from an already trusted caller. Direct-caller counts and Ghidra boundary
agreement alone do not raise trust. The automatic promoter records a successful
round trip at the lowest level unless explicit evidence qualifies it.

**Consequences:** Existing ranges can be downgraded without changing bytes or
the exact full-ROM rebuild. Audits must retain artifact identity, ROM identity,
entry/range linkage and concrete xref sources. Missing dynamic evidence remains
unknown and is never synthesized.

## ADR-0007 — Natural runtime evidence is range-local and fail-closed
**Status:** Accepted
**Date:** 2026-09-06

**Context:** M11.16 needs to distinguish a naturally observed program counter
from a forced hook, a stale report, or a report whose provenance artifact is
missing. Dynamic reachability of one routine must not silently trust callers,
callees, or adjacent ranges.

**Decision:** Accept only `DYNAMIC_NATURAL` evidence whose ROM identity,
scenario, artifact hash, target address and audited half-open range all match.
Reuse of an existing artifact is preferred to rerunning an emulator. Missing
or forced evidence remains non-promoting, and trust changes are local to the
observed range. A pre-run selection report is mandatory for bounded target
passes.

**Consequences:** Dynamic passes can end with a reachability-limited result
without weakening static exactness. Historical runtime claims without a
retained artifact are explicitly reported as unaccepted context. Callers and
callees require their own evidence.

## ADR-0008 — Structured data classifications are explicit and non-heuristic
**Status:** Accepted
**Date:** 2026-09-06

**Context:** The full-ROM split contains millions of bytes whose apparent
shape is not proof of data. M11.17 needs a useful data inventory while keeping
unknown bytes and code/data boundaries honest.

**Decision:** Accept `DATA_STRUCTURE_SUPPORTED` only when an exact range,
element width, deterministic count/end, canonical byte identity and a proven
consumer or parser are present. Use `DATA_REGION_SUPPORTED` for a bounded
non-code region whose field semantics are incomplete. A trusted data range
creates a conflict record on code overlap and vetoes future code promotion;
weak or unknown data hypotheses do not veto.

**Consequences:** Structured data can be reported without replacing the
blob-backed full-ROM representation. Resource payloads remain unknown until
their own compressed boundaries are proven, and code/data conflicts cannot be
resolved silently.

## ADR-0009 — Bounded native controlled-screen platform seam
**Status:** Accepted
**Date:** 2026-09-06

**Context:** M11.18 requires the first interactive native vertical slice while
reconstructed-source and broad RE expansion are frozen. The repository has
portable runtime/game logic but no window or input backend.

**Decision:** Add a small `oasis_platform` adapter. On Windows it owns a native
Win32 window, keyboard polling, focus-loss clearing and scaled software-DIB
presentation. The game layer owns the deterministic fixture, movement and
software rasterization. Non-Windows builds keep a compile-only unavailable
adapter until a concrete backend is justified.

**Consequences:** The core/game path remains platform-independent and has no
new third-party dependency. The first playable runtime is Windows-only; this
is an explicit bounded limitation, not a claim of cross-platform GUI support.
The screen geometry is synthetic and cannot be used as evidence for an
original room.

**Affected files/milestones:** `src/platform/`, `src/game/controlled_screen.*`,
`src/game/render/`, M11.18.

## ADR-0010 — Address-level GPGX manual runtime evidence
**Status:** Accepted
**Date:** 2026-09-07

**Context:** The instrumented Genesis Plus GX core now provides persistent
manual-realtime executed-PC bitmaps for the canonical Beyond Oasis ROM. This
is stronger evidence than static reachability for individual instruction
starts, but it does not establish function boundaries, semantics, or complete
range execution.

**Decision:** Add `GPGX_MANUAL_REALTIME` as a provenance-bound evidence source
that records only `CODE_EXECUTED_AT_ADDRESS` facts. The importer must verify
the canonical ROM identity, bitmap size and metadata/file hashes, retain
unsupported decoder results, and reject odd, out-of-range or data-conflicting
addresses. Evidence is duplicate-safe by ROM, source and capture identity.
It may report existing-range coverage, but it must not promote or otherwise
change range classifications.

**Alternatives considered:** Promoting an entire static range from one
executed PC was rejected because it invents boundaries and branch coverage.
Reclassifying trusted data on runtime overlap was rejected because conflicts
must remain explicit and fail-closed. Replay or input automation was rejected
because this source is specifically manual realtime capture.

**Consequences:** Runtime execution evidence is available to later trust
analysis at address granularity. Unknown executed addresses remain separately
reported as `RUNTIME_EXECUTED_UNKNOWN`; `RUNTIME_DATA_CONFLICT` is retained as
a critical diagnostic. No gameplay code, `main`, ROM or extracted asset is
changed.

**Affected files/milestones:** `src/tools/gpgx_import_gpgx_coverage.cpp`,
`CMakeLists.txt`, M11.19.

## ADR-0011 — Developer-only hybrid migration experiment
**Status:** Accepted
**Date:** 2026-09-07

**Context:** The user explicitly replaces M11.27 manual ID3 hunting with a
bounded hybrid execution PoC for the already translated `0x3820` decompressor.

**Decision:** A developer-only adapter may observe natural GPGX calls, run
existing C++ translations on copied bounded inputs and compare their effects.
Its dispatch states are `EMULATED`, `SHADOW_NATIVE`, `NATIVE_OVERRIDE`. Shadow
always preserves the original CPU result. Override must fail closed until all
CPU, memory, return and execution-timing effects needed by continuation are
proven; output equivalence alone cannot authorize it.

**Consequences:** GPGX remains an external developer tool. No production target
may depend on the emulator, its CPU context, original PCs or its RAM layout
through this experiment. Exactly one routine is in scope; no emulator rewrite,
AI generation, routine expansion or manual gameplay search is authorized.

## ADR-0012 — Target-specific developer-only native override boundary
**Status:** Accepted
**Date:** 2026-09-07

**Context:** M11.29 requires one native override whose CPU/hardware contract is
simpler than `0x3820`, while preserving the original GPGX execution path and
avoiding a generic replacement engine.

**Decision:** Use the naturally executed `0x2D66` leaf as the single override
target. Its adapter captures and proves only its bounded registers, full SR,
source/output RAM and saved stack window. On override it applies those effects
and uses GPGX's existing `m68k_set_reg(PC)` transition rather than reimplementing
prefetch or IR state. The adapter stays in developer-only hybrid tooling.

**Consequences:** The 600-frame neutral scenario proves one native replacement
and exact checkpoint/video continuation. The proof does not generalize to
other routines; `0x3820` remains blocked by its separate timing/CCR.X/prefetch
contract. Production targets and dependencies remain unchanged.

## ADR template
Copy this block for new decisions:

```text
## ADR-NNNN — Title
Status: Proposed | Accepted | Superseded | Rejected
Date: YYYY-MM-DD

Context:

Decision:

Alternatives considered:

Consequences:

Affected files/milestones:
```

## ADR-0010 — Minimal developer-only hybrid replacement registry
**Status:** Accepted for migration experiments only
**Date:** 2026-09-07

**Context:** M11.29 proved one target-specific override. M11.30 tests repeatability
across a small natural batch without making the emulator hook a production
runtime dependency or a generic M68K replacement engine.

**Decision:** Keep a small registry mapping explicit ROM PCs to target-owned
`EMULATED`, `SHADOW_NATIVE` and `NATIVE_OVERRIDE` adapters. Each adapter owns
its exact state/effect contract; the registry only routes hook events and
aggregates metrics. Return/prefetch transitions use GPGX's internal bridge
state. Timing and hardware phase are not synthesized by the registry.

**Consequence:** Multiple routines can share the migration boundary, while
unsafe candidates fail closed. The M11.30 batch shadow is clean, but override
promotion remains blocked until the exact GPGX bus-refresh/VDP/sound phase
contract is proven. Production Sega-Thor code remains emulator-free.

## ADR-0013 — Comparative projects are method references only
**Status:** Accepted
**Date:** 2026-09-08

**Context:** M11.31 compares public Streets of Rage 2/3 disassembly,
extraction and static-recompilation projects with the existing Beyond Oasis
evidence. The comparison found no public exact-ROM reassembly proof and no
evidence sufficient to establish shared programmer style, binary modules or
sound-driver lineage.

**Decision:** Transfer only the bounded evidence workflow: canonical identity,
address-preserving maps, explicit unknown/indirect-entry ledgers, natural
evidence gates, and separation of generated/mechanical output from native
helpers. Do not import SoR labels, RAM/object/audio assumptions, generated CPU
recompiler machinery or external code/data. Keep sound lineage as a separate
specialized investigation.

**Consequences:** M11.32, if started, remains a localized Beyond Oasis pass
using existing decoders/contracts. Similarity claims stay separated into
programmer style, binary structure, RE method and sound lineage. No production
runtime dependency changes.

**Affected files/milestones:** M11.31 report and governance documents; no
production source or ROM data.
