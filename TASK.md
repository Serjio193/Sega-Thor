# Current Task

TASK: M11.5 bounded callee-effect audit for direct callee `0x60BCC`
WHY: determine only the proven register/stack effect across the call boundary
that blocks `0x60BFA` and `0x60C08`; no ABI or general interprocedural inference.
CURRENT MILESTONE: M11.5 follow-up under M11
MILESTONE UNDERSTANDING CONFIDENCE: 95%
CURRENT SLICE UNDERSTANDING CONFIDENCE: 96%
SLICE MODE: RE_TOOLING_ONLY
STATUS: COMPLETE

## Verified result

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

## Acceptance criteria

- [x] bounded CFG, return site, A0-A7 effect table and stack timeline reported;
- [x] `oasis.m68k.re-callee-effect.v1` JSON/text is deterministic and developer-only;
- [x] synthetic callee effect/stack/return-address/call-boundary tests pass;
- [x] USA oracle executable checks exact bytes/CFG/memory evidence when ROM is supplied;
- [x] Debug/Release/GNU-equivalent CTest, file-limit and diff-check pass.

## Hard boundaries and exact next action

No calling convention, recursive callee scan, emulator, dynamic tracing,
whole-ROM analysis, gameplay behavior or M12 work was added. The local USA ROM
was not present in this workspace, so the reference executable remains to be
run against the user-supplied ROM. STOP and await an explicit next bounded task.
