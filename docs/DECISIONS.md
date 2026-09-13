# ADR-0042 — Keep 0x062AE0 natural evidence developer-only
**Status:** Accepted for M11.61
**Date:** 2026-09-10

**Context:** The natural callee crossed by M11.60 G0 preserves A5 exactly and
has a complete observed safe-RAM/G0-relative/stack effect set. Its static CFG
also contains an unresolved indexed JSR at `0x062CEC` and multiple latent RTS
exits.

**Decision:** Record only the natural preservation/effect contract with a
developer-only observer and regression test. Classify the static boundary as
`INDIRECT_CFG`; retain the M11.60 parent-lifetime transaction gate and typed
data block. Do not recurse into a subsystem or add a production abstraction.

**Consequences:** One G0 callee dependency is reduced to an exact natural
fact. The latent indirect path and other G0 callees remain explicit blockers.

**Evidence:** `docs/reports/CALLEE_062AE0_CONTRACT_M11_61.md`.

# ADR-0041 — Keep the 0x060182 A5 lifetime parent-owned
**Status:** Accepted for M11.60
**Date:** 2026-09-10

**Context:** The natural generation written by `0x060182 LEA FF001A,A5` has a
bounded CFG and exact +0/+4 consumers, while the +7 arm is dead. Its endpoint
is the parent's `0x06027E MOVEM` restore. Calls crossed while G0 is live do not
yet have complete whole-callee effect proofs, and M11.59 retains broader raw
aliases and external writers.

**Decision:** Record the generation with a developer-only hybrid observer and
decoder regression test. Classify it as `A5_LIFETIME_MERGES_WITH_PARENT` and
keep the transaction and typed-data gates fail-closed. Do not add a type,
subsystem, production helper, or native routine.

**Consequences:** The exact consumer/lifetime evidence is reusable for one
future callee-preservation closure. Parent frame, GPGX, timing, ROM-PC and
raw-storage ownership remain outside `oasis_core`.

**Evidence:** `docs/reports/A5_CONSUMER_LIFETIME_M11_60.md`.

# ADR-0040 — Preserve raw ownership boundary after M11.59 census
**Status:** Accepted for M11.59
**Date:** 2026-09-10

**Context:** M11.58 left the typed-data gate open because fixed bytes had
external writers and the A5-derived range had unresolved alias/lifetime
evidence. M11.59 found three A5 materializers; one path saves and reloads A5
and post-increments it through a broad range, while other routines consume
multiple derived offsets. The sibling 0x60BCC writers also retain a hardware
prefix.

**Decision:** Keep `ParentSuffix + RamFlag` raw and parent-parameterized. Do
not add a typed structure, shared-memory owner, subsystem wrapper or
0x60BCC promotion. Permit only a standalone decoder provenance regression
test; keep ROM/GPGX, hardware ordering, lifetime and continuation evidence in
developer-only tooling and the parent adapter.

**Consequences:** The aliasing boundary is proven negative for typed
replacement while the behavior-cluster contract remains valid. M11.60 must
close one bounded A5 consumer/lifetime contract before revisiting typed data.

**Evidence:** `docs/reports/RAW_DATA_OWNERSHIP_M11_59.md`.

# ADR-0039 — Keep the first behavior cluster raw and parent-parameterized
**Status:** Accepted for M11.58
**Date:** 2026-09-10

**Context:** M11.57 proved a parent-owned suffix helper that composes the
portable RamFlagRoutine. M11.58 audited its exact raw footprint and bounded
ROM/runtime provenance. The fixed `FF0010..FF0014` bytes have known external
writers, the flag addresses are used by other bounded code, and the
`FF001A + 5..7` derived range has unresolved alias and lifetime boundaries.

**Decision:** Treat the existing ParentSuffix + RamFlag composition as the
portable behavior-cluster contract, retaining raw address parameters and each
component's independent continuation tokens. Do not add a typed shared-memory
structure, opaque replacement wrapper, subsystem owner or new routine. Keep
parent frame/SR/hardware/epilogue/RTS ownership and all ROM/GPGX provenance in
the parent or developer-only hybrid adapter.

**Consequences:** The cluster can be tested and shadow/native-proven without
inventing gameplay meaning or hiding unresolved ownership. M11.59 must close
one dominant alias/lifetime and external-writer blocker before any typed data
replacement is considered. `PORTABLE_SUBSYSTEM_BOUNDARY_PROVEN` remains false.

**Evidence:** `docs/reports/PORTABLE_BEHAVIOR_CLUSTER_M11_58.md`.

# ADR-0031 — Portable mechanical primitive layer in `oasis_core`
**Status:** Accepted for M11.50
**Date:** 2026-09-09

**Context:** M11.49 proved one generic resumable executor for four exact
copy/clear plus DBF loops, but its implementation still lived in the
developer-only hybrid library. The semantics were reusable; the registry,
canonical bytes, GPGX bridge, shadow comparator and evidence were not.

**Decision:** Move only the generic mechanical contract and executor into
`oasis_core`. Use opaque instruction tokens, a portable machine interface and
explicit continuation state. Keep ROM PCs, candidate names, canonical opcodes
and displacement, GPGX/BasicBlock timing/prefetch adapters, shadow snapshots,
metrics and reporting in `tools/hybrid`. Add a standalone core test and a
repository-visible dependency-boundary check. Do not add new primitive forms,
gameplay semantics or hardware modeling.

**Consequences:** There is one implementation of copy/clear/DBF semantics and
it is independently linkable without GPGX/libretro. The hybrid adapter remains
responsible for hardware/bus mapping and canonical provenance. The extracted
layer preserves the M11.49 shadow/native identity; future native routines may
compose these operations only after separate evidence closes their contracts.

**Affected files/milestones:** `src/core/mechanical_primitive.*`, hybrid
adapter/registry, standalone core and boundary tests, CMake and M11.50 report.

# ADR-0030 — Metadata-driven mechanical primitive family
**Status:** Accepted for M11.49 developer-only hybrid tooling
**Date:** 2026-09-09

**Context:** M11.48 proved one resumable `CLR.B`/`DBF` loop. M11.47 also
identified two byte copy loops and one word clear loop, but the family
abstraction, exact source/destination and width contracts, and coexistence
behavior had not been proven.

**Decision:** Generalize the architecture-neutral primitive executor around a
metadata-only `MechanicalLoopContract`. Promote only the exact two
`MOVE.B (A2)+,(A1)+` plus DBF forms and `CLR.W (A0)+` plus DBF form that pass
deterministic vectors, generated-oracle shadow, existing bus/timing/refresh
contracts and unchanged native identity. Keep ROM PCs in registry metadata,
keep generated code as oracle/fallback, use explicit ordered byte copy and
width-specific clear writes, and fail closed on unsupported forms. Do not
broaden hardware emulation or add a production dependency.

**Consequences:** Four mechanical loops share one reusable implementation and
preserve exact 32-bit address, DBF, CCR/X, bus ordering and interruption
behavior. Native execution represents 42,384 guest instructions while the
generated-plus-mechanical translated-equivalent count remains 6,241,765;
checkpoint/video, yields, resumptions and CPU equivalence remain unchanged.
The promoted family has no hardware-visible access; the existing hardware
boundary remains unchanged. Extraction is a future boundary, not part of M11.49.

**Affected files/milestones:** `mechanical_primitive.*`, runner report,
synthetic primitive test, M11.49 governance and evidence report.

# ADR-0029 — Resumable native mechanical primitive layer
**Status:** Accepted for M11.48 developer-only hybrid tooling
**Date:** 2026-09-09

**Context:** M11.47 proved repeated safe-memory copy/clear structures but did
not define a higher-level replacement contract. The first replacement must
preserve instruction-boundary timing, refresh, event/interrupt behavior and
continuation state, while remaining independent of GPGX internals.

**Decision:** Add a small architecture-neutral `MechanicalMachine` interface
and resumable primitive registry in `src/tools/hybrid/mechanical_primitive.*`.
Promote only the exact `CLR.B (A5)+` plus `DBF D0` loop at `0x061266`/`0x061268`.
Dispatch metadata identifies the proven ROM loop; the implementation executes
one guest body/DBF pair at a time and fails closed on canonical-byte or bridge
contract mismatch. Keep generated blocks and generic basic-block glue separate;
retain the generated path as the shadow oracle and fallback for all other PCs.
Do not add gameplay meaning, hardware emulation or a production dependency.

**Consequences:** The native path represents 3,780 guest instructions through
1,890 resumable clear iterations and preserves 86 synthetic/observed
mid-operation yields in shadow evidence. The unchanged 600-frame proof retains
checkpoint/video/CPU identity, 150,135 total yields, 288 resumptions and zero
fallback/hardware accesses. The three other discovered loops remain unpromoted.

**Affected files/milestones:** M11.48 mechanical primitive API, runner/CMake,
synthetic primitive test, governance and evidence report.

# ADR-0028 — Bounded safe-memory semantic tranche and mechanical primitives
**Status:** Accepted for M11.47 developer-only hybrid tooling
**Date:** 2026-09-09

**Context:** M11.46 resolved runtime memory classes for a bounded set of
interpreter PCs. Seven high-payoff rows were observed only in safe main RAM,
but their exact instruction semantics and bus effects still required proof.
Repeated copy/clear loops also appeared mechanically structured, while their
native higher-level replacement contract was not yet needed.

**Decision:** Independently verify only the exact seven forms required by the
M11.47 safe-memory tranche, emit them through the decoder-owned mechanical
generator and shared runtime helpers, and certify each through the existing
instruction-boundary GPGX shadow and unchanged 600-frame native gates. Keep
hardware-reachable, mixed, unresolved, indirect-CFG and decoder rows
fail-closed. Record copy/clear structures as mechanical future replacement
candidates only; do not replace loops or assign gameplay semantics in M11.47.

**Consequences:** Seven generated ranges remove exactly 42,047 interpreter
executions, raising translated execution to 96.1933% while preserving the
authoritative checkpoint/video identity, bus/timing/event contract and zero
hardware-visible native accesses. The remaining ledger closes at 247,008
executions. Generated bodies, registry metadata, shared helpers and handwritten
boundary glue remain separate.

**Affected files/milestones:** M11.47 hybrid generator/runtime, generated
block registry, semantic regression test and M11.47 reports.

# ADR-0027 — Observe runtime addresses without promotion
**Status:** Accepted for M11.46 developer-only hybrid tooling
**Date:** 2026-09-09

**Context:** M11.45 left 149,678 interpreter executions in 635
`UNKNOWN_WITH_EVIDENCE` register-address PCs. Static decoding could not prove
their runtime memory classes, while the existing GPGX path already exposes
instruction and top-level data-bus boundaries.

**Decision:** Add a separate observer mode around the existing GPGX hook and
M11.45 block registry. Record PC counts, data-bus address/width/direction/order
and A-register transitions, and classify only observed addresses using the
existing Genesis address contract. Keep mapper-dependent cartridge SRAM
unproven when address alone is insufficient. Do not promote semantics, broaden
hardware emulation, add address-specific execution bodies or connect the
observer to production runtime.

**Consequences:** Three independent processes matched the M11.45 identity and
metrics. Runtime classes were resolved for 147,847 executions; 1,831
address-computation-only executions remain exact-evidence unresolved. The result
is `BOUNDED_RUNTIME_ADDRESS_PROVENANCE_PROVEN`; M11.45 history and all prior
negative gates remain intact.

# Architecture Decision Log

Use this file for decisions that can redirect architecture, dependencies, scope, or reverse-engineering strategy.

## ADR-0026 — Bounded semantic closure with independent shadow vetoes
**Status:** Accepted for M11.45 developer-only hybrid tooling
**Date:** 2026-09-09

**Context:** M11.44 left 560,968 interpreter executions. The highest-payoff
decoder-backed semantic/proven candidates could exceed the 236,530 execution
threshold needed for a 95% result, but exact GPGX timing and flags remained
unproven for new forms.

**Decision:** Select only decoder-owned rows with proven static memory class and
exact mechanical support, excluding register-based/other memory, hardware,
indirect CFG, decoder gaps and unknown runtime addresses. Verify exact helpers
with deterministic vectors, generate all candidate bodies mechanically, and let
the existing per-boundary shadow gate veto candidates independently. Do not add
candidate-specific timing constants or hardware behavior.

**Consequences:** 551 candidates passed 6,199,718 shadow comparisons and native
proof, removing 271,913 executions. LSR.W, ROR.W and CMPI.B were retained as
fail-closed shadow rejections. The resulting 95.5453% translated share proves
`REMAINING_ATTRIBUTION_AND_DYNAMIC_COVERAGE_95_PROVEN`; the final ledger still
accounts for every 289,055 interpreter execution.

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
# ADR-0032 — Keep first portable native routine shadow-only pending identity proof
**Status:** Accepted for M11.51
**Date:** 2026-09-09

**Context:** M11.29 supplied complete structural evidence for the bounded
`0x2D66..0x2D84` table-copy leaf, and M11.50 supplied the portable mechanical
ownership boundary. A structured `TableCopyRoutine` can now be extracted, but
the current pinned GPGX authoritative replacement does not preserve the frozen
M11.50 checkpoint aggregate even though video and isolated accounting match.

**Decision:** Keep the portable routine and zero-divergence shadow adapter as
developer-visible evidence, but do not promote it as authoritative execution.
The hybrid generated/interpreter path remains the oracle/fallback. Any future
promotion must independently close CPU, RAM, VDP, sound, interrupt,
timing/refresh and continuation identity against the frozen 600-frame run.
No timing constant, hardware behavior or gameplay meaning is invented to force
the gate.

**Consequences:** `oasis_core` demonstrates the next abstraction boundary
without weakening behavioral parity. The milestone result is
`PORTABLE_NATIVE_ROUTINE_SHADOW_PROVEN_REPLACEMENT_BLOCKED`; the next task must
explain the exact identity mismatch before another authoritative replacement.

**Affected files/milestones:** `src/core/table_copy_routine.*`, the 2D66
developer adapter and tests, M11.51 governance and report.

## ADR-0033 — Per-instruction hybrid continuation is required for native routine promotion
**Status:** Accepted for M11.52
**Date:** 2026-09-09

**Context:** M11.51's portable `TableCopyRoutine` shadow matched, but its
authoritative native adapter changed four canonical checkpoint bytes after
frame 120. The adapter's routine-level lump-sum cycle update produced the same
exit cycle count as the interpreter while missing intermediate GPGX refresh
sampling. The shadow comparison stopped before the scheduler-visible state
where the mismatch appeared.

**Decision:** Keep portable routine semantics in `oasis_core`. Extend the
developer-only hybrid bridge with instruction fetch and per-instruction
begin/finish callbacks, and require the adapter to reconstruct the exact
opcode/extension stream, DBF continuation decision, MOVEM dynamic timing and
RTS prefetch state. Do not use checkpoint canonicalization or aggregate-hash
exceptions to promote a native routine. Promotion requires paired per-frame
canonical checkpoint identity, video identity, continuation timing and
execution accounting.

**Alternatives considered:** Keep the lump-sum adapter; rejected because it
misses refresh/scheduler semantics. Canonicalize the four bytes; rejected
because RAM, sound and Z80 interrupt fields are semantic. Add a candidate-local
checkpoint patch or a full CPU emulator; rejected by project scope and evidence
rules.

**Consequences:** The 2D66 routine is promoted with a generic hybrid
continuation contract, while the generated/interpreter path remains the
oracle/fallback for other routines. Future routine adapters must prove their
own post-return and serialized-state boundary.

**Affected files/milestones:** `src/tools/hybrid/replacement.hpp`,
`src/tools/hybrid/gpgx_bridge.c`, `src/tools/hybrid/runner.cpp`, the 2D66
adapter/test and M11.52 evidence/governance.
# ADR-0034 — Second portable native routine contract
Status: Accepted for M11.53
Date: 2026-09-09

Context: M11.52 proved that native replacement requires per-instruction GPGX
continuation. M11.53 had one complete natural candidate, 0x604BC, whose old
adapter used a lump-sum timing handoff.

Decision: Extract only the structured BSET/Scc/LEA/RTS semantics of
0x604BC..0x604E6 into oasis_core with opaque tokens, portable registers and
memory, and explicit resumable boundaries. Keep ROM metadata, canonical
opcode/extension validation, GPGX fetch/begin/finish, prefetch/refresh,
boundary sampling, block-hook continuation return and RTS return state in
tools/hybrid. Promote only after
paired dual-routine checkpoint/video identity and separate accounting.

Consequences: TableCopyRoutine and the second routine coexist through the
existing registry without shared candidate state. No gameplay meaning,
hardware behavior, subsystem abstraction or second timing model is added.
The old lump-sum adapter path is removed for 0x604BC; generated execution
remains the oracle/fallback.

Affected files/milestones: src/core/ram_flag_routine.*, the 0x604BC adapter
and regression test, replacement accounting, M11.53 report.

# ADR-0035 — Stop at the first proven native routine cluster
Status: Accepted for M11.54
Date: 2026-09-09

Context: M11.52 and M11.53 authoritatively proved two portable native routine
contracts, TableCopyRoutine at 0x2D66..0x2D84 and RamFlagRoutine at
0x604BC..0x604E6. M11.54 reproduced the dual proof twice and audited exact
caller/callee and raw-memory evidence.

Decision: Classify the RamFlag direct callers at 0x604F6 and 0x60BCC, together
with the raw 0x00FF0010..0x00FF0016 window, as a CALL_GRAPH_CLUSTER and
MEMORY_STRUCTURE_CLUSTER. Do not call it a gameplay subsystem or move callers
into oasis_core until caller CFG, data ownership and hardware ordering close.
Keep TableCopy and RamFlag as separate ownership islands. Do not add a third
routine or typed data in M11.54.

Consequences: The architectural result is PORTABLE_ROUTINE_CLUSTER_PROVEN,
not FIRST_PORTABLE_SUBSYSTEM_BOUNDARY_IDENTIFIED. The next proposed task is a
bounded RamFlag caller/data closure audit. The existing dependency direction
is preserved: oasis_core owns portable semantics/tokens, while tools/hybrid
owns ROM provenance, GPGX continuation, oracle/accounting and hardware.

Affected files/milestones: M11.54 governance documents and
reports/NATIVE_ROUTINE_CLUSTER_M11_54.md; no production source change.

# ADR-0036 — Keep RamFlag caller regions developer-only after M11.55 closure
Status: Accepted for M11.55
Date: 2026-09-09

Context: M11.54 had static-only caller edges for `0x0604F6` and `0x060BCC`.
M11.55 added a natural-entry observer and reproduced the unchanged dual-native
proof twice. The observer deterministically attributes one `0x0604F6` entry
and three `0x060BCC` entries, while exact bounded slices expose continuation,
sibling-call, A5-relative memory and hardware-boundary gaps.

Decision: Accept `RAMFLAG_CALLER_CONTRACTS_PROVEN` only as a bounded caller-
region result. Keep attribution, ROM byte provenance, GPGX timing/hooks,
address provenance and the `0x00A11100` hardware interaction in
`tools/hybrid`. Treat the `0x0604F0` path and `0x060BC4` path as partial
contracts, not portable routines. Do not create a typed structure for
`FF0010..FF0016`; offset `FF0015`, A5-relative effects, lifetime and aliasing
remain unknown. Do not implement a caller or subsystem unless a future task
closes its complete continuation and memory/hardware contract.

Consequences: Dynamic caller provenance is now reproducible and fail-closed for
unknown caller classes without widening `oasis_core`. The exact
`0x060BC4` hardware-prefix/RamFlag-suffix ordering is recorded, but the wider
caller remains boundary-blocked. `PORTABLE_ROUTINE_CLUSTERS` remains 1 and
`PORTABLE_SUBSYSTEM_BOUNDARIES` remains 0. M11.56 must choose one falsifiable
continuation or sibling/data closure and is not executed by this decision.

Affected files/milestones: caller attribution observer/test, extracted hybrid
library wrapper, M11.55 governance documents and report; no production source
change.

# ADR-0037 — Keep the parent-owned 0x604F0 suffix internal
Status: Accepted for M11.56
Date: 2026-09-09

Context: Exact natural evidence identifies the previously unknown 0x611D6
destination as the shared saved-register/SR epilogue of the 0x60004/0x6042A
parent. 0x604F0 has no ordinary standalone return at A7; it inherits 58
bytes of parent state. The old decoder budget also includes another arm.

Decision: Record THIRD_ROUTINE_NOT_A_STANDALONE_ROUTINE with dominant blocker
ENCLOSING_ROUTINE_BOUNDARY. Do not manufacture a third routine by attaching
the shared restore/RTS tail or absorbing the parent's hardware prefix.
Retain full-SR/event continuation as partial. Keep the bounded read-only
observer and local evidence validator in tools/hybrid; do not change core,
RamFlag semantics, typed data or subsystem ownership.

Consequences: Structural continuation ownership is now proven for the natural
path, while a portable independent entry contract is still absent. Inventory
remains two authoritative routines and one cluster, with no subsystem boundary.
A future M11.57 may test an explicit parent/suffix handoff contract, preserving
the distinction between a portable internal helper and a complete routine.

Affected files/milestones: caller continuation observer/validator/test,
M11.56 report and governance. No production implementation.
# ADR-0038 — Parent-owned portable suffix helper
**Status:** Accepted for M11.57
**Date:** 2026-09-10

**Context:** M11.56 proved that `0x604F0` is an internal fallthrough carrying
the parent frame into a shared epilogue, so it cannot be promoted as a third
standalone routine. Its hardware-free suffix nevertheless has a closed
entry/exit contract and composes with the already-proven RamFlag routine.

**Decision:** Add one small architecture-neutral `ParentSuffixMachine` core
executor with opaque phase and parent-continuation tokens. It owns only the
five safe-RAM SF writes and structural RamFlag call. Keep ROM addresses,
canonical bytes, GPGX fetch/timing, nested BSR representation and the mapping
to `0x611D6` in the developer-only hybrid adapter. Keep parent frame/SR,
hardware, shared epilogue and RTS ownership with the parent.

**Consequences:** The exact suffix can be tested and resumed independently
without inventing gameplay meaning or a subsystem boundary. The native helper
is counted as `PORTABLE_INTERNAL_HELPER`; it is never called a third routine.
Full-SR and hardware behavior remain outside the helper and require a separate
milestone if they are ever considered.

**Affected files/milestone:** `src/core/parent_suffix.*`,
`src/tools/hybrid/candidate_parent_suffix.*`, `tests/parent_suffix_test.cpp`,
M11.57 report and hybrid accounting.

# ADR-0044 — Scoped temporal provenance sidecar for M12

**Status:** Accepted design contract; implementation stages not completed
**Date:** 2026-09-12

**Context:** Controlled BizHawk capture is working, but repeated manual
RAM/register/ROM dependency reconstruction duplicates work. Existing Carver
IntervalDB owns the ROM partition and non-owning evidence reports; its graph
does not encode instruction instances, memory versions or capture completeness.
Callback PC and lagged input observations already demonstrate false-proof risks.

**Decision:** Design a developer-only Python/SQLite sidecar that separates
immutable observations, temporal byte/register versions and scoped relations.
Use demand-driven slicing, explicit data/address/control dependencies,
coverage certificates and proof-obligation scheduling. Reuse the controlled
harness and M12 static analyzers. Export non-owning ROM evidence to Carver;
existing exact promoters alone may update ownership under their current gates.
V1 must recover the known canary with minimal RAM/register semantics, not
merely import an expected graph or log A372 followed by UNKNOWN.

**Consequences:** No monolithic rewrite, new production dependency, graph
server or general symbolic/emulator implementation. Repeats establish
reproducibility rather than independent semantic proof. Savestate roots are
scope boundaries, not invented reset provenance. Hardware visibility and
static completeness have separate gates. V0 capability validation precedes
V1 implementation; no milestone completion or ownership change is implied.

**Evidence/design:** `docs/reports/THOR_EVIDENCE_ENGINE_ARCHITECTURE.md` and
`docs/reports/THOR_EVIDENCE_ENGINE_STAGES.md`; verified baseline manifest
contains 1,475,368 SOURCE_OWNED bytes. ADR-0043 remains authoritative for the
M12 ASM → M13 rebuilt parity → M14 systematic C++ sequence.

# ADR-0045 — V0.1 raw completion and launch identity gate

**Status:** Accepted bounded repair; V1 remains gated
**Date:** 2026-09-12

**Context:** The V0 gate review reproduced sealing of a raw prefix ending at a
valid epoch, incorrect collector attribution, and capability conclusions that
survived missing evidence. A sealed transport must not be mistaken for a
complete execution or a causal proof.

**Decision:** Require a strict `thor.evidence.raw.v0.1` header/footer envelope
and a completed local launch receipt before normalization. The receipt binds
the actual Lua collector, normalizer, harness, emulator/core/config, watch-plan
order, mode/reverse flag, ROM/state and scenario. Report capabilities are
derived from runtime witnesses or hashed BizHawk API source receipts; absent or
contradictory evidence yields UNKNOWN/ERROR. Historical V0 captures remain
valid only as historical evidence when their execution identity cannot be
reconstructed.

**Consequences:** V0.1 can establish a truthful capture boundary and a local
FF13CC precondition matrix without implementing causal provenance. Generic bus
width, overlap, IRQ, same-value completeness and input causality remain explicit
frontiers. No ownership mutation, ROM discovery or production dependency is
introduced.

**Evidence:** `docs/reports/THOR_EVIDENCE_ENGINE_V0_1.md`, the V0.1 receipt,
raw-envelope and report tests, and the three repeated bounded captures under
ignored `build/thor-evidence/v0`.

# ADR-0046 — Local FF13CC V1-gate remains fail-closed

**Status:** Accepted bounded gate; V1 remains blocked
**Date:** 2026-09-13

**Context:** The accepted V0.1 capture proves ordered exact-hook observations
around one `A372` execution, but does not provide dense instruction coverage or
an interruption boundary. Treating an exact hook miss as absence of an
overlapping writer would create the causal false proof the V1 gate is intended
to prevent.

**Decision:** Add only a local gate certificate and adversarial validator. The
checked static `MOVE.L D2,(A5)+` form, selected ordered pairing and four-byte
temporal representation may be certified. Writer completeness requires an
explicit dense instruction list with `NO_MEMORY_WRITE`, `WRITE_DISJOINT`, or
`WRITE_TARGET_OVERLAP`; any `UNKNOWN_MEMORY_EFFECT`, omitted target coverage,
or unresolved interruption blocks the gate. One-byte overlap and same-value
writes remain writes. No provenance graph, general last-writer engine,
register propagation or ownership action is introduced.

**Consequences:** The local result is `PARTIAL / V1 BLOCKED`: pairing is
closed, while dense writer coverage and interruption remain explicit frontier
items. A future V1 authorization must supply those evidence classes before any
causal claim.

**Evidence:** `docs/reports/THOR_EVIDENCE_ENGINE_V1_GATE.md`,
`src/tools/thor_evidence/v1_gate.py`, and the persisted negative fixtures.

# ADR-0043 — Complete ASM reconstruction before systematic C++ migration
**Status:** Accepted for M12.0
**Date:** 2026-09-10

**Context:** The existing exact reassembly proof is a local-ROM-backed split
with 13,550 exact 68000 ASM bytes and 3,132,178 blob bytes. The prior roadmap
would have started Inventory/UI/Save as a portable C++ milestone before the
executable ROM was represented as complete assembler/source.

**Decision:** Rebase the project sequence to ROM -> complete reassemblable ASM
-> rebuilt-ROM runtime parity -> systematic ASM-to-portable-C++ migration.
Define ASM_CODE_COMPLETE, ASM_ROM_MAP_COMPLETE, ASM_REASSEMBLY_BYTE_EXACT and
ASM_REBUILT_ROM_BOOT_PROVEN as separate gates. Keep current native C++ proofs
preserved and set CPP_MIGRATION=PAUSED_PENDING_ASM_COMPLETION. Do not begin
M12.1 implementation in this decision.

**Consequences:** The former Inventory/UI/Save proposal is no longer the
immediate M12 milestone. Executable code cannot be hidden in local blobs;
unknown data and copyrighted payloads may remain local only under deterministic
tooling and explicit classification. M13 owns rebuilt-ROM boot/runtime parity
and M14 owns systematic C++ migration.

**Evidence:** docs/reports/ASM_COMPLETION_CENSUS_M12_0.md.
# ADR-0047 — Close only local dense FF13CC coverage before V1

**Status:** Accepted for V1-GATE-COVERAGE
**Date:** 2026-09-13

**Context:** The prior gate had a proven A372 pairing but exact-address hooks
could not establish that every instruction in the local interval was observed.
It also lacked a local interruption boundary. BizHawk 2.11.1 exposes
`event.on_bus_exec_any`, so a bounded dense capture can answer those two
questions without implementing provenance.

**Decision:** Use one bounded dense capture (with one corrected retry only for
capture serialization) from the proven `A370` pre-boundary through the `A374`
post-fetch boundary. Decode every captured execution instance with the existing
checked ROM decoder, compute concrete memory-write ranges from the pre-execution
register snapshot, retain all target-overlapping and same-value writes, and
fail closed on unknown effects or discontinuity. Treat `A374` as a boundary
witness, not as an unobserved post-boundary instruction.

**Consequences:** The selected interval has complete local execution coverage,
one concrete `FF13CC..FF13CF` writer and a continuous `A370 -> A372 -> A374`
control-flow boundary. This remains a local capability certificate. It does
not establish global IRQ behavior, causal input provenance, a last-writer
engine, a provenance graph, or V1 readiness by itself.

**Evidence:** `docs/reports/THOR_EVIDENCE_ENGINE_V1_GATE_COVERAGE.md`.
# ADR-0048 — First V1 provenance slice is engine-derived and canary-bounded
**Status:** Accepted for V1 FF13CC canary
**Date:** 2026-09-13

**Context:** V0 and the local V1 gate established sealed transport, checked
static forms, dense execution coverage and explicit capability frontiers. The
next authorized step is one causal query for the existing FF13CC canary.

**Decision:** Build provenance only from the checked ROM-bound static producer
and sealed dynamic witnesses. Represent temporal value versions, register
bit-slices, execution instances and dependencies with explicit VALUE, ADDRESS
and CONTROL roles. Persist the derived certificate in a separate SQLite
sidecar table. Reject unknown transforms, address-only edges, PC heuristics,
ROM low-byte substitution and cross-epoch merges. Keep access width, overlap,
same-value writer completeness, IRQ/exception and input-read capabilities
explicitly UNKNOWN.

**Consequences:** The engine can explain the first `FF13CC` value-version as
ROM high24 plus incremented `D5.low8`, and can be re-imported idempotently. The
certificate is not a general last-writer proof and does not authorize V2 or
source ownership.

**Evidence:** `docs/reports/THOR_EVIDENCE_ENGINE_V1_FF13CC_CANARY.md`.
# ADR-0049 — Reusable V2 RAM byte versions remain coverage-gated
**Status:** Accepted for V2 RAM provenance
**Date:** 2026-09-13

**Context:** V1 proved one FF13CC output through a bounded engine-derived
chain, but its RAM output versions were still local to the canary builder.

**Decision:** Generalize only RAM byte temporal versions and write operations.
Represent byte/word/long writes in big-endian physical order, preserve
previous versions and explicit pre-capture roots, isolate restore epochs, and
require a trace-bound complete coverage certificate for `PROVEN` last-writer
results. Return explicit frontiers for gaps, unsupported transforms and
conflicts. Extend the existing SQLite sidecar; do not create another store.
Reuse the primitive for the V1 canary and leave the held-out FF188A query at
`INCOMPLETE_CAPTURE` when existing coverage is insufficient.

**Consequences:** V2 can answer bounded RAM byte last-writer queries without
numeric-value or address-only identity collapse. Register/control provenance,
global coverage, IRQ/input causality, ROM RE and V3 remain separate gates.

**Evidence:** `docs/reports/THOR_EVIDENCE_ENGINE_V2_RAM_PROVENANCE.md`.

# ADR-0050 — V2.1 verified coverage and V1 RAM identity repair
**Status:** Accepted for V2.1 soundness repair
**Date:** 2026-09-13

**Context:** The independent V2 gate audit reproduced false promotion from
metadata-only coverage, future-version temporal queries, partial-write ghosts,
SQLite identity splicing and a V1 adapter path that discarded overlaps.

**Decision:** Keep the V2 byte engine and SQLite sidecar, but separate
unverified coverage claims from verified certificates. A certificate must be
content-bound to a contiguous capture basis, receipt, epoch, byte scope,
decoder/rule and execution instances. Keep immutable initial roots separate
from current state, validate complete write ranges before mutation, reject
inconsistent imported identities atomically, and expose the V1 target through
the concrete V2 byte versions and operation.

**Consequences:** The bounded FF13CC canary remains the only promoted query.
The historical FF188A evidence remains a negative frontier. Global coverage,
IRQ/input/DMA and V3 provenance remain outside this repair.

**Evidence:** `docs/reports/THOR_EVIDENCE_ENGINE_V2_1_SOUNDNESS_REPAIR.md`.

# ADR-0051 — V3 register/control provenance build skeleton
**Status:** Accepted for V3 BUILD
**Date:** 2026-09-13

**Context:** V2.1 re-audit left known soundness defects open, while the next
authorized milestone requires reusable register, execution and local control
structures before stabilization.

**Decision:** Add a developer-only V3 skeleton with temporal D/A register
slices, checked local M68K rules, explicit execution instances, bounded
call/return and local control facts, typed dependency roles, RAM interop,
canonical graph/export and explain APIs. Extend the existing SQLite sidecar;
do not create a second database. New generalized claims default to
OBSERVED/PROVISIONAL/UNKNOWN/CONFLICT and no V2 path is widened.

**Consequences:** V3 can represent the FF13CC and one existing execution
fixture, including partial register writes and address/control roles. The
engine is not a sound causal proof system. V2.1 defects remain in the known
defect ledger and V4 is prohibited until the V3 build gate is published.

**Evidence:** `docs/reports/THOR_EVIDENCE_ENGINE_V3_REGISTER_CONTROL_BUILD.md`.
