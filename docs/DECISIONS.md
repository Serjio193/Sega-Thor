# Architecture Decision Log

Use this file for decisions that can redirect architecture, dependencies, scope, or reverse-engineering strategy.

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
