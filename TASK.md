# Current Task

TASK: M11.5 external emulator boot-trace oracle PoC
WHY: determine whether an existing external Mega Drive emulator can produce
reproducible reset-to-boot evidence without becoming a project dependency.
CURRENT MILESTONE: M11.5 follow-up under M11
MILESTONE UNDERSTANDING CONFIDENCE: 95%
CURRENT SLICE UNDERSTANDING CONFIDENCE: 96%
SLICE MODE: RE_TOOLING_ONLY
STATUS: BLOCKED_EXTERNAL_BACKEND

LAST_VERIFIED_RESULT: neutral external-capture importer and `oasis.m68k.emulator-trace.v1` normalizer pass synthetic tests; no installed emulator/debug interface is available for real boot execution
NEXT_ACTION: provide/install an approved external emulator with PC/register/memory-debug access, then run only `boot_initial`
BLOCKERS: no installed external Mega Drive emulator or debugger automation interface found; no real boot oracle can be claimed

## External adapter result

The developer-only adapter is separate from `oasis_core`. It accepts an
external normalized capture, records ROM/emulator metadata, `boot_initial`
limits, PC/events, optional D0-D7/A0-A7/SR snapshots, direct call edges,
branch/call/return/memory/indirect counts, safely observed blocks and ranges,
static reset vectors, and Atlas-known versus Atlas-unknown PCs. Its schema is
`oasis.m68k.emulator-trace.v1`; control-flow targets are separately split into
Atlas-known and Atlas-unknown sets. Frame/cycle fields are retained as
non-deterministic metadata and excluded from `trace_hash`; optional register
snapshots participate in the deterministic hash.

Local inventory found no BlastEm, RetroArch, MAME, Mednafen, BizHawk, Ares,
Kega/Fusion or equivalent executable in PATH, common install roots, user
folders or package listings. No emulator source, binary, ROM or fake dynamic
trace was added. Real boot execution, first observed PC, replay match,
`0x6121A` observation and watchpoint capability are therefore UNKNOWN. The
checkpoint stops at the adapter and documented external-backend blocker.

## Acceptance criteria

- [x] external capture parser rejects malformed input and normalizes event order;
- [x] deterministic JSON/text report, hash, coverage, calls, branches,
  indirect targets, reset-vector comparison and Atlas split are implemented;
- [x] synthetic adapter tests pass without ROM or emulator;
- [x] adapter remains outside `oasis_core`, with no emulator dependency;
- [ ] real `boot_initial` execution, reset agreement, replay comparison,
  `0x6121A` observation and watchpoint capability require an external backend.

## Hard boundaries and exact next action

No emulator, copied emulator source, production runtime dependency, whole-game
trace, autoplay, semantic Atlas changes, call-clobber resolution or M12 work
was added. STOP. Provide one approved external emulator with documented
automation/debug access, then rerun only this bounded boot oracle.

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
