# Current Task

TASK: M11.5 Atlas-driven unresolved evidence ranking
WHY: Turn the existing unresolved set into an objective priority loop without
claiming that structural candidates are already resolved.
CURRENT MILESTONE: M11.5 follow-up under M11
MILESTONE UNDERSTANDING CONFIDENCE: 95%
CURRENT SLICE UNDERSTANDING CONFIDENCE: 96%
SLICE CONFIDENCE EVIDENCE: accepted typed Atlas over bounded `re_program`,
`re_trace` and `re_diff`; USA Atlas has 577 unresolved memory refs.
SLICE MODE: RE_TOOLING_ONLY
STATUS: COMPLETE

## Scope and method

`oasis_re_atlas_rank` consumes Atlas typed unresolved records and emits
`oasis.m68k.re-ranking.v1` JSON/text. Each record remains linked to raw
instruction, block and function addresses and is grouped by addressing mode,
register, instruction family, containing function and frequency. Separate raw
candidate dimensions mark the bounded dynamic scenario and instructions with
immediates; unsupported decoder items are separate and use zero potential refs.
Counts overlap across dimensions and are explicitly not semantic guarantees.

## Acceptance criteria

- [x] rank all Atlas unresolved records deterministically;
- [x] expose mode/register/family/function/frequency and candidate dimensions;
- [x] print `If support X: potentially resolve N refs` priorities;
- [x] preserve unsupported decoder dependencies separately;
- [x] add synthetic grouping/format tests and USA oracle counts;
- [x] pass Debug/Release/GNU CTest, JSON parse, file-limit and diff-check;
- [x] keep the loop developer-only with no production/runtime changes.

## Verified result

The USA report ranks 577 refs in 31 groups. Largest structural opportunities:
`address_displacement` 446, containing function `0x60004` 424, register `A6`
387, `move_address` 360, immediate-based propagation candidates 168 and the
bounded dynamic scenario 2. Unsupported decoder evidence is 4 items. These
counts are objective prioritization signals, not automatic resolution.

## Known unknowns and hard boundaries

Candidate flags do not prove constant propagation or dynamic coverage; the
bounded trace backend remains limited to its accepted scenario. No production
behavior, emulator, whole-ROM scan, recompiler, semantic names or M12 work was
added.

## Exact next action

Stop at this verified ranking checkpoint. Await explicit instruction; the next
tool improvement must be selected from the ranked evidence, not started here.
