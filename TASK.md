# Current Task

TASK: M11.5 bounded natural reachability search for 0x60B8C / 0x60D4A
WHY: determine whether a minimal deterministic raw controller input can reach
either already proven direct caller from hardware reset.
CURRENT MILESTONE: M11.5 follow-up under M11
MILESTONE UNDERSTANDING CONFIDENCE: 95%
CURRENT SLICE UNDERSTANDING CONFIDENCE: 96% for the observed bounded caller
path; broader path semantics and the unobserved second caller remain unknown
SLICE MODE: RE_TOOLING_ONLY
STATUS: COMPLETE_WITH_LIMITATIONS

LAST_VERIFIED_RESULT: one raw `Start` pulse at frame 120 in the existing
BizHawk 2.11.1 probe reaches `0x60B8C` at frame 423 after 424 frame advances;
the same run observes downstream `0x6121A`, and two identical replays match
byte-for-byte; GitHub Actions CI run `33871898019` is successful
NEXT_ACTION: STOP; await an explicitly requested bounded evidence task
BLOCKERS: this probe samples frame boundaries plus exact watched bus-exec hooks,
not every instruction; `0x60D4A` was not reached by the first successful
variant, and no broader input search or gameplay interpretation is justified

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
