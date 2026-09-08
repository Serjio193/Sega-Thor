# Architecture Decision Log

Use this file for decisions that can redirect architecture, dependencies, scope, or reverse-engineering strategy.

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
