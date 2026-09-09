# Current task

TASK: M11.45 Bounded Semantic Closure Toward 95% Dynamic Coverage
STATUS: COMPLETE — `REMAINING_ATTRIBUTION_AND_DYNAMIC_COVERAGE_95_PROVEN`
BASELINE: `b1c624072e1341bbe26a569841d325152cf3bd7b`
SCOPE: Resume from M11.44 with an eligible semantic-only tranche. Preserve the
generic boundary, timing/refresh, checkpoint, bus and developer-only hybrid
contracts; do not broaden hardware, indirect CFG, decoder or unresolved runtime
address scope.
RESULT: The restart gate matched twice. After retaining three exact shadow
rejections (LSR.W, ROR.W and CMPI.B), 551 decoder-owned generated candidates
removed exactly 271,913 interpreter executions. Shadow completed 6,199,718
comparisons with zero divergence; native 600-frame proof preserved checkpoint
`251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d` and video
`5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`.
Final total is 6,488,773, translated 6,199,718, interpreter 289,055,
registered ranges 580, yields 149,059, interrupted resumptions 288, and
translated share 95.5453%. No fallback or hardware-visible access occurred.
EVIDENCE: `docs/reports/REMAINING_INTERPRETER_ATTRIBUTION_M11_45.md` and
`docs/reports/SEMANTIC_CLOSURE_M11_45.md`
NEXT ACTION: Stop; do not implement the next milestone in this task.

TASK: M11.44 Restart — Remaining Interpreter Attribution and 95% Coverage Gate
STATUS: COMPLETE — `REMAINING_INTERPRETER_ATTRIBUTION_PROVEN_SEMANTICS_BLOCKED`
BASELINE: `f71c92eecc128ef3eb2ffac835ec9f9dfd6bde94`
SCOPE: Resume M11.42 only after the M11.43 checkpoint identity repair; build an
exhaustive decoder-backed interpreter ledger, re-test 0x03A7AE, and promote only
the exact mechanically generated block that passes the existing shadow/native
gates. No hardware-contract broadening, indirect-CFG invention or 95% forcing.
RESULT: The 661,916 baseline remainder is closed exactly. The generated
0x03A7AE–0x03A7B8 block removed 100,948 executions after zero-divergence shadow
and native proof. The final run has 560,968 interpreter executions and 91.3548%
translated share; the 95% gate is not met. Hardware 0x060BA4 remains fallback.
EVIDENCE: `docs/reports/REMAINING_INTERPRETER_ATTRIBUTION_M11_44.md` and
`docs/reports/REMAINING_INTERPRETER_ATTRIBUTION_FINAL_M11_44.md`
NEXT ACTION: Do not implement the next milestone in this task.

TASK: M11.43 Complete GPGX Checkpoint Canonicalization Contract
STATUS: COMPLETE — `CHECKPOINT_CANONICALIZATION_COMPLETED_NEW_AUTHORITATIVE_IDENTITY`
BASELINE: `c59dd1bb62e03df0639484de214a482204eb0d63`
SCOPE: developer-only exact layout contract and identity canonicalization for the
pinned GPGX v1.7.6 STATE_SIZE format. M11.42 attribution, semantic expansion,
generation, shadow and coverage work were not started.
RESULT: The proven contract preserves semantic bytes and clears only generated
YM2612/Z80 host-pointer and ABI-padding spans. Five independent current proofs,
three raw current runs, and current/exact-historical/M11.41 raw replay agree on
aggregate `251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d`.
The prior `c9236218...` identity is superseded because M11.41 cleared semantic
bytes; its history remains unchanged below.
EVIDENCE: `docs/reports/CHECKPOINT_CANONICALIZATION_M11_43.md`
NEXT ACTION: M11.42 PHASE 1 may be resumed as a separate task; do not begin
PHASE 2 in M11.43.

TASK: M11.42 Restart — Remaining Interpreter Attribution and 95% Coverage Gate
STATUS: BLOCKED — `M11.42_BASELINE_BLOCKED_CHECKPOINT_CANONICALIZATION_INCOMPLETE`
BASELINE: `5c19e22a93150ab17f51c69ed5aadaa59651a70a`
SCOPE: restart PHASE 1 only until the repaired authoritative checkpoint identity
is reproduced. If the gate passes, account for all `661,916` interpreter
executions through exact evidence, then apply the existing semantic, generator,
shadow, hardware-boundary and native gates independently.
RESULT: ROM/DLL, video, execution counts, 28-range registry, yields, resumptions
and zero starts inside translated ranges matched, but the required aggregate
`c9236218f55fb18f7f1d5095e4970b25f228de588bd03bd7cbbffccc2e225fd1` was not
reproduced; two fresh runs produced
`d5de401ceb64da875219d3ca2564160b954d55a217f4bb47ff0dc204c547ae36`.
Raw evidence proves the committed M11.41 canonicalization leaves a host-pointer
byte at serialized offset `140734`.
EVIDENCE: `docs/reports/REMAINING_INTERPRETER_ATTRIBUTION_M11_42.md`
NEXT ACTION: repair and independently re-prove checkpoint identity in a new
bounded task; do not perform M11.42 PHASE 2 or weaken the restart gate.

## Historical M11.41 result

TASK: M11.41 Checkpoint Identity Provenance and Reproduction Repair
STATUS: COMPLETE — `CHECKPOINT_BASELINE_IDENTITY_RESTORED`
BASELINE: M11.40 commit `23aab8997592726a001d1483b7f109c6915f6bd8` and exact
M11.39 coverage commit `37857a31a1c2965ecbe68e3695ca5aa187617c2f`.
SCOPE: checkpoint pipeline provenance, byte-level divergence proof and the
minimum developer-only identity repair. Do not perform M11.40 PHASE 2.
RESULT: GPGX wholesale serialization included host pointer/ABI representation
bytes. The first differing byte was frame 60, state offset `140654`, inside
the first YM2612 `FM_SLOT.DT` pointer. Raw buffers remain evidence-only; the
authoritative repaired aggregate is
`c9236218f55fb18f7f1d5095e4970b25f228de588bd03bd7cbbffccc2e225fd1`.
Three current and two exact historical-checkout repaired runs agree on
checkpoint sequence, video, instruction counts, yields and resumptions.
EVIDENCE: `docs/reports/CHECKPOINT_IDENTITY_M11_41.md`
NEXT ACTION: start M11.40 PHASE 2 as a separate bounded task; do not expand
scope in M11.41.

## Historical M11.40 result

TASK: M11.40 Remaining Interpreter Attribution and 95% Coverage Gate
STATUS: BLOCKED — `M11.40_BASELINE_BLOCKED_CHECKPOINT_IDENTITY_MISMATCH`
RESULT: the M11.40 attribution/95% gate was stopped before PHASE 2 because its
raw checkpoint aggregate did not match the historical M11.39 report. M11.41
proved the serialization cause and restored a deterministic authoritative
identity, but did not execute M11.40 PHASE 2.
EVIDENCE: `docs/reports/REMAINING_INTERPRETER_ATTRIBUTION_M11_40.md`

# Historical M11.39 result

TASK: M11.39 Hot-Path Multi-Block Coverage Expansion
STATUS: COMPLETE — `HOT_PATH_DYNAMIC_COVERAGE_80_PROVEN`
BASELINE: committed M11.38 `INTERRUPT_SAFE_MULTI_INSTRUCTION_BLOCKS_PROVEN` at
`daa0a09b5cd8845a48733e771774491b35e73d9e`
SCOPE: one bounded post-M11.38 interpreter profile, two decoder-owned ranges,
the exact new `ADD.W (An)+,Dn` semantic form, generic M11.38 boundary yielding,
and explicit offline promotion. No runtime JIT, indirect-target invention,
hardware-bridge broadening or M11.40 implementation.
RESULT: the ranked profile contained 2,152 remaining interpreter PCs. The
selected `[0x000380,0x0003A0)` 16-instruction loop and `[0x03A864,0x03A868)`
direct BNE range passed independent semantics/generator/provenance and the
full GPGX shadow gate. Native translated 5,826,857 of 6,488,773 guest
instructions (89.7991%), with 661,916 interpreter executions, zero divergence,
zero starts inside translated ranges, exact checkpoint/video hashes, 140,065
boundary yields and 274 interrupted continuations.
EVIDENCE: `docs/reports/HOT_PATH_MULTI_BLOCK_COVERAGE_M11_39.md`
NEXT ACTION: stop; do not implement M11.40 in this task.

# Historical M11.38 result

TASK: M11.38 Interrupt-Safe Multi-Instruction Block Execution
STATUS: COMPLETE — `INTERRUPT_SAFE_MULTI_INSTRUCTION_BLOCKS_PROVEN`
BASELINE: committed M11.37 `CONTROLLED_DYNAMIC_COVERAGE_EXPANSION_PROVEN` at
`5d353beaddffbca73c7388903cc22a790c634330`
SCOPE: only the four M11.37 rejected two-instruction ranges; generic
instruction-boundary yield, per-boundary shadow proof, interrupted continuation
and bounded native promotion. No new discovery, atomic blocks, runtime JIT,
second scheduler or production emulator dependency.
RESULT: all four frozen ranges passed 4,122,062/4,122,062 shadow comparisons
with zero divergence. Native translated 4,122,062 of 6,488,773 guest
instructions (63.5261%), recorded 111,009 event boundary yields and 274 exact
continuations after actual GPGX interrupt service. Current-run checkpoint and
video hashes matched the EMULATED baseline; no hardware-visible accesses or
original starts inside translated ranges occurred.
EVIDENCE: `docs/reports/INTERRUPT_SAFE_MULTI_INSTRUCTION_BLOCKS_M11_38.md`
NEXT ACTION: stop; do not expand coverage without a new milestone.

# Historical M11.37 result

TASK: M11.37 Controlled Dynamic Coverage Expansion
STATUS: COMPLETE — `CONTROLLED_DYNAMIC_COVERAGE_EXPANSION_PROVEN`
RESULT: 16 new natural single-instruction blocks were promoted beside the six
historical entries. Four multi-instruction candidates remained interpreter-only
after observed interrupt interleaving; the hardware-visible `0x060BA4`
candidate remained rejected. Evidence: `docs/reports/CONTROLLED_DYNAMIC_COVERAGE_M11_37.md`.

RESULT: GPGX `m68k.cycles` and `refresh_cycles` are frame-relative accumulated
master-cycle counters rebased by `mcycles_vdp`; the M11.35 mismatch compared a
pre-rebase prediction with a post-rebase execution observer. The generic
post-instruction hook now closes shadow comparison before the next scheduler
or frame transition. The old M11.35 mismatch is preserved in history.
The existing gate passed `185975/185975` shadow comparisons with zero
divergences, then native promotion passed for all three M11.35 candidates and
the three preserved M11.33 blocks.
VALIDATION: final Debug, Release and GNU-equivalent full CTest, semantic and
generator/provenance tests, source-limit/diff/hygiene checks, final GPGX shadow,
and final native 600-frame regression are recorded in the worklog.
NEXT ACTION: stop; do not expand block coverage without a new milestone.

# Historical task

TASK: M11.33 Recomp Generator v1
STATUS: COMPLETE — `MECHANICAL_BLOCK_GENERATION_PROVEN`
SCOPE: Generate and execute the exact three M11.32 basic blocks from the
shared decoder/exact IR through developer-only hybrid helpers.
ACCEPTANCE: generated provenance-bound C++ covers `0x2D66`, `0x604BC` and
`0x61032`; unsupported forms fail closed; generated execution is used by the
registry; existing M11.32 proof remains green; production targets remain
emulator-free; Debug/Release/GNU-equivalent checks and file limits pass.
NON-GOALS: M11.34 semantic-core verification, block discovery, indirect
dispatch discovery, new gameplay coverage, `0x3820` repair, or production CPU
emulation.

RESULT: decoder-owned generated C++ replaced the three handwritten hybrid
bodies. The canonical ROM generator run produced 7 + 1 + 1 instructions with
guest address/opcode/assembly provenance. GPGX shadow/native 600-frame runs
were 14/14 and 14/14 with zero divergence, matching state/video hashes and
20 translated guest instruction executions in native mode.
VALIDATION: Final sequential Debug, Release and fresh GNU-equivalent builds
and full CTest were 51/51 after the fetch and MOVEM-order corrections; final
changed-path hybrid tests were 6/6 in all three configurations. Final GPGX
shadow/native proof passed.
NEXT ACTION: M11.34 semantic core; do not expand block discovery or production
runtime dependencies in this task.

# Historical Task

TASK: M11.32 Basic-Block Recompilation Timing Proof
STATUS: COMPLETE — `BASIC_BLOCK_RECOMP_TIMING_PROVEN`
BASELINE: `c1e28b2f7db6b4d756bf29df4b20dbaffa3ba78d`
SCOPE: developer-only GPGX basic-block execution for `0x2D66`, `0x604BC`
and `0x61032` in the existing 600-frame neutral scenario.
RESULT: 14/14 shadow comparisons passed; 14 native overrides completed with
zero divergences, zero original-body starts, matching serialized checkpoints
and matching video sequence. Production runtime remains emulator-free.
EVIDENCE: docs/reports/BASIC_BLOCK_RECOMPILATION_TIMING_M11_32.md
LIMITS: no ID3 work, no `0x3820` override repair, no manual gameplay, no
coverage expansion, no generic CPU emulator, and no tracked ROM/assets/binaries.
NEXT ACTION: stop; do not broaden the block batch without a new milestone.

TASK: M11.31 Comparative Disassembly Method Transfer Baseline
STATUS: COMPLETE — partial method transfer; no high-confidence template
SCOPE: bounded comparison of public Streets of Rage 2/3 disassembly, extraction
and static-recompilation workflows against existing Beyond Oasis evidence.
RESULT: the RuiNelson workflow is `PARTIAL_TRANSFERABILITY`; IDA assembly and
Pancakes are `SPECIALIZED_ONLY`; programmer style is `NON_DISCRIMINATING`;
binary structure is only `PARTIAL`; sound-driver lineage is unresolved and
`SPECIALIZED_ONLY`.
LIMITS: no inspected public project demonstrates exact ROM reassembly; no ROM,
asset, IDA database, generated assembly or external source was imported.
EVIDENCE: docs/reports/COMPARATIVE_DISASSEMBLY_METHOD_TRANSFER_M11_31.md
EXACT NEXT ACTION: M11.32 may run one bounded Beyond Oasis pass using the
adapted evidence workflow. Do not import SoR-specific code, expand coverage,
investigate ID3, repair 0x3820 timing, or add production emulator dependencies.

## Historical task context

TASK: M11.18 Native Controlled Screen Vertical Slice
WHY: prove input -> native fixed-step runtime -> movement -> terrain collision -> software
framebuffer -> visible window without expanding reconstructed-source or RE tooling.
CURRENT MILESTONE: M11.18 native controlled screen vertical slice; M12 remains TODO.
SLICE MODE: NATIVE_RUNTIME
STATUS: NATIVE_VERTICAL_SLICE_PARTIAL
BASELINE: M11.17 structured-data classification and exact canonical USA ROM identity.

ACCEPTANCE: interactive Windows window path, four-way/diagonal input and focus release;
synthetic visible terrain/player fixture; deterministic fixed timestep; 600-frame replay;
framebuffer hash; preserved graphics tests; canonical ROM rejection; no runtime RE/emulator/
assembler dependency.

CURRENT RESULT: core movement/collision and software rasterization are implemented and tested.
The Windows adapter is compiled through MSVC. Non-Windows GUI presentation and manual
end-to-end visual/input observation remain bounded limitations.

EXACT NEXT ACTION: recommendation D — fix the remaining vertical-slice blocker. Do not
implement the next step here.


HISTORICAL CHECKPOINTS:

TASK: M11.6.2 Static Translation Trust Repair
WHY: repair the three bounded PoC defects that made the earlier static B/C
evidence unreliable: A8DA memory operands, CCR X semantics and Release test
assertion coverage.
CURRENT MILESTONE: M11.6 static translation trust repair
SLICE MODE: RE_TOOLING_ONLY
STATUS: STATIC_TRANSLATION_TRUST_RESTORED

BASELINE: synchronized `main` and `origin/main` at `a2bc039`.

ROM TRUTH: the canonical decoder confirms A8DA as
`CMPI.W #$50,D5; BCC.S 0xA8EE; ADDQ.W #1,D5; MOVE.W D2,(A5)+;
MOVE.W D5,D0; ADD.W D4,D0; MOVE.W D0,(A5)+; MOVE.W D3,(A5)+;
MOVE.W D1,(A5)+; RTS`. The normal path makes four ordered word writes and
advances A5 by 8; the BCC early path executes only the compare and branch.
`0x62CC` remains the six-instruction leaf `MOVEQ #0,D0; MOVE.L D0,0x4E(A6);
MOVE.L D0,0x52(A6); MOVE.W #0,0x2A(A6); MOVE.W #0,0x04(A6); RTS`.

REPAIRS: `mechanical_A8DA` now uses `BoundedMemory`, performs the four real
postincrement word writes and preserves word-operation upper halves. Narrow
CCR helpers model X as bit 4: MOVE/MOVEQ/CMPI preserve X, while ADD/ADDQ set
X together with carry. `mechanical_62CC` preserves X and applies the correct
MOVEQ/MOVE.L/MOVE.W flags. Success status is neutral `EXECUTED`; external
comparisons/tests own the VERIFIED conclusion. The static translation test is
now in the Release `-UNDEBUG` list and the MinGW runtime-path test list.

OLD EVIDENCE INVALIDATED: the earlier M11.6 Case B fixture was not a valid
oracle. It compared empty memory and accepted register results from the wrong
operand model, so it did not detect the missing `(A5)+` writes. It is replaced
by an independent instruction-derived fixture with non-zero memory/register
upper halves, four exact writes, A5 delta, D0/D5 and CCR/X assertions.

VALIDATION: MinGW Debug and Release builds and CTest pass 33/33; Release
compile output contains `-UNDEBUG` for the static translation test. The 0x3820
positive control remains green. GitHub Actions run `34026330082` completed
successfully. MSVC is unavailable on this host; that limitation is recorded in
the final worklog entry.

DECISION: STATIC_TRANSLATION_TRUST_RESTORED.
EXACT NEXT ACTION: recommendation C — expand independent machine-semantics
reference tests. Do not implement that next step here.
DO_NOT_WORK_ON: production runtime, recompiler, interpreter, CPU emulator,
BizHawk reachability, ant/scenario work, new translated routines or M12.

TASK: M11.8 Natural Reachability Recovery for `0x62CC`
WHY: recover one natural runtime caller for the target or advance the root
cause beyond M11.7 with a concrete blocker and a reproducible next experiment.
CURRENT MILESTONE: M11.8 natural reachability recovery
SLICE MODE: RE_TOOLING_ONLY
STATUS: ROOT_CAUSE_ADVANCED

RESULT: a new hardware-reset scenario was added at
`src/tools/re_bizhawk_m11_8_natural_scenario.txt`. It contains 25 natural
input events covering title/start, movement, attack/use and interaction/room
phases, all 33 static direct incoming sites, the known player/event owners,
and 21 RAM state bytes. The enhanced BizHawk probe records the exact target
hooks, bounded register snapshots, input schedule, frame-boundary PC and
per-frame RAM samples without writing emulator memory, registers, flags, ROM
or savestates.

FINAL RUNTIME RESULT: the canonical USA ROM ran from hardware reset for
1800/1800 frames. The report covered 43 targets and 25 input events. Positive
control `0x3820` hit 13 times and startup/control entry `0x60004` hit 5 times.
`0x62CC` and all 33 direct incoming PCs hit 0 times. The sampled frame PC
stream remained concentrated in the startup/system transition helpers
`0x32EE/0x32F4` and `0x3A8E8/0x3A8EE`, while watched RAM values changed; this
is evidence of a live pre-game transition path, not a frozen zero-RAM run.
No branch outcome or CCR value at an unexecuted `0x62CC` predecessor is
claimed.

ROOT CAUSE ADVANCED: the blocker is narrowed from M11.7's generic
`CALLER_NOT_REACHED` to a missing natural transition from the startup/system
layer into the player/event owner band. The next prepared experiment is a
 bounded timing/hold-input sweep around the observed startup/transition
helpers (`0x6135E`, `0x32EE`, `0x3A8E8`) with exact target hooks
and the already implemented RAM/input/frame capture. This remains evidence
gathering only; no forced PC, RAM, register, CCR, ROM, callback or savestate
operation is allowed.

VALIDATION: the full local build/CTest and CI checks for this checkpoint are
listed in the final worklog entry. No ROM, state, trace or generated report is
tracked.

TASK: M11.7 Single Target Reachability Root-Cause PoC
WHY: determine why the existing natural scenarios do not reach the single
target `0x62CC`, stopping at the first evidence boundary and without forcing
emulator state or creating a new gameplay scenario.
CURRENT MILESTONE: M11.7 bounded single-target reachability root cause
SLICE MODE: RE_TOOLING_ONLY
STATUS: COMPLETE

RESULT: static slices and a target-local ROM reference scan found a valid
`0x62CC` leaf with 33 direct incoming branch/call encodings. The existing
neutral 300-frame and `120:Start` 1800-frame scenarios reached none of the 33
incoming PCs or the target. They also did not reach the gameplay-loop/player
owners `0x8B22`, `0x8B2E`, `0x557A`, `0x59B8`, `0x61F6`, `0x62E4` or the
event owner `0x7B2A`. Root cause is `CALLER_NOT_REACHED`, not a proven branch
condition failure, writer failure or translation failure.

MINIMUM NATURAL REQUIREMENT: naturally reach one target-owned caller; the
player alternatives require `0x85E2` to return CCR.C=0 at `0x61FE`/`0x62EC`,
while the event alternative requires D0=`0x01FF` at `0x7B3C`.
EXACT NEXT ACTION: recommendation A — build one minimal natural scenario that
causes the missing state. Do not implement it in this checkpoint.

VALIDATION: fresh GNU/MinGW Debug and Release builds succeeded; CTest passed
33/33 in both configurations after adding the active MinGW DLL directory to
the test-process PATH. The previous `libgcc_s_seh-1.dll`/`libstdc++-6.dll`
dialogs were an environment-path issue, not a project failure. File-limit,
artifact hygiene and `git diff --check` passed. GitHub Actions run
`34021104372` for commit `6a48232` completed successfully.

TASK: M11.6.1 Runtime Capture Fixup for Static Translation PoC
WHY: attempt natural BizHawk runtime captures for the already-selected `0xA8DA`
and `0x62CC` leaves before treating their static fixtures as runtime evidence.
CURRENT MILESTONE: M11.6.1 bounded runtime capture
SLICE MODE: RE_TOOLING_ONLY
STATUS: COMPLETE

RUNTIME RESULT: the existing hardware-reset scenario and canonical USA ROM were
used without forced PC, register, flag, RAM, ROM or savestate mutation. A
developer-only target override was added to observe exactly `0xA8DA` and
`0x62CC`. Neutral input ran for 300 frames and the existing `120:Start`
scenario ran for 1800 frames; both reported zero hits for both targets, with
`target_reached=false` and no entry snapshot. Therefore both routines are
recorded as `RUNTIME_CAPTURE_UNAVAILABLE_EXISTING_SCENARIOS`, not a translation
mismatch. No runtime register/memory capture, replay comparison or new natural
scenario was fabricated.

DECISION: no one of the three runtime decision predicates applies: the task
defines `STATIC_TRANSLATION_RUNTIME_PARTIAL` only when at least one B/C capture
is confirmed, but neither B nor C was reached in the existing scenarios. This
is recorded as `RUNTIME_CAPTURE_UNAVAILABLE_EXISTING_SCENARIOS`, not as a
translation failure.
EXACT NEXT ACTION: recommendation B — first expand natural gameplay scenarios
in a separately authorized task, then repeat only this bounded capture. Do not
implement a translator, interpreter, production CPU model or M12 work here.

VALIDATION: the pre-capture M11.5/M11.6 commits were pushed as focused commits
`a02e6b4` and `6e56c06`; MSVC Debug/Release and GNU/MinGW-equivalent CTest
passed 33/33, and GitHub Actions run `34019056461` passed for `6e56c06`.
The capture reports were emitted successfully; they contain only zero-hit
reachability evidence for B/C and no runtime fixtures.

TASK: M11.6 Verified Static Translation PoC
WHY: measure whether three bounded, evidence-backed routines can be emitted as
ordinary C++ and differentially checked without introducing a runtime 68000
interpreter or production CPU emulator.
CURRENT MILESTONE: M11.6 verified static translation experiment
SLICE MODE: RE_TOOLING_ONLY
STATUS: COMPLETE

CASES: A=`0x3820` decompressor; B=`0xA8DA` clean arithmetic leaf from the
current mass/explorer output; C=`0x62CC` bounded RAM-state leaf from the same
output. A uses the two existing USA-ROM vectors. B and C use small normalized
state fixtures whose opcode bytes, instruction counts and bounds are anchored
to the current ROM/mass evidence; no ROM, trace or savestate is committed.

RESULT: A passed two-way C++ differential verification against the existing
native decompressor on both local USA-ROM vectors (`1217 -> 3072` and
`112 -> 128`), whose known original-ROM hashes remain the behavioral oracle.
B passed two normalized static-state fixtures including the early branch. C
passed one normalized RAM-state fixture with four ordered writes. These B/C
fixtures are not BizHawk runtime captures; they are bounded expected states
anchored to the current ROM bytes and mass/explorer evidence. The comparator reports the
first differing register/CCR/write/byte, and an unsupported opcode returns an
explicit STOP status. No fallback interpreter, PC dispatcher, production CPU
emulator or production runtime change was added.

METRICS: three routines; three verified slices; one explicit unsupported
fixture; generated/mechanical implementation 297 LOC plus 77 LOC interface;
handwritten fixups 0 LOC by the experiment definition; 306/10/6 original
instructions for A/B/C; attempted/passed vectors 2/2 for A, 2/2 for B and
1/1 for C; state mismatches 0 in verification; approximate manual work 3-4
hours; first verified result approximately 2 hours from implementation start.
The 297 LOC includes the bounded stream helpers and comparison support, so the
experiment does not claim that all of it would be emitted by a future compiler.

DECISION: `STATIC_TRANSLATION_POC_NEEDS_FIXUPS`. The bounded approach is
useful for small known slices, but the A implementation still contains
mechanically shaped helper code and B/C use captured fixtures rather than a
general compiler frontend. EXACT NEXT ACTION: choose recommendation C — use
mechanical translation only as a verification aid. Do not implement it here.

VALIDATION: MSVC Debug and Release builds passed; full CTest passed 33/33 in
both configurations. MinGW/GNU-equivalent build and CTest passed 33/33 after
the configured MinGW `bin` directory was supplied on the process PATH (the
first attempt reproduced the visible missing `libstdc++-6.dll` launcher error).
File-limit, artifact-hygiene and `git diff --check` passed.

HISTORICAL CHECKPOINTS:

TASK: M11.5 Ant Reachability Diagnostic PoC v1
WHY: determine whether existing deterministic scenarios can reach ten selected
unresolved `INDIRECT_FLOW` source PCs before spending ant target-resolution cost.
CURRENT MILESTONE: M11.5 reachability diagnostic
SLICE MODE: RE_TOOLING_ONLY
STATUS: COMPLETE

RESULT: two existing scenarios were checked in two batched BizHawk runs. All
ten frontier contexts were `NOT_REACHED`; static bytes and frontier classes
were valid. The known-positive `0x045A -> 0x307A` natural control resolved at
frame 113 in 104103 ms, proving the C: ROM/emulator/PC-hook environment works.
No sampled frontier had a matched scenario, so no sampled ant retest or merge
was attempted and no dynamic target was invented.

DECISION: `SCENARIO_COVERAGE_INSUFFICIENT`.
EXACT NEXT ACTION: choose recommendation B — create a small bounded set of new
natural gameplay scenarios. Do not implement that recommendation in this task.

HISTORICAL CHECKPOINTS:

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
VALIDATION: local Debug/Release/GNU-equivalent CTest 32/32 passed; GitHub
Actions CI run `33963041522` for implementation commit `cb1c78b` completed
successfully. WSL has no installed Linux distribution, so native Linux
validation remains unavailable.
EXACT NEXT ACTION: hard stop. Do not schedule another frontier job, add
parallelism, or begin M12.

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
