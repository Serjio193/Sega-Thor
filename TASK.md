# Current Task

TASK: M11.5 bounded reachable-unresolved closure audit for `0x60004`
WHY: analyze exactly the 16 reachable unresolved displacement refs with bounded
backward provenance, without investigating the 17 unreachable islands.
CURRENT MILESTONE: M11.5 follow-up under M11
MILESTONE UNDERSTANDING CONFIDENCE: 95%
CURRENT SLICE UNDERSTANDING CONFIDENCE: 96%
SLICE CONFIDENCE EVIDENCE: accepted Atlas/ranking, bounded decoder/CFG and
USA oracle for `0x60004`; results remain raw address evidence.
SLICE MODE: RE_TOOLING_ONLY
STATUS: COMPLETE

## Scope and method

The closure consumes the existing Atlas, decoder CFG and resolution result for
`[0x60004, 0x61204)`. It records opcode/operand mode, A-register,
displacement, block predecessors, current reason, bounded backward definitions
and nearest proven pre-use state. Only existing conservative transfer rules are
used; calls invalidate state and unknown entry state is never guessed.

Output schema is `oasis.m68k.re-reachable-closure.v1`; JSON and text are
deterministic. The CLI is local-USA-ROM-only developer tooling and remains
outside `oasis_core` and gameplay runtime.

## Acceptance criteria

- [x] exact 16 reachable unresolved records are reported and reason-counted;
- [x] backward definitions, predecessor CFG and provenance are bounded to target CFG;
- [x] call clobber, unsupported transfer and entry-state uncertainty stay explicit;
- [x] synthetic backward/merge/call/boundary tests and USA oracle pass;
- [x] Debug/Release/GNU-equivalent CTest, JSON determinism, file-limit and
  diff-check pass; tooling remains separate from `oasis_core`.

## Verified result

Prior resolution remains 390 examined / 294 resolved / 96 unresolved
(92 unknown-base, 4 CFG-merge), Atlas 577→283. This closure accounts for all
16 reachable refs: 14 `call_clobber`, 2 `unsupported_transfer`, newly resolved
0, reachable unresolved after 16. Raw unresolved stays 577, displacement
backlog stays 446, and the 80 nonreachable refs remain separately audited.

## Known unknowns and hard boundaries

No speculative resolution or dynamic scenario was used. No calling convention,
entry register value, semantic field name, emulator, whole-ROM scan, recompiler,
production C++ behavior or M12 work was added.

## Exact next action

Stop at this verified closure checkpoint. Await explicit selection of the next
bounded evidence class; do not begin dynamic tracing, whole-ROM discovery or
M12 automatically.
