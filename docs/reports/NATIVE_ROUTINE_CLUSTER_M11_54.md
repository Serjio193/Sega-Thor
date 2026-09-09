# M11.54 — Native routine cluster and first subsystem boundary discovery

## Result

STATUS: PORTABLE_ROUTINE_CLUSTER_PROVEN

M11.54 proves one bounded call-graph/raw-memory cluster around the
authoritative RamFlagRoutine and its two direct call-sites. It does not prove
a gameplay subsystem. The authoritative TableCopyRoutine and RamFlagRoutine
remain separate: no edge or shared data structure connecting the two was
proven. No third routine, typed structure, hardware behavior or subsystem
implementation is warranted.

The next architectural target is B — ROUTINE_CLUSTER: close the bounded
RamFlag caller/data boundary before introducing a higher-level native owner.
M11.55 is specified below but not executed.

## Scope and identity

Baseline: 6c81803dbd230ff54862d6ae8e8a04a7c79f727d.

| Item | Value |
| --- | --- |
| ROM | C:/Github/gpgx-test-roms/Beyond Oasis (USA).md |
| ROM SHA-256 | eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263 |
| pinned GPGX DLL SHA-256 | 140c00fc7475cf22ca22415d65dfc7ffc66523de128454f9994e7ab77d9826fd |
| frozen checkpoint aggregate | 251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d |
| frozen video sequence | 5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58 |

ROM-backed outputs are local and untracked. ROMs, assets, emulator binaries
and game.srm remain outside Git.

## Phase 1 — unchanged M11.53 baseline

Two unchanged 600-frame cold-reset neutral native runs passed:

| Measure | Run A | Run B |
| --- | ---: | ---: |
| checkpoint aggregate | 251fab…f32d38d | 251fab…f32d38d |
| video sequence | 5e74ec…437a58 | 5e74ec…437a58 |
| total guest instructions | 6,488,773 | 6,488,773 |
| interpreter remainder | 6,488,699 | 6,488,699 |
| TableCopy native instructions/calls | 34 / 1 | 34 / 1 |
| RamFlag native instructions/calls | 40 / 4 | 40 / 4 |
| fallback / divergence | 0 / 0 | 0 / 0 |
| RamFlag yield / resumption | 1 / 1 | 1 / 1 |
| continuation token | 0x604DA | 0x604DA |

Accounting is 6,488,773 = 6,488,699 INTERPRETER + 34 TableCopy +
40 RamFlag. The runner call log records target entries, not caller PCs; it is
not used as caller provenance.

## Phase 2 — caller/callee provenance

Exact ROM decoding proves the direct edges. The native runner proves target
entry totals but does not pair each entry with the preceding caller PC.
Therefore the direct edges are STATIC_PROVEN; target execution is
DYNAMIC_PROVEN; no edge is promoted to BOTH.

| Target and caller | Exact call | Return PC | Natural count | Caller evidence |
| --- | --- | --- | ---: | --- |
| TableCopy 0x002D66..0x002D84, bounded caller 0x002D58 | 61 00 00 0C, direct BSR.W 0x002D66 | 0x002D5C | target total 1; per-caller unknown | caller writes raw bit at 0x00FF164D; no hardware in bounded slice; no sibling callee |
| RamFlag 0x0604BC..0x0604E6, bounded caller 0x0604F6 | 61 00 FF C4, direct BSR.W 0x0604BC | 0x0604FA | target total 4; per-caller unknown | surrounding writes 0x00FF0010..0x00FF0014; no hardware in bounded slice |
| RamFlag, bounded caller block 0x060BC4..0x060CDA, site 0x060BCC | 61 00 F8 EE, direct BSR.W 0x0604BC | 0x060BD0 | target total 4; per-caller unknown | preceding 0x060BC4 writes hardware 0x00A11100; continuation writes 0x00FF0010..14 and 0(A5) |

Both native callees have no direct callees, indirect flow or internal loop.
The 0x060BCC context has sibling direct calls at 0x060C18, 0x060C2E and
0x060C44 to 0x060F8C. Their semantics are not imported into the cluster.

Evidence sources: canonical exact-slice runs
build-m1154-2d58.json/.txt, build-m1154-604ec.json/.txt and
build-m1154-60b50.txt; the existing callee-effect and caller-stack USA
oracles; M11.29/M11.53 ledger entries. No inferred call relationship is used.

## Phase 3 — shared-memory structures

| Range / address | Access | Routines | Classification and limits |
| --- | --- | --- | --- |
| 0x00FF134C + sign_extend_word(offset) | sequential word writes; source words through A6 | TableCopy | mechanical output BUFFER; base/width/order proven, dynamic size/lifetime/aliasing not |
| 0x00FF0628, 0x00FF06F2 | byte read/modify/write, BSET bit 4 | RamFlag | individual FLAG effects only; meaning/lifetime unknown |
| A5+5..A5+7 | three ordered byte writes | RamFlag | RAW_RANGE; runtime base, fields and aliasing unknown |
| 0x00FF0010..0x00FF0016 | byte writes at offsets 0,1,2,3,4,6 | RamFlag and callers | shared RAW_RANGE; offset 5 is not proven touched; no field/lifetime/aliasing/meaning proof |
| 0x00FF164D | one raw byte bit operation | TableCopy caller | separate raw access; no sharing with RamFlag proven |
| 0x00A11100 | word hardware write | 0x060BCC context | adapter/hardware boundary, not portable data |

The only shared structure evidence is the raw 0x00FF0010..0x00FF0016 window
between RamFlag and its callers. No portable typed data is introduced.

## Phase 4 — bounded neighborhood inventory

| Member | Classification | Exact blocker |
| --- | --- | --- |
| 0x002D66..0x002D84 | AUTHORITATIVE_NATIVE, ROUTINE_CONTRACT_COMPLETE | none for closed ten-instruction leaf |
| 0x002D58 caller | ROUTINE_CONTRACT_PARTIAL | enclosing owner and raw-bit meaning not proven |
| 0x0604BC..0x0604E6 | AUTHORITATIVE_NATIVE, ROUTINE_CONTRACT_COMPLETE | none for closed ten-instruction leaf |
| 0x0604F6 caller context | INSUFFICIENT_EVIDENCE | whole caller boundary and semantic closure |
| 0x060BCC caller context | INSUFFICIENT_EVIDENCE | hardware predecessor and unknown caller/sibling effects |
| 0x00FF0010..0x00FF0016 | ROUTINE_CONTRACT_PARTIAL | field boundaries, lifetime, aliasing and meaning |
| 0x61032..0x610C8 | CONTINUATION_BLOCKED | routine-specific per-instruction/indirect-read continuation; old adapter is lump-sum |
| 0x3820..0x3B3E | CONTINUATION_BLOCKED | CCR.X, interrupt suspension, prefetch/IR and multi-exit timing |
| 0x6121A..0x61230 | HARDWARE_BLOCKED | VDP-visible writes to 0xC00011 |
| broad graphics slices | SEMANTICS_BLOCKED | no closed routine/data ownership |
| M11.47 0x00026A, 0x06193C, 0x061954 | ROUTINE_CONTRACT_PARTIAL | isolated forms lack caller/exit/semantic contracts |

## Phase 5 — cluster detection

### C1 — RamFlag caller/data cluster

Members are RamFlagRoutine, direct sites 0x0604F6 and 0x060BCC, and raw
window 0x00FF0010..0x00FF0016. Classification is CALL_GRAPH_CLUSTER plus
MEMORY_STRUCTURE_CLUSTER. It satisfies the minimum connected-member condition
through exact BSR edges and repeated raw byte offsets. It is not a
PORTABLE_SUBSYSTEM_CANDIDATE: caller CFGs are not closed, 0x060BCC is
hardware-coupled, and semantics remain raw.

### C2 — TableCopy caller cluster

Members are 0x002D58 and TableCopyRoutine. Classification is
STRUCTURAL_CLUSTER / CALL_GRAPH_CLUSTER, based on the exact direct BSR and
closed callee. It contains only one portable routine and has no proven shared
structure with RamFlag.

Overall, C1 is the first useful routine/data cluster, but the two authoritative
portable routines remain separate ownership islands. This supports
PORTABLE_ROUTINE_CLUSTER_PROVEN, not
FIRST_PORTABLE_SUBSYSTEM_BOUNDARY_IDENTIFIED.

## Phases 6–10 — architectural decision and typed-data gate

Selected target: B — ROUTINE_CLUSTER. A third isolated routine would add
coverage without closing ownership. A typed structure is rejected: the raw
window lacks complete field/lifetime/aliasing proof; A5-relative output lacks
a proven base; TableCopy output has runtime size/ownership unknown. Thus
PORTABLE_TYPED_STRUCTURES = 0 and PORTABLE_SUBSYSTEM_CANDIDATES = 0.

The generic M11.52/M11.53 bridge is already available. 0x61032 remains
ROUTINE_SPECIFIC_COMPLEXITY: conditional 24/25-write effects, one computed
indirect read and exact per-instruction refresh/return proof are missing.
0x3820 remains ROUTINE_SPECIFIC_COMPLEXITY: its long branch-heavy body needs
CCR.X, interrupt, prefetch/IR, multi-return and timing closure. No generic
extension or new timing constant is justified.

Exact 0x6121A is LEA $00C00011,A0, four immediate byte writes (#$9F, #$BF,
#$DF, #$FF) to (A0), then RTS. The ordering is hardware behavior. No useful
pure-prefix/pure-suffix split is proven; it remains HARDWARE_BLOCKED.

The accepted partial boundary is: oasis_core owns portable TableCopy/RamFlag
semantics and opaque continuation tokens; tools/hybrid owns ROM PC/opcode
provenance, GPGX timing/prefetch/IR/RTS, checkpoint/oracle, accounting and
hardware interaction. Core has no ROM PCs, GPGX/libretro types, checkpoint
serialization or emulator API. No subsystem boundary is accepted.

## Phase 11 — third-routine gate

Not warranted. The cluster is already proven at structural/raw-data level; a
third routine would not close caller ownership or the hardware boundary. No
third routine was reconstructed or promoted.

## Phase 12 — architecture metric inventory

| Metric | Count |
| --- | ---: |
| AUTHORITATIVE_NATIVE_ROUTINES | 2 |
| PORTABLE_MECHANICAL_PRIMITIVES | 4 |
| COMPLETE_ROUTINE_CONTRACTS | 2 |
| PARTIAL_ROUTINE_CONTRACTS | 3 |
| PORTABLE_TYPED_STRUCTURES | 0 |
| PORTABLE_SUBSYSTEM_CANDIDATES | 0 |
| HARDWARE_BLOCKED_ROUTINES | 1 |
| CONTINUATION_BLOCKED_ROUTINES | 2 |

The inventory is not a percentage target.

## Validation

Fresh Debug and Release MinGW builds passed full CTest `66/66`; the fresh
GNU-equivalent UCRT/MinGW build also passed `66/66`. The direct mechanical and
developer-only candidate/dispatch regression executables passed from the
fresh Debug build. `git diff --check` and the source-code line-limit gate
passed. The GPGX-linked `oasis_hybrid_poc` target is conditional on the
external instrumented GPGX source/configuration and was not emitted by these
fresh standalone builds; the unchanged M11.53 proof executable nevertheless
reproduced the authoritative dual-native baseline twice before this report.

## Proposed M11.55 — not executed

Proposed name: Close the RamFlag caller/data cluster boundary.

Scope: audit only 0x0604F6, 0x060BCC, their bounded CFGs, sibling effects and
raw 0x00FF0010..0x00FF0016 accesses. Prove or reject whole caller boundaries,
return PCs and per-site natural attribution. Establish whether the 0x00A11100
operation can remain wholly adapter-owned. Add a typed structure only if every
base, size, offset, width, direction, lifetime, aliasing and endian gate
closes. Do not emulate new hardware or search for new gameplay callers.

Acceptance: exact static/dynamic provenance, bounded CFG closure, raw-memory
ledger, hardware isolation, standalone differential vectors, paired native and
shadow checkpoint/video/accounting proof, zero fallback/divergence and the
metric inventory. Implementation is not warranted at M11.55 start; it becomes
warranted only after caller/data closure. Otherwise record the negative result.

## STOP

The unchanged baseline passed twice. Static slices, the callee-effect oracle
and caller-stack oracle were run against the canonical ROM. M11.53 source and
runtime behavior were not changed. M11.55 was not executed.
