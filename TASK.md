# Current Task

TASK: M11.5 bounded natural reachability scenario for 0x6121A
WHY: determine whether a real hardware-reset execution can reach the primary
static target with a finite, reproducible controller scenario.
CURRENT MILESTONE: M11.5 follow-up under M11
MILESTONE UNDERSTANDING CONFIDENCE: 95%
CURRENT SLICE UNDERSTANDING CONFIDENCE: 95% for bounded reachability evidence;
exact source instruction and callee state remain unknown
SLICE MODE: RE_TOOLING_ONLY
STATUS: COMPLETE_WITH_LIMITATIONS

LAST_VERIFIED_RESULT: frozen neutral-input BizHawk 2.11.1 hardware-reset scenario reaches `0x6121A` at frame 113 (114 frames executed), with two exact target-hook hits, identical A/B reports, and GitHub CI run `33868387017` successful
NEXT_ACTION: STOP; await an explicitly requested bounded evidence task
BLOCKERS: the exact instruction before `0x6121A` is not observable from the target hook; frame-boundary PC `0x6135E` is not caller evidence; `0x60B8C`, `0x60D4A`, `0x60BCC`, `0x60BD0`, `0x60BFA` and `0x60C08` were not observed in this scenario

## Natural reachability result

The frozen scenario is `src/tools/re_bizhawk_natural_scenario.txt` with schema
`oasis.m68k.emulator-scenario.v1`, canonical USA SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`, backend
`bizhawk-lua-natural-input`, `start_state=hardware_reset`, no input events and
`max_frames:300`. The developer-only Lua probe uses only real frame input and
exact target execution hooks; it never writes ROM, registers or memory.

Two independent runs reached primary target `0x6121A` at frame 113, after 114
frame advances, with target-hook count 2. The entry snapshot is PC
`0x0006121A`, A7 `0x00FF0BE2`, and stack window start `0x00FF0BC2`; D0-D7,
A0-A7 and SR are recorded in the ignored natural report. A/B natural reports
and normalized imported traces are byte-identical. Secondary target counts are
zero. The trace coverage is explicitly
`frame_boundary_samples_plus_exact_target_hooks`, not a full instruction trace.

Static bounded evidence independently confirms direct call-sites
`0x60B8C -> 0x6121A`, `0x60D4A -> 0x6121A` and `0x611EE -> 0x6121A`; the
natural run does not identify which one led to the observed target hit.
Previous frame-boundary PC `0x6135E` is retained only as sampled context.

## Acceptance criteria

- [x] deterministic `oasis.m68k.emulator-scenario.v1` parser/JSON and malformed-input tests;
- [x] hardware-reset natural scenario with finite horizon and real neutral input;
- [x] primary target reached and full entry register/A7/stack snapshot captured;
- [x] two-run replay equality and local USA-ROM scenario oracle;
- [x] natural probe remains developer-only and separate from `oasis_core`;
- [x] normalized human-readable import report generated from the captured trace;
- [ ] exact transfer caller and stack provenance remain unresolved because target
  hooks do not expose the preceding instruction; watched secondary targets were
  not reached.

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
