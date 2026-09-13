# 2026-09-13 — THOR Evidence Engine V3 REGISTER/CONTROL BUILD — PASS

TASK: Assemble the reusable temporal register, execution and local control
provenance skeleton requested for V3 BUILD. Scope is developer-only and does
not repair known V2.1 soundness defects, promote SOURCE_OWNED bytes, or begin
V4.

CHANGES: Added register slice semantics and local rules, execution/call/return
and control facts, typed graph/export/explain, existing-sidecar SQLite tables,
known-defect ledger, V3 report/ADR and CTest integration.

SOURCE_OWNED remains `1,475,368 / 3,145,728`, delta `0`. Validation results and
final SHA: publication commit (exact SHA in the final gate response). Windows Release build and 169/169 non-line-limit CTest
tests passed; all six evidence helpers passed on Windows and GNU/Linux
equivalent. The tracked source-size check reported zero violations. Exact
publication CI is recorded with the final gate response.

# 2026-09-13 — THOR Evidence Engine V2 RAM PROVENANCE — PASS

TASK: Generalize only the proven temporal RAM mechanism into reusable byte
versions and coverage-aware last-writer queries. Baseline:
`1cf1038fd7b9e09a86421de47b86430689cb525e`.

RESULT: `ram_versions.py` now models big-endian byte SSA across byte/word/long
writes, partial overlap, same-value overwrite, explicit pre-capture roots and
restore epochs. Last-writer results are PROVEN only under trace-bound complete
coverage; otherwise the engine returns EXTERNAL_STATE, PRE_CAPTURE_ORIGIN,
INCOMPLETE_CAPTURE, UNKNOWN_TRANSFORM or CONFLICT. Per-address indexes keep
bounded queries out of the obvious quadratic path.

V1 REGRESSION: FF13CC is rebuilt through the reusable engine. Four concrete
byte queries prove `00 88 09 01` and retain the existing provenance branches.
HELD-OUT: existing FF188A evidence is queried without new coverage and
correctly returns INCOMPLETE_CAPTURE; no writer is promoted by observation
alone. SOURCE_OWNED remains `1,475,368 / 3,145,728`, delta 0.

VALIDATION: targeted V0/V1/V2 helpers pass; Release CTest and smoke build are
required next. Debug MSVC retains the pre-existing `std::to_string` failure;
no unrelated fix is made. No V3, register/control generalization, whole-game
trace, ROM RE or ownership promotion.

# 2026-09-13 — THOR Evidence Engine V1 FF13CC CANARY — PASS

TASK: Implement the first bounded engine-derived causal provenance slice for
the already authorized FF13CC canary. Scope excludes V2, new ROM reverse
engineering, and SOURCE_OWNED promotion. Baseline:
`c74e68d4a9c8013ca42891e8a5aead865386f91f`.

RESULT: The checked static producer and sealed two-epoch canary capture now
produce a temporal SQLite-backed certificate. The query proves ROM high24 plus
incremented D5.low8 to the first concrete FF13CC value-version, with explicit
VALUE, ADDRESS and CONTROL roles. Epochs, execution instances and partial
register slices remain distinct. The callback PC is retained only as a raw
witness; no PC-2 heuristic is present.

ARTIFACTS: `src/tools/thor_evidence/canary_engine.py`, the developer-only
canary collector/launcher, schema/store extensions, the V1 canary test and
negative fixture, and `docs/reports/THOR_EVIDENCE_ENGINE_V1_FF13CC_CANARY.md`.
The capture raw SHA is
`e69c73e6c71677066d6a711d5ab1052a27f112fb43f35d243a138b50f499b09`; the
certificate SHA is `af8dafbf276a527df0e589ca876803c6ed3261d20de65b1a4b4229ae31f1b0e5`.

FRONTIER: access width, overlap/range semantics, same-value writer
completeness, IRQ/exception interaction and input-read causality remain
explicit UNKNOWN. They do not block this exact canary. C10 emits no input
causal edge; C11 remains a separate structural SAT/DMA join.

VALIDATION: helper test passes; V0/V1-gate/dense-gate/V1-canary CTest tests
pass; Release `oasis_smoke` builds; certificate derivation, SQLite import and
idempotent re-import pass. Full Debug MSVC remains blocked by the pre-existing
`std::to_string` error in `src/core/ram_flag_routine.cpp`; no unrelated fix was
made. No Linux-equivalent build was available in this Windows session.
SOURCE_OWNED remains `1,475,368 / 3,145,728`, delta 0.

# 2026-09-13 — THOR Evidence Engine V1-GATE-COVERAGE — PASS / V1 BLOCKED

TASK: Close only dense execution, checked memory-effect classification and the
local interruption boundary for the already selected FF13CC A372 instance.
V1 provenance, graph construction, general last-writer logic, new ROM discovery
and SOURCE_OWNED promotion remain out of scope.
BASELINE: a5cc2d09dc2698127f9977b3bf942f3d8e845ee3.
RESULT: The existing BizHawk 2.11.1 API `event.on_bus_exec_any` captured every
instruction callback in the minimal interval from pre-boundary `A370` through
the `A374` post-fetch boundary. The corrected second attempt produced two
restore epochs with raw events `A370 -> A372 -> WRITE FF13CC -> A374`; the
selected epoch-1 interval contains two executed instructions and one boundary
witness. The target store is statically decoded as `MOVE.L D2,(A5)+`; runtime
A5=`FF13CC`, width is four bytes, and the observed value is `00880901`.
CLASSIFICATION: `NO_MEMORY_WRITE=1`, `WRITE_DISJOINT=0`,
`WRITE_TARGET_OVERLAP=1`, `UNKNOWN_MEMORY_EFFECT=0`. The overlap writer's
concrete range is `FF13CC..FF13CF`, with a raw WRITE witness. The checked
ROM-derived decoder mapping is bound by ROM, static JSON and decoder hashes.
INTERRUPTION: `NO_INTERRUPTION_IN_INTERVAL`; both next-PC edges are continuous
(`A370 -> A372 -> A374`) and no vector/handler event appears in the dense
interval. This is local evidence only, not a global IRQ model.
ADVERSARIAL FIXTURES: persisted coverage cases reject removal, reordering,
unknown effects, forged disjoint EA, lost one-byte overlap, lost same-value
overwrite, discontinuity, incomplete handler body, duplicate-PC collapse and
omitted loop iteration. Correct one-byte and same-value writers remain accepted
only when explicitly represented.
SOURCE_OWNED: 1,475,368 / 3,145,728, delta 0.
VALIDATION: dense capture/receipt/normalization succeeded; dense-gate tests,
V0 tests and V1-gate tests pass. Release and Linux-equivalent CTest remain the
required build checks; the pre-existing full Debug MSVC `std::to_string`
failure remains outside this bounded developer-only gate.
STOP: dense coverage and local interruption closure are proven. Do not
implement V1 without separate authorization.

# 2026-09-12 — THOR Evidence Engine V0.1 — COMPLETE / V1 BLOCKED

TASK: Repair the three P1 V0 foundation defects found by the independent gate
review: strict raw completion, truthful execution identity, and evidence-backed
capability reporting. V1, ROM reverse engineering and SOURCE_OWNED mutation are
out of scope.
BASELINE: 38e702cbae29e2b8f6d09c70b9ffd2e73db0d739.
RESULT: Added `thor.evidence.raw.v0.1`, launch receipts, strict duplicate-key
raw parsing, two-epoch completion validation, collector/normalizer separation,
mode/reverse/watch-plan binding, and witness-backed capability claims. The old
V0 captures are retained as `VALID_ONLY_FOR_V0_HISTORICAL_EVIDENCE`; three
bounded corrected probe captures were produced with the same scenario. Probe A/B
have identical raw event and normalized event streams; reverse order differs.
SAT/fields remain `0088090187810088`/`00100080`, and SOURCE_OWNED delta is 0.
V1 local preconditions remain blocked only for A372/write pairing, relevant
writer completeness and interruption boundary; input causality remains allowed
UNKNOWN.
VALIDATION: focused Python tests pass 8/8; Release, Linux-equivalent and
focused CTest are the applicable checks. Existing full Debug MSVC failure in
`src/core/ram_flag_routine.cpp` (`std::to_string`) remains pre-existing.
STOP: publish V0.1, verify exact CI and stop. Do not begin V1.

# 2026-09-13 — THOR Evidence Engine V1-GATE — PARTIAL / V1 BLOCKED

TASK: Close only the local FF13CC capability obligations before V1. No
provenance graph, general last-writer engine, register provenance, new ROM
discovery, ownership promotion or M13/C++ work.
BASELINE: 81f910415498f20702c790b3155985c57172375e.
RESULT: Reused the checked A372 static producer/decoder evidence and corrected
V0.1 capture. The exact form is `2A C2 = MOVE.L D2,(A5)+`, width 4, next PC
`A374`. Selected runtime instance `epoch 1 / seq 117` pairs ordered
`EXEC A372 -> WRITE FF13CC (callback PC A374) -> next EXEC A374`, with
pre-A5=`FF13CC`, post-A5=`FF13D0`, D2/value=`00880901`. Four independent byte
versions are represented with raw witnesses. The checker deliberately never
uses a universal PC-2 rule.
BLOCKERS: V0.1 exact-address hooks do not provide dense all-instruction
coverage for interval seq 117..120; relevant overlapping-writer completeness
is therefore BLOCKED. No interruption/vector/stack boundary certificate exists,
so interruption is BLOCKED. Existing pairing evidence is not promoted to V1
causal provenance.
NEGATIVE FIXTURES: persisted synthetic cases reject omitted overlapping writer,
forged pairing, shifted callback PC, unknown memory effect and insufficient
interruption coverage; one-byte overlap and same-value writes remain explicit
writers.
SOURCE_OWNED: 1,475,368 / 3,145,728, delta 0.
VALIDATION: focused V1 gate tests pass; CTest integration added beside V0.1.
FINAL: PARTIAL / V1 BLOCKED. Hard stop after report, publication and exact CI.

TASK: V0 foundation/storage identity and bounded BizHawk capability contract.
WHY: Establish reliable temporal observations before any causal provenance.
CURRENT MILESTONE: M12; V0 only, mandatory STOP after report/publication.
MILESTONE UNDERSTANDING CONFIDENCE: partial; no production behavior is inferred.
CURRENT SLICE UNDERSTANDING CONFIDENCE: storage contract high; callback semantics
are experimental UNKNOWN until measured. No production C++ is authorized.
SLICE CONFIDENCE EVIDENCE: ADR-0044, controlled harness and existing M12 tests.
BASELINE: b886d2507c9010caac2a75ccbb629b809399ffd0, main == origin/main.
The previous task's uncommitted architecture documents are retained and will
be published with their bounded V0 implementation, not redesigned.
ACCEPTANCE CRITERIA: sealed/idempotent/cross-ROM-safe temporal storage; synthetic
fixtures A–K; measured PC/phase/width/overlap/restore capabilities; two identical
cold-process logical streams; bounded non-interference and performance report;
SOURCE_OWNED 1,475,368 unchanged; relevant Debug/Release/Linux-equivalent checks,
source limit, exact final commit/push/CI. No V1+, promotions or gameplay search.
EVIDENCE AVAILABLE: exact ROM/state, current descriptor-candidate manifest,
build/bizhawk-controlled-harness/run.ps1 and controlled.lua.
KNOWN UNKNOWNS: callback width/phase/filter semantics; IRQ completeness; MMIO
peek effects. Every unresolved capability retains an explicit V1 consequence.
EXPERIMENT BUDGET: each process <=30 s; fixed state with 3 settling +1 measured
frame; at most one additional restore of that same window per probe. Initial
batch: baseline/uninstrumented/minimal plus two identical probes and one reversed
installation-order control. No guest RAM/register writes or new savestates.

RESULT: V0 implementation and bounded capability matrix completed. Sealed JSONL
transport, SQLite receipt/import, full-hash identity, epoch/event/value-version
guards, UNKNOWN-only relations, synthetic fixtures and one developer-only Lua
collector are tracked. The two cold probe raw streams and logical event streams
were byte-identical; reversed hook installation changed the event stream as
expected. Minimal/probe/reverse reached SAT `0088090187810088`, fields
`00100080`, six EXEC hits and one FF13CC write per epoch. Uninstrumented reached
the same oracle with zero execution/write callbacks. SOURCE_OWNED remained
1,475,368 bytes (delta 0); process wall times were 3.29–4.18 seconds.

VALIDATION: `python tests/thor_evidence_v0_test.py` passed 3/3. A normalized
probe capture imported transactionally into SQLite. Tracked source-limit scan
checked 554 files with zero violations; `git diff --check` passed. Focused V0
CTest passed in Debug (`-C Debug`), Release and GNU/Linux-equivalent WSL.
Release and WSL builds passed. The full Debug multi-config build was attempted
and remains blocked by the existing `std::to_string` compile error in
`src/core/ram_flag_routine.cpp`; this is outside V0 and the focused V0 target
still passes. The project CMake source-limit script was attempted but exceeded
the local runner timeout while walking the very large Windows workspace; the
tracked-source equivalent check is the result above.

PUBLICATION: implementation commit `897faf3c330b98e34802fe0cc8f67175133cfc29`
was pushed to `origin/main`; `git ls-remote` matched it. GitHub CI run
`34716072864` completed successfully. Final documentation publication follows
as a small focused commit; no V1 or additional runtime work is authorized.

# 2026-09-12 — THOR Evidence Engine architecture — DESIGN CONTRACT

TASK: Critically design a developer-only M12 evidence/provenance engine around
the working controlled BizHawk harness and existing Carver tooling. Specify
temporal identity, scoped proof rules, persistence, capture completeness,
static integration, scheduling, a known canary and bounded implementation stages.
Optional V0/V1 implementation is not claimed by this documentation task.

ACCEPTANCE: Address all sixteen design questions and components A–N; give V0–V9
goals, scope, artifacts, tests, budgets, STOP conditions and transition evidence.
Ground the canary in current writer/selector evidence and preserve the exact
manifest/reconstruction boundary. Validate the proposed SQL core and document
the difference between a design check and a working capture/provenance engine.

RESULT: Added THOR_EVIDENCE_ENGINE_ARCHITECTURE.md and
THOR_EVIDENCE_ENGINE_STAGES.md, with ADR-0044 and architecture/file-map links.
Decision: MODIFY IDEA, then BUILD in bounded stages. Separate immutable events,
temporal byte/register versions and scoped relations; use demand-driven slicing,
SQLite sidecar, coverage certificates and non-owning Carver export. Minimal
RAM/register semantics move into V1 so the canary proves an actual chain.

BASELINE: HEAD at inspection b886d2507c9010caac2a75ccbb629b809399ffd0. Original
ROM SHA256 was checked live. The descriptor-candidate manifest hash is
996ce8100ff8354d682d217b34b7c61bd7e5cad49ec5a4cb5373f69ef76be70b and records
1,475,368 SOURCE_OWNED bytes; older screen-root-c has 1,475,346 and must not
be silently reused. Runtime evidence was read from existing reports/scripts;
no new emulator execution, ownership transaction or runtime change occurred.

VALIDATION: The 24-table design DDL was executed in Python sqlite3 3.45.3;
foreign keys, integrity and negative FK/hash-length/size constraints passed.
Local report links and component/stage inventories were verified. Final
diff/whitespace review and repeated ROM/manifest identity checks passed;
no executable source was changed. This checks the relational design only,
not an importer, semantic validator or emulator capability. Native builds/CTest and
CI are not required or claimed for this documentation-only change. Existing
untracked workspace artifacts remain outside this task's change set.

NEXT: V0 synthetic storage/identity tests and measured capability validation;
V1 and unknown-frontier expansion remain unimplemented. Roadmap milestone
status is unchanged; M13/M14 are not started.

# 2026-09-12 — Independent BizHawk controlled harness diagnosis — WORKING

TASK: Verify the installed BizHawk 2.11.1 CLI/Lua startup, canonical ROM and
existing QuickSave1 load, Genesis readiness, exact one-frame P1 Right input,
and only the known A372 execution / FF13CC four-byte oracle. Check process
coexistence without CUA. No RE, new addresses, new savestate, RAM writes,
manual gameplay or long replay. Local scripts/evidence stay in ignored
`build/bizhawk-controlled-harness`; production and milestone scope are unchanged.

ACCEPTANCE: Hash-verified original inputs, reproducible CLI/Lua launch with
stage logs and bounded exit, observed state frame and controller input,
one-frame delta plus A372 hit or known FF13CC transition; otherwise identify
the exact failing stage and distinguish verified APIs from untested options.

RESULT: CLI ROM/config/Lua startup, explicit Lua state load at frame 2117,
three neutral settling frames, exact one-frame Right at 2120 -> 2121,
A372 execution (six hits), register reads and FF13CC write hooks all passed.
FF13CC changed 00000000 -> 00880901. Three successful Right logs, including a
run alongside a held EmuHawk process, were byte-identical. Original ROM/state
hashes remained exact; no native UI automation or new savestate was needed.

DIAGNOSIS: The CUA error is not a runtime blocker. Existing mixed-prefix
`joypad.set({ ["P1 Right"] = true }, 1)` fails to assert Right; the verified
call is `joypad.set({ Right = true }, 1)`. Neutral input produces the same
oracle: frame 2121 is lagged with zero input-poll callbacks, so no gameplay
causality is claimed. CLI --load-state entered Lua at 2118; reload inside Lua
is the reliable frame synchronization. Single-instance forwarding only loads
args[0]; isolated configuration with that mode disabled worked with coexistence.

EVIDENCE: `docs/reports/BIZHAWK_CONTROLLED_HARNESS_DIAGNOSIS.md` and ignored
`build/bizhawk-controlled-harness` contain the exact workflow, stage logs,
early probe-assertion corrections, controls and version-specific sources.
Production source and existing probes remain unchanged. No native build or CI
execution was performed for this diagnostic/documentation task.

POLICY UPDATE: The independent harness is now the required runtime entrypoint
for future M12 work. The older `Right -> A372` wording is superseded: neutral
input reaches the identical lag-frame transition and no input-poll callback;
only controller installation and source-side reachability are proven.

# 2026-09-12 — M12 controlled runtime root capture — NEUTRAL CAUSAL RESULT

After a fresh baseline `run.ps1` returned `result=PASS`, a one-frame developer-
only root probe compared Right and all-false control from exact QuickSave1.
Both arms reached A196/A342 once and A372 six times at frame 2120 with identical
D2/D5/A5 values, FF1858=00, and FF188A/FF188C updates to 0010/0080 at frame
2121. Right was visible in the hook's controller state; the neutral arm was
not. No FF1858 write occurred. The SAT source ended at 00 88 09 01 in both.

RESULT: the restored runtime evidence confirms the controlled producer/root
path but gives a negative causal control. Right -> A372/FF13CC remains
unproven; no input-poll callback and the lag frame remain. No ROM ownership,
semantic label, savestate or C++ change was made.

RE-RANK: next autonomous M12 fork is static exact consumer closure for the 12
direct FF1858 accesses plus FF188A/FF188C callers; second is the blocked
03BDA6 consumer boundary; input causality is currently low-value/blocked until
a separately validated input-polling state exists. Full values and limits are
in `docs/reports/THOR_M12_CONTROLLED_RUNTIME_ROOT_CAPTURE.md`.

VALIDATION: Live controlled/control/coexistence runs, final original hashes,
`git diff --check`, PowerShell syntax and tracked-source size scan passed.
Local Lua/PowerShell files are 106/81 lines. The stock recursive CMake size
glob was stopped without a result; tracked sources were checked separately.

# 2026-09-12 — M12 static 0xA372 shadow-SAT producer grammar — POSITIVE STRUCTURAL RESULT

TASK: Close one producer family only, statically, around the existing runtime
write at `0xA372`. Do not replay, extend captures, chase B730/0x3820, scan
graphics globally, or assign frame/object semantics without proof.

RESULT: Exact Ghidra/raw-ROM cross-checks close the producer routine as
`0xA342..0xA438` (end exclusive), with incoming callers `0xA19C` and
`0xA6A0`, `RTS` at `0xA436`, and loop edges `0xA37C -> 0xA36C` (6 records) and
`0xA3A6 -> 0xA396` (3 optional records). PC-relative LEA semantics correct the
previous shorthand roots: exact targets are `0xA438` and `0xA480`; labels
`0xA43A`/`0xA482` are root+2 aliases. This also explains the existing
`0x00880901` observation: record 0 at `0xA438` supplies upper `0x008809`, and
`D5.B=0x01` supplies the low byte.

The two exact roots are finite 9-record tables, each stride `0x08`, ending at
`0xA480` and `0xA4C8` respectively. Fields are `+0` longword, `+4` word, and
`+6` word. D2 construction is
`(record[+0] & 0xFFFFFF00) | ((D5.B + 1) & 0xFF)`. A5 starts at
`0x00FF13CC + sign_extended_word(0x00FF188C)`, advances 8 bytes per record,
and writes the updated offset/counter to `0xFF188C`/`0xFF188A`.

`FF1858` is proven locally as a boolean root selector: zero selects `A438`,
nonzero selects `A480`, with readers at `A358` and `A4F2`. The sibling routine
`A4C8..A69C` has caller `A8B1E`, shares the root selection and writes the
selected root longword at `A4FE`, but is not merged into the A342 boundary.
The exact bounded join is producer -> `FF13CC` shadow SAT -> existing DMA
`0x27EC` -> VRAM SAT `0xD000`; same-frame VRAM publication remains unclaimed.
Frame/object/animation/piece semantics remain fail-closed.

TOOLING: Added payload-free `src/tools/m12_a372_shadow_sat_producer.py`, its
regression test, CTest registration, and report
`docs/reports/THOR_M12_A372_SHADOW_SAT_PRODUCER.md`. No ROM, state, capture,
decoded asset, or ownership promotion was added. `SOURCE_OWNED` remains
`1,475,368 / 3,145,728 = 46.9006856283%`.

VALIDATION: direct Python test and `py_compile` passed; Debug and Release
focused M12 CTest passed 9/9; the fresh Ubuntu 24.04/GCC 13.3 GNU tree
configured, built, linked, and passed its five static M12 tests. Release
rebuild passed. The current MSVC 18.9 Debug rebuild remains blocked by the
pre-existing `src/core/ram_flag_routine.cpp` `std::to_string` error, so the
Debug full CTest tree could only run the available tests; no code in that
routine was changed here. Tracked source-limit scan and `git diff --check`
passed.

NEXT/STOP: This bounded static producer family is closed. Stop; any selector
join or semantic frame grammar requires a materially different, separately
authorized evidence class.

# 2026-09-12 — M12 resumed Ali frame transition from BizHawk F1 — POSITIVE SOURCE-SIDE RESULT

TASK: Resume exactly one controlled Ali idle-to-first-movement transition from
the manually created BizHawk F1/slot-1 state. Use the canonical ROM, bounded
SAT/source/selector hooks only, and stop at the first structural change.

RESULT: The state loaded successfully from
`C:\Dev\SegaThorTools\BizHawk-2.11.1-win-x64\Genesis\State\Beyond Oasis (U) [!].Genplus-gx.QuickSave1.State`
through `savestate.loadslot(1, true)`. The exact ROM was
`C:\Github\Sega-Thor\build\reference\Beyond Oasis (USA).bin`, SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
After three no-input settling frames, State A was emulator frame `2120`.
The Right arm reached first State B at frame `2121`; later neutral control
testing produced the identical A372/FF13CC transition, so Right causality is
not proven and this historical wording is superseded.

PROVEN CHANGE: SAT shadow source `0xFF13CC..0xFF13CF` changed from
`00 00 00 00` to `00 88 09 01`; SAT VRAM `0xD000`, selector/descriptor, and
bounded `0x03BDA6` relative bytes remained unchanged. The bounded write watch
reported callback PC `0xA374`, value `0x00880901`; because BizHawk reports the
next fetch PC, exact store is `0xA372` (`MOVE.L D2,(A5)+`). Static context
closes the local writer inputs to default ROM root `0xA43A` and conditional
alternate root `0xA482` (`0xFF1858` controls the choice), with a six-record
`DBF D0=5` copy loop. Semantics stay neutral.

RESOURCE/RENDER RESULT: No `0x3820` and no `0xB730` execution occurred in this
first transition capture. The known DMA event contract around the state is
`0x000027EC`, source `0xFF13CC`, destination `0xD000`, 64 words, but no new
SAT VRAM change followed State A. This is a positive source-side transition,
not proof of a rendered SAT change, resource load, or finite frame sequence.

TOOLING NOTE: The first local JSON had a serialization-only `b730_calls`
counter initialization defect; it was corrected in the developer-only probe
without rerunning the one transition. No ROM, state, capture, decoded asset,
or payload was added. `SOURCE_OWNED` remains
`1,475,368 / 3,145,728 = 46.9006856283%`.

NEXT/STOP: Stop this replay-only pass. Do not run another direction or
animation. Any further attempt must use a materially different evidence class
or a separately bounded static closure.

VALIDATION: `git diff --check` passed; focused M12 CTest passed 6/6 in both
existing Debug and Release trees; manual source-limit scan passed with the
new Lua probe at 288 lines. The pushed publication CI run `34690039459` for
the predecessor commit completed successfully: Build, Test, Post Checkout,
and Complete all passed.

# 2026-09-12 — M12 targeted Ali frame-transition backtrace — BOUNDED NEGATIVE RESULT

TASK: Attempt exactly one controlled Ali idle-to-first-movement transition
from a reusable local BizHawk gameplay state, then backtrace only the bounded
SAT/B730/selector path. Do not start another replay campaign, add generic
tracing, force state, or begin M13/ASM-to-C++ work.

RESULT: The known BizHawk 2.11.1 installation was verified at
`C:\Dev\SegaThorTools\BizHawk-2.11.1-win-x64\EmuHawk.exe`, with the canonical
ROM present and matching the recorded SHA-256. The published argv was attempted
once from the installation directory. It returned code 0 but emitted only
`parsing command-line flags` and created no capture. A GUI-only launch of the
same pre-existing executable produced a responsive `BizHawk` window, proving
the installation is present and starts; the GUI was stopped after this check.

SETUP EVIDENCE: No BizHawk savestate was found in the checked local locations,
including `Genesis\State`; no savestate was created or committed. The Windows
UI-control backend returned `Trusted RPC service is not configured: sky`, so
controlled navigation and one-time gameplay-state creation were unavailable.
No equivalent replay, state forcing, global RAM/read trace, or unrelated
animation experiment was attempted.

CLASSIFICATION: `ALI_TRANSITION_SETUP_BLOCKED`. State A/B frames, first causal
changed value, writer PC, and Ali frame-table/root are therefore unobserved.
The existing proven selector/descriptor -> child/relative ->
`0x03B448 -> 0xB730 -> SAT` chain remains unchanged. `SOURCE_OWNED` remains
`1,475,368 / 3,145,728 = 46.9006856283%` before and after. This task stops at
the exact evidence-class boundary; the next step requires either restoration
of a usable BizHawk UI/save-state path or a separately authorized controlled
state/static-consumer evidence class.

VALIDATION: `git diff --check`, Python compilation, and direct analyzer tests
passed. Focused M12 CTest passed 5/5 in both existing Debug and Release trees.
The existing published full Debug/Release CTest evidence (152/152 each) is
preserved. WSL Ubuntu 24.04 / GCC 13.3 configure and Release build/link passed;
the five focused WSL M12 tests passed. The WSL source-size CTest was bounded
and interrupted during its slow `/mnt/c` scan; the native
`cmake -P tests/check_file_limits.cmake` scan then passed. No source structure
changed, so FILE_MAP and ROADMAP remain unchanged.

# 2026-09-12 — M12-GFX-MAX bounded checkpoint static provenance audit — CHECKPOINT PUBLISHED

TASK: Stop the current M12-GFX-MAX run at the bounded publication checkpoint.
Preserve the already obtained BizHawk evidence, record the final exact static
negative evidence, and do not start another graphics campaign, detector,
runtime scenario, M13, or ASM-to-C++ work.

RESULT: The exact developer-only slices add no source-owned bytes. `0x001432`
calls `0x03BF86` without defining `A1`; the path reaches `0x03C074`, where
`A0=0x00172168` is direct but `A1` remains inherited. `0x03D59A`/`0x03D5AE`
is a RAM-mediated entity record path with no local ROM-source proof.
`0x03E61A` calls before local `A0/A1` definitions; only later arms are the
already-owned `0x0016943C -> 0x00FF2FA8` calls.

CHECKPOINT STATE: `SOURCE_OWNED` is `1,475,368 / 46.9006856283%` before and
after this final static audit. The seven runtime-unclosed dynamic blockers are
unchanged: `0x00D54A`, `0x00D650`, `0x02DB52`, `0x02F6A0`, `0x03C07C`,
`0x03D5AE`, and `0x03E61A`. The prior BizHawk run remains the only runtime
evidence: 1,800 frames, two byte-identical captures, 13 `0x3820` hits, nine
caller-paired hits (`0x03B236=4`, `0x03B28A=4`, `0x03B2FE=1`), all paired
`A1=0x00FF316C` with ROM `A0`, and eight already-owned source spans totaling
25,636 compressed bytes.

ARTIFACTS: ignored slice hashes and the full positive/negative evidence are
recorded in the M12-GFX-MAX report and reverse-engineering log. No ROM,
decoded asset, generated payload, or developer-only slice artifact is tracked.

VALIDATION: focused M12 helper tests passed. The previously published
implementation already passed MinGW Debug/Release builds and full Windows
Debug/Release CTest (`152/152` each, including source-size); this docs-only
diff passed `git diff --check`. A fresh local CTest rerun reached the common
workspace-wide `project_file_line_limit` scan but was stopped after it did not
complete in bounded time; no source files changed. GitHub Actions then passed
Build, Test, and Complete for the pushed checkpoint.

PUBLICATION: docs-only checkpoint commit
`c11a7ee80b5720d38c68781d6e1ce580061cd21e` is on `origin/main`; exact
GitHub Actions CI run `34671385113` passed Build, Test, and Complete.

# 2026-09-12 — M12-GFX-MAX targeted runtime provenance for 0x03B1D0 — CHECKPOINT PUBLISHED

TASK: Use only the existing frozen BizHawk natural-input scenario to resolve
the three remaining `0x03B1D0` dynamic `0x3820` arms far enough to prove or
reject ROM source provenance. Do not force PC/register/RAM state, decode or
commit copyrighted payload, promote from decoder validity alone, or begin M13.

ACCEPTANCE CRITERIA: observe the exact caller PCs and `0x3820` entry;
capture `A0/A1` without emulator writes; repeat the 1,800-frame scenario;
require byte-identical captures and canonical ROM identity; independently
decode exact source ends; require current manifest ownership; and keep any
non-ROM source fail-closed.

IMPLEMENTATION: added the developer-only
`src/tools/re_bizhawk_m12_gfx_provenance.lua` targeted register probe and the
fail-closed `src/tools/re_m12_gfx_runtime_provenance.py` validator, with
synthetic contract tests. The probe watches only the ten unresolved caller PCs
and `0x3820`, uses the existing `m11_8_natural_reachability_v1` schedule, and
emits metadata/source probes only; no decoded asset is written.

PROVEN: BizHawk 2.11.1, canonical ROM SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`, hardware
reset, 1,800 frames, and two byte-identical captures with JSON SHA-256
`D61150BD828DB22CA35A2E6C93A103BDC04ABE667883B94339FD05390924000B`.
There are 13 `0x3820` target hits and nine caller-paired hits: `0x03B236=4`,
`0x03B28A=4`, and `0x03B2FE=1`. Every paired target has `A1=0x00FF316C` and
ROM `A0`; eight unique source spans terminate exactly and are already
`LOCAL_ROM_DERIVED_ASSET`, totaling 25,636 compressed bytes. The runtime
evidence therefore closes these three producer blockers without adding
SOURCE_OWNED bytes, and leaves seven dynamic blockers.

VALIDATION: focused Python probe/validator tests and the live validator pass.
Both MinGW Debug and Release builds pass, and full Windows Debug/Release CTest
passes `152/152` in each configuration, including the source-size gate. The
GNU/Linux-equivalent Ubuntu 24.04 / GCC 13.3 build and link pass. A full WSL
CTest attempt reached the same workspace-wide source-size scan but was stopped
because the mounted `C:/` scan did not complete in bounded time; the required
Linux build/link gate is green and the Windows source-size gate is green.
`git diff --check` passes with only Git's LF/CRLF normalization warnings.

PUBLICATION: implementation commit
`7012fd7861b5bd09164c46aa46152337dd4357fd` is on `origin/main`; exact
GitHub Actions CI run `34670755667` passed Build, Test, and Complete. The
following metadata-only commit records this result and is the final repository
HEAD for this checkpoint.

CHECKPOINT: this bounded publication intentionally stops here. No additional
graphics campaign, runtime scenario, detector, M13 work, or ASM-to-C++
migration is authorized by this checkpoint. The seven remaining dynamic
producers remain explicitly fail-closed for a future, separately authorized
task.

# 2026-09-12 — M12-GFX-MAX residual 0xD406 caller census — IN PROGRESS

TASK: Classify the five exact direct `0x00D406` callers left after the screen
continuations, descriptor-shaped candidates, and proven sibling helpers were
accounted for: `0x02DB40`, `0x02DD8C`, `0x02DE58`, `0x02DFC2`, and `0x02E084`.
Do not infer ROM ownership from zero selectors or inherited registers.

ACCEPTANCE CRITERIA: verify the canonical total of 173 direct `0x00D406` calls;
verify all five residual sites and exact bounded body fingerprints; classify
their local edges and source blockers; make no promotion; and provide a
deterministic regression-tested artifact.

RESULT: `0x02DB40` is the already-known `D406` post-source continuation into
`0x003820` at `0x02DB52`, with later `0x00D7C0` cleanup. `0x02DD8C` and
`0x02DE58` share the exact 28-byte `A2`-inherited/zero-selector body;
`0x02DFC2` is a 10-byte zero-selector wrapper; and `0x02E084` is a 12-byte
zero-D0-to-A0 wrapper. None defines a ROM source and no bytes are promoted.

ARTIFACT: ignored local `build/m12-gfx-d406-residual-census.json`, schema
`oasis.m68k.m12-gfx-d406-residual-census.v1`. Canonical ROM SHA-256 is
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

VALIDATION: Python compilation, focused residual-census regression test,
canonical-ROM generation, and deterministic repeat passed. Both JSON artifacts
have SHA-256
`544986B32D7B0917FDEC5FE35574BCAF004D6D65E763D15317C90BB2C6B1C881`.
Debug/Release focused builds and full Windows CTest passed (`150/150` each,
including source-size); the WSL GNU/Linux build/link and eight relevant
graphics helpers passed (`8/8`); and `git diff --check` passed. Source files
remain within the 500-line limit. Implementation commit
`761df67298c8e2214a38f747b6ef36f9a613f46b` is pushed and exact CI run
`34668962782` passed. This residual caller classification is published; no ROM
or decoded asset was added.

NEGATIVE EVIDENCE: the only residual dynamic source relation is the already
recorded `0x02DB40 -> 0x02DB52` post-source continuation. The other four are
inherited/zero-source wrappers and cannot establish a ROM resource boundary.

NEXT: after publication, continue only if new evidence reaches the remaining
dynamic `0x3820` producers; the five residual `0x00D406` sites are explicitly
classified and should not be reopened without new source evidence.

# 2026-09-12 — M12-GFX-MAX proven sibling-helper census — IN PROGRESS

TASK: Classify the remaining directly reachable sibling helpers named by the
graphics graph: `0x00D950`, `0x002CBC`, and `0x002E1E`. Prove their complete
canonical direct-xref sets and bounded body contracts, distinguish VDP/RAM
helpers from ROM-source loaders, and do not promote bytes from helper shape.

ACCEPTANCE CRITERIA: the canonical scan must find the exact direct call set for
each target; each bounded body must match its exact byte fingerprint and local
edges; the report must explicitly state whether a ROM source/boundary exists;
the result must be deterministic and regression-tested.

RESULT: The census finds 12 direct calls to `0x00D950`, four to `0x002CBC`,
and 24 to `0x002E1E`, for 40 direct sibling-helper xrefs. Exact bounded bodies
are `[0x00D950,0x00D9A4)` (84 bytes), `[0x002CBC,0x002CE4)` (40 bytes), and
`[0x002E1E,0x002E78)` (90 bytes). `0x00D950` is a VDP-transfer helper with
local `0x00D962`; `0x002CBC` is a VDP-fill helper; and `0x002E1E` is a RAM/state
helper with local `0x002F6E`. None contains a ROM source operand or a
`0x003820` decompressor edge. Classification is complete for these three
helpers; promotion is zero.

ARTIFACT: ignored local `build/m12-gfx-sibling-census.json`, schema
`oasis.m68k.m12-gfx-sibling-census.v1`. Canonical ROM identity is checked by
SHA-256 `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

VALIDATION: Python compilation, focused sibling-census regression test,
canonical-ROM census generation, and deterministic repeat passed. Both JSON
artifacts have SHA-256
`25A787BDF61088BCD18870AD3293FDDF78F0F5B4C1D8E0B55CCF87EB53E099FC`.
Debug/Release builds and full Windows CTest passed (`149/149` each, including
source-size); the WSL GNU/Linux build/link and seven relevant graphics helpers
passed (`7/7`); and `git diff --check` passed. Source files are within the
500-line limit. Implementation commit `8a0d141418abc8f3214efb9b87131008c470c8ef`
is pushed and exact CI run `34668227200` passed. This helper classification is
published; no ROM or decoded asset was added.

NEGATIVE EVIDENCE: these are VDP/RAM/state helpers, not independent ROM-source
loaders. Their exact xrefs are now accounted for, but no resource boundary or
SOURCE_OWNED bytes can be inferred from them.

NEXT: continue from the ten unresolved `0x3820` producers and the 27 unmatched
`0x00D406` screen/non-screen candidates; do not reopen these three classified
helpers unless new caller/source evidence changes their contracts.

# 2026-09-12 — M12-GFX-MAX direct 0x36D4 wrapper classification — IN PROGRESS

TASK: Classify the directly reachable `0x0036D4` graphics wrapper after the
`0x37D2` and `0xD3B2` family closures. Prove its complete direct caller set,
bounded body, decompressor/postprocessor edges, RAM destination, and exact
source blocker. Do not convert inherited A0 into ROM ownership.

ACCEPTANCE CRITERIA: the canonical ROM scan must find all direct
`JSR 0x0036D4` sites; each caller must have exact local setup; the body must
have exact start/end and proven `0x3820`/`0x2CBC` edges; source provenance must
remain explicit and fail-closed; output must be deterministic and tested.

RESULT: The exact scan finds six callers at `0x03D25E`, `0x03D2D8`,
`0x03D512`, `0x03DD70`, `0x03DF14`, and `0x03E544`. Their exact D0 values are
`0x20`, `0x4000`, `0x20`, `0x20`, `0x20`, and `0x20`, all with the reviewed
`A5 = 0x60000003` setup. The bounded wrapper is `[0x36D4,0x372A)` (86 bytes),
sets `A1 = 0x00FF2FA8`, calls `0x3820`, then `0x2CBC`; A0 remains inherited.
Classification is `CALLER_A0_NOT_ROM_PROVEN`; promotion is zero.

ARTIFACT: ignored local `build/m12-gfx-36d4-census.json`, schema
`oasis.m68k.m12-gfx-36d4-census.v1`, SHA-256
`57A39EE08090CCB233EAA61962E6742CA61491CF98A719E962B2004CB49E959A`.
The deterministic repeat has the identical SHA-256.

VALIDATION: Python compilation, focused wrapper-census test, canonical-ROM
census generation, deterministic repeat comparison, exact body/caller checks,
and negative source classification passed. Debug/Release builds and full
Windows CTest passed (`148/148` each, including source-size); the WSL
build/link and five relevant graphics helpers passed (`5/5`); `git diff --check`
passed. Commit, push, and CI validation remain for this transaction.

NEGATIVE EVIDENCE: the wrapper is a proven graphics-related RAM-mediated
consumer, but no canonical ROM source or exact compressed boundary is proven
at its inherited A0 input; no bytes are promoted.

NEXT: validate and publish this classification, then continue with remaining
dynamic `0x3820` producers and non-screen `0x00D406` paths. The other reviewed
sibling helpers remain evidence-only until a bounded source contract exists.

# 2026-09-12 — M12-GFX-MAX direct 0xD3B2 indexed-loader/root closure — IN PROGRESS

TASK: Close the proven indexed `0x00D3B2` graphics-loader family after the
`0x37D2` sibling census. Scan every exact direct caller, verify immediate
selector/D1 setup, reconstruct the complete fixed-width root table, and match
all finite children to existing manifest ownership. Do not promote already
owned bytes or infer semantic roles from table shape alone.

ACCEPTANCE CRITERIA: the canonical ROM scan must find the complete direct
`JSR 0x00D3B2` set; every site must have exact immediate D0/D1 evidence; the
108-entry table must verify entry 0 null and entries 1..107 finite; every child
must match one exact owned manifest span; output must be deterministic and
regression-tested.

RESULT: The exact scan finds seven direct callers at `0x02CFAA`, `0x02CFB8`,
`0x02D410`, `0x032174`, `0x032182`, `0x032884`, and `0x032892`. Their exact
selectors are `3`, `4`, `0x57`, `0x23`, `0x24`, `0x23`, and `0x24`; D1 is
`0x4000` or `0x5000`. The root `[0x05CE96,0x05D046)` has 108 entries, with
entry 0 null and all 107 finite children matched to `LOCAL_ROM_DERIVED_ASSET`
spans totaling 238,087 compressed bytes. Promotion is zero.

ARTIFACT: ignored local `build/m12-gfx-d3b2-census.json`, schema
`oasis.m68k.m12-gfx-d3b2-census.v1`, SHA-256
`DDBA1EF1EC7D5B89830DA74A0835BC6506D8BBB7C2283440AD5BCE8B32653895`.
The deterministic repeat has the identical SHA-256.

VALIDATION: Python compilation, focused census test, canonical-ROM census
generation, deterministic repeat comparison, exact selector/root checks, and
manifest ownership matching passed. Debug/Release builds and full Windows
CTest passed (`147/147` each, including source-size); the WSL build/link and
four relevant graphics helpers passed (`4/4`); `git diff --check` passed.
Commit, push, and CI validation remain for this transaction.
Implementation commit `3fe64956be986bfe3e76f3d6a4251abba4525865` is pushed and
its exact CI run `34667380039` passed; the current change only finalizes report
publication identifiers.
Implementation commit `bcbd3cdd89d1ef45e2864022fbf8e294af0d1b61` is pushed and
its exact CI run `34666468223` passed. Publication commit
`ad1ca215348874d0a69e07168a2dc1296aceedbe` and CI run `34666594438` also
passed; the current commit only finalizes these exact identifiers.

NEGATIVE EVIDENCE: all finite indexed resources were already owned by the
existing resource-boundary closure; this step adds no SOURCE_OWNED bytes and
does not promote from selector or table shape alone.

NEXT: after validation, continue from remaining dynamic `0x3820` producers
and non-screen `0x00D406` paths; sibling helper xrefs remain evidence-only
until their source contracts are independently bounded.

# 2026-09-12 — M12-GFX-MAX direct 0x37D2 wrapper-family closure — IN PROGRESS

TASK: Extend the graphics loader census from the 52 absolute `0x3820` callers
to the direct `0x37D2` wrapper family. Prove every direct wrapper call's local
source setup where available, revalidate exact Ancient boundaries and current
manifest ownership, and keep inherited/post-state paths fail-closed. Do not
promote already-owned bytes, start M13, or migrate ASM to C++.

ACCEPTANCE CRITERIA: the canonical ROM scan must find the complete direct
`JSR 0x0037D2` set; each site must be classified as direct-ROM or inherited;
direct-ROM sites must have exact `LEA source/A1`, immediate D0, deterministic
Ancient end, destination, and ownership evidence; output must be deterministic
and regression-tested.

RESULT: The exact scan finds seven direct wrapper calls at `0x00D9B2`,
`0x02F6B6`, `0x03C0DE`, `0x03C2BE`, `0x03C632`, `0x03CA40`, and `0x03CD54`.
Five have exact direct-ROM setup and deterministic source spans:
`0x1744EE..0x17502A`, `0x18CCD0..0x18CF97`, `0x18EC26..0x18F214`,
`0x1911EA..0x191F09`, and `0x19911A..0x199CBA`. They total 11,440 bytes and
are already `LOCAL_ROM_DERIVED_ASSET`, so promotion is zero. `0x00D9B2` remains
an inherited-A6 source and `0x02F6B6` a `0x00D406` post-source continuation;
both remain blocked.

ARTIFACT: ignored local `build/m12-gfx-37d2-census.json`, schema
`oasis.m68k.m12-gfx-37d2-census.v1`, SHA-256
`EC247A5304BE12B458515CB7CE081A2FF54A1EA0C9DED51AEC7FBEC8F975068E`.
The deterministic repeat has the identical SHA-256.

VALIDATION: Python compilation, focused wrapper-census test (`2/2`), actual
canonical-ROM census generation, deterministic repeat comparison, and source
ownership checks passed. Debug and Release builds passed; full Windows Debug
and Release CTest passed (`146/146` each, including the source-size gate);
GNU/Linux build/link and the three relevant graphics CTest helpers passed
(`3/3`); `git diff --check` and the source-size gate passed.

NEGATIVE EVIDENCE: no new source range is promotable from this family because
all five direct spans are already owned; the two inherited paths do not close a
canonical ROM source. The wrapper census therefore changes loader coverage and
blocker accounting, not SOURCE_OWNED totals.

NEXT: continue the graphics graph from the ten unresolved dynamic `0x3820`
producers and directly reachable non-screen `0x00D406` paths; do not use
decoder validity or wrapper adjacency as ownership.

# 2026-09-12 — M12-GFX-MAX verified 0x00D406 continuations — IN PROGRESS

TASK: Close the bounded `0x00D406` screen continuation candidates with
instruction-level 68000 control-flow and `A1` evidence. Keep the relation
non-owning; do not promote ROM bytes from continuation proof alone.

RESULT: The closed verifier confirms 15 of the 17 previously missing screen
continuations. Every decoded path reaches the exact `JSR abs.l,0x00D406` and
none writes `A1`; 14 are straight fall-through and `0x02E994` is a conditional
branch whose taken and fall-through paths both reach `0x02E99A`. The only
remaining screen continuation sites are `0x038FD6` and `0x03959A`. No ROM
ownership changed.

ARTIFACT: local ignored `build/m12-gfx-loader-census.json`, schema
`oasis.m68k.m12-gfx-loader-census.v1`, SHA-256
`3E9F5D9D11AA3B38500859E35449CCEE945CA0BDBA993C50A8F0D461CA40270E`.

VALIDATION: Python compilation, direct harness and focused regression tests
(`4 passed`), deterministic canonical-ROM census generation, Debug/Release
builds, full Windows Debug/Release CTest (`144/144` each, including the source
size gate), WSL Release build and CTest excluding only the slow Linux source
size gate (`143/143`), `git diff --check`, and source files below 500 lines.
The full Linux CTest attempt reached `project_file_line_limit` and was stopped
after approximately 90 seconds of `/mnt/c` scanning; this is an environment
limit, not a code failure.

NEGATIVE EVIDENCE: `0x038FD6` is a code-like `MOVEM.L`/RAM-state entry with no
bounded `0xD406` continuation. `0x03959A` is a code-like `MOVEM.L`/`LEA.L`
entry with a separate unbounded `0x0395D8` sibling call. Both remain blocked
on parent/caller `A1` provenance; neither is promoted. The machine census now
stores these blocker reasons explicitly.

NEXT: investigate `0x038FD6` and `0x03959A`, then independently close the five
descriptor-shaped unmatched calls before returning to the ten dynamic `0x3820`
producers.

# 2026-09-12 — M12-GFX-MAX bounded 0x00D406 relation census — IN PROGRESS

TASK: Continue the M12-GFX-MAX graphics closure from the published exact
`0x00D406` census. Record bounded relations for missing screen continuations
and descriptor-shaped unmatched calls, while preserving the fail-closed
ownership boundary. Do not start M13, migrate ASM to C++, or promote bytes from
adjacency alone.

RESULT: The machine census now records 15 of 17 missing screen expected sites
with a nearest exact direct call within 16 bytes, leaving `0x038FD6` and
`0x03959A` without a bounded successor. It also records five unmatched calls
whose `call - 0x1A` prefix is descriptor-shaped with a nonzero in-ROM pointer
and four resource IDs in the closed screen domain: records at `0x02CF82`,
`0x02D3E8`, `0x02DCD8`, `0x02E0D4`, and `0x02E1D8`. These are explicit
`*_CANDIDATE` evidence only; no ROM ownership changed.

ARTIFACT: local ignored `build/m12-gfx-loader-census-next.json`, schema
`oasis.m68k.m12-gfx-loader-census.v1`, SHA-256
`FCFF91F123FABE0330DCFD0CC364BBDA0C87CA214E74C7BD552235CD3BA9A233`.

VALIDATION: Python compilation, focused regression tests (`3 passed`),
canonical-ROM census generation, `git diff --check`, Debug/Release builds, and
full Debug/Release CTest (`144/144` each) passed. The Linux Release build and
the other 143 Linux CTest cases passed; the Linux `project_file_line_limit`
case was stopped after more than ten minutes of `/mnt/c` filesystem scanning.
The same size gate passed in both Windows configurations (`162.26 s` Debug,
`161.14 s` Release), so this is an environment/runtime limitation, not a
reported source violation.

NEXT: inspect the 15 bounded continuations with instruction-level control-flow
and `A1` evidence, then resolve the two remaining screen sites and five
descriptor-shaped records before returning to the ten dynamic `0x3820` sources.

IMPLEMENTATION: focused commit
`641abc9bf0ca8a0e30254e551e8711f714de4f77` is on `origin/main`; exact GitHub
Actions CI run `34656525243` passed build and test. A final docs-only
publication commit `640a1d1266d305e99e9d7e51688a8fbbcace8cd1` is on
`origin/main`; exact publication CI run `34656666017` passed build and test.

# 2026-09-12 — M12-GFX-MAX exact 0x00D406 loader census — IN PROGRESS

TASK: Continue the unfinished M12-GFX-MAX graphics closure by separating the
complete direct `0x00D406` loader xref set from the already closed screen
descriptor census. Do not repeat the whole-ROM Ancient detector sweep, promote
decoder-validity candidates, add ROM/decoded graphics, start M13, or migrate
ASM to C++.

ACCEPTANCE CRITERIA: scan the canonical ROM with the exact `JSR abs.l` encoding;
cross-check every direct call against every screen descriptor use; retain the
unmatched direct calls and missing direct continuations as explicit evidence;
make no ownership change from xrefs alone; provide deterministic JSON and a
regression test.

RESULT: `src/tools/re_m12_gfx_loader_census.py` records all 173 direct
`0x00D406` call sites. 146 match a screen descriptor at `descriptor + 0x1A`.
17 unique screen expected sites have no direct `0x00D406` encoding, and 27
direct calls do not match a screen descriptor relation. The 27-call list and
17-site missing list are published in
`docs/reports/THOR_M12_GRAPHICS_MAXIMUM_CLOSURE.md`. The census performs no
promotion and preserves the canonical ROM SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

ARTIFACT: ignored local `build/m12-gfx-loader-census.json`, schema
`oasis.m68k.m12-gfx-loader-census.v1`, SHA-256
`69C5EF8C9DC2A1DA88F0058C1F8E7FC688901B4B1134A731874574034F969C39`.

VALIDATION: Python compilation, direct-census regression tests, canonical-ROM
census generation, deterministic two-run JSON hashing, Debug/Release builds,
full Debug/Release CTest (`144/144` each), GNU/Linux-equivalent WSL Release
build, Linux census helper CTest, `git diff --check`, and the source-code line
limit gate passed. No ROM or decoded payload was added.

NEXT: bounded provenance analysis of the 27 unmatched direct calls and the 17
missing direct screen continuations; do not promote until `A1`, record format,
and exact source boundaries are independently proven.

IMPLEMENTATION: focused commit
`85131ce42619fd9318f59d09dfbb88a69ac33d0a` is on `origin/main`; exact GitHub
Actions CI run `34653604741` passed build and test. A docs-only publication
commit `d153ee90fc843f184562f7a34243eec1b10abeed` is on `origin/main`; exact
publication CI run `34653772559` also passed build and test.

# 2026-09-12 — M12-GFX-2 0x3820 caller-to-asset closure — COMPLETE

TASK: Starting from the published `bdbbfada3dd5de3ffc042302afc2fb2770a68695`
baseline, prove the existing 0x3820 ABI, classify all 52 static call sites,
recover bounded direct/table/sequential source sets, canonicalize only caller-
derived Ancient streams, and feed exact results into the existing M12
materialization/Carver accounting. Do not repeat the M12-GFX-1 whole-ROM scan,
require CRAM/SAT, add heuristic detector families, begin M13, or migrate ASM
to C++.

ACCEPTANCE CRITERIA: every call site has a containing bounded routine/family,
source and destination origin, source-set kind, selector/table status,
confidence, and explicit blocker when unresolved; exact table-derived and
sequential resources have half-open boundaries, declared compressed sizes,
decompressed sizes, output hashes, overlap status, and canonical ROM proof;
only wholly UNKNOWN non-code spans meeting the caller-derived ownership
contract are promoted; the final report records ABI, census, caller graph,
resource/table accounting, Carver B/F/G before/after, sibling-loader result,
all blockers, hashes, validation gates, implementation SHA, and exact CI.

BASELINE: canonical USA ROM is 3,145,728 bytes with SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`;
SOURCE_OWNED is `1,427,873` bytes (`45.3908602397%`).

RESULT: Reused the published M12-GFX-1 census without repeating its whole-ROM
scan. The 52 callers are cataloged in the machine report with bounded routine,
source/destination origin, source-set kind, table/selector details, confidence,
and explicit blockers. Exact direct, 107-entry table-derived, and sequential
Ancient resources carry half-open boundaries, declared compressed/decompressed
sizes, output SHA-256, overlap metadata, baseline owner, and canonical-ROM
proof. Ten wholly UNKNOWN sequential spans totaling `47,389` bytes were
promoted as `LOCAL_ROM_DERIVED_ASSET`; SOURCE_OWNED is now `1,475,262`
(`46.8973159790%`). Carver B/F/G changed from `113/623,036`,
`56/58,789`, `589/1,036,030` to `113/623,036`, `56/58,789`,
`588/988,641`. All 107 table targets were already baseline-owned. The 64 KiB
target remains intentionally unmet because ten dynamic source producers are
not ROM-proven; no generic detector, visual inference, CRAM/SAT dependency,
M13 work, or ASM-to-C++ migration was added.

ARTIFACTS: `src/tools/m12_gfx_caller_closure.py` is 471 lines and
`tests/m12_gfx_caller_closure_test.py` is 47 lines. The generated canonical
machine report was `build/m12-gfx2-caller-closure-d/caller_closure_report.json`
with SHA-256 `d2cecf6d34472505bac16f372d963f5f1b1fc43283d9307a87a3611269c96808`;
its materialized rebuilt ROM matched the canonical ROM byte-for-byte. The
tracked Markdown report contains no ROM or decoded payload.

VALIDATION: Python compile, dedicated caller-closure regression, `git diff
--check`, and the source-code limit check passed. Full Debug and Release
MinGW builds passed. Release CTest passed `142/142`; Debug CTest passed
`142/142` when rerun sequentially after the initial concurrent-run file-lock
race. GNU/Linux CMake configure, build, and link passed; Linux CTest reached
the workspace-wide file-limit test but was stopped after more than seven
minutes of `/mnt/c` I/O after the same check had independently passed on
Windows. IMPLEMENTATION: focused commit
`28cceb2b17b15bc884124c63729d4d24e456668b` was pushed to `origin/main`; exact
implementation-SHA GitHub Actions CI run `34647644049` completed successfully.
The docs-only publication commit `70e2a8e6f09200f55798f9bfbb42f0da44f2ecf2`
is the final publication SHA; its exact CI run `34647889523` also completed
successfully. No ROM or decoded payload is tracked.

# 2026-09-11 — M12-AUTO56 overlapping PC-relative word table — <90% / CONTINUING

TASK: Continue the M12 >=90% SOURCE-OWNED ROM MAP while preserving the
byte-exact canonical ROM and keeping C++ migration out of scope.

ACCEPTANCE CRITERIA: Promote only bytes in a wholly UNKNOWN span when an exact
PC-relative consumer closes the table base, selector mask, word stride, count,
and overlap with an already confirmed table. Do not reclassify the existing
overlap or infer semantics beyond the consumer contract.

RESULT: AUTO56 promotes the unknown prefix `[0x062DA8,0x062DC0)` (24 bytes)
of the exact 16-word table `[0x062DA8,0x062DC8)`. Consumer `0x061A22` has the
exact PC-relative base, masks the selector to `0..15`, doubles it, and reads
one word per selected entry. The final four words `[0x062DC0,0x062DC8)` overlap
the already confirmed AUTO28 table and are therefore not re-promoted. The map
reaches 1,426,357 / 3,145,728 bytes (`45.3426678975%`); 1,404,799 bytes remain
to the integer 90% threshold.

EXACTNESS: The transaction is
`build/m12-auto56-pc-word-overlap-a/materialized/manifest.json`. Canonical
CRC32 is `C4728225`, SHA-1 is
`2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256 is
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

VALIDATION: The dedicated helper regression, Python compilation, contract and
overlap checks, materialization, and full-ROM hash check passed. Debug and
Release builds remain blocked by the pre-existing
`src/core/ram_flag_routine.cpp` `std::to_string`/`std::runtime_error` errors;
no C++ source was changed and no C++ migration began.

# 2026-09-11 — M12-AUTO55 bounded 0x03B8DE descriptor table — <90% / CONTINUING

TASK: Continue the M12 >=90% SOURCE-OWNED ROM MAP while preserving the
byte-exact canonical ROM and keeping C++ migration out of scope.

ACCEPTANCE CRITERIA: Promote only the wholly UNKNOWN suffix of a table whose
PC-relative base, selector range, fixed stride, and record count are exact.
Do not assign semantic field types or promote adjacent mixed data.

RESULT: AUTO55 promotes `[0x03B8E2,0x03B95E)` (124 bytes), the unknown suffix
of the eight-record, 16-byte table based at `0x03B8DE`. The exact consumers at
`0x03A91C`, `0x03A9E2`, `0x03A9EE`, and `0x03AA18` close selector values `0..7`,
record count `8`, and stride `0x10`. The map reaches 1,426,333 /
3,145,728 bytes (45.3419049581%); 1,404,823 bytes remain to the integer 90%
threshold. The unusual final record field is retained as raw structured data;
no target semantics are inferred.

EXACTNESS: The transaction is
`build/m12-auto55-table-03b8de-a/materialized/manifest.json`.
Canonical CRC32 is `C4728225`, SHA-1 is
`2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256 is
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

VALIDATION: The dedicated helper, CTest registration, Python compilation,
selector/record contract, materialization, and full-ROM hash check passed.
Debug and Release builds still fail at the pre-existing
`src/core/ram_flag_routine.cpp` `std::to_string`/`std::runtime_error` errors;
no C++ source was changed and no C++ migration began.

# 2026-09-11 — M12-AUTO52 direct loader graphics streams — <90% / CONTINUING

TASK: Continue the M12 >=90% SOURCE-OWNED ROM MAP while preserving the
byte-exact canonical ROM and keeping C++ migration out of scope.

ACCEPTANCE CRITERIA: Promote only graphics streams whose direct loader
literal, exact ROM start, and decompressor terminator close a half-open
boundary. Keep parser-only census candidates and adjacent unproven bytes
UNKNOWN.

RESULT: AUTO52 promotes three direct-loader graphics streams:
`[0x1911EA,0x191F09)`, `[0x19911A,0x199CBA)`, and
`[0x19D6A0,0x19EC4C)`, totaling 11,883 bytes. The loader literals at
`0x03CA32`, `0x03CD46`, and `0x03DD62` identify the exact starts; the
decompressor output contracts close the ends. The map reaches 1,425,633 /
3,145,728 bytes (45.3196525574%); 1,405,522 bytes remain to the integer 90%
threshold.

EXACTNESS: The transaction is
`build/m12-auto52-direct-loader-streams-a/materialized/manifest.json`.
Canonical CRC32 is `C4728225`, SHA-1 is
`2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256 is
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

VALIDATION: The dedicated helper regression, Python compilation, exact
loader/table contracts, manifest conflict/gap checks, and full-ROM hash check
passed. No C++ migration began.

# 2026-09-11 — M12-AUTO51 static stream pointers — <90% / CONTINUING

RESULT: AUTO51 promotes nine static-pointer graphics streams totaling 28,000
bytes. The map reaches 1,413,750 bytes (44.9419021606%). The four table-backed
and five direct/repeated pointer contracts are independently closed; adjacent
unknown bytes remain unowned.

# 2026-09-11 — M12-AUTO50 multi-resource families — <90% / CONTINUING

RESULT: AUTO50 promotes 76,019 bytes from two repeated descriptor/table
families plus three direct-loader streams. Exact pointer provenance and
decompressor boundaries leave the `0x207595..0x207738` gap UNKNOWN. The map
reaches 1,385,750 bytes (44.0518061320%).

# 2026-09-11 — M12-AUTO49 descriptor-backed graphics streams — <90% / CONTINUING

RESULT: AUTO49 promotes ten descriptor-backed graphics streams totaling
72,284 bytes. Each repeated 22-byte descriptor supplies a ROM pointer and the
exact decompressor terminator supplies the end. The adjacent
`0x1F66A0..0x1F683C` candidate remains UNKNOWN because its descriptor/resource
contract is not closed. The map reaches 1,309,731 bytes (41.6352272034%).

# 2026-09-11 — M12-AUTO48 through AUTO46 exact dispatch families — <90% / CONTINUING

RESULT: AUTO48 adds 36 bytes from a PC-relative eight-word dispatch table and
closed branch/continuation code; AUTO47 adds 1,026 bytes from a multi-dispatch
family; AUTO46 adds 246 bytes from a static dispatch family. Their cumulative
map checkpoints are 1,237,447 bytes (39.3373807271%), 1,237,411 bytes
(39.3362363180%), and 1,236,385 bytes (39.3036206563%), respectively. All
three transactions preserve the canonical ROM hashes and leave unresolved
targets/data outside the exact contracts UNKNOWN.

# 2026-09-11 — M12-AUTO45 exact indexed offset table — <90% / CONTINUING

TASK: Continue the M12 >=90% SOURCE-OWNED ROM MAP while preserving the
byte-exact canonical ROM and keeping C++ migration out of scope.

ACCEPTANCE CRITERIA: Promote only a wholly UNKNOWN table whose PC-relative
base, selector mask/shift, consumer instruction, and exact half-open boundary
are independently closed. Do not infer ownership for the adjacent code at
`0x00AD76` or for the dynamic ROM words read by the DMA queue at `0x002888`.

RESULT: AUTO45 promotes `[0x00AD56,0x00AD76)` (32 bytes) as a 16-entry
big-endian word offset table. `LEA.L ($FFFFFB7A,PC),A5` at `0x00B1DA`
resolves to `0x00AD56`; `ANDI.W #$0F00,D4` plus `LSR.W #7,D4` at
`0x00B242` closes offsets `0,2,...,30`; and `MOVE.W 0(A5,D4.W),D4` at
`0x00B248` reads exactly the 16 words. The next decoded instruction begins
at `0x00AD76`, so no neighboring bytes are assigned. The map reaches
1,236,139 / 3,145,728 bytes (39.2958005269%); 1,595,017 bytes remain to the
integer 90% threshold.

EXACTNESS: The AUTO45 transaction is
`build/m12-auto45-indexed-offset-table-transaction-c/materialized/manifest.json`.
The canonical ROM is preserved exactly: CRC32 `C4728225`, SHA-1
`2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
This data-only transaction reuses the canonical baseline rebuild; no new ASM
was introduced and no C++ migration began.

VALIDATION: The dedicated helper regression, Python compilation, canonical
ROM identity check, table byte/value contract, selector closure, manifest
materialization, and inherited full-ROM hash check passed. The current
0x002888 DMA-queue path remains evidence-only because its A0 source is dynamic
RAM state and does not close a ROM container boundary.

# 2026-09-11 — M12-AUTO39 exact relative selector table — <90% / CONTINUING

TASK: Continue the M12 >=90% SOURCE-OWNED ROM MAP while preserving the
byte-exact canonical ROM and keeping C++ migration out of scope.

ACCEPTANCE CRITERIA: Promote only the selector table whose index set is closed
by exact caller control flow; leave every selected payload UNKNOWN until its
own parser boundary is proven.

RESULT: AUTO39 promotes `[0x15A9A6,0x15A9B0)` (10 bytes) as five big-endian
relative offsets. Consumer `0x003EFA` can select only byte offsets `0,2,4,6,8`
and applies each word relative to the selected field address. The five targets
are `0x15A9B0`, `0x15A9C4`, `0x15A9DE`, `0x15A9FE`, and `0x15AA24`; all target
payloads remain UNKNOWN. The map reaches 1,233,231 / 3,145,728 bytes
(39.2033576965%); 1,597,925 bytes remain to the integer 90% threshold.

EXACTNESS: The AUTO39 transaction is
`build/m12-auto39-relative-selector-transaction-a/materialized/manifest.json`.
It has zero manifest gaps/overlaps and rebuilds CRC32 `C4728225`, SHA-1
`2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

VALIDATION: The promoter, helper regression, exact consumer/table/hash
contracts, vasm round-trip, and full-ROM verification pass. Debug/Release
MSVC builds remain blocked by the existing `std::to_string` error in
`src/core/ram_flag_routine.cpp`. No ROM, BIOS, commercial asset, or C++
migration was added.

# 2026-09-11 — M12-AUTO38 exact menu offset table and record streams — <90% / CONTINUING

TASK: Continue the M12 >=90% SOURCE-OWNED ROM MAP while preserving the
byte-exact canonical ROM and keeping C++ migration out of scope.

ACCEPTANCE CRITERIA: Promote only a wholly UNKNOWN range whose relative offset
table, bounded count streams, direct callers, and shared parser all close the
same byte interval without assigning the neighboring graphics separator.

RESULT: AUTO38 promotes `[0x15B9D4,0x15BAC2)` (238 bytes) as a menu offset
table plus four contiguous count-bounded six-byte record streams. The exact
callers at `0x004AF2`, `0x004B08`, and `0x004B18` select the streams through
`ADDA.W`; the shared parser at `0x00B730` consumes the leading count and
advances six bytes per record. The map reaches 1,233,221 / 3,145,728 bytes
(39.2030398051%); 1,597,935 bytes remain to the integer 90% threshold.

EXACTNESS: The AUTO38 transaction is
`build/m12-auto38-menu-record-stream-transaction-a/materialized/manifest.json`.
It has zero manifest gaps/overlaps and rebuilds CRC32 `C4728225`, SHA-1
`2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

VALIDATION: The promoter, helper regression, exact consumer/parser contracts,
vasm round-trip, and full-ROM verification pass. Debug/Release MSVC builds
remain blocked by the existing `std::to_string` error in
`src/core/ram_flag_routine.cpp`. No ROM, BIOS, commercial asset, or C++
migration was added.

STATUS: The >=90% gate remains unmet; exact data/parser/consumer provenance
investigation continues.

# 2026-09-11 — M12-AUTO37 exact enum lookup — <90% / CONTINUING

TASK: Continue the M12 >=90% SOURCE-OWNED ROM MAP while preserving the
byte-exact canonical ROM and keeping C++ migration out of scope.

ACCEPTANCE CRITERIA: Promote only the exact selector-indexed table closed by
the caller range and neighboring table boundary.

RESULT: AUTO37 promotes `[0x05CE56,0x05CE96)` (64 bytes) as a
`BYTE_ENUM_LOOKUP_TABLE`. Reader `0x007A6C` indexes it from `FF1976`; the
preceding guard closes the selector at `0..0x3F`, values are `0..4`, and the
next confirmed pointer table begins exactly at `0x05CE96`. The map reaches
1,232,983 / 3,145,728 bytes (39.1954739889%).

EXACTNESS: The AUTO37 transaction is
`build/m12-auto37-enum-lookup-transaction-b/materialized/manifest.json` and
rebuilds the canonical ROM hashes exactly. The standalone AUTO37 helper and
vasm full-ROM verification pass.

VALIDATION: Debug/Release MSVC builds remain blocked by the existing
`std::to_string` error in `src/core/ram_flag_routine.cpp`. No ROM, BIOS,
commercial asset, or C++ migration was added.

# 2026-09-11 — M12-AUTO36 exact menu graphics streams — <90% / CONTINUING

TASK: Continue the M12 >=90% SOURCE-OWNED ROM MAP while preserving the
byte-exact canonical ROM and keeping C++ migration out of scope.

ACCEPTANCE CRITERIA: Promote only direct graphics streams with exact 68000
consumer bytes, independently reproduced decoder boundaries, and no padding
byte reassignment.

RESULT: AUTO36 promotes `[0x15BAC2,0x15C238)` (1910 bytes),
`[0x15C238,0x15CA9B)` (2147 bytes), and `[0x15CA9C,0x15CEA0)` (1028 bytes).
The direct consumers are `0x004966`, `0x004974`, and `0x004982`; decoder output
sizes are 3200, 3200, and 2656 bytes. The `0xFF` separator at `0x15CA9B`
remains UNKNOWN. The map reaches 1,232,919 / 3,145,728 bytes
(39.1934394836%); 1,598,237 bytes remain to the integer 90% threshold.

EXACTNESS: The AUTO36 transaction is
`build/m12-auto36-menu-graphics-transaction-b/materialized/manifest.json`.
It has zero manifest gaps/overlaps and rebuilds CRC32 `C4728225`, SHA-1
`2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

VALIDATION: The promoter, helper regression, external decoder boundaries,
vasm round-trip, and full-ROM verification pass. Debug/Release MSVC builds
remain blocked by the existing `std::to_string` error in
`src/core/ram_flag_routine.cpp`. No ROM, BIOS, commercial asset, or C++
migration was added.

STATUS: The >=90% gate remains unmet; exact data/parser/consumer provenance
investigation continues.

# 2026-09-11 — M12-AUTO35 exact bit-7 lookup table — <90% / CONTINUING

TASK: Continue the M12 >=90% SOURCE-OWNED ROM MAP while preserving the
byte-exact canonical ROM and keeping C++ migration out of scope.

ACCEPTANCE CRITERIA: Promote only a wholly UNKNOWN byte lookup whose reader
closes the index range and whose neighboring payload remains unowned.

RESULT: AUTO35 promotes `[0x05CE16,0x05CE56)` (64 bytes) as a
`BYTE_BIT7_LOOKUP_TABLE`. Reader `0x00F61C` applies `ANDI.W #$3F` and tests
bit 7 at the selected byte. The adjacent `[0x05CE56,0x05CE96)` payload is
not promoted. The map reaches 1,227,834 / 3,145,728 bytes (39.0317916870%);
1,603,322 bytes remain to the integer 90% threshold.

EXACTNESS: The AUTO35 transaction is
`build/m12-auto35-bit7-transaction-a/materialized/manifest.json`. It has
zero manifest gaps/overlaps and rebuilds CRC32 `C4728225`, SHA-1
`2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

VALIDATION: The promoter and regression helper pass, and full-ROM vasm
verification passes. Debug/Release MSVC builds remain blocked by the existing
`std::to_string` error in `src/core/ram_flag_routine.cpp`. No ROM, BIOS,
commercial asset, or C++ migration was added.

STATUS: The >=90% gate remains unmet; exact data/parser/consumer provenance
investigation continues.

# 2026-09-11 — M12-AUTO34 exact compressed-resource pointer table — <90% / CONTINUING

TASK: Continue the M12 >=90% SOURCE-OWNED ROM MAP while preserving the
byte-exact canonical ROM and keeping C++ migration out of scope.

ACCEPTANCE CRITERIA: Promote only a wholly UNKNOWN pointer container whose
reader, entry width, sentinel, monotone targets, and following boundary are
exact. Do not re-promote the already-owned target streams or infer their
semantic names.

RESULT: AUTO34 promotes `[0x05CE96,0x05D046)` (432 bytes) as a
`COMPRESSED_RESOURCE_POINTER_TABLE`. Reader `0x00D3B2` shifts its 0-based
resource ID by two, reads an absolute longword from the table, and calls the
verified graphics decoder at `0x003820`. Entry 0 is zero; entries 1..107 are
strictly increasing and span the already-owned stream starts
`0x1AD000..0x1E6EDA`. The next independently confirmed table starts at
`0x05D046`. The map reaches 1,227,770 / 3,145,728 bytes (39.0297571818%);
1,603,386 bytes remain to the integer 90% threshold.

EXACTNESS: The AUTO34 transaction is
`build/m12-auto34-resource-pointer-table-transaction-a/materialized/manifest.json`.
It has zero manifest gaps/overlaps and rebuilds CRC32 `C4728225`, SHA-1
`2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

VALIDATION: The promoter, regression, and full-ROM vasm verification pass;
the source line-limit gate remains required. The project Debug/Release MSVC
build remains blocked by the pre-existing `std::to_string` error in
`src/core/ram_flag_routine.cpp`. No ROM, BIOS, commercial asset, or C++
migration was added.

STATUS: The >=90% gate remains unmet; exact data/parser/consumer provenance
investigation continues.

# 2026-09-11 — M12-AUTO33 exact 64-entry item label table — <90% / CONTINUING

TASK: Continue the M12 >=90% SOURCE-OWNED ROM MAP while preserving the
byte-exact canonical ROM and keeping C++ migration out of scope.

ACCEPTANCE CRITERIA: Promote only a wholly UNKNOWN fixed-record container
whose consumer stride and physical boundary are exact, whose bytes satisfy
the deterministic format contract, and whose neighboring payload remains
unowned.

RESULT: AUTO33 promotes `[0x05CC16,0x05CE16)` (512 bytes) as a
`FIXED_WIDTH_ITEM_LABEL_TABLE`: 64 exact 8-byte printable records. Consumers
`0x0041A6` and `0x0041CA` use the byte-selected index, double it, then shift
left three for the 8-byte stride. The next byte at `0x05CE16` is outside the
printable record container and remains UNKNOWN. The map reaches 1,227,338 /
3,145,728 bytes (39.0160242716%); 1,603,818 bytes remain to the integer 90%
threshold.

EXACTNESS: The AUTO33 transaction is
`build/m12-auto33-item-label-table-transaction-a/materialized/manifest.json`.
It has zero manifest gaps/overlaps and rebuilds CRC32 `C4728225`, SHA-1
`2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

VALIDATION: The promoter, regression, and full-ROM vasm verification pass;
the source line-limit gate remains required. The project Debug/Release MSVC
build remains blocked by the pre-existing `std::to_string` error in
`src/core/ram_flag_routine.cpp`. No ROM, BIOS, commercial asset, or C++
migration was added.

STATUS: The >=90% gate remains unmet; exact data/parser/consumer provenance
investigation continues.

# 2026-09-11 — M12-AUTO32 exact fixed-width menu label table — <90% / CONTINUING

TASK: Continue the M12 >=90% SOURCE-OWNED ROM MAP while preserving the
byte-exact canonical ROM and keeping C++ migration out of scope.

ACCEPTANCE CRITERIA: Promote only a wholly UNKNOWN fixed-record table with an
exact consumer, a statically closed selector range, deterministic extraction,
and no neighboring format mixed into the range.

RESULT: AUTO32 promotes `[0x05CBA6,0x05CBD6)` (48 bytes) as a
`FIXED_WIDTH_MENU_LABEL_TABLE`: six exact 8-byte records. Consumer `0x003F8E`
uses `FF1861 << 3`; the source switch at `0x003DCC` accepts five one-based
cases and the alternate path explicitly writes `5`, closing the selector at
`0..5`. The map reaches 1,226,826 / 3,145,728 bytes (38.9997482300%);
1,604,330 bytes remain to the integer 90% threshold.

EXACTNESS: The AUTO32 transaction is
`build/m12-auto32-label-table-transaction-a/materialized/manifest.json`.
It has zero manifest gaps/overlaps and rebuilds CRC32 `C4728225`, SHA-1
`2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

VALIDATION: The promoter, regression, and full-ROM vasm verification pass;
the source line-limit gate remains required. The project Debug/Release MSVC
build remains blocked by the pre-existing `std::to_string` error in
`src/core/ram_flag_routine.cpp`. No ROM, BIOS, commercial asset, or C++
migration was added.

STATUS: The >=90% gate remains unmet; exact data/parser/consumer provenance
investigation continues.

# 2026-09-11 — M12-AUTO31 exact sentinel threshold table — <90% / CONTINUING

TASK: Continue the M12 >=90% SOURCE-OWNED ROM MAP while preserving the
byte-exact canonical ROM and keeping C++ migration out of scope.

ACCEPTANCE CRITERIA: Promote only a wholly UNKNOWN table whose exact consumer
closes the entry stride and termination condition, with deterministic
full-ROM materialization and no neighboring mixed payload.

RESULT: AUTO31 promotes `[0x05D906,0x05D918)` (18 bytes) as a
`SENTINEL_TERMINATED_THRESHOLD_TABLE`. Consumer `0x010666` compares each
post-incremented word, returns on `BLS`, advances the result by 8, and reaches
the exact `0xFFFF` sentinel at the ninth word. The next table begins at the
already-owned `0x05D918`. The map reaches 1,226,778 / 3,145,728 bytes
(38.9982223511%); 1,604,378 bytes remain to the integer 90% threshold.

EXACTNESS: The AUTO31 transaction is
`build/m12-auto31-threshold-table-transaction-a/materialized/manifest.json`.
It has zero manifest gaps/overlaps and rebuilds CRC32 `C4728225`, SHA-1
`2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

VALIDATION: The promoter, regression, and full-ROM vasm verification pass;
the source line-limit gate remains required. The project Debug/Release MSVC
build remains blocked by the pre-existing `std::to_string` error in
`src/core/ram_flag_routine.cpp`. No ROM, BIOS, commercial asset, or C++
migration was added.

STATUS: The >=90% gate remains unmet; exact data/parser/consumer provenance
investigation continues.

# 2026-09-11 — M12-AUTO30 exact state dispatch pointer table — <90% / CONTINUING

TASK: Continue the M12 >=90% SOURCE-OWNED ROM MAP while preserving the
byte-exact canonical ROM and keeping C++ migration out of scope.

ACCEPTANCE CRITERIA: Promote only a wholly UNKNOWN, exact structured-data
container with a credible consumer contract, a closed byte range, no code
ownership conflict, and deterministic full-ROM materialization. Do not infer
ownership for the pointed-to handler bodies.

RESULT: AUTO30 promotes `[0x00DF54,0x00E0B8)` (356 bytes) as an
`ABSOLUTE_STATE_DISPATCH_POINTER_TABLE`. Three identical consumers at
`0x00FD70`, `0x00FDF4`, and `0x00FF7E` form the table base, double the state
selector, and read a longword through the exact `-44(A0,D6.W)` displacement
before `JSR (A0)`. The table contains 89 canonical longwords for the physical
container ending immediately before the already-owned `RTS` at `0x00E0B8`.
No target handler range is promoted. The map reaches 1,226,760 / 3,145,728
bytes (38.9976501465%); 1,604,396 bytes remain to the integer 90% threshold.

EXACTNESS: The AUTO30 transaction is
`build/m12-auto30-dispatch-pointer-table-transaction-a/materialized/manifest.json`.
It has zero manifest gaps/overlaps and rebuilds CRC32 `C4728225`, SHA-1
`2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

VALIDATION: The promoter, regression, full-ROM vasm verification, and source
line-limit gate pass. The project Debug/Release MSVC build remains blocked by
the pre-existing `std::to_string` error in `src/core/ram_flag_routine.cpp`.
No ROM, BIOS, commercial asset, or C++ migration was added.

STATUS: The >=90% gate remains unmet; exact data/parser/consumer provenance
investigation continues.

# 2026-09-11 — M12-AUTO29 preserved candidate-map code census — <90% / CONTINUING

TASK: Continue the M12 >=90% SOURCE-OWNED ROM MAP while preserving the
byte-exact canonical ROM and keeping C++ migration out of scope. Exercise the
preserved candidate-map/Ghidra provenance path before switching to data and
parser contracts.

ACCEPTANCE CRITERIA: Reconstruct only the preserved `ghidra_range` claims into
a machine-readable developer-only map, then run the strict transactional
auto-promoter without inventing new decompiler claims or weakening ownership,
boundary, and exact-rebuild gates.

RESULT: The converter reconstructs 496 bounded function ranges from the
preserved candidate map. The strict auto-promoter discovers 534 candidates,
but has 0 eligible, 0 attempted, and 0 accepted candidates against the
AUTO28 map. This is a negative result for this specific preserved code-census
method, not a global M12 blocker; it adds no ROM ownership and does not
justify promoting mixed or unbounded code.

EXACTNESS: The reconstructed map is
`build/m12-ghidra-reconstructed-candidate-map-b.json`; the no-change
transaction is `build/m12-auto29-code-census-transaction-a/materialized/manifest.json`.
The current map remains 1,226,404 / 3,145,728 bytes (38.9863332113%), with
1,604,752 bytes remaining to the integer 90% threshold. The canonical ROM
hashes remain CRC32 `C4728225`, SHA-1
`2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

VALIDATION: The converter regression and Python compilation pass; the
transaction reports exact full-ROM identity and no ownership delta. The
project Debug/Release MSVC build remains blocked by the pre-existing
`std::to_string` error in `src/core/ram_flag_routine.cpp`. No ROM, BIOS,
commercial asset, or C++ migration was added.

STATUS: The >=90% gate remains unmet. The preserved automatic code-census
method is exhausted for its current evidence set; investigation continues
with exact data/parser/consumer contracts and additional bounded provenance.

# 2026-09-11 — M12-AUTO28 exact 16-entry nibble lookup — <90% / CONTINUING

TASK: Continue the M12 >=90% SOURCE-OWNED ROM MAP while preserving the
byte-exact canonical ROM and keeping C++ migration out of scope.

ACCEPTANCE CRITERIA: Promote only a wholly UNKNOWN table closed by an exact
PC-relative consumer and a bounded selector, with deterministic full-ROM
materialization.

RESULT: AUTO28 promotes `[0x062DC0,0x062DE0)` (32 bytes). The consumer at
`0x0624D2` forms the PC-relative base, masks `D3` with `0xF`, doubles it, and
reads one of exactly 16 words. The map reaches 1,226,404 / 3,145,728 bytes
(38.9863332113%); 1,604,752 bytes remain to the integer 90% threshold. The
surrounding code and mixed region remain UNKNOWN.

EXACTNESS: The AUTO28 transaction is
`build/m12-auto28-nibble-lookup-transaction-a/materialized/manifest.json`.
It has zero manifest gaps/overlaps and rebuilds CRC32 `C4728225`, SHA-1
`2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

VALIDATION: The promoter and regression compile, the exact values are hash
checked, and the transactional full-ROM vasm rebuild is exact. The CMake
helper/file-limit gate is rerun after this checkpoint. No ROM, BIOS,
commercial asset, or C++ migration was added.

STATUS: The >=90% gate remains unmet; provenance investigation continues.

# 2026-09-11 — M12-AUTO27 exact event dispatch table — <90% / CONTINUING

TASK: Continue the M12 >=90% SOURCE-OWNED ROM MAP while preserving the
byte-exact canonical ROM and keeping C++ migration out of scope.

ACCEPTANCE CRITERIA: Promote only a wholly UNKNOWN table closed by an exact
68000 consumer selector, signed-relative target contract, and deterministic
full-ROM materialization.

RESULT: AUTO27 promotes `[0x00532C,0x005378)` (76 bytes) as a
`SIGNED_RELATIVE_EVENT_DISPATCH_TABLE`. The owned dispatcher at `0x00530C`
subtracts `0x1A`, doubles the bounded selector, and applies each signed word
relative to its table entry; 38 entries resolve either to the bounded handler
cluster or the owned `RTS` at `0x00532A`. The map reaches 1,226,372 /
3,145,728 bytes (38.9853159587%); 1,604,784 bytes remain to the integer 90%
threshold. No handler code or mixed payload was inferred.

EXACTNESS: The AUTO27 transaction is
`build/m12-auto27-event-dispatch-transaction-a/materialized/manifest.json`.
It has zero manifest gaps/overlaps and rebuilds CRC32 `C4728225`, SHA-1
`2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

VALIDATION: The promoter and regression compile, the transactional full-ROM
vasm rebuild is exact, and the CTest helper/file-limit gate is rerun after
this change. The project Debug/Release MSVC build remains blocked by the
pre-existing `std::to_string` error in `src/core/ram_flag_routine.cpp`.
No ROM, BIOS, commercial asset, or C++ migration was added.

STATUS: The >=90% gate remains unmet; provenance investigation continues.

# 2026-09-11 — M12-AUTO26 exact field-86 callback entrypoints — <90% / CONTINUING

TASK: Continue the M12 >=90% SOURCE-OWNED ROM MAP while preserving the
byte-exact canonical ROM and keeping C++ migration out of scope.

ACCEPTANCE CRITERIA: Promote only ROM entrypoints whose exact callback address
is initialized by a bounded 68000 path, dispatched through `JSR (A0)`, and
whose developer-only ASM reproduces the complete ROM range with vasm.

RESULT: AUTO26 promotes `[0x010000,0x010034)` (52 bytes) and the `RTS` stub
`[0x030000,0x030002)` (2 bytes). The dispatcher at `0x00E0BA` loads field
`86(A6)` into `A0` and calls it; exact initializers set `0x10000` and `0x30000`.
The candidate at `0x80000` remains UNKNOWN because its F-line/data-like bytes
do not provide a closed code boundary. The map reaches 1,226,296 / 3,145,728
bytes (38.9828999837%); 1,604,860 bytes remain to the integer 90% threshold.

EXACTNESS: AUTO26 vasm round-trips both callbacks and the full transaction
rebuilds CRC32 `C4728225`, SHA-1 `2944910c07c02eace98c17d78d07bef7859d386a`,
and SHA-256 `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

VALIDATION: Python compilation, callback regression, vasm callback round-trip,
full-ROM equality, and local diff checks passed. The project Debug/Release
MSVC build remains blocked by the pre-existing `std::to_string` error in
`src/core/ram_flag_routine.cpp`; no ROM, BIOS, commercial asset, or C++
migration was added.

STATUS: The >=90% gate remains unmet; mixed payloads and unresolved callback
families remain under systematic provenance investigation.

# 2026-09-11 — M12-AUTO25 exact PC-relative consumer tables — <90% / CONTINUING

TASK: Continue the M12 >=90% SOURCE-OWNED ROM MAP while preserving the
byte-exact canonical ROM and keeping C++ migration out of scope.

RESULT: AUTO25 promotes 352 bytes from 11 exact PC-relative consumer tables:
two masked 16-byte lookups, two 9-record 8-byte tables, two bounded 3-byte
tables, two copied 10-byte records, a masked 4-byte table, a 15-word VDP
initialization table, and a 32-word VDP initialization table. Every range was
wholly UNKNOWN, non-overlapping, and bounded by the consuming loop or copy.
The full transaction remains byte-exact; no decoder-only graphics candidate
was promoted.

# 2026-09-11 — M12-AUTO24 exact static caller-backed island — <90% / CONTINUING

TASK: Continue the M12 >=90% SOURCE-OWNED ROM MAP while preserving the
byte-exact canonical ROM and keeping C++ migration out of scope.

ACCEPTANCE CRITERIA: Promote only an UNKNOWN range that is wholly covered by
a caller-backed Ghidra function, passes the bounded exact decoder, and passes
a vasm round-trip without overlap or manifest gaps. Also record rejected
pointer-table and candidate-census results.

RESULT: AUTO24 promotes `[0x0167BE,0x01685A)` (156 bytes). The range has two
Ghidra caller xrefs (`0x01672E` and `0x016742`), no decoder gap or unsupported
form after the bounded operand check, and exact vasm output. A decoder caveat
was found at the terminal `C9 4E`: the decoder emitted `exg.w D4,A6`, while
the independently assembled 68000 opcode is `EXG A4,A6`; only this generated
ASM spelling is normalized, and the ROM bytes remain unchanged.

The read-only census tested 26 caller-backed Ghidra functions wholly inside
UNKNOWN; 25 failed exact-boundary/unsupported-form checks. A dense absolute
or signed-relative pointer-table scan produced no candidate window meeting
the minimum density threshold. No 0x80000 bank or broad mixed range was
promoted. The map reaches 1,225,890 / 3,145,728 bytes (38.9699935913%); the
integer 90% threshold still requires 1,605,266 bytes.

EXACTNESS: The AUTO24 transaction has zero manifest gaps and overlaps and
rebuilds CRC32 `C4728225`, SHA-1 `2944910c07c02eace98c17d78d07bef7859d386a`,
and SHA-256 `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

VALIDATION: Helper regression, Python compilation, exact island vasm
round-trip, full-ROM equality, local diff check, file-limit check, and the
remote CI build/test passed. The project Debug build remains blocked by the
pre-existing MSVC `std::to_string` error in `src/core/ram_flag_routine.cpp`;
this task did not modify that source. No ROM, BIOS, commercial asset, or C++
migration was added.

STATUS: The >=90% gate remains unmet; the large mixed payloads and save tails
remain UNKNOWN pending independent parser/consumer boundaries.

# 2026-09-11 — M12-AUTO23 save serialization slots — <90% / CONTINUING

TASK: Continue the M12 >=90% SOURCE-OWNED ROM MAP while preserving the
byte-exact canonical ROM and keeping C++ migration out of scope.

ACCEPTANCE CRITERIA: Promote only save ranges closed by exact serializer and
checker contracts, preserve zero manifest gaps/overlaps, and require a
full-ROM equality check.

RESULT: AUTO23 promotes the primary save-slot range `[0x2025CD,0x2026E1)`
(276 bytes) and the independently bounded secondary prefix
`[0x2026E1,0x2026F5)` (20 bytes). The exact 68000 routines at `0x001DEC`,
`0x001E46`, `0x001EDE`, and `0x001F26` establish the six stride-2 tag bytes,
the 130-byte plus checksum primary payload, and the four-byte secondary value.
The map reaches 1,225,734 / 3,145,728 bytes (38.9650344849%); 1,605,422
bytes remain to the integer 90% threshold. The later save/SRAM area remains
UNKNOWN because no bounded consumer closes it.

EXACTNESS: The transaction has zero manifest gaps and overlaps and rebuilds
CRC32 `C4728225`, SHA-1 `2944910c07c02eace98c17d78d07bef7859d386a`, and
SHA-256 `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

VALIDATION: The save-slot helper regression, Python compilation, vasm
full-ROM round trip, artifact byte comparison, and targeted CTest helper
passed. The project Debug build remains blocked by the pre-existing MSVC
`std::to_string` error in `src/core/ram_flag_routine.cpp`; this task did not
modify that source. No ROM, BIOS, commercial asset, or C++ migration was
added.

STATUS: The >=90% gate remains unmet; broad save tails and mixed payloads
remain UNKNOWN pending independent boundaries.

# 2026-09-11 — M12-AUTO22 exact small tables — <90% / CONTINUING

TASK: Continue the M12 >=90% SOURCE-OWNED ROM MAP while preserving the
byte-exact canonical ROM and keeping C++ migration out of scope.

ACCEPTANCE CRITERIA: Promote only exact fixed tables with a byte contract,
closed consumer loop/index shape, no overlap with existing ownership, and a
full-ROM equality check.

RESULT: AUTO22 promotes 58 bytes from three exact tables: the eight-longword
copy table `[0x3E0B8,0x3E0D8)`, the eight-byte selector table
`[0x522E,0x5236)`, and six three-byte source records `[0x5CBD6,0x5CBE8)`.
The map reaches 1,225,438 / 3,145,728 bytes (38.9556248983%); 1,605,718
bytes remain to the integer 90% threshold. No broad neighboring range is
classified.

EXACTNESS: The transaction has zero manifest gaps and overlaps and rebuilds
CRC32 `C4728225`, SHA-1 `2944910c07c02eace98c17d78d07bef7859d386a`, and
SHA-256 `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

VALIDATION: Helper regression, Python compilation, vasm full-ROM round trip,
artifact byte comparison, and the two targeted CTest helpers passed. The
project Debug build remains blocked by the pre-existing MSVC error in
`src/core/ram_flag_routine.cpp` (`std::to_string` unavailable); this task did
not modify that source. No ROM, BIOS, commercial asset, or C++ migration was
added.

STATUS: The >=90% gate remains unmet; unbounded pointer and decoder-only
candidates remain UNKNOWN.

# 2026-09-11 — M12-AUTO21 CC-B0 relative-target tables — <90% / CONTINUING

TASK: Close only the nested CC-B0 tables whose consumer provides an exact
selector-derived size.

RESULT: AUTO21 promotes 7,238 previously UNKNOWN bytes covered by 20 unique
target windows, merged into 14 non-overlapping intervals. The exact consumer
at `0x00CCB0` masks the selector to one byte, scales it by two, and reads a
signed relative word, closing 256 two-byte slots per target. The map reaches
1,225,380 / 3,145,728 bytes (38.9537811279%) before AUTO22.

EXACTNESS: The transaction preserved zero gaps/overlaps and the canonical
ROM hashes. Target windows overlapping AUTO15 selected slots were split
without replacing existing ownership.

# 2026-09-11 — M12-AUTO20 table-selected graphics — <90% / CONTINUING

TASK: Continue M12 toward a >=90% SOURCE-OWNED ROM MAP while preserving
byte-exact ROM and not starting C++ migration.

ACCEPTANCE CRITERIA: Promote only graphics streams selected by the exact
99-row table field1 pointers, with independent deterministic decoder boundaries,
zero overlap with existing ownership, and canonical full-ROM equality.

RESULT: The developer-only AUTO20 transaction promotes 35 wholly UNKNOWN
field1-selected graphics streams totaling 115,011 bytes. The table contract is
`[0x3F306,0x3FF66)`, with field1 at offset `+4`; each selected row is checked
against the independent graphics census boundary and decompressed byte count.
The map reaches 1,218,142 / 3,145,728 bytes (38.7236913045%); 1,613,014
bytes remain to the integer 90% threshold. Twenty-two table-selected census
streams overlapping existing ownership remain unpromoted.

EXACTNESS: The transaction has zero manifest gaps and overlaps and reconstructs
the canonical ROM with CRC32 `C4728225`, SHA-1
`2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

VALIDATION: Table-graphics helper regression, Python compilation, multi-file
census boundary validation, vasm full-ROM round trip, and artifact byte
comparison passed. The helper is provenance-only; no ROM, BIOS, commercial
asset, or C++ migration was added.

STATUS: The >=90% gate remains unmet but work continues. Raw pointer-only and
decoder-only candidates remain UNKNOWN.

# 2026-09-11 — M12-AUTO19 Ancient Music Driver data archive — <90% / CONTINUING

TASK: Continue M12 toward a >=90% SOURCE-OWNED ROM MAP while preserving
byte-exact ROM and not starting C++ migration.

ACCEPTANCE CRITERIA: Promote only the bounded ROM data container supported by
the exact 68000 sound consumer and deterministic offset-table checks. Preserve
the canonical full-ROM identity and keep tertiary targets outside the terminal
data boundary rejected rather than counted.

RESULT: The developer-only AUTO19 transaction promotes
`0x064E38..0x07D780` as `DATA_KNOWN`, totaling 100,680 bytes. The exact
consumer at `0x060B50` indexes the driver table at `0x0638D4`; the validated
primary, secondary, and tertiary offset groups contain 29, 126, and 4 active
targets in the data range. The exact `0x07D780..0x080000` `0xFF` run closes the
container boundary; 13 tertiary targets outside it remain rejected. The map
reaches 1,103,131 / 3,145,728 bytes (35.0675900777%); 1,728,025 bytes remain
to the integer 90% threshold. No ROM, BIOS, commercial asset, or C++ migration
was added.

EXACTNESS: The inherited-baseline transaction has zero manifest gaps and
overlaps and reconstructs the canonical ROM with CRC32 `C4728225`, SHA-1
`2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

VALIDATION: Sound-data helper regression, Python compilation, transaction
table/boundary validation, artifact byte comparison, and source-file limit
checks passed. Full Debug/Release/GNU-equivalent build status remains the
inherited pre-existing MSVC `std::to_string` issue at
`src/core/ram_flag_routine.cpp:181`; no unrelated source was modified.

STATUS: The >=90% gate remains unmet but work continues. Decoder-only or raw
pointer-only candidates remain UNKNOWN.

# 2026-09-11 — M12-AUTO18 exact 0x3820 graphics consumers — <90% / CONTINUING

TASK: Continue M12 toward a >=90% SOURCE-OWNED ROM MAP while preserving
byte-exact ROM and not starting C++ migration.

ACCEPTANCE CRITERIA: Promote only UNKNOWN streams with a bounded local decoder
boundary and an exact static `LEA → D9A4 → 37D2 → 0x3820` consumer chain.
Revalidate the inherited AUTO17 runtime streams and preserve canonical
full-ROM equality.

RESULT: The developer-only transaction adds two static-consumer streams:
`0x167E48..0x16821F` (983 bytes), selected by consumers at `0x02D444` and
`0x02E204`, and `0x168442..0x168492` (80 bytes), selected at `0x02D416`.
The shared `D9A4 → 37D2 → 0x3820` chain is byte-verified at `0x00D9B2` and
`0x0037D8`; local decoding consumes exactly both boundaries. The map reaches
1,002,451 / 3,145,728 bytes (31.8670590719%); 1,828,705 bytes remain to the
integer 90% threshold. No surrounding census candidate, ROM, BIOS,
commercial asset, or C++ migration was added.

EXACTNESS: The inherited-baseline transaction reconstructs the canonical ROM
with CRC32 `C4728225`, SHA-1
`2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

VALIDATION: Runtime-correlation revalidation, static consumer contracts,
pointer literals, local decoder boundaries, helper regression, Python
compilation, CMake/CTest and source-limit gates passed. Full Debug/Release/GNU
build status remains the inherited pre-existing MSVC `std::to_string` issue
at `src/core/ram_flag_routine.cpp:181`; no unrelated source was modified.

STATUS: The >=90% gate remains unmet. Remaining census candidates without a
closed consumer/parser graph stay UNKNOWN; continue from the next bounded
static or runtime graph.

# 2026-09-11 — M12-AUTO17 runtime-correlated graphics streams — <90% / BLOCKED

TASK: Continue M12 toward a >=90% SOURCE-OWNED ROM MAP while preserving
byte-exact ROM and not starting C++ migration.

ACCEPTANCE CRITERIA: Promote only UNKNOWN graphics ranges whose canonical
runtime reader is the exact `0x3820` decoder, whose observed boundary equals
the local decoder's `source_consumed` boundary, and which have independent
static pointer literals. Preserve the inherited canonical full-ROM result.

RESULT: The developer-only transaction closes two local-ROM-derived streams:
`0x15E052..0x160E19` (11,719 bytes) and `0x2119D2..0x211F79` (1,447 bytes).
The GPGX correlation records both ranges as first-read by PC `0x003830`, the
runtime execution evidence confirms that decoder PC, the local inspector
consumes exactly both ranges, and the ROM contains five plus one independent
big-endian pointer literals. The map reaches 1,001,388 / 3,145,728 bytes
(31.8332672119%); 1,829,768 bytes remain to the integer 90% threshold. No
surrounding census candidate, ROM, BIOS, commercial asset, or C++ migration
was added.

EXACTNESS: The inherited-baseline transaction reconstructs the canonical ROM
with CRC32 `C4728225`, SHA-1
`2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

VALIDATION: The helper regression, Python compilation, runtime-reader,
pointer, and local decoder gates passed. The new helper is
`src/tools/re_m12_runtime_graphics_promote.py`; generated transaction and
decoder images remain ignored build evidence. Fresh full Debug/Release/GNU
build status remains the inherited pre-existing MSVC `std::to_string` issue
at `src/core/ram_flag_routine.cpp:181`; no unrelated source was modified.

STATUS: The >=90% gate remains unmet. The remaining decoder-census candidates
still require a closed table/parser or independent runtime consumer edge;
continue from the next independently closed graph.

# 2026-09-11 — M12-AUTO16 exact runtime-correlated probe slices — <90% / BLOCKED

TASK: Continue M12 toward a >=90% SOURCE-OWNED ROM MAP while preserving
byte-exact ROM and not starting C++ migration.

ACCEPTANCE CRITERIA: Promote only bounded 68000 slices with canonical-equal
probe ASM and binary artifacts, decoder-bound metadata, and runtime PC
corroboration. Exclude surrounding mixed ranges and require canonical full-ROM
equality.

RESULT: The developer-only transaction closes nine exact probe slices totalling
1,430 CODE_VERIFIED bytes: the shared movement/G0 slices at
0x008F12..0x009330, the exact 0x03B1D0 graphics-consumer routine,
0x060090..0x060286, and 0x061232..0x061328. Each slice has a local
canonical-equal binary, decoder-bounded ASM, and at least one observed
instruction start in the canonical runtime evidence. The map reaches
988,222 / 3,145,728 bytes (31.4147313436%); 1,842,934 bytes remain to the
integer 90% threshold. No surrounding mixed range, ROM, BIOS, commercial
asset, or C++ migration was added.

EXACTNESS: The inherited-baseline transaction reconstructs the canonical ROM
with CRC32 C4728225, SHA-1
2944910c07c02eace98c17d78d07bef7859d386a, and SHA-256
eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263.
Fresh assembler round-trip remains unavailable because no external vasm
executable is installed; every new binary slice was compared byte-for-byte.

VALIDATION: The helper regression and Python compilation passed. The local
transaction passed its nine-slice evidence/runtime gates and canonical full-ROM
hash checks. The new AUTO16 helper, AUTO14 helper and AUTO15 helper passed in
Debug, Release and GNU-labelled CTest configurations (3/3 each). The bounded
source scan passed for 394 checked files with CMakeLists.txt at 500 lines.
Full Debug/Release/GNU-labelled builds reached the existing MSVC failure at
src/core/ram_flag_routine.cpp:181 because std::to_string is not visible in that
translation unit; AUTO16 did not change that file, so this unrelated issue was
not modified. Remote CI remains the clean publication gate. Commit 56f778e was
published and GitHub Actions CI run 34550041954 completed successfully: build
and test passed.

STATUS: The >=90% gate remains unmet. Dynamic/mixed code, nested subtables,
and decoder-only resource candidates remain UNKNOWN; continue from the next
independently closed edge.

# 2026-09-11 — M12-AUTO15 CC-B0 selected relative-pointer slots — <90% / BLOCKED

**TASK:** Continue M12 toward a >=90% SOURCE-OWNED ROM MAP while preserving
byte-exact ROM and not starting C++ migration.

**ACCEPTANCE CRITERIA:** Close only 16 statically constant-D0 caller-selected
16-bit relative-pointer slots reached through the AUTO14 CC-B0 table. Exclude
dynamic callers, unproven low-byte bounds, and nested table extents; require
canonical full-ROM equality.

**RESULT:** The developer-only transaction closes 15 unique slots / 30
`STRUCTURED_DATA_CONFIRMED` bytes. Each slot is reached by an exact constant-D0
caller, resolves through a closed CC-B0 group-table base, and contains a valid
even canonical-ROM target. The map reaches 986,792 / 3,145,728 bytes
(`31.3692728678%`); 1,844,364 bytes remain to the integer 90% threshold. No
C++ migration, ROM, BIOS, or extracted commercial asset was added.

**EXACTNESS:** The transaction reconstructs the canonical ROM with CRC32
`C4728225`, SHA-1 `2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

**VALIDATION:** The selected-slot helper regression and Python compilation
passed. The local transaction used explicit `INHERITED_BASELINE_FULL_ROM`
verification because no external vasm executable is installed; the AUTO14
canonical rebuilt ROM and inherited ASM/blob artifacts were verified unchanged,
and all 30 new bytes were compared to their canonical slices. Fresh Debug,
Release and GNU/Unix-equivalent builds plus CTest 92/92 passed locally; the
untracked generated root evidence files were excluded from the bounded local
source scan. Remote CI for this AUTO15 publication passed as run
`34548261762`, including build, all tests, and the repository source-file-limit
gate.

**STATUS:** The >=90% gate remains unmet. Dynamic CC-B0 callers, nested
subtables, and other mixed code/data/resource spans remain UNKNOWN; continue
from the next independently closed edge.

# 2026-09-11 — M12-AUTO14 CC-B0 group pointer table — <90% / BLOCKED

**TASK:** Continue M12 toward a >=90% SOURCE-OWNED ROM MAP while preserving
byte-exact ROM and not starting C++ migration.

**ACCEPTANCE CRITERIA:** Close only the physical 32-entry longword table at
`0x04371E..0x04379E` selected by the exact `0x00CCB0` high-byte shift. Leave
nested target subtables UNKNOWN and require canonical full-ROM equality.

**RESULT:** The developer-only transaction closes 128 `STRUCTURED_DATA_CONFIRMED`
bytes. All 32 entries are even canonical-ROM targets and the next byte begins
the nested target-subtable area. The map reaches 986,762 / 3,145,728 bytes
(`31.3683191935%`); 1,844,394 bytes remain to the integer 90% threshold. No
C++ migration, ROM, BIOS, or extracted commercial asset was added.

**EXACTNESS:** The transaction reconstructs the canonical ROM with CRC32
`C4728225`, SHA-1 `2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

**VALIDATION:** The helper regression and Python compilation passed. The local
transaction used explicit `INHERITED_BASELINE_FULL_ROM` verification because
no external vasm executable is installed: the previous AUTO13 rebuilt ROM is
canonical, unchanged ASM artifacts were byte-compared with that baseline, and
the new table blob was compared to its canonical slice. A fresh assembler
round-trip remains required when vasm is available. Debug, Release and
GNU/Unix-equivalent builds passed; CTest passed 91/91 in each configuration,
and the bounded source-limit check passed for 386 files. Publication and
the first remote CI run `34546697525` exposed `CMakeLists.txt` at 502 lines;
commit `234bb5b` reduced it to exactly 500 lines. Remote CI run
`34547024445` then passed build, all 92 tests, and the source-limit gate.

**STATUS:** The >=90% gate remains unmet. Nested target subtables and other
mixed code/data/resource spans remain UNKNOWN; continue from the next
independently closed edge.

# 2026-09-11 — M12-AUTO13 BCEA field3 sentinel-list edge — <90% / BLOCKED

**TASK:** Continue from M12-AUTO12 toward a >=90% SOURCE-OWNED ROM MAP while
preserving byte-exact ROM and not starting C++ migration.

**ACCEPTANCE CRITERIA:** Promote only field3 list bytes selected by the exact
BCEA selector transform, signed relative pointer, 44-byte row stride, and
negative-key sentinel. Preserve the existing AUTO12 map, reject ambiguous
container bytes, and require canonical full-ROM equality.

**RESULT:** The developer-only promoter accepted 100 exact list views from 30
field3 bases and merged them into five non-overlapping ranges:
`0x058012..0x0580BE`, `0x058296..0x058348`, `0x05855C..0x05855E`,
`0x05857A..0x058ABA`, and `0x058B76..0x05A232`. Only 7,516 bytes were added;
the surrounding `0x58000..0x5D046` container remains UNKNOWN. The map reaches
986,634 / 3,145,728 bytes (`31.364250183%`); 1,844,522 bytes remain to the
integer 90% threshold. No C++ migration, ROM, BIOS, or extracted commercial
asset was added.

**EXACTNESS:** AUTO13 reconstructs the canonical ROM with CRC32 `C4728225`,
SHA-1 `2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

**VALIDATION:** The new helper regression and Python compilation passed. The
transactional vasm round-trip passed with the canonical hashes. Full fresh
Debug/Release/GNU-equivalent gates and remote CI remain the publication gate.

**STATUS:** The >=90% gate remains unmet. The parser records the structural
contract only; it does not assign semantic field names or promote the complete
field3 container. Continue M12 from the next independently closed edge.

# 2026-09-11 — M12-AUTO12 exact code continuation — <90% / BLOCKED

**RESULT:** The systemic decoder/reassembler correction accepted 12/12
caller-backed candidates. AUTO12 adds 1,092 exact ASM bytes to AUTO11 and
reaches 979,118 / 3,145,728 bytes (`31.125322978%` before AUTO13). It preserves
the canonical ROM and does not start C++ migration.

# 2026-09-11 — M12-AUTO11 count-bounded record-stream family — <90% / BLOCKED

**TASK:** Continue from M12-AUTO10 toward a >=90% SOURCE-OWNED ROM MAP while
preserving byte-exact ROM and not starting C++ migration.

**ACCEPTANCE CRITERIA:** Close only the exact fixed-stride record family selected
through `0x3F2FA` and streams whose leading count plus the shared 68000 `DBF`
consumer determines an exact six-byte-record boundary. Preserve existing ASM
artifacts, reject ambiguous overlaps, and require canonical full-ROM equality.

**RESULT:** The developer-only promoter verified the 99-record table
`0x3F306..0x3FF66` and 78 unique count-bounded stream starts. Overlapping stream
views were merged only where every byte remained covered by at least one exact
count/stride view. The transaction promotes 3,168 table bytes and 46,858 stream
bytes, adding 50,026 source-owned bytes. The map reaches 978,026 / 3,145,728
bytes (`31.090609233%`); 1,853,130 bytes remain to the integer 90% threshold.
No C++ migration, ROM, BIOS, or extracted commercial asset was added.

**EXACTNESS:** AUTO11 reconstructs the canonical 3,145,728-byte ROM with CRC32
`C4728225`, SHA-1 `2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

**VALIDATION:** The helper regression, Python compilation, parser contract audit,
transactional vasm round-trip, independent manifest/hash audit, source file-limit
check, and `git diff --check` passed. Fresh Debug/Release/GNU-equivalent builds,
full CTest, push, and remote CI remain the publication gate.

**STATUS:** The >=90% gate remains unmet. The new table/stream edge is recorded;
remaining mixed code/data/resource spans stay conservative blobs. Continue M12
from the next independently closed provenance edge.

# 2026-09-11 — M12-AUTO10 exact code continuation — <90% / BLOCKED

**TASK:** Continue from the published M12-AUTO9 checkpoint toward a >=90%
SOURCE-OWNED ROM MAP, preserving byte-exact ROM and not starting C++ migration.

**ACCEPTANCE CRITERIA:** Reuse the existing automatic exact-code promotion
workflow, carry forward every ASM-backed baseline entry including header/vector
ASM, promote only candidates with exact decode, vasm slice round-trip, static
caller evidence, and full-ROM equality; preserve canonical hashes and document
rejections.

**RESULT:** The corrected batch discovered 534 candidates, retained 14 eligible
candidates, attempted all 14, and accepted two exact code islands:
`0x00F0EC..0x00F10C` (32 bytes) and `0x00B28E..0x00B34C` (190 bytes). The map
reaches 928,000 / 3,145,728 bytes (`29.500325521%`); 1,903,156 bytes remain to
the integer 90% threshold. Twelve candidates were rejected as unsupported exact
IR. The developer-only orchestrator now preserves all entries whose emitted
artifact type is ASM, including `HEADER_VECTOR_ASM`; no C++ migration, ROM,
BIOS, or extracted commercial asset was added.

**EXACTNESS:** AUTO10 full materialization is 3,145,728 bytes with CRC32
`C4728225`, SHA-1
`2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

**VALIDATION:** The updated automatic-promotion regression, Python compilation,
transactional vasm round-trip, independent manifest/hash audit, source
file-limit check, and `git diff --check` passed. The Debug/Release/GNU and
remote-CI validation results for the preceding AUTO9 commit remain green; this
follow-up requires its own publication gate.

**STATUS:** The >=90% gate remains unmet; unresolved mixed code/data/resource
spans remain conservative blobs. Continue M12 from the next independently
closed provenance edge.

# 2026-09-11 — M12-AUTO9 erased alignment padding — <90% / BLOCKED

**TASK:** Continue from the published M12-AUTO8 checkpoint toward a >=90%
SOURCE-OWNED ROM MAP, preserving byte-exact ROM and not starting C++ migration.

**ACCEPTANCE CRITERIA:** Promote only complete UNKNOWN ranges that are exact
`0xFF` fill, at least 256 bytes, and terminate on a 4 KiB ROM alignment
boundary; preserve zero-gap, zero-overlap full-ROM materialization and
canonical hashes; add helper regression coverage and current M12
documentation; leave mixed, zero-filled, and decoder-only spans UNKNOWN.

**RESULT:** M12-AUTO9 promotes 16 exact erased alignment runs totaling 132,630
bytes. The map reaches 927,778 / 3,145,728 bytes (`29.493268331%`); 1,903,378
bytes remain to the integer 90% threshold. The promoted runs end at
`0x058000`, `0x060000`, `0x080000`, `0x088000`, `0x090000`, `0x098000`,
`0x0A0000`, `0x0A8000`, `0x0B0000`, `0x0B8000`, `0x0C0000`, `0x150000`,
`0x170000`, `0x1AD000`, `0x260000`, and `0x300000`. No C++ migration, ROM,
BIOS, or extracted commercial asset was added.

**EXACTNESS:** AUTO9 full materialization is 3,145,728 bytes with CRC32
`C4728225`, SHA-1
`2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

**VALIDATION:** The synthetic FF-run helper regression, Python compilation,
transactional full vasm round-trip, independent manifest/hash audit, source
file-limit check, and `git diff --check` passed. Fresh Debug, Release, and
GNU-equivalent builds passed; full CTest passed `90/90` in all three
configurations after registering the new helper. Generated transactions remain
local ignored build evidence.

**STATUS:** The >=90% gate remains unmet; unresolved mixed code/data/resource
spans remain conservative blobs. Continue M12 from the next independently
closed provenance edge.

# 2026-09-11 — M12-AUTO8 direct graphics chain continuation — <90% / BLOCKED

**TASK:** Continue from the published M12-AUTO7 checkpoint toward a >=90%
SOURCE-OWNED ROM MAP, preserving byte-exact ROM and not starting C++ migration.

**ACCEPTANCE CRITERIA:** Promote only exact sequential `0x3820` consumer chains
with decoder-verified source boundaries; preserve zero-gap, zero-overlap
full-ROM materialization and canonical hashes; validate already-owned chain
anchors without double promotion; add helper regression coverage and current
M12 documentation; leave ambiguous spans UNKNOWN.

**RESULT:** M12-AUTO8 promotes five new non-overlapping continuation streams,
totaling 21,016 bytes: `[0x18955A,0x18CC6E)` and `[0x18D01B,0x18EB1F)`.
The map reaches 795,148 / 3,145,728 bytes (`25.277074178%`); 2,036,008 bytes
remain to the integer 90% threshold. Three adjacent chain anchors were
revalidated but were already owned and were not counted twice: `0x172168`,
`0x1894EA`, and `0x18CF98`. No C++ migration, ROM, BIOS, or extracted
commercial asset was added.

**EXACTNESS:** AUTO8 full materialization is 3,145,728 bytes with CRC32
`C4728225`, SHA-1
`2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

**VALIDATION:** The eight-consumer byte audit, eight decoder source-length
checks including three existing anchors, full vasm round-trip, and AUTO8 helper
regression pass locally. Fresh Debug and Release MinGW builds plus full CTest
are `89/89` in both configurations. The GNU-equivalent Release build/link
passed and its 18-test focused suite passed, including the line-limit scan. An
independent AUTO8 manifest/hash audit, Python compilation, source-size check,
and `git diff --check` passed; generated transactions remain local ignored
build evidence.

**STATUS:** The >=90% gate remains unmet; unresolved mixed code/data/resource
spans remain conservative blobs. Continue M12 from the next independently
closed provenance edge.

# 2026-09-11 — M12-AUTO7 direct graphics provenance — <90% / BLOCKED

**TASK:** Continue from the published M12-AUTO6 checkpoint toward a >=90%
SOURCE-OWNED ROM MAP, preserving byte-exact ROM and not starting C++ migration.

**ACCEPTANCE CRITERIA:** Promote only exact direct-consumer streams with
decoder-verified source boundaries; preserve zero-gap, zero-overlap full-ROM
materialization and canonical hashes; add helper regression coverage and
current M12 documentation; leave rejected or adjacent ambiguous data UNKNOWN.

**RESULT:** M12-AUTO7 promotes seven non-overlapping direct graphics streams,
totaling 40,065 bytes: `[0x150000,0x1503D3)`, four sequential streams spanning
`[0x152340,0x15335A)`, and two sequential streams spanning
`[0x180C56,0x1894EA)`. The map reaches 774,132 / 3,145,728 bytes
(`24.608993530%`); 2,057,024 bytes remain to the integer 90% threshold. The
fifth adjacent call after the first chain was decoder-rejected and remains
UNKNOWN. No C++ migration, ROM, BIOS, or extracted commercial asset was added.

**EXACTNESS:** AUTO7 full materialization is 3,145,728 bytes with CRC32
`C4728225`, SHA-1
`2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

**VALIDATION:** The direct-consumer byte audit, seven decoder source-length
checks, full vasm round-trip, and AUTO7 helper regression pass. Fresh Debug and
Release MinGW builds plus full CTest are `88/88` in both configurations. The
GNU-equivalent Release build/link passed and its 17-test focused suite passed,
including the line-limit scan. The independent AUTO7 manifest/hash audit,
Python compilation, source-size check, and `git diff --check` passed; generated
transactions remain local ignored build evidence.

**STATUS:** The >=90% gate remains unmet; unresolved mixed code/data/resource
spans remain conservative blobs. Continue M12 from the next independently
closed provenance edge.

# 2026-09-11 — M12-AUTO6 fixed-stride table provenance — <90% / BLOCKED

**TASK:** Continue toward a >=90% SOURCE-OWNED ROM MAP from the published
M12-AUTO5 checkpoint, preserving byte-exact ROM and not starting C++ migration.

**ACCEPTANCE CRITERIA:** Promote only a bounded, independently evidenced ROM
range; prove its exact consumer/stride/boundary contract; preserve zero-gap,
zero-overlap full-ROM materialization and canonical hashes; add helper
regression coverage and current M12 documentation; leave neighboring ambiguous
spans UNKNOWN.

**RESULT:** M12-AUTO6 promotes only `[0x5D046,0x5D686)`, exactly 50 records of
32 bytes, adding 1,600 source-owned bytes. The map reaches 734,067 / 3,145,728
bytes (`23.335361481%`); 2,097,089 bytes remain to the integer 90% threshold.
The four exact consumers are `0x00D72C`, `0x010086`, `0x0100AA`, and `0x039100`.
The following `0x0EEE` table is used as the independent upper boundary; no
semantic fields or adjacent UNKNOWN spans were promoted. No C++ migration,
ROM, BIOS, or extracted commercial asset was added.

**EXACTNESS:** AUTO6 full materialization is 3,145,728 bytes with CRC32
`C4728225`, SHA-1
`2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

**VALIDATION:** Fixed-stride helper regression, Python compile checks, exact
consumer/fingerprint audit, full vasm round-trip, and `git diff --check` pass.
The generated transaction is local ignored build evidence at
`build/m12-auto6-fixed-stride-transaction-a`; Debug/Release/GNU-equivalent
builds and full CTest remain required before publication.

**STATUS:** The >=90% gate remains unmet; unresolved mixed code/data/resource
spans remain conservative blobs. Continue M12 from the next independently
closed provenance edge; do not redefine the goal around this checkpoint.

# 2026-09-10 — M12-AUTO exact islands and resource map — COMPLETE / GLOBAL BLOCKER

**TASK:** Continue autonomously from baseline
`33cb6d9985b3a87cd9eb92d4ad0883a736bde9ff` toward a >=90% source-owned ROM
map, preserving byte-exact ROM and not starting M13 or C++ migration.

**RESULT:** `M12_AUTO_BLOCKED_GLOBAL_NO_FULL_ROM_DATA_PROVENANCE`. The combined
checkpoint promotes 181 non-overlapping caller-backed exact 68000 islands
(23,430 bytes beyond M12.5), all 107 streams from the proven compressed
resource pointer table (238,087 local-ROM-derived asset bytes), and 47
alignment bytes. Final source ownership is 279,468 / 3,145,728 bytes
(8.884048461%); remaining blob bytes are 2,866,260. No guessed asset or
executable classification was used.

**EVIDENCE:** The broad bounded probe examined 253 Ghidra intervals and found
209 exact round-trips; static caller gating and overlap resolution selected
181. The resource boundary scanner confirmed 107/107 streams from
`0x05CE96..0x05D046`; decompressor-consumed boundaries and 47 one-byte gaps
are deterministic. Screen-group pointer spans remain inconclusive because
they intersect unresolved code candidates.

**EXACTNESS:** Combined materialization is 3,145,728 bytes with canonical
CRC32 `C4728225`, SHA-1
`2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`, with
zero gaps and overlaps. See
`docs/reports/ASM_AUTONOMOUS_TO_90_PERCENT_M12_AUTO.md`.

**VALIDATION:** `tests/re_m12_auto_test.py`, `tests/re_m12_5_test.py`, Python
compile checks, resource scanner Debug build/run, 181 exact range assembly,
combined full-ROM assembly/hash verification, and source-size checks passed.
Debug/Release CTest, diff/hygiene audit, and independent manifest/hash audit
passed. Implementation checkpoint SHA, pushed `origin/main` SHA, and exact
CI are `5aaaa23fcfaea6de56629280f56bcd9c46935f58`,
`5aaaa23fcfaea6de56629280f56bcd9c46935f58`, and GitHub Actions run
`34502629852` (`build-test`, passed). CI emitted only an upstream Node.js 20
deprecation annotation.

**BLOCKER:** 2,866,260 bytes remain without a complete independent
code/data/resource provenance graph. The largest are
`0x062D6C..0x1AD000` and `0x1E7236..0x300000`. They remain blobs; M13 and
ASM-to-C++ migration stay prohibited.

**TASK:** Continue M12_AUTO from baseline `33cb6d9985b3a87cd9eb92d4ad0883a736bde9ff`
by independently promoting exact source-owned islands inside the recomputed
P0 candidate `0x003B3E..0x004A92`. Preserve the canonical ROM byte-for-byte;
do not begin M13 or ASM-to-C++ migration.

**ACCEPTANCE CRITERIA:** Every promoted interval has exact bounded decode and
vasm round-trip; the full materialized ROM has zero gaps/overlaps and canonical
CRC32/SHA-1/SHA-256; helper regression coverage and M12 documentation are
updated; enclosing mixed/ambiguous regions remain explicitly blob-backed.

# 2026-09-10 — M12.3 transactional ASM promotion — COMPLETE

# 2026-09-10 — M12.4 ROM-start transactional ASM promotion — COMPLETE

TASK: Execute M12.4 from baseline
`3e667c304382c5bce81d2e5a4ff75751139db314` for the mixed
`0x000000..0x0007C4` ROM-start region. Acceptance required explicit vector and
header ownership, evidence-bounded startup code/data promotion, exact target
and full-ROM round trips, classed remaining bytes, deterministic regressions,
documentation, one recomputed M12.5 proposal, and no C++ migration or emulator
expansion.

RESULT: `M12_4_ROM_START_PROMOTION_PARTIAL_EXACT`. The transaction promotes
512 `HEADER_VECTOR_ASM` bytes, 700 exact 68000 ASM bytes, 108 structured-data
bytes, and 0 padding bytes. The target retains 668 blob-backed bytes:
560 `POSSIBLE_CODE`, 104 `UNKNOWN_DATA`, and 4 `UNRESOLVED_BOUNDARY`.
Remaining intervals and reasons are in
`docs/reports/ASM_PROMOTION_000000_0007C4_M12_4.md`.

EVIDENCE: The 64 vector longwords are structurally consumed and the reset
target is `0x00020E`. The fixed Genesis header is emitted byte-for-byte with
explicit `dc.b`. Startup CFG promotion closes at returns/terminal stubs, the
unresolved indirect call at `0x00045A`, and the trusted ASM boundary at
`0x0007C4`. The vasm USP spelling was independently checked; exact `dc.w` is
used for `MOVE USP` because the spelling reverses the canonical `0x4E6x`
encoding.

METRICS: Whole-ROM after materialization is 244 ASM ranges / 15,858 ASM bytes,
2 `HEADER_VECTOR_ASM` ranges / 512 bytes, 1 structured-data ASM range / 108
bytes, 147 blob ranges / 3,129,250 blob bytes, 0 gaps and 0 overlaps. Canonical
CRC32/SHA1/SHA256 remain `C4728225`,
`2944910c07c02eace98c17d78d07bef7859d386a`, and
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

NEXT: The recomputed queue proposes exactly `0x003B3E..0x004A92` (3,924
bytes, 12 observed PCs, 3 static xrefs) for M12.5; it was not started.

VALIDATION: Debug CTest `78/78` passed; Release CTest `78/78` passed. Debug,
Release, and GNU-equivalent MinGW builds linked successfully. Independent
evidence audit reports full-ROM exactness with 239 ASM round-trip records and
5 statically supported records; an independent manifest/source audit passed
contiguous coverage, target partitioning, non-target preservation, explicit
directives, and canonical rebuilt hash. `git diff --check`, source-size
policy, and tracked-artifact hygiene passed. Commit `8b1a70c4c4f9d5dc60e2156aac5d227ac62bb744`
is pushed to `origin/main`. GitHub Actions CI run `34497428251` (`CI`,
`build-test`) passed for that exact SHA. The workflow emitted only the existing
Node.js 20 deprecation annotation.

TASK: Execute the single M12.3 transaction for P0
`0x006516..0x0083D4` from baseline
`b9b55fc46fad88eb2ff1285e85bd9d8169d0289d`. Acceptance required exact
evidence-backed ASM ownership, conservative gap classification, byte-exact
target/full-ROM round trips, deterministic regressions, documentation, and
one recomputed next proposal. No ASM-to-C++ migration, production/runtime
expansion, emulator expansion, G0 reopening, or M12.4 execution was allowed.

RESULT: `M12_3_P0_CODE_PROMOTION_PARTIAL_EXACT`. Fourteen non-overlapping
exact islands were accepted: 258 + 18 + 4 + 10 + 22 + 10 + 48 + 16 + 4 + 14
+ 42 + 28 + 74 + 148 = 696 ASM bytes. The target changes from 0 ASM / 7,870
blob bytes to 696 ASM / 7,174 conservative unresolved-boundary bytes.
`POSSIBLE_CODE_BYTES=0`, `UNKNOWN_DATA_BYTES=0`, and no structured-data or
padding ownership is claimed. The remaining intervals and exact reasons are
in `docs/reports/ASM_PROMOTION_006516_0083D4_M12_3.md`.

EVIDENCE: The 25 observed PCs in `0x0082FC..0x00832C` are interior fallthrough
of the proven `0x0082F8` entry; the priority report's zero static xrefs is
subregion accounting, not absence of code. Atlas-local verified entries at
`0x007A28` and `0x0082AE`, direct CFG edges, exact fallthrough, and bounded
returns/branches support the accepted islands. Multi-seed walks stop at
returns, external unconditional branches, unresolved indirect exits, invalid
or unsupported instructions, data, and overlaps.

DECODER: Added only the required dynamic-bit forms (`BSET/BCHG/BCLR/BTST`),
including `BSET D1,(A1,D0.W)` at `0x007B24` and
`BSET D0,($00FF0DBA).L` at `0x007BEC`. Exact reassembly retains canonical
byte-immediate `0xFF` extension words with `dc.w` when vasm would normalize
the mnemonic encoding. This remains developer-only executable ASM source.

WHOLE-ROM: The exact map changes from 222 ASM ranges / 14,462 bytes
(0.459734599%) and 144 blob ranges / 3,131,266 bytes (99.540265401%) to
236 ASM ranges / 15,158 bytes (0.481859843%) and 147 blob ranges / 3,130,570
bytes (99.518140157%). Gaps and overlaps remain zero. Canonical and rebuilt
ROM identities are unchanged: CRC32 `C4728225`, SHA-1
`2944910c07c02eace98c17d78d07bef7859d386a`, SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

VALIDATION: M12.3 helper and exact-reassembly regressions pass. Fresh Debug
and Release MinGW builds pass, and full Debug and Release CTest are both
`77/77`. A fresh GNU/MinGW-equivalent Release build/link passes; targeted GNU
CTest is `2/2` (`oasis_re_assemble` and `oasis_re_m12_3_helpers`). The
independent materialized-manifest audit confirms all fourteen target
boundaries, four retained intervals, and exact full-ROM SHA-256. The
evidence-integrity audit reports the full ROM exact and 236/236 ranges
materialized; its historical provenance view retains 29 mismatches without
changing the transaction result. `git diff --check`, the source-size limit
(CMakeLists.txt 500 lines; decoder 499; all other changed source files below
500), and tracked-artifact hygiene pass. No ADR was added because this is a
bounded developer-tooling change with no architecture change.

NEXT PROPOSAL: Exactly one M12.4 candidate, `0x000000..0x0007C4` (1,988
bytes, 23 observed PCs, 3 static xrefs), selected by remaining observed-PC
concentration. It was not started. STOP after M12.3.

DELIVERY: Implementation commit `5f22a5ae2644c73f95179d529796a4bcb25c1465`
is published on `origin/main`; the remote SHA matched exactly. GitHub Actions
CI run `34492049406` passed build and test. The only annotation is the
existing Node.js 20 deprecation notice for `actions/checkout@v4`; it is not a
test or build failure.

# 2026-09-10 — M12.1 transactional ASM promotion — COMPLETE

TASK: Execute the single M12.1 transaction for P0
`0x06042A..0x0611F4` from baseline
`b4937c8e1b99de7ca4f45cb20b3cdf37df51e932`. No C++ gameplay/runtime
migration, emulator expansion, ROM/asset commit, or M12.2 execution was
authorized.

RESULT: `M12_1_P0_CODE_PROMOTION_PARTIAL_EXACT`. Six non-overlapping exact
slices were accepted: 90 + 32 + 394 + 10 + 10 + 10 = 546 ASM bytes. The
target changes from 0 ASM / 3,530 UNKNOWN blob bytes to 546 ASM / 2,984
conservative UNKNOWN blob bytes. The gap census is
`POSSIBLE_CODE_BYTES=0`, `UNKNOWN_DATA_BYTES=0`,
`UNRESOLVED_BOUNDARY_BYTES=2,984`. No data interpretation is claimed for the
remaining bytes. The whole materialized map is 209 ASM ranges and 138 blobs,
with 0 gaps and 0 overlaps.

EXACTNESS: All six slices assemble with the existing vasm M68k assembler and
match the canonical bytes. Full rebuilt-ROM identity is exact: 3,145,728
bytes, CRC32 `C4728225`, SHA-1
`2944910c07c02eace98c17d78d07bef7859d386a`, SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
The current decoder normalizes the three legacy `MOVE SR` forms at
`0x06042A`, `0x0611DC`, and `0x0611E6`; the regression and exact reassembly
pass. Remaining dispatch/case and continuation boundaries stay blob-backed.

EVIDENCE: `docs/reports/ASM_PROMOTION_06042A_0611F4_M12_1.md` and the local
transaction output under `build/m12-1-transaction-d`.
NEXT PROPOSAL: Exactly one M12.2 candidate,
`0x00DE00..0x00E338` (1,336 bytes, 31 observed PCs, 87 static xrefs), not
started. STOP after M12.1.

VALIDATION: Targeted Debug/Release reconstruction-tool builds and helper
regression passed before the final full validation. Final Debug/Release CTest,
GNU-equivalent build/link, exact materialized-manifest audit, diff-check,
source-size and tracked-artifact checks, commit/push and CI are recorded below
when complete.

FINAL VALIDATION: Full Debug CTest is 75/75 and full Release CTest is 75/75.
Fresh GNU-equivalent MinGW Release build/link passed for
`oasis_re_assemble`, `oasis_re_assemble_range` and `oasis_re_assemble_test`;
targeted GNU CTest is 2/2. The independent materialized-manifest evidence
audit reports 209/209 ASM ranges round-trip exact; its historical provenance
view retains five mismatches and one dynamic-evidence range without changing
the transaction result. `git diff --check`, changed-source file limits and
tracked artifact hygiene passed. No ADR was added because this is a bounded
developer-tooling change with no architecture change.

DELIVERY: Implementation commit `0d81ad81372a43d623f118965ae9b84d95401402`
is published on `origin/main`; remote SHA matched exactly. GitHub Actions CI
run `34477397124` passed build and test. The only annotation is the existing
Node.js 20 deprecation notice for `actions/checkout@v4`; it is not a test or
build failure.

# 2026-09-10 — M11.64 G0 portability boundary consolidation — COMPLETE

TASK: Consolidate M11.60–M11.63 into one bounded G0 architecture and close the
M11 line. Acceptance required the exact 6b0e43d6ab6547f3232b1bee11e52e80347e425f
baseline gate, complete dependency ledger, four-gate matrix, explicit 0x060286
decision, hardware evidence, method catalog, and no production changes.
RESULT: `G0_PORTABILITY_BOUNDARY_PROVEN_M11_LINE_CLOSED`. The 600-frame target set
0x2D66,0x604BC,0x604F0 matches checkpoint/video and 6,488,773 accounting;
native 5 calls have zero fallback/divergence, shadow is 5/5 with zero divergence,
and the repeat is identical. G0 lifetime is parent-owned; 0x0623AC hardware is a
real portability boundary; typed data remains blocked. 0x060286 is deferred and
has `NO_ARCHITECTURAL_DECISION_CHANGE`.
SCOPE: Documentation/governance only; no new observer, hook, callee, routine,
typed RAM, VDP abstraction, gameplay or production/core change.
EVIDENCE: docs/reports/G0_PORTABILITY_BOUNDARY_M11_64.md; docs/RE_LEDGER.md;
docs/RE_METHOD_CATALOG.md.
VALIDATION: Existing Debug/Release CTest results are 74/74. UCRT remains
LOCAL_TOOLCHAIN_ENVIRONMENT because tests/raw_data_provenance_test.cpp fails
before diagnostics; no new UCRT coverage is claimed. Diff-check, source-limit and tracked hygiene passed before commit/push. Implementation
commit `df9d2b8600cf8ab5fb492982aef5795738a35e57` is on `origin/main`; GitHub Actions
CI run `34459998820` passed build and test. Recommend M12 — Inventory / UI / Save
only as a proposal; STOP after M11.64.

# 2026-09-10 — M11.63 exact 0x0623AC natural callee closure — COMPLETE

TASK: Close the natural G0-crossed callee rooted at 0x0623AC from baseline
0d8844e98c4cbf6143e4bdf0cdc807479b3cf8e4. No excluded routine or production
boundary was in scope.
RESULT: CALLEE_0623AC_BLOCKED_HARDWARE. Static CFG is INDIRECT_CFG at
0x062878. Natural evidence is exact: 1,944 entries/returns, four parent call
sites at 486 each, A5 equality 1,944/1,944, 1,297 direct nested returns and
12 indirect nested returns across four deterministic targets. Effects total
24,993: 2,483 G0-relative, 17,361 safe-RAM, 104 ROM reads, 4,638 stack and
407 VDP writes at 0xC00011; interrupt and unresolved active effects are zero.
EVIDENCE: docs/reports/CALLEE_0623AC_CONTRACT_M11_63.md; repeated traces are
byte-identical with SHA-256 BBAFF3DC24F2EAF94B66D3ECB775C84F9CCCFF73489F9046DE92B30D13531566.
VALIDATION: Baseline and post-change identity retain checkpoint/video hashes
and 6,488,773 accounting (6,488,692 interpreter + 34 TableCopy + 40
RamFlag + 7 ParentSuffix). Targeted Debug regression passed; full Debug and
Release CTest pass 74/74, including source-limit and the new regression.
Diff-check and tracked hygiene are clean. UCRT remains
LOCAL_TOOLCHAIN_ENVIRONMENT because the existing raw_data_provenance_test.cpp
fails before diagnostics; no new-test UCRT coverage is claimed. Implementation commit `b61aab47bd7fb4023b4e9f889b7f09a26598caec` is on `origin/main`; GitHub Actions CI run `34449689357` passed build and test.
NEXT ACTION: Recompute only if new evidence changes the bounded-G0 ledger;
otherwise STOP after M11.63.

# Development Worklog
Chronological record of meaningful project actions. New entries go at the top.

Each task records objective, actions, evidence, tests, result, unresolved questions and exact next step.

# 2026-09-10 — M11.62 exact 0x061934 natural callee closure — COMPLETE

TASK: Close the natural G0-crossed callee rooted at 0x061934 from baseline
3ad8870529486688b7a86974bad0f0cc2180abb8. No unrelated routine or production
boundary was in scope.
RESULT: CALLEE_061934_NATURAL_CONTRACT_PROVEN. The bounded static slice is
INDIRECT_CFG only at unresolved 0x061F60. Natural evidence is exact: 2,916
entries/returns, six parent call sites at 486 each, A5 equality 2,916/2,916,
+0 read count 2,430, +4 write count 1,361, 2,655 direct nested returns and
13 indirect nested returns. Four indirect targets are observed deterministically.
The complete natural effects are 7,017 G0-relative, 36,108 safe-RAM, 338 ROM
reads and 22,302 stack effects; hardware and unresolved active effects are 0.
EVIDENCE: docs/reports/CALLEE_061934_CONTRACT_M11_62.md; repeated traces are
byte-identical with SHA-256 C9C2F9077C62233182854963608E9533377856BD08ABBEEDEFCE526C6CD8CFF6.
VALIDATION: Baseline and post-change identity retain checkpoint/video hashes
and 6,488,773 accounting. Debug and Release CTest pass 73/73; diff-check and
source-limit checks are clean. UCRT again fails at existing
raw_data_provenance_test.cpp before meaningful diagnostics and remains
LOCAL_TOOLCHAIN_ENVIRONMENT. Implementation commit `c3a4d346d38c3b6024a73376c97eca7ddc22632d` is on `origin/main`; GitHub Actions CI run `34447382790` passed build and test.
NEXT ACTION: Propose M11.63 only for the highest-ranked remaining natural G0 blocker, currently 0x0623AC or 0x060286; stop after M11.62.
# 2026-09-10 — M11.61 exact 0x062AE0 callee closure — COMPLETE

TASK: Close one unresolved callee crossed by M11.60 G0 from baseline
424c92c3f583e40c70cc82dcf1a7e9488483e4a6. No other routine or production/core
boundary was in scope.

RESULT: CALLEE_A5_PRESERVATION_PROVEN_EFFECTS_BLOCKED. The expanded static
slice classifies 0x062AE0 as INDIRECT_CFG because reachable 0x062CEC performs
an unresolved indexed JSR. Natural G0 evidence is exact: 486 entries, 486
returns at 0x0601E6, A5 equality 486/486, three paths (310/175/1), zero direct
or indirect nested calls, 2,302 data effects, and no hardware hook events.
Observed effects are FF0013 safe-RAM read, FF001A/+0 G0 read, FF001E/+4 G0
write, A4/A6-derived safe-RAM accesses and stack return reads. The M11.60
transaction gate remains BOUNDED_A5_TRANSACTION_BLOCKED_PARENT_LIFETIME;
typed data remains TYPED_DATA_BLOCKED_OVERLAPPING_ACCESS.

EVIDENCE: docs/reports/CALLEE_062AE0_CONTRACT_M11_61.md; repeated natural
traces are byte-identical with SHA-256
80D4CDCBC9BD85013C70B15FE621070F5BB65AFEC8B78A43784D0790A04BDAC4.
VALIDATION: Baseline and post-change authoritative identity retain the frozen
checkpoint/video and 6,488,773 accounting. Debug and Release CTest pass 72/72
after the new regression. The configured UCRT build was retried and remains
LOCAL_TOOLCHAIN_ENVIRONMENT because the existing raw_data_provenance_test.cpp
fails before compiler diagnostics. Push/CI results are appended after commit.
COMMIT/CI: implementation commit de248d98b5ca9fee058c7d26dd031e0eb202ed26 is on origin/main; GitHub Actions run 34445181596 passed build and test.
NEXT ACTION: Propose M11.62 only for the dominant remaining bounded G0 gap.

# 2026-09-10 — M11.60 bounded A5 consumer/lifetime closure — COMPLETE

TASK: Close the natural A5 generation rooted at 0x060182 from baseline
37ef10694bdab1a52b042f79bc8e1f480f3f25f2 without changing production/core.
ACCEPTANCE CRITERIA: reproduce M11.59 identity; recover exact CFG and G0
consumers; prove or falsify the lifetime endpoint; classify calls, relation to
M11.58 and typed gate; add only developer-only deterministic provenance test;
validate Debug/Release/UCRT and governance.

RESULT: BOUNDED_A5_CONSUMER_LIFETIME_PROVEN. Static CFG classifies the region
as PARENT_OWNED_REGION. TST/BCC makes the +7 arm dead; natural G0 consumers
are 06193C +0 read (10) and 061946 +4 write (1,361). The exact endpoint is
06027E MOVEM restore, so A5_LIFETIME_MERGES_WITH_PARENT. The raw transaction
is BOUNDED_A5_TRANSACTION_BLOCKED_PARENT_LIFETIME; relation to M11.58 is
ARE_ALTERNATE_PRODUCER_CONSUMER_PATHS; typed gate remains
TYPED_DATA_BLOCKED_OVERLAPPING_ACCESS. No typed data, subsystem, 0x60BCC
analysis or new routine was added.

EVIDENCE: docs/reports/A5_CONSUMER_LIFETIME_M11_60.md; the opt-in observer
produced three byte-identical 600-frame traces with 486 generations/kills.
VALIDATION: authoritative checkpoint/video/accounting identity unchanged;
targeted hybrid and line-limit checks passed. Full Debug and Release CTest
pass 71/71. The configured UCRT build still fails compiling the existing
raw_data_provenance_test.cpp before source diagnostics (exit 1), so its prior
69/69 suite is not evidence for this new test. This remains
LOCAL_TOOLCHAIN_ENVIRONMENT, per scope.
Post-change authoritative 600-frame runs also match exactly: native override
has 6,488,692 interpreter + 81 translated = 6,488,773 total, zero fallback
and zero divergence; shadow has 5/5 comparisons, zero divergence and the
expected five emulated fallback entries.
NEXT ACTION: Propose M11.61 for one selected callee preservation/effect gap,
starting at 062AE0; do not execute it in M11.60.
COMMIT/CI: implementation commit c5cc77fb6b3718f773b9e0e7b16e156a381e7033
is on origin/main; GitHub Actions run 34442718782 passed build and test.

# 2026-09-10 — M11.59 raw data alias/lifetime closure — COMPLETE

TASK: Close the smallest raw-data ownership, alias/lifetime and external-writer
blocker around the M11.58 cluster from baseline `9d75a836f727989158761d666c112cc0b76d2888`.
ACCEPTANCE CRITERIA: reproduce paired native/shadow identity; complete the
all-ROM fixed census; trace A5 producers, copies, readers and writers; close
the bounded 0x60BCC writer relationship without promotion; falsify aliases;
classify A-E; add no typed data unless every gate condition passes; update
ledgers and run pre-push validation.

RESULT: `RAW_DATA_ALIASING_BOUNDARY_PROVEN_TYPED_DATA_BLOCKED`. Three LEA
producers establish A5=`FF001A`; the `061258` path saves/reloads A5 and clears
through `0x762` post-increments, while other paths consume 0/4/5/7(A5).
Fixed bytes have multiple bounded writers. The 0x60BCC sibling writes match
the raw order after an A11100 hardware prefix and remain
`HARDWARE_ORDERED_WRITER`. No production/core abstraction or typed structure
was added.

EVIDENCE: `docs/reports/RAW_DATA_OWNERSHIP_M11_59.md`; added the standalone
decoder provenance regression `tests/raw_data_provenance_test.cpp` and CTest
registration.
VALIDATION: Two native and two shadow runs preserve the frozen checkpoint,
video, exact `6,488,773` accounting, zero fallback/divergence and shadow 5/5.
Debug and Release CTest pass `70/70`; diff, source-size and hygiene checks
pass. Existing UCRT CTest was `69/69`; the configured UCRT compiler now exits
1 without diagnostics even for a trivial compile, leaving the new test not
run. This is recorded as a local toolchain limitation.
NEXT ACTION: Propose M11.60 only for one bounded A5 consumer/lifetime closure.

# 2026-09-10 — M11.58 portable behavior cluster boundary — COMPLETE

TASK: Determine whether the proven RamFlagRoutine + parent-owned ParentSuffix
composition has an evidence-backed raw-data ownership contract from baseline
`f5f0118325dad3b36961a546ca5e06fd866d8f95`.
ACCEPTANCE CRITERIA: reproduce the M11.57 identity twice; document control,
data, continuation and adapter dependencies; census every raw byte/range;
close or reject typed data; classify the cluster; run independent and 600-frame
shadow/native gates; update ledgers and preserve all parent/hardware boundaries.

BASELINE GATE: PASS. Two native and two shadow runs retain checkpoint
`251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d`, video
`5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`, exact
`6,488,773 = 6,488,692 interpreter + 34 TableCopy + 40 RamFlag + 7 helper`,
zero fallback/divergence and shadow `5/5`.

AUDIT: The existing ParentSuffix contract is already the smallest explicit
composition and keeps RamFlag, raw addresses and opaque continuation separate.
The footprint is `FF0010..FF0014`, `FF0016`, `FF0628`, `FF06F2` and the
parent-base-derived `FF001F..FF0021`. Known external writers and unresolved
alias/lifetime boundaries block any typed structure; no semantic names were
introduced.

RESULT: `PORTABLE_BEHAVIOR_CLUSTER_CONTRACT_PROVEN_REPLACEMENT_BLOCKED`.
No production code changed. Full Debug MinGW, Release MinGW and GNU/UCRT CTest
pass `69/69` each; diff, line-limit and repository-hygiene checks pass. Debug
and Release rebuilds pass. The configured UCRT compiler relink returns
`collect2.exe` exit 53 without a source diagnostic; its existing build remains
CTest-green and this local toolchain limitation is recorded.
`game.srm` remains unchanged and untracked. Evidence:
`reports/PORTABLE_BEHAVIOR_CLUSTER_M11_58.md`.

NEXT ACTION: Propose M11.59 only for the dominant raw-data alias/lifetime and
external-writer blocker; do not execute it in this milestone.

# 2026-09-10 — M11.57 parent-owned suffix handoff — COMPLETE

TASK: Determine whether the proven hardware-free `0x604F0` suffix can be an
architecture-neutral parent-owned helper from baseline
`ca95f24ebb7b92d2943166a802d8aeef8a70d5d0`.
ACCEPTANCE CRITERIA: exact baseline twice; minimal entry/exit and ownership
contract; adverse entry vectors; every helper boundary event/resume; structural
RamFlag composition; natural shadow; authoritative native identity; explicit
accounting and documentation.

BASELINE GATE: PASS twice with checkpoint
`251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d`, video
`5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`,
`6,488,773 = 6,488,699 interpreter + 34 TableCopy + 40 RamFlag`, zero
fallback/divergence, attribution `1 x 0x604F6 / 3 x 0x60BCC / 0 unknown`, and
the existing one yield/resumption.

IMPLEMENTATION: Added `oasis_core` `ParentSuffixMachine` with opaque phase and
parent-continuation tokens. It owns the five safe-RAM SF writes and composes
the existing RamFlag executor. The hybrid adapter retains ROM/GPGX validation,
per-instruction fetch/timing, nested BSR representation, and `0x611D6` mapping.
Parent frame, hardware prefix, SR restoration, shared epilogue and RTS remain
parent-owned.

RESULT: `FIRST_PORTABLE_INTERNAL_HELPER_PROVEN`.
`oasis_parent_suffix_test` passes exact write order, invalid A5/continuation
rejection and all 17 represented boundary resumptions without duplicate
writes or RamFlag calls. The 600-frame `SHADOW_NATIVE` run is 5/5 with zero
divergence and frozen checkpoint/video identity. The 600-frame `NATIVE_OVERRIDE`
run reports 7 helper instructions, 40 RamFlag, 34 TableCopy and 6,488,692
interpreter instructions, summing exactly to 6,488,773 with zero fallback or
divergence. The legacy report `full_cpu_equivalence` field remains false for
NATIVE_OVERRIDE by existing schema policy; serialized checkpoint/video,
timing/refresh, RAM and continuation gates are exact.

TESTS: Full Debug MinGW, Release MinGW and GNU/UCRT CTest pass 69/69 each;
Release and UCRT authoritative helper runs also preserve both frozen hashes,
exact accounting, zero fallback and zero divergence. `git diff --check` and
the source line-limit check are pending final review.
EVIDENCE: reports/PARENT_SUFFIX_HANDOFF_M11_57.md.
NEXT ACTION: Complete cross-configuration validation, review diff and commit.

# 2026-09-09 — M11.56 caller continuation closure — COMPLETE (negative)

TASK: Close the exact 0x604F0 entry/exit contract from baseline
47c37ead7beb9a7e063588cd39a5cbf65deb41e8 and attempt promotion only after
every contract gate passes. WHY: the 0x611D6 continuation is unresolved.
CURRENT MILESTONE: M11.56. MILESTONE UNDERSTANDING CONFIDENCE: 70%.
CURRENT SLICE UNDERSTANDING CONFIDENCE: 70%; evidence-gathering only.
SLICE CONFIDENCE EVIDENCE: exact seven-instruction region and four-instruction
shared restore/RTS tail; enclosing stack ownership still needs provenance.
ACCEPTANCE CRITERIA: unchanged dual-native baseline twice; deterministic
bounded entry/exit and memory evidence; exact positive or negative gate;
Debug/Release/UCRT CTest, hygiene, focused commit/push and exact-SHA CI check.
EVIDENCE AVAILABLE: M11.55 report, canonical ROM, external GPGX bridge.
KNOWN UNKNOWNS: entry owner, saved stack frame, all incoming edges and
portable continuation. No 0x60BCC investigation, typed data or coverage search.

BASELINE GATE: PASS twice, including frozen hashes, 6,488,773 total,
6,488,699 interpreter, 34 TableCopy, 40 RamFlag, zero fallback/divergence,
one yield/resumption, and byte-identical M11.55 caller attribution.

RESULT: THIRD_ROUTINE_NOT_A_STANDALONE_ROUTINE. Dominant blocker is
ENCLOSING_ROUTINE_BOUNDARY. The natural 0x604EA -> 0x604F0 suffix inherits
the 58-byte frame created by 0x60004/0x6042A; 0x611D6 is its shared
restore/return epilogue and returns to original caller 0x424. The exact
seven-instruction span ends at 0x60516; a different arm enters there inside
the old 0x60520 budget. Parent prefix hardware and full-SR/event semantics
prevent promoting the enclosing path. No production routine was implemented.

EVIDENCE: reports/RAMFLAG_CALLER_ROUTINE_M11_56.md. Paired natural EMULATED
trace SHA-256 8b23fce6088956ecdc443bf432f183a6204eabe6fdf0aeb71dbe9e29087ea877;
21 path instructions / 32 ordered RAM accesses, complete parent save/restore
and nested/original return discrimination. Independent validator rejects
wrong final A7, missing MOVEM extra read, and incomplete capture.

TESTS: Rebuilt Debug and Release MinGW full CTest 68/68 each; rebuilt
GNU-equivalent GCC/UCRT full CTest 68/68. These include TableCopy/RamFlag,
mechanical primitives, native continuation/dispatch adapters, caller
attribution, new observer and checkpoint canonicalization/layout tests.
Post-edit Debug/Release 600-frame dual-native runs match every baseline gate,
all ten canonical checkpoint/cycle pairs and attribution. New third-routine
tests/shadow/three-routine proof are not applicable because promotion stopped.
The source line-limit and git diff checks pass. Existing game.srm SHA-256
CD1CD62F7EB68F68D0CB13E22FC160CD6396B1CBAE1FE35C8BBE79B38E352F1F is unchanged.
No ROM, asset, emulator binary or raw run evidence is staged. GCC/UCRT provides
local GNU link/portability validation; native Linux CI is not claimed locally.

DECISION: ADR-0037; high confidence in the bounded negative result, production
slice remains below 90%. NEXT ACTION: STOP. Proposed M11.57 closes only the
explicit parent-owned suffix handoff and full-SR/event contract; no unrelated
parent-arm/hardware expansion or relabeling an internal helper as a routine.

# 2026-09-09 — M11.55 RamFlag caller/data closure — COMPLETE

TASK: From baseline `160422e8a280890a01f6e314a57f7e024b644ba0`, reproduce the
M11.54 dual-native proof, dynamically attribute every natural `0x0604BC`
entry, close bounded caller CFG/semantics and shared-memory provenance for
`0x0604F6`/`0x060BCC`, audit the `0xA11100` boundary, and stop before
conditional implementation unless a complete contract is proven.

RESULT: `RAMFLAG_CALLER_CONTRACTS_PROVEN`. One natural call from `0x0604F6`
and three from `0x060BCC` were paired with exact return PC, A7, frame,
cycle/refresh and register snapshots; unknown attribution is zero and the two
run JSON artifacts are byte-identical. The `0x0604F0` and `0x060BC4` caller
regions have exact direct CFG evidence, but neither is a complete routine.
Fixed RAM byte effects are proven at offsets 0,1,2,3,4,6; offset 5, A5-
relative effects, lifetime, aliasing and type are unresolved. `0x060BC4` is a
proven hardware prefix before RamFlag, but the wider region remains hardware-
boundary incomplete. No production source, third routine, typed structure or
subsystem was implemented.

EVIDENCE: `docs/reports/RAMFLAG_CALLER_DATA_CLOSURE_M11_55.md`.

TESTS: Fresh GPGX-enabled Debug MinGW full CTest `67/67`; Release GPGX-
enabled MinGW full CTest `67/67`; GNU-equivalent UCRT/MinGW full CTest
`67/67`; synthetic attribution test passed; two fresh 600-frame
`NATIVE_OVERRIDE` runs matched checkpoint/video/accounting/yield/resumption
identity with zero fallback/divergence; `git diff --check` and source-code
file-limit validation passed.

DECISION: `RAMFLAG_CALLER_CONTRACTS_PROVEN`; conditional implementation gates
remain closed. Proposed M11.56 only: independently close either the
`0x0604F0` continuation owner or the `0x060BC4..0x060CDA` sibling/data
contract. Do not execute that milestone here. No ROM, asset, binary or run
evidence is tracked; pre-existing `game.srm` and historical artifacts remain
unmodified.

# 2026-09-09 — M11.54 native routine cluster discovery — COMPLETE

TASK: From baseline 6c81803dbd230ff54862d6ae8e8a04a7c79f727d, reproduce the
unchanged M11.53 dual-native proof, establish exact caller/callee and
shared-memory provenance for 0x2D66 and 0x604BC, classify bounded
neighborhoods/blockers, and stop without forcing a third routine or subsystem.

RESULT: Baseline A/B passed with aggregate
251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d, video
5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58, total
6,488,773 = 6,488,699 interpreter + 34 TableCopy + 40 RamFlag, zero
fallback/divergence, and one yield/resumption at 0x604DA.

EVIDENCE: Static exact slices prove 0x2D58 -> 0x2D66 and
0x604F6/0x60BCC -> 0x604BC. Target entry totals are dynamic-only because the
runner does not capture caller PCs. The RamFlag-centered call/data cluster
shares raw 0x00FF0010..0x00FF0016, but caller CFG/semantics and hardware
isolation are incomplete. TableCopy and RamFlag have no proven shared edge or
structure. Architecture inventory is 2 authoritative routines, 4 mechanical
primitives, 2 complete contracts, 3 partial contracts, 0 typed structures,
0 subsystem candidates, 1 hardware-blocked and 2 continuation-blocked.

DECISION: PORTABLE_ROUTINE_CLUSTER_PROVEN. No third routine, typed data,
hardware behavior or subsystem boundary was implemented. Proposed M11.55 is
bounded caller/data closure for 0x604F6/0x60BCC and is not executed here.
Full report: docs/reports/NATIVE_ROUTINE_CLUSTER_M11_54.md.

TESTS: Fresh Debug and Release MinGW full CTest passed `66/66`; fresh
GNU-equivalent UCRT/MinGW full CTest passed `66/66`. Direct mechanical and
developer-only candidate/dispatch regression executables passed. The fresh
standalone configurations do not emit the conditional GPGX-linked hybrid POC
target; the unchanged M11.53 POC reproduced the native baseline twice.
`git diff --check` and the source-code file-limit gate passed. No source or
runtime files changed.

# 2026-09-09 — M11.53 second portable native routine — COMPLETE

**TASK:** From baseline f2d82cba8fc7c038d28652949783d38f0df5f1a0, prove and
promote exactly one second complete portable native routine. The pre-change
baseline gate must reproduce M11.52 twice; the selected routine must have a
closed contract, independent core vectors, per-instruction hybrid continuation,
paired 600-frame identity and separated accounting. If any gate fails, stop
and record the negative result.

**ACCEPTANCE:** Candidate 0x604BC..0x604E6 is the selected bounded direct-RTS
RAM flag/output leaf. It has one reachable block, ten instructions, no calls,
indirect edges or loops, four natural calls and safe RAM/stack effects.
oasis_core owns only the structured flag/output semantics and continuation
tokens; tools/hybrid owns ROM opcode/extension validation, GPGX fetch,
begin/finish, prefetch/RTS state and boundary mapping. The dual native proof
must preserve the frozen aggregate/video identity and total
6,488,773 = 6,488,699 interpreter + 34 TableCopy + 40 second routine.

**CONFIDENCE BEFORE PRODUCTION CODE:** 95%. The candidate's exact range,
natural ledger, safe-memory evidence and 4/4 shadow proof were already
documented; the remaining uncertainty was the M11.52 per-instruction
continuation and full checkpoint identity, which are now being tested
explicitly.

**RESULT:** Baseline A/B reference/native passed exactly. The extracted
0x604BC adapter passed 4/4 shadow, standalone core vectors, exact bridge
regression and dual 600-frame native A/B. The authoritative paired aggregate
is 251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d and the
video hash is 5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58.
Accounting is 6,488,699 interpreter + 34 TableCopy + 40 second routine =
6,488,773, with one native event yield, one resumption, and zero
fallback/divergence. Debug, Release and GNU-equivalent full CTest all passed
66/66, including source line-limit and core dependency-boundary checks;
`git diff --check` passed; the source-limit gate reports `runner.cpp` at the
500-line ceiling, and tracked repository hygiene is clean with all ROM-backed
run evidence left untracked.

## 2026-09-09 — M11.52 native routine checkpoint mismatch root-cause closure — COMPLETE
**TASK:** Close the M11.51 authoritative checkpoint mismatch for the single
`TableCopyRoutine` call at `0x2D66..0x2D84`.

**ACCEPTANCE:** Identify every differing byte and owner; localize the first
temporal divergence; explain the shadow blind spot; repair only the causally
proven generic continuation layer; add a regression that fails under M11.51;
preserve checkpoint comparison and complete paired authoritative evidence.

**ROOT CAUSE:** M11.51's portable register/memory routine was correct. Its
hybrid adapter replaced 34 represented 68000 instructions with one `2828`
cycle update and one refresh update. Entry/exit cycles matched, but refresh
ended at `194704` instead of reference `196622`; GPGX instruction-boundary
prefetch/refresh and the following scheduler phase were not represented.
The shadow compared portable state/output/stack before that post-return state,
so it had `SHADOW_FIELD_NOT_COMPARED`, `POST_RETURN_STATE_NOT_COMPARED`,
`PREFETCH_NOT_FULLY_MODELED` and `SCHEDULER_PHASE_NOT_COMPARED` blind spots.

**BYTE EVIDENCE:** The first M11.51 divergence was frame 120 and consisted of
four isolated canonical bytes: `3060 F4->EE` (work RAM `0xFF0BE4`),
`144468 EE->F4` and `144482 13->2F` (pinned semantic sound/PSG region with
field names unresolved due to the M11.43 source/object layout discrepancy),
and `144558 96->87` (Z80 `iff1`, base `144504 + 54`). None is a host
representation span; canonicalization was unchanged.

**IMPLEMENTATION:** Added developer-only hybrid callbacks for exact instruction
fetch/begin/finish. The 2D66 adapter now validates opcodes and extension words,
reconstructs DBF branch continuation, uses the existing MOVEM dynamic timing
operation and restores RTS PC/prefetch state at the correct boundary. The
portable `oasis_core` routine and checkpoint canonicalizer were not changed.

**REGRESSION:** The candidate test checks the exact bridge sequence, DBF
extension, MOVEM/DBF timing adjustment and absence of one-shot refresh skip; a
legacy boundary model asserts the M11.51 refresh result is not `196622`.

**AUTHORITATIVE EVIDENCE:** Repaired 600-frame native replacement matches all
ten per-frame canonical reference hashes and the frozen aggregate
`251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d`, exact
video `5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`,
cycles `193626->196454`, refresh `193808->196622`, 34 native instructions,
13 iterations, 6,488,773 total and zero fallback. Repaired shadow remains one
comparison/zero divergence. Full evidence: `docs/reports/NATIVE_ROUTINE_MISMATCH_M11_52.md`.

**RESULT:** `FIRST_PORTABLE_NATIVE_ROUTINE_PROVEN`. No second routine,
canonicalization exception, gameplay semantics or production emulator
dependency was added. Final Debug, Release and GNU/MinGW-equivalent builds
and CTest all pass `64/64`; `git diff --check`, source-limit,
core-dependency and repository-hygiene gates pass. Two final native runs are
byte-for-byte deterministic at the canonical manifest level and each matches
the reference. GitHub Actions `CI` run `34369580063` for commit
`d84bf14610cd3699edbd1f9f43f68f372668e811` passed build and test.

## 2026-09-09 — M11.51 First Portable Native Routine Reconstruction — REPLACEMENT BLOCKED
**Objective:** Extract exactly one complete, evidence-backed structural ROM
routine into `oasis_core` as portable structured C++, while preserving the
M11.50 generated/mechanical baseline and stopping on any identity mismatch.

**Acceptance criteria:** Reproduce the M11.50 baseline twice before edits;
inventory and select one complete routine; prove its CFG, entry/exit, effects,
flags, timing boundary and resumable contract; add an independent core oracle;
run unchanged 600-frame shadow and authoritative replacement gates; close
separate execution accounting; update governance and push a focused result.

**Actions:** Selected structural `TableCopyRoutine` at `0x2D66..0x2D84` from
the existing exact decoder, M11.29 natural shadow and M11.45–M11.50 ledgers.
Added the ROM-independent `src/core/table_copy_routine.*` contract/executor,
hybrid adapter extraction, native routine accounting, standalone semantic and
synthetic CFG tests, and the M11.51 report. The core has no ROM PC constants,
GPGX/libretro types, generated block types, checkpoint dependency or PC
dispatch. Existing generated code remains the oracle/fallback.

**Evidence:** Two unchanged M11.50 native baselines matched checkpoint
`251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d`, video
`5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`, total
6,488,773, generated 6,199,381, mechanical 42,384, interpreter 247,008,
587 ranges, 150,135 yields and 288 resumptions. Standalone routine, CFG and
hybrid shadow tests pass with zero divergence; natural shadow observed one
call, 34 represented instructions and 13 copied words. The isolated native
candidate closes 6,488,773 as 6,488,739 interpreter plus 34 native routine
instructions and preserves video, but produces checkpoint aggregate
`ae8887f5b32a4973a8243775612d68b891b588f6f5dc693558a5a8a2489e5403`.

**Validation:** Targeted Debug builds/tests passed. Full Debug, Release and
GNU/MinGW-equivalent CTest each passed 64/64, including standalone core,
primitive regression, dependency boundary, semantic/CFG/yield/oracle,
provenance and checkpoint tests. Final 600-frame baseline A/B runs matched
the frozen identity; shadow had one comparison and zero divergence. The native
candidate closed accounting at 6,488,773 but failed the frozen CPU/RAM/
checkpoint identity while video remained exact, so it is not promoted.
Source-limit, `git diff --check` and hygiene checks are also green.

**Result:** `PORTABLE_NATIVE_ROUTINE_SHADOW_PROVEN_REPLACEMENT_BLOCKED`.

**Unresolved:** The exact current-pinned-GPGX CPU/RAM/timing/refresh continuation
delta causing the native checkpoint mismatch is not attributed. No timing or
hardware behavior was invented to force equality. Nearby candidates remain
blocked by their documented hardware/bus/semantic contracts.

**Exact next step:** Stop; investigate the four-byte serialized checkpoint
delta and its timing/refresh provenance in a separate bounded milestone.

## 2026-09-09 — M11.50 Portable Mechanical Primitive Layer — COMPLETE
**Objective:** Extract the proven M11.49 mechanical primitive semantics into
`oasis_core` while keeping ROM-specific registry/provenance and all GPGX
hybrid machinery developer-only. Do not add candidates, gameplay semantics or
hardware behavior.

**Acceptance criteria:** Reproduce the M11.49 native baseline twice before
editing; classify dependencies; provide a minimal portable contract and
continuation state; add standalone synthetic semantics and dependency-boundary
tests; reconnect the hybrid adapter; pass the existing primitive shadow/native
identity; update governance and preserve ROM/run-evidence hygiene.

**Actions:** Added `src/core/mechanical_primitive.*` with opaque instruction
tokens, portable register/memory/timing-boundary callbacks, ordered byte copy,
byte/word clear, CCR/X effects, DBF low-word semantics, 32-bit wrapping and
fail-closed continuation validation. Removed semantic ownership from the
hybrid implementation. The hybrid layer now supplies only the four proven ROM
contracts, canonical body/DBF encoding checks, GPGX/BasicBlock timing and
prefetch adapter, detached snapshot, shadow comparator, registry metrics and
reporting. Added a standalone core test and source/CMake dependency boundary.

**Evidence:** Two unchanged pre-extraction native runs matched checkpoint
`251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d`, video
`5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`, totals
6,488,773 / 6,199,381 generated / 42,384 mechanical / 247,008 interpreter,
150,135 yields and 288 resumptions. Post-extraction shadow compared
6,284,149 combined generated/mechanical instructions including 42,384
mechanical instructions with zero divergence. Post-extraction native preserved
the same identity and all frozen counts, with zero fallback, hardware access,
divergence or starts inside translated ranges.

**Validation:** Targeted Debug standalone, hybrid and dependency tests passed;
Debug shadow/native 600-frame proofs passed. Full Debug/Release/GNU-equivalent
CTest, source-limit, diff and hygiene checks are recorded in the M11.50 report.

**Result:** `PORTABLE_MECHANICAL_PRIMITIVE_LAYER_PROVEN`.

**Unresolved:** The M11.49 interpreter Pareto remains unchanged at 247,008;
`0x060BA4` remains hardware-visible blocked. No next routine was implemented.

**Exact next step:** Stop; require a separate bounded milestone before the
next abstraction inventory is implemented.

## 2026-09-09 — M11.49 Mechanical Primitive Family Closure — COMPLETE
**Objective:** From M11.48 baseline `678534862ad16be7cc1627fe4ed5008f53c7365f`,
determine whether the proven resumable mechanical primitive generalizes to the
remaining M11.47 copy/clear loops. Do not add gameplay semantics, candidates,
hardware emulation or production dependencies.

**Acceptance criteria:** Reproduce the M11.48 authoritative native baseline
twice; reconstruct exact contracts for both byte copies and word clear; add
generic fail-closed executor and per-candidate provenance; prove synthetic
semantics and resumable coexistence; pass unchanged 600-frame shadow/native
gates with exact state, video, counts, yields and resumptions; update all
governance; preserve ROM/game.srm/run-evidence hygiene.

**Actions:** Replaced the single-form mechanical API with metadata-driven
`MechanicalLoopContract` and one generic body/DBF executor. The registry now
covers `MEMORY_COPY` at `0x003A0C`/`0x00389E` and `MEMORY_CLEAR` word at
`0x0003F0`, while retaining the M11.48 byte-clear member. The implementation
keeps generated M11.47 bodies as oracle/fallback, performs explicit ordered
copy accesses, preserves width-specific CCR/X behavior, records per-candidate
reads/writes/hardware visibility and fails closed on unsupported forms.

**Evidence:** Two pre-change native baselines matched the authoritative
aggregate `251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d`,
video `5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`,
6,488,773 total / 6,237,985 generated / 3,780 mechanical / 247,008
interpreter, 587 ranges, 150,135 yields and 288 resumptions. Shadow compared
42,384 mechanical instructions with zero divergence. Native preserved the same
identity and exact totals with 6,199,381 generated / 42,384 mechanical /
247,008 interpreter instructions, 150,135 yields, 288 resumptions, zero
unexpected fallback entries and zero starts inside translated ranges. The
family recorded 2,216 invocations, 21,192 iterations, 14,737 reads and 21,192
writes with zero hardware-visible accesses; no hardware behavior was added.
The four candidate counts sum to 42,384 exactly.

**Validation:** Targeted mechanical unit test passed after synthetic fixes.
Debug, Release and GNU-equivalent full CTest, semantic/generator/provenance/
checkpoint/boundary tests, two authoritative baseline runs, mechanical shadow,
mechanical native proof, `git diff --check`, source line limits and repository
hygiene were rerun on the final diff. CI was checked after push.

**Result:** `NATIVE_MECHANICAL_PRIMITIVE_FAMILY_PROVEN`.

**Unresolved:** The M11.47 interpreter Pareto remains unchanged at 247,008;
`0x060BA4` remains hardware-visible blocked. Extraction is classified ready as
a future refactoring boundary but was not performed. No next milestone is
implemented.

**Exact next step:** Stop.

## 2026-09-09 — M11.48 First Proven Native Mechanical Primitive Replacement — COMPLETE
**Objective:** From M11.47 baseline `f51f3b370b8e7acdc3613b1cec7f9ffcdf05f14e`,
promote the safest complete mechanical loop without adding gameplay semantics,
hardware behavior, candidates or a production emulator dependency.

**Acceptance criteria:** Keep generated/basic-block code as oracle/fallback;
provide an architecture-neutral resumable primitive API; prove synthetic and
GPGX instruction-boundary interruption behavior; pass unchanged 600-frame
shadow/native gates; update governance and hygiene; commit and push this
focused result.

**Actions:** Added `mechanical_primitive.hpp/.cpp` as a developer-only generic
machine/registry layer. The selected `MEMORY_CLEAR` loop is dispatched by
metadata, executes one body/DBF instruction at a time, preserves CCR/register/
memory/timing state, and yields/resumes at existing event boundaries. The
existing generated block oracle remains active in primitive shadow mode; the
primitive adapter contains no GPGX types or handwritten candidate body.
Added synthetic interruption coverage and runner/CMake integration.

**Evidence:** Baseline native run was reproduced twice before promotion with
aggregate `251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d`,
video `5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`,
6,488,773 total / 6,241,765 translated / 247,008 interpreter, 587 ranges,
150,135 yields and 288 resumptions. Primitive shadow compared 3,780 logical
guest instructions with zero divergence, including 86 mid-operation yields and
86 resumptions. Native 600-frame proof preserved identity and full CPU
equivalence with 3,780 mechanical executions, zero fallback and zero hardware
accesses. The M11.47 remainder ledger was rerun and closed at 247,008 exactly.

**Validation:** Debug targeted build/test and full CTest, Release full CTest,
GNU-equivalent full CTest, semantic/generator/provenance/checkpoint/boundary
tests, 600-frame primitive shadow/native runs, `git diff --check`, source line
limits and repository hygiene passed. Final detailed evidence is in
`docs/reports/NATIVE_MECHANICAL_PRIMITIVE_M11_48.md`.

**Result:** `FIRST_NATIVE_MECHANICAL_PRIMITIVE_PROVEN`.

**Unresolved:** The three other M11.47 copy/clear structures remain
`NEEDS_CONTRACT_WORK`; no hardware behavior was broadened. `game.srm`, ROM,
assets, emulator binaries and run outputs remain untracked.

**Exact next step:** Stop; do not implement the next milestone.

## 2026-09-09 — M11.47 Safe-Memory Semantic Closure and Primitive Discovery — COMPLETE
**Objective:** From baseline `7cf947cffee7507e6157e147049bb2b746baa3fa`, close
only the highest-payoff M11.46 `SAFE_MEMORY_OBSERVED` interpreter rows with
independent exact semantics and mechanical generated code. Discover only
evidence-backed mechanical copy/clear primitives; do not broaden hardware or
replace primitives natively.

**Acceptance criteria:** Reproduce the M11.45/M11.46 baseline twice; verify the
seven required exact forms with deterministic vectors; generate canonical
decoder-owned bodies with shared helpers and fail-closed unsupported variants;
pass per-instruction GPGX shadow and unchanged 600-frame native proof; close
the remaining interpreter ledger exactly; update governance, hygiene and all
required validation evidence; commit and push the focused result.

**Actions:** Added shared generated helpers for the exact MOVE/CLR/BTST forms,
mechanical generator branches, generated M11.47 bodies/registry and an
independent semantic/provenance test. The predecrement long write helper uses
the pinned GPGX-observed high-word-then-low-word 16-bit bus sequence with
24-bit bus-visible addresses and 32-bit register wrap. Added the exhaustive
post-promotion ledger and primitive suitability report.

**Evidence:** Two unchanged baseline runs matched checkpoint
`251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d`, video
`5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`, totals
6,488,773 / 6,199,718 / 289,055, 580 ranges, 149,059 yields and 288
resumptions. The seven selected rows remove exactly 42,047 executions.
Shadow completed 6,241,765 comparisons with zero divergence. Native completed
600 frames at 6,241,765 translated and 247,008 interpreter executions across
587 ranges, preserving the authoritative identity and zero hardware-visible
accesses. The final ledger closes exactly at 247,008.

**Validation:** Targeted M11.47 semantic tests, generator/provenance tests,
shadow and native proof passed. Full Debug CTest passed 59/59, full Release
CTest passed 59/59 on the serialized rerun, and GNU-equivalent MinGW CTest
passed 59/59. The first parallel Release attempt hit a shared temporary-file
collision between self-tests; the isolated rerun was green. `git diff --check`,
changed/new source line-limit checks and repository hygiene passed.

**Result:** `SAFE_MEMORY_SEMANTIC_CLOSURE_PROVEN`.

**Exact next step:** Review the final diff, commit and push this focused M11.47
result, verify CI and stop.

## 2026-09-09 — M11.46 Runtime Address Provenance and Memory-Class Resolution — COMPLETE
**Objective:** Resume from the committed M11.45 baseline and resolve, with
runtime evidence only, the dominant register-address `UNKNOWN_WITH_EVIDENCE`
remainder. Do not promote semantics, broaden hardware, alter generated blocks
or redefine the 95% gate.

**Acceptance criteria:** Reproduce the unchanged 600-frame baseline twice;
observe the existing GPGX fallback path in three independent processes; close
the 635 register-address unknown PCs with exact counts; record address, width,
direction, ordered bus sequence and A-register transitions; classify observed
addresses without invented semantic names; preserve unresolved no-bus cases;
update governance, hygiene and validation evidence; commit and push the
bounded positive result.

**Actions:** Re-read repository instructions and required project documents.
Added `address_provenance` to the developer-only hybrid contract and a distinct
`BASIC_BLOCK_ADDRESS_PROVENANCE` runner mode. The observer wraps the existing
GPGX execution/data-bus/post-instruction callbacks and does not read or write
machine state. Mapper-dependent cartridge SRAM remains unclassified when the
address alone is insufficient.

**Evidence:** Two unchanged native runs matched checkpoint
`251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d`, video
`5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`, totals
6,488,773 / 6,199,718 / 289,055, 580 ranges, 149,059 yields and 288
resumptions. Three provenance runs matched byte-for-byte and recorded
289,055 entries with zero unclosed instructions. The 635 final unknown PCs
sum exactly to 149,678 executions; 147,847 are runtime-classified and 1,831
address-computation-only executions remain exact-evidence unresolved.

**Validation:** Debug full CTest 58/58 passed; Release full CTest 58/58
passed; GNU-equivalent MinGW full CTest 58/58 passed. The exhaustive ledger
recheck closed at 289,055. Final shadow completed 6,199,718 comparisons with
zero divergence, followed by an unchanged native 600-frame proof preserving
all M11.45 identity metrics. The address observer regression passed in Debug
and Release. `git diff --check` and source line-limit checks passed. Repository
hygiene review found no tracked ROM/assets/emulator binaries/generated run
evidence; `game.srm` remains untouched and untracked.

**Result:** `BOUNDED_RUNTIME_ADDRESS_PROVENANCE_PROVEN`; no semantic candidate,
hardware contract or native runtime path was changed. Evidence:
`docs/reports/RUNTIME_ADDRESS_PROVENANCE_M11_46.md`.

**Exact next step:** Complete the required repository validation, commit and
push this focused M11.46 result, then stop.

## 2026-09-09 — M11.45 Bounded Semantic Closure Toward 95% Dynamic Coverage — COMPLETE
**Objective:** Resume from the committed M11.44 baseline and close only a bounded,
decoder-backed semantic tranche toward 95% translated dynamic execution. Do not
force coverage or broaden hardware, indirect-control-flow, decoder, or unresolved
runtime-address scope.

**Acceptance criteria:** Reproduce the M11.44 checkpoint, video and execution
metrics twice; rank the complete semantic remainder and select an eligible tranche
capable of at least 236,530 executions; independently verify only exact required
68000 forms; mechanically generate and provenance-check those forms; pass
per-candidate GPGX shadow and unchanged 600-frame native gates; close the final
ledger and Pareto categories; update governance, hygiene and validation evidence;
commit and push the meaningful positive or negative result.

**Actions:** Re-read repository instructions and all required governance docs;
reproduced the unchanged Release baseline twice. Ranked the complete final
M11.44 ledger, excluding hardware/indirect/decoder/unknown runtime-address
classes. Independently verified the selected exact helper forms with deterministic
68000 vectors and emitted 551 decoder-owned candidates mechanically. Three forms
were vetoed independently by the existing shadow contract: LSR.W at 0x0038E0
(+14 cycles), ROR.W at 0x06115A (+112 cycles), and CMPI.B at 0x0038AA (X flag).

**Evidence:** The final native run preserved checkpoint
`251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d`, video
`5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`,
6,488,773 total, 6,199,718 translated, 289,055 interpreter executions,
580 registered ranges, 149,059 yields, 288 interrupted resumptions and zero
original starts inside translated ranges.

**Validation:** The final shadow completed 6,199,718/6,199,718 comparisons with
zero divergence. The unchanged 600-frame native run completed with zero
fallbacks, zero hardware-visible accesses, zero starts inside translated ranges,
checkpoint/video/CPU/RAM/VDP/sound identity, 149,059 boundary yields and 288
interrupted resumptions. The final ledger closes at 289,055 exactly. Full MSVC
Debug CTest passed 57/57, full MSVC Release CTest passed 57/57 and the GNU/
MinGW-equivalent full CTest passed 57/57, including semantic, generator,
provenance, checkpoint, boundary and source-limit tests. `git diff --check` and
repository hygiene passed; the final generated output is split into <=500-line
translation units.

**Result:** `REMAINING_ATTRIBUTION_AND_DYNAMIC_COVERAGE_95_PROVEN`. Final native
metrics are 6,199,718 translated and 289,055 interpreter executions, 580
registered ranges and 95.5453% translated share. No hardware contract or
production runtime was broadened.

**Exact next step:** commit and push this focused M11.45 result, then stop.

## 2026-09-09 — M11.44 Restart and Remaining Interpreter Attribution — COMPLETE
**Objective:** Reproduce the M11.43 authoritative baseline twice, then account
for all `661,916` remaining interpreter executions with decoder-backed evidence.
Coverage promotion remains conditional on exact semantic, shadow, boundary and
native gates; no coverage is to be forced.

**Acceptance criteria:** exact restart metrics; complete PC/count ledger whose
counts sum to `661,916`; preserved `0x03A7AE` historical rejection re-tested
against the current bridge; independent verification and mechanical generation
only for proven candidates; governance, hygiene, full Debug/Release/GNU checks,
commit, push and CI result.

**Actions:** Re-read repository instructions and all current governance docs;
reproduced the M11.43 authoritative restart baseline twice. Added the bounded
developer-only `interpreter_ledger`, reusing the existing profile and exact
slice decoder, and generated complete pre-/post-promotion per-PC ledgers.
Re-tested 0x03A7AE/0x03A7B4 under the current bridge and boundary contract;
generated the exact decoder-owned block mechanically and registered it as
`generated_blocks_m1144.cpp`. No handwritten candidate execution body,
address-specific semantic patch, candidate timing constant or hardware-model
change was added.

**Evidence:** Restart runs matched aggregate
`251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d`, video
`5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`, total
6,488,773, translated 5,826,857, interpreter 661,916, 28 ranges, 140,065
yields, 274 resumptions and zero starts inside translated ranges. The historical
0x03A7AE mismatch (`actual IR/prefetch 0x4E73`, expected `0x4A79`) is obsolete:
the current generated block passed 100,948 shadow comparisons with zero
divergence. Native promotion preserved exact checkpoint/video identity, CPU /
RAM / VDP / sound and boundary equivalence; final metrics are 29 ranges,
5,927,805 translated, 560,968 interpreter, 142,813 yields and 288 resumptions.
The 0x060BA4 `0xA00003` access remains hardware-visible fallback.

**Validation:** The baseline and final profiles close exactly at 661,916 and
560,968 respectively. Full MSVC Debug CTest passed 56/56, MSVC Release CTest
passed 56/56, and GNU-equivalent MinGW CTest passed 56/56. The focused semantic,
generator/provenance, checkpoint and boundary suites are included in those
runs; diff hygiene, source line-limit and repository hygiene passed before push.

**Result:** `REMAINING_INTERPRETER_ATTRIBUTION_PROVEN_SEMANTICS_BLOCKED`.
Final translated share is 91.3548%; the 95% gate was not forced. M11.39,
M11.40, M11.41, M11.42 and M11.43 history remains preserved.

**Exact next step:** stop; do not implement the next milestone in this task.

## 2026-09-09 — M11.43 GPGX Checkpoint Canonicalization Contract — COMPLETE
**Objective:** Resolve the M11.42 checkpoint identity blocker by proving the
pinned GPGX v1.7.6 raw-state layout and canonicalizing only host representation.
Do not resume interpreter attribution or coverage expansion.

**Actions:** Re-read repository instructions and governance; audited pinned
GPGX source, ABI declarations, active MD serializer and the pinned DLL's raw
state. Added the developer-only `gpgx_checkpoint_layout` contract with exact
x64 `sizeof`/`offsetof` assertions and one generated non-overlapping span table.
Updated `checkpoint_evidence` to guard version/size, preserve raw buffers and
clear only representation spans. Added adversarial layout/canonicalization and
raw replay tests.

**Evidence:** Proven layout: `FM_SLOT=80`, `FM_CH=400`, `YM2612=3576`,
`Z80_Regs=88`; YM serialized base `140652`; pinned-DLL Z80 base `144504`, with
`daisy` at `144576` and callback at `144584`. The source/object timestamp drift
explains why source-only arithmetic predicts an earlier Z80 boundary; no
conflicting source-only offset was used for the pinned binary. The table has
55 host-pointer spans, one function-pointer span, 111 ABI-padding spans and
281 padding bytes. Three raw current runs were identical; current versus
preserved M11.41 raw evidence varied only 110 bytes per record across 10
records, all proven YM pointers or Z80 `daisy`.

**Validation:** Five independent current 600-frame `BASIC_BLOCK_NATIVE` proofs
agree on aggregate `251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d`,
video `5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`,
6,488,773 total, 5,826,857 translated, 661,916 interpreter, 140,065 yields,
274 resumptions, 28 ranges and zero starts inside translated ranges. Canonical
raw replay from current, exact M11.39 and M11.41 evidence agrees on the new
aggregate. The old `c9236218...` identity is superseded because M11.41 also
cleared semantic bytes. No ROM/assets/binaries/run evidence was tracked;
`game.srm` stayed untouched and untracked.

After the final Release assertion fix, full CTest passed 56/56 in MSVC Debug,
56/56 in MSVC Release and 56/56 in the GNU-equivalent MinGW configuration.
`git diff --check` and the project source-file line-limit check passed.

**Result:** `CHECKPOINT_CANONICALIZATION_COMPLETED_NEW_AUTHORITATIVE_IDENTITY`.
M11.39, M11.40, M11.41 and M11.42 historical records remain preserved; M11.42
PHASE 2 was not started.

**Exact next step:** Resume M11.42 PHASE 1 in a separate bounded task.

## 2026-09-09 — M11.42 Restart Gate — BLOCKED
**Objective:** Restart the remaining interpreter attribution and 95% coverage
gate from M11.41's committed baseline without weakening checkpoint identity.

**Actions:** Re-read repository instructions and current project governance;
rebuilt the developer-only hybrid runner from `5c19e22`; ran the unchanged
600-frame `BASIC_BLOCK_NATIVE` scenario twice with the canonical USA ROM,
external GPGX DLL and 28-range registry. One run retained opt-in raw checkpoint
evidence for a bounded byte comparison.

**Evidence:** ROM/DLL identity, video hash, 6,488,773 total, 5,826,857
translated, 661,916 interpreter, 140,065 yields, 274 resumptions, 28 ranges
and zero starts inside translated ranges all matched. The required repaired
aggregate `c9236218...` did not; both fresh runs produced `d5de401c...`.
The first raw difference remains offset `140654`; after the committed M11.41
scrub spans, offset `140734` still differs as the next `FM_SLOT.DT` host-pointer
byte.

**Result:** `M11.42_BASELINE_BLOCKED_CHECKPOINT_CANONICALIZATION_INCOMPLETE`.
PHASE 2 through PHASE 11 were not performed. M11.39, M11.40 and M11.41 history
remains preserved. No source, generated block, semantic candidate, hardware
contract or production runtime was changed.

**Validation:** The baseline hybrid target build and both 600-frame runs passed.
After the documentation update, final Debug, Release and GNU-equivalent full
CTest each passed `55/55`; the source-limit check, `git diff --check` and
tracked-artifact hygiene check also passed.

**Exact next step:** repair and independently re-prove the developer-only
checkpoint identity in a separate bounded task; do not classify remainder PCs or
claim coverage from this blocked baseline.

## 2026-09-09 — M11.41 Checkpoint Identity Provenance and Reproduction Repair — COMPLETE
**Objective:** Determine why the M11.39 checkpoint aggregate was not
reproducible, prove the first differing serialized field, and restore one
authoritative deterministic identity without starting M11.40 PHASE 2.

**Actions:** Re-read repository instructions and all required project documents;
reconstructed the runner pipeline from `retro_run`, `retro_serialize_size`,
`retro_serialize`, per-record hashing and ordered aggregate hashing. Built the
exact M11.39 commit twice before instrumentation; it produced stable aggregate
`3923a3d6...`, not the recorded `20217e...`. Added ignored opt-in raw evidence
with per-record offsets/hashes and manifest order. The first independent raw
divergence was frame 60, state offset `140654`, byte 3 of the first YM2612
`FM_SLOT.DT` host pointer. Added the minimum format-guarded canonical identity
adapter that removes only proven pointer/padding representation bytes while
retaining raw buffers and semantic bytes.

**Result:** `CHECKPOINT_IDENTITY_SERIALIZATION_BUG_PROVEN` and
`CHECKPOINT_BASELINE_IDENTITY_RESTORED`. The authoritative aggregate is
`c9236218f55fb18f7f1d5095e4970b25f228de588bd03bd7cbbffccc2e225fd1`.

**Proof:** Three repaired current runs and two repaired exact historical-checkout
runs matched the checkpoint sequence, aggregate, video hash, 6,488,773 total,
5,826,857 translated, 661,916 interpreter, 140,065 yields and 274 resumptions.
The M11.40 negative report, M11.39 report/hash, ROM/DLL identities and all local
raw evidence remain preserved; no run evidence is tracked and `game.srm` was
not edited.

**Exact next step:** start M11.40 PHASE 2 in a separate task. Do not expand
M11.41 scope.

## 2026-09-09 — M11.40 Remaining Interpreter Attribution and 95% Coverage Gate — BLOCKED
**Objective:** Reproduce the exact M11.39 baseline before classifying the
remaining interpreter executions or attempting any semantic, generator,
shadow or native promotion.

**Acceptance:** use the M11.39 commit, canonical USA ROM, external GPGX
identity, 28-range registry and unchanged 600-frame neutral scenario; stop if
the baseline differs; do not force 95% coverage or implement M11.41.

**Actions:** Rebuilt the existing developer-only `oasis_hybrid_poc` target and
ran `BASIC_BLOCK_NATIVE` twice with the same external GPGX DLL and canonical
ROM. No production source, registry, generated block or emulator code was
changed.

**Evidence:** Both runs reproduced `6,488,773` total guest instructions,
`5,826,857` translated, `661,916` interpreter executions, 28 ranges, 89.7991%
translated share, the M11.39 video hash, 140,065 boundary yields and 274
interrupted resumptions. Both current checkpoint aggregates are
`fffe59fcdbed7fdac8ef22badb4f7236b8619459fed27c9931e0f93906549052`, differing
from historical M11.39 `20217e10565c51b571db6a4e474aa40854ea6c22277ba14c46ea97c4b1c60a04`.
The discrepancy's cause is not proven.

**Tests/build:** Baseline hybrid target build passed; the two 600-frame runs
completed without divergence. Full Debug, Release and GNU-equivalent CTest
passed `54/54` in each configuration, including source-limit and all existing
semantic/generator/provenance/boundary tests. GPGX shadow/native promotion
gates were not run after the failed baseline identity gate, so no coverage
result is eligible for promotion.

**Result:** `M11.40_BASELINE_BLOCKED_CHECKPOINT_IDENTITY_MISMATCH`. The complete
M11.40 remainder ledger and 95% gate are not claimed. M11.39 remains the
latest valid coverage result.

**Unresolved:** determine why the historical checkpoint aggregate differs from
the stable current aggregate despite matching ROM/DLL identity and all other
recorded metrics.

**Exact next step:** resolve checkpoint identity, then restart M11.40 PHASE 1;
do not begin PHASE 2 or implement M11.41.

## 2026-09-09 — M11.39 Hot-Path Multi-Block Coverage Expansion — COMPLETE
**Objective:** Starting from committed M11.38
`daa0a09b5cd8845a48733e771774491b35e73d9e`, profile only interpreter
executions left by the proven 26-range registry and promote a bounded set of
the highest dynamic-payoff exact ranges through the existing semantic,
generator, provenance, shadow and native gates.

**Acceptance:** use the unchanged canonical USA ROM, external GPGX identity
and cold-reset neutral 600-frame scenario; select no more than 40 ranges; do
not invent indirect CFG targets or broaden hardware modeling; compare every
selected instruction boundary; update all required governance documents; keep
generated code separate from handwritten glue; preserve M11.32–M11.38 and keep
ROM/assets/binaries/run evidence/game.srm out of Git.

**Profile and implementation:** The M11.38 native registry run recorded 2,152
remaining interpreter PCs and 2,366,711 interpreter executions. The dominant
eligible path was `[0x000380,0x0003A0)`: sixteen exact `ADD.W (A0)+,D0`
instructions, 1,572,608 dynamic instructions and 122,886 entries. The second
selected path was `[0x03A864,0x03A868)`, one exact direct `BNE.W`, with 132,187
dynamic instructions. The independent semantic harness added only the exact
word post-increment ADD form; four edge vectors passed. The generator emitted
both bodies and the generated 28-entry registry. A profile writer emits local
ranked JSON evidence; it is not a runtime policy or auto-promoter.

**Evidence:** GPGX shadow completed `5,826,857/5,826,857` per-instruction
comparisons with zero divergence. Native completed 600/600 frames and
translated `5,826,857` of `6,488,773` guest instructions (`89.7991%`), leaving
`661,916` interpreter executions. There were zero original starts inside
translated ranges, zero hardware-visible accesses, 140,065 event-boundary
yields and 274 interrupted continuations. Checkpoint hash is
`20217e10565c51b571db6a4e474aa40854ea6c22277ba14c46ea97c4b1c60a04`; video
hash is `5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`.

**Validation:** Debug semantic, generator, provenance and boundary tests
passed, followed by final GPGX shadow and native runs. The final Debug,
Release and GNU-equivalent full CTest runs, source-limit, `git diff --check`
and repository hygiene checks are recorded before push. No ROM, asset,
emulator binary, generated run evidence or `game.srm` is tracked.

**Result:** `HOT_PATH_DYNAMIC_COVERAGE_80_PROVEN`.
**Unresolved:** The remaining 661,916 executions are retained by the
conservative Pareto ledger; no M11.40 work or indirect-target set is claimed.
**Next action:** stop; require M11.40 as a separate bounded milestone.

## 2026-09-08 — M11.38 Interrupt-Safe Multi-Instruction Block Execution — COMPLETE
**Objective:** Starting from committed M11.37
`5d353beaddffbca73c7388903cc22a790c634330`, prove generic conservative
instruction-boundary yielding and exact continuation for only the four M11.37
multi-instruction candidates rejected for interrupt interleaving.

**Acceptance:** preserve the four original rejection records; use authoritative
GPGX interrupt/trace/timing/scheduler ownership; generate `BlockExit` bodies
mechanically; compare every instruction boundary; reproduce a natural
interrupted continuation; promote only after shadow; do not add discovery,
atomic blocks, a second scheduler, runtime JIT or production dependencies.

**Frozen evidence:** The ranges were `[0x0032EE,0x0032F6)` with natural count
1,121,997, `[0x03A9AC,0x03A9B4)` with 248,291, `[0x03A9B4,0x03A9BC)` with
248,290 and `[0x03A9CA,0x03A9D4)` with 248,290. Their exact forms are,
respectively, TST.W/Bcc.S, TST.W/Bcc.S, TST.B/Bcc.S and TST.W/Bcc.W. Each
range has one internal boundary, resolved conservatively after instruction 1
and before instruction 2. TST absolute-long and Bcc short/word were already
independently verified. The M11.37 range-level rejection remains unchanged.

**Implementation:** Added generic `BlockExit {next_pc, reason,
instructions_executed}`, instruction-granular generated continuation bodies,
per-boundary state prediction/comparison, GPGX boundary-reason bridge and
interrupt-resumption metrics. Generated output is split into historical,
M11.37 and M11.38 translation units plus a generated registry; handwritten
registry/reference glue is separate. No candidate-specific timing or interrupt
branch was added.

**Evidence:** Current external GPGX source is
`d60d079934977aa6973e220d123533387159f66e`, DLL SHA-256
`140c00fc7475cf22ca22415d65dfc7ffc66523de128454f9994e7ab77d9826fd`, and
canonical ROM SHA-256 is
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
The 600-frame shadow run completed 4,122,062/4,122,062 comparisons with zero
divergence. Native execution translated 4,122,062/6,488,773 instructions
(63.5261%), yielded 111,009 event boundaries, observed 486 interrupt services
and resumed 274 translated continuations after actual interrupt service. The
current EMULATED/native checkpoint hash is
`20217e10565c51b571db6a4e474aa40854ea6c22277ba14c46ea97c4b1c60a04`; video
hash remains `5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`.
All four per-candidate boundary counts and resumptions are in the dedicated
M11.38 report.

**Validation:** Generated-block yield/resume, semantic, generator,
provenance and basic-block tests pass in Debug. Final Debug, Release and
GNU-equivalent full CTest each passed 54/54; source-limit, `git diff --check`
and tracked-artifact hygiene checks passed. Final GPGX shadow/native runs
passed after the last source change.

**Result:** `INTERRUPT_SAFE_MULTI_INSTRUCTION_BLOCKS_PROVEN`.
**Next action:** stop; require a new bounded milestone before further coverage.

## 2026-09-08 — M11.37 Controlled Dynamic Coverage Expansion — COMPLETE
**Objective:** Starting from committed M11.36
`d2cf0b942eaff6ba61cdfe34b440fc595a5ebc86`, prove whether the existing
developer-only trace → exact decode → independent semantics → mechanical
generation → GPGX shadow → bounded promotion pipeline can expand translated
execution beyond the six-entry registry.

**Acceptance criteria:** Freeze exact before/after metrics using the canonical
USA ROM, external GPGX identity and unchanged cold-reset neutral 600-frame
scenario. Build a bounded natural-execution candidate queue of approximately
10–25 new blocks; record every considered candidate's entry/count/instruction
forms/control-flow/semantic status/rejection reason; require independent
verification, generator/provenance and zero-tolerance GPGX shadow gates before
promotion. Success requires at least 10 new promoted blocks, unchanged CPU,
RAM, VDP, sound, interrupt, checkpoint and video equivalence, and a measurable
increase in translated guest-instruction share. Runtime JIT, automatic trust,
whole-ROM recompilation and ROM-byte coverage are excluded.

**Discovery/baseline:** The unchanged canonical 600-frame run recorded
6,488,773 interpreter instruction executions, 2,188 unique interpreter PCs,
600 video frames, checkpoint hash
`b8e1e07908d75e9c8b21f3ed661352a7005c51dcf3120e2502cc0f73788a4bd4` and video
hash `5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`.
The queue considered 61 generator-eligible natural entries; the complete
entry/form/status ledger is in `reports/CONTROLLED_DYNAMIC_COVERAGE_M11_37.md`.

**Implementation:** Reused the exact decoder and generator, adding only
generated `GeneratedBlockSpec` metadata and a generic developer-only registry
prediction adapter. No new semantic form was accepted: promoted Bcc/DBcc/TST
forms were independently verified by the existing M11.34/M11.35 harness.
Generated bodies remain in `generated_blocks.cpp`; handwritten glue is in the
registry/reference files. Four multi-instruction candidates failed closed on
observed interrupt interleaving and `0x060BA4` failed closed on hardware access.

**Gate:** Full GPGX shadow completed `388308/388308` comparisons with zero
divergences and full CPU equivalence. Native promotion completed 600/600
frames, translated `388314` of `6488773` guest instructions (`5.9844%`), left
`6100459` interpreter instructions and 12 fallback entries, with zero original
starts inside translated blocks and zero hardware-visible accesses. Checkpoint
and video hashes exactly matched the EMULATED baseline.

**Validation:** Targeted Debug GPGX-enabled build, generator regeneration,
semantic/generator/provenance/basic-block tests, final GPGX shadow and native
600-frame runs passed. Full Debug CTest passed 54/54, Release CTest passed
54/54, and GNU-equivalent CTest passed 54/54; source-limit, `git diff --check`
and tracked-artifact hygiene checks also passed. No ROM, asset, emulator
binary or generated run evidence is tracked; `game.srm` remains untouched and
untracked.

**Result:** `CONTROLLED_DYNAMIC_COVERAGE_EXPANSION_PROVEN`.
**Next action:** stop; require a new bounded milestone before further coverage.

## 2026-09-08 — M11.36 GPGX Timing / Refresh Bridge Contract — COMPLETE
**Objective:** Starting from M11.35 `291012425bdc85f37cb5c11ffede71223916e3a4`,
resolve only the cycle/refresh/interrupt-boundary bridge for the existing
`0x3A85E`, `0x3A8BA` and `0x3A88C` candidates, then rerun the existing gate.

**Evidence:** GPGX source inspection established that `m68k.cycles` and
`refresh_cycles` are accumulated signed master-cycle counters owned by the
68K core, relative to the current frame and rebased by subtracting
`mcycles_vdp` in `system.c`. `m68k_run` polls interrupts at entry, invokes the
block hook before normal fetch, and charges refresh/instruction timing in the
normal interpreter. The M11.35 mismatch was a pre-rebase prediction versus a
post-rebase next-entry observer: `896114/896268` became `74/228`, with
`mcycles_vdp=896040`. Decode, TST semantics, RAM access and refresh primitive
were not the blocker.

**Implementation:** Added a generic developer-only GPGX post-instruction hook
before the next scheduler/frame transition and taught the block registry to
close shadow comparison at that authoritative boundary. The GPGX bridge ABI
is now `2`; generated instruction semantics and the existing M11.33 bodies
were unchanged. Added `hybrid_basic_block_test.cpp`, which fails under the old
entry-only observer and passes only when the post-instruction boundary closes
the shadow. Generated code remains in `generated_blocks.cpp`; handwritten
bridge/registry glue remains separate.

**Gate:** Final external GPGX source commit was
`d60d079934977aa6973e220d123533387159f66e`; final DLL SHA-256 was
`9b345293c239805cbfe22bb3c582e7d42164a701ba2c50e1934e1a8ef80b2ec8`.
The six-entry shadow run completed `185975/185975` comparisons with zero
divergences. Native then completed the unchanged 600-frame scenario with
`185975` native entries, `185981` translated guest instruction executions,
zero original-body starts, zero fallback entries, zero interrupts and zero
hardware accesses. All three M11.35 candidates were naturally promoted; the
three M11.33 blocks remained exact.

**Equivalence:** Shadow and native both report full CPU equivalence and equal
checkpoint hash `66e2a51afdd0ee4e790063b0e603dff77ae678b43e6c411c9da57ed2f27a3cba`
and video hash `5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`.
The old negative M11.35 result remains unchanged in its historical entry.

**Validation:** Final Debug, Release and GNU-equivalent full CTest were run;
the targeted semantic, generator/provenance and new bridge regression tests
passed. Final GPGX shadow/native runs passed. `git diff --check`, source file
limits and repository hygiene checks are recorded before commit. The
pre-existing tracked generated `gpgx-coverage-report.txt` was removed; no ROM,
asset, emulator binary or generated run evidence is tracked; `game.srm` remains
untouched and untracked.

**Result:** `GPGX_TIMING_REFRESH_BRIDGE_PROVEN` and
`DEMAND_DRIVEN_BLOCK_PROMOTION_PROVEN`.

**Next action:** stop; require a new bounded milestone for any additional
block discovery or coverage.

## 2026-09-08 — M11.35 Demand-Driven Block Promotion Pilot — COMPLETE / RUNTIME BLOCKED
**Objective:** Starting from committed M11.34 `32708bdcec086c6e954acdbb53715e4e3293fd2e`,
run one bounded natural guest trace, discover a small candidate set outside the
M11.33 blocks, decode and classify exact forms, independently verify only newly
required semantics, generate provenance-bound bodies, and require exact GPGX
shadow equivalence before any native promotion.

**Acceptance criteria:** preserve the M11.33 three-block proof; record only
3–8 natural candidates and rejects; refuse unverified exact IR; retain
independent semantic vectors; generate direct-successor bodies with guest
provenance; compare full CPU/RAM/PC/SR/IR/prefetch/cycle/refresh/hardware state
through the GPGX bridge; promote zero candidates unless every shadow gate passes;
leave failed candidates on interpreter fallback; keep production targets
emulator-free and keep ROM/assets/generated evidence out of Git.

**Discovery:** The existing cold-reset neutral 600-frame scenario was run in
`DISCOVER_BLOCKS` mode against external instrumented GPGX source commit
`d60d079934977aa6973e220d123533387159f66e` and DLL SHA-256
`ba6c10fdb1fe421e1e38c88b70ce18b0d2a122f4472f477b8a1a7aba14fce274`.
The trace recorded 2,188 unique guest PCs. The bounded final shortlist was
`0x3A85E`, `0x3A8BA` and `0x3A88C`; all are direct TST absolute-long entries
with exits `0x3A864`, `0x3A8C0` and `0x3A892`. DBF and Bcc alternatives were
examined but rejected at the runtime/timing/interrupt boundary; broader
indirect-control-flow or unsupported candidates were not expanded.

**Semantic/generation gate:** The test-only independent model added deterministic
Bcc, DBcc and TST edge vectors, including flag, loop-counter and timing edges.
The TST vectors passed. The exact decoder classified the selected forms as
`NEW_VERIFICATION_REQUIRED`; the generator emitted only supported TST helper
calls and rejected unsupported forms. Generated bodies contain only
decoder-derived guest PC/opcode/assembly comments, fetch/finish boundaries and
direct successors. M11.33 generated bodies and handwritten hybrid glue remain
separate.

**Shadow evidence:** The final bounded shadow run rebuilt the current code and
stopped on the first candidate at `0x3A85E`:
`FIRST_DIVERGENCE block=0x3a85e timing actual_cycles=74 expected_cycles=896114
actual_refresh=228 expected_refresh=896268`. The required exact cycle/refresh
and interrupt-boundary contract was therefore not established. Later shortlist
entries were not promoted or treated as independently proven.
Native promotion was not run after the failed shadow gate. The M11.35 entries
remain an offline shadow experiment with interpreter fallback; no production
runtime or automatic trust path was added.

**M11.34 preservation:** The previously proven M11.33 three-block result was
left in place and its 600-frame regression remained the baseline. No ID3,
`0x3820`, whole-game coverage, timing optimization, emulator dependency or
toolchain archaeology was added.

**Tests/build:** Current GPGX hybrid targets rebuilt successfully and the
independent semantic executable passed. Full Debug, Release, GNU-equivalent and
GPGX build+CTest each passed 53/53, including the source-file line-limit test.
The generator reproduced `src/tools/hybrid/generated_blocks.cpp` byte-for-byte
for all six generated blocks; `git diff --check` passed. The expected negative
GPGX shadow result is the milestone gate blocker, not a passing equivalence
claim. `game.srm` remains untouched and untracked; no ROM/assets/emulator or
generated run evidence is staged.

**Result:** `DEMAND_DRIVEN_PROMOTION_RUNTIME_BLOCKED`.

**First blocker:** the developer-only GPGX cycle/refresh and interrupt-boundary
contract prevents exact timing agreement for the first selected TST candidate.

**Exact next step:** stop. Resolve that bridge contract in a new bounded task
before attempting any M11.35 native promotion or scalable promotion loop.

## 2026-09-08 — M11.34 Independent M68K Semantic Core Verification — COMPLETE
**Objective:** Independently verify the exact semantic subset emitted by the
M11.33 mechanical generator without using the decoder/emitter as its oracle.

**Acceptance criteria:** Freeze the seven used instruction/addressing-mode/size
combinations; compare deterministic independent-reference vectors for exact
register, PC, SR/CCR, memory and ordered-access results; verify decode length
and provenance; preserve the M11.33 600-frame proof; classify every used form.

**Independent reference:** Motorola/NXP 68000 Family Programmer's Reference
Manual, `https://www.nxp.com/docs/en/reference-manual/M68000PRM.pdf`, used as
the semantic specification for the test-only reference model. No external
implementation is linked into production targets.

**Non-goals:** whole-ROM coverage, M12, ID3, `0x3820`, timing optimization,
production emulator work, speculative/AI-generated semantics, or decoder rewrite.

**Plan:** Add only a test-only reference/vector harness, exact decode and
provenance assertions, then rerun the bounded M11.33 external GPGX proof and
the required local build/test gates.

**Implementation:** Added `oasis_hybrid_semantic_core`, a test-only reference
model independent from the decoder/emitter and transcribed from the Motorola/NXP
manual. The frozen surface remained exactly seven used combinations; no
supported-but-unused emitter combination was introduced. The harness emits
first-mismatch diagnostics containing opcode, decoded form, complete pre-state,
expected state and actual state.

**Evidence:** 23 deterministic vectors passed: three MOVEM mask/order cases,
three CLR flag/upper-half cases, three MOVE.B flag/address-wrap cases, two LEA
address cases, four ADDA sign-extension/wrap cases, three overlapping MOVE.W
ordering cases and five ADD.L carry/overflow/X cases. Exact decode checks passed
for all three generated blocks, including opcode, operand kind/value/register,
size, extension-word count and next-PC continuity. Generated provenance comments
were present for every emitted instruction.

**M11.33 regression:** external GPGX checkout `d60d079934977aa6973e220d123533387159f66e`
and DLL SHA-256 `ba6c10fdb1fe421e1e38c88b70ce18b0d2a122f4472f477b8a1a7aba14fce274`
repeated the 600-frame cold-reset neutral proof. Shadow was 14/14 with zero
divergences; native was 14/14 overrides with zero divergences, skipped all
original target bodies, and retained `full_cpu_equivalence=true`, matching
checkpoint hash `fffe59fcdbed7fdac8ef22badb4f7236b8619459fed27c9931e0f93906549052`
and video hash `5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`.

**Tests/build:** Debug, Release and fresh GNU-equivalent builds and full CTest
passed after the harness was added; targeted semantic tests passed in Debug,
Release and GNU-equivalent configurations. `git diff --check` and source
file-limit checks pass. `game.srm` remains untracked and untouched.

**Result:** `M68K_SEMANTIC_CORE_INDEPENDENTLY_VERIFIED`.

**Unresolved:** no disagreement remains in the seven used combinations.
All other exact-IR forms, including branches, DBcc, status-register operations,
other addressing modes and wider instruction families, remain unverified and
must not be emitted without a new bounded milestone.

**Exact next step:** stop; do not expand the semantic surface in this checkpoint.

## 2026-09-08 — M11.33 Recomp Generator v1 — COMPLETE
**Objective:** Replace the three handwritten M11.32 hybrid instruction bodies
with deterministic C++ generated from the shared decoder/exact IR, while
retaining the existing GPGX timing/hardware contract.

**Acceptance criteria:** generator output retains guest PC/opcode/decoded
assembly provenance; the three blocks `0x2D66`, `0x604BC` and `0x61032` are
generated and used by the hybrid registry; unsupported forms fail closed; the
M11.32 behavioral proof remains unchanged; production targets remain untouched.

**Non-goals:** common semantic-core verification, automatic discovery,
indirect dispatch discovery, new gameplay scenarios, `0x3820` repair and
production emulator work.

**Implementation:** Added `oasis_hybrid_recomp_generate`, which consumes the
shared decoder/exact IR and emits provenance-bound C++ for the exact M11.32
blocks. Added a small generated-helper boundary for MOVEM, MOVE, LEA, ADDA,
ADD and CLR forms, including checked opcode/extension fetches and the proven
post-instruction MOVEM timing/refresh ordering. The hybrid registry now calls
the generated bodies; production targets remain unchanged.

**Evidence:** The canonical USA ROM generated 7 instructions at `0x2D66`, 1 at
`0x604BC` and 1 at `0x61032`. A second generator run matched the checked-in
artifact SHA-256 `06F1C15137D8A239862C083EF6E358FEC8C42798BB54DA4A150B89A4EF5D46E9`.
The external GPGX bridge (instrumented source `d60d079`, DLL SHA-256
`ba6c10fdb1fe421e1e38c88b70ce18b0d2a122f4472f477b8a1a7aba14fce274`) completed
the 600-frame cold-reset neutral scenario in shadow and native modes: 14/14
calls, zero divergences, `full_cpu_equivalence=true`, matching state/video
hashes, and 20 native guest instruction executions.

**Tests/build:** Full Debug CTest was 51/51, full Release CTest was 51/51,
and full fresh GNU-equivalent Release CTest was 51/51. After the final fetch
and MOVEM-order corrections, changed-path hybrid tests were 6/6 in Debug,
Release and GNU-equivalent builds. `git diff --check` and the source file-limit
check pass. The stale pre-existing `game.srm` remains untracked and untouched.

**Result:** `MECHANICAL_BLOCK_GENERATION_PROVEN`.

**Unresolved:** Generated helper semantics are intentionally still scoped to
the three M11.32 forms. General CCR/SR semantics, wider instruction-family
coverage and automatic block discovery remain M11.34+ work.

**Exact next step:** M11.34 — independently verify the common M68K semantic
core; do not expand discovery or production dependencies here.

## 2026-09-08 — M11.32 basic-block recompilation timing proof — COMPLETE
**Objective:** prove repeatable developer-only basic-block replacement for the
three M11.30 targets while keeping GPGX responsible for state, memory, bus,
prefetch, timing and scheduler behavior.

**Implementation:** added the smallest block hook boundary before GPGX opcode
dispatch, a target-specific registry, and exact mechanical blocks for
`0x2D66`, `0x604BC` and `0x61032`. The MOVEM block delegates immediate fetch,
memory access, cycle-table accounting and refresh skipping to GPGX helpers;
no generic M68K interpreter or production dependency was added.

**Evidence:** canonical ROM SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`; GPGX
source `d60d079934977aa6973e220d123533387159f66e`; DLL SHA-256
`ba6c10fdb1fe421e1e38c88b70ce18b0d2a122f4472f477b8a1a7aba14fce274`.
Shadow was 14/14 with zero divergences. Native override was 14/14, with 20
translated guest instructions, zero original-body starts, zero interrupts or
hardware accesses, complete 600-frame scenario, and exact state/video hashes
matching EMULATED. The full evidence is in
`docs/reports/BASIC_BLOCK_RECOMPILATION_TIMING_M11_32.md`.

**Checks:** current Debug and Release builds and CTest 49/49 pass; changed
source files remain below 500 lines; `git diff --check` passes. The existing
`build-gnu` tree is stale and points to an old `D:/Proect` checkout, so it was
not used; a fresh MinGW GNU-equivalent configuration (`build-m1132-gnu`) built
and passed CTest 49/49. `game.srm` remains untracked.

**Decision:** `BASIC_BLOCK_RECOMP_TIMING_PROVEN`. Stop this migration batch;
do not expand coverage, investigate ID3, repair `0x3820`, add AI generation,
or move hybrid code into production.

## 2026-09-08 — M11.31 comparative disassembly method transfer — COMPLETE
**Objective:** Compare public Streets of Rage 2/3 disassembly and recompiler
projects with the existing Beyond Oasis evidence, then dry-run the strongest
transferable method on one already-proven bounded subsystem. No ROM, asset,
external source, emulator binary or generated run evidence may enter Git.

**Sources:** `gsaurus/sor-disassemblies` at `4dd719f`, `gsaurus/sor_pancakes`
at `96cef9b`, the `StreetsOfRageProject` meta repository at `4987085`,
`RageDecompiler` at `9d58a51`, and `StreetsOfRageRecompilation` at `dc578f5`.
Each exact revision and URL is recorded in the report. Exact ROM reassembly is
not demonstrated by any inspected project.

**Result:** the static IDA repository is `SPECIALIZED_ONLY`; Pancakes is
`SPECIALIZED_ONLY`; the RuiNelson process is `PARTIAL_TRANSFERABILITY` because
its byte map, explicit unknown/auxiliary-entry state, bounded direct closure,
natural evidence loop and generated/native separation transfer. Programmer
style is `NON_DISCRIMINATING`, binary structure is `PARTIAL`, and sound-driver
lineage is `SPECIALIZED_ONLY`/unresolved.

**Dry-run:** applied the sequence to existing `0x2D66` evidence: canonical ROM
seed, exact decoder/RTS range, raw bounded state contract, natural shadow, and
promotion gate. It reproduces the M11.29 proof without importing SoR labels,
RAM offsets, object formats, audio assumptions or a recompiler runtime. No new
structural claim or code change was justified.

**Checks:** documentation-only change; `git diff --check`, source file-limit
check, focused hybrid Debug/Release CTests and the existing CI-equivalent
checks are run before commit. `game.srm` remains untracked.

**Decision:** retain localized Beyond Oasis methods. A possible M11.32 is one
bounded evidence-led pass over vectors/interrupts, raw RAM symbols, one
state/object/resource path, exact decoding and natural evidence. Do not expand
coverage, investigate ID3, repair `0x3820` timing, or add production emulator
dependencies.

## 2026-09-07 — M11.30 hybrid migration small batch — SHADOW PROVEN / OVERRIDE BLOCKED
**Objective:** Reuse the M11.29 hybrid boundary for a bounded batch of naturally
executed routines without broad coverage, manual gameplay, ID3 work or changes
to production runtime dependencies.

**Selection:** `0x604BC` and `0x61032` were selected from existing deterministic
execution evidence. Both are fully decoded direct-RTS leaf routines with bounded
RAM/register effects and no observed interrupt or hardware I/O. `0x6121A`,
`0x611F4`, `0x611EA` and `0x60004` were rejected for VDP, YM/Z80, nested-call
or non-RTS control-flow contracts.

**Evidence:** The shared registry observed 14 natural calls in 600 frames
(`0x2D66`: 1, `0x604BC`: 4, `0x61032`: 9). Shadow comparisons were 14/14 with
zero divergences and zero interrupts. Native override calls were 14 with zero
original-body instruction starts, zero fallback calls, complete scenario and
matching video sequence. Serialized state first diverged at frame 120 in four
VDP/sound-state bytes; later checkpoints diverged. Exact hashes and the blocker
contract are recorded in `docs/reports/HYBRID_NATIVE_OVERRIDE_BATCH_POC.md`.

**Tests:** Focused hybrid Debug build and four hybrid CTests pass. The native
batch is intentionally not reported as equivalent because state checkpoints are
not clean.

**Result:** `HYBRID_OVERRIDE_NOT_YET_REPEATABLE`.

**Next:** Stop. Do not add timing-engine work, repair `0x3820`, investigate ID3,
search manually, broaden coverage or add AI generation.

## 2026-09-07 — M11.29 minimal safe override target — PHASE 1
**Objective:** Select one naturally executed, short direct-return routine from
the existing 600-frame neutral scenario before implementing override. Reuse
existing execution/CFG evidence and the hybrid hook, with no ranking framework,
manual gameplay, ID3 work or repair of the 0x3820 timing/CCR/prefetch contract.

**Prerequisite:** M11.28 is committed and pushed as
`ae1b76d951ba8c125e96873bc81f404bfa44c4db`. Remote `main` matches; exact-SHA CI
run `34153157452` is completed/success. Source-only Linux CI-equivalent build
and 47/47 tests, and Windows focused Debug/Release 5/5 each passed before push.

**Acceptance:** Identify one fully decoded bounded target with natural calls,
direct entry/RTS, no indirect flow, no I/O/self-modification, and an exact
register/RAM/CCR contract. Check observed interrupt absence and existing GPGX
return/prefetch mechanisms without implementing replacement. Report the target
and reasons at the requested pre-implementation checkpoint. Subsequent shadow
and override must prove full effects, body skipping, and EMULATED checkpoint/
video equivalence; those proofs are not inferred from candidate selection.

**Candidate selection:** `0x2D66..0x2D84` is selected. Existing 600-frame
neutral PC bitmap contains the target, and static ROM evidence has direct
caller `0x2D58` plus the exact 10-instruction decoder range. The body is
`MOVEM.L D7/A3,-(A7); CLR.W D7; MOVE.B (A6)+,D7; LEA $FF134C,A3;
ADDA.W D7,A3; MOVE.B (A6)+,D7; MOVE.W (A6)+,(A3)+; DBF D7,loop;
MOVEM.L (A7)+,D7/A3; RTS`. It has no nested call, indirect flow, VDP/Z80/I/O
address, self-modifying write or unknown CCR.X producer; the final `MOVE.W`
derives N/Z/V/C and X is preserved. `DBF` preserves SR, and MOVEM/RTS preserve
the remaining flags. Its only effects are bounded A6 post-increments,
`0xFF134C` RAM writes, A7 saved-register stack traffic and restored D7/A3,
making it materially simpler than `0x3820`. The nearby executed `0x6121A`
leaf was rejected because it writes VDP address `0xC00011`; `0x62CC` and
`0xA8DA` were rejected because the deterministic 600-frame bitmap does not
contain them. Candidate selection is complete before override code.

## 2026-09-07 — M11.29 minimal safe override target — PROVEN
**Objective:** Prove one native override on a naturally executed routine with a
materially simpler CPU contract than `0x3820`, using the existing deterministic
600-frame neutral scenario and developer-only GPGX hook.

**Implementation:** Added target-specific `Candidate2D66` shadow/override and
bridge register/memory setters. It captures D0-D7/A0-A7, PC and full SR,
canonical ROM source bytes, bounded output and the exact 12-byte stack window.
Shadow checks MOVEM bus order, output/stack writes, full SR, post-increments and
RTS return. Override applies those effects and uses GPGX `m68k_set_reg(PC)` for
the documented CPU jump; it does not emulate prefetch or create a replacement
engine. The adapter remains developer-only and is absent from production links.

**Evidence:** Canonical ROM SHA-256 is
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
Instrumented GPGX source is commit `d60d079934977aa6973e220d123533387159f66e`
and its DLL SHA-256 is
`4488ae775fd8252d8a8cb169000f20ac388e6d102fa6e1b1d4a4eccaa7f49799`.
Target `0x2D66` had one natural call in 600 frames, one clean shadow
comparison and zero divergences. Native override had one call, zero original
target-body instruction starts and completed all 600 frames. Checkpoint sequence
SHA-256 is `1690fd7d9bd1a26aeacaccfeff943f17f2bb30a5e130f28a88973916e8e9abca`;
video sequence SHA-256 is
`5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`; both
match EMULATED. No interrupt occurred inside the candidate.

**Tests/build:** Synthetic candidate shadow/override test passes in MSVC Debug,
Release and GNU/Linux. Focused hybrid contract/dispatch/candidate CTests pass
3/3 in all three configurations; project file-limit CTest passes in Windows
Debug/Release (4/4 selections including the three hybrid tests). The
GNU/Linux-equivalent Release build and three hybrid tests pass; its mounted-NTFS
file-limit scan was not used because that known scan is too slow on WSL. The
real EMULATED, SHADOW_NATIVE and NATIVE_OVERRIDE 600-frame runs pass in both
MSVC configurations. `git diff --check` passes; generated run directories
remain untracked.

**Result:** `HYBRID_NATIVE_OVERRIDE_MINIMAL_PROVEN`.

**Exact next step:** STOP. Do not repair `0x3820`, investigate ID3, add manual
gameplay, broaden coverage, add AI generation or change production runtime
dependencies.

## 2026-09-07 — M11.28 hybrid native migration PoC — SHADOW PROVEN / OVERRIDE BLOCKED
**Objective:** Stop M11.27 manual ID3 runtime hunting. Prove a developer-only
natural GPGX dispatch boundary for exactly `0x3820`, with `EMULATED`,
`SHADOW_NATIVE` and fail-closed `NATIVE_OVERRIDE` states. Reuse the existing
mechanical and native decompression implementations and deterministic neutral
startup scenario; no gameplay search, ID3 investigation or coverage expansion.

**Acceptance:** Record canonical ROM/build identity, bounded entry and return
state, natural-call/comparison/divergence counts and independent output/source
equivalence. Override is permitted only with a complete proven CPU/memory/return
contract and clean bounded shadow corpus; otherwise name its precise missing
contract. If enabled, prove body skipping and continued scenario equivalence.
Run relevant Debug/Release tests/builds, GNU/Linux linking, file limits and
diff review. Keep all hybrid code outside production targets and dependencies.

**Implementation:** Added the small developer-only `src/tools/hybrid/`
contract/dispatch/frontend, synthetic regression tests and a read-only bridge
installed in the existing external GPGX hook build. The only external existing
source edit is a coverage/UI opt-out guard; CPU architecture is unchanged.
ADR-0011 records the scope. Production source/link dependencies are unchanged.

**Evidence:** Six distinct natural calls (two format A, four format B) per
600-frame cold-reset neutral scenario. Two Debug and one Release shadow runs
provide 18 clean comparisons; all call logs are byte-identical. Ten emulator
state checkpoints and all 600 video callbacks match the EMULATED baseline.
Six interrupts occur during calls in each run. Capture is bounded to the exact
source/output/saved-stack footprint; no broad trace or coverage is collected.
Canonical ROM/build identity, per-call values and hashes are recorded in
`reports/HYBRID_NATIVE_MIGRATION_POC.md` and local `build/m1128/` evidence.

**Tests/build:** Relevant GCC Debug/Release targets and focused CTest selections
pass 5/5 each (hybrid contract, dispatch, existing graphics/mechanical tests,
source limits). Canonical graphics references pass in both configurations.
GNU/Linux Release frontend/contract/test linking and both hybrid plus existing
graphics/mechanical tests pass. The external Windows GPGX DLL rebuild passes;
no Linux GPGX runtime comparison was performed. The override CLI rejects before
loading GPGX or creating a scenario directory. No commit/push was performed.
The redundant Linux source-limit scan over mounted NTFS was stopped after
more than six minutes; Windows source-limit checks passed. Final Linux
functional CTest selection: 4/4. This is not a full repository CI claim.

**Result:** `HYBRID_SHADOW_PROVEN_OVERRIDE_BLOCKED`. This is a bounded shadow
effect proof, not full CPU equivalence: SR mask is `0xFFEF`, excluding X.
CCR.X, cycle/refresh and interrupt scheduling, and prefetch/IR return state
lack independent replacement contracts. Native override calls = 0; original
body skip = false; continuation after override = not run. Preliminary failed
capture guards are retained and explicitly excluded from accepted evidence.

**Exact next step:** STOP as requested. No manual gameplay/ID3 search, other
routine, AI generation or broader tooling work is active.

**Publication closure (subsequent user request):** Finalized the focused M11.28
implementation/tests/report for commit and push before starting M11.29.
Fresh Windows Debug/Release builds and focused checks pass 5/5 each. A
source-only copy on the native WSL filesystem passes the complete GNU/Linux
Debug build and all 47/47 CTests, including file limits; this resolves the
earlier mounted-NTFS validation limitation. Staged diff review and
`git diff --cached --check` pass. Only source, tests and governance/report prose
are included; ROM, captures, binaries, generated evidence and existing
untracked `game.srm` are excluded. Remote main and exact-commit CI will be
verified after push before M11.29 candidate discovery.

## 2026-09-07 — M11.26 resource ID 3 visual-role investigation — PARTIAL
**Objective:** Determine the bounded visual consumption of the verified ID 3
resource without broad RE, new runtime capture, Screen 0 implementation or
semantic naming.

**Evidence:** The caller at `0x02CFA2..0x02CFB8` selects ID `3` and destination
`0x4000`, then ID `4` and destination `0x5000`, through the verified `0xD3B2`
loader. With 32 bytes per Genesis 4bpp tile, ID 3 occupies pattern indices
`0x0200..0x027F` (128 tiles). The adjacent `0xD406..0xD7AE` path initializes
scene/map RAM and VDP state, but the bounded decode contains no proven CRAM
association or exact plane-name-table/sprite selection for those patterns.

**Implementation:** No production code or diagnostic palette was changed.
`DIAGNOSTIC_PALETTE` remains explicitly non-authentic. Added the bounded report
`docs/reports/NATIVE_ROM_RESOURCE_ID3_VISUAL_ROLE.md` and updated the file map.

**Tests:** Existing M11.25 resource/negative/tile-oracle evidence remains the
input baseline. Bounded exact-decoder/assembler checks for the ID 3 caller,
`0xD3B2`, `0xD406..0xD7AE` and `0xD7C0` completed. No ROM or generated artifact
was added.

**Result:** `RESOURCE_ID3_VISUAL_ROLE_PARTIAL`: a graphics/data bank in a scene
initialization path is supported, but palette and displayed consumer remain
unknown. No whole-resource semantic promotion was made.

**Exact next step:** A — connect verified ID 3 visual data to one authentic
layout. Do not implement it in M11.26.

## 2026-09-07 — M11.25 first authentic native ROM resource baseline — HIGH VALUE
**Objective:** Use only the verified M11.24 resource ID 3 contract in the
production native executable. No GPGX, replay, new capture, room integration or
semantic resource naming was added.

**Implementation:** Added a bounded resource loader that reads the verified
table range from the canonical ROM, invokes the existing native decompressor,
checks the `0x702` compressed consumption, `0x1000` output size and expected
SHA-256, and returns normalized resource bytes. Added a VDP transfer and a
diagnostic native mode using the existing 4bpp tile decoder. The mode renders
128 tiles from native VRAM `0x4000..0x4FFF` with an explicitly documented
`DIAGNOSTIC_PALETTE`; the existing ControlledScreen mode remains unchanged.

**Evidence:** Resource output and VRAM range SHA-256 are both
`36bbea13cb564ab194dd438b1fe75076525525068b57f2f02a4c0142630dc277`.
The deterministic diagnostic framebuffer SHA-256 is
`d2b7655501ff3babf6ef9dd44af720b3afab3ba3e2673fa7aeff33248a3ab1b1`.
The native executable opened the diagnostic window and remained responsive.

**Tests:** Debug build, Release production/reference targets, all `45/45`
CTest tests, deterministic canonical-ROM reference checks in Debug and
Release, negative resource/VRAM checks, source limits and artifact hygiene
passed. No ROM, extracted asset, capture or binary is tracked.

**Result:** `NATIVE_ROM_RESOURCE_ID3_HIGH_VALUE`. This proves an authentic
ROM-backed native resource path and structural tile compatibility, not a
semantic room/background/player/sprite assignment.

**Exact next step:** A — identify resource ID 3 palette/visual role. Do not
implement it in M11.25.

## 2026-09-07 — M11.24 bounded resource contract `0x02CFAA -> 0xD3B2` — VERIFIED
**Objective:** Prove one bounded resource-loading contract for resource ID `3`
without new runtime instrumentation, replay, broad RE tooling or semantic
naming.

**Evidence:** Natural retained GPGX execution facts reach caller `0x02CFAA`,
loader `0xD3B2` and decompressor `0x3820`. The caller sets `D0=3` and
`D1=0x4000`. Canonical ROM table entry `0x05CEA2` contains `0x1AE1A8`; the
existing native decompressor consumes `1794` bytes to `0x1AE8AA`, which is the
next table pointer, and emits `4096` bytes at the loader's
`0xFF2FA8` destination. Native and the independent mechanical original-derived
`0x3820` translation are byte-identical with output SHA-256
`36bbea13cb564ab194dd438b1fe75076525525068b57f2f02a4c0142630dc277`.

**Immediate consumer:** The bounded post-call sequence at `0xD3D2..0xD3E4`
queues source word address `0x7F97D4` (`0xFF2FA8 >> 1`), destination `D1`, and
`0x800` words in the DMA descriptor queue at `0xFF1892`. No semantic asset or
room label was added.

**Tests:** Existing native decompressor tests and ROM-backed reference checks
pass; the one-off M11.24 calculation was removed after recording its result.
`git diff --check` and the source-file limit check pass. No ROM, capture,
bitmap, binary or generated commercial data is tracked.

**Result:** `RESOURCE_CONTRACT_ID3_VERIFIED`. Semantic role remains
`RESOURCE_ROLE_PARTIAL`; only the bounded selection, stream, output and
immediate transfer contract are promoted.

**Exact next step:** E — return to native gameplay. Do not implement it in
M11.24.

## 2026-09-07 — M11.23 GPGX evidence integrity repair — REPAIRED
**Objective:** Repair only confirmed producer attribution, exact bitmap-union,
JSON parsing, bounded completeness and provenance defects before the bounded
resource contract. No new capture or RE work was started.

**Implementation:** GPGX range emitters now split on first-reader PC or access
width changes. Sega-Thor correlation validation now proves sorted,
non-overlapping range union equals the ROM-read bitmap. The importer now uses
small dependency-free structural JSON modules instead of line-oriented regex.
Bounded classification independently reconciles sequential exact-decoder
starts/lengths through the target boundary. Capture metadata/build IDs and
persistent merge compatibility are checked; old captures remain
`LEGACY_WEAK`.

**Evidence:** External repaired GPGX commit
`d60d079934977aa6973e220d123533387159f66e` builds with GCC 16.2.0. Native
reader-range and auto-analysis regressions pass. Existing raw M11.19–M11.22
execution and ROM-read facts remain valid; reader group/correlation outputs
dependent on old grouping require a future capture/regeneration.

**Tests:** Focused Python tests, importer self-test and external native tests
pass. Full Debug build and CTest pass (`45/45`), and the focused Release
importer target/self-test pass. The full Release MinGW build is blocked by an
unrelated pre-existing `oasis_re_callee_effect` static-library link failure;
this local toolchain limitation is not hidden or repaired in M11.23.

**Result:** `GPGX_EVIDENCE_INTEGRITY_REPAIRED`. Full Debug/CTest and all
focused regressions are green; the documented full Release MinGW limitation is
unrelated to this evidence pipeline.

**Historical next step (superseded by M11.24):** bounded resource contract
`0x02CFAA -> 0xD3B2`. M11.23 itself did not implement that contract.

## 2026-09-07 — GPGX analysis ROM-read to reader-PC correlation — HIGH VALUE
**Objective:** Correlate the existing post-startup analysis ROM-read regions
with their first executed reader PCs, without adding runtime instrumentation,
semantic naming, replay, destination provenance or trust promotion.

**Implementation:** Added the developer-only deterministic tool
`src/tools/oasis_gpgx_rom_reader_correlation.py`. It validates canonical ROM,
analysis bitmap/ranges, analysis metadata, GPGX execution provenance and the
current classification artifact; deduplicates exact regions; groups only by
the retained first-reader field; and emits per-region, per-reader,
classification, top-20 and checksum-contamination summaries. Added focused
synthetic tests and CTest registration.

**Evidence:** The retained automatic post-startup capture contains `103,674`
analysis ROM bytes in `1,412` regions and `90` first-reader PCs. Correlation
produced `1,099` ASM-roundtrip bytes, `93,877` statically supported bytes and
`8,698` runtime-executed-unknown bytes. Reader `0x000380` has zero analysis
regions/bytes, so checksum coverage does not dominate the analysis artifact;
the reader is not hardcoded or blacklisted. Exact first-reader-only limits are
reported rather than expanded into multi-reader claims.

**Artifacts:** Human report
`docs/reports/GPGX_ROM_READER_CORRELATION.md`; generated JSON is kept under
ignored `build/` and no ROM, DLL, bitmap or runtime dump is tracked. The
report records canonical ROM SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`, GPGX
SHA `27426f00aa68f9f358c86919e8a40985326fa05b`, and the input artifact hashes.

**Tests:** Tool self-test, focused Python tests, CTest
`oasis_gpgx_rom_reader_correlation`, deterministic JSON/report rerun,
`git diff --check`, and the project file-limit check all passed. CMake was
configured in the separate local `build-correlation` directory because the
existing `build` cache points at an old checkout path.

**Result:** `GPGX_ROM_READER_CORRELATION_HIGH_VALUE`.

**Exact next step:** STOP GPGX EXPERIMENT and request independent Astra audit.

## 2026-09-07 — M11.22 bounded static classification for `0x060BB6` — PASS
**Objective:** Classify only the proven bounded unit `0x060BB6-0x060BC4`
without expanding boundaries, naming a routine, starting a new capture or
changing the broad trust model.

**Implementation:** Added the developer-only
`src/tools/gpgx_bounded_classification.py` and its regression test. The tool
re-verifies canonical ROM bytes, exact decode and address-level runtime
execution, then requires the existing exact bounded decoder and structural
explorer evidence for the four requested edges. It emits the deterministic
artifact `build/m11-22-gpgx-bounded-classification.json` and changes no
adjacent classifications.

**Evidence:** All 8/8 target instruction starts decode and are observed. The
bounded unit changes from current `UNKNOWN`/runtime-unknown evidence to
`CODE_STATIC_SUPPORTED`; each observed PC retains
`CODE_EXECUTED_AT_ADDRESS`. Boundary status remains
`LIKELY_INTERNAL_BLOCK`, with no routine identity or whole-routine promotion.
The artifact records canonical ROM, runtime, exact-decoder, bounded-decoder,
explorer and M11.21 report SHA-256 provenance.

**Tests:** Python bounded-classification tests and `--self-test` passed. CMake
reconfigured successfully with MSYS2 UCRT64; the importer self-test and new
CTest both passed (`2/2`).

**Result:** `BOUNDED_REGION_060BB6_STATIC_SUPPORTED`.

**Artifacts:** `docs/reports/RUNTIME_REGION_060BB6_CLASSIFICATION.md` and
`build/m11-22-gpgx-bounded-classification.json`.

**Exact next step:** recommendation **A — investigate parent routine boundary
around `0x060B90`**; do not implement it in this task.

## 2026-09-07 — M11.21 bounded investigation of `0x060BB6` — STRUCTURALLY UNDERSTOOD
**Objective:** Determine the instruction boundaries and bounded direct control
flow of runtime-executed region `0x060BB6-0x060BC4` using retained M11.19/M11.20
evidence, without new capture, replay, ranking changes, semantic naming or
automatic trust promotion.

**Evidence:** The canonical exact decoder reports all eight target PCs as
decoded and observed: `0x060BB6`, `0x060BB8`, `0x060BBA`, `0x060BBC`,
`0x060BBE`, `0x060BC0`, `0x060BC2` and `0x060BC4`. The bounded CFG confirms
the loop-back `0x060BC2 -> 0x060B90`, the alternate entry
`0x060BAA -> 0x060BC4`, fallthrough entry `0x060BAE -> 0x060BB6`, and the
successor call `0x060BCC -> 0x0604BC`. The lower/upper static window was
extended only to include complete instructions crossing its requested edges.

**Result:** `RUNTIME_REGION_060BB6_STRUCTURALLY_UNDERSTOOD`. The target is a
likely internal bounded fragment, not a proven whole routine. Evidence supports
`CODE_EXECUTED_AT_ADDRESS` for the eight PCs only; no range-level promotion was
made. Rank-1 `0x000374-0x0003A0` remains higher by transparent score but has
no static support or gameplay-specific corroboration in the retained artifacts.

**Tests:** Existing exact decoder, candidate-map and bounded explorer outputs
were inspected. No new deterministic translation/classification logic was
added, so no new test was required.

**Artifacts:** `docs/reports/RUNTIME_REGION_060BB6.md`.

**Exact next step:** recommendation **B — classify bounded region with static
support**; do not implement it in this task.

## 2026-09-07 — M11.20 runtime-unknown prioritization — HIGH VALUE
**Objective:** Produce a deterministic bounded shortlist from the M11.19
manual-realtime executed-PC evidence without new capture, replay automation,
semantic naming, mass classification or automatic trust changes.

**Implementation:** Added `src/tools/gpgx_unknown_priority.py`. It validates
the canonical ROM against the M11.19 artifact, consumes the existing exact
decoder output, audited classifications, candidate map, Ghidra evidence and
bounded structural-explorer output, then groups only contiguous even
instruction starts as neutral `RUNTIME_EXECUTED_REGION`s. The transparent
score rewards observed/decoded PCs, known xrefs/edges, candidate/Ghidra/
explorer overlap and proximity to trusted ranges; it penalizes unsupported
decodes, tiny fragments and conflicts. Repeated-hit data was unavailable and
contributes zero. Top-five slices follow only direct local control flow and
stop at unsupported, indirect, external or region-boundary edges.

**Evidence:** `12,698` runtime-unknown PCs form `9,012` regions. The top
target is `0x000374-0x0003A0` with score `99`; it was not selected by anchor
priority. `525` regions are separately marked
`RUNTIME_EXECUTED_STATIC_CORROBORATED`; the top-20 includes
`0x060BB6-0x060BC4` with score `80`. Anchor checks remain independent:
`0x62CC`, `0x9BF2` and `0xD3B2` are exact-decoder
`CODE_STATIC_SUPPORTED` addresses outside unknown regions; `0xA8DA` was not
observed. The systemic pattern is Ghidra overlap without corresponding
trusted/static corroboration (`4,949/9,012` regions overlap Ghidra, while
`525` have stronger candidate/audit support). No mass fix was applied.

**Artifacts:** deterministic JSON is `build/m11-20-gpgx-priority.json`; the
required report is `docs/reports/RUNTIME_EXECUTED_UNKNOWN_PRIORITY.md`.
Running the tool twice produced identical JSON SHA-256
`E37BA2E5C83B8F8E79AED002E89033B7F185D5FF86950A016AFAB3D98217E540` and
identical report SHA-256
`4889FD7AAAE25725922146275C7D5C846DCB210532D2FC4A796CC993FB8AE181`.

**Tests:** Python self-test, persistent coverage semantics and source line
limit CTest passed. The exact-decoder report and bounded explorer were run
from existing tools; no runtime capture was started.

**Result:** `RUNTIME_UNKNOWN_PRIORITIZATION_HIGH_VALUE`.

**Exact next step:** `A. investigate highest-ranked region` — do not
implement it in this task.

## 2026-09-07 — M11.19 GPGX executed-PC evidence import — HIGH VALUE
**Objective:** Integrate the validated manual-realtime Genesis Plus GX capture
into the trust pipeline without changing instrumentation semantics, range
classifications, gameplay code or `main`.

**Implementation:** Added the developer-only
`oasis_re_import_gpgx_coverage` tool and its CTest self-test. It validates the
canonical ROM SHA-256, bitmap size and metadata/file hashes; imports global
and session bitmaps idempotently; retains even address-level
`CODE_EXECUTED_AT_ADDRESS` facts; records `DECODE_UNSUPPORTED`; reports
`RUNTIME_EXECUTED_UNKNOWN` and data conflicts; and emits deterministic JSON
plus `docs/reports/GPGX_RUNTIME_EXECUTION_TRUST.md`. It reports coverage of
existing ranges but applies no range-level promotion.

**Evidence:** Canonical ROM SHA-256 is
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
The imported global capture has 14,732 unique PCs; the current session has
1,447 new PCs from 11,153 frames. The evidence artifact has 14,732 unique
addresses and 16,179 address/capture facts, with 14,638 decoded starts and
94 unsupported decoder results. Classification summary is
`ASM_ROUNDTRIP_EXACT=1666`, `CODE_STATIC_SUPPORTED=362`, `CODE_EXECUTED=6`,
`DATA_REGION_SUPPORTED=0`, `DATA_STRUCTURE_SUPPORTED=0`, `UNKNOWN=0`,
`RUNTIME_EXECUTED_UNKNOWN=12698`, `RUNTIME_DATA_CONFLICT=0`; range-level
changes are zero. Anchors observed are `0x3820`, `0x62CC`, `0x9BF2`,
`0xD3B2`, `0x6121A`; `0xA8DA` was not observed.

**Tests:** `oasis_re_import_gpgx_coverage --self-test` passed. CTest passed
`oasis_re_import_gpgx_coverage_self_test` and `project_file_line_limit`.
The built importer was run against both validated captures; the emitted
capture metadata, fact counts and deterministic report were inspected.

**Result:** `GPGX_RUNTIME_EXECUTION_EVIDENCE_HIGH_VALUE`. The trust pipeline
now has address-level runtime evidence only; it does not claim function
boundaries, semantics or full-range execution.

**Exact next step:** `A. prioritize RUNTIME_EXECUTED_UNKNOWN regions for
bounded static investigation` is recommended. Do not implement it in this
task.

## 2026-09-07 — GPGX persistent coverage validation run — PASS
**Objective:** Validate one fresh manual realtime capture after the SHA-256 metadata fix, including provenance and persistent merge invariants.

**Evidence:** RetroArch was launched through the desktop shortcut with the fixed instrumented DLL; the user played manually and performed normal shutdown. The capture contains `11,153` frames, `185.883` seconds and `113,589,477` instruction starts. Metadata reports `known_pcs_before=13,285`, `session_unique_pcs=12,006`, `new_pcs_session=1,447`, `known_pcs_after=14,732`; `13,285 + 1,447 = 14,732`. Global bitmap load status was `loaded` and commit status was `saved`.

**Independent checks:** all three metadata bitmap SHA-256 values exactly matched the on-disk files. Bitmap size was `196,608` bytes for all maps; session-new was a subset of session-all, disjoint from reconstructed global-known-before, and `global_after == global_before OR session_all` passed. The NEW report contains exactly `1,447` PCs in `1,114` neutral regions; `1,442` decoded plus `5` `DECODE_UNSUPPORTED` equals `1,447`.

**Classification summary:** `ASM_ROUNDTRIP_EXACT=139`, `CODE_STATIC_SUPPORTED=18`, `CODE_EXECUTED=0`, `DATA_REGION_SUPPORTED=0`, `DATA_STRUCTURE_SUPPORTED=0`, `UNKNOWN=1,290`, runtime/data conflicts `0`.

**Anchors:** `0x3820=85`, `0x62CC=267`, `0x9BF2=21,406`, `0xA8DA=0`, `0xD3B2=4`, `0x6121A=9`. `0xD3B2` was newly discovered in this session; the other reached anchors were already known.

**Provenance:** `rom_file_sha256` is explicitly `unavailable` because the frontend buffer is not claimed as the exact original file. The canonical local ROM independently remains SHA-256 `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`. The corrected runtime SHA implementation was verified against the standard `abc` test vector.

**Result:** `PERSISTENT_COVERAGE_HIGH_VALUE`.

**Exact next step:** `C. integrate GPGX executed-PC evidence into trust pipeline` may be considered, but integration is intentionally not implemented in this task.

## 2026-09-07 — GPGX persistent coverage and NEW executed-code report
**Objective:** Extend the external Genesis Plus GX realtime PC capture with persistent global knowledge and a developer-side report for PCs newly executed in the current session.

**Actions:**
- Added persistent `global_known_pc_bitmap.bin` loading with exact-size validation, session-all/session-new maps, atomic global temp-and-replace commit, sorted PC lists, neutral adjacent-PC ranges, and expanded coverage metadata.
- Kept the M68K hook bounded to bitmap marking, atomic counters, last-PC and anchor updates; no I/O, decoder, UI or semantic promotion is performed in the hook.
- Extended the live window with the main `NEW PCs THIS SESSION` indicator, known-before/after counts, global load/commit state and separate dirty flags.
- Added `oasis_gpgx_coverage_report`, which consumes a canonical ROM and `session_new_pc_bitmap.bin`, uses the existing exact decoder, records `DECODE_UNSUPPORTED` explicitly, and optionally compares code/data classification ranges without inventing function boundaries.

**Files changed:** `core/debug/coverage.c`, `core/debug/coverage.h`, `core/debug/coverage_storage.c`, `core/debug/coverage_storage.h`, `core/debug/coverage_ui.c`, `libretro/libretro.c` in `C:\Github\Genesis-Plus-GX-instrumented`; `src/tools/gpgx_coverage_report.cpp`, `CMakeLists.txt`, `tests/gpgx_persistent_coverage_test.py`, `docs/FILE_MAP.md` here.

**Evidence/tests:** instrumented GPGX DLL built with MSYS2 UCRT64 `make -f Makefile.libretro platform=win HOOK_CPU=1 -j4`; CMake configured in `build-codex-msys2` and `oasis_gpgx_coverage_report` built; persistent three-session contract CTest passed; report smoke test decoded 14,621 PCs into 10,093 neutral regions from the existing local capture bitmap. This smoke input is legacy all-session coverage, not claimed as a new persistent gameplay session.

**Result:** implementation and developer tooling are locally build-verified. A fresh manual RetroArch session is still required to verify the new persistent files and global merge using real user input.

**Unresolved:** no automated input or RetroArch shutdown was performed; no new gameplay capture was generated in this turn.

**Exact next step:** user manually starts the instrumented core with `GPGX_COVERAGE_DIR` pointing at the capture directory, plays, exits normally, and checks the five new session/global files before a second session validates `new_pcs_session` against the first global bitmap.

## 2026-09-06 — M11.19 Test A live GPGX coverage experiment
Objective: validate the existing Genesis-Plus-GX `HOOK_CPU` path with a
minimal address-level M68K instruction-start bitmap, while keeping this
repository's gameplay code and `main` unchanged.

Scope was limited to Test A. No Stable-Retro, live-map, Atlas, RL or C++
gameplay translation work was started. The official upstream repository was
cloned outside this repository at `C:\Github\Genesis-Plus-GX` and pinned at
`27426f00aa68f9f358c86919e8a40985326fa05b`. The instrumented worktree is
`C:\Github\Genesis-Plus-GX-instrumented` on branch
`experiment/gpgx-live-coverage`; the baseline source tree remained
unmodified, with only its generated DLL untracked.

Installed tools were checked rather than reinstalled: Git for Windows
2.55.0.2, MSYS2 UCRT64 with GCC 16.2.0, GNU Make 4.4.1 and zlib 1.3.2-2,
and RetroArch 1.22.2. The unmodified baseline command passed:
`make -f Makefile.libretro platform=win -j$(nproc)`. The clean upstream
`HOOK_CPU=1` build first failed at link time because the upstream header's
tentative `cpu_hook` definition is rejected by this GCC default (`-fno-common`);
the exact failure was multiple `cpu_hook` definitions. After that cause was
established, the instrumented branch made only the declaration correction to
`extern`, then added the coverage callback and bounded export outside the
instruction hook. The hook build passed with:
`make -f Makefile.libretro platform=win HOOK_CPU=1 -j$(nproc)`.

The callback only checks `HOOK_M68K_E` and the `0x300000` cartridge-ROM
address bound, sets one bit per instruction-start byte in a 393216-byte fixed
bitmap, and increments a counter. It performs no logging, file I/O, JSON,
disassembly, allocation, locking, IPC, GUI or AI work. Export is performed at
core unload/deinit; bounded checkpoints are sampled every 60 frames.

The canonical 3 MiB Beyond Oasis ROM was loaded through RetroArch with both
DLLs. The reproducible 600-frame replay was
`C:\Github\gpgx-test-roms\test-a-600.rpl` (SHA-256
`B36FFD6782DD8ADDD1C84CC992C516B2F18BFF7B16CE918BE56C9F5A3344CB0C`). It
was a no-input startup/title segment, not a manually played movement segment.
Instrumentation evidence was positive: unique PCs grew from 173 at frame 60
to 2188 at frame 600, with 6488885 instruction starts total. Two independent
runs produced identical report SHA-256
`CBFF415FF07CCF328A451B1B9F27FAB64E5AA17235D8AA23D9CEE88C762BCFA2` and
bitmap SHA-256
`BF97D2CCCB1CA1400C3A386B5A62AB8C07C5A185BE5C95BA7C78C595A366674E`.

Seven fast-forward wall-clock runs over the same replay gave median baseline
8.274 ms and instrumented 15.177 ms for 600 frames, an overhead ratio of
1.8342x. This is a process wall-time measurement including frontend startup,
not a core-internal frame timer. The core reported nominal 59.92 FPS while
loaded. The instrumentation and deterministic replay checks pass, but strict
Test A is `FAIL`: the measured segment contained no gameplay input and the
native automation surface did not provide a human visual/input check, so
gameplay correctness and human playability remain unverified.

Exact next step: do not start Test B or build workaround architecture. Obtain
a real input-bearing RetroArch gameplay replay (or a user-run equivalent),
repeat the same baseline/instrumented measurement, and reassess Test A.

## 2026-09-06 — M11.18.1 Native vertical slice validation closure
Objective: close the remaining M11.18 validation gaps on the current checkout
without expanding gameplay scope.

Baseline: verified `19d364403757e1f19afbff5607235692523e893d` on `main`.

Manual Win32 evidence: launched the Debug native executable with the canonical
USA retail ROM. The window opened with the expected title and stayed responsive.
Cardinal movement, all four diagonal combinations, discrete release, focus-loss
clearing, focus restoration without stuck movement, visible wall collision and
continued responsiveness were checked through the native UI sequence. Explorer
was used as the foreground focus target. The key API exposes press/release
events, so no separate held-key event could be injected; the observed release
and foreground-transition behavior was recorded without claiming a stronger
held-key observation than the harness supports.

Automated evidence: the existing 600-frame replay and framebuffer SHA-256
oracle remained green with hash
`3e1c211e1560ea42243e27f05be0704537c51ec3901995d89f228b0e616aa0da`.
Canonical-ROM acceptance and beta/unsupported-ROM rejection remained green.
MSVC Debug/Release builds and CTest passed; Linux build and CTest passed with
the mounted-NTFS slow source-limit case excluded, while source-limit passed in
the Windows configurations and separate source scan. MinGW `_WIN32`
syntax-only checks passed; compile/link still fails in the saved local driver at
assembler/collect2 with no diagnostics because its cached sysroot is absent.
This is `TOOLCHAIN_UNAVAILABLE_LOCAL`, not a project regression, and tests were
not weakened.

Result: `NATIVE_VERTICAL_SLICE_PLAYABLE` for the supported Win32 runtime. No
reproducible product blocker was found. The intentionally unavailable
non-Windows GUI adapter is a portability limitation, not an M11.18 Win32
acceptance blocker.

Exact next step: recommendation D — platform portability. Do not implement it
as part of this closure.

CI: GitHub Actions run `34052574735` for closure commit
`7de43a393409ab7c4c0548645a9467afbfbed438` completed successfully. A final
documentation-only commit recorded this result. Its final exact-SHA CI run is
recorded below.

Final CI: GitHub Actions run `34052627403` for
`4118867a487227275d18246c93b8ebbe7980d8f2` completed successfully.

## 2026-09-06 — M11.18 Native controlled screen vertical slice
Objective: implement the first interactive native path while freezing broad RE
and reconstructed-source expansion.

Actions: reused `RuntimeLoop`, `ControllerState`, existing player movement
constants, `ByteGridView` footprint aggregation and the terrain gate. Added a
synthetic 32x28 fixture with free terrain, joined wall/corner, isolated block
and explicit non-zero player footprint. Added deterministic software
rasterization into a 320x224 framebuffer. Added `oasis_platform` with a
minimal Win32 window, arrow-key polling, foreground/focus-safe input release
and scaled DIB presentation. The executable now validates canonical ROM
identity before opening the window. Added a 600-frame state replay and fixed
framebuffer SHA-256 oracle.

Files changed: `src/game/controlled_screen.*`, `src/game/render/framebuffer.*`,
`src/platform/window.*`, `src/game/player/player.cpp`, `src/main.cpp`,
`CMakeLists.txt`, `tests/native_vertical_slice_test.cpp`, and M11.18
documentation/state files.

Evidence: existing source-of-truth movement values remain right `0x36000`,
diagonal `0x2A000/0x25800`. The fixture test covers all 16 direction nibbles,
release, free movement, wall blocking, joined-corner blocking, map boundary
blocking and non-zero footprint blocking. Replays are equal after every one of
600 logical frames at presentation intervals 1, 7 and 31, including repeated
runs. Framebuffer hash is
`3e1c211e1560ea42243e27f05be0704537c51ec3901995d89f228b0e616aa0da`.

Tests/build: MSVC Visual Studio 18 2026 Debug and Release full builds passed;
Debug and Release CTest passed 40/40, including the vertical-slice test and
source-limit test. WSL Ubuntu 24.04 GNU/Linux configure/build passed and CTest
passed 39/39 when excluding only the mounted-NTFS slow source-limit test; the
same source-limit check passed in both Windows configurations and the separate
source scan. All changed C++ files passed MinGW `-fsyntax-only`, including
`_WIN32` platform syntax. The saved MinGW driver could not compile/link: its
assembler/collect2 exited with codes 1/53 without diagnostics because the
cached toolchain sysroot is unavailable. The canonical-ROM CLI smoke opened a
responsive window process with the expected title; a manual key/focus sequence
was not performed. CI was not run because no push/remote CI invocation was
requested.

Result: `NATIVE_VERTICAL_SLICE_PARTIAL`. Native game logic, fixture collision,
fixed-step replay and Windows presentation path exist; non-Windows GUI support
and end-to-end visual/input proof remain bounded limitations.

Unresolved: no original room, camera, sprite engine, combat, audio or ROM-derived
visual semantics were introduced. No RE detour was needed.

Exact next step: recommendation D — fix the remaining vertical-slice blocker.

## 2026-09-06 — M11.17 structured data classification PoC
TASK: prove a small set of ROM data structures without guessing unknown bytes,
running new emulator sweeps or changing production code.

IMPLEMENTATION: added `src/tools/re_structured_data.py` and its CTest helper.
The classifier parses the 64-entry vector table, fixed ROM header region,
two 16-byte terrain tables, the 21-entry `0xC92C` group table, the 108-entry
`0x5CE96` resource-pointer table and four 26-byte screen descriptors. It
records exact range/byte hashes, width, count, termination, consumers,
references, unresolved fields and explicit code/data conflicts. Updated the
future code-promotion gate to veto only explicit trusted-data overlap; weak
data hypotheses remain non-blocking.

RESULT: 10 ranges accepted: 9 `DATA_STRUCTURE_SUPPORTED` ranges (908 bytes)
and 1 `DATA_REGION_SUPPORTED` header range (256 bytes). There were 0 rejected
candidates, 0 conflicts and no payload-boundary guess. Full-ROM representation
and code trust counts remain unchanged. Decision:
`STRUCTURED_DATA_HIGH_VALUE`.

TESTS: synthetic structured-data and auto-promotion helper tests passed;
MSVC Debug/Release and Linux CTest remain green after CMake registration.
Source-limit, diff-check and artifact hygiene remain green. MinGW is not
available on this host; the prior M11.15 MinGW matrix is the available result.

NEXT ACTION: exactly one recommendation, A — return to the native vertical
slice. Do not implement it in this task.

## 2026-09-06 — M11.16 targeted dynamic code confirmation
TASK: confirm a bounded set of critical ranges with natural runtime evidence,
reusing retained reports and never forcing emulator state.

IMPLEMENTATION: added `src/tools/re_dynamic_confirm.py`, a pre-run selection
artifact and fail-closed confirmation report. Five ranges were selected:
`0x3820`, `0x62CC`, `0x9BF2`, `0xA8DA` and `0xD3B2`. Added pure helper tests
for exact range linkage, partial coverage, forced-evidence rejection, natural
promotion and non-propagation, and registered them with CTest. Updated the
ledger, roadmap, file map, task state and ADR-0007. No emulator, scenario,
production runtime, ROM or generated artifact was added to Git.

EVIDENCE: retained `natural-final-a.json` and `natural-final-b.json` share the
canonical ROM SHA-256 and confirm the `0x6121A` positive control twice at frame
113. No retained JSON artifact substantiates the older M11.8 prose count of 13
hits for `0x3820`; it remains unaccepted context. None of the five selected
ranges has an accepted natural hit artifact. Trust counts remain 197
`ASM_ROUNDTRIP_EXACT`, 5 `CODE_STATIC_SUPPORTED`, 1 `CODE_EXECUTED` and 0
`BEHAVIOR_VERIFIED`; full-ROM hashes remain exact.

RESULT: `TARGETED_DYNAMIC_REACHABILITY_LIMITED`. New-run efficiency is not
applicable because existing evidence was reused and no new run was authorized
or available in this checkout. The single next recommendation is D — one
separately authorized bounded timing/hold-input sweep around the M11.8 startup
transition; it is not implemented here.

VALIDATION: MSVC Debug and Release builds completed sequentially and CTest
passed 38/38 in both configurations. GNU/Linux CMake build completed and
CTest passed 37/37 when excluding the mounted-workspace source-limit test;
the Windows source-limit test passed, while the equivalent WSL mounted-path
run timed out as in the prior baseline. The standalone helper test passed,
`git diff --check` passed, and all edited executable/source files are below
500 lines. MinGW is not installed on this host, so a fresh MinGW matrix was
not runnable; the M11.15 MinGW 37/37 baseline remains the available result.

## 2026-09-06 — M11.15 evidence integrity audit and classification trust repair
TASK: audit every M11.14 reconstructed range and separate exact ASM bytes from
proof that a range is executable code; do not add promotions.

IMPLEMENTATION: added `src/tools/re_evidence_audit.py` and its bounded helper
test. The audit assembles all 203 artifacts, validates ROM/entry/range and
artifact identity, records concrete incoming xref sources and trusted caller
levels, emits an audited manifest/report, and normalizes RTS/MOVEQ/.s branch
form metrics. Added a deterministic negative data-like corpus and mismatch
tests. The promoter now labels a successful round trip `ASM_ROUNDTRIP_EXACT`
and raises trust only from explicit evidence fields. The source-limit CMake
check now covers Python, Lua, PowerShell, shell, JavaScript and TypeScript.

RESULT: 203/203 ranges round-trip exactly. Levels are 197
`ASM_ROUNDTRIP_EXACT` (12,520 bytes), 5 `CODE_STATIC_SUPPORTED` (1,006 bytes),
1 `CODE_EXECUTED` (24 bytes) and 0 `BEHAVIOR_VERIFIED`. One known Ghidra range
mismatch remains at `0x3820` (`0x38D0` vs `0x3B3E`); no weak caller chain is
trusted. The audited full ROM remains exact with canonical hashes; stored M11.9,
M11.10, M11.11, M11.12, M11.13 and M11.14 controls all report `MATCH`.

DECISION: `EVIDENCE_TRUST_NEEDS_FIXUPS`. Exactly one next recommendation is
B — targeted dynamic confirmation of critical code; it is deferred.

## 2026-09-06 — M11.14 automatic promotion large batch II
TASK: continue automatic blob-to-ASM promotion from the M11.13 manifest, with
up to 150 new candidates and no repeated attempted candidates except affected
slice mismatches retried after systemic investigation.

IMPLEMENTATION: the runner consumes a prior promotion report, excludes its
attempts, retries the two old slice mismatches, records mnemonic/operand/raw
opcode metadata, clusters rejects, and reports 25-attempt acceptance windows.
The general byte-immediate emitter preserves source extension words; vasm still
rejects noncanonical byte immediates. A decoder experiment that rejected the
repeated CMP opcode was reverted because it broke the M11.9 0x3820 control.

RESULT: 89 new candidates plus 2 retries produced 91 attempts, 73 accepted and
18 rejected (8 UNSUPPORTED_FORM, 7 ASSEMBLER_SYNTAX, 3 SLICE_MISMATCH). ASM
changed 6,462 -> 13,550 bytes and blobs 3,139,266 -> 3,132,178 bytes. The
manifest has 339 entries (203 code, 136 unknown), and no structured data was
promoted. Acceptance windows were 72%, 88%, 76% and 87.5% (`STABLE`).

OLD MISMATCHES: 0x020802 is an IR_OPERAND/nonrepresentable vasm byte-extension
case; 0x00B6A6 is ASM_ENCODING for the repeated cmp.w register form. Both stay
rejected and are documented in the batch report. All canonical hashes remain
exact; handwritten overrides remain zero.

DECISION: AUTO_PROMOTION_BATCH2_HIGH_VALUE. Exactly one next recommendation is
A — run another automatic code batch; it is deferred.

## 2026-09-06 — M11.13 automated promotion scale pass
TASK: continue M11.12 transactional blob-to-source promotion from its
post-promotion manifest, with at most 100 deterministic attempts and no new
address list.

IMPLEMENTATION: the runner default is now 100 candidates, resolves artifacts
from generated M11.12 final outputs, excludes candidates that no longer fit an
UNKNOWN range, classifies rejection classes, records accepted instruction forms
and reject-form counts, and preserves transactional rollback. A latent slice
difference reporting call was corrected. The shared emitter prints immediate
ori/andi/eori to CCR as .b; tests/re_assemble_test.cpp covers the general rule.

RESULT: 534 records discovered, 189 eligible after M11.12 exclusion, 100
attempted, 84 accepted and 16 rejected (14 UNSUPPORTED_FORM, 2 SLICE_MISMATCH).
ASM changed 2,632 -> 6,462 bytes and blobs 3,143,096 -> 3,139,266 bytes. The
final manifest has 231 contiguous entries (130 code, 101 unknown), and no
structured-data promotion occurred. Both former CCR candidates (0x00DA2A,
0x00B9EC) now pass. The full ROM is 3,145,728 bytes with canonical hashes.

REGRESSION: M11.9, M11.10, M11.11 and M11.12 controls all report MATCH.

DECISION: AUTO_PROMOTION_SCALE_HIGH_VALUE. Exactly one next recommendation is
A — continue automatic code promotion with another large batch; it is deferred.

## 2026-09-06 — M11.12 automated blob-to-source promotion PoC
TASK: reduce UNKNOWN blob coverage from the M11.11 full-ROM split using only
existing candidate-map and mass-verification evidence.
MILESTONE UNDERSTANDING CONFIDENCE: 99% — M11.11 already proves the complete
layout and exact comparison; this slice only changes accepted source ownership.
CURRENT SLICE UNDERSTANDING CONFIDENCE: 98% — candidates require agreed bounded
decoder ranges, supported IR, no conflicts and two exact byte comparisons.
SLICE CONFIDENCE EVIDENCE: 210 eligible records were ranked deterministically;
21 promotions passed slice and full-ROM verification, while four transactions
were rejected and rolled back.

IMPLEMENTATION: added `oasis_re_assemble_range` as a thin CLI over the existing
decoder and ASM emitter, plus `src/tools/re_auto_promote.py`. The runner loads
the M11.11 manifest, discovers/ranks candidates from existing evidence, emits a
trial ASM range, assembles and compares it, rebuilds the full layout, and only
then commits the promotion to its local generated output. Added bounded helper
tests, the PC-relative emitter regression and `docs/AUTO_BLOB_PROMOTION.md`.
No production runtime, semantic naming or classifier rewrite changed.

RESULT: 534 records discovered, 210 eligible, 25 attempted and 21 accepted.
Coverage changed from ASM 1,846 to 2,632 bytes (+786) and blobs from 3,143,882
to 3,143,096 bytes (-786). Structured data and conflicts remain zero. The final
manifest has 88 entries (46 code, 42 unknown), zero gaps and zero overlaps.
The full 3,145,728-byte ROM matches canonical CRC32/SHA-1/SHA-256 exactly.

REJECTIONS: `0x00E6BA` and `0x00E268` were unsupported exact-IR forms;
`0x00DA2A` and `0x00B9EC` reached the assembler's `ori.w #$1,CCR` blocker.
No rejected trial changed the accepted manifest.

REGRESSION: the M11.9 controls, M11.10 corpus and M11.11 full-ROM baseline are
rerun by the runner and remain exact. Handwritten opcode overrides: 0.

TESTS: Python helper tests pass; the bounded PC-relative emitter test passes.
MSVC Debug/Release CTest pass 36/36 each, including the source-limit test.
MinGW Debug/Release CTest pass 35/35 with the mounted-tree line-limit test
excluded. Linux GCC builds successfully and CTest passes 35/35 with that test
excluded. Generated ROMs, blobs, reports and binaries remain ignored.

DECISION: `AUTO_PROMOTION_HIGH_VALUE`. Exactly one next recommendation is A —
increase the automatic promotion batch to 100 candidates. It is deferred and
not implemented by this task.

OPEN QUESTIONS: structured-data promotion remains at zero because exact table
length/representation evidence is insufficient; semantic ownership of promoted
code remains UNKNOWN.

## 2026-09-06 — M11.11 full-ROM split reassembly baseline
TASK: build a deterministic full-ROM split for the canonical USA ROM without
attempting whole-ROM semantic disassembly.
MILESTONE UNDERSTANDING CONFIDENCE: 99% — M11.9/M11.10 provide the trusted
decoder/emitter corpus and the task explicitly permits local-ROM blobs.
CURRENT SLICE UNDERSTANDING CONFIDENCE: 99% — the split is contiguous and the
assembler comparison is byte-level, with no semantic claims for blob ranges.
SLICE CONFIDENCE EVIDENCE: 25 M11.10 ranges are reused, vasm emits the same
flags as the prior exact baseline, and the full output has no first difference.

IMPLEMENTATION: added `src/tools/re_full_split_run.py`. It invokes the existing
M11.10 runner, copies only its 25 verified ASM ranges, extracts all other bytes
from the hash-verified local ROM into ignored blobs, writes a contiguous
manifest/layout, assembles one full output and reports first differences with
manifest entry and artifact type. Added deterministic helper tests and a
CTest registration when Python is available. No production runtime or
classifier/data semantics changed.

RESULT: `build/m11-11/full1` contains a 50-entry manifest spanning
`[0x000000,0x300000)`: 25 `CODE_VERIFIED` ranges and 25 `UNKNOWN` blob ranges,
zero gaps, zero overlaps, smallest range 10 bytes and largest range 3,091,450
bytes. Full output is 3,145,728/3,145,728 bytes and exact. Coverage is ASM
1,846 bytes (0.0586827596%), structured data 0, blobs 3,143,882 bytes
(99.9413172404%), conflicts 0. Canonical CRC32/SHA-1/SHA-256 all match.

REGRESSION: M11.9 controls, M11.10 25-slice corpus, expanded split and legacy
split all report MATCH before the full comparison. Rebuilt ROM, blobs,
assembler binaries and local corpus remain ignored.

TESTS: `python tests/re_full_split_test.py` passes. Full baseline runs pass with
MSVC Debug/Release and MinGW Debug/Release `oasis_re_assemble`. MSVC Debug and
Release CTest are 35/35; MinGW Debug and Release CTest are 34/34 with the
mounted-tree line-limit test excluded; Linux GCC build and CTest are 34/34 with
that same exclusion. The source-limit test passes in both MSVC configurations;
`git diff --check` and artifact-hygiene checks are clean.

DECISION: `FULL_ROM_SPLIT_EXACT`. Exactly one next recommendation is A — begin
automatically replacing verified blob regions with ASM/data. It is deferred and
not implemented by this task.

OPEN QUESTIONS: 99.9413172404% remains deliberately opaque blob data; no
semantic ownership, data structure or full-ROM disassembly claim is made.

## 2026-09-06 — M11.10 diverse reassembly coverage expansion
TASK: expand the M11.9 exact reassembly experiment to exactly 25 diverse
mass/explorer-selected routines while retaining controls `0x3820`, `0x62CC`
and `0xA8DA`.

IMPLEMENTATION: `re_assemble_report.cpp` now emits 25 sorted bounded slices.
The standard-library runner inventories operation/width/operand-kind forms,
classifies the bounded mismatch families, and reconstructs the M11.9 mixed
split `[0x1108,0xA8F0)` as a separate regression. vasm source naming is made
unique for that legacy layout because vasm resolves a conflicting `main.asm`
basename from its working directory. No per-function opcode override or raw
`dc.w` patch was added.

RESULT: local canonical-USA run `build/m11-10/corpus23` is 25/25 exact,
606 instructions, 1,846/1,846 selected bytes, expanded split MATCH, and legacy
mixed split MATCH. The bounded inventory is 105 distinct practical forms;
105/105 are exact (100%), with zero selected unsupported forms and zero final
mismatches. The runner records the complete machine-readable inventory in
`result.json`. Generated ROM-derived files remain ignored.

TESTS: MSVC 19.51 Debug and Release builds and CTest pass 34/34. The focused
MSVC reassembly test covers indexed EA, SWAP, EXT, exact operands, branch widths,
MOVEM, address arithmetic, and first-difference behavior. Ubuntu WSL GCC rebuilt
all targets successfully; its behavioral CTest pass is 33/33 when the known slow
mounted-filesystem line-limit scan is excluded. The equivalent MSVC line-limit
test passes in both configurations.
MinGW CMake compilation currently fails in the local toolchain's temporary
assembler invocation without diagnostics; direct MSVC/Linux builds are green.

DOCS: TASK, PROJECT_STATE, REVERSE_ENGINEERING and REASSEMBLY_POC now record the
25-slice evidence, 105-form metric, mismatch classes and legacy split. The
decision is `DIVERSE_REASSEMBLY_HIGH_VALUE`; exactly one next recommendation is
A — establish a full-ROM split baseline. That recommendation is not implemented.

OPEN QUESTIONS: runtime ownership and semantic meaning of the new leaves remain
UNKNOWN; bounded form coverage is not a whole-ROM/general-68000 claim. The Linux
mounted-filesystem line-limit scan was started but terminated after over four
minutes without progress; explicit changed-file counts remain below 500 lines.
Run final diff/artifact review, then commit and push this checkpoint.

## 2026-09-06 — M11.9 exact reassembly verified locally
CI/PUSH: implementation commit `1d2d10d` was pushed after local validation.
GitHub Actions run `34029898270` completed successfully (configure/build/test).
CI uses synthetic inputs; the commercial-ROM round-trips remain locally verified.
The sole annotation was the existing checkout action's Node.js deprecation.
TASK STATUS: DONE / `REASSEMBLABLE_DISASM_POC_HIGH_VALUE`.

RESULT: `REASSEMBLABLE_DISASM_POC_HIGH_VALUE`. Canonical ROM through the existing
decoder, decoder-owned exact operands, generated ASM and vasm matches 5/5 slices:
0x3820 (306 instructions/798 bytes), 0x62CC (6/24), A8DA (10/22), 0x1108 (4/10),
0x2B6E (4/28). Total 330 instructions, 882/882 ASM bytes, 100%, zero final
mismatches/unsupported selected forms/handwritten opcode overrides. Full 0x3820
is covered; all four A8DA postincrement writes are retained.

IMPLEMENTATION: developer-only exact normalization within the existing decoder,
ASM and raw-word/typed-operand JSON emission, first-difference verification and
standard-library Python assembler runner. SUBA.L's existing SUBX-mask collision
was repaired; address-arithmetic long EA and MOVEM word widths are retained.
The decoder, ROM identity and frozen mass candidate evidence were reused; no
second decoder, architecture change/ADR, production runtime or C++ emitter.

SPLIT: the local layout covers only [0x1108,0xA8F0), 38,888 bytes, with 38,006
unknown bytes preserved in four exact local-ROM blobs. The whole bounded layout
also matches. All generated ASM/IR, blobs, binaries and tool sources are ignored.
Per-routine bounds, status, evidence, assumptions, meanings and test status are
recorded in REVERSE_ENGINEERING and REASSEMBLY_POC.

ASSEMBLER: no installed M68K assembler was found. Official source host refused
connection; public vaelen/vasm mirror revision
`8ecb8e6c7a31350ef32c7f1fee289e3677a9a8f0` was built locally with configured
MinGW GCC. vasm 1.8g/backend 2.3f/Motorola syntax 3.13, `-m68000 -no-opt -Fbin`.
Without `-no-opt`, 0x3820 shrinks to 794 bytes and first diverges at 0x382B.
One bounded assembler workaround class, no per-instruction encoding patches.
License/provenance and reproduction commands are in docs/REASSEMBLY_POC.md;
no claim is made about Ancient's historical assembler.

VERIFICATION: MinGW Debug and Release full builds and CTest pass 34/34, with
assertions enabled in Release. Both CLI pipelines independently match all five
routines and the split. MSVC 19.51 was found via vswhere despite older notes;
Debug and Release full builds/CTest pass 34/34. Ubuntu-24.04 WSL GCC Debug
full build/link and CTest pass 34/34; the filesystem line-limit scan took 244s
on the mounted Windows checkout. Final focused checks after review are recorded
below. No missing local toolchain is claimed.

FINAL PRE-PUSH CHECKS: after the last IR metadata review, MinGW and MSVC full
Debug/Release builds and CTest again pass 34/34. Linux full relinking and all
33 behavioral tests pass; its already-green filesystem-only line-limit test was
not repeated across the slow mount. The current Windows CTest file-limit check
and explicit counts including the Python runner pass (maximum changed source:
465 lines). Final Debug/Release pipelines match 5/5 plus the split and produce
17 identical ASM/IR/manifest/binary artifacts. Staged diff review, diff --check
and artifact hygiene pass: only the 17 reviewed source/build/documentation files
are staged; no ROM, commercial output, state or assembler binary is tracked.

NEGATIVE CONTROL: deliberately changing rebuilt A8DA byte 9 returns exit 1 and
`FIRST_DIFFERENCE rom_offset=0x00A8E3 slice_offset=0x9 expected=0xC2 actual=0xC3
instruction=0x00A8E2 move.w D2,(A5)+`. Synthetic tests also cover missing/extra
bytes, invalid bounds, exact operand/branch sizes and golden deterministic ASM.

FILES CHANGED: decoder/IR, bounded emitter/comparator/CLI/runner, CMake target and
test, TASK/PROJECT_STATE and project worklog/RE/file map/roadmap/reproduction docs.
OPEN QUESTIONS: semantics/runtime of new leaves remain UNKNOWN; A8DA's older
mass multiple-entry flag is retained. No full-ROM or general-encoding claim.
Manual engineering effort estimate: 2–3 hours, not measured human labor.
EXACT NEXT ACTION: recommendation A — expand to 25–50 verified routines;
not implemented. Push and verify CI for this checkpoint, then STOP.

## 2026-09-06 — M11.9 initial acceptance criteria
TASK: prove canonical ROM -> existing decoder with exact operands -> deterministic
68000 ASM -> external assembler -> byte comparison for exactly five selected slices.
WHY: make encoding fidelity independently verifiable before future emitters.
CURRENT MILESTONE: post-M11 bounded RE tooling, M11.9; M12 remains deferred.
MILESTONE UNDERSTANDING CONFIDENCE: 93% for the tooling boundary.
CURRENT SLICE UNDERSTANDING CONFIDENCE: 95% for operand extraction; assembler
round-trip is still unverified. SLICE MODE: RE_TOOLING_ONLY.
SLICE CONFIDENCE EVIDENCE: existing decoder, canonical local ROM, prior A8DA
repair and mass candidate evidence; no handwritten C++ is an instruction oracle.
BASELINE: synchronized clean main/origin/main at `77c0c44`.
ACCEPTANCE CRITERIA: five bounded selections, A8DA mandatory exact and at least
four exact round-trips; typed operands/raw words, deterministic ASM and useful
first difference; ignored local split/blobs with offsets; synthetic tests,
Debug/Release CTest, Linux link/build, file limits and artifact hygiene green.
EVIDENCE AVAILABLE: local canonical ROM, current MinGW toolchain, Ubuntu WSL,
mass map and existing decoder/Atlas. Initial clean leaf choices: 0x1108 and
0x2B6E (4 instructions each, no mass failure reasons or indirect flow).
KNOWN UNKNOWNS: assembler encoding choices; gameplay semantics of leaves;
Ancient's historical assembler remains UNKNOWN. No architecture change/ADR,
production runtime, C++ emitter, full ROM split or automatic discovery is planned.

## 2026-09-06 — M11.6.2 static translation trust restored
TASK: repair only the three bounded static-translation PoC defects identified
by the task: A8DA memory operands, CCR X semantics and Release assertion
coverage.

BASELINE: synchronized `main` and `origin/main` at `a2bc039`. The canonical
USA ROM remained local-only with the previously recorded SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

EVIDENCE: the existing bounded decoder was rerun for `0xA8DA` and `0x62CC`.
It confirms `0x3AC2` is `MOVE.W D2,(A5)+`, three further A8DA word stores
through `(A5)+`, and the complete six-instruction 0x62CC leaf. No scenario,
BizHawk, ant, runtime or production work was performed.

IMPLEMENTATION: A8DA now takes `BoundedMemory`, performs four exact big-endian
word writes, postincrements A5 after each store and preserves upper register
halves. Narrow independent CCR helpers implement bit-4 X preservation for
MOVE/MOVEQ/CMPI and X=carry for ADD/ADDQ. 0x62CC now preserves X across its
MOVEQ/MOVE.L/MOVE.W sequence. `TranslationStatus::verified` was replaced by
neutral `EXECUTED`; verification remains external. The static translation test
was added to both Release `-UNDEBUG` and MinGW runtime-path CMake lists.

OLD ERROR: the former M11.6 Case B fixture is invalidated. It used empty
memory and expected the same register-only operand interpretation as the
incorrect implementation, so it could pass while missing every RAM write.
The replacement fixture independently derives four addresses/word values,
A5 delta, D0/D5, upper halves, CCR/X and the no-write early path. 0x62CC now
uses non-zero starting memory, non-zero upper register halves and X=1.

TESTS: MinGW Debug full build and CTest `33/33` passed. MinGW Release full
build and CTest `33/33` passed. The actual Release compile command for
`tests/re_static_translation_test.cpp` contained `-O3 -DNDEBUG -UNDEBUG`,
proving assertions are active. 0x3820 positive-control tests remained green.
The CTest file-line-limit check passed. MSVC tools were not available on this
host and are not a blocker under the task instructions. GitHub Actions run
`34026330082` completed successfully; its only annotation was the unrelated
Node.js 20 action deprecation warning.

RESULT: `STATIC_TRANSLATION_TRUST_RESTORED`. Production runtime is unchanged.

EXACT NEXT ACTION: recommendation C — expand independent machine-semantics
reference tests. Do not implement the next step in this task.

## 2026-09-06 — M11.8 natural reachability recovery advanced root cause
TASK: recover a natural caller for `0x62CC`, or produce a concrete blocker and
reproducible next experiment beyond M11.7.

BASELINE: synchronized `main` and `origin/main` at `67c99b7`. The canonical
USA ROM remained local-only with SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

IMPLEMENTATION: added the developer-only
`re_bizhawk_m11_8_natural_scenario.txt` with 25 natural input events, all 33
static incoming targets, player/event owners, `0x3820`/`0x60004` controls and
21 RAM watches. Extended `re_bizhawk_natural_reach.lua` to emit the input
schedule, frame-boundary PC/RAM samples, optional PC-region counters and a
bounded per-target snapshot policy. No emulator state, PC, register, CCR, RAM,
ROM or savestate was written.

RUNTIME EVIDENCE: canonical USA hardware reset, 1800/1800 frames. The final
report covered 43 targets, 21 RAM bytes and 25 input events. `0x3820` hit 13
times and `0x60004` hit 5 times. `0x62CC` and every one of its 33 incoming PCs
hit 0 times. Frame PCs were dominated by startup/system transition helpers
`0x32EE/0x32F4` and `0x3A8E8/0x3A8EE`; watched RAM changed, so this is a live
pre-game transition blocker rather than a zero-RAM freeze. The result is
`ROOT_CAUSE_ADVANCED`, not `FAILED` and not a claim of global unreachability.

STATIC TRIAGE: all 33 sites were ranked from bounded local slices. The highest
priority natural hypotheses are `0x5850` in the `0x557A` player owner,
`0x61FE/0x62EC` after `0x85E2`, and the `0x7AC2/0x7B60` event/entity path.
The next experiment is a bounded timing/hold-input sweep around the startup/
transition helpers `0x6135E`, `0x32EE` and `0x3A8E8` retaining exact target
hooks and state capture.

TESTS: MinGW Debug build and CTest passed 33/33; MinGW Release build and
CTest passed 33/33. `git diff --check`, tracked-artifact inspection and the
CTest file-line-limit test passed. MSVC tools/`VsDevCmd.bat` were not present
on this host, so MSVC Debug/Release was not runnable. A short BizHawk scenario
self-check after the probe changes produced a valid JSON report with the
expected target/RAM/sample fields. GitHub Actions run `34024337014` completed
successfully for the pushed implementation commit.

NEXT ACTION: commit this focused tooling/docs/scenario change, push `main`,
wait for GitHub Actions, verify the remote SHA and clean tree, then stop at
the bounded timing/hold-input experiment boundary.

## 2026-09-06 — M11.7 single-target reachability root cause completed
TASK: explain why natural execution does not reach only `0x62CC`, using the
existing static tools and two existing BizHawk scenarios without creating a
new gameplay scenario or forcing emulator state.

BASELINE: synchronized `main` and `origin/main` at `942b00a779eb2fe4ff2ec62c4dc9ba44c9e41050`.

STATIC EVIDENCE: `0x62CC` is the valid six-instruction leaf
`[0x62CC,0x62E4)`. Bounded slices confirmed direct local edges
`0x5850 -> 0x62CC`, `0x61FE -> 0x62CC`, `0x62EC -> 0x62CC` and
`0x7B60 -> 0x62CC`; a complete even-address ROM branch-reference scan found
33 direct incoming branch/call encodings. The boot slice confirms
`0x8B2E -> 0x557A`; the player dispatcher is the bounded `0x59B8` indirect
state path. Static requirements are CCR.C=0 after `0x85E2` for the player
`BCC.W` edges, or D0=`0x01FF` at `0x7B3C` after the first `0x60004` call on
the `0x7B2A` event path.

RUNTIME EVIDENCE: the existing hardware-reset neutral scenario ran 300 frames
and the existing `120:Start` scenario ran 1800 frames. A single-target
observation of `0x62CC` plus all 33 direct incoming PCs reported zero hits for
every incoming PC and the target in both scenarios. Neither scenario reached
`0x8B22`, `0x8B2E`, `0x557A`, `0x59B8`, `0x61F6`, `0x62E4` or `0x7B2A`.
The nearest observed shared raw entry was `0x60004` (2 neutral hits and 5
`120:Start` hits) on the already-known `0x611EE`/`0x6121A` path; it is not a
static predecessor of `0x62CC`. No branch outcome, RAM value or writer was
promoted from an unexecuted target path.

RESULT: `CALLER_NOT_REACHED`. The current scenario corpus remains in the
boot/transition path before the target-owned player/event callers. This is not
evidence that `0x62CC` is globally unreachable, and it is not a static or
translation failure. The exact first gameplay-state transition remains outside
the current evidence boundary.

TESTS: after the SSD environment repair, fresh GNU/MinGW Debug and Release
builds passed. CTest passed 33/33 in both configurations with the active MinGW
DLL directory in the test-process PATH; the earlier system dialogs were
missing-DLL launch failures. File-limit, artifact hygiene and `git diff --check`
passed. GitHub Actions run `34021104372` for `6a48232` completed successfully.

NEXT ACTION: recommendation A — build one minimal natural scenario that causes
the missing state. Stop here; do not implement that scenario, force a branch,
edit RAM/registers/CCR/ROM, install a callback, expand ant work or begin M12.

## 2026-09-06 — M11.6.1 runtime capture fixup completed with existing-scenario gap
TASK: attempt only natural runtime capture/replay for the existing static
translation leaves `0xA8DA` and `0x62CC`.
BASELINE: focused M11.5 commit `a02e6b4` and M11.6 commit `6e56c06` were
pushed to `main`; both local and remote were `6e56c06080c61277c3e3fb4515904aee8d764ff2`,
and GitHub Actions run `34019056461` completed successfully.
IMPLEMENTATION: added only the developer-only `OASIS_TARGET_ADDRESSES` override
to `re_bizhawk_natural_reach.lua`; existing scenario, hardware-reset start and
natural input policy remain unchanged. No PC/register/CCR/RAM/ROM/savestate
mutation was used.
EVIDENCE: the ROM path was passed with correct quoting. Neutral hardware-reset
ran for 300 frames and existing `120:Start` ran for 1800 frames. Both emitted
valid reports with `target_hits=0` for `0xA8DA` and `0x62CC`,
`target_reached=false` and `entry=null`. The earlier BizHawk exception was only
an unquoted `Start-Process` argument and was not the final runtime result.
RESULT: both `0xA8DA` and `0x62CC` are
`RUNTIME_CAPTURE_UNAVAILABLE_EXISTING_SCENARIOS`. No natural invocation was
accepted, so no register/flag/memory/PC/return replay or mismatch comparison
exists. The static fixtures remain unchanged. The task's `PARTIAL` decision
predicate requires at least one confirmed B/C capture; with both unavailable,
no one of the three runtime decision enums applies, and no translation failure
is inferred.
TESTS: the pushed M11.6 baseline had MSVC Debug/Release and GNU/MinGW CTest
33/33 plus successful CI. The post-capture change is Lua/docs only; final
repository checks are rerun before the fixup push. No ROM, savestate, emulator
binary or generated report is tracked.
EXACT NEXT ACTION: recommendation B — create a small bounded set of new
natural gameplay scenarios in a separately authorized task and repeat only this
bounded capture. Stop before translator, interpreter, production CPU model or
M12 work.

## 2026-09-06 — M11.6 verified static translation PoC completed
TASK: test whether three confirmed/bounded 68000 slices can become ordinary
compiled C++ with evidence-preserving differential checks, without adding a
runtime interpreter or production CPU emulator.
BASELINE: synchronized `main` and `origin/main` at
`02e532ce832f88abf8d039d5961638c4d2263cdd`; pre-existing M11.5 reachability
and queue changes were preserved.
IMPLEMENTATION: added developer-only `re_static_translation.*`, a CLI and one
contract test. The minimal state is D0-D7, A0-A7, CCR and bounded big-endian
memory. Mechanical functions are ordinary compiled C++; there is no PC loop,
fallback interpreter, full bus or production dependency.
CASES: A=`0x3820` (306 instructions) reproduced both existing local USA-ROM
vectors against `oasis::game::decompress_graphics`: `1217 -> 3072` and
`112 -> 128`, with no mismatch. B=`0xA8DA` (10 instructions) passed two
normalized static register/CCR fixtures, including the early branch. C=`0x62CC`
(6 instructions) passed one normalized RAM-state fixture with four ordered
writes. B/C are not claimed as BizHawk runtime captures; their states are
anchored to static ROM/mass evidence. The comparator tests first register and
memory-byte divergence.
Unsupported opcode handling returns explicit STOP and is tested.
METRICS: routines `3`; verified `3`; unsupported fixtures `1`; mechanical
implementation `297` LOC plus `77` LOC interface; handwritten fixups `0` by
the experiment definition; attempted/passed `2/2`, `2/2`, `1/1`; verification
mismatches `0`; approximate manual work `3-4 h`; first verified result about
`2 h`. These metrics include bounded support/comparison code and do not claim
that all mechanical LOC came from an automatic frontend.
DECISION: `STATIC_TRANSLATION_POC_NEEDS_FIXUPS`. The bounded output is useful,
but A still uses shaped helper code and B/C use captured fixtures rather than
a general frontend. Recommendation C: use mechanical translation only as a
verification aid. Do not implement the next step here.
TESTS: MSVC Debug/Release builds and full CTest passed `33/33` in each
configuration. MinGW/GNU-equivalent build and full CTest passed `33/33` after
the configured MinGW `bin` directory was added to the process PATH; without
that PATH, Windows showed the expected missing `libstdc++-6.dll` launcher
error. File-limit passed, tracked sensitive-artifact count was zero, and
`git diff --check` passed. Native Linux/WSL remains unavailable.
OPEN QUESTIONS: whether a future verification aid should emit more leaf forms;
no production translation is authorized by this checkpoint.
EXACT NEXT ACTION: stop.

## 2026-09-06 — M11.5 ant reachability diagnostic completed
TASK: determine whether existing deterministic scenarios reach ten selected
unresolved `INDIRECT_FLOW` frontier source PCs before ant target resolution.
BASELINE: synchronized `main` and `origin/main` at `02e532ce832f88abf8d039d5961638c4d2263cdd`.
The fresh bounded explorer contained 35 unresolved indirect frontiers; the
three previously accepted dynamic sources `0x045A`, `0x61F60` and `0x62878`
were excluded. Queue selection supplied ten contexts: `0x0790`, `0x5328`,
`0x59B8`, `0x85F8`, `0xA322`, `0xA332`, `0xA680`, `0xA690` and two owners of
`0xA7E2`.
IMPLEMENTATION: extended the existing developer-only natural reach probe with
a reachability-only mode and first-hit snapshots. It does not write emulator
state, add workers or change production code. Watch-only manifests reused the
existing hardware-reset neutral scenario and the existing `120:Start` scenario.
EVIDENCE: all ten contexts were `NOT_REACHED` in both scenarios (18 matrix
cells, 9 unique watched PCs, two batched runs). Static ROM bytes matched every
frontier record; no static suspect was found. The historical negative controls
`0x0790` and `0x5328` remained unreached. A fresh positive control resolved
`0x045A -> 0x307A` at frame 113, sequence 1734712, in 104103 ms with BizHawk
2.11.1 and the canonical USA ROM SHA-256.
METRICS: sampled 10; existing scenarios inspected 4 applicable scenario
families; matrix combinations 18 and executed 18 via 2 batched runs; source
PCs reached 0; naturally/checkpoint reachable 0; no existing scenario reach 10;
static suspects 0; sampled ant retests 0; resolved sampled edges 0; NOT_REACHED
after matching 10. Median and p90 time-to-source are not applicable. The old
blind ant cost is about 104 s/frontier from the prior queue; the two shared
reachability runs took about 65 s total, avoiding an estimated 16+ minutes of
blind attempts for this sample.
DECISION: `SCENARIO_COVERAGE_INSUFFICIENT`; the environment is healthy but the
current scenario corpus has no matched path for this sample. Recommendation B:
create a small bounded set of new natural gameplay scenarios. Stop here; do not
resume the blind queue, add parallelism, build checkpoints, or begin M12.
TESTS: fresh MSVC Debug/Release and MinGW builds; CTest 32/32 in each; project
file-limit test included. `git diff --check` remains required before commit.

## 2026-09-05 — M11.5 medium sequential ant queue started
TASK: scale the proven single-worker sequential queue from five jobs to a
bounded Phase A sample of exactly 25 frozen jobs, then decide whether Phase B=50
is justified by measured yield.
BASELINE: synchronized `main` and `origin/main` at
`02e532ce832f88abf8d039d5961638c4d2263cdd`. Reuse audit confirms the existing
`re_ant.*`, `re_ant_queue.*`, BizHawk worker and explorer are the only required
machinery; no scheduler or second worker is authorized.
SCOPE: extend the bounded queue-size guard/CLI and add prefix/selection/failure
metrics only. Phase A must remain frozen after generation; natural evidence and
the existing lifecycle/dedup rules remain unchanged. Phase B is conditional on
the Phase A gate and cannot exceed 50.
KNOWN UNKNOWNS: the prior neutral scenario reached three of five jobs; lower
ranks may be dominated by `NOT_REACHED`, making frontier reachability rather
than worker throughput the likely bottleneck.
EXACT NEXT ACTION: implement bounded Phase A scaling and metrics, then run the
25-job queue strictly sequentially.

## 2026-09-05 — M11.5 sequential ant queue PoC completed
TASK: process exactly five frozen `oasis.m68k.re-ant-job.v1` jobs through one
sequential BizHawk worker and batch-merge accepted natural observations.
IMPLEMENTATION: added the developer-only `oasis.m68k.re-ant-queue.v1` model,
deterministic frontier ranking, bounded lifecycle/claim/recovery/finalize API,
queue CLI, queue contract tests and provenance-aware input/result fields in the
existing ant worker. The queue is frozen at five jobs and refuses additions;
only one job may be `CLAIMED`. Duplicate suppression and stale-claim recovery
are covered synthetically. Production runtime is unchanged.
QUEUE: `queue-0x4C23AB2632531710`, initial queue SHA-256
`DC3FC952C6B7682E7F8E28F0180F4C1E3AC9A5D2B55187F92B1E55AE71D97EA6`.
Selection ranked observed neutral frontiers first (`0x045A`, `0x61F60`,
`0x62878`), then stable-address fallbacks (`0x0790`, `0x5328`).
A/B: both runs used the identical frozen queue, one BizHawk process at a time
with a process restart between jobs. Three jobs resolved to `0x307A`, `0x6211A`
and `0x62900`; two ended `FAILED_FINAL` with `NOT_REACHED`. The normalized
result-set SHA-256 was identical in A/B:
`3B38333E9688208096CDA5D92178CFA0F01FBD32A15514E3A2AA9B9FE2657BFE`.
MERGE/ROI: one batch merge accepted three `DYNAMIC_NATURAL` edges and reran
the explorer once. Instruction bytes `60916 -> 61506` (+590), decoded
instructions `19623 -> 19765` (+142), discovered entries `504 -> 508` (+4),
entries processed `537 -> 541` (+4), unresolved indirects `35 -> 32` (-3),
frontiers `148 -> 145` (-3), dynamic edges `0 -> 3`.
PERFORMANCE: A worker execution totaled 520627 ms and B totaled 517110 ms;
three accepted edges over those sums are approximately 0.346/0.348 edges per
minute. Static explorer time was measured by the CLI but is not promoted here
as a cross-run invariant. Queue counts were selected/claimed/attempted 5,
resolved 3, NOT_REACHED 2, timeout 0, retryable 0, final 2,
nondeterministic 0, duplicate_jobs_avoided 0 and unique dynamic edges 3.
No ROM, savestate or trace artifact is tracked.
TESTS: Debug CTest 32/32, Release CTest 32/32 and GNU/MinGW-equivalent CTest
32/32 passed, including the queue contract and project file-limit tests.
`git diff --check` and the tracked sensitive-artifact check passed; WSL has no
installed Linux distribution, so a native Linux run was unavailable.
CI: GitHub Actions run `33963041522` for implementation commit `cb1c78b`
completed successfully.
UNKNOWNS: the fallback jobs remain naturally unreachable under the bounded
neutral scenario; no forced state, random input or semantic interpretation was
introduced. No ADR is required because the project direction is unchanged.
EXACT NEXT ACTION: record the successful CI result in this worklog, push the
docs-only update, verify its CI, then hard stop before parallelism or M12.

## 2026-09-05 — M11.5 sequential ant queue PoC started
TASK: extend the completed single-ant loop to one frozen bounded queue of five
sequential ant jobs and one BizHawk worker instance at a time.
WHY: measure whether explicit claim/finalize lifecycle, duplicate suppression,
honest NOT_REACHED results and one batch merge provide cumulative structural
gain without parallel emulator processes.
REUSE AUDIT: reuse `re_ant.*`, `re_bizhawk_ant.lua`, `re_explore.*`, existing
ROM identity and scenario/input normalization. No second ant implementation,
CPU emulator, database or scheduler is planned.
EVIDENCE: baseline main is `55ffd5bc5034db90c202c4ace8d1650e14ffdead`.
The current fresh explorer has 35 INDIRECT_FLOW frontiers. A full 1800-frame
neutral natural probe reaches `0x045A`, `0x61F60` and `0x62878`; the queue
will rank observed frontiers first and use a deterministic stable-address
fallback for two additional frozen frontier jobs. This allows honest
NOT_REACHED outcomes without inventing dynamic targets.
ACCEPTANCE: exactly five frozen jobs, deterministic queue A/B, one CLAIMED at a
time, lifecycle/recovery/duplicate tests, one worker at a time, batch merge,
before/after metrics, ROI, full local validation, push and hard stop.
KNOWN UNKNOWNS: two fallback frontiers may not be reached naturally; this is
data rather than a reason to force registers/flags or expand the scenario.
EXACT NEXT ACTION: add queue model/CLI and job-input support, then run the
five-job queue A/B.

## 2026-09-05 — M11.5 single-ant closed-loop PoC completed
TASK: implement one deterministic job → natural emulator observation → merge →
static rerun cycle for a single indirect-flow frontier.
IMPLEMENTATION: added developer-only `re_ant` job/result schemas and parser,
stable job identity, natural-only merge validation, dynamic-edge provenance,
the `oasis_re_ant` CLI, one BizHawk worker and synthetic contract tests. The
explorer now accepts only validated dynamic evidence and serializes its dynamic
edge class. Production C++ runtime behavior is unchanged.
SELECTION: `0xA7E2` was rejected after zero reachability in the existing
natural scenarios. The current frontier at `0x020E:0x045A` (`4E 91`, `JSR
(A1)+`) was reached naturally during selection at frame 114.
EVIDENCE: job `ant-0x43919998981C2FF` used the canonical USA ROM, BizHawk 2.11.1,
hardware reset, neutral input, no checkpoint, and limits 3000000 instructions /
300 frames. Runs A/B both observed A1 and next PC/target `0x307A` at frame 113,
sequence 1734712, result hash `0x21238399`; only wall-clock timing differed.
MERGE: accepted `DYNAMIC_NATURAL`, rejected-path rules covered by tests, one
dynamic edge preserved the source/frontier/backend/scenario provenance across
the job/result/edge artifacts, and the selected frontier disappeared on rerun.
ROI: instruction bytes `60916 -> 61396` (+480), decoded instructions
`19623 -> 19729` (+106), entries processed `537 -> 539` (+2), unresolved
indirects `35 -> 34` (-1), frontiers `148 -> 147` (-1), dynamic edges `0 -> 1`.
The worker took approximately 51.5 seconds per run; static explorer time was
15.5 seconds baseline and 15.8 seconds in the merge rerun.
TESTS: synthetic ant tests passed; Debug CTest 31/31, Release CTest 31/31 and
GNU/MinGW-equivalent CTest 31/31 passed, including the project file-limit test;
diff-check and sensitive-artifact checks also passed. WSL has no installed
Linux distribution, so a native Linux run was unavailable.
UNKNOWNS: `0x307A` is a resolved structural target only; its game role and
runtime semantics remain unknown. No savestate or ROM artifact is tracked.
STATUS: COMPLETE.
EXACT NEXT ACTION: run the final local validation gate, commit/push this single
conceptual change, verify CI, then hard stop without a second ant job.

## 2026-09-05 — M11.5 single-ant closed-loop PoC started
TASK: implement one deterministic `oasis.m68k.re-ant-job.v1` / result / merge
cycle for exactly one real `INDIRECT_FLOW` frontier.
WHY: prove the static-frontier → one natural BizHawk worker → runtime evidence
→ provenance-preserving merge → static rerun architecture without a swarm,
scheduler, forced state or production emulator dependency.
MILESTONE UNDERSTANDING CONFIDENCE: 94%.
CURRENT SLICE UNDERSTANDING CONFIDENCE: 93% for the bounded contract; target
observation remains pending.
SELECTION EVIDENCE: the latest ROM-wide explorer JSON is external-only and
contains 35 indirect-flow records. Preferred `0xA7E2` was tested with the
existing natural reset/Start scenarios and reached zero times. A single
bounded selection probe watching the current indirect PCs reached
`source_entry=0x020E`, `source_pc=0x045A` at frame 114; its instruction bytes
are `4E 91` (`JSR (A1)+`). This is the selected frontier. No job has yet been
created or merged.
ACCEPTANCE: one deterministic job; one BizHawk worker; natural-only evidence;
A/B normalized equality; wrong-frontier/ROM/forced/nondeterminism tests;
dynamic-provenance merge and one static explorer rerun; before/after metrics;
no tracked ROM/savestate/emulator artifact; local Debug/Release/GNU validation;
then STOP.
KNOWN UNKNOWNS: the runtime value in A1 and next PC/target are unknown until
the natural worker observes them; no semantic role may be assigned.
EXACT NEXT ACTION: add the minimal model, one Lua worker, merge and tests;
run only the selected job twice and stop.

## 2026-09-05 — M11.5 recursive structural explorer completed
TASK: implement and verify oasis_re_explore bounded recursive exploration
engine v1.
IMPLEMENTATION: added the developer-only re_explore model, guarded
deterministic priority worklist, compact persistent address map, explicit
entry states, edge/stop records, stable frontier identities, blocker
clusters, deterministic JSON/text formatters and CLI. It reuses
re_slice_decoder, re_atlas and re_candidate_map; no decoder, CFG or emulator
implementation was duplicated and no production target changed.
RAW BASELINE: fresh Ghidra 12.1.3 A/B exports from the canonical USA ROM were
390972 bytes each with identical SHA-256
613A3AA6DEB8D2DCF994C82ADC6A6939B7D5F27AF67A51D05C16F090D60A5315. Fresh
candidate-map and mass-verifier reruns using the approved local Beta input
were deterministic with SHA-256
9ACD162CE078C2D31C108BA480F0306DA75D6B8FFDFF2D008A1DAE8253263A9B and
6B9A6A366C0CEC047C1616A5840C8DA59D7AC89DD75021A0240F4F8E89D69C0D.
BOUNDED: 15 control entries passed. All six known direct-edge anchors were
recovered; 0x60004 -> 0x6042A is a direct jump encoding. RTS/RTE and the
0xA7E2 indirect blocker were observed as expected, and Atlas data guards were
not traversed as code.
ROM-WIDE: after the gate, one run processed 537 entries from 547 seeds,
analyzed 451, blocked 25 on indirect flow and 28 on unsupported instructions,
decoded 19623 instructions and emitted 148 frontiers. Coverage was 60916
instruction bytes, 432 pointer-data bytes, 6 probable-data bytes, 0 conflict
bytes and 3084374 unclassified bytes (98.05%). Frontier classes ranked by
count were DECODE_FAILURE 81, INDIRECT_FLOW 35 and UNSUPPORTED 32. These are
structural prioritization measurements, not semantic or expected byte gains.
DETERMINISM: ROM-wide JSON A/B SHA-256
8CD0C9B669786C76C16FF8E276A4314B5789925371153EB181783FBE8181F8DE and text
A/B SHA-256
E1E314077F300AE71AD3B6362865243CE6C917C48C18733786AD133EB5687460.
Measured CLI wall-clock was 15507 ms and 15462 ms; it is excluded from
serialized identity.
TESTS/VALIDATION: Debug CTest 30/30, Release CTest 30/30 and
GNU/LLVM-MinGW-equivalent CTest 30/30 passed; file-limit and git diff check
passed. WSL/Linux runtime/link check is unavailable because WSL is not
installed. No ROM, raw trace, Ghidra project/database, emulator or commercial
asset entered Git.
STATUS: DONE.
UNRESOLVED: indirect targets, Ghidra boundaries, unknown table extents and
routine semantics remain unknown. No dynamic ant or scheduler was started.
EXACT NEXT ACTION: stop; a future checkpoint may implement one bounded
single-emulator ant PoC against this frontier format.

## 2026-09-05 — M11.5 bounded recursive explorer started
TASK: implement `oasis_re_explore` bounded recursive exploration engine v1.
WHY: automate provable 68000 structural flow and preserve uncertainty as
explicit persistent map/frontier evidence without changing production runtime.
CURRENT MILESTONE: M11.5 post-M11 evidence tooling.
MILESTONE UNDERSTANDING CONFIDENCE: 95%.
CURRENT SLICE UNDERSTANDING CONFIDENCE: 95% for the bounded worklist, seed
provenance, address-map, blocker identity and deterministic serialization.
SLICE CONFIDENCE EVIDENCE: existing `re_slice_decoder`, `re_program`, Atlas,
candidate-map and prior bounded CFG audits; fresh raw Ghidra A/B export and
fresh candidate-map/mass pipeline rerun completed before implementation.
ACCEPTANCE: add small developer-only model/engine/report modules; use tiered
seeds and deterministic priority; recurse through direct calls/branches and
valid fallthrough; stop returns and record blockers; guard Atlas data; retain
multiple owners; emit deterministic map/frontier/metrics; gate one ROM-wide run
on the control corpus; add synthetic CI-safe tests and full local validation.
REUSE AUDIT: decoder/operand decode and flow edges come directly from
`re_slice_decoder`; Atlas and candidate-map provide data guards/evidence and
seed provenance; no decoder or CFG implementation is copied.
EVIDENCE: canonical USA ROM size 3145728, SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`; fresh raw
Ghidra export A/B size 390972, SHA-256
`613A3AA6DEB8D2DCF994C82ADC6A6939B7D5F27AF67A51D05C16F090D60A5315`; fresh
candidate-map SHA-256 `9ACD162CE078C2D31C108BA480F0306DA75D6B8FFDFF2D008A1DAE8253263A9B`
and mass JSON SHA-256 `6B9A6A366C0CEC047C1616A5840C8DA59D7AC89DD75021A0240F4F8E89D69C0D`.
KNOWN UNKNOWNS: indirect targets, Ghidra boundaries, data extents for
unbounded table starts and all routine semantics.
STATUS: EVIDENCE COMPLETE; implementation in progress.
EXACT NEXT ACTION: add the explorer model and guarded deterministic engine.

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
missing `<stdexcept>` declaration in the test; corrected follow-up run
33890964756 passed. WSL/Linux remains unavailable locally.
STATUS: DONE.
EXACT NEXT ACTION: hard stop at the M11.5 measurement checkpoint; restore the
raw Ghidra export before any independent rerun or systemic fix.

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

## 2026-09-12 — M12-GFX-MAX graphics root closure — IN PROGRESS

TASK: Continue the graphics evidence sweep after the 0x3820 caller closure.
Close the proven screen-resource root table, revalidate the complete known
screen descriptor/stream census, and publish a bounded fixed-point report for
the current graphics loader families. Do not add ROMs or decoded payloads, do
not repeat a whole-ROM detector sweep, and do not start M13 or ASM-to-C++
migration.

ACCEPTANCE CRITERIA: the root table has an exact boundary and consumer;
screen descriptors and decompressor streams are cross-checked against the
canonical ROM and current M12 manifest; every known 0x3820 caller remains
accounted for with its blocker; the candidate materialization is byte-exact;
the report records the remaining dynamic/non-screen blockers and validation
limits.

RESULT SO FAR: the 21-entry table `[0x00C92C,0x00C980)` is confirmed by the
`0x00C8F0` dispatcher and its 21 in-ROM longword roots, and is the only new
range promoted. The existing screen census contains 167 descriptor uses, 163
unique 26-byte descriptors, and 159 accepted compressed streams totaling
337,514 bytes; all are already SOURCE_OWNED. The 52-call 0x3820 census and
ten explicit dynamic blockers remain unchanged. Candidate SOURCE_OWNED is
1,475,346 bytes (46.8999862671%), +84 bytes from the M12-GFX-2 closure.

VALIDATION: helper compile/test passed; Debug and Release builds passed;
full Debug and Release CTest passed 143/143; the GNU/Linux-equivalent WSL
Release build and helper CTest passed; the candidate rebuilt ROM matches the
canonical SHA-256. The existing full-layout assembler attempt is recorded as
blocked by duplicate labels already present in the baseline layout; the
byte-exact baseline rebuilt ROM was used only as an explicit verification
fallback. Final CI remains pending for this change.

IMPLEMENTATION PUBLICATION: commit `a27175fe2269744d0497c3545d50bb59d0085849`;
GitHub Actions CI run `34651277101` passed. A documentation publication
commit and its final HEAD CI remain pending.

PUBLICATION: documentation commit
`dd5a23bde2ba936fc4536fb5048849b8da7c400f`; exact publication CI run
`34651453898` passed. The final report identity is now fixed; only the
post-publication documentation-check CI remains to be observed.

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
# 2026-09-10 — M12.0 ASM reconstruction roadmap rebase and exact census

TASK: Rebase the roadmap to complete reassemblable ASM before rebuilt-ROM
runtime parity and systematic ASM-to-C++ migration. Produce an exact
evidence-backed full-ROM census, executable blob inventory, dependency audit,
toolchain status and one M12.1 blocker. No production behavior changed.

BASELINE: 37c6bc1695771cb74839f48b05d40aa348ba7874; canonical USA ROM
3145728 bytes, CRC32 C4728225, SHA-1
2944910c07c02eace98c17d78d07bef7859d386a and SHA-256
eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263.

RESULT: M12_0_CENSUS_COMPLETE_ASM_COMPLETION_BLOCKED. The current M11.15
manifest has 203 exact 68000 ASM ranges totaling 13550 bytes and 136
canonical-local-ROM blobs totaling 3132178 bytes. It is contiguous with
0 gaps and 0 overlaps. No Z80 ASM, source-owned structured data,
header/vector ASM, padding map or accepted asset source exists. M11.17
separately accepts 10 bounded structured-data ranges totaling 1164 bytes,
but these remain blob-backed in the exact rebuild. Eight coarse blob ranges
contain observed execution evidence and total 22394 coarse bytes; 128 remain
P3 code/data/asset ambiguity.

DEPENDENCY: Current exact reconstruction still requires the original ROM at
build time to generate local blobs and verify identity. vasm 1.8g with
-m68000 -no-opt -Fbin is the current flat-binary assembler; no separate
linker is used. Ancient's historical assembler remains UNKNOWN.

M12.1_SELECTION: Exactly one target is selected:
0x06042A..0x0611F4, a 3530-byte P0 coarse blob with 76 unique observed PCs
and 77 static xrefs. It is selected by evidence concentration, not subsystem
attractiveness. M12.1 is not started.

FILES: PROJECT_STATE.md, TASK.md, docs/ROADMAP.md, docs/FILE_MAP.md,
docs/RE_LEDGER.md, docs/RE_METHOD_CATALOG.md, docs/DECISIONS.md and
docs/reports/ASM_COMPLETION_CENSUS_M12_0.md. No executable/source production
file changed and no ROM or asset was added to the repository.

VALIDATION: Existing M11.15/M11.17 evidence was inspected and the M11.15
audit was rerun against the 203-range manifest. It passed 203/203 exact ASM
slice round trips and rebuilt 3145728 bytes exactly with CRC32 C4728225,
SHA-1 2944910c07c02eace98c17d78d07bef7859d386a and canonical SHA-256.
Debug CTest passed 74/74; Release CTest passed 74/74 when run sequentially.
The source-size check passed in both matrices, git diff --check passed, and
tracked-artifact hygiene found no prohibited tracked ROM/build/payload file.
The initial concurrent CTest attempt produced one transient Release test
failure while both build directories were being exercised; the sequential
rerun passed and is the accepted result. The older re_full_split helper also
passed its M11.10 50-entry control, while the current 203-range result is
the M11.15 audit above.
## M12.1 — Transactional ASM promotion of P0 0x06042A..0x0611F4

TASK: promote only evidence-backed source-owned 68000 ASM intervals inside
the selected 3,530-byte P0 region, preserve exact canonical-ROM identity, and
leave all unresolved code/data boundaries blob-backed. No C++ runtime or
ASM-to-C++ migration is in scope.

ACCEPTANCE: baseline `b4937c8e1b99de7ca4f45cb20b3cdf37df51e932` and canonical
ROM identity must match before work; each promoted interval must assemble and
round-trip exactly; the full split must remain gap/overlap-free and hash-exact;
Debug/Release/GNU-equivalent validation, file limits, diff check and artifact
hygiene must pass; one M12.2 candidate must be selected without starting it.

The first bounded probe found that the coarse target is not one reassemblable
interval: existing M11 evidence has eight internal coverage gaps and the
decoder lacked exact IR for executable `MOVE SR` encodings at `0x06042A`,
`0x0611DC` and `0x0611E6`. The decoder/tooling change is limited to exact
normalization of `MOVE SR,<ea>` and `<ea>,SR`; no gameplay code is added.
# M12.2 — Transactional ASM promotion of P0 region `0x00DE00..0x00E338`

## Acceptance criteria

- Start from baseline `0dac30bcca103ae03a6372d24c2aca25e1fb6460` and operate
  only on the 1,336-byte target.
- Promote only evidence-backed exact ASM intervals, with explicit indirect
  exits, boundaries, classifications, and conservative unresolved blobs.
- Preserve exact bytes for every slice and the full canonical ROM, with no
  manifest gaps or overlaps.
- Add deterministic regressions, run Debug/Release/GNU-equivalent checks,
  source-limit/diff/hygiene checks, update the RE ledger/state/report, and
  stop after recomputing exactly one M12.3 proposal.

## Implementation and current result

The target's independently closed ASM islands are being materialized through
`src/tools/re_m12_2_promote.py`. The decoder now has exact `ADDX`, `MULU`, and
EOR/CMP direction handling required by the target; the range checker permits
exact indirect calls but remains fail-closed for indirect JMP dispatch. The
focused decoder/reassembler regression and the transactional full-ROM pass
have succeeded locally. Final configuration/build/CI results are recorded
below before commit.

## Final validation

- Target transaction `build/m12-2-transaction-g` passed all 13 slice round trips
  and the full 3,145,728-byte ROM round trip; exact hashes are in the report.
- Debug build and CTest: **76/76 passed**.
- Release build and CTest: **76/76 passed**.
- Fresh GNU-equivalent MinGW build completed; targeted CTest: **2/2 passed**.
- Independent evidence audit: **222/222** materialized ASM ranges round-trip
  exactly; 15 historical provenance mismatches are retained as an audit
  limitation and do not alter ownership.
- Static bounded report emitted 44 exact routines; `git diff --check` passed;
  source-size gate passed with the largest edited source at 500 lines; no
  tracked ROM, save, commercial asset or generated evidence was staged.
- Implementation commit and CI run SHA are appended after push.

## Published result

Implementation commit: `7cbcbf0b2c73606f6572849d5384a0890fd496a9`.
`origin/main` matched this SHA after push. GitHub Actions CI run
`34480825830` completed successfully for that commit (build and test green;
only the platform's Node.js 20 deprecation annotation was reported).
# 2026-09-10 — M12-AUTO2 consumer graph and bounded census — <90% / BLOCKED

**TASK:** Continue M12 toward a >=90% SOURCE-OWNED ROM MAP while preserving
the canonical ROM byte-for-byte and not starting M13, native gameplay/runtime
C++, or emulator expansion. Acceptance required exact ownership evidence,
zero manifest gaps/overlaps, full-ROM hash identity, and no promotion of
unknown bytes solely to increase the percentage.

**RESULT:** The exact M12-AUTO2 transaction is
`build/m12-auto2-consumer-transaction-d/materialized/manifest.json` with
718,252 / 3,145,728 source-owned bytes (`22.832616170%`). It adds 79,176
consumer-backed graphics bytes and 9,664 bytes of fixed parser records to the
previous Z80/screen checkpoint. The >=90% gate is not reached; 2,112,904 more
source-owned bytes are required.

**POSITIVE EVIDENCE:** The 68000 upload at `0x06134E` copies the exact
`0x62E38..0x64E38` Z80 image to `$A00000`; reset/signature checks and the
source representation are byte-exact. Direct `0x3820`/`0x37D2` consumers
close 29 deterministic graphics streams. The bounded 16-byte table at
`0x03B8DE`, selected by `0x03A9EE` and consumed through `0x03B1D0`, closes
seven more graphics streams. The `0x0012E8` copy routine closes eight
consecutive 1208-byte records at `0x200009..0x2025C9`; no save bytes after
`0x2025C9` were included. A census of `0x1AD000..0x1E7236` found 113 valid
streams, all already covered by the original 107-entry resource graph.

**EXACTNESS:** The transaction rebuilt 3,145,728 bytes with CRC32
`C4728225`, SHA-1 `2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

**VALIDATION:** Debug MinGW and Release MinGW full CTest each passed 83/83;
the GNU/Linux-equivalent build and five relevant M12/helper tests passed. GNU
full CTest reached the file-limit test but was stopped after its mounted-NTFS
scan remained active for roughly ten minutes; the same test passed in Debug
and Release. `git diff --check`, source-file limits, Python helper checks, and
the exact manifest/hash audit passed.

**STOP:** The remaining largest unknown ranges include
`0x064E38..0x141580` (902,984 bytes), `0x25FEC2..0x2E2FE6` (536,868
bytes), `0x03E7F4..0x060000` (137,228 bytes), and `0x1ED5EC..0x200009`
(76,317 bytes). Graphics census results without a closed table/consumer
contract were not promoted. No percentage gaming, guessed padding, unknown
asset classification, or ASM-to-C++ migration was performed.

**EVIDENCE:** The final report is
`docs/reports/ASM_AUTONOMOUS_PROVENANCE_TO_90_PERCENT_M12_AUTO2.md`.

**PUBLISHED:** Implementation commit `c388fc341443154d108e458d639054af8bd5a38c`
is pushed and matches `origin/main`. GitHub Actions CI run `34522064935`
completed successfully; build and test passed, with only the upstream Node.js
20 deprecation annotation.

# 2026-09-10 — M12-AUTO3 indexed script-table provenance — <90% / BLOCKED

**TASK:** Continue M12 toward a >=90% SOURCE-OWNED ROM MAP while preserving
the canonical ROM byte-for-byte and not starting M13, native gameplay/runtime
C++, or emulator expansion. Acceptance required a closed parser/table graph,
zero manifest gaps/overlaps, full-ROM hash identity, and no semantic text
claims or percentage-only promotion.

**RESULT:** The exact M12-AUTO3 transaction is
`build/m12-auto3-script-transaction-b/materialized/manifest.json` with
729,658 / 3,145,728 source-owned bytes (`23.195203145%`). It adds the 196-byte
16-bit table `[0x51514,0x515D8)` and 98 table-resolved NUL-terminated streams
totalling 11,210 bytes. The `0x00C2EC` caller enumerates `0x00..0x61` and
explicitly skips only `0x1B..0x1D`; the `0x00C326` parser computes the exact
entry-relative target and terminates at byte zero.

**EXACTNESS:** The transaction rebuilt 3,145,728 bytes with CRC32
`C4728225`, SHA-1 `2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

**VALIDATION:** The focused Python helper passed before transaction execution;
the transaction's full-ROM vasm round trip and independent manifest/hash audit
passed. Debug MinGW full CTest passed 84/84, Release MinGW full CTest passed
84/84, and the GNU/Linux-equivalent build plus four relevant M12 helper tests
passed. `git diff --check` and the source-file line-limit check passed.

**STOP:** The >=90% gate remains unmet; 2,101,498 additional bytes are
required. Unknown spans remain conservative blobs, including the 36-byte
unresolved area between the table and first stream and all other mixed
regions. No guessed padding, unknown asset classification, or ASM-to-C++
migration was performed. The publication gate was then satisfied by the
focused commit/push and its remote CI result below.

**PUBLISHED:** Implementation commit `fd775d1afe7df672dddc0d49dc3063eb5eb30cf6`
is pushed and matches `origin/main`. GitHub Actions CI run `34524552827`
completed successfully; build and test passed, with only the upstream Node.js
20 deprecation annotation.

# 2026-09-10 — M12-AUTO4 nested level-table provenance — <90% / BLOCKED

**TASK:** Continue M12 toward a >=90% SOURCE-OWNED ROM MAP while preserving
the canonical ROM byte-for-byte and not starting M13, native gameplay/runtime
C++, or emulator expansion. Acceptance required a closed nested table/group/
record graph, zero manifest gaps/overlaps, full-ROM hash identity, and no
promotion of the unindexed record-shaped gap.

**RESULT:** The new developer-only promoter
`src/tools/re_m12_level_table_promote.py` records outer table
`[0x5D918,0x5D958)`, contiguous count-bounded groups
`[0x5D958,0x5DB44)`, 186 non-zero inner edges, and 185 unique bounded records
through `0x5E1A0`. The transaction is
`build/m12-auto4-level-transaction-a/materialized/manifest.json` and reaches
731,827 / 3,145,728 source-owned bytes (`23.264153798%`), adding 2,169 bytes.
The 8-byte unindexed record-shaped span `0x5E0FE..0x5E106` remains UNKNOWN.

**EXACTNESS:** The rebuilt ROM is 3,145,728 bytes with CRC32 `C4728225`,
SHA-1 `2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

**VALIDATION:** Focused helper, Python compilation, promoter round trip, and
independent manifest/hash audit passed. Debug MinGW full CTest passed 85/85;
Release MinGW full CTest passed 85/85; the GNU/Linux-equivalent build linked
successfully and all 11 M12 helper tests passed. `git diff --check` and the
source-file line-limit check passed.

**STOP:** The >=90% gate remains unmet; 2,099,329 additional bytes are
required. No guessed padding, unknown asset classification, semantic text
claim, ROM mutation, or ASM-to-C++ migration was performed.

**PUBLISHED:** Implementation/docs commit `fa883c8770dbf37401ec76381e8181cf0ed733aa`
is pushed and matches `origin/main`. GitHub Actions CI run `34526971234`
completed successfully; build and test passed, with only the upstream Node.js
20 deprecation annotation.

# 2026-09-10 — M12-AUTO5 exact lookup-table provenance — <90% / BLOCKED

**TASK:** Continue M12 toward a >=90% SOURCE-OWNED ROM MAP while preserving
the canonical ROM byte-for-byte and not starting M13, native gameplay/runtime
C++, or emulator expansion. Acceptance required exact consumer-backed table
boundaries and no promotion of adjacent unclosed bytes.

**RESULT:** The developer-only promoter
`src/tools/re_m12_lookup_table_promote.py` closes the 64-entry word table
`[0x5D686,0x5D706)` through `0x00302E` and the 512-byte indexed byte table
`[0x5D706,0x5D906)` through consumers including `0x030200`'s `0x1FE` mask.
The transaction is
`build/m12-auto5-lookup-transaction-b/materialized/manifest.json` and reaches
732,467 / 3,145,728 source-owned bytes (`23.284498851%`), adding 640 bytes.

**EXACTNESS:** The rebuilt ROM is 3,145,728 bytes with CRC32 `C4728225`,
SHA-1 `2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

**VALIDATION:** Focused helper, Python compilation, promoter round trip, and
independent manifest/hash audit passed. Debug MinGW full CTest passed 86/86;
Release MinGW full CTest passed 86/86; the GNU/Linux-equivalent build linked
successfully and all 12 M12 helper tests passed. `git diff --check` and the
source-file line-limit check passed.

**STOP:** The >=90% gate remains unmet; 2,098,689 additional bytes are
required. No guessed padding, unknown asset classification, ROM mutation, or
ASM-to-C++ migration was performed.

**PUBLISHED:** Implementation/docs commit `123bee41977e14f9bc996a1e6b3a051ae49edb5b`
is pushed and matches `origin/main`. GitHub Actions CI run `34528572561`
completed successfully; build and test passed, with only the upstream Node.js
20 deprecation annotation.

# 2026-09-11 — M12-AUTO40 runtime-observed exact routine — <90% / CONTINUING

TASK: Continue the M12 >=90% SOURCE-OWNED ROM MAP with byte-exact canonical
ROM preservation and no M13/C++ migration.

ACCEPTANCE CRITERIA: Promote only a bounded routine supported by independent
runtime execution evidence, complete local decoding, an explicit terminator,
and an exact assembler round-trip; preserve all unrelated UNKNOWN spans.

RESULT: AUTO40 promotes `[0x00B79A,0x00B852)` (184 bytes) as
`RUNTIME_OBSERVED_EXACT_ASM_ROUNDTRIP`. Runtime evidence observes entry
`0x00B79A` and 52 instruction starts; all facts inside the interval are
`DECODED`. The routine terminates at `RTS` `0x00B850`. vasm output matches
the canonical ROM byte-for-byte. The transaction
`build/m12-auto40-runtime-code-promotion-b/materialized/manifest.json`
reaches 1,233,415 / 3,145,728 bytes (39.2092068990%); 1,597,741 bytes
remain to the integer 90% threshold.

EXACTNESS: Rebuilt ROM CRC32 `C4728225`, SHA-1
`2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

VALIDATION: AUTO40 helper test, Python compilation, promoter materialization,
full-ROM hash audit, and vasm slice round-trip passed. The existing Debug
`oasis_smoke` build remains blocked by pre-existing `src/core/ram_flag_routine.cpp`
errors (`std::to_string` and default `std::runtime_error` construction); no
C++ source was changed. Full Debug/Release gates and push remain pending.

STOP: The >=90% gate is not met. No guessed data, padding, or semantic
payload ownership was added; M13/C++ migration remains out of scope.
# 2026-09-11 — M12-AUTO41 runtime-observed exact routine range — <90% / CONTINUING

TASK: Continue the M12 >=90% SOURCE-OWNED ROM MAP with byte-exact canonical
ROM preservation and no M13/C++ migration.

ACCEPTANCE CRITERIA: Promote only a wholly UNKNOWN bounded range with a
runtime-observed entry, complete decoded instruction coverage, explicit
RTS boundaries, and an exact assembler round-trip.

RESULT: AUTO41 promotes `[0x03AE74,0x03B092)` (542 bytes) as
`RUNTIME_OBSERVED_EXACT_ASM_ROUNDTRIP`. Runtime evidence observes all 100
instruction starts in the interval and marks them `DECODED`; `0x03AE72` is
the preceding `RTS`, and `0x03B090` is the terminal `RTS`. vasm output
matches canonical ROM byte-for-byte. The transaction
`build/m12-auto41-runtime-code-promotion-a/materialized/manifest.json`
reaches 1,233,957 / 3,145,728 bytes (39.2264366150%); 1,597,199 bytes
remain to the integer 90% threshold.

EXACTNESS: Rebuilt ROM CRC32 `C4728225`, SHA-1
`2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

VALIDATION: AUTO41 helper test, Python compilation, promoter materialization,
full-ROM hash audit, and vasm slice round-trip passed. Debug/Release build
gates remain blocked by the pre-existing `src/core/ram_flag_routine.cpp`
errors; no C++ source was changed.

STOP: The >=90% gate is not met. No guessed data, padding, or semantic
payload ownership was added; M13/C++ migration remains out of scope.
# 2026-09-11 — M12-AUTO42 caller-backed runtime routine — <90% / CONTINUING

TASK: Continue the M12 >=90% SOURCE-OWNED ROM MAP with byte-exact canonical
ROM preservation and no M13/C++ migration.

ACCEPTANCE CRITERIA: Promote only a wholly UNKNOWN routine with exact static
callers, complete runtime decoding, closed conditional branches, an RTS
boundary, and an exact assembler round-trip.

RESULT: AUTO42 promotes `[0x03B358,0x03B486)` (302 bytes) as
`CALLER_BACKED_RUNTIME_EXACT_ASM_ROUNDTRIP`. Exact BSR callers are at
`0x03A900` and `0x03AA2A`; all 73 instruction starts are runtime-observed
and decoded, and the routine ends at `RTS` `0x03B484`. vasm output matches
canonical ROM byte-for-byte. The transaction
`build/m12-auto42-runtime-code-promotion-a/materialized/manifest.json`
reaches 1,234,259 / 3,145,728 bytes (39.2360369364%); 1,596,897 bytes
remain to the integer 90% threshold.

EXACTNESS: Rebuilt ROM CRC32 `C4728225`, SHA-1
`2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

VALIDATION: AUTO42 helper test, Python compilation, promoter materialization,
full-ROM hash audit, and vasm slice round-trip passed. Build gates remain
blocked by the pre-existing `src/core/ram_flag_routine.cpp` errors; no C++
source was changed.

STOP: The >=90% gate is not met. No guessed data, padding, or semantic
payload ownership was added; M13/C++ migration remains out of scope.
# 2026-09-11 — M12-AUTO43 small caller-backed runtime routine — <90% / CONTINUING

TASK: Continue the M12 >=90% SOURCE-OWNED ROM MAP with byte-exact canonical
ROM preservation and no M13/C++ migration.

ACCEPTANCE CRITERIA: Promote only a wholly UNKNOWN caller-backed routine with
an exact BSR caller, complete runtime decoding, RTS boundaries, and an exact
assembler round-trip.

RESULT: AUTO43 promotes `[0x0083F8,0x00846C)` (116 bytes). Caller `0x006230`
targets the entry immediately after `RTS` `0x0083F6`; all 33 instruction
starts are runtime-observed and decoded, ending at `RTS` `0x00846A`. vasm
output matches canonical ROM byte-for-byte. The transaction
`build/m12-auto43-runtime-code-promotion-a/materialized/manifest.json`
reaches 1,234,375 / 3,145,728 bytes (39.2397244771%); 1,596,781 bytes
remain to the integer 90% threshold.

EXACTNESS: Rebuilt ROM CRC32 `C4728225`, SHA-1
`2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

VALIDATION: AUTO43 helper test, Python compilation, promoter materialization,
full-ROM hash audit, and vasm slice round-trip passed. Build gates remain
blocked by the pre-existing `src/core/ram_flag_routine.cpp` errors; no C++
source was changed.

STOP: The >=90% gate is not met. No guessed data, padding, or semantic
payload ownership was added; M13/C++ migration remains out of scope.
# 2026-09-11 — M12-AUTO44 runtime record-region provenance — <90% / CONTINUING

TASK: Continue the M12 >=90% SOURCE-OWNED ROM MAP with byte-exact canonical
ROM preservation and no M13/C++ migration.

ACCEPTANCE CRITERIA: Promote only runtime-correlated regions whose reader
contract and count-plus-six-byte parser tiling close the exact boundaries;
exclude already-owned regions from the transaction count.

RESULT: AUTO44 promotes 27 previously UNKNOWN regions totaling 1,732 bytes.
Readers `0x00AF16` and `0x00ABB4` account for every promoted region; each
region is fully tiled by `2 + 6 * (count + 1)` bytes. The other 24 matching
regions totaling 1,346 bytes were already owned by AUTO11 and were not
double-counted. The transaction
`build/m12-auto44-runtime-record-regions-b/materialized/manifest.json`
reaches 1,236,107 / 3,145,728 bytes (39.2947832743%); 1,595,049 bytes
remain to the integer 90% threshold.

EXACTNESS: Rebuilt ROM CRC32 `C4728225`, SHA-1
`2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

VALIDATION: AUTO44 helper test, Python compilation, runtime correlation
validation, parser tiling, promoter materialization, and full-ROM hash audit
passed. Build gates remain blocked by the pre-existing
`src/core/ram_flag_routine.cpp` errors; no C++ source was changed.

STOP: The >=90% gate is not met. No guessed data, padding, or semantic
payload ownership was added; M13/C++ migration remains out of scope.
# 2026-09-11 — M12-AUTO54 PC-island tables — <90% / CONTINUING

TASK: Continue the M12 >=90% SOURCE-OWNED ROM MAP while preserving the
byte-exact canonical ROM and keeping C++ migration out of scope.

ACCEPTANCE CRITERIA: Promote only PC-relative tables/literals with exact
consumer bytes, fixed record/lookup shape, and an independently checked
half-open boundary. Keep mixed code/data islands and unclosed indexed regions
UNKNOWN.

RESULT: AUTO54 promotes 15 closed PC-relative island tables/literals totaling
376 bytes, including the 4x16-byte tables at `0x01953C`, `0x025C24`, and
`0x0288A2`, plus three repeated 6-byte status lookups. The map reaches
1,426,209 / 3,145,728 bytes (45.3379631042%). The deliberately retained
UNKNOWN candidates include `0x0108D0`, `0x01142C`, `0x0230A4`, `0x04BAE`,
and `0x061588` because their indexed extent or mixed code/data boundary is
not closed.

EXACTNESS: The transaction is
`build/m12-auto54-pc-island-tables-a/materialized/manifest.json`.
Canonical CRC32 is `C4728225`, SHA-1 is
`2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256 is
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

VALIDATION: The dedicated helper regression, Python compilation, exact
consumer/boundary contracts, manifest materialization, and full-ROM hash
check passed. No C++ migration began.

# 2026-09-11 — M12-AUTO53 PC-relative lookup family — <90% / CONTINUING

RESULT: AUTO53 promotes five fixed PC-relative lookup tables totaling 200
bytes. The exact consumer families close 4x16-byte, 20-word, and 4x8-byte
record shapes; the map reaches 1,425,833 bytes (45.3260103861%).
# 2026-09-11 — M12-AUTO57 bounded word-transform table — <90% / CONTINUING

TASK: continue the ASM-first M12 source-owned ROM map toward `>=90%` from the
AUTO56 canonical manifest; preserve byte-exact ROM and do not begin C++
migration. Acceptance: promote only a closed ROM interval with exact routine,
caller, stride/count, provenance and materialization evidence; rebuild the
canonical ROM byte-for-byte; update the M12 ledger and tests.

RESULT: AUTO57 promotes `[0x03BD86,0x03BE06)` (128 bytes) as
`BOUNDED_WORD_TRANSFORM_TABLE`. Exact decode of routine `0x03B7C0` shows one
big-endian word read per iteration, post-increment by two, and `DBF D7` back to
`0x03B7CC`. Direct callers at `0x03A7A6`/`0x03A854` and RAM-pointer callers at
`0x03A9A8`/`0x03AA50` establish the same base; the pointer initializer at
`0x03A87A` is exact. The largest caller sets `D7=0x3F`, closing 64 words and
the interval end at `0x03BE06`; the smaller `D7=0x0F` caller is an independent
bounded consumer. No neighboring UNKNOWN bytes were promoted.

SOURCE_OWNED_BYTES: `1,426,485` / `3,145,728` = `45.346736907958984%`.
The `>=90%` integer threshold is `2,831,156` bytes; remaining gap is
`1,404,671` bytes. AUTO57 materialized from
`build/m12-auto56-pc-word-overlap-a/materialized/manifest.json` into
`build/m12-auto57-bounded-word-transform-a/materialized/manifest.json`.

ROM identity remains exact: size `0x300000`, CRC32 `C4728225`, SHA1
`2944910c07c02eace98c17d78d07bef7859d386a`, SHA256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

# 2026-09-11 — M12 AUTO29-AUTO60 baseline CI input boundary

TASK: publish the complete local AUTO29..AUTO60 checkpoint before starting
M12-CARVER-1, with no ROM or generated evidence artifacts committed.

RESULT: checkpoint `57493a97cffad4d335ef64bcf186a5b84ea96bfa` was committed and
pushed. Its GitHub Actions run `34600345849` built successfully but failed nine
ROM-backed helper tests because CI correctly has no user-supplied
`build/reference/Beyond Oasis (USA).bin`. The failure was not a code or
canonical-ROM mismatch.

REPAIR: ROM/census-backed helper tests now skip explicitly when their local
evidence inputs are unavailable, while retaining full assertions when those
inputs exist. This preserves the no-copyrighted-ROM-in-CI rule and keeps the
pure split/contract checks deterministic.

VALIDATION: exact SHA `9c3587c704c719138847b78dbfdfe734bf18e3eb` passed GitHub
Actions CI run `34600737417` (Configure, Build, Test). The local canonical ROM
and generated census remain external and uncommitted.

VALIDATION: `python tests/re_m12_bounded_word_transform_test.py` passed;
`py_compile` passed; the full materializer passed and emitted the same ROM
hashes. No C++ source was changed and no C++ migration was started. Existing
Debug/Release project builds remain blocked by the pre-existing
`src/core/ram_flag_routine.cpp` `std::to_string`/`std::runtime_error` errors.

NEXT: continue with another exact bounded data/resource family; do not infer
ownership from raw decodability, runtime reads without a closed extent, or
neighboring bytes.
# 2026-09-11 — M12-AUTO59 bounded word-copy tables — <90% / CONTINUING

TASK: continue the ASM-first M12 source-owned ROM map from AUTO58 with exact
bounded data consumers; preserve byte-exact ROM and do not begin C++ migration.

RESULT: AUTO59 promotes `[0x000472,0x0004B4)` (66 bytes, 32 words) and
`[0x0031FC,0x003218)` (28 bytes, 13 words) as
`BOUNDED_WORD_COPY_TABLE`. Exact routine `0x002D66` consumes a two-byte header,
uses the first byte as the RAM destination offset, the second as `D7`, and
copies one word per `DBF D7` iteration. Callers at `0x0089C4` and `0x003260`
establish the two ROM bases. Each end is computed from its own header and loop
count; adjacent UNKNOWN bytes are excluded.

SOURCE_OWNED_BYTES: `1,426,715` / `3,145,728` = `45.35404841105143%`.
The integer `>=90%` threshold remains `2,831,156`; remaining gap is
`1,404,441` bytes. Materialization is in
`build/m12-auto59-bounded-word-copy-a/materialized/manifest.json`.

VALIDATION: helper test, `py_compile`, and full materialization passed; the
rebuilt ROM retains size `0x300000`, CRC32 `C4728225`, SHA1
`2944910c07c02eace98c17d78d07bef7859d386a`, and SHA256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

# 2026-09-11 — M12-AUTO58 exact static routine 0x003260 — <90% / CONTINUING

TASK: promote a bounded exact-decode ASM island left UNKNOWN by the previous
map, only after source round-trip and direct control-flow evidence.

RESULT: AUTO58 promotes `[0x003260,0x0032E8)` (136 bytes) as
`STATIC_EXACT_BOUNDED_ROUTINE`. The `0x003240` control-flow path reaches the
routine; its exact ASM has supported instructions throughout and an explicit
`RTS` at `0x0032E6`. The isolated vasm wrapper round-trips the slice exactly.
The inherited full layout still has pre-existing duplicate labels elsewhere,
so no green full-layout assembler claim is made for this checkpoint.

SOURCE_OWNED_BYTES after AUTO58: `1,426,621` / `3,145,728` =
`45.351060231526695%`; remaining gap to 90% was `1,404,535` bytes. No C++
source changed and no migration started.
# 2026-09-11 — M12-AUTO60 contiguous static islands — <90% / CONTINUING

TASK: run the systematic direct-caller-to-RTS static-island family against the
AUTO59 map; accept only exact range-tool + vasm slice round-trips wholly inside
UNKNOWN entries, preserving the canonical ROM and avoiding C++ migration.

RESULT: the audit examined 33 candidate runs. Fifteen exact slices passed both
the range tool and vasm byte comparison, totaling 1,158 bytes; 18 were
rejected with `RANGE_TOOL_FAIL`, `ASSEMBLER_FAIL`, or `SLICE_MISMATCH` and were
not promoted. AUTO60 promotes only the 15 passing intervals as
`STATIC_DIRECT_CALLER_RTS_ISLAND`, with per-slice caller and ASM/binary SHA256
provenance recorded in `build/m12-auto60-contiguous-islands-a/promotion_report.json`.

SOURCE_OWNED_BYTES: `1,427,873` / `3,145,728` = `45.39086023966471%`.
The integer `>=90%` threshold is `2,831,156` bytes; remaining gap is
`1,403,283` bytes. The materialized manifest is
`build/m12-auto60-contiguous-islands-a/materialized/manifest.json`.

VALIDATION: contiguous-island helper test, `py_compile`, audit range-tool +
vasm slice checks, and full materialization passed. Canonical ROM identity is
unchanged: CRC32 `C4728225`, SHA1
`2944910c07c02eace98c17d78d07bef7859d386a`, SHA256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

# 2026-09-11 — M12-CARVER-1 Stage 1 evidence/gap orchestration

TASK: after the pushed AUTO60 baseline gate, add the first minimal Thor ROM
Carver layer without replacing promoters or starting C++ migration. Acceptance
requires exact whole-ROM manifest coverage, unchanged confirmed ownership,
typed evidence/provenance/conflict storage, deterministic UNKNOWN-gap and
campaign reporting, adapters for existing evidence producers, regression tests,
and a Stage 1 stop report.

BASELINE: AUTO29..AUTO60 was committed and pushed at
`8fa79225584cf0fbac76356d81c0b1c9adacfb85`; exact GitHub Actions run
`34600920230` passed Configure, Build, and Test before Carver work began.

IMPLEMENTATION: added the developer-only `m12_carver.py` IntervalDB and
`m12_carver_adapters.py` existing-producer adapters, the `re_m12_carver.py`
CLI, and deterministic synthetic regression tests. Import is half-open and
coverage-preserving over `[0x000000,0x300000)`. Carver Stage 1 never changes
manifest classification, never creates SOURCE_OWNED bytes, requires confirmed
parents for candidate derivation, and treats conflicts as promotion blockers.

LOCAL RESULT: AUTO60 materialization imported 2,448 manifest ranges, including
758 UNKNOWN ranges and 1,690 confirmed ranges, with total bytes `3,145,728`,
zero coverage gaps/overlaps, and `1,427,873` SOURCE_OWNED bytes unchanged.
The existing evidence bundle produced 23,113 evidence records, 25,565 graph
nodes, 46,341 directed edges, and zero conflicts. The report ranked 758
deterministic campaigns; its recommended next campaign is the analysis-only
UNKNOWN interval `[0x0C0000,0x11F360)` (389,984 bytes). No detector expansion
or campaign promotion was started.

ROM identity remained exact: size `0x300000`, CRC32 `C4728225`, SHA1
`2944910c07c02eace98c17d78d07bef7859d386a`, SHA256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
Generated IntervalDB/report files remain ignored build outputs because they
reference the external user ROM and local evidence; no copyrighted ROM or
asset was committed. Stage 1 stops after the report.

FINAL CI: commit `9b43a8c4cfe2309a67ebe5222a8bb31bae2ba360` was pushed and its
exact GitHub Actions run `34602767835` passed Configure, Build, and Test.
The run reported only the hosted-action Node.js 20 deprecation annotation.

# 2026-09-11 — M12-CARVER-2 graph-guided evidence expansion

TASK: continue from the pushed M12-CARVER-1/AUTO60 checkpoint using Carver as
the orchestrator. Attempt provenance closure through the confirmed 99-row
field1 table and existing graphics census; do not replace promoters, create
SOURCE_OWNED bytes, start M13, or migrate ASM to C++.

IMPLEMENTATION: added `m12_carver_expansion.py` and its CLI wrapper. The pass
adds typed table pointer evidence, resource nodes, directed graph edges, and
five CANDIDATE raw-size hypotheses from confirmed table parents. It leaves the
manifest and all existing promoter transactions unchanged.

RESULT: 68 field1 pointers matched existing graphics-census starts; 47 were
already closed, 21 partially overlap confirmed data, and 28 had no decoder
boundary. Decoder-only hits are now CANDIDATE, yielding 3,871 graphics
candidates and 91 blocking boundary conflicts. The five raw-size hypotheses
total 17,408 bytes but lack an exact field6 raw-resource consumer/boundary
contract and were rejected. SOURCE_OWNED remains 1,427,873 bytes; fixed point
reached with no safe growth >=16 KiB.

The decoder-only classification correction is intentionally non-owning:
candidate spans wholly inside already confirmed ranges are not conflicts, while
candidate spans crossing a confirmed/UNKNOWN boundary remain explicit blocking
conflicts. The resulting graph contains 3,876 candidates and 91 conflicts.
The deterministic report is `docs/reports/THOR_ROM_CARVER_M12_STAGE2.md`.

VALIDATION: Carver tests, Python compilation, and `git diff --check` passed;
Debug/Release/GNU-equivalent build and test gates remain to be rerun for the
final commit. The canonical ROM hashes remain unchanged. Stage 2 stops at the
stable fixed point; no detector expansion, M13, or ASM-to-C++ work started.

# 2026-09-11 — M12-CARVER-3 global runtime-evidence sweep

TASK: perform one global evidence-acquisition and fixed-point pass over every
canonical-ROM-compatible deterministic runtime, census and checkpoint artifact
under the local evidence root. Do not inspect or promote UNKNOWN intervals
one-by-one; do not start M13 or ASM-to-C++ migration.

IMPLEMENTATION: added `m12_carver_global_sweep.py` and its CLI wrapper. The
tool discovers recognized existing producer schemas, validates ROM identity,
deduplicates identical payloads for IntervalDB ingestion, retains artifact
repetitions for scenario accounting, aggregates sequential/repeated runtime
observations into typed ROM spans, and emits whole-ROM status plus blocker
classification for every UNKNOWN. BizHawk and MAME binaries were checked and
are unavailable on the current host; no tool was installed and no external
capture was invented.

RESULT: the global input root contained 859 compatible artifacts and 784 unique
payloads, including 13 runtime scenario variants and 94 stored checkpoint
summaries. The normalized runtime set contains 25,037 observations and 16,367
spans covering 117,000 ROM bytes; 9,764 of those bytes intersect UNKNOWN
manifest ranges. No exact `src_before`/`src_after` decoder boundary recovery
was available in the stored captures. Carver reaches a fixed point with
1,427,873 SOURCE_OWNED bytes unchanged, 18,004 non-owning candidates rejected,
and 168 blocking conflicts remaining.

GLOBAL BLOCKER CENSUS: all 758 remaining UNKNOWN ranges are classified: B
runtime-observed with unknown consumer/parser (113 ranges, 623,036 bytes), F
candidate/conflict overlap (56 ranges, 58,789 bytes), or G static evidence with
no available scenario observation (589 ranges, 1,036,030 bytes). No source
range is promoted by runtime observation alone.

VALIDATION: global-sweep helper test, Python compilation, source-size review,
and `git diff --check` are required final gates. The canonical ROM identity is
unchanged; no ROM/assets, unknown `dc.b` ownership, M13 work, or ASM-to-C++
migration was added. The full report is
`docs/reports/THOR_ROM_CARVER_M12_GLOBAL_SWEEP.md`.

FINAL IMPLEMENTATION SHA: `5b59e88257b1283eaf51f49c3120a5c7765908f7`.
Exact GitHub Actions CI run `34619527956` passed Configure, Build and Test for
that SHA. The follow-up commit only records this CI result in documentation.

# 2026-09-11 — M12-CARVER-4 global static consumer recovery

TASK: continue from the pushed M12-CARVER-3 checkpoint and recover consumers,
selectors, parser contracts, container boundaries and code/data evidence across
the entire remaining UNKNOWN map using existing static artifacts only. Do not
repeat the runtime sweep, begin M13, migrate ASM to C++, or create ownership
from candidate/decoder/statistical evidence.

IMPLEMENTATION: added the developer-only deterministic static recovery pass and
CLI. It discovers 559 canonical-compatible non-runtime artifacts, parses all
available re-slice memory references/control-flow edges, table fields, static
consumer relationships, parser-boundary observations and candidate records,
then attaches typed evidence and graph edges to the existing Carver IntervalDB.
All 758 UNKNOWN ranges are clustered automatically into 136 consumer families
and receive a complete blocker record. The existing M12 promotion system remains
the only ownership writer.

RESULT: 24,172 typed references were recovered: 21,618 CODE_TO_ROM_RANGE,
259 CODE_TO_POINTER_TABLE, 405 TABLE_TO_ROM_RANGE and 1,890 CODE_XREF. There
were 2,116 contract observations and 2,725 static candidates. No exact UNKNOWN
boundary was recovered; no candidate or code byte was promoted. SOURCE_OWNED is
unchanged at 1,427,873 bytes. B/F/G remain 113/56/589 ranges (623,036 /
58,789 / 1,036,030 bytes), and the stored 168 blocking conflicts remain
explicit with zero resolved. Carver reached fixed point with an empty queue.

REPORT: `docs/reports/THOR_ROM_CARVER_M12_STATIC_CONSUMER_RECOVERY.md`.
Machine-readable output remains ignored build data under
`build/m12-carver-m12c4-static-recovery-e/`.

IMPLEMENTATION PUBLICATION: commit `a2c58294cc994a0097899ae8103493bf4bad2aac`
was pushed to `origin/main`. Exact GitHub Actions CI run `34633366873` passed
Build and Test. The final docs-only publication SHA and its exact CI run remain
to be recorded after this documentation update.

# 2026-09-11 — M12-CARVER-5 whole-ROM format/container reconstruction

TASK: perform one global, bottom-up structural reconstruction pass over every
remaining UNKNOWN range using the post-Carver-4 manifest and recorded evidence.
Do not repeat runtime/static sweeps, inspect gaps one-by-one, start M13, migrate
ASM to C++, or claim ownership from decoder/statistical evidence.

IMPLEMENTATION: added the developer-only deterministic format reconstruction
core and CLI. It fingerprints all 758 UNKNOWN ranges, computes bounded global
container/table candidates, performs duplicate-family comparison, scans the
existing BO graphics grammar at every even candidate start, and attaches typed
non-owning evidence to the existing IntervalDB. Candidate IDs include the full
structural identity so alternate endian views do not create false conflicts.
The current exact M12 promoters and manifest writer were not replaced.

RESULT: 858,249 starts were checked; 1,454 valid BO grammar hits were found and
722 deterministic spans retained. Structural candidates include 31 count/stride,
32 monotonic pointer-family, 48 sentinel, and 182 uniform-padding observations;
128 exact-duplicate provenance observations were recorded. None closed the
required reusable grammar plus exact representation plus code/semantic
exclusion contract. SOURCE_OWNED remains exactly 1,427,873 bytes; UNKNOWN
remains 758 ranges / 1,717,855 bytes; B/F/G remain 113/56/589 ranges and
623,036/58,789/1,036,030 bytes. Carver fixed point is reached with 4,494
evidence records, 7,083 nodes, 10,148 edges, and zero conflicts.

REPORT: `docs/reports/THOR_ROM_CARVER_M12_FORMAT_CONTAINER_RECONSTRUCTION.md`.
Ignored machine output is under
`build/m12-carver-m12c5-format-reconstruction-c/`. Canonical ROM SHA-256 is
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

VALIDATION: helper tests, Python compilation, local build/test/source-limit,
`git diff --check`, and final SHA CI status must be recorded before publication.
The known unrelated MSVC Debug issue in `src/core/ram_flag_routine.cpp` remains
outside this task. M12-CARVER-5 stops at the global structural fixed point.

IMPLEMENTATION PUBLICATION: commit `dcb1218` was pushed to `origin/main`.
Exact GitHub Actions CI run `34637435506` passed Configure, Build and Test.
The report records this exact implementation-SHA result; any later
documentation-only publication does not alter the implementation or Carver
hashes.
# 2026-09-11 — M12-GFX-1 whole-ROM graphics decompiler sweep — COMPLETE / GLOBAL NEGATIVE

RESULT: The independent Ancient parser validated the canonical vector at
`0x16943C` (`1217` compressed bytes, `3072` output bytes,
`65e99e74020fedbdcb97c8249a5ccfe540aca5bb5d29bfb260352cd6f388c31a`) and
the malformed-stream safety tests. The byte-offset sweep tested `3,145,725`
starts, `3,020,239` viable headers, and retained `7,577` strict-valid streams
with `1,409,840` compressed and `4,411,088` decompressed bytes across unique
records. It found `52` static absolute callers of `0x3820`.

CLASSIFICATION: Strict candidates yielded 417 `GFX_TILE_CANDIDATE`, 1,192
`PALETTE_CANDIDATE`, 1,931 `TILEMAP_CANDIDATE`, and 4,037
`COMPRESSED_GENERIC` records. The raw tile, palette, and tilemap scans remain
secondary heuristic evidence; no PNG or extracted asset was written.

PROMOTION: `6,836` strict streams are wholly inside baseline UNKNOWN/BLOB
ranges and were rejected because this pass has no independently closed
consumer/container contract. `699` are already owned and `42` overlap a
baseline-owned/unknown boundary. Promoted spans/bytes are `0`; SOURCE_OWNED
remains `1,427,873 / 3,145,728 = 45.3908602397%`. Existing blocker totals B/F/G
remain `623,036 / 58,789 / 1,036,030` bytes (113/56/589 ranges) before and
after.

PROVENANCE: Existing runtime artifacts confirm decoder PCs `0x3820`/`0x3830`
and reader correlation over `0x152340..0x211F78` (75,969 unique ROM bytes),
but no fresh graphics hook was available. The prior exact ID3 static control
remains the only retained ROM→RAM→DMA/VRAM chain in this pass:
`0x1AE1A8..0x1AE8AA` → `0xFF2FA8..0xFF3FA8` → VRAM `0x4000..0x4FFF`.
CRAM line/source and SAT captures are unavailable; none were fabricated.

IDENTITY: canonical CRC32/SHA-1/SHA-256 are `C4728225`,
`2944910c07c02eace98c17d78d07bef7859d386a`, and
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
Local JSON output is `build/m12-gfx1-whole-rom/whole_rom_report.json` with
SHA-256 `fdd0f9e2ded5a8fd000193b7ddfda1bf9873d3d4c086fe1b203da91213ba6fc1`.
The full report is `docs/reports/THOR_M12_GRAPHICS_DECOMPILER_SWEEP.md`.

VALIDATION: Python compilation and parser tests passed; Debug and Release
MinGW graphics targets/reference checks passed; WSL Ubuntu GNU/Linux-equivalent
graphics build, CTest, and reference checks passed; `git diff --check` and the
source-size check passed. Visual Studio and Ninja were unavailable on this
host. Implementation SHA `06db09a741b9aae465cb9a6a8ea09604615c1fb3` exact
final-SHA CI `34643596746` passed Build/Test/Complete; no M13 or ASM-to-C++
migration started.

# 2026-09-11 — M12-GFX-1 whole-ROM graphics decompiler sweep — IN PROGRESS

TASK: Run one graphics-focused whole-ROM reverse-engineering campaign from
the `origin/main` baseline `afa3d1fc48386cfc710b32e22911d04b2c7eb6d0`,
covering independent Ancient-format validation, strict global stream census,
graphics heuristics, static `0x3820` caller enumeration, and available local
runtime/provenance inputs. Preserve the canonical ROM and keep all tooling
developer-only; no M13, ASM-to-C++ migration, ROM/assets, or visual-identity
ownership claims are in scope.

ACCEPTANCE CRITERIA: the independent parser must validate the known
`0x16943C` vector, reject malformed streams safely, scan every ROM offset
without decoder-only promotion, emit deterministic hashes and exact stream
metrics, classify tile/palette/tilemap evidence as candidates, enumerate
static/runtime evidence, and publish
`docs/reports/THOR_M12_GRAPHICS_DECOMPILER_SWEEP.md` with explicit unavailable
runtime layers and SOURCE_OWNED before/after accounting.

# 2026-09-12 — M12-GFX-MAX bounded 0x00D650 continuation audit — IN PROGRESS

TASK: Continue the graphics closure inside the shared `0x00D406` loader by
closing the register relation around its second `0x3820` call. Preserve the
canonical ROM, developer-only scope, and the no-M13/no-ASM-to-C++ boundary;
do not promote a stream from register propagation alone.

RESULT: The exact body copies the first `0x3820` post-source/post-destination
`A0/A1` into `A4/A5` at `0x00D550/0x00D552`. If bit 2 of `0x00FF16F1` is set,
`0x00D64C/0x00D64E` pass those post-states to the second call at `0x00D650`,
and `0x00D656/0x00D658` capture its post-state. The second call is therefore
proven to be a sequential continuation, not an independent selector. The
first source's ROM origin and exact continuation boundary remain unproven;
no bytes, descriptors, streams, or code ranges changed ownership.

IMPLEMENTATION: narrowed the developer-only dynamic ledger, added a
regression assertion, and updated the M12-GFX-2, M12-GFX-MAX, and reverse-
engineering records. The blocker is now
`FIRST_STREAM_AND_CONTINUATION_NOT_ROM_PROVEN` rather than a generic
register-value description.

NEXT: continue the remaining screen/descriptor blockers, then the other
dynamic `0x3820` producers; a caller-closed source path or targeted trace
capturing the first `A0` and the post-state boundary is required before
promotion.

PUBLICATION: focused commit
`6a773cae39dffefac82bdcff5b357a7122f032ba` is on `origin/main`; exact
GitHub Actions CI run `34660611094` passed Build, Test, and Complete.

# 2026-09-12 — M12-GFX-MAX bounded 0x02DB52 post-source audit — IN PROGRESS

TASK: Continue the dynamic `0x3820` closure by tracing the exact local
producer immediately before `0x02DB52`; preserve byte-exact ROM and do not
promote a RAM post-state without its source and boundary proof.

RESULT: The closed slice `0x02DB3C..0x02DBB6` calls `0x00D406` at `0x02DB40`,
loads `A0` from `0x00FF17AA` at `0x02DB46`, and sets `A1 = 0x00FF2FA8` before
`0x02DB52`. The shared loader writes `0x00FF17AA` from post-source `A4` at
`0x00D65A`, so this is a `0x00D406` post-source continuation, not an
independent caller argument. The incoming `D0` and exact stream boundary
remain unproven; no ownership changed.

IMPLEMENTATION: narrowed the dynamic ledger, added a regression assertion,
and updated the M12-GFX-2, M12-GFX-MAX, and reverse-engineering records. The
blocker is now `D406_POST_SOURCE_NOT_ROM_PROVEN`.

NEXT: continue with `0x02F6A0` and the remaining dynamic producers; only a
closed source path or targeted register trace can promote this continuation.

PUBLICATION: focused commit
`04ad9eb7cf110d408c51ea7a23a8693d9741b15a` is on `origin/main`; exact
GitHub Actions CI run `34661095122` passed Build, Test, and Complete.

# 2026-09-12 — M12-GFX-MAX bounded 0x02F6A0 post-state audit — IN PROGRESS

TASK: Continue the dynamic `0x3820` closure by tracing the exact predecessor
of `0x02F6A0`; preserve byte-exact ROM and do not promote a RAM post-state
without source and boundary proof.

RESULT: The local slice begins with `0x00D406` at `0x02F67C`, then loads
`A0/A1` from `0x00FF17AA/0x00FF17AE` at `0x02F692/0x02F698` before calling
`0x003820` at `0x02F6A0`. The shared loader writes those fields from its
post-source/post-destination `A4/A5` at `0x00D65A/0x00D660`. This is a
`D406` post-state continuation, not an independent caller argument. The
incoming `D406` source and exact boundary remain unproven; ownership changed
nowhere.

IMPLEMENTATION: narrowed the dynamic ledger, added a regression assertion,
and updated the M12-GFX-2, M12-GFX-MAX, and reverse-engineering records. The
blocker is now `D406_POST_STATE_NOT_ROM_PROVEN`.

NEXT: continue with the `0x03B1D0` family and remaining dynamic producers;
only a closed source path or targeted register trace can promote this chain.

# 2026-09-12 — M12-GFX-MAX bounded 0x03B1D0 source-arm audit — IN PROGRESS

TASK: Close the interprocedural relation around the three `0x03B1D0` family
calls without treating the shared RAM destination as a ROM source.

RESULT: The exact family uses one output buffer, `A1 = 0x00FF316C`, with
three source arms: `A0 = 4(A5)` at `0x03B236`, `A0 = A3` at `0x03B28A`, and
`A0 = A4` at `0x03B2FE`. The sibling helpers before the third arm do not write
`A4`: `0x002CBC` writes `A5`/data registers, while `0x00D950` preserves data
registers/`A2` and its `0x00D962` body does not write `A4`. The three source
values remain inherited and not ROM-proven; no ownership changed.

IMPLEMENTATION: narrowed the dynamic ledger, added three regression
assertions, and updated the M12-GFX-2, M12-GFX-MAX, and reverse-engineering
records. Blockers are now `A5_FIELD_NOT_ROM_PROVEN`,
`A3_ARGUMENT_NOT_ROM_PROVEN`, and `A4_ARGUMENT_NOT_ROM_PROVEN`.

NEXT: continue with the remaining dynamic producers, beginning with
`0x03C07C`; only a closed source path or targeted register trace can promote
these arms.

PUBLICATION: focused commit
`7b638801d1e7b6f0abc721219da989b8d48e5e00` is on `origin/main`; exact
GitHub Actions CI run `34661832137` passed Build, Test, and Complete.

# 2026-09-12 — M12-GFX-MAX bounded 0x00D54A provenance audit — IN PROGRESS

TASK: Continue the graphics closure by narrowing the first unresolved dynamic
`0x3820` producer without promoting RAM-mediated data. Inspect only the exact
`0x00F80E` helper and the shared `0x00D406` path; preserve the canonical ROM,
developer-only scope, and the no-M13/no-ASM-to-C++ boundary.

RESULT: The helper at `0x00F80E` saves/restores all address registers and
therefore cannot produce `A4`. In the shared loader, `0x00D42E` sets
`A4 = entry A1 + 4`, then `0x00D542` loads `A0 = (A4)` before `0x00D54A`.
The source relation is now narrowed to a longword in the inherited entry
record/RAM field. The source is still not proven ROM-originated, so no bytes,
descriptor, stream, or code range changed ownership.

IMPLEMENTATION: updated the developer-only dynamic ledger, its regression
assertion, the M12-GFX-2 caller report, the M12-GFX-MAX report, and the
reverse-engineering record. The blocker is now named
`INHERITED_A1_FIELD_NOT_ROM_PROVEN` rather than helper output.

NEXT: continue with the remaining screen/descriptor blockers, then the other
dynamic `0x3820` producers; a targeted trace capturing `A1`/`A0` at the call or
a caller-closed source path is required before promotion.

PUBLICATION: focused commit
`398be47fde617efbd847398cafd5e884eebeddd3` is on `origin/main`; exact
GitHub Actions CI run `34659553539` passed Build, Test, and Complete.

# 2026-09-12 — M12-GFX-MAX bounded 0x03C07C source audit — IN PROGRESS

TASK: Close the exact `0x03C07C` dynamic producer far enough to distinguish
its ROM source from its inherited destination, without promoting a partial
decompression contract.

RESULT: The `0x03BF86` initialization path reaches `0x03C074` after the
`0x03C956` helper. The helper does not write `A1`; `0x03C074` loads
`A0 = 0x00172168` directly from ROM, copies inherited `A1` to `A3`, and calls
`0x003820`. The parent preserves all address registers. The blocker is now
`A1_INHERITED_NOT_ROM_PROVEN`; no source, destination, or resource bytes were
promoted.

IMPLEMENTATION: narrowed the developer-only dynamic ledger, added regression
assertions, and updated the M12-GFX-2, M12-GFX-MAX, and reverse-engineering
records with the exact static path.

NEXT: continue the remaining dynamic producers; a closed `A1` source path or
targeted register trace is required before any resource promotion.

PUBLICATION: focused commit
`a016de2d42f45c9b4b93487b39bb0bcfd84c2816` is on `origin/main`; exact
GitHub Actions CI run `34662243757` passed Build, Test, and Complete.

# 2026-09-12 — M12-GFX-MAX final dynamic static-site audit — IN PROGRESS

TASK: Check the final two dynamic producers, `0x03D5AE` and `0x03E61A`, for a
closed local source relation and record negative evidence where none exists.

RESULT: `0x03D5AE` starts with `JSR 0x003820`, defines neither `A0` nor `A1`
before the call, and then writes only entity-state RAM at `0x00FF19A2`,
`0x00FF19A6`, `0x00FF19AA`, and `0x00FF19AC`. `0x03E61A` also calls before
new `A0`/`A1` definitions; only its later `0x03E662` and `0x03E704` direct
arms are already closed at `0x0016943C -> 0x00FF2FA8`. No ledger
classification or ownership changed.

CONCLUSION: Existing exact static slices are exhausted for all ten dynamic
producers. Further progress requires a new caller-closed source path or a
targeted runtime register capture; no heuristic or decoder-validity-only
promotion is justified.

NEXT: close the two remaining screen sites and five descriptor-shaped records,
then revisit dynamic producers only with new evidence.

PUBLICATION: focused commit
`8b9ed0fbc07a4a7eec9e7241f770164c4e835a02` is on `origin/main`; exact
GitHub Actions CI run `34662639911` passed Build, Test, and Complete.
# 2026-09-12 — M12-GFX-MAX extended D406 closure and descriptor candidate — IN PROGRESS

TASK: Continue the active M12-GFX-MAX graphics closure after the bounded
0x00D406 census. Close the two long screen continuations with exact
instruction-level CFG evidence and independently validate the remaining
descriptor-shaped candidate. Keep continuation proof non-owning, preserve the
canonical ROM, and do not start M13 or ASM-to-C++ migration.

RESULT: The exact developer-only `re_slice_decoder` CFG reports close
`0x038FD6 -> 0x0390A4` and `0x03959A -> 0x0395D8`; every reachable path reaches
the exact `JSR abs.l,0x00D406`, no return or unsupported instruction occurs
before the target, and the reviewed paths write no `A1`. The screen census is
now 17/17 verified continuations with zero unresolved screen sites. The five
descriptor-shaped records are accounted for: four were already owned, and
`0x02E1D8..0x02E1EE` is promoted as a 22-byte repeated-family descriptor whose
`+4` pointer selects the exact already-owned stream `0x2119D2..0x211F79`
(18,712 decompressed bytes). The four-byte gap before the D406 call remains
unpromoted.

ARTIFACTS: ignored `build/m12-gfx-loader-census-extended.json`, SHA-256
`55B7A55F6C67017996C859350E2A18841BAC8C7D9A84BF6128B965A84DAED479`; ignored
`build/m12-gfxmax-screen-descriptor-candidate-a/promotion_report.json`, SHA-256
`5C45C2873DC1A8850D8E1331C5D9E0A729EE3E9D74FD75C6F420ECAFC444155D`.

OWNERSHIP: candidate manifest SOURCE_OWNED is 1,475,368 bytes / 46.9006856283%;
UNKNOWN is 1,670,360 bytes. The materialized rebuilt ROM remains byte-exact:
CRC32 `C4728225`, SHA-1 `2944910c07c02eace98c17d78d07bef7859d386a`, SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

VALIDATION: Python compilation; focused helper tests (`6 passed`); deterministic
extended census; candidate promotion/materialization; and byte-for-byte ROM
identity checks passed. Existing project Debug/Release and Linux validation
status remains as recorded in the M12-GFX-MAX report; no C++ or ROM files were
changed.

NEXT: obtain new caller-closed or targeted register evidence for the ten
dynamic `0x3820` producers. Existing exact static slices are exhausted for
those ten; no ownership promotion without a proven ROM source and exact
boundary.
# 2026-09-12 — M12 emulator sprite-system reconstruction — IN PROGRESS

TASK: Recover the actual object/animation/frame/sprite path with bounded
emulator evidence, beginning by measuring the installed BizHawk observables
and adding a developer-only hardware-sprite reconstruction contract. Keep the
canonical ROM external and byte-exact; do not start M13, ASM-to-C++ migration,
manual full-game playthrough, or a whole-ROM graphics sweep.

ACCEPTANCE CRITERIA: produce a machine-readable capability report from the
installed BizHawk environment; record which register, ROM/RAM, callback,
memory-domain, save-state, and VDP-related observables are actually exposed;
add deterministic synthetic tests for Genesis SAT decoding, 4bpp tile
decoding, CRAM palette decoding, flips, bounded piece composition, malformed
input rejection, and provenance identity; keep all emulator and ROM-specific
logic under `src/tools`; update the M12 report with PROVEN/OBSERVED/INFERRED/
CANDIDATE/BLOCKED classifications and exact validation results.

NON-GOALS: no production gameplay/rendering change, no ROM/assets/decoded
copyrighted images in Git, no ownership promotion from visual similarity or
decoder validity, and no claim that the complete object table is recovered
until a running consumer relationship and finite enumeration are proven.

RESULT: The previously working BizHawk installation was recovered at
`C:\Dev\SegaThorTools\BizHawk-2.11.1-win-x64\EmuHawk.exe`, with working
directory equal to that installation directory. The exact existing M12-GFX
workflow was replayed once with the natural scenario and 1800 frames: exit 0,
capture SHA-256 `D61150BD828DB22CA35A2E6C93A103BDC04ABE667883B94339FD05390924000B`,
byte-identical to both published captures. The canonical ROM was present and
matched the recorded 3,145,728-byte identity.

LIVE EVIDENCE: BizHawk 2.11.1 exposed `M68K BUS`, `VRAM`, `CRAM`, `VSRAM`, and
`MD CART`, plus register, execution/read/write callback, and frame APIs. The
developer-only SAT probe observed VDP register 5 at PC `0x2AF4` selecting SAT
base `0xD000`; 512 bounded DMA launches from RAM `0xFF13CC` to that base were
issued by PC `0x27EC`. The bounded sprite-context probe then observed the
exact `0x3B416..0x3B448` selector/table path: table base `0x3B8DE`, selector
RAM `0xFFAFAE`, selected table-field pointer `0x3B95C` observed at `0x3B426`
(`MOVEA.L (A0),A0`), and four ROM-backed source starts passed at `0x3B448`
to `0xB730`. `0xB730` stores the SAT source record
at `0xB752`, `0xB764`, `0xB76E`, and `0xB77A`; the BizHawk write callback's
`0xB754` PC is the post-store callback for the first store. 43/43 same-frame
DMA payload/SAT snapshot comparisons matched byte-for-byte; observed linked
chains reached eight entries. No emulator state writes occurred.

LIMITATION: An unrestricted or PC-filtered BizHawk `on_bus_read` backward trace
was too expensive in this environment and was stopped before producing an
artifact. The exact bounded selector/table/record edge is now observed, but
the semantic object root, animation grammar, complete enumeration, and all
frame/resource linkage remain unclaimed. No SOURCE_OWNED count changed; the
checkpoint remains 1,475,368 / 3,145,728 (46.9006856283%).

ARTIFACTS: `docs/reports/THOR_M12_BIZHAWK_CAPABILITY_REPORT.json`,
`docs/reports/THOR_M12_EMULATOR_SPRITE_TABLE_RECONSTRUCTION.md`,
`src/tools/re_bizhawk_m12_sat_provenance.lua`,
`src/tools/re_bizhawk_m12_sprite_context.lua`, and the ignored runtime
captures under `build/m12-gfx-runtime/`. The synthetic reconstruction test
passed.

VALIDATION: MinGW Debug and Release configure/build passed; Debug and Release
CTest each passed 153/153, including the source file-limit check. Python
synthetic tests, Python compilation, report/runtime consistency, and
`git diff --check` passed. A GNU/Linux-equivalent check was not available:
WSL was callable, but its shell exposed no `cmake`, `g++`, or `ninja`; no Linux
claim is made.

NEXT: use the now-closed `0x3B8DE`/`0xB730` edge to enumerate only the proven
record count/stride and follow its runtime consumers toward animation/object
roots; keep semantic assignment and SOURCE_OWNED promotion fail-closed until
the complete graph is independently bounded. Keep all such work
developer-only.

# 2026-09-12 — M12 bounded live-context catalog — IN PROGRESS

TASK: Turn the proven BizHawk selector/table/producer observation into a
repeatable, payload-free catalog. Do not infer semantic object, animation, or
frame names from addresses alone and do not modify SOURCE_OWNED.

RESULT: Added `src/tools/m12_sprite_context_catalog.py`. Against the canonical
ROM and `sprite-context-current-v5.json` it validates the ROM hash and
read-only capture, enumerates the exact eight `0x10`-byte records at
`0x03B8DE..0x03B95E`, and requires an ordered bounded execution edge through
`0x3B41A`, `0x3B422`, `0x3B426`, `0x3B448`, `0xB730`, and all four SAT stores.
The current capture closes that edge in 12 frames and records four observed
ROM-backed source starts. The output contains addresses and provenance only;
no ROM payload, graphics bytes, or decoded asset is emitted. Record semantics
and complete object/animation/frame enumeration remain `UNRESOLVED`.

ARTIFACT: ignored runtime catalog
`build/m12-gfx-runtime/sprite-context-catalog-current.json`, SHA-256
`0EC167EF498537D50796F9F4E1BA92D287BF4A7D0640728B3FE854C01AFA7F0C`.

VALIDATION: focused catalog tests passed; Python compilation passed. Debug and
Release CTest must be rerun after the new CTest registration. No C++,
production runtime, ROM, asset, or ownership manifest change was made.

NEXT: use the bounded catalog as the invariant for the next targeted live
trace; recover an independently proven frame/animation consumer before any
semantic naming or full enumeration claim.

# 2026-09-12 — M12 exact-address ROM-read provenance — IN PROGRESS

TASK: Strengthen the bounded selector/table/producer chain with direct ROM
read events without repeating the prohibitively expensive global BizHawk
`on_bus_read` trace.

RESULT: Added `src/tools/re_bizhawk_m12_targeted_reads.lua`, watching only the
confirmed table-field/pointer and observed source-start addresses. BizHawk
2.11.1 completed a 730-frame replay with 270 read events and no state writes.
The capture directly records `0x3B8EA -> 0x3B95C` 77 times, then
`0x3B95C -> 0x00171832` 77 times, and reads all four observed source starts
(`0x171864`, `0x17186C`, `0x171874`, `0x17187C`) in the `0xB730` body context.
The payload-free catalog now validates these events in addition to the ordered
execution edge and the exact eight-record table geometry.

ARTIFACT: ignored local capture
`build/m12-gfx-runtime/targeted-reads-current.json`, SHA-256
`53D36B893E6CB22D8CE86BDC9329B157591384D4CB8B68177F2DB6DAD1F6DB65`.

LIMITATION: This closes direct bounded ROM reads, not semantic pointer meaning
or the complete object/animation/frame graph. No semantic name, asset
extraction, SOURCE_OWNED promotion, C++, or M13 work was introduced.

VALIDATION: focused catalog tests, Python compilation, catalog generation, and
capture/report hash consistency passed. The prior full Debug/Release CTest
result was `154/154`; the new Lua probe is developer-only and does not alter
build targets beyond the documented source map.

NEXT: use the exact read-PC evidence to target the next frame-descriptor or
animation consumer, retaining the same bounded-address strategy and refusing
full enumeration until a finite grammar is independently proven.

# 2026-09-12 — M12 1800-frame targeted-read expansion — IN PROGRESS

TASK: Extend the exact-address ROM-read probe across the already accepted
1800-frame deterministic scenario, using only statically enumerated table
fields/pointers and the previously observed source window.

RESULT: `re_bizhawk_m12_targeted_reads.lua` completed with exit 0 and 2,836
bounded read events. Runtime reads now cover table fields for records 0, 1,
and 2: `0x3B8EA -> 0x3B95C` (734 events), `0x3B8FA -> 0x3B998` (1), and
`0x3B90A -> 0x3B982` (474). Pointer reads directly reach `0x171832` (689)
and `0x1742DC` (469). The four source starts observed in the prior context
capture remain ROM-read in the `0xB730` body context. The payload-free catalog
now reports observed table-field edges by record index while leaving all
semantic names unresolved.

ARTIFACT: ignored local capture
`build/m12-gfx-runtime/targeted-reads-current-1800-v2.json`, SHA-256
`D519BC1373D9F5BF5C0E45F115E86146C9D97FB48E6FD97AE7C0EDA30E2CACF5`; catalog
SHA-256 `91B526D27411D574730F9E4AF56458B6AA2820D504A54EDECA7AADEF90FED0F3`.

LIMITATION: Records 3 through 7 have not been observed on this scenario, and
no pointer value is promoted to an object, animation, frame, or sprite name.
The full graph and all-frame enumeration remain `UNRESOLVED`; no SOURCE_OWNED
change was made.

NEXT: use the newly observed record-2 pointer/read edge to add only its exact
consumer PCs or bounded descriptor words, then seek an independently proven
finite frame grammar.

# 2026-09-12 — M12 bounded ROM source-start census — IN PROGRESS

TASK: Re-run the exact-address probe with only the previously observed
`0xB730` source starts and their four statically evidenced words, extending the
live census without a range scan or manual gameplay search.

RESULT: The deterministic 1800-frame replay completed with exit 0. The probe
retained 4,096 bounded read events and observed 24 distinct first-word source
starts from `0x171864` through `0x171A54` (including repeated starts at
`0x171A04`, `0x171A14`, and `0x171A1C`), with corresponding `+2/+4/+6` reads
at the `0xB74C`, `0xB76C`, and `0xB776` callback PCs where retained. Table
fields for records 0, 1, and 2 and pointer targets `0x171832`/`0x1742DC`
remain directly observed. The payload-free catalog now reports 24 observed
source starts and marks the event-cap boundary.

ARTIFACT: ignored local capture
`build/m12-gfx-runtime/targeted-reads-current-1800-v3.json`, SHA-256
`C48705F8900A489A97930C398CBD903FF1B02C9B6E75B68764552224D0FFCD8A`; catalog
SHA-256 `907E1F484EDC95795F363BCFA40320CABC63019BB4E9C16A8BAAE5684A509DE6`.

LIMITATION: The 4096-event cap means the census is not complete. Records 3–7,
full pointer/descriptor grammar, semantic object/animation/frame names, and
all-frame reconstruction remain unresolved. No ROM payload, asset, C++, or
SOURCE_OWNED change was made.

NEXT: target the exact consumers and continuation around the newly observed
`0x1719xx/0x171Axx` source records, preserving bounded event caps and requiring
an independently proven finite grammar before enumeration claims.

# 2026-09-12 — M12 exact selector value and runtime record mapping — IN PROGRESS

TASK: Add the exact RAM-selector read to the bounded ROM/source probe, while
preventing the unrelated high-frequency selector consumer from exhausting the
capture buffer.

RESULT: BizHawk 2.11.1 completed the 1800-frame deterministic replay with
6,334 events under a 16,384-event cap. At the known selector-read callback
PCs, `0xFFAFAE` reads value 0 in 733 events and value 2 in 473 events. The
bounded table-field mapping is directly observed as selector 0 -> record 0
field `0x3B8EA` -> `0x3B95C`, and selector 2 -> record 2 field `0x3B90A`
-> `0x3B982`; the subsequent pointer reads return `0x171832`/`0x1742DC`.
The same capture retains 24 distinct first-word source starts read by the
`0xB730` body. The catalog now validates selector values, table-field
addresses, pointer values, and the source-start census without assigning
semantics.

ARTIFACT: ignored local capture
`build/m12-gfx-runtime/targeted-reads-current-1800-v6.json`, SHA-256
`17CD010F9535CAA5EE5823031F02E77A94EC0F6E865BF6BD34BD88268638201C`; catalog
SHA-256 `8758355D33F85A9E6E27091F1BE8276206EA980638663601CEA9F7AA64FAAC3A`.

LIMITATION: The live scenario selects only records 0 and 2; one unrelated
field read observes record 1. Records 3-7 and the semantic object,
animation, frame, and full sprite grammar remain unresolved. No SOURCE_OWNED,
ROM payload, asset, C++, or M13 change was made.

NEXT: follow the exact record-2 pointer consumer and the 24 source-start
continuations with bounded read/exec evidence, then prove a finite frame
descriptor grammar before any completeness claim.

# 2026-09-12 — M12 selector-2 secondary word and source continuation — IN PROGRESS

TASK: Follow the exact selector-2 pointer consumer only through the already
observed call-context and source-start watch set; do not interpret raw words
as semantic frame metadata without an independent contract.

RESULT: The bounded 1800-frame read capture now observes selector 2 -> record
2 field `0x3B90A` -> `0x3B982` -> `0x1742DC`. The secondary address
`0x1742DC` returns `0x0000001C` in 320 events, and the subsequent `0xB730`
first-word read is directly observed at `0x1742F8` in 160 events; its
`+2/+4/+6` reads are also retained. The runtime source census is now 25
distinct starts. The catalog records this as an address/value continuation,
not as a frame count or semantic descriptor.

ARTIFACT: ignored local capture
`build/m12-gfx-runtime/targeted-reads-current-1800-v7.json`, SHA-256
`3651A12DC22B02E88B994F32DF4A18946B87C06FCF11C8739589DB709970E808`; catalog
SHA-256 `D624033CCBF10329D326040941C94513E2BCABEA573B1234C5948BC6736D160E`.

LIMITATION: The scenario still selects only records 0 and 2 on the principal
selector path; record 1 is seen only through an unrelated field read, and
records 3–7 remain unobserved. Full object/animation/frame grammar and
completeness remain unresolved. No SOURCE_OWNED or payload change was made.

NEXT: target the exact record-2 continuation PCs around the `0x1742DC ->
0x1742F8` transition and prove whether the 25 starts share a finite record
grammar, using independent read and execution evidence.

# 2026-09-12 — M12 exact-read v8 and independent B730 call context — IN PROGRESS

TASK: Reconcile the latest bounded exact-address read capture with the
payload-free catalog and add an independent register-context corroboration of
the selector-to-B730 provenance edge. Preserve BizHawk instrumentation and do
not infer frame/object semantics from raw words.

RESULT: The known-good BizHawk 2.11.1 invocation remains reproducible with the
canonical ROM and existing workflow. The v8 exact-address capture
`build/m12-gfx-runtime/targeted-reads-current-1800-v8.json` contains 8,780
read events below the 16,384 cap (SHA-256
`833E8157A9B51C1BB57B11E9CAE7AA8AFC3B7FC98157B2640019C0E90149513E`). At
the confirmed selector callback PCs, `0xFFAFAE` reads values 0 and 2;
selector 0 maps to `0x3B8EA -> 0x3B95C -> 0x171832`, while selector 2 maps to
`0x3B90A -> 0x3B982 -> 0x1742DC`. The same bounded read capture observes
`0x1742DC=0x1C`, `0x1742E2=0x76`, following B730 starts `0x1742F8` and
`0x174358`, and 26 distinct first-word source starts at callback PC
`0xB73E`. These remain address/value observations, not semantic frame or
descriptor assignments.

INDEPENDENT EVIDENCE: The payload-free call-context capture
`build/m12-gfx-runtime/b730-calls-current-v2.json` contains 15,870 events over
1,800 frames (SHA-256
`5CF7A6D708A1D252464893D61A8D0B764CD3407B8A5167518DA227D5073AE0CB`). At
`0x3B426`, `A0=0x3B95C` occurs 689 times and `A0=0x3B982` 469 times; at
`0x3B428`, the targets are `0x171832` and `0x1742DC`; at `0x3B448` and
`0xB730`, the bounded context reaches `0x1742F8` and `0x174358` among the
observed source starts. The new catalog wording records the distinction
between four starts in the context stream and 26 starts in the paired
exact-read census. Catalog SHA-256:
`472082214FE195DD225C8224B2B68FE6E300B8D8B44540502DA5DC267C94233C`.

LIMITATION: The scenario still selects only records 0 and 2 on the principal
selector path; record 1 is seen only through an unrelated field read, and
records 3–7 remain unobserved. The 26-start census is bounded by the watched
address set, not a complete ROM enumeration. Full object/animation/frame
grammar and all-frame reconstruction remain unresolved. No SOURCE_OWNED,
ROM payload, asset, C++, or M13 change was made.

VALIDATION: Python catalog and reconstruction tests, Python compilation,
report/runtime hash consistency, `git diff --check`, and the source-size
check passed. Full Debug and Release CTest each passed `154/154`; the focused
catalog CTest also passed in both configurations. GNU/Linux-equivalent build
validation remains unavailable because WSL exposes no `cmake`, `g++`, or
`ninja`.

NEXT: use the independent call/read evidence to target the finite continuation
grammar for records 0 and 2, then obtain selector coverage for records 3–7
without broad bus tracing or semantic overclaiming.

# 2026-09-12 — M12 replay-expansion bound and evidence-class switch — IN PROGRESS

TASK: Perform the one final replay-only expansion pass for records 3–7 using
the existing bounded exact-address probe, then stop replay expansion regardless
of outcome and switch to static consumer analysis if those records are not
observed.

RESULT: The already published positive evidence remains the authoritative
runtime result: v8 is an 1800-frame, 8,780-event exact-read capture with
selector values 0 and 2, direct table/pointer reads, 26 bounded source starts,
and the independent 15,870-event B730 register-context corroboration. The
final probe configuration added only the statically confirmed selector-reader
callback PCs and all four longword fields for each of the eight records. The
EmuHawk invocation accepted the command-line flags but produced no new capture
file, so records 3–7 were not observed in this pass. This is recorded as
`NO_NEW_CAPTURE`, not as negative runtime evidence.

BOUNDARY: Replay-based expansion is now closed for this scenario. No fifth
equivalent replay will be started. The current positive chain and all hashes
remain published in the M12 reports; no ROM payload, asset, C++, M13, or
SOURCE_OWNED change was made.

NEXT: switch evidence class to static consumer analysis around
`0x03A9EE..0x03AA18`, `0x03B8DE`, and the direct callers of `0x03B1D0`, then
use controlled state forcing only if static contracts cannot close selector
coverage without semantic invention.

# 2026-09-12 — M12 static selector/dispatch consumer catalog — IN PROGRESS

TASK: Replace replay expansion with bounded static consumer analysis after
records 3–7 were not observed in the final replay-only pass. Preserve exact
ROM identity and do not assign semantic object/animation/frame names from
dispatch targets.

RESULT: Added `src/tools/m12_static_sprite_dispatch_catalog.py` and regression
coverage in `tests/m12_static_sprite_dispatch_catalog_test.py`. The canonical
ROM validates the descriptor consumer at `0x03A9EE..0x03AA0E`: selector RAM
`0xFFAFAE`, eight rows, `0x10`-byte stride, all four longword offsets
`0/4/8/12`, and direct call `0x03AA0E -> 0x03B1D0`. Two finite selector-masked
dispatch tables are enumerated: `0x03B8A6..0x03B8C6` and
`0x03B8C2..0x03B8E2`, both eight entries with mask `0x0007` and stride `4`.
They overlap at `0x03B8C2..0x03B8C6`; the second overlaps the descriptor table
at `0x03B8DE..0x03B8E2`. The bounded static `0x03B1D0` slice observes direct
calls to decompressor `0x00003820` at `0x03B236`, `0x03B28A`, and `0x03B2FE`.

ARTIFACT: ignored static catalog
`build/m12-gfx-runtime/static-sprite-dispatch-catalog-current.json`, SHA-256
`79906506AE138318AAB9B8B0F1D581E5102B23CDCC2A2CCBF4CDC71A04728EC5`.

LIMITATION: The target addresses are static dispatch targets only, not proven
object, animation, frame, or sprite semantics. Live runtime coverage remains
records 0 and 2 on the principal path; records 3–7 still need a materially
different evidence source if their consumers must be observed. No ROM payload,
asset, C++, M13, or SOURCE_OWNED change was made.

VALIDATION: Static catalog test, Python compilation, and canonical-ROM catalog
generation passed. Full Debug and Release CTest remain valid at `154/154`
before this Python-only addition; the new focused CTest must be run after CMake
reconfiguration.

NEXT: inspect the eight static dispatch targets and their resource/decompressor
call contracts, then decide whether controlled state forcing is necessary to
obtain live records 3–7 evidence.

# 2026-09-12 — M12 finite selector/descriptor grammar — STATIC BOUNDARY

TASK: Replace the closed replay-expansion path with one bounded static evidence
pass. Enumerate the complete selector/descriptor/dispatch graph, classify all
four descriptor fields, recursively validate proven child tables, and stop at
the first unresolved semantic/extent edge. Do not run another equivalent
replay, add ROM/assets, promote ownership, or begin M13/C++ work.

RESULT: Added the developer-only payload-free catalog
`src/tools/m12_selector_descriptor_grammar.py`. It validates the canonical
USA ROM hash, all ten direct `0x00FFAFAE` xrefs (two writers and eight
read/control sites), selector domain `0..7`, the descriptor table
`0x03B8DE..0x03B95E`, both eight-entry dispatch tables, the descriptor setup
`0x03A9EE -> 0x03AA0E -> 0x03B1D0`, and direct decompressor calls at
`0x03B236`, `0x03B28A`, and `0x03B2FE`. The catalog SHA-256 is
`EF1055715FE4FD72A7125A899D796AB6437EAAC243C78A9094200E418288799F`.

CHILD GRAPH: Descriptor `+12` rows 0–6 point to seven exact child tables,
each closed by a high-bit `0xFFFF` sentinel. Boundaries are
`[0x03B95C,0x03B982)`, `[0x03B998,0x03B9A6)`, `[0x03B982,0x03B998)`,
`[0x03B9A6,0x03B9BC)`, `[0x03B9BC,0x03B9D2)`, `[0x03B9D2,0x03B9E8)`, and
`[0x03B9E8,0x03BA46)`. Child 6 ends at the secondary dispatch index-7
target, which is recorded as an alias. Selector 7 value `0xFFFF0017` stays
`NON_ROM_LONGWORD_ADDRESS_UNRESOLVED`.

PROVENANCE: Field `+4` and `+8` are ROM-source pointers to the bounded
`0x3820` calls; `+12` is a child state-record/relative-table edge and reaches
`0x03B448 -> 0x0000B730`, after which the already-published SAT producer chain
continues. Field `+0` is classified only as the `0xFFAFA8 -> 0x03B7C0` path.
The shared relative-word candidate at `0x03BDA6` has 18 observed words but
no complete consumer boundary. Dispatch targets remain neutral static
contracts; no object/animation/frame/sprite semantics are assigned.

BOUNDARY: Status is `STATIC_FINITE_GRAPH_WITH_UNRESOLVED_SEMANTICS`. Static
finite structure is proven, but complete semantic grammar and shared-table
extent are not. No controlled forcing was needed for this catalog. Replay
expansion remains closed; no fifth equivalent campaign was run.

OWNERSHIP: SOURCE_OWNED is unchanged at `1,475,368 / 3,145,728` bytes
(`46.9006856283%`) before and after. No ROM, decoded asset, ignored capture,
C++, M13, or production runtime change was added.

VALIDATION: Focused Python test, canonical catalog generation and `py_compile`
pass. The new CTest is registered in `cmake/m12_auto2.cmake`; the previous
`154/154` Debug/Release evidence remains the historical baseline, and the
current sequential Windows Debug/Release runs are both `156/156`. GNU/Linux
WSL configuration and
full build/link completed with GCC 13.3.0; its full CTest reached the same
slow `project_file_line_limit` scan on `/mnt/c` and was stopped after more
than seven minutes. The Windows Debug and sequential Release full CTest runs
are the complete `156/156` evidence. Next action stops at the unresolved
shared-table/semantic edge unless a materially different evidence class is
explicitly authorized.

# 2026-09-12 — M12 upstream selector source and relative pair grammar — BOUNDED RESULT

TASK: Close the upstream source of the selector initialization/increment loop
and the downstream consumer grammar rooted at `0x03BDA6` without another replay
campaign, ROM/assets, M13, or ASM-to-C++ work.

RESULT: Added the payload-free static analyzers
`src/tools/m12_selector_control_analysis.py` and
`src/tools/m12_relative_table_analysis.py`, with focused regression tests.
The reset vector `0x000004 -> 0x0000020E` reaches the exact dispatcher latch
`0x0000042C..0x0000045C`; its indirect `JSR (A1)` at `0x0000045A` selects the
five-entry table prefix at `0x0000045E`, where masked state `0x10` targets
`0x03A748`. No direct incoming edge to `0x03A748` was found. This proves the
upstream control source and leaves the exact indirect edge as the only entry
mechanism.

The handler statically proves `0xFFAFAE := -1`, increment, compare with `7`,
and a neutral finite loop over `0..7`. Direct `FF10AC` writers enumerate the
higher-level state values `-1`, `4`, `8`, `0x0C`, and `0x10`; handler exit
writes `4`. Semantic labels remain intentionally unresolved.

The relative analyzer proves `0x03BDA6..0x03BDCA` as 18 two-byte pointer words,
11 statically consumed pointer entries, signed `ADDA.W` offsets, and a 4-byte
pair grammar whose words feed `A6+8` and `A6+6`; cursor stride is four bytes
and the second word's bit 15 controls reset versus advance. Closed prefixes
reach `0x03BF76`. Full extent remains fail-closed because the stream at
`0x03BDD8` has no consumer marker before code boundary `0x03BF86`.

JOIN: higher-level `FF10AC` state -> dispatcher -> `0x03A748` -> selector
`0..7` -> descriptor `+12` -> child `+8` pair -> `A6+8` -> child `+0` table
-> `0x03B448` -> `0x0000B730` -> SAT. No runtime replay or controlled state
forcing was needed.

OWNERSHIP: `SOURCE_OWNED` remains `1,475,368 / 3,145,728` bytes
(`46.9006856283%`) before and after. No ROM, decoded asset, or generated
capture was added.

VALIDATION: Focused analyzer tests and Python compilation pass. CTest
registration is in `cmake/m12_auto2.cmake`; full Debug/Release and GNU/Linux
equivalent validation are required before publication. The detailed result is
in `docs/reports/THOR_M12_SELECTOR_RELATIVE_CLOSURE.md`.

BOUNDARY: Upstream is structurally proven. Downstream grammar is proven but
its total logical extent is blocked by one precise consumer-boundary edge;
stop this static pass here and use a materially different evidence class if
that edge is pursued later.

# 2026-09-12 — M12 two indirect body calls — FINITE TARGET CLOSURE

TASK: Resolve only the two previously unresolved selector-masked body calls at
`0x03AA28` and `0x03AAA8`, enumerate their proven finite targets and routine
boundaries, and join the result to the existing descriptor/relative/B730/SAT
graph. No equivalent replay campaign, ROM scan, ROM/assets, M13, or C++ work.

RESULT: Added `src/tools/m12_indirect_body_dispatch.py` and its focused test.
Exact static slices prove both call forms read selector RAM `0x00FFAFAE` into
`D0`, mask with `0x0007`, scale by four, load a longword target into `A0`, and
execute `JSR (A0)`. The primary source is `0x03AA12..0x03AA2A` with call
`0x03AA28` and table `0x03B8A6..0x03B8C6`; the secondary source is
`0x03AA92..0x03AAAA` with call `0x03AAA8` and table `0x03B8C2..0x03B8E2`.

The two tables enumerate 15 unique targets. Fourteen routine entries are
closed by exact RTS boundaries from `0x03AAAE..0x03B092`; secondary index 7
targets `0x03BA46`, which is classified as data/descriptor overlap rather
than code. Target direct calls, static RAM input/output sites, and ROM-source
edges are recorded in the payload-free analyzer output. `0x03ACA8` and
`0x03ADB4` statically call `0x3820` from ROM sources `0x17A750` and
`0x17E3BA`, respectively.

JOIN: The calls are now structurally integrated inside the already proven
`FF10AC -> dispatcher -> 0x03A748 -> selector -> descriptor/child relative
pairs -> 0x03B448 -> 0x0000B730 -> SAT` path. Semantic object/animation/
frame/sprite labels remain unassigned. One separate downstream edge remains
open at `0x03B0A8: JMP (A0)` in callee `0x03B092`, reached from target
`0x03AD66`; it is outside the two requested body calls and is not guessed.

OWNERSHIP: SOURCE_OWNED is unchanged at `1,475,368 / 3,145,728` bytes
(`46.9006856283%`). No ROM, decoded assets, captures, or production-runtime
changes were added.

VALIDATION: Focused analyzer test, Python compilation, and canonical
payload-free report generation pass. The new CTest registration is in
`cmake/m12_auto2.cmake`; full Debug/Release CTest, GNU/Linux equivalent
build/link, `git diff --check`, source-size validation, commit, push, and
exact CI verification remain publication gates for this bounded result.

BOUNDARY: Both requested indirect body calls are statically resolved. Stop
here unless a materially different evidence class is explicitly authorized
for `0x03B0A8` or unresolved semantic labels.

# 2026-09-12 — M12 B092 indirect tail — FINITE TARGET CLOSURE

TASK: Resolve only the second-level indirect tail rooted at `0x03B092` /
`0x03B0A8`. No broad graphics pass, replay campaign, ROM scan, ROM/assets,
M13, or C++ work.

RESULT: Added `src/tools/m12_b092_tail_dispatch.py` and its focused test.
The exact bounded routine is `MOVE.W (0x00FFAFB0),D0`, `ANDI.W #3`, two
`ADD.W D0,D0`, `LEA 0x03B0AA(PC),A0`, `MOVEA.L (A0,D0.W),A0`, then
`JMP (A0)` at `0x03B0A8`. Thus the index is an unsigned masked RAM-state
word, scale 4, and table `0x03B0AA..0x03B0BA` has exactly four absolute
big-endian longword entries: `0x03B0BC`, `0x03B0E8`, `0x03B132`, and
`0x03B0BA`.

All four target routines are RTS-closed: `03B0BA..03B0BC`,
`03B0BC..03B0E8`, `03B0E8..03B132`, and `03B132..03B188`. Their static I/O
and terminal behavior are recorded in the payload-free JSON contract. They
have no direct calls, ROM accesses, `0x3820`/`0xB730` calls, or A6/
selector/descriptor references. The family is RAM/flag transformation code,
not a proven command/frame interpreter.

SOURCE: `0x03AD66` calls `0x03B092` at `0x03ADAC`; the local path does not
assign `0x00FFAFB0` before the call. The second-level index is therefore
`0x00FFAFB0 & 3`, distinct from selector `0xFFAFAE`; no semantic state label
is invented. Selector-7 `0xFFFF0017` remains unrelated and unresolved.

JOIN: `FF10AC -> 0x03A748 -> body selector -> 0x03AD66 -> 0x03B092 ->
0x03B0AA target table`; the existing relative-pair -> `0x03B448` ->
`0xB730` -> SAT path remains unchanged. No runtime experiment was needed.

OWNERSHIP: SOURCE_OWNED remains `1,475,368 / 3,145,728` bytes
(`46.9006856283%`). No ROM, assets, captures, or production-runtime change.

VALIDATION: Focused test, Python compilation, and canonical payload-free
report generation pass. Full Debug/Release CTest, WSL build/link,
`git diff --check`, source-size validation, commit/push, and exact CI remain
publication gates.

BOUNDARY: The `0x03B092` tail and all four finite targets are structurally
exhausted. Stop here; do not start another indirect-dispatch pass.

# 2026-09-12 — M12 static scheduler-to-A372 caller join — STRATEGIC FORK

TASK: Continue M12 with a materially different static cross-subsystem join
from the closed A372 shadow-SAT producer through its scheduler callers. Do not
repeat indirect-dispatch closure or replay, promote ownership, add ROM/assets,
start M13, or migrate ASM to C++.

RESULT: Added the payload-free analyzer
`src/tools/m12_a372_caller_join.py` and regression test. Exact 68000 contracts
prove the main-loop order `0x008B2E -> 0x00557A`, `0x008B42 -> 0x008E90`,
`0x008B86 -> 0x00A196`; `A196` executes `SF.B FF1651`, calls `A342` at
`0xA19C`, then tests `FF1996`. The separate `A6A0` family remains closed by
callers `0x0428A` and `0x04A5A`.

The new cross-subsystem edge is `0x03C6E4 ST.B FF1858` immediately followed by
`JSR 0x008E90` and `JSR 0x00A196`, thereby joining the local boolean root
selector to the scheduler -> `A342..A438` -> `A372` -> `FF13CC` -> DMA/SAT
path. The neighboring `0x03C262..0x03C454` range writes `FF1858` at `0xC328`
but contains no opcode-level direct call to `A196`; indirect/local effects
remain unresolved. Semantic object/animation/frame/piece labels remain
unproven.

OWNERSHIP: `SOURCE_OWNED` remains `1,475,368 / 3,145,728 = 46.9006856283%`;
zero ROM bytes were added. Detailed cycles, evidence, negative result, and
ranked next directions are in
`docs/reports/THOR_M12_A372_CALLER_JOIN.md`.

VALIDATION: focused Python test, canonical payload-free report generation,
`py_compile`, Debug CTest `162/162`, Release CTest `162/162`, source-size scan,
`git diff --check`, and canonical ROM identity checks passed. The GNU/Linux
equivalent Release configure/build/link and targeted M12 CTest passed `2/2`.
The exact final pushed SHA and matching GitHub Actions run are recorded in the
publication handoff after push.

DECISION: Stop after three bounded cycles at a strategic fork between targeted
runtime register capture, wider FF1858 state closure, and the unresolved
`0x03BDA6` consumer boundary. Do not run a fourth equivalent caller pass.

# 2026-09-12 — M12 A372 runtime register/root capture — BOUNDED NEGATIVE

TASK: Take the highest-information fork from the caller-join report: one
developer-only BizHawk capture of `A196 -> A342 -> A372`, recording only
bounded register/RAM context and source-write addresses. Preserve the frozen
natural scenario, canonical ROM identity, no state writes, no payload, no
manual gameplay search, no promotion, and no C++/M13 work.

PASS 1: Revalidated canonical ROM identity and the exact static contracts
`0x008B86 -> 0x00A196`, `0x00A19C -> 0x00A342`, and `0x00A372` as the source
producer. The new Lua probe, Python summarizer, synthetic regression, and
`py_compile` passed.

PASS 2: BizHawk 2.11.1 ran the existing `m11_8_natural_reachability_v1`
hardware-reset scenario for exactly 1,800 frames and exited 0. Execution hooks
reported `A196=0`, `A342=0`, `A372=0`; no root/register sample was observed.
The bounded source-write list reached its cap of 4,096 without a paired A372
event, so it is explicitly non-diagnostic and no write semantics are inferred.
Ignored capture: `build/m12-gfx-runtime/a372-runtime-pass2.json`, SHA-256
`eeb8489191a3649a32237b701ced7b294d0f4edcab842a6f8019398e0af562e2`.

RESULT: `NO_NEW_TARGET_RUNTIME_REACH` for this frozen scenario only. This is
not a global reachability negative and does not weaken the static scheduler
join. No runtime root-selection, same-frame SAT, object, animation, frame, or
piece claim is made. `SOURCE_OWNED` remains `1,475,368 / 3,145,728 =
46.9006856283%`; ROM bytes added: 0.

VALIDATION: focused runtime summarizer test, Python compilation, canonical ROM
hash/size, static join regression, and payload-free report generation passed.
Full Debug/Release CTest, GNU/Linux build/link, `git diff --check`, source-size
scan, commit/push, and exact CI remain publication gates.

BOUNDARY: Two-pass rule complete. Do not repeat an equivalent 1,800-frame
natural replay. Further progress requires a separately validated controlled
state or a materially different static evidence class.

# 2026-09-12 — M12 shadow-field direct-access closure — TWO-PASS STATIC RESULT

TASK: After publishing the verified BizHawk checkpoint, close direct absolute
accesses to `FF1858`, `FF188A` and `FF188C`. Keep semantic labels neutral,
avoid another generic input replay, and stop after two materially different
bounded passes.

RESULT: Pass 1 enumerated the existing 14,621 decoded-instruction census with
`src/tools/m12_shadow_sat_access_graph.py`: 6 direct accesses to `FF1858`, 34
to `FF188A`, and 29 to `FF188C`. Operation types, widths, and the four
longword `FF188A` writes overlapping `FF188C` are recorded. Pass 2 closed the
bounded A342 producer/cursor path and ran one writer-only QuickSave1 probe for
the specific static ambiguity. Both Right and neutral arms produced the same
writer sequence: `A42A/A430 -> 0030/0006`, `B000/B006 -> 0060/000C`, and
`ACE6/ACEC -> 0080/0010`. Thus `FF188C=0080` and `FF188A=0010` are accounted
for by `ACE6` and `ACEC`; Right causality remains unproven.

OWNERSHIP: `SOURCE_OWNED` remains `1,475,368 / 3,145,728 = 46.9006856283%`;
delta `0`. No ROM, asset, savestate, or payload was added.

VALIDATION: published baseline harness returned `result=PASS`; access graph
synthetic and local-census tests passed; Python compilation passed. Debug and
Release CTest passed 163/163 each with the pre-existing slow source-size test
excluded; a fresh Ubuntu 24.04/GCC 13.3 Release tree passed 163/163 and built
successfully. A tracked source-size scan and `git diff --check` passed.

STOP: Direct absolute closure over the decoded census is complete. Remaining
register aliases, unsupported/unexecuted bytes, and dispatcher/continuation
caller edges are explicit unresolved edges in
`docs/reports/THOR_M12_SHADOW_SAT_ACCESS_CLOSURE.md`; a third equivalent xref
sweep is prohibited. Next rank is the `0x03BDA6/0x03BDD8` consumer boundary.
# 2026-09-13 — THOR Evidence Engine V2.1 SOUNDNESS REPAIR — READY FOR REAUDIT

TASK: Repair only the reproduced V2 foundation defects from the independent
gate audit: verified coverage trust, overlap retention, historical roots,
atomic writes, SQLite identity integrity, and real V1-to-V2 RAM integration.

RESULT: `CoverageCertificate` is now an untrusted claim and only a contiguous,
receipt-bound `VerifiedCoverageCertificate` can authorize a proven writer.
Initial roots no longer alias future writes; full write ranges validate before
mutation; duplicate immutable events are idempotent; imported RAM payloads are
checked by embedded identities and content digests. The FF13CC target now
exposes four persisted V2 byte versions and its selected A372 operation.

VALIDATION: all five Release evidence CTest helpers pass; V2 adversarial tests
cover forged/truncated coverage, epoch boundary, temporal roots, overlap,
failed FFFFFF write, duplicate event, SQLite rollback/retry/reopen, and lookup
timing. SOURCE_OWNED remains `1,475,368 / 3,145,728`, delta `0`. V3 remains
unauthorized; implementation commit `7b054d2bd8fc406eef04a0c62a5cb2b8484dbc4b`
is complete; publication SHA and CI are recorded after the documentation amend.
# 2026-09-13 — THOR Evidence Engine V4 ROM/RESOURCE/HARDWARE BRIDGE — PASS

TASK: Build only V4 from the accepted V3 baseline. Add canonical ROM/resource
roots, `0x3820`/Ancient transform interop, bounded VDP memory domains, DMA,
cross-domain graph edges and existing-sidecar persistence. Do not start V5.

RESULT: V4 skeleton and tests pass. SOURCE_OWNED remains
`1,475,368 / 3,145,728`, delta `0`. Known V2.1/V3 defects remain carried and
V4 timing/alias/IRQ frontiers are explicit. Publication SHA and exact CI are
recorded in the final gate response.
# 2026-09-13 — THOR Evidence Engine V5 STATIC/CARVER BRIDGE — IN PROGRESS

TASK: Build only V5 after the published V4 gate. Add bounded static
request/response identities, runtime-seeded query generation, typed static
certificates, non-owning Carver merge/export and explicit unresolved frontiers.

SOURCE_OWNED remains `1,475,368 / 3,145,728`, delta `0`. V5 validation and
publication SHA/CI are recorded after the stage gate.

# 2026-09-13 — THOR Evidence Engine V6 MULTI-SCENARIO DIFFERENTIAL — PASS

TASK: Build only V6 after the published V5 gate. Add controlled scenario
identities, isolated per-scenario graphs, neutral-repeat and motivated-alternate
validation, and deterministic COMMON/scenario-specific manifests without
cross-run temporal splicing.

RESULT: `differential.py` and its helper test pass. Shared facts are explicitly
observation-only and causal claims/cross-scenario edges are empty. SOURCE_OWNED
remains `1,475,368 / 3,145,728`, delta `0`. Publication SHA and exact CI are
recorded after the stage gate.

# 2026-09-13 — THOR Evidence Engine V7 FRONTIER SCHEDULER — PASS

TASK: Build only V7 after the published V6 gate. Add unresolved-frontier
inventory, deterministic information/cost/risk ranking, bounded evidence
requests, two-pass exhaustion and fixed-point reporting without whole-ROM trace.

RESULT: `frontier.py` and its helper test pass. Whole-ROM requests are rejected,
non-progress frontiers exhaust after two passes, and scheduler exports remain
non-owning. SOURCE_OWNED remains `1,475,368 / 3,145,728`, delta `0`.
Publication SHA and exact CI are recorded after the stage gate.

# 2026-09-13 — THOR Evidence Engine V8 HELD-OUT FRONTIER — PASS

TASK: Build only V8 after the published V7 gate. Evaluate one real unknown
frontier through existing static enumeration plus a ROM-bound runtime receipt,
preserving UNKNOWN/CONFLICT and forbidding ownership promotion.

RESULT: The canonical `0x03BDA6/0x03BDD8` evaluation produced 11 consumed
pointer entries and retained the unterminated `0x03BDD8` stream as UNKNOWN. The
runtime observation did not close the static frontier; the join remains
UNKNOWN/causal false. SOURCE_OWNED remains `1,475,368 / 3,145,728`, delta `0`.
Publication SHA and exact CI are recorded after the stage gate.

# 2026-09-13 — THOR Evidence Engine V9 OPERATIONAL INTEGRATION — PASS

TASK: Build only V9 after the published V8 gate. Assemble capture import,
persistent SQLite state, bounded scheduler/static/Carver cycle, deterministic
manifest export and a small CLI workflow. Do not begin stabilization.

RESULT: `orchestrator.py` and `cli.py` pass deterministic cycle, persistence,
non-owning Carver merge and CLI tests. SOURCE_OWNED remains
`1,475,368 / 3,145,728`, delta `0`. V9 is the final autonomous build stage;
publication SHA and exact CI are recorded after the stage gate.

# 2026-09-13 — THOR Evidence Engine stabilization P0 — REPAIRS IN PROGRESS

REPRODUCED: V2.1-001 accepted fabricated tagged coverage; V2.1-003 accepted a
wrong RAM producer/output association in SQLite; V2.1-005 accepted an
under-validated `ram_engine` certificate branch; V2.1-006 allowed forged RAM
coverage to bypass the FF188A negative oracle.

REPAIRED: coverage bases now require content-attested event hashes and reject
NOTE/unknown-effect substitutions; SQLite validates byte producer, address,
big-endian value, predecessor and rollback semantics; the V2 canary validator
checks certificate, operation/version identities, target outputs and coverage.
Adversarial regressions are green. Lifecycle remains `REGRESSION_ADDED` until
the stabilization checkpoint receives exact CI verification.
