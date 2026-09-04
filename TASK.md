# Current Task

TASK: M11.5 bounded unreachable-CFG evidence audit for `0x60004`
WHY: classify the exact 80 displacement records outside the entry-reachable
CFG without reducing raw unresolved Atlas evidence or inventing semantics.
CURRENT MILESTONE: M11.5 follow-up under M11
MILESTONE UNDERSTANDING CONFIDENCE: 95%
CURRENT SLICE UNDERSTANDING CONFIDENCE: 96%
SLICE CONFIDENCE EVIDENCE: accepted Atlas/ranking, bounded decoder/CFG and
USA oracle for `0x60004`; results remain raw address evidence.
SLICE MODE: RE_TOOLING_ONLY
STATUS: COMPLETE

## Scope and method

The audit consumes the existing Atlas and bounded decoder CFG for
`[0x60004, 0x61204)`. It records instruction/block context, bounded incoming
edges, lexical fallthrough, outgoing direct edges, nearest reachable blocks,
padding/data and decoder status. It classifies records only as candidates:
unreachable code, embedded data, secondary entry, decoder artifact, boundary
tail or unknown. It does not infer semantics or mutate Atlas/ranking.

Output schema is `oasis.m68k.re-cfg-audit.v1`; JSON and text are deterministic.
The CLI is local-USA-ROM-only developer tooling and remains outside
`oasis_core` and gameplay runtime.

## Acceptance criteria

- [x] exact 80 outside-reachable records are audited and grouped into 17 islands;
- [x] bounded incoming/outgoing CFG context and reachability factors are reported;
- [x] classifications remain hypotheses/unknown; raw Atlas counts are unchanged;
- [x] synthetic classification/island/format tests and USA oracle pass;
- [x] Debug/Release/GNU-equivalent CTest, JSON determinism, file-limit and
  diff-check pass; tooling remains separate from `oasis_core`.

## Verified result

Prior resolution remains 390 examined / 294 resolved / 96 unresolved
(92 unknown-base, 4 CFG-merge), Atlas 577→283. This audit accounts for 80 of
those 96 as nonreachable records: 77 unreachable-code candidates (332 bytes)
and 3 unknown (18 bytes), with zero known incoming edges. Reachable unresolved
remains 16; raw unresolved stays 96, Atlas stays 577 and displacement ranking
stays 446 because classification does not remove evidence.

## Known unknowns and hard boundaries

No secondary-entry, embedded-data, decoder-artifact or boundary-tail candidate
was confirmed. No dynamic scenario, interprocedural inference, symbolic
execution, emulator, decompiler, whole-ROM scan, recompiler, production C++
behavior or M12 work was added.

## Exact next action

Stop at this verified audit checkpoint. Await explicit selection of the next
bounded evidence class; do not begin dynamic tracing, whole-ROM discovery or
M12 automatically.
