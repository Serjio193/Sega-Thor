# Current Task

TASK: M11.5 bounded address-displacement resolution PoC around `0x60004`
WHY: test whether conservative local register propagation reduces the largest
Atlas unresolved class without speculative semantics or runtime integration.
CURRENT MILESTONE: M11.5 follow-up under M11
MILESTONE UNDERSTANDING CONFIDENCE: 95%
CURRENT SLICE UNDERSTANDING CONFIDENCE: 96%
SLICE CONFIDENCE EVIDENCE: accepted Atlas/ranking, bounded decoder/CFG and
USA oracle for `0x60004`; results remain raw address evidence.
SLICE MODE: RE_TOOLING_ONLY
STATUS: COMPLETE

## Scope and method

`oasis_re_resolution` consumes the Atlas unresolved displacement records for
the existing `[0x60004, 0x61204)` bounded slice. It supports only immediate or
absolute/PC-relative address-register setup, address-register copies, immediate
ADDA/SUBA/ADDQ/SUBQ and conservative CFG propagation when predecessor values
agree. Calls and unknown address-register writes invalidate state. It does not
infer calling conventions, cross-function state, semantics or gameplay behavior.

Output schema is `oasis.m68k.re-resolution.v1`. JSON and text retain function,
block, instruction, signed displacement, status, effective-address class and
provenance. Atlas/ranking deltas are reported without mutating the static Atlas.

## Acceptance criteria

- [x] bounded `0x60004` displacement candidates are examined deterministically;
- [x] local register propagation resolves only supported evidence chains;
- [x] unknown base, CFG merge and unsupported-transfer outcomes stay explicit;
- [x] effective addresses are classified as ROM/RAM/outside/unknown;
- [x] provenance, reason counts, ranges and Atlas/ranking delta are reported;
- [x] synthetic propagation/merge/format tests and USA oracle pass;
- [x] Debug/Release/GNU-equivalent CTest, JSON determinism, file-limit and
  diff-check pass; tooling remains separate from `oasis_core`.

## Verified result

The USA report examines 390 displacement refs in the bounded target and
resolves 294. The remaining 96 are 92 `unresolved_unknown_base` and 4
`unresolved_cfg_merge`; provenance failures are 0. All 294 concrete addresses
are RAM references, with 78 unique addresses; no ROM concrete address was
invented. Global Atlas ranking changes from 577 to 283 unresolved refs,
displacement 446→152, A6 387→123 and immediate candidates 168→54.

## Known unknowns and hard boundaries

The 80 candidates outside the entry-reachable CFG path remain explicit
unresolved evidence. No dynamic scenario, interprocedural inference, symbolic
execution, emulator, decompiler, whole-ROM scan, recompiler, production C++
behavior or M12 work was added.

## Exact next action

Stop at this verified bounded resolution checkpoint. Await explicit selection
of the next evidence class; do not automatically begin dynamic tracing,
whole-ROM discovery or M12.
