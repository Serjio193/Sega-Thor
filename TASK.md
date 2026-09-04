# Current Task

TASK: M11.5 bounded dynamic caller discrimination for 0x6121A
WHY: determine which already confirmed direct call-site transfers control to
the naturally reached target during the frozen frame-113 execution.
CURRENT MILESTONE: M11.5 follow-up under M11
MILESTONE UNDERSTANDING CONFIDENCE: 95%
CURRENT SLICE UNDERSTANDING CONFIDENCE: 96% for the bounded caller/entry
pairing; callee behavior and broader path semantics remain unknown
SLICE MODE: RE_TOOLING_ONLY
STATUS: COMPLETE_WITH_LIMITATIONS

LAST_VERIFIED_RESULT: frozen neutral-input BizHawk 2.11.1 scenario proves `0x611EE -> 0x6121A` for both frame-113 target hits, with A7 deltas `-4`, matching stack return `0x611F2`, identical A/B reports, and GitHub CI run `33870143848` successful
NEXT_ACTION: STOP; await an explicitly requested bounded evidence task
BLOCKERS: caller hooks expose bus-exec snapshots but no separate post-instruction timing channel; `0x60B8C` and `0x60D4A` remain unobserved in this scenario; callee effects and stack-writer provenance remain outside scope

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

## Acceptance criteria

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
