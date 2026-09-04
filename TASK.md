# Current Task

TASK: M11.5 bounded `MOVEA.L (A7)+,An` postincrement transfer support for
`0x60BFA` and `0x60C08`
WHY: remove one proven decoder boundary without inventing stack state or an ABI.
CURRENT MILESTONE: M11.5 follow-up under M11
MILESTONE UNDERSTANDING CONFIDENCE: 95%
CURRENT SLICE UNDERSTANDING CONFIDENCE: 96%
SLICE CONFIDENCE EVIDENCE: accepted bounded closure, exact 0x205F decode and
USA bytes around 0x60BCC..0x60C08; results remain raw address evidence.
SLICE MODE: RE_TOOLING_ONLY
STATUS: COMPLETE

## Scope and method

The existing `oasis.m68k.re-reachable-closure.v1` report recognizes only
longword `MOVEA.L (A7)+,An`: source mode 3/A7, destination mode 1/An,
`An = memory[old A7]`, `A7 += 4`. A value is propagated only when bounded
evidence proves the stack slot. The narrow layer accepts only immediate long
push, known-address PEA, proven `MOVE.L An,-(A7)` and this exact pop.

Output schema is `oasis.m68k.re-reachable-closure.v1`; JSON and text are
deterministic. The CLI is local-USA-ROM-only developer tooling and remains
outside `oasis_core` and gameplay runtime.

## Acceptance criteria

- [x] exact targets `0x60BFA` and `0x60C08` retain `0x205F` pop evidence;
- [x] synthetic push/pop, PEA, unknown, conflict, call-boundary and A7 tests pass;
- [x] USA oracle confirms `0x60BCC` BSR, `0x60BD0` pop, `0x60BF6` ADDQ and targets;
- [x] 14 prior `call_clobber` remain; the 2 target refs are honest stack-unknown;
- [x] Debug/Release/GNU-equivalent CTest, JSON determinism, file-limit and
  diff-check pass; tooling remains separate from `oasis_core`.

## Verified result

The report remains at 16 reachable unresolved refs: 14 `call_clobber`, 2
`other` with `stack_value_unknown_call_boundary`, newly resolved 0, and zero
speculative resolutions. Raw Atlas remains 577 and displacement backlog 446;
the 80 nonreachable refs remain separate.

## Known unknowns and hard boundaries

The real stack value before `0x60BD0` is not proven because the bounded path
crosses `BSR 0x60BCC`; no callee effect, return-address model, calling
convention or A7 entry value is assumed. No other postincrement form, general
stack emulator, dynamic tracing, production behavior or M12 work was added.
STOP at this checkpoint.

## Exact next action

Stop at this verified closure checkpoint. Await explicit selection of the next
bounded evidence class; do not begin dynamic tracing, whole-ROM discovery or
M12 automatically.
