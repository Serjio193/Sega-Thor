# Current Task

TASK: M11.5 bounded caller-stack provenance audit before direct call `0x60BCC`
WHY: determine only whether `memory[P]` is provable immediately before the
known `BSR.W 0x604BC`; no ABI or general interprocedural inference.
CURRENT MILESTONE: M11.5 follow-up under M11
MILESTONE UNDERSTANDING CONFIDENCE: 95%
CURRENT SLICE UNDERSTANDING CONFIDENCE: 96%
SLICE MODE: RE_TOOLING_ONLY
STATUS: COMPLETE

## Verified result

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

## Previous verified result

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

## Caller-stack acceptance criteria

- [x] bounded caller CFG, containing/predecessor blocks, stack events and merges reported;
- [x] `oasis.m68k.re-caller-stack.v1` JSON/text is deterministic and developer-only;
- [x] synthetic push/PEA/pop/merge/unknown-call and bounded escape tests pass;
- [x] USA oracle executable checks exact bytes, CFG and both unknown-call blockers;
- [x] Debug/Release/GNU-equivalent CTest, source file-limit and diff-check pass.

## Hard boundaries and exact next action

No calling convention, recursive callee scan, emulator, dynamic tracing,
whole-ROM analysis, gameplay behavior or M12 work was added. STOP and await an
explicit next bounded task.
