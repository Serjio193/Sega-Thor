# Current Task

TASK: M11.5 Single Worker Sequential Ant Queue PoC v1
WHY: extend the proven one-frontier natural ant loop to a small frozen queue
processed strictly sequentially by one worker, with explicit lifecycle,
duplicate suppression, honest failures and cumulative static feedback.
CURRENT MILESTONE: M11.5 post-explorer sequential evidence queue
MILESTONE UNDERSTANDING CONFIDENCE: 93%
CURRENT SLICE UNDERSTANDING CONFIDENCE: 92% for a bounded five-job queue,
single-worker restart policy, deterministic lifecycle and batch merge.
SLICE MODE: RE_TOOLING_ONLY
STATUS: COMPLETE

Acceptance criteria:
- [x] create a deterministic frozen queue of exactly five current explorer
  `INDIRECT_FLOW` frontiers using documented ranking;
- [x] reference existing `oasis.m68k.re-ant-job.v1` jobs without duplicating
  their schema fields unnecessarily;
- [x] enforce one `CLAIMED` job at a time and explicit lifecycle transitions;
- [x] suppress already accepted resolved duplicates and recover stale claims;
- [x] process the queue sequentially with one BizHawk worker instance at a time;
- [x] run the identical frozen queue A/B and compare queue/result determinism;
- [x] preserve `DYNAMIC_NATURAL`/checkpoint provenance and reject forbidden or
  contradictory results;
- [x] batch-merge accepted results and rerun `oasis_re_explore` once;
- [x] record queue/lifecycle/ROI/performance metrics and stop before parallelism.

EVIDENCE AVAILABLE: previous checkpoint `55ffd5b` resolved `0x020E:0x045A`
to `0x307A`. A full 1800-frame neutral natural probe currently reaches three
distinct indirect frontiers: `0x045A`, `0x61F60` and `0x62878`. The queue
policy will rank these observed candidates first and fill the bounded five-job
queue with the next stable unresolved frontiers; any NOT_REACHED result remains
valid data and is not forced.
KNOWN UNKNOWNS: the two stable-address fallback jobs were not reached by the
existing natural scenario. Their `FAILED_FINAL` results are retained as evidence;
no target was invented or forced.
RESULT: queue `queue-0x4C23AB2632531710` used five jobs. Three resolved to
`0x307A`, `0x6211A` and `0x62900`; two fallback jobs at `0x0790` and `0x5328`
ended `FAILED_FINAL` after bounded neutral runs. A/B normalized result sets,
including observed registers and targets, were equal with SHA-256
`3B38333E9688208096CDA5D92178CFA0F01FBD32A15514E3A2AA9B9FE2657BFE`.
Batch merge accepted three dynamic edges and one explorer rerun changed
instruction bytes `60916 -> 61506`, decoded instructions `19623 -> 19765`,
entries processed `537 -> 541`, unresolved indirects `35 -> 32`, and frontiers
`148 -> 145`. Production runtime remains unchanged.
EXACT NEXT ACTION: commit, push, verify CI, then STOP. Do not schedule another
frontier job, add parallelism, or begin M12.

TASK: M11.5 Single Ant Closed-Loop PoC v1
WHY: prove one real indirect frontier can become a deterministic natural
emulator job, produce runtime target evidence, merge back into static
exploration and yield measured before/after structural results.
CURRENT MILESTONE: M11.5 post-explorer dynamic evidence bridge
MILESTONE UNDERSTANDING CONFIDENCE: 94%
CURRENT SLICE UNDERSTANDING CONFIDENCE: 93% for the one-frontier job/result,
natural BizHawk observation, provenance-preserving merge and static rerun;
the selected target is still runtime evidence to be captured.
SLICE MODE: RE_TOOLING_ONLY
STATUS: COMPLETE

Acceptance criteria:
- [x] select exactly one current `INDIRECT_FLOW` frontier with deterministic
  identity and existing natural provenance;
- [x] create deterministic `oasis.m68k.re-ant-job.v1` JSON with no forced
  register/flag mutation and bounded limits;
- [x] execute exactly one BizHawk worker and emit
  `oasis.m68k.re-ant-result.v1` with source/register/next-PC evidence;
- [x] run the identical job twice and compare normalized result hashes;
- [x] reject wrong frontier, ROM mismatch, forced evidence and nondeterministic
  results in CI-safe synthetic tests;
- [x] merge only NATURAL_OBSERVED evidence while preserving static/dynamic
  provenance and rerun `oasis_re_explore` with the dynamic edge;
- [x] record before/after metrics, ROI, performance and checkpoint policy;
- [x] keep production runtime unchanged, pass local validation and stop after
  this one job.

SELECTED FRONTIER: `0x0000020E:0x0000045A:INDIRECT_FLOW`, identity
`size=3145728;fnv1a64=EA6BB7880F4BB247:0x0000020E:0x0000045A:INDIRECT_FLOW:address_indirect:computed target is unresolved`.
`0xA7E2` was rejected because current natural bounded scenarios did not reach
it; `0x045A` was reached from hardware reset at frame 114 and resolved twice
to `0x307A` with result hash `0x21238399`.

RESULT: job `ant-0x43919998981C2FF` accepted as `DYNAMIC_NATURAL`. The static
rerun closed the selected frontier and added one provenance-tagged dynamic
edge; instruction coverage grew `60916 -> 61396` bytes, decoded instructions
`19623 -> 19729`, entries processed `537 -> 539`, and frontiers fell
`148 -> 147`. Production runtime was unchanged.
EXACT NEXT ACTION: STOP. Do not schedule another frontier job or begin M12.

TASK: M11.5 oasis_re_explore bounded recursive exploration engine v1
WHY: automate structurally provable 68000 ROM exploration while preserving
explicit uncertainty for indirect flow, unsupported instructions, boundaries
and code/data conflicts. This is developer-only RE tooling; production runtime
and gameplay semantics remain out of scope.
CURRENT MILESTONE: M11.5 post-M11 evidence tooling
MILESTONE UNDERSTANDING CONFIDENCE: 95%
CURRENT SLICE UNDERSTANDING CONFIDENCE: 95% for the bounded worklist,
provenance, region-map, blocker/frontier and deterministic report contracts;
ROM-wide coverage remains an evidence measurement, not semantic truth.
SLICE MODE: RE_TOOLING_ONLY
STATUS: COMPLETE

Acceptance criteria:
- [x] fresh raw Ghidra baseline is verified A/B and candidate-map/mass metrics
  are rerun before Ghidra candidates are used as seeds;
- [x] reuse audit is recorded; existing decoder/CFG logic is reused without
  duplication;
- [x] deterministic tiered seed model and priority worklist exist;
- [x] recursive direct calls, jumps, conditional branches, DBcc, returns and
  valid fallthrough are recorded with explicit path stop reasons;
- [x] persistent instruction/data/conflict address map and explicit analysis
  states are emitted;
- [x] indirect/unsupported/data/conflict frontiers have stable identities and
  machine-readable records;
- [x] CI-safe synthetic tests cover the required control-flow, ownership,
  guard, conflict, suppression, identity and serialization cases;
- [x] bounded control corpus passes before one optional ROM-wide measurement;
- [x] deterministic A/B JSON/text comparison, Debug/Release/GNU-equivalent
  validation, file-limit, diff-check and sensitive-artifact hygiene pass;
- [x] blocker clusters are ranked and the task stops before any dynamic ant,
  emulator scheduler, semantic translation or M12 work.

Reuse audit:
| Capability | Existing implementation | Reuse | Extension |
| instruction/operand decode | `re_slice_decoder` | direct | none |
| direct target resolution | `DecodedInstruction`/`ControlFlowEdge` | direct | edge classification |
| CFG representation | `DecodedSlice`/`BasicBlock` | direct | guarded traversal |
| reachable traversal | `re_cfg_audit` local walk | pattern only | explorer-owned guarded worklist |
| call-edge representation | `ControlFlowEdge`/`AtlasCallEdge` | direct | recursive provenance |
| unresolved indirect/unsupported | decoder slice fields | direct | frontier records |
| Atlas/candidate input | `re_atlas`/`re_candidate_map` | direct | tiered seeds and guards |
| deterministic JSON helpers | existing formatters | convention | explorer formatter |

RESULT: implemented and verified. The bounded gate passed for 15 control
entries and all six known edge anchors. One gated ROM-wide measurement
processed 537 entries and emitted deterministic map/frontier evidence.
KNOWN BLOCKERS: WSL/Linux runtime validation is unavailable because WSL is not
installed; no semantic or dynamic conclusion is claimed.
EXACT NEXT ACTION: stop before the dynamic ant/scheduler PoC; consume the
frontier format only in a separately authorized bounded emulator checkpoint.

TASK: M11.5 Mass Structural Verification Pass v1
WHY: batch-check the normalized Ghidra candidate map with the existing
project-specific decoder and Atlas evidence, then measure failure classes and
rank one to three systemic follow-ups without promoting semantics.
CURRENT MILESTONE: M11.5 post-M11 evidence tooling
MILESTONE UNDERSTANDING CONFIDENCE: 95%
CURRENT SLICE UNDERSTANDING CONFIDENCE: 94% for the bounded decode,
classification, clustering and deterministic report contract; semantic
confidence remains intentionally limited to independent project evidence.
SLICE MODE: RE_TOOLING_ONLY
STATUS: COMPLETE

## Current task — mass structural verification

Acceptance criteria:
- [x] process all 534 normalized entries in stable ascending-address order;
- [x] record bounded entry decode, reachable flow, boundary and overlap signals;
- [x] classify failure reasons and aggregate quantitative clusters;
- [x] preserve all 11 confirmed control entries and mark heuristic misses
  without downgrading them;
- [x] provide GHIDRA_ONLY and STATIC_SUPPORTED structural breakdowns plus leaf
  analysis and top-three systemic-fix ROI estimates;
- [x] add synthetic CI-safe tests for classification, clustering, controls,
  boundary/leaf behavior, deterministic ordering, duplicates and malformed
  input;
- [x] pass Debug, Release and GNU/MinGW-equivalent full validation and
  deterministic A/B report comparison.

RESULT: added developer-only `oasis_re_mass_verify`. The 534-entry bake-off
processed 483 previous `GHIDRA_ONLY`, 39 `STATIC_SUPPORTED`, 11 `CONFIRMED`
and 1 `CONFLICT` record. The current local run used the prior normalized
candidate-map evidence to reconstruct the external Ghidra-shaped input because
the original 390972-byte export is not present in this checkout; this is a
reproducibility exercise, not an independent re-export. The pass produced
208 clean leaves of 234, with 7 unsupported, 13 indirect-flow, 6 boundary-
conflict and 0 terminal-failure leaves. The largest measured clusters were
boundary-longer-than-Ghidra (185), multiple-entry-overlap (82), and
unsupported-opcode (54). Top fix classes were boundary continuation (277
affected), decoder coverage (61), and static edge recovery (51).

DETERMINISM: mass JSON SHA-256
`C826718A0FD1AB0CFE1730B3C7E184A79C70954DD356DC6FC009216DF46A1EA6` and
human report SHA-256
`3A905792BBA276D444EA10D280CCFAED59C903FC54DF2EC2EC239316AA5BC616` matched
between runs A/B. The report excludes wall-clock duration from serialized
bytes; the two local runs took 13385 ms and 13434 ms respectively.

NEXT_ACTION: STOP after measurement. The next checkpoint may implement exactly
one systemic fix, selected from the measured ROI, after the raw Ghidra export
is restored for an independent rerun.
BLOCKERS: raw external Ghidra export is absent from this checkout; Linux/WSL
runtime validation is unavailable because WSL is not installed. No broad
runtime trace or semantic translation was started.

## Current task — candidate integration

Acceptance criteria:
- [x] parse the prior deterministic Ghidra export and reject malformed input;
- [x] merge Ghidra functions/candidates, Atlas entries, existing static edges,
  documented runtime observations and bounded Beta correspondence;
- [x] preserve the 11 known confirmed benchmark entries and never promote a
  Ghidra-only record to `CONFIRMED`;
- [x] classify every normalized entry and retain code/data and boundary
  conflict metadata;
- [x] emit deterministic full JSON and top-20 human-readable ranking with
  documented scoring and top-10 non-confirmed quality audit;
- [x] synthetic merge/ranking/serialization tests and Debug/Release/GNU
  validation pass;
- [x] two real runs produce byte-identical candidate JSON and text reports.

RESULT: the developer-only `oasis_re_candidate_map` tool consumes the external
`oasis.m68k.ghidra-map.v1` export and existing bounded Atlas/evidence. The real
union has 534 unique entries: 11 `CONFIRMED`, 39 `STATIC_SUPPORTED`, 0
`DYNAMIC_OBSERVED`, 483 `GHIDRA_ONLY` and 1 `CONFLICT`. Dynamic observations
remain visible on records even when stronger static/project evidence determines
the classification. Complexity counts are LEAF 234, SHALLOW 172, COMPLEX 90
and UNKNOWN 38. The selected next bounded target is `0x611EA`, a shallow
Ghidra function with existing static call-site support and no recorded conflict.

INPUTS: external Ghidra v3 JSON SHA-256
`613A3AA6DEB8D2DCF994C82ADC6A6939B7D5F27AF67A51D05C16F090D60A5315`; current
Atlas; existing bounded USA Beta correspondence; existing A6A4 dynamic
scenario and documented BizHawk natural observations for `0x60B8C`, `0x611EE`
and `0x6121A`. No new ROM-wide trace was run.

DETERMINISM: full JSON SHA-256
`5C17F6A735DC715B18CD5A5E8FA34F876CAD5CEB5D4511C80547E2C8720A22AC` and
human report SHA-256
`9E5A5884FE05B816C0265535529E79E2C336FBB4CE176D60AF57F522DAFD9C84` matched
between runs A/B.

NEXT_ACTION: STOP after this bounded candidate-ranking checkpoint. The next
task may inspect `0x611EA`, but must not begin it in this checkpoint.
BLOCKERS: Ghidra boundaries, indirect-flow coverage and routine semantics
remain independent-reverification unknowns.

## Previous checkpoint — availability gate

Acceptance criteria:
- [x] baseline synchronized: local `main == origin/main` at
  `5aeb338c8d8be86f1de0178815abdfd00e6db890`;
- [x] required project governance and evidence documents reviewed;
- [x] local Ghidra GUI and headless availability checked without downloading
  an unofficial build or installing through CI;
- [x] canonical USA ROM import and fingerprint verification;
- [x] conservative baseline auto-analysis and deterministic map export;
- [x] known-entry, call-edge, data-table and indirect-flow benchmarks;
- [x] bounded false-positive sample and evidence-based A/B/C decision;
- [x] Ghidra export parser/tests and local Debug/Release/GNU validation, if
  reusable repository code is added.

Availability check: initially no Ghidra installation was present. The user
then explicitly authorized an official developer-only install. Ghidra
12.1.3 was downloaded from the official NSA GitHub release
`Ghidra_12.1.3_build` and verified with SHA-256
`93a5d11a9ad510622acaaf908c556a7b9b764d338e78a7567f3689bf5081fd54`.
Headless `support\\analyzeHeadless.bat` runs. It exposes the 68000 family
language `68000:BE:32:default` (big-endian, 32-bit address space; the release
describes the default variant as Motorola 68040, while the processor family
is 68000). Temurin JDK `21.0.12.1+1` 64-bit LTS is installed outside the
repository and verified with SHA-256
`f9d6e191ab098c0d416e7d588a24420a8621cd2f4720dab2459b8b7b2d2d8b4e`.

The supplied canonical USA ROM matched the required size `3145728` and
SHA-256 `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
The official developer-only Ghidra baseline imported it as raw binary at
`0x000000` with `68000:BE:32:default`; the project and exports remain outside
the repository. Repeated runs produced byte-identical JSON.

Benchmark result: 7/11 exact function matches, 1 wrong boundary, 1 code-only
entry and 2 missed entries; 9/11 entries had code presence. Required call
edges were found 4/6. All four data-table addresses decoded as non-code and
had useful xrefs, but only `0xC92C` was recognized as defined data. The
`0xA7E2` indirect-flow observation found a `jmp`, but Ghidra exposed no
indirect target. The bounded 20-item false-positive sample contained 19
`LIKELY_CODE` and 1 `AMBIGUOUS` item. Decision:
`GHIDRA_USEFUL_WITH_PROJECT_FIXUPS`. Ghidra is retained as a structural
discovery layer; `oasis_re` and runtime evidence remain the verification
authority. Do not begin M12.

LAST_VERIFIED_RESULT: the frozen scenario reaches `0x60BFA` and `0x60C08` at
frame 423. Actual A0 is `0x0006F8B0` / `0x0006F8B2`, so the byte reads resolve
to ROM `0x0006F8B1`=`0x13` / `0x0006F8B3`=`0x00`. A0 differs from the earlier
post-`0x60BD0` `0x0006F8AE`; this is scenario-only evidence. Fresh A/B JSON
SHA-256: `CF092C8B91BD2FDA858E3E165A75D3A891F8B90997D6F3E839A65FD053C97D91`.
NEXT_ACTION: STOP at this bounded M11.5 checkpoint; await an explicit new task
BLOCKERS: writer callback width, semantic role and cross-scenario invariance
remain unknown

## Current checkpoint result — downstream runtime resolution

Both static operations were independently verified against the canonical USA
ROM: `0x60BFA` bytes `16 28 00 01`, `MOVE.B 1(A0),D3`; `0x60C08` bytes
`14 28 00 01`, `MOVE.B 1(A0),D2`. BizHawk 2.11.1 captures full D0-D7/A0-A7/SR
snapshots at both boundaries. The report records effective addresses derived
from actual A0 plus displacement 1, ROM classification, raw byte values,
`resolution_scope=scenario_only` and `resolution_status=runtime_resolved_for_scenario`.

The previous relevant unresolved count is 2 target rechecks; 2 are now
scenario-resolved and 0 are globally resolved. Global invariance is not proven.

## Current checkpoint result — bounded runtime stack provenance

Implementation: developer-only `src/tools/re_bizhawk_stack_provenance.lua`,
schema `oasis.m68k.re-stack-runtime-provenance.v1`, reusing the frozen
hardware-reset `start_pulse_120` scenario and canonical USA ROM. The bounded
probe watches only the requested path and concrete stack range `[P,P+4)`;
it does not add an emulator, interpreter, generic tracer or production
dependency.

Raw runtime chain: `0x60B8C` A7=`0x00FF0BA8`; `0x60BCC` has the same P and
`memory[P]=0x0006F8AE`; `0x60BCC` is `BSR.W 0x604BC` with return address
`0x60BD0`; callee entry A7=`0x00FF0BA4` and return slot=`0x00060BD0`;
`0x604E4 RTS` returns to P; pre-`0x60BD0` A7=P and stack longword is unchanged;
post-`0x60BD0` A0=`0x0006F8AE`, A7=`0x00FF0BAC`. The report records target
boundaries `0x60BFA` and `0x60C08` as reached only, with raw event snapshots.

Bounded writer evidence: static USA bytes at `0x60B66` are `2F 08`
(`MOVE.L A0,-(A7)`). BizHawk reports two concrete-range bus writes at
`0x00FF0BAA`=`0x0000F8AE` and `0x00FF0BA8`=`0x00000006`, callback PC
`0x00060B68`, correlated to instruction `0x60B66`; callback width is UNKNOWN.
The reconstructed final four bytes are `0x0006F8AE`. No semantic name is
assigned, and this scenario-local observation does not globally resolve the
static value.

Validation: Debug, Release and GNU/MinGW-equivalent full CTest 27/27; USA
stack-provenance oracle and prior natural oracle pass; deterministic A/B JSON
and human reports match; file-limit and `git diff --check` pass. No ROM,
emulator binary, savestate or raw trace is tracked. GitHub Actions CI run
`33874638457` for implementation commit `ea14f898d93f3508b877ce1f059f7926cfebe2cd`
completed successfully. This checkpoint is complete with the bounded BizHawk
writer-width limitation documented.

## Natural caller search result

Search family `natural_reach_60b8c_60d4a_v1` reused
`src/tools/re_bizhawk_natural_scenario.txt`, canonical USA ROM SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`, BizHawk
2.11.1, hardware reset and the existing Lua probe. The additive report fields
are in `oasis.m68k.natural-reach.v1`; no new emulator or trace framework was
introduced.

The bounded search tested neutral baseline, then stopped at the first success:
`start_pulse_120` (`120:Start`, max 1800). It reached `0x60B8C` at frame 423
after 424 frame advances. The watched counts were `0x60B8C=3`, `0x60D4A=0`,
`0x6121A=5` and `0x611EE=2`. The first `0x60B8C` event has PC
`0x60B8C`, A7 `0x00FF0BA8`, and stack window
`[0x00FF0B88,0x00FF0BE8)`. A downstream `0x6121A` event is paired with the
same raw caller and its BSR return address `0x60B90` matches the observed
stack longword. No semantic role is assigned to any value.

Bounded minimization changed only the timing of the same single `Start` pulse:
`119:Start` and `121:Start` also reached `0x60B8C` at frame 423. Removing the
only input (neutral baseline) did not reach either requested caller. Thus the
successful input has one event and one button; timing equivalence near frame
120 is recorded, not interpreted. Two fresh `start_pulse_120` replays have
identical JSON SHA-256 `20AA010BAECFE696A119D431A7EE6562074848219DD9C08A16D00BE3BBD994F2`
and trace SHA-256 `66F0095A195A9899789F08D0D4E8C5CF45EEFDDEE97EEE14B8A24516A9FB2271`.

This is a bounded natural reachability result only. The probe captured 438
ordered report events, but it is not a complete instruction trace and reports
no inferred basic blocks. `0x60D4A` remains unresolved by this scenario;
return state beyond the watched downstream hook, input meaning and gameplay
semantics remain UNKNOWN.

## Dynamic caller discrimination result

The same frozen scenario `src/tools/re_bizhawk_natural_scenario.txt` is used:
schema `oasis.m68k.emulator-scenario.v1`, canonical USA SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`, BizHawk
2.11.1, hardware reset, neutral input and `max_frames:300`. The existing
developer-only Lua probe adds only four exact execution hooks and never writes
ROM, registers or memory.

Both runs reached `0x6121A` twice at frame 113. Exact ordered pairs are
`caller seq=113, frame=113, PC=0x611EE` → `target seq=114, frame=113,
PC=0x6121A`, then `caller seq=115` → `target seq=116`. Both callers are the
same static site; the second pair has a different raw A7 state. Caller and
target snapshots include D0-D7/A0-A7/SR and stack windows.

For hit 1, caller A7=`0x00FF0BE6`, target-entry A7=`0x00FF0BE2`, delta `-4`,
stack longword at target entry=`0x000611F2`, expected return=`0x000611F2`,
match=`true`. For hit 2, caller A7=`0x00FF0BAC`, target-entry
A7=`0x00FF0BA8`, delta `-4`, with the same matching return longword. Raw
register delta for both pairs contains only A7; D0-D7, A0-A6 and SR are
unchanged in the captured snapshots. This is CPU-level evidence only.

The normalized trace has 117 events, 23 unique PCs, 0 inferred basic blocks,
0 inferred branch/call/return/read/write events and deterministic hash
`0x52F951E69F5A7100`. Natural reports, normalized traces and imported reports
are byte-identical across A/B. The secondary natural targets remain zero.

Static caller verification is exact: `0x60B8C` bytes `61 00 06 8C`, size 4,
return `0x60B90`; `0x60D4A` bytes `61 00 04 CE`, size 4, return `0x60D4E`;
`0x611EE` bytes `61 00 00 2A`, size 4, return `0x611F2`. All compute target
`0x6121A`. The natural run therefore upgrades only the specific edge
`0x611EE -> 0x6121A` to executed in this scenario. It does not upgrade the
other two edges, and it does not assign a name or purpose to `0x6121A`.

The proven caller is `0x611EE`, so
`relevant_to_existing_stack_blocker=no`: this neutral path is distinct from
the existing `0x60B8C`/`0x60D4A` stack-blocker paths. The two target hits are
two observed caller-target events; whether the second is re-entry or another
raw control-flow circumstance is UNKNOWN. `0x6135E` remains only a prior
frame-boundary sample, not caller evidence.

## Search acceptance criteria

- [x] existing hardware-reset USA scenario and BizHawk backend reused;
- [x] bounded search stopped at the first primary caller hit;
- [x] one raw `Start` pulse reaches `0x60B8C` and downstream `0x6121A` is observed;
- [x] caller/target snapshots include the requested `[A7-0x20,A7+0x40)` window;
- [x] neutral removal check, adjacent timing minimization and two-run replay equality passed;
- [x] local USA oracle, Debug/Release/GNU CTest, file-limit and diff-check passed;
- [x] no production runtime, emulator, whole-game trace, broad search or M12 work added;
- [ ] `0x60D4A`, complete instruction/basic-block trace and input meaning remain unknown.

## Previous caller-discrimination acceptance criteria

- [x] exact canonical-USA bytes, BSR.W displacement and return addresses verified for all three callers;
- [x] only the frozen natural scenario and four exact caller/target hooks used;
- [x] both target hits paired with `0x611EE` in exact event order;
- [x] caller/entry D0-D7/A0-A7/SR, A7 delta and stack return longword captured;
- [x] two-run replay equality, deterministic normalized hash and local USA oracle;
- [x] natural probe remains developer-only and separate from `oasis_core`;
- [ ] callee semantics, stack-writer provenance and meaning of the second hit remain unknown;
- [ ] `0x60B8C` and `0x60D4A` are not dynamically selected by this scenario.

## Hard boundaries and exact next action

No emulator, copied emulator source, production runtime dependency, whole-game
trace, autoplay, semantic Atlas changes, call-clobber resolution or M12 work
was added. STOP at this verified natural reachability result; do not begin
dynamic tracing expansion or another analysis scope without explicit instruction.

## Previous caller-stack checkpoint

The additive developer-only schema `oasis.m68k.re-caller-stack.v1` walks only
the existing reachable `[0x60004,0x61204)` CFG to the call-site block
`[0x60BC4,0x60CDA)`, with reachable predecessor `0x60BA4`. It tracks symbolic
A7 from `S`, exact bounded push/pop events and conservative path merges. The
USA oracle records two relevant paths. One includes the `0x6042A`, `0x60430`,
`0x60B66` push sequence and unknown direct call `0x60B8C -> 0x6121A`; the
other crosses unknown `0x60D4A -> 0x6121A` and a locally proven balanced call.
These unknown calls invalidate stack contents and A7 provenance; the
later known `0x60BCC -> 0x604BC` effect is used only to record its balanced
return-address mechanics. Therefore `memory[P]` is not proven and both
`0x60BFA` and `0x60C08` remain unresolved: reachable unresolved `16→16`, with
the 14 `call_clobber` items unchanged and speculative resolutions zero.

The exact raw paths were checked against the supported USA ROM. No semantic
role is assigned to any stack value.

## Previous callee-effect checkpoint

The call-site bytes `61 00 F8 EE` at `0x60BCC` decode to direct `BSR.W
0x604BC`; the bounded callee is `[0x604BC,0x604E6)` with one reachable block
and `RTS` at `0x604E4`. It has no nested calls, unsupported instructions or
indirect flow. A0 is overwritten_unknown by `(A0)+` writes, A1-A5 are
not_touched, A6 is overwritten_known `0x00FF06F2`, and A7 is preserved.
The callee has no explicit stack delta; `RTS` removes the BSR return address.

At raw level, caller A7=`P` → BSR pushes return address `0x60BD0` at `P-4`
→ callee returns with A7=`P` → `0x60BD0` executes outside the callee and reads
longword memory at `P`, then increments A7 by 4. That stack value remains
unknown, so targets `0x60BFA` and `0x60C08` remain unresolved; 14
`call_clobber` refs are untouched and speculative resolutions are zero.
