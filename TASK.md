# Current Task

TASK: M11.5 first bounded ROM Atlas prototype
WHY: Combine accepted bounded RE evidence into a reproducible machine-readable
map without whole-ROM discovery, semantic invention or runtime changes.
CURRENT MILESTONE: M11.5 follow-up under M11
MILESTONE UNDERSTANDING CONFIDENCE: 95%
CURRENT SLICE UNDERSTANDING CONFIDENCE: 96%
SLICE MODE: RE_TOOLING_ONLY
STATUS: COMPLETE

## Scope and method

`oasis_re_atlas` builds `oasis.m68k.re-atlas.v1` from a typed manifest and the
existing `re_program`, `re_trace` and `re_diff` APIs. It records raw-address
entries for proven code/table evidence, confirmed versus bounded ranges,
callers/callees, ROM/RAM refs, unresolved/unsupported evidence, beta
correspondence, dynamic `0xA6A4` facts, native status and coverage. Markdown is
documentation, not the primary database; bounded windows are not ownership.

## Acceptance criteria

- [x] include only the 9 documented code targets and 4 named table targets;
- [x] preserve exact boundaries separately from bounded evidence windows;
- [x] expose deterministic calls, refs, unresolved evidence and queries;
- [x] include accepted USA/Beta correspondences and A6A4 dynamic facts;
- [x] detect incompatible evidence overlap without choosing silently;
- [x] emit deterministic JSON/text and synthetic model/query tests;
- [x] pass USA+Beta oracle, Debug/Release CTest, GNU-equivalent link,
  file-limit and `git diff --check` before push.

## Verified result

Atlas contains 13 entries: 5 bounded/exact code entries from the previous
program report plus `0x82AE`, `0x7A28`, `0x938E`, `0x9BF2`, and four named table
starts. The local USA/Beta oracle reproduces exact correspondences at
`0x3820/0x37D0`, `0x60004`, `0x82AE/0x825E`, `0x7A28/0x79D8`, and structural
`0xA6A4/0xA654` with changed block 10. Dynamic evidence remains raw:
`A7D4->FF2954`, `A7DE->FF2976`, `A7E2->A7E4`.

## Known unknowns and hard boundaries

Some table sizes, bounded function ownership, unresolved register refs and
routine semantics remain UNKNOWN. Atlas is developer-only and does not add an
emulator, whole-ROM scan, dynamic-tracing expansion, recompiler, generated
C++ or gameplay-runtime dependency.

## Exact next action

Stop at this verified Atlas checkpoint. Await explicit instruction; do not
start M12, whole-ROM discovery, wider tracing, similarity search or
recompilation.
