# Development Worklog
Chronological record of meaningful project actions. New entries go at the top.

Each task records objective, actions, evidence, tests, result, unresolved questions and exact next step.

## 2026-09-04 — M11.5 CI portability follow-up
TASK: address the first Linux CI failure without changing the mass-verification
measurement or its serialized output.
WHY: the initial implementation passed the local Debug, Release and
GNU/LLVM-MinGW gates but GitHub Actions failed during its Ubuntu build step.
IMPLEMENTATION: added direct standard-library includes for the formatter's
`std::size` call, the mass verifier's optional/pair/iterator/string use, and
the test's `std::invalid_argument` catch. No algorithm, classification, report
field or production target changed.
TESTS/VALIDATION: Debug, Release and GNU/LLVM-MinGW builds plus full CTest
29/29 passed locally after the include-only fixes. Ubuntu CI identified the
missing `<stdexcept>` declaration in the test; its corrected follow-up result
is pending. WSL/Linux remains unavailable locally.
STATUS: READY FOR CI RECHECK.
EXACT NEXT ACTION: push this focused portability fix and wait for the Ubuntu
workflow result before declaring the checkpoint complete.

## 2026-09-04 — M11.5 Mass Structural Verification Pass v1 completed
TASK: batch-verify every normalized candidate-map entry with the existing
project-specific bounded decoder and Atlas evidence, cluster blockers and
estimate the highest-value systemic follow-ups. Production C++ and gameplay
semantics were out of scope.
WHY: replace candidate-by-candidate triage with one deterministic measurement
pass while preserving Ghidra as a discovery layer and `oasis_re`/runtime as
verification authority.
CURRENT MILESTONE: M11.5 post-M11 evidence tooling.
MILESTONE UNDERSTANDING CONFIDENCE: 95%.
CURRENT SLICE UNDERSTANDING CONFIDENCE: 94% for bounded decode, structural
classification, clustering and deterministic serialization; semantic
confidence remains intentionally unclaimed.
SLICE CONFIDENCE EVIDENCE: existing `oasis.m68k.re-candidate-map.v1` model,
bounded `re_slice_decoder`, Atlas typed data/code intervals, prior Beta and
dynamic evidence fields, and the fixed 11-entry control set.
ACCEPTANCE: process 534 entries in address order; record decode/reachable-flow,
boundary and overlap signals; preserve confirmed anchors; classify and cluster
failures; provide leaf plus GHIDRA_ONLY/STATIC_SUPPORTED breakdowns; estimate
the top three fix classes; add synthetic validation; repeat JSON/text runs.
EVIDENCE AVAILABLE: canonical USA ROM SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`, existing
normalized candidate-map output, Atlas and Beta/reference artifacts under the
ignored build workspace. The raw Ghidra v3 export itself is absent from this
checkout.
KNOWN UNKNOWNS: Ghidra boundaries and indirect targets remain unverified;
runtime semantics remain out of scope; Linux/WSL validation is unavailable in
this environment.
IMPLEMENTATION: added developer-only `oasis_re_mass_verify` with reusable
batch model/API, deterministic JSON/text formatting and local CLI. It checks
bounded entry decode, reachable instruction/block counts, RTS/RTE and known
transfer terminals, direct BSR/JSR/vector/static signals, boundary categories,
Atlas data/code overlap, existing Beta/dynamic flags and non-exclusive failure
clusters. Added precise BSR/JSR/vector source flags to candidate-map records.
Added synthetic tests for categories, leaf accounting, controls,
deterministic output, malformed empty input and duplicate candidates. No
production target changed.
RESULT: the local 534-entry measurement processed previous classes 483
GHIDRA_ONLY, 39 STATIC_SUPPORTED, 11 CONFIRMED and 1 CONFLICT. Leaves: 234
total, 208 clean, 7 unsupported, 13 indirect-flow, 6 boundary-conflict and
0 terminal failures. Largest clusters: boundary-longer-than-Ghidra 185,
multiple-entry-overlap 82 and unsupported-opcode 54. Top fix classes: boundary
continuation 277 affected, decoder coverage 61 and static edge recovery 51.
Six confirmed control entries were heuristic misses and five passed; none was
downgraded. Because the raw Ghidra export was absent, this run reconstructed
the Ghidra-shaped input from prior normalized evidence and is not an
independent re-export.
DETERMINISM: JSON SHA-256
`1F5B31C2789D62752FC6CE0F8E5977357914573116965FCA96A02DA55FA296BC` and text
SHA-256 `E1F9E895E245BFB6A504B1D34891D2E12B5CE2F6A04F43D848BA37E87A062BAF`
matched between runs A/B. Measured wall-clock durations were 13385 ms and
13434 ms and are intentionally excluded from serialized identity.
TESTS/VALIDATION: Debug full CTest 29/29, Release full CTest 29/29 and
GNU/LLVM-MinGW full CTest 29/29 passed; file-limit and `git diff --check`
passed. WSL/Linux was unavailable because WSL is not installed. No ROM,
Ghidra project/database, emulator, savestate, raw trace or commercial asset
was added to Git.
STATUS: DONE for the measurement checkpoint.
EXACT NEXT ACTION: restore the raw Ghidra export and rerun independently;
then choose exactly one systemic fix from the measured top-three list. STOP;
do not start M12, semantic translation, leaf translation or broad tracing.

## 2026-09-04 — M11.5 Ghidra-to-Atlas candidate integration completed
TASK: normalize the deterministic external Ghidra map, merge it with the
existing ROM Atlas and reuse already recorded static, dynamic and beta
evidence to produce a conservative ranked candidate set. No new ROM-wide
scanner, emulator trace, production C++ or gameplay behavior is in scope.
WHY: make Ghidra's wide structural discovery useful for selecting the next
bounded reverse-engineering target without promoting Ghidra hypotheses to
project truth.
CURRENT MILESTONE: M11.5 post-M11 evidence tooling.
MILESTONE UNDERSTANDING CONFIDENCE: 95%.
CURRENT SLICE UNDERSTANDING CONFIDENCE: 94% for the normalized merge,
classification and deterministic scoring contract; semantic confidence is not
claimed for Ghidra-only records.
SLICE CONFIDENCE EVIDENCE: the external Ghidra export is deterministic and
schema-validated; Atlas exposes existing static/beta/dynamic evidence; the
project ledger fixes the 11-entry benchmark and natural BizHawk observations.
ACCEPTANCE: parse the prior export with malformed-input rejection; merge
duplicate/function/candidate records deterministically; preserve all existing
confirmed benchmark entries; classify each normalized entry as CONFIRMED,
STATIC_SUPPORTED, DYNAMIC_OBSERVED, GHIDRA_ONLY or CONFLICT; apply a documented
explainable score with address tie-breaking; emit full JSON and top-20 text;
cover merge priority, conflicts, overlays, ranking and serialization with
synthetic CI-safe tests; repeat the real report and compare hashes.
EVIDENCE AVAILABLE: external v3 Ghidra JSON, current Atlas and bounded Beta
comparison, existing Atlas dynamic A6A4 scenario, and documented BizHawk
natural caller observations at 0x60B8C/0x611EE/0x6121A.
KNOWN UNKNOWNS: Ghidra boundary quality, indirect-flow target recovery,
unobserved runtime callers and all routine semantics remain unknown.
IMPLEMENTATION: added developer-only `oasis_re_candidate_map` with strict
Ghidra JSON parsing, duplicate merge, conservative classification, explainable
ranking and full/top-report serialization. Added synthetic tests and registered
the tooling in CMake; no production target or gameplay code changed.
RESULT: the real USA/Beta merge produced 534 unique entries: 11 CONFIRMED, 39
STATIC_SUPPORTED, 0 DYNAMIC_OBSERVED, 483 GHIDRA_ONLY and 1 CONFLICT. Complexity
counts are LEAF 234, SHALLOW 172, COMPLEX 90 and UNKNOWN 38. Existing dynamic
facts remain flags when stronger static/project evidence determines the class.
The highest-ranked new bounded Ghidra function is `0x611EA` with shallow
structure, existing static support and no recorded conflict.
DETERMINISM: candidate JSON SHA-256
`5C17F6A735DC715B18CD5A5E8FA34F876CAD5CEB5D4511C80547E2C8720A22AC` and human
report SHA-256
`9E5A5884FE05B816C0265535529E79E2C336FBB4CE176D60AF57F522DAFD9C84` matched
between fresh runs A/B. External Ghidra v3 input SHA-256 is
`613A3AA6DEB8D2DCF994C82ADC6A6939B7D5F27AF67A51D05C16F090D60A5315`.
TESTS/VALIDATION: clean Debug, Release and GNU/MinGW-equivalent builds and
full CTest passed 28/28 in each configuration; parser/merge tests passed;
JSON was parsed independently; file-limit, `git diff --check` and tracked
sensitive-artifact checks passed. No ROM, Ghidra project, emulator, savestate
or raw trace was added.
CI: GitHub Actions run `33887363813` for implementation commit `aa514db`
completed successfully.
UNKNOWN/NEXT: Ghidra boundary correctness, indirect targets, unobserved
callers and semantics remain UNKNOWN. STOP at this bounded checkpoint; the
next task may inspect `0x611EA` but must not begin it here.

## 2026-09-04 — M11.5 Ghidra ROM Mapping PoC — COMPLETE WITH FIXUPS DECISION
TASK: run the bounded Ghidra mapping PoC against the supplied canonical USA
ROM and make exactly one A/B/C usefulness decision. Production C++ and M12
were out of scope.
ACCEPTANCE: verify the requested ROM fingerprint; use only official Ghidra;
run conservative raw-binary M68000 auto-analysis; export Ghidra functions,
candidate sources, direct calls, vector-derived targets, unresolved-call
field, benchmarks and a bounded false-positive sample; repeat the export;
run the required repository validation; stop after the decision.
ACTIONS: verified `D:\Proect\Github\Sega-Thor\local-roms\Beyond Oasis (USA).md`
is 3145728 bytes with SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`. The
official NSA GitHub release Ghidra 12.1.3 (`Ghidra_12.1.3_build`) and official
supported Temurin JDK 21.0.12.1+1 remain installed only under
`C:\Users\Serjio\Tools\Sega-Thor-Ghidra`, outside the repository. Because the
Windows Ghidra wrapper rejected the supplied `.md` suffix before import, a
developer-only external copy was used at `BeyondOasisUSA.bin`; its size and
SHA-256 were reverified identical. No copy was placed in a tracked path.
The raw BinaryLoader used base `0x000000`, language
`68000:BE:32:default`, compiler `default`, normal headless auto-analysis,
`OasisGhidraMap`, and `-max-cpu 1`; no manual labels or custom whole-ROM
scanner were used.
EVIDENCE: schema `oasis.m68k.ghidra-map.v1`; 496 Ghidra functions, 438
candidates (432 recognized functions plus 6 vector/direct-call candidates),
364 direct BSR targets, 106 direct JSR targets, 6 vector-derived targets and
0 exported unresolved call targets. Entry benchmark: 7
`EXACT_FUNCTION_MATCH`, 1 `WRONG_BOUNDARY`, 1 `CODE_ONLY`, 2
`MISSED_ENTRY`; code presence is 9/11 (81.8%) and exact function match is 7/11
(63.6%). Call-edge benchmark is 4/6 (66.7%). Data checks: all four addresses
were non-code with useful xrefs; only `0xC92C` was recognized as defined data,
while `0x5CE96`, `0x96E8` and `0x96F8` remained undefined but xref-bearing.
At `0xA7E2`, Ghidra listed `jmp`, did not mark the flow indirect and exposed
no target. The bounded 20-entry sample contained 19 `LIKELY_CODE` and one
`AMBIGUOUS` item (`0x020E`).
REPEATABILITY: two fresh external projects and exports were byte-identical;
both JSON files are 390972 bytes with SHA-256
`613A3AA6DEB8D2DCF994C82ADC6A6939B7D5F27AF67A51D05C16F090D60A5315`.
RESULT/DECISION: `GHIDRA_USEFUL_WITH_PROJECT_FIXUPS` (option B). Ghidra is a
useful broad structural discovery layer, but the missed entries, one known
boundary mismatch, incomplete data typing and unresolved indirect flow make
it unsuitable as the authority. Keep `oasis_re`, ROM oracles and runtime
captures as verification authority. No production code, ROM, Ghidra project,
database, raw disassembly or generated commercial-data report was committed.
TESTS/VALIDATION: Debug, Release and GNU/MinGW-equivalent builds/tests passed
27/27; project file-limit checks passed; `git diff --check` and tracked-file
sensitive-artifact checks passed. The JSON was parsed with PowerShell
`ConvertFrom-Json`. Existing CI run `33882125335` for the exporter commit was
green; no new production target was introduced.
UNKNOWN/NEXT: Ghidra's warnings on several decompilations and the lack of
semantic confidence are recorded limitations. STOP at M11.5; do not begin M12.

## 2026-09-04 — M11.5 Ghidra ROM Mapping PoC availability gate — ROM BLOCKED
TASK: determine whether a local Ghidra M68000 analyzer is available for the
bounded discovery-layer experiment described by the user.
WHY: the checkpoint requires measuring Ghidra's own conservative auto-analysis
of the canonical USA ROM; it explicitly forbids replacing Ghidra with a new
whole-ROM scanner or installing it through CI.
CURRENT MILESTONE: M11.5 post-M11 evidence tooling.
MILESTONE UNDERSTANDING CONFIDENCE: 95%.
CURRENT SLICE UNDERSTANDING CONFIDENCE: 100% for the availability gate; no
analysis confidence is claimed because Ghidra was unavailable.
ACCEPTANCE: check local GUI/headless availability and version without fetching
unofficial software; if available, create an ignored local project, verify the
USA fingerprint, run one conservative baseline and export the required
structured map; if unavailable, document the blocker and stop.
ACTIONS: synchronized with `git fetch`; verified clean `main` and
`origin/main` at `5aeb338c8d8be86f1de0178815abdfd00e6db890`; reviewed the
required project governance, architecture, roadmap, evidence and toolchain
guidance documents. After explicit user authorization, downloaded the
official NSA GitHub release Ghidra 12.1.3 (`Ghidra_12.1.3_build`) and verified
SHA-256 `93a5d11a9ad510622acaaf908c556a7b9b764d338e78a7567f3689bf5081fd54`.
Downloaded official supported Temurin JDK 21.0.12.1+1 and verified SHA-256
`f9d6e191ab098c0d416e7d588a24420a8621cd2f4720dab2459b8b7b2d2d8b4e`. Both
are installed only under `C:\\Users\\Serjio\\Tools\\Sega-Thor-Ghidra`, outside
the repository. `support\\analyzeHeadless.bat` runs; the available processor
language is `68000:BE:32:default`.
EVIDENCE: the canonical USA ROM is not present in the workspace or checked
local user roots, and no same-size `3145728`-byte candidate was found. ROM
fingerprint, project creation, analysis results, export counts and benchmark
decision therefore remain UNKNOWN. A developer-only exporter was added at
`src/tools/ghidra/OasisGhidraMap.java`; its Ghidra 12.1.3 headless synthetic
smoke-test passed with `68000:BE:32:default`, but it has not run on the
canonical ROM.
RESULT: the Ghidra availability blocker is resolved, but the PoC is blocked
at the required ROM-input gate. No production C++, oasis_re replacement
scanner, emulator, ROM, Ghidra project/database or generated commercial-data
report was added. `docs/FILE_MAP.md` records the new exporter; no reverse-
engineering fact or semantic Atlas entry was changed.
TESTS/VALIDATION: existing Debug, Release and GNU/MinGW-equivalent build
directories rebuilt and full CTest passed 27/27 in each configuration. The
project file-limit test passed again in all three configurations,
`git diff --check` passed, and `git ls-files` found no tracked ROM, archive,
savestate, Ghidra project or decompiler/disassembly dump. The exporter passed
a Ghidra 12.1.3 headless synthetic smoke-test; it has not been run against the
canonical ROM.
CI: GitHub Actions run `33882125335` for implementation commit `3a55b8a`
completed successfully.
UNKNOWN/NEXT: provide the canonical USA ROM locally; then verify size,
CRC32, SHA-1 and SHA-256, create the ignored project, run one conservative
headless analysis/export, perform the required bounded benchmarks and make
one A/B/C decision. Do not begin M12 or implement a replacement scanner.

## 2026-09-04 — M11.5 bounded downstream runtime resolution CI follow-up
RESULT: GitHub Actions CI run `33878595233` (`#326`) for implementation
commit `2351545f74b68fb26e7ea6b146e6bd3467a8f3af` completed successfully.
The docs-only follow-up records the final green remote validation; no code,
scope or runtime evidence changed.
UNKNOWN/NEXT: writer callback width, semantic role and cross-scenario A0
invariance remain UNKNOWN. STOP at this bounded M11.5 checkpoint.

## 2026-09-04 — M11.5 bounded downstream runtime resolution completed
TASK: resolve only the already reached downstream memory operations at
`0x60BFA` and `0x60C08` on frozen `start_pulse_120` / `120:Start`.
WHY: determine whether the observed stack-derived A0 is sufficient for these
two references without inferring global invariance or semantics.
ACCEPTANCE: reverify USA bytes/decoding; capture full BizHawk register state,
A0 and memory at both boundaries; compute effective addresses and address
classes from actual registers; compare two fresh runs; preserve the
scenario-only/global distinction; update the RE ledger and oracle.
EVIDENCE: USA bytes are `16 28 00 01` (`MOVE.B 1(A0),D3`) at `0x60BFA` and
`14 28 00 01` (`MOVE.B 1(A0),D2`) at `0x60C08`. BizHawk 2.11.1 reaches both
at frame 423 with A0=`0x0006F8B0` / `0x0006F8B2`, not the earlier consumed
`0x0006F8AE`. Effective addresses are ROM `0x0006F8B1` / `0x0006F8B3`, with
raw bytes `0x13` / `0x00`.
RESULT: both targets are `runtime_resolved_for_scenario`; neither is
`static_global`. Fresh A/B JSON and human reports are byte-identical with
hashes `CF092C8B91BD2FDA858E3E165A75D3A891F8B90997D6F3E839A65FD053C97D91`
and `34717649BB6DA2C389180A994DE226715F51739036A742BDCDD6B573B7FDE0C4`.
The two target rechecks were the relevant previous unresolved count: two are
scenario-resolved, zero globally resolved. No recursive writer chase or M12
work was started.
UNKNOWN/NEXT: A0 invariance across other paths, writer callback width and
semantic meaning remain UNKNOWN. VALIDATION: Debug, Release and GNU/MinGW
builds and sequential full CTest are 27/27 in each configuration; all three
USA stack-provenance oracles pass; BizHawk A/B JSON and text captures match;
file-limit, `git diff --check` and tracked-ROM/emulator hygiene pass.
NEXT: commit/push, verify GitHub CI and then STOP at this bounded checkpoint.

## 2026-09-04 — M11.5 bounded runtime stack-value provenance CI follow-up
RESULT: GitHub Actions CI run `33874638457` for implementation commit
`ea14f898d93f3508b877ce1f059f7926cfebe2cd` completed successfully. The focused
checkpoint is synchronized to GitHub; no code or scope change was made.
UNKNOWN/NEXT: BizHawk writer callback width and scenario-to-scenario invariance
remain UNKNOWN. Push the documentation result if needed, verify final
`main==origin/main` and clean status, then STOP.

## 2026-09-04 — M11.5 bounded runtime stack-value provenance completed
TASK/SCOPE: capture only the runtime stack chain on the already proven
`start_pulse_120` / `120:Start` hardware-reset path. No `0x60D4A` search,
generic tracer, emulator, interpreter, production behavior, ABI inference,
semantic naming or M12 work.
IMPLEMENTATION: added developer-only
`src/tools/re_bizhawk_stack_provenance.lua`, schema
`oasis.m68k.re-stack-runtime-provenance.v1`; extended the existing local USA
oracle with stack mode, exact writer bytes and raw target boundary checks.
The probe watches only the requested PCs and optional concrete range `[P,P+4)`.
EVIDENCE: BizHawk 2.11.1 with canonical USA ROM
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263` reaches
`0x60B8C`, `0x6121A`, `0x60B90`, `0x60BCC`, `0x604BC`, `0x604E4`, `0x60BD0`,
`0x60BFA` and `0x60C08`. Raw values are caller A7=P=`0x00FF0BA8`,
`memory[P]=0x0006F8AE`, callee entry A7=P-4=`0x00FF0BA4` with return slot
`0x00060BD0`, RTS restoration to P, then `0x60BD0` consume into A0
`0x0006F8AE` and A7=`0x00FF0BAC`.
WRITER: USA bytes `0x60B66=2F 08`; BizHawk concrete-range callbacks correlate
PC `0x60B68` to static instruction `0x60B66` and observe `P+2=0x0000F8AE`
then `P=0x00000006`. Callback width is UNKNOWN; reconstructed final value is
raw evidence only. MAME was not used because the bounded BizHawk evidence
identified the static writer and ordered concrete writes without expanding
scope.
DETERMINISM: fresh A/B JSON SHA-256 is
`EFB30EEBF3EE0CEE929A02075088D684A2900B0DAFA192BC00390E484A846D0D`; human
report SHA-256 is
`4239B4182758FAEA49C56524953632C7568A52C508EF4E5BFFDCE6995872F7AC`; both
match byte-for-byte.
TESTS: Debug, Release and GNU/MinGW-equivalent full CTest 27/27; stack and
prior natural USA oracles pass; file-limit and `git diff --check` pass. No ROM,
emulator binary, savestate or raw trace is tracked. CI was pending at the time
of this entry and is recorded as successful in the follow-up entry above.
RESULT/UNKNOWN: the previous scenario path had `memory[P]` UNKNOWN; it is now
concrete only for this controlled runtime scenario. Writer callback width,
cross-scenario invariance, full instruction trace, downstream references and
semantic role remain UNKNOWN. CURRENT SLICE UNDERSTANDING CONFIDENCE: 96% for
the raw chain, lower for any interpretation. NEXT: commit, push, verify GitHub
CI and final `main==origin/main`, then STOP.

## 2026-09-04 — M11.5 bounded runtime stack-value provenance started
TASK/SCOPE: use exactly the frozen successful BizHawk scenario to test whether
runtime evidence can identify the value consumed by `0x60BD0 MOVEA.L (A7)+,A0`
after `0x60BCC`; keep tooling separate from the native runtime.
KNOWN/UNKNOWN: static evidence distinguishes the BSR return slot at P-4 from
memory[P], but the runtime P, value, writer and post-consume state are UNKNOWN
at task start. ACCEPTANCE: capture the bounded path, concrete stack value,
callee mechanics, concrete-range writer evidence, deterministic A/B output,
USA oracle and local validation without introducing a general stack/emulator.
NEXT: run the narrow probe, compare A/B, document raw evidence or bounded
failure, then stop.

## 2026-09-04 — M11.5 bounded natural reachability search started
TASK/SCOPE: search only for the first deterministic natural reset-to-input
execution of the already proven direct callers `0x60B8C` or `0x60D4A`.
Reuse the existing BizHawk natural probe and frozen USA ROM scenario; do not
add a trace framework, emulator, autoplay, production behavior, deeper callee
analysis, whole-ROM search or M12 work.
CURRENT MILESTONE: M11.5 follow-up under completed M11
MILESTONE UNDERSTANDING CONFIDENCE: 95%
CURRENT SLICE UNDERSTANDING CONFIDENCE: 78%; the natural input sequence and
whether either caller is reachable from hardware reset are UNKNOWN. Until a
caller is observed, this task remains RE/probe/documentation only.
ACCEPTANCE: test at most twelve structured raw controller variants, with a
maximum horizon of 1800 frames per variant; stop at the first primary caller
hit. Record exact caller/target snapshots, watched context, bounded stack
window `[A7-0x20,A7+0x40)`, return-address evidence where observed and a
repeatability comparison. If no caller is observed, record every tested
variant and the closest bounded result without inventing reachability.
KNOWN/UNKNOWN: static direct edges to `0x6121A` from `0x60B8C`, `0x60D4A` and
`0x611EE` are proven. Frozen neutral execution reaches only the `0x611EE`
caller path so far; whether input can reach the two requested callers,
whether `0x6121A` follows them, and any deeper runtime state are UNKNOWN.
NEXT: add the minimal search-mode stop/report fields to the existing Lua
probe, run the structured variants from hardware reset, then stop on success
or bounded NOT OBSERVED and document the evidence.
## 2026-09-04 — M11.5 bounded natural reachability search completed
TASK/SCOPE: bounded natural reset-to-input search for the two already proven
direct callers `0x60B8C` and `0x60D4A`; stop after the first primary hit. No
whole-game search, emulator work, production runtime, semantic interpretation
or M12 work.
IMPLEMENTATION: added only additive search-mode metadata and stop/report
fields to the existing developer-only BizHawk Lua probe. Search mode watches
`0x60B8C`, `0x60D4A`, `0x6121A` and `0x611EE`, captures the requested
`[A7-0x20,A7+0x40)` window and preserves exact caller/target hook evidence.
The existing frozen neutral scenario and report schema remain compatible.
PROVEN: canonical USA ROM SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`, BizHawk
2.11.1, hardware reset and one raw `Start` pulse at frame 120 reach
`0x60B8C` at frame 423 after 424 frame advances. Watched counts are
`0x60B8C=3`, `0x60D4A=0`, `0x6121A=5`, `0x611EE=2`; a downstream
`0x6121A` event is paired with the raw caller and matching BSR return address
`0x60B90`. The first caller A7 is `0x00FF0BA8`, and its stack window starts
at `0x00FF0B88`.
MINIMIZATION/DETERMINISM: neutral baseline did not reach either requested
caller. One-event pulses at frames 119 and 121 also reached `0x60B8C` at
frame 423, so the successful sequence is minimal by event/button count within
the bounded check. Two fresh frame-120 runs are byte-identical: JSON SHA-256
`20AA010BAECFE696A119D431A7EE6562074848219DD9C08A16D00BE3BBD994F2`, trace
SHA-256 `66F0095A195A9899789F08D0D4E8C5CF45EEFDDEE97EEE14B8A24516A9FB2271`.
The report has 438 ordered sampled/hook events; it is not a complete
instruction or basic-block trace.
TESTS: Debug, Release and GNU/MinGW-equivalent CTest 27/27 passed; frozen USA
natural oracle and caller-search USA oracle passed; exact BizHawk replay and
hash comparison passed; file-limit and `git diff --check` passed. No ROM,
trace, emulator binary or generated artifact is tracked.
CI: GitHub Actions run `33871898019` for implementation commit `81e3642`
completed successfully. Final docs synchronization commit `df9791c` was also
validated by GitHub Actions run `33871984433`, completed successfully.
RESULT: bounded natural reachability for `0x60B8C` is confirmed; `0x60D4A`
remains unobserved. Input meaning, complete execution trace, callee effects,
broader reachability and semantic roles remain UNKNOWN. CURRENT SLICE
UNDERSTANDING CONFIDENCE: 96% for the observed raw caller/downstream path;
not a claim about the unobserved caller or routine semantics.
NEXT: commit this focused checkpoint, push only after the local pre-push gate,
verify GitHub CI, then STOP. Do not begin broader input search or M12.
## 2026-09-04 — M11.5 bounded dynamic caller discrimination completed
TASK/SCOPE: reuse only frozen `natural_idle_to_6121a_v1` to discriminate the
three known direct callers of `0x6121A`; no input changes, autoplay, callee
analysis, ABI inference, stack-writer tracing, production behavior or M12.
IMPLEMENTATION: extended the existing developer-only BizHawk natural Lua
probe with exact caller/target hooks, ordered watched events, full D0-D7/A0-A7/
SR snapshots, `[A7-0x10,A7+0x20)` windows, raw target-entry stack longword,
return-address comparison and raw register delta. Extended the existing
natural report additively; no new general trace framework was added.
STATIC EVIDENCE: USA oracle verifies `0x60B8C=61 00 06 8C` -> return
`0x60B90`, `0x60D4A=61 00 04 CE` -> `0x60D4E`, and
`0x611EE=61 00 00 2A` -> `0x611F2`; all are four-byte `BSR.W` to `0x6121A`.
PROVEN: both frame-113 target hits pair directly with `0x611EE`:
`113->114` and `115->116`. Caller/entry A7 are
`00FF0BE6->00FF0BE2` and `00FF0BAC->00FF0BA8`, both delta `-4`. The raw
longword at target entry is `0x000611F2` for both and matches the expected
return address. Register delta contains only A7; D0-D7/A0-A6/SR are unchanged.
The two pairs are observed as distinct events; whether the second is re-entry
is UNKNOWN. This is not relevant to the existing `0x60B8C`/`0x60D4A` blocker.
DETERMINISM: two fresh identical runs have byte-identical caller reports and
traces. Existing importer result: 117 events, 23 unique PCs, 0 inferred
blocks, hash `0x52F951E69F5A7100`, and zero inferred branch/call/return/read/
write events because the bounded trace has no instruction-size/event typing.
UNKNOWN/LIMITATIONS: BizHawk exposes no separate post-instruction callback;
callee behavior, return state and stack-writer provenance remain unexamined.
CURRENT SLICE CONFIDENCE: 96% for this caller/entry pairing, not for callee
semantics. TESTS: Debug CTest 27/27, Release CTest 27/27 and GNU/MinGW CTest
27/27 passed; local USA oracle passed with exact bytes, pairings, A7/return
values and raw register deltas; two fresh BizHawk runs and normalized imports
are byte-identical; file-limit and `git diff --check` passed; no tracked ROM,
emulator binary, savestate or generated trace exists. NEXT: commit/push this
focused checkpoint, verify GitHub CI, record its result and STOP. GitHub Actions
run `33870143848` for implementation commit `3d1d2793b2319c12b1d5d710bf8e65ebca767052`
completed successfully.
## 2026-09-04 — M11.5 bounded dynamic caller discrimination started
TASK: reuse `natural_idle_to_6121a_v1` to identify which of the three already
confirmed direct call-sites transfers control to `0x6121A` during frame 113.
WHY: the previous natural probe proves target reachability but not the dynamic
predecessor, return-address stack value or call/entry register delta.
CURRENT MILESTONE: M11.5 follow-up under completed M11
MILESTONE UNDERSTANDING CONFIDENCE: 95%
CURRENT SLICE UNDERSTANDING CONFIDENCE: 78%; caller/hook timing and stack
evidence are not yet captured, so this remains RE/probe/documentation only.
SLICE CONFIDENCE EVIDENCE: static USA bytes already prove direct edges from
`0x60B8C`, `0x60D4A` and `0x611EE` to `0x6121A`; BizHawk proves two natural
target hits at frame 113 but its prior report has no exact predecessor.
ACCEPTANCE: watch only the three callers and `0x6121A`; capture ordered full
register snapshots and `[A7-0x10,A7+0x20)` windows; validate BSR return address
and A7 delta; report both target hits; compare two fresh identical replays;
keep CI independent of ROM/emulator and stop after this result. No callee
analysis, stack-writer tracing, autoplay, production behavior or M12.
KNOWN/UNKNOWN: expected return addresses are `0x60B90`, `0x60D4E` and
`0x611F2`, pending exact-byte oracle; dynamic caller, target-entry A7, stack
return longword, register delta and whether duplicate hook hits are distinct
entries are UNKNOWN.
NEXT: add the minimal ordered caller/target event capture to the existing
BizHawk natural probe, run the frozen scenario twice, and record only the
result or an explicit hook-timing limitation.
## 2026-09-04 — M11.5 bounded natural reachability scenario completed
TASK/SCOPE: determine whether natural reset-to-input execution can reach raw
target `0x6121A` using only the existing BizHawk adapter and finite real
controller input. No forced PC/register/memory state, ROM patch, autoplay,
production runtime, emulator dependency, call-clobber analysis or M12 work was
added.
IMPLEMENTATION: added the bounded developer-only
`oasis.m68k.emulator-scenario.v1` parser/JSON model, frozen scenario
`natural_idle_to_6121a_v1`, exact target hooks and the natural report path in
`re_bizhawk_natural_reach.lua`. Added synthetic parser/serialization tests and
the local USA oracle `oasis_re_natural_reference`.
PROVEN: with the canonical USA ROM (SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`), BizHawk
2.11.1 from hardware reset with neutral input reached `0x6121A` at frame 113;
114 frames executed and the exact hook counted two hits. Entry PC is
`0x6121A`, A7 is `0x00FF0BE2`, and the captured stack window starts at
`0x00FF0BC2`. Two fresh runs produced byte-identical JSON and normalized
trace/report output. Static USA bytes also confirm direct callers
`0x60B8C`, `0x60D4A` and `0x611EE` to `0x6121A`.
UNKNOWN/LIMITATIONS: the probe samples PC at frame boundaries and exact target
hooks, so the immediately preceding instruction, dynamic caller, return state
and stack provenance are unknown; `0x6135E` is not treated as caller evidence.
The six secondary watched addresses were not reached. MAME writer provenance
was not repeated because it would not resolve this missing predecessor within
the bounded scope. Current bounded reachability confidence is 95%; no semantic
claim follows from the snapshot.
TESTS: Debug CTest 27/27 passed; Release CTest 27/27 passed; GNU/MinGW
equivalent CTest 27/27 passed; local USA natural oracle passed against the
canonical ROM and report; natural A/B JSON, trace and imported JSON hashes are
equal; `git diff --check` and file-limit passed. The normalized report records
114 events, 22 unique PCs, 0 inferred blocks and 0 inferred branch/call/return/
memory events, as required by its bounded coverage mode. GitHub Actions CI run
`33868387017` completed successfully for implementation commit
`461ddb18d2e2f9618c2253910ae8918983a48c9b`.
NEXT: commit this focused checkpoint, push after the AGENTS pre-push gate is
green, verify GitHub CI, record its result, push the documentation-only CI
update, then STOP and await explicit work.
## 2026-09-04 — M11.5 real external emulator bake-off completed
OBJECTIVE/SCOPE: run only the canonical USA ROM through the user-installed
MAME and BizHawk backends, obtain a bounded deterministic `boot_initial` trace,
prove register/memory-writer capture, and import both results through the
existing developer-only `oasis.m68k.emulator-trace.v1`. No emulator was added
to the repository, no production dependency or gameplay runtime change was
made, and no M12 work started.
IMPLEMENTATION: added MAME debugger scripts for fixed 512-instruction trace
and a separate RAM write watchpoint, a BizHawk Lua script using
`event.on_bus_exec_any`/`event.on_bus_write` with D0-D7/A0-A7/SR snapshots, and
a local normalizer for both raw formats. The neutral capture metadata now also
records `backend` and `stop_condition`; the schema remains
`oasis.m68k.emulator-trace.v1`.
EVIDENCE: verified MAME `D:\Program Files\Mame\mame.exe` `0.289` and BizHawk
`D:\Program Files\BizHawk-2.11.1-win-x64\EmuHawk.exe` `2.11.1` against the
ignored USA ROM SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`. BizHawk
produced 512 instruction plus 128 write events (640 total), unique PCs
`0x26A,0x26C`, and replay hash `0x5CCA6906FAA6A219` on both runs. MAME
produced 512 instruction events, 32 unique PCs, and replay hash
`0xC2B1C053D1E43D76` on both runs. The independent MAME watchpoint caught a
16-bit write at `0x00FFFFFE`, stopped PC `0x0000026A`, value `0`.
PROVEN/UNKNOWN: both backends execute the ROM and expose useful M68K state;
BizHawk is primary and MAME secondary. First observed PCs (`0x26C` and
`0x214`) differ from reset PC `0x20E`, so hidden reset/bootstrap transitions
remain unknown. Neither trace reached `0x6121A`, `0x60B8C` or `0x60D4A`.
The adapters intentionally do not infer branch/call/return/read events from
instruction text; those normalized counts are zero. Save-state and read-hook
capabilities were not probed. Generated traces, emulator files and ROM remain
ignored and untracked.
TESTS: fresh Debug/Release/GNU-equivalent CTest passed 26/26 in each
configuration. ROM imports passed; BizHawk C/D and MAME E/F normalized replay
streams matched. Source file-limit and `git diff --check` passed. GitHub Actions
CI run `33864901965` for implementation commit `9f487ac` passed successfully.
NEXT: complete the local gate, commit this focused bake-off, push to GitHub,
wait for CI, record the CI result, push the documentation-only CI update, and
STOP. Do not expand tracing scope without explicit instruction.
## 2026-09-04 — M11.5 external emulator boot-trace oracle PoC blocked at backend
OBJECTIVE/SCOPE: test only whether an existing external Mega Drive emulator can
produce deterministic reset-to-boot evidence; no emulator implementation,
copied source/binary, production dependency, whole-game trace, autoplay,
semantic Atlas changes, call-clobber resolution or M12.
IMPLEMENTATION: added neutral `oasis.m68k.emulator-trace.v1` capture importer,
normalizer, deterministic hash/ordering, optional D0-D7/A0-A7/SR snapshots,
safe PC/block/range coverage, direct call and indirect target aggregation,
static reset-vector reader and Atlas-known/unknown comparison. Added CLI and
synthetic parser/normalization/edge/coverage/malformed-input tests. Adapter is
outside `oasis_core` and has no emulator dependency.
INVESTIGATION: PATH, common install roots, user folders and package listings
contained no BlastEm, RetroArch, MAME, Mednafen, BizHawk, Ares, Kega/Fusion or
equivalent debugger-capable executable. No real boot was run and no fake trace
was substituted. First PC, reset agreement, replay match, `0x6121A` observation
and watchpoint/writer capability remain UNKNOWN.
TESTS: synthetic validation passed, including parser rejection, normalization,
ordering, coverage, direct-call/indirect-target edges, Atlas PC/target split,
reset-vector comparison and register-sensitive deterministic hashing. Full
Debug/Release/GNU CTest passed 26/26 in each configuration; file-limit,
diff-check and the focused rebuilt target passed. GitHub Actions CI run
`33859968218` for implementation commit `e901686` completed successfully.
PROVEN/UNKNOWN: the adapter is useful for importing a future external capture,
but no boot execution, first PC, reset agreement, replay match, `0x6121A`
observation or watchpoint capability can be claimed without a backend.
NEXT: push only after the local gate, then provide/install an approved external
emulator with PC/register/memory-debug automation and run only `boot_initial`.
## 2026-09-04 — M11.5 external emulator boot-trace oracle PoC started
TASK: determine whether an already installed external Mega Drive emulator/debug
interface can produce a reproducible reset-to-boot normalized trace for the
canonical USA ROM.
SCOPE: boot_initial only; no emulator implementation, copied emulator source,
production dependency, whole-game tracing, autoplay, semantic Atlas changes,
call-clobber resolution or M12.
ACCEPTANCE: identify a viable existing interface or document the blocker; keep
the normalized adapter separate from `oasis_core`; add synthetic parser,
normalization, deterministic replay, coverage, Atlas comparison and malformed
input tests; run local boot oracle only if emulator and ROM are available; pass
Debug/Release/GNU CTest, source file-limit, diff-check and CI before push.
EVIDENCE/UNKNOWN: previous static checkpoint is `07f106b`; reset vector,
emulator availability, debugger fields, watchpoint support and first observed
PC are not yet established. NEXT: inventory local emulator executables and
documented automation/debug capabilities.
## 2026-09-04 — M11.5 bounded caller-stack provenance audit completed
TASK/SCOPE: audit only reachable paths in `[0x60004,0x61204)` before `0x60BCC`,
using symbolic A7 and no ABI, general stack model, caller discovery, emulator,
dynamic tracing, gameplay behavior or M12.
GOVERNANCE: main `c76ae829` policy was followed: the 500-line limit applies to
executable/source/build-code; prose Markdown/documentation is exempt.
IMPLEMENTATION: added `oasis.m68k.re-caller-stack.v1` with containing block
`[0x60BC4,0x60CDA)`, predecessor `0x60BA4`, deterministic paths/events,
symbolic merges and target rechecks. It supports only encountered
`MOVE.W SR,-(A7)`, `MOVEM.L regs,-(A7)` depth, longword push/PEA, A7 adjustment,
and `MOVEA.L (A7)+,An`; unknown direct calls invalidate stack provenance.
EVIDENCE: the USA oracle finds two relevant paths. One records `0x6042A`,
`0x60430`, `0x60B66`, then unknown `BSR.W 0x6121A` at `0x60B8C`; another
crosses unknown `0x60D4A -> 0x6121A` and one locally proven balanced call.
The previously proven `0x60BCC -> 0x604BC` effect is recorded without ABI
assumptions. Therefore memory[P] is unknown, `0x60BFA`/`0x60C08` remain
unresolved, reachable unresolved is `16→16`, the 14 `call_clobber` refs are
unchanged, speculative resolutions are zero, and no semantic role was assigned.
USA metrics: 2 paths, 14 unique stack events, 4 prior direct calls, 1 known
effect and 3 unknown call effects.
TESTS: Debug/Release/GNU-equivalent full CTest (25/25), USA oracle,
deterministic JSON/text, source file-limit and diff-check all passed. The USA
oracle ran against the supported local reference ROM and verified exact bytes,
CFG, events, both unknown-call blockers and unresolved target results. GitHub
Actions CI runs `33858039902` (implementation) and `33858141062`
(documentation follow-up) completed successfully.
UNKNOWN/NEXT: unknown effects of callees `0x6121A`; stop at this checkpoint and
await explicit instruction.
## 2026-09-04 — M11.5 bounded callee-effect audit completed
TASK/SCOPE: audited only direct call-site `0x60BCC` -> actual callee `0x604BC`; no ABI, recursion, emulator, runtime or M12.
EVIDENCE: bounded `[0x604BC,0x604E6)` has one block/RTS `0x604E4`, no nested call/indirect/unsupported flow; A0 unknown, A1-A5 untouched, A6 known `0x00FF06F2`, A7 preserved.
STACK: caller A7=P -> BSR return `0x60BD0` at P-4 -> callee no explicit stack delta -> RTS restores P -> outside-callee `0x60BD0` reads unknown longword at P and A7+=4.
RESULT: schema `oasis.m68k.re-callee-effect.v1`; targets `0x60BFA`/`0x60C08` remain unresolved; reachable 16→16; 14 `call_clobber` unchanged; speculative 0.
TESTS: synthetic suite, Debug/Release/GNU-equivalent full CTest (24/24), deterministic report, file-limit and diff-check passed. USA oracle executable added but local ROM was absent here. CI `build-test` run `33854626198` passed for `ebbb4f0`.
UNKNOWN/NEXT: stack longword at caller pre-BSR A7 and its raw role remain unknown. STOP; await next bounded evidence task.
## 2026-09-04 — M11.5 bounded MOVEA postincrement transfer checkpoint completed
TASK/RESULT: Added only longword `MOVEA.L (A7)+,An` with exact mode/register
decode, four-byte A7 increment, and narrow proven push/PEA/source-register
stack provenance. USA `0x60BD0=205F`; both targets cross `BSR 0x60BCC`, so no
stack value is invented. Metrics: targets 2, newly resolved 0, reachable 16→16,
14 prior `call_clobber` preserved, 2 stack-unknown `other`, speculative 0.
TESTS: synthetic stack/merge/call-boundary, USA oracle, deterministic JSON/text,
Debug/Release/GNU CTest, file-limit and diff-check passed. UNKNOWN/NEXT: no A7
entry value, callee effect, return-address model or ABI; stop at this checkpoint.
CI: local pre-push gate green; GitHub Actions run 33851500297 for commit
219ee585cf7b1283f72fe214c5d6320f16286f1b completed successfully.
## 2026-09-04 — M11.5 reachable-unresolved closure audit completed
RESULT: `oasis.m68k.re-reachable-closure.v1` accounted for 16 reachable refs:
14 call-clobber and 2 former unsupported-transfer boundaries; nonreachable 80
stays separate. Bounded CFG/provenance, synthetic/USA/Debug/Release/GNU tests,
determinism and CI run 33849249267 passed. No ABI, semantics or M12 work.
## 2026-09-04 — M11.5 unreachable-CFG audit completed
RESULT: `oasis.m68k.re-cfg-audit.v1` preserved raw 577/96 Atlas counts; USA
classified 80 nonreachable refs into 17 islands. CI 33847410245 succeeded.
## 2026-09-03 — M11.5 Atlas-driven unresolved evidence ranking completed

**RESULT:** Added typed Atlas unresolved records and `oasis_re_atlas_rank` /
`oasis.m68k.re-ranking.v1`; USA ranks 577 refs: displacement 446, function
`0x60004` 424, A6 387, move/address 360, immediate 168, dynamic 2; 4
unsupported items remain separate. **TESTS:** synthetic, USA/Beta oracle,
Debug/Release/GNU CTest 20/20, JSON/diff-check/file-limit and CI
`33767878177` green. **UNKNOWN/NEXT:** candidates are not resolution; choose
the next bounded improvement only, then stop—no M12.
## 2026-09-03 — M11.5 third checkpoint: bounded dynamic tracing PoC completed

**RESULT:** Isolated bounded scenario tracing at `0xA7E2` recorded 5 PCs,
3 blocks, 1 branch, 2 RAM reads, 1 return and `A7E2→A7E4`, resolving 3 static
items; backend is outside `oasis_core`. **TESTS:** synthetic/USA oracle,
Debug/Release CTest 17/17, deterministic JSON, file-limit/diff-check and CI
`33756628793` green. **UNKNOWN/NEXT:** other refs, targets and semantics;
stop without full tracing or M12.
## 2026-09-03 — M11.5 second RE-acceleration slice completed

**RESULT:** `oasis_re_program` covers `0x3820`, `0x8E90`, `0xA6A4`, `0xD3B2`:
421 instructions, 131 blocks, 1 direct call graph edge, 18 confirmed refs,
114 unresolved refs and explicit unsupported categories. USA evidence and
Debug/Release CTest 16/16 passed; no runtime behavior was added.

## 2026-09-03 — RE-acceleration checkpoint: bounded 68000 slice report

**TASK/RESULT:** Added isolated `oasis_re_tooling` and CLI for reachable entry
`0x60004`; corrected its byte-proven target to `0x6042A`. The report has 801
instructions, 109 blocks, 72 direct branches, 17 direct calls, 3 ROM and 114
RAM refs. Synthetic unresolved/unsupported cases and USA evidence passed.
**TESTS:** Debug/Release CTest 15/15, USA oracle, deterministic JSON,
file-limit and diff-check passed. No native gameplay behavior was added.

## 2026-09-03 — M11 activated: event/script router investigation
**RESULT:** Confirmed raw `0x82AE` producer fields and `0x7A28` router boundary;
added the bounded event router/trace and local USA oracle. Debug/Release CTest,
six ROM oracles and CI `33742561205` passed. Dialogue, progression, caller
identity and driver semantics remain unknown; M11 was completed and work stopped.

## 2026-09-03 — M10 summon-entry candidate added to the deterministic slice

**Concrete task:** translate the newly confirmed caller/initializer path at
`0x7A10 -> 0x846C` without assigning an unproven spirit name or ability.

**Acceptance criteria:** gate the path on the raw caller fields used at
`0x7A10`, reproduce the verified `0x846C` writes to singleton `FF1AA4`,
add synthetic coverage and extend the USA-ROM oracle, keep the separate
`FF19E8` target-query semantics explicitly unknown.

**Evidence:** `0x7A10` rejects caller records with flag bit 1 at `+0x37`,
then calls `0x846C` only when `+0x30 == 0x18`. `0x846C` writes raw type
`0x16` to `FF1AA4`, copies caller position fields, writes raw `0x13` to
`+0x10`, copies `+0x17` to `+0x66` and caller `+0x14`, sets both resource
words to `0x4F8`, and clears the two long fields at `+0xA6/+0xAA`.

**Implementation boundary:** model this as an observed raw summon-entry
seed. The table-derived velocity calculation after `0x84B2` remains outside
the native API until its complete data contract is translated.

**Exact next step:** implement and verify this raw entry, then reassess M10
completion without promoting `0x17A96`/`0x17CA6` to an ability contract.

## 2026-09-03 — M10 raw summon-entry slice implemented

**Result:** added `initialize_observed_summon`, which reproduces the raw gate
at `0x7A10` and the direct singleton-record writes at `0x846C`. Synthetic
tests cover the accepted and rejected gates, copied position/raw fields,
constant raw fields and cleared long fields. The local USA-ROM oracle now
checks both entry addresses and the native result.

**Boundary:** this proves a summon-entry candidate and its caller/data
contract, but does not identify a spirit by name or prove that the separate
`FF19E8` type check is the same object. The existing raw target-query slice is
retained as an evidence-backed interaction contract without semantic labels.

**Verification:** Debug and Release CTest pass `13/13`, file limits and
`git diff --check` pass. GitHub Actions CI for commit `d64e584` completed
successfully.
**Result:** M10 is complete. The `FF19E8` producer relationship remains a
documented unknown and is not used to invent spirit or ability semantics.
The next milestone may begin.
## 2026-09-03 — M10 closure attempt stopped at the evidence boundary
**Concrete task:** attempt to close M10 by tracing the producer and caller/data
chain for the raw type `0x16` check in the `FF19E8` target pool.

**Evidence:** the additional scan confirms that `0xFFDE` is a data-stream
initializer for the six-record `FF2D8C` pool: it obtains a record through
`0xD9F0`, copies a data-driven type and installs callback `0x17A96`. The
generic stream loaders around `0xFF1A` and `0xFCB8` allocate from `FF1CD8`,
not `FF19E8`. No caller/data record was found that proves a raw type `0x16`
write into `FF19E8`, the only literal write remains `0x847C -> FF1AA4`.

**Result:** M10 cannot be honestly closed. The cross-pool target query,
slot-gated dispatch and deterministic tests are complete, but summon entry,
summon identity and ability/interaction semantics remain UNKNOWN. M10 stays
ACTIVE and M11 remains out of scope.

**Verification:** the previously recorded Debug/Release CTest `13/13`, local
USA-ROM oracle and file-limit checks remain valid, this investigation changed
documentation only.

**Exact next step:** obtain runtime trace or identify the data table that
feeds a `0x16` record in `FF19E8`, then prove its caller relationship to
`0x17A96`/`0x17CA6` before adding behavior or closing M10.

## 2026-09-03 — M10 raw type-0x16 producer remains unresolved
**Evidence:** the static scan found one literal `move.w #$16,$0(a6)` at
`0x847C`, where `A6` is explicitly `FF1AA4`. Main-pool construction sites at
`0x11DF0`, `0x27B2A` and `0x2BD20` initialize other literal/table types or
consume data-driven values, none proves a raw type `0x16` producer in
`FF19E8`.

**Action/result:** kept the native cross-pool raw contract unchanged and
recorded the producer as an explicit unknown. No summon name, ability name or
runtime data layout was inferred.

**Exact next step:** obtain the missing runtime/data evidence for the
`FF19E8` record that satisfies the `0x17CA6` type check, then map it to the
slot-gated dispatch only if that relationship is demonstrated.

## 2026-09-03 — M10 target exclusion wording corrected
**Result:** clarified that `0xB922` excludes the owner by pointer equality,
not by numeric pool index. This matches the verified cross-pool path and keeps
the reverse-engineering record consistent with the native implementation.

**Exact next step:** trace the data producer that populates raw type `0x16` in
`FF19E8`, do not assign summon or ability semantics without that evidence.

## 2026-09-03 — M10 target selection committed
**Result:** The target-selection slice was committed as `3042689` and pushed to `origin/main`. The working tree is clean and `main` matches the remote branch.

**Exact next step:** trace the caller/data path that proves whether the slot-gated record is a summon or a different interaction, do not add another semantic layer until that evidence is found.

## 2026-09-03 — M10 target linkage remains unproven
**Evidence:** `0x846C` initializes raw type `0x16` in the separate record at `FF1AA4`, while `0xB922` scans the 21-record pool at `FF19E8`. The shared numeric type alone does not establish that these are the same object or that `0x17CA6` is a spirit ability.

**Action/result:** kept the native implementation unchanged and recorded the storage split as an explicit unknown. No summon name, ability name or interaction meaning was inferred.

**Exact next step:** trace producers of type `0x16` in `FF19E8` and the data/caller path that enters the `0x17CA6` behavior before extending M10.

## 2026-09-03 — M10 cross-pool target boundary correction
**Concrete task:** correct the native target-query boundary after proving that
the callback containing `0x17CA6` runs with an owner from `FF2D8C`, while
`0xB922` scans targets from `FF19E8`.

**Acceptance criteria:** represent separate owner and target pools, preserve
the observed owner-relative bounds, active-record filtering, first spatial
match and raw type `0x16` check, add synthetic coverage and extend the local
USA-ROM oracle with the callback/allocator evidence, do not assign summon,
ability or object names.

**Evidence:** initializer `0xFFDE` calls allocator `0xD9F0` for `FF2D8C`,
installs callback `0x17A96`, and that callback branches to `0x17CA6` when its
state word reaches zero. `0x17CA6` passes the owner in `A6` to `0xB922`, whose
scan base is hardcoded to `FF19E8`.

**Result:** `find_observed_target` now accepts separate owner and target pools,
synthetic coverage verifies that an equal numeric index is not incorrectly
excluded across pools. Debug and Release CTest pass `13/13`, the local USA-ROM
oracle passes, and all checked files remain at or below 500 lines.

**Remaining unknown:** the cross-pool raw contract is confirmed, but its
summon/ability meaning and the producer of raw type `0x16` in `FF19E8` remain
unproven.

**Exact next step:** trace the data producer that populates raw type `0x16` in
`FF19E8` and determine whether the `0x17A96`/`0x17CA6` path is a summon or a
different interaction before adding semantic behavior.

## 2026-09-03 — M10 spirit slot/dispatch trace
**Objective:** Translate the first evidence-backed spirit slot lifecycle and active-dispatch gate.

**Evidence:** `0x7BE8` maps event codes `0x16..0x19` to bits `0..3` of `FF0DBA`, `0x5202` reads that flag byte and `0x522E` contains the contiguous selector table `0x12..0x19`, `0x31B80` gates the observed path on raw entity `+0x41` bits `3|1`, checks slot bit 1 and guard bit 0 of `FF0DC4`, calls `0xC2EC` with selector `0x13` when open, sets the guard, then queues selector `0x15` through `0xCA24`.

**Actions:** Added `src/game/spirits/spirit_slots.*` with raw address/selector constants, event-to-slot mapping and a deterministic dispatch trace. Added synthetic coverage and a USA-ROM byte oracle. No spirit names, button meanings, targeting, attacks, rendering or audio semantics were invented.

**Result:** First M10 implementation slice is verified: Debug/Release build and CTest pass 13/13, and the local USA-ROM oracle passes. Files remain under 500 lines and ROM remains ignored/untracked.

**Exact next step:** commit/push this conceptual M10 slice, then investigate the next caller/data dependency needed for a proven summon or ability contract.

## 2026-09-03 — M10 observed target selection
**Objective:** Translate the target-selection portion of the caller at `0x17CA6` without assigning semantic names to the raw collision fields.

**Acceptance criteria:** mirror `0xB922` bounds and pool order for the observed query `D1=-10,D2=-6,D3=10,D4=6,D5=0,D6=4`, skip the owner, preserve the original first-spatial-match behavior before the caller's type `0x16` check, add synthetic coverage and ROM-byte oracle, keep files at or below 500 lines and leave the prior pushed slice intact.

**Evidence boundary:** `0x17CA6` passes the query to `0xB922`, compares the returned record's raw `+0x00` word with `0x16`, then changes its own raw state and queues selector `0x80022F`. The interaction meaning and spirit identity remain unproven.

**Implementation plan:** add a bounded `find_observed_target` query over the existing main entity pool, preserve the first spatial match before the raw type check, and verify it against the exact ROM bytes at `0x17CA6` and `0xB922`.

**Result:** `find_observed_target` is implemented and verified by synthetic tests, Debug/Release CTest (13/13 each) and the local USA-ROM oracle. No interaction or spirit identity was assigned to raw type `0x16`.

**Exact next step:** commit/push this target-selection slice, then trace the caller/data path that proves whether the slot-gated record is a summon or a different interaction.

## 2026-09-03 — M10 spirit lifecycle investigation started
**Objective:** Recover one evidence-backed spirit summon/slot/dispatch lifecycle slice and, only if its dependencies are proven, one ability or interaction contract.

**Acceptance criteria:** identify the summon caller and ROM data, identify slot storage and lifecycle transitions, identify active-spirit dispatch, implement a deterministic portable slice with synthetic tests and a local USA-ROM oracle, keep rendering/audio/unproven semantics out, update the file map, reverse-engineering ledger and roadmap, keep files at or below 500 lines, finish with green CI.

**Current boundary:** M10 is authorized. No spirit names, targeting rules, abilities, interactions or presentation behavior are treated as facts until direct callers/data prove them.

**Exact next step:** scan the local USA ROM for summon/slot/dispatch candidates and record the first confirmed address chain.

## 2026-09-03 — M9 common entity pool slice
**Objective:** Recover and implement the common entity-pool iteration contract before translating representative enemy/NPC behavior.
**Evidence:**
- `0x8E90` scans `FF2954` with 4 records at stride `0x5A` and routes active entries to `0x8F12`,
- `0x8EB2` scans `FF19E8` with 21 records at stride `0xBC` and routes active entries to `0x8F22`,
- `0x8ED4` scans `FF2D8C` with 6 records at stride `0x5A` and routes active entries to `0x8F12`,
- each loop reads word `+0x00` and enters movement only when the signed word is positive.

**Actions:** Added raw `EntityPoolSpec` descriptors, bounded `EntityPoolView`, synthetic active-record tests and a local USA-ROM oracle for the three loop shapes. Updated the file map and M9 reverse-engineering ledger.
**Result:** The common pool framework is represented without inventing whether a record is an enemy, NPC or effect. No entity behavior has been started.

**Exact next step:** trace the shared record fields consumed at `0x8F22` and identify one representative non-player behavior with caller/data evidence.

## 2026-09-03 — M9 shared record contract
**Objective:** Trace the common movement fields and close one representative non-player dispatch path without inventing AI semantics.

**Evidence:** `0x8F22..0x9382` directly accesses raw offsets `+0x9C`, `+0x9D`, `+0x2A`, `+0x2C`, `+0x2E`, `+0x30`, `+0x32`, `+0x37`, `+0x38`, `+0x72` and `+0x76`, `0x8D06` initializes `+0x26` and `+0x22`. The `FF2954` block at `0xA6A4` gates a four-slot record on `+0x00` and bit 2 of `+0x3A`, then `0xA7D4` jumps through `+0x22`.

**Actions:** Added bounded big-endian `EntityRecordView`, raw offset constants, synthetic boundary tests and local USA-ROM byte assertions for the shared accesses and representative dispatch contract. Kept callback invocation, AI, attacks, animation and spawn semantics outside the native layer.
**Result:** The shared record contract and one raw non-player callback-dispatch behavior are evidenced and tested, semantic callback translation remains the next M9 decision point.

**Exact next step:** run full M9 acceptance checks, update roadmap status, and only then decide whether M9 can be marked DONE.

## 2026-09-03 — M9 acceptance — DONE
**Objective:** Verify the completed M9 slice and close the milestone without expanding into M10.

**Checks:** Debug CTest `12/12` passed, Release CTest `12/12` passed, local USA-ROM entity oracle passed, file-size check passed, ROM archive stayed local-only and untracked, GitHub Actions CI #254 for `eae566c` completed successfully.
**Result:** M9 is accepted. The repository now contains the common entity-pool contract, shared raw movement-field view and one evidence-backed `FF2954` callback-dispatch path. AI, attacks, animation and spawn semantics remain outside the milestone.

**Exact next step:** await explicit authorization before starting M10.

## 2026-09-03 — M8 acceptance review
**Objective:** Close M8 after local, ROM-reference and remote CI verification.
**Evidence/tests:**
- Debug CTest: 11/11 passed from a clean shell,
- Release CTest: 11/11 passed from a clean shell,
- local USA-ROM player oracle passed,
- GitHub Actions `CI #250` for `c705157` completed with `Success`,
- file-size check passed and the ROM archive is ignored/untracked.

**Result:** M8 is accepted. The portable player movement/state slice, terrain-gated consumer integration, optional raw velocity context and Windows CTest runtime setup are committed on `main`, the commercial ROM remains local-only.
**Unresolved:** presentation callbacks/animation fields and the lifecycle of bit 4 of `FF16F1` remain outside the verified scope.

**Exact next step:** no M9 work, await explicit new project scope.

## 2026-09-03 — M8 state-2/state-4 branch closure
**Objective:** Validate the stop and turning branches reached through the player state dispatcher before modelling any presentation state.
**Evidence:**
- state `2` enters at `0x62E4`, no direction branches to `0x62CC`, which clears `+0x4E/+0x52`, writes `+0x2A=0` and state `+0x04=0`, cleanup of `+0x72/+0x76` belongs to shared movement,
- state `4` enters at `0x6516`, maps the current direction through `0x83D4`, and uses `+0x17` plus the normalized input to select an axis,
- the valid state-4 path accumulates deltas in `+0x72/+0x76`, the no-input path compares `FF197E` against `6` and writes state `+0x04=0x000C` after the threshold.

**Actions:** Added ROM byte-oracle assertions for the state-2 stop block and state-4 turning/timeout branches. Recorded only raw offsets and observed transitions, presentation meanings of `+0x2A`, `+0x26` and related callbacks remain unknown.
**Result:** State-2 stopping and state-4 directional accumulation are closed at the branch level. The native slice now models those raw state transitions and leaves presentation callbacks outside the API.
**Exact next step:** verify the native state driver against the shared movement consumer and isolate the remaining state-4 slowdown context around `0x64C4`.

## 2026-09-03 — M8 shared movement consumer integration
**Objective:** Verify that the native state driver hands accumulated deltas to the same terrain-gated consumer and preserve the footprint mask needed by `0x64C4`.

**Actions:** Added an integration test covering state-4 axis selection through `try_move`, terrain aggregation and accumulated-delta cleanup. Preserved the aggregate OR mask as raw entity `+0x6F` and added a ROM oracle for the `0x64C4` branch structure.

**Result:** State-driver output reaches the shared movement consumer and the native player records the confirmed footprint mask. The remaining external flags at `FF1985`, `FF1984` and `FF16F1` are isolated as slowdown context, not presentation state.

**Exact next step:** determine the lifecycle and writers of the three `0x64C4` global flags before adding a slowdown context object.

## 2026-09-03 — M8 velocity-adjust context isolation
**Objective:** Determine the writers and ownership boundary of the external flags consumed by `0x64C4`.

**Evidence:**
- `FF1985` has direct writes in event/control paths at `0x56F2`, `0x57C6` and `0x7E50`, while player update and `0x64C4` only read it,
- `FF1984` is cleared/set by the active-entity scans at `0x2D220..0x2D23A` and `0x2F250..0x2F286`, with the main entity selected through `FF19E8`,
- bit 4 of `FF16F1` is read at `0x64D8`, but no direct bit-4 write appears among the scanned references.

**Result:** The three conditions are external timing/context inputs, not player presentation fields. A future context object can expose them without assigning gameplay names that the ROM does not establish.

**Exact next step:** add a small `VelocityAdjustContext` and differential unit tests for the three confirmed `0x64C4` branch outcomes.

## 2026-09-03 — M8 velocity-adjust context implementation
**Objective:** Add the confirmed `0x64C4` branch outcomes without assigning semantics to external RAM flags.

**Actions:** Added an optional raw `VelocityAdjustContext` carrying `FF1985`, `FF1984`, bit 4 of `FF16F1` and footprint `+0x6F`. Added unit coverage for bypass, half-both and Y-only-half outcomes, and connected the context to state-4 retained-axis accumulation.

**Result:** The native driver reproduces the isolated `0x64C4` arithmetic when a caller supplies external context, the default path remains unchanged when context is unavailable.

**Exact next step:** verify frame-boundary ordering between state-4 context sampling, footprint update and shared movement consumption.

## 2026-09-03 — M8 frame-boundary ordering
**Objective:** Verify that state-4 velocity context is sampled before the shared movement consumer updates the footprint mask.

**Actions:** Added a two-frame test: the first state-4 update uses the old `footprint_any_bits`, then `try_move` records the new aggregate mask, the second update consumes that new mask.

**Result:** The native order matches the recovered loop: player update reads prior-frame context, shared movement commits/cleans accumulated deltas, and the footprint OR mask becomes available to the next frame.

**Exact next step:** finish M8 acceptance review and keep the ROM reference workflow local-only.

## 2026-09-03 — Windows test runtime fix
**Objective:** Make the documented CTest command work from a clean PowerShell session.

**Actions:** Added a Windows GNU/Clang CMake test environment that supplies the active compiler's runtime DLL directory to CTest, and documented the behavior in the README.

**Result:** Debug and Release CTest runs pass 11/11 without manually editing `PATH`, the previous `libwinpthread-1.dll` dialog is no longer produced by the test workflow.

**Exact next step:** finish M8 acceptance review and keep the ROM reference workflow local-only.

## 2026-09-03 — M8 player update dispatch closure
**Objective:** Close the indirect player update path and preserve its confirmed state transition in the native slice.

**Evidence:**
- main loop `0x8B22` calls player update `0x557A` before shared movement `0x8E90`,
- `0x557A` selects `FF19E8` and `0x5670` dispatches through table `0x59BA`,
- dispatch entries route state `0` to `0x61F6`, state `2` to `0x62E4`, and state `4` to `0x6516`,
- `0x61F6` writes direction `+0x16`, intent deltas `+0x4E/+0x52`, accumulated deltas `+0x72/+0x76`, and state `+0x04=2`.

**Actions:** Added the confirmed update/state addresses to the player API, modelled the state-2 transition, and extended the local ROM oracle to validate the dispatch entries.

**Result:** The earlier absence of direct calls to `0x61F6` is explained by the indirect `0x59BA` dispatcher. Animation callback/presentation meaning remains intentionally outside the native movement slice.

**Exact next step:** validate the state-2/state-4 stop and turning branches against additional ROM traces before modelling any presentation state.

## 2026-09-03 — M8 player movement evidence and native slice
**Objective:** Translate the confirmed player input-to-movement path into a portable deterministic model.

**Actions before implementation:**
- installed CMake 4.4.3 and LLVM-MinGW locally because the workspace had no C++ toolchain,
- traced player initialization at `0x13D6..0x142E`, which selects entity slot `0xFF19E8` and type `2`,
- confirmed controller normalization at `0x2992` into `0xFF165C..0xFF1661`,
- decoded the direction dispatch table at `0x85E2`: low nibble of `0xFF165E` maps to cardinal/diagonal fixed-point vectors,
- confirmed the shared movement cluster starts at `0x8F12`, samples the entity footprint through `0x9BF2`, and gates terrain changes through `0x938E`.

**Understanding confidence:** 93% for input mapping, player slot and movement-gate contract, unknown animation/entity callback semantics remain outside this slice.

**Acceptance criteria for this task:**
- portable player state and deterministic direction mapping,
- default speeds and diagonal vectors match the confirmed `0x85E2` table,
- free and blocked movement tests reuse the M7 footprint and terrain-gate APIs,
- local Debug and Release builds/tests pass,
- evidence, file map and milestone state are updated.

**Exact next step:** implement and test the isolated player movement slice, then run both build configurations.

## 2026-09-03 — Repository merge and ROM hygiene
**Objective:** Bring the native implementation into `main` without tracking the user-supplied commercial ROM.

**Actions:**
- merged `origin/oasis-cpp-bootstrap` into `main`,
- removed the ROM archive from the Git tree while retaining it locally for reference checks,
- added the ROM archive and generated build/inspection artifacts to `.gitignore`,
- removed GitHub workflows that attempted to retrieve the ROM from the repository.

**Result:** `main` contains the native source, tests and documentation, the reference ROM remains local-only.

**Unresolved:** ROM-backed probes must be run locally or through a separately provisioned private CI mechanism.

## 2026-09-03 — M7 world/map/collision foundations — DONE
**Objective:** Establish a verified room/screen loading path and tested collision/world-grid primitives before player translation.

**Actions:**
- resolved screen dispatcher `0xC8F0` through group table `0xC92C` to 26-byte descriptors and added ROM-backed native descriptor loading,
- confirmed auxiliary byte-grid descriptors at `0xFF1766` / `0xFF1770` and 8-pixel world-to-cell addressing,
- identified `0x9C40` as a single-cell grid query,
- identified `0x9BF2` as OR/AND footprint aggregation across covered cells and translated it to `ByteGridView`,
- recovered terrain tables `0x96E8` / `0x96F8` and their class mapping role,
- identified `0x938E` as the directional terrain movement gate: carry set blocks, carry clear allows,
- added portable terrain-gate semantics and synthetic tests,
- reduced completed M7 research workflow to manual dispatch only.

**Evidence/tests:**
- confirmed descriptor mappings include `0x0009 -> 0x02CF82`, `0x000C -> 0x02D3E8`, `0x0704 -> 0x032144`, `0x0705 -> 0x03285C`,
- movement loop calls `0x938E` before committing movement and branches on carry to the stop path,
- byte-grid, footprint and terrain-gate synthetic tests pass,
- final `build-test`, reference-ROM workflow and M7 probe all completed successfully.

**Result:** M7 acceptance criteria are satisfied. M8 player-system work is activated.

**Exact next step:** identify the player entity/update entry point and trace controller input into the movement cluster and confirmed terrain gate.

## 2026-09-03 — M7 indexed resources and screen descriptor pattern
**Objective:** Move from generic world-data searching to a reproducible original loading path without inventing room semantics.

**Actions:**
- added ROM-backed `m7-world-probe` workflow,
- identified indexed compressed-resource table at `0x05CE96` with 108 entries including a zero entry and 107 dense ROM pointers,
- established reader routine `0xD3B2`: incoming `D0` is multiplied by four, used to select a pointer, and the selected stream is decompressed to `0xFF2FA8`,
- found seven direct `D3B2` calls using immediate resource IDs `3`, `4`, `35`, `36`, and `87`,
- located the ROM developer screen-name inventory beginning at `0x05DB4D`,
- found repeated 26-byte blocks immediately before several setup sequences calling `0xD406`,
- observed that `D406` reads a structured input through `A1` through at least offset `+24`,
- launched a focused probe for pointer tables referencing candidate 26-byte descriptor starts.

**Evidence:**
- `0xD3B2` performs `A0=0x5CE96`, `D0<<=2`, pointer lookup, `JSR 0x3820`,
- caller pairs `3/4` and `35/36` pass `D1=0x4000/0x5000`, followed by original VDP/DMA activity, indicating these selected resources are likely graphics/data banks rather than the room record itself,
- index 87 is separately loaded by another setup path,
- developer strings include `VILLAGE`, `ECAPITAL`, `HARBOR`, numbered dungeon screens, boss labels, and `14-01 KING`,
- candidate descriptor spans such as `0x2CF82..0x2CF9B` and `0x2D3E8..0x2D401` are exactly 26 bytes, matching `D406` accesses through offset `+24`.

**Result:** A stable scene-resource loader and a strong candidate screen-descriptor shape were established for later closure evidence.

## 2026-09-03 — M7 world/map/collision research started
**Objective:** Identify raw room/map/collision data before creating native structures.

**Actions:**
- activated M7 after verified completion of deterministic runtime/input,
- inspected public reverse-engineering work for map-related evidence,
- confirmed its documented tilemaps are fixed UI/screen tilemaps rather than proven world-room formats.

**Result:** No semantic room structure was invented, binary-first probing was selected.

## 2026-09-03 — M6 deterministic runtime/input — DONE
**Objective:** Establish portable one-frame stepping and controller snapshots before gameplay translation.

**Actions:**
- added `src/core/runtime.hpp/.cpp`,
- defined portable Genesis-style button masks and two controller ports,
- added explicit integer frame index and `FrameContext`,
- added `RuntimeLoop::step()` with one update per explicit call,
- added replay-style deterministic tests,
- detected and removed a duplicate experimental `game/runtime` API so one source of truth remains.

**Evidence/tests:** identical input sequences produce identical recorded traces, changing one frame changes the trace, no wall-clock, SDL or OS input API is used, CI passed.

**Result:** M6 completed, M7 activated.

## 2026-09-03 — M5 narrow VDP model — DONE
**Objective:** Replace the placeholder video scaffold with only the Mega Drive state needed for reconstruction.

**Actions:** modeled 64 KiB VRAM, 128-byte CRAM and 80-byte VSRAM, bounded byte/word access, tile pattern-name fields, minimal plane/sprite data, explicit unsupported emulator semantics, tests.

**Result:** M5 completed without full VDP emulation or renderer dependency.

## 2026-09-03 — M4 local asset inspection tool — DONE
**Objective:** Inspect graphics from a user-owned ROM using the verified native decompressor.

**Actions:** Genesis 4bpp tile decoding, CRAM-to-RGB conversion, `oasis_inspect`, PGM/PPM output, ignored generated outputs, ROM-backed inspector verification.

**Result:** M4 completed, no extracted commercial assets are intentionally part of the native code model.

## 2026-09-03 — M3 native graphics decompressor — DONE
**Objective:** Translate original 68000 decompression routine `0x3820` into verified native C++.

**Evidence:** format A `0x16943C`: `1217 -> 3072`, SHA-256 `65e99e74020fedbdcb97c8249a5ccfe540aca5bb5d29bfb260352cd6f388c31a`; format B `0x1894EA`: `112 -> 128`, SHA-256 `167d4e5409f6b075b3b6f2bc61dbb747e8d8c857e8699745184ddf48d83bcda9`.

**Result:** Native C++ matches original traces; M3 completed.

## 2026-09-03 — M2 canonical ROM identity verified — DONE
**Confirmed fingerprint:** size `3145728`; CRC32 `C4728225`; SHA-1 `2944910c07c02eace98c17d78d07bef7859d386a`; SHA-256 `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

**Result:** M2 completed with executable evidence.

## 2026-09-03 — Governance enforcement — DONE
Mandatory AI workflow, architecture/vision/file-map/roadmap/decision/research/work logs, `PROJECT_STATE.md`, `TASK.md`, and <=500-line CTest enforcement established.

## 2026-09-03 — Initial C++ bootstrap — DONE
C++20/CMake project, ROM loader, memory/VDP scaffold, known symbol table, tile-copy compatibility helpers and smoke test established.

## Entry template
```text
## YYYY-MM-DD — Short task name
Objective:
Actions:
Files changed:
Evidence:
Tests/build:
Result:
Unresolved:
Exact next step:
```
