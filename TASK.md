# Current Task

TASK: M11.5 external emulator boot-trace oracle PoC
WHY: determine whether an existing external Mega Drive emulator can produce
reproducible reset-to-boot evidence without becoming a project dependency.
CURRENT MILESTONE: M11.5 follow-up under M11
MILESTONE UNDERSTANDING CONFIDENCE: 95%
CURRENT SLICE UNDERSTANDING CONFIDENCE: 96%
SLICE MODE: RE_TOOLING_ONLY
STATUS: COMPLETE_WITH_LIMITATIONS

LAST_VERIFIED_RESULT: real MAME 0.289 and BizHawk 2.11.1 `boot_initial` captures imported through `oasis.m68k.emulator-trace.v1`; BizHawk is primary and MAME secondary
NEXT_ACTION: stop at this checkpoint; use the normalized boot evidence only in a future explicitly requested bounded RE task
BLOCKERS: adapters do not yet normalize instruction-level branch/call/return or memory-read events; save-state APIs and full target reachability remain untested

## External adapter result

The developer-only adapter is separate from `oasis_core`. It accepts an
external normalized capture, records ROM/emulator/backend metadata, `boot_initial`
limits, PC/events, optional D0-D7/A0-A7/SR snapshots, direct call edges,
branch/call/return/memory/indirect counts, safely observed blocks and ranges,
static reset vectors, and Atlas-known versus Atlas-unknown PCs. Its schema is
`oasis.m68k.emulator-trace.v1`; control-flow targets are separately split into
Atlas-known and Atlas-unknown sets. Frame/cycle fields are retained as
non-deterministic metadata and excluded from `trace_hash`; optional register
snapshots participate in the deterministic hash.

The local bake-off verified `D:\Program Files\Mame\mame.exe` version `0.289`
and `D:\Program Files\BizHawk-2.11.1-win-x64\EmuHawk.exe` version `2.11.1`
against the ignored canonical USA ROM. BizHawk's Lua bus hooks produced 512
instruction events plus 128 writes; MAME's debugger trace produced 512
instruction events and a separate RAM watchpoint caught a real writer at
`0x0000026A` for address `0x00FFFFFE`. BizHawk replay hashes matched
`0x5CCA6906FAA6A219`; MAME replay hashes matched
`0xC2B1C053D1E43D76`. BizHawk is primary because its event hook supplies
direct executed-PC/register/write records; MAME is secondary because its
debugger trace requires PC normalization and its memory probe is separate.
Both runs began observing after reset setup (`0x26C` and `0x214`, versus reset
PC `0x20E`), so hidden bootstrap transitions are recorded, not inferred.
`0x6121A`, `0x60B8C` and `0x60D4A` were not observed. No emulator source,
binary, ROM or generated trace was added to the repository.

## Acceptance criteria

- [x] external capture parser rejects malformed input and normalizes event order;
- [x] deterministic JSON/text report, hash, coverage, calls, branches,
  indirect targets, reset-vector comparison and Atlas split are implemented;
- [x] synthetic adapter tests pass without ROM or emulator;
- [x] adapter remains outside `oasis_core`, with no emulator dependency;
- [x] real `boot_initial` execution, normalized replay comparison, register
  capture and RAM writer/watchpoint capability were verified with installed
  external backends;
- [ ] reset PC is not the first observed callback PC, target routines were not
  reached in this 512-instruction window, and branch/call/read event adapters
  remain unimplemented.

## Hard boundaries and exact next action

No emulator, copied emulator source, production runtime dependency, whole-game
trace, autoplay, semantic Atlas changes, call-clobber resolution or M12 work
was added. STOP at this verified bake-off; do not begin another tracing or
analysis scope without explicit instruction.

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
