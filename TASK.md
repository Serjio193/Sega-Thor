# Current Task

TASK: M11.5 fifth checkpoint — retail/beta changed block ordinal 10 detail
WHY: Compare only retail `0xA6A4` block ordinal 10 with beta `0xA654` block
ordinal 10 at instruction, raw-data and CFG level, without semantic inference.
CURRENT MILESTONE: M11.5
MILESTONE UNDERSTANDING CONFIDENCE: 95%
CURRENT SLICE UNDERSTANDING CONFIDENCE: 96%
SLICE MODE: RE_TOOLING_ONLY
STATUS: COMPLETE

## Scope and method

`oasis_re_diff` continues to load both binaries through `oasis::Rom::load` and
keeps the existing five-target bounded scan. The `oasis.m68k.re-diff.v1` report
now adds detail only for aligned changed blocks: exact block ranges, direct and
conditional-fallthrough CFG edges, raw decoded instruction records, addressing
mode evidence, condition codes, constants, memory references and conservative
diff classifications. A changed immediate is `relocation_only` only when both
values point to corresponding decoded instructions at the same slice-relative
offset; otherwise it is `constant_changed`. Unsupported or unresolved evidence
remains explicitly classified and no semantics are assigned.

## Acceptance criteria

- [x] localize retail and beta changed block ordinal 10 and exact ranges;
- [x] report predecessor, direct successor and conditional fallthrough edges;
- [x] emit deterministic instruction-by-instruction raw/evidence detail;
- [x] classify relocation, constants, offsets, branches, addressing and unknowns;
- [x] add synthetic changed-block classification coverage;
- [x] add retail/beta exact-byte, block-range, instruction and CFG oracle;
- [x] keep the tooling bounded, developer-only and outside gameplay runtime;
- [x] pass Debug/Release CTest, USA oracle, GNU-equivalent link, file-limit and
  `git diff --check` before push.

## Verified result

Retail block ordinal 10 is `[0xA786,0xA792)` (12 bytes, 3 instructions);
beta block ordinal 10 is `[0xA736,0xA742)` (12 bytes, 3 instructions).
Each has one direct predecessor (`A6BA->A786` / `A66A->A736`), one taken
conditional edge (`A78E->A7D4` / `A73E->A784`) and one fallthrough edge
(`A78E->A792` / `A73E->A742`). The CFG topology is unchanged. The first
instruction is raw `2F3C0000A6BE` versus `2F3C0000A66E`; its immediate points
to corresponding local decoded instruction offsets, so the difference is
classified `relocation_only`. The remaining `4A46` and `6B000044` instructions
are byte-identical; condition code `0xB` is recorded for the branch.

## Known unknowns and hard boundaries

The result proves only bounded byte/decoder/CFG correspondence and a
relocation-only classification for this block. It does not prove function
semantics, the meaning of the pushed address, runtime behavior, or equivalence
outside the analyzed windows. No production behavior, emulator, recompiler,
wide similarity search, full-game tracing or M12 work was added.

## Exact next action

Stop at this verified block-detail checkpoint and await explicit instruction.
Do not start wider beta scans, full-game tracing, recompilation or M12.
