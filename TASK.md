# Current Task

TASK: M11.5 Ghidra-to-Atlas Candidate Integration PoC
WHY: connect deterministic Ghidra wide-discovery output to the existing ROM
Atlas and evidence overlays without promoting Ghidra hypotheses to project
truth.
CURRENT MILESTONE: M11.5 post-M11 evidence tooling
MILESTONE UNDERSTANDING CONFIDENCE: 95%
CURRENT SLICE UNDERSTANDING CONFIDENCE: 96% for the bounded normalized merge,
classification and deterministic ranking; semantic confidence remains
intentionally limited to independent project evidence.
SLICE MODE: RE_TOOLING_ONLY
STATUS: COMPLETE

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
