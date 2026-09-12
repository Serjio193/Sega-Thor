# M12-GFX-MAX — Maximum Graphics Closure Checkpoint

Status: bounded graphics fixed point for the currently proven loader graph;
the overall graphics system is not claimed complete while ten dynamic
`0x3820` producers and non-screen loader families remain source-blocked.

## Result

This pass closes the last unowned root of the already proven screen graphics
family:

| range | bytes | classification | proof |
| --- | ---: | --- | --- |
| `0x00C92C..0x00C980` | 84 | `STRUCTURED_DATA_CONFIRMED` / `SCREEN_GROUP_POINTER_TABLE` | `0x00C8F0` selects 21 fixed big-endian longword roots |

The candidate manifest is byte-exact to the canonical local ROM. No ROM,
decoded graphics, PNG, CRAM, SAT, or generated payload is tracked.

| metric | M12-GFX-2 after | M12-GFX-MAX candidate | delta |
| --- | ---: | ---: | ---: |
| SOURCE_OWNED bytes | 1,475,262 | 1,475,346 | +84 |
| SOURCE_OWNED percent | 46.8973159790% | 46.8999862671% | +0.0026702881 pp |
| UNKNOWN bytes | 1,670,466 | 1,670,382 | -84 |
| UNKNOWN ranges | 758 | 757 | -1 |

The largest remaining UNKNOWN interval is `[0x0C0000,0x11F360)` (389,984
bytes). It is not promoted: no independent graphics-loader boundary or
ownership contract closes it.

## Canonical ROM and machine evidence

The canonical local ROM is 3,145,728 bytes, CRC32 `C4728225`, SHA-1
`2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

The developer-only machine report is
`build/m12-gfxmax-screen-root-c/gfx_max_report.json` with SHA-256
`F4F4DB57BCACC1DED3D9CBC0001D8871BA348F0A74E9F1B1C90F607286CB9AE`.
Its materialized rebuilt ROM has the canonical SHA-256 and the same CRC32 and
SHA-1. The report schema is
`oasis.m68k.m12-gfx-max-closure.v1`.

## Complete known graphics-loader census

The census combines the published exact `0x3820` call graph with the screen
root/descriptor scan. Counts are static call sites, not gameplay frequency.

| loader family | callers | direct | inherited/indirect | blocked |
| --- | ---: | ---: | ---: | ---: |
| `0x00C326` bounded graphics loader | 1 | 1 | 0 | 0 |
| `0x00D3B2` indexed-resource loader | 1 | 0 | 1 | 0 |
| `0x00D406` shared resource loader | 3 | 0 | 3 | 2 |
| `0x01454C` menu graphics family | 4 | 4 | 0 | 0 |
| `0x024858`, `0x02A652`, `0x02B194` bounded loaders | 3 | 3 | 0 | 0 |
| `0x02DB24`, `0x02F662` resource families | 2 | 0 | 2 | 2 |
| `0x03A748` screen initialization | 1 | 1 | 0 | 0 |
| `0x03ACA8`, `0x03ADB4` tilemap loaders | 2 | 2 | 0 | 0 |
| `0x03B1D0` resource family | 3 | 0 | 3 | 3 |
| `0x03C04C` resource family | 1 | 0 | 1 | 1 |
| `0x03C1E8` sequential family | 3 | 0 | 3 | 0 |
| `0x03C59C` sequential family | 4 | 0 | 4 | 0 |
| `0x03C9CC` sequential family | 3 | 0 | 3 | 0 |
| `0x03CB9E` sequential family | 3 | 0 | 3 | 0 |
| `0x03CC4C` sequential family | 2 | 0 | 2 | 0 |
| `0x03CCCA` sequential family | 3 | 0 | 3 | 0 |
| `0x03CE98` sequential family | 5 | 0 | 5 | 0 |
| `0x03CFDE`, `0x03D228` graphics families | 3 | 3 | 0 | 0 |
| `0x03D59A` entity initializer | 1 | 0 | 1 | 1 |
| `0x03E4DC`, `0x03E7F4` graphics families | 4 | 3 | 1 | 1 |
| **total** | **52** | **17** | **35** | **10** |

The source-set census is 17 `EXACT_ROM_ADDRESS`, 2
`FINITE_TABLE_DERIVED_SET`, 23 `PARAMETERIZED_SEQUENTIAL_FAMILY`, and 10
`UNRESOLVED_PARAMETERIZED_FAMILY`. The published closure contains 34 unique
caller-derived resource starts, 82,861 unique resource bytes, and 47,389
newly promoted bytes. Its ten blockers are unchanged:

`0x00D54A` inherited entry field; `0x00D650` sequential first stream;
`0x02DB52` caller
argument; `0x02F6A0` caller argument; `0x03B236` RAM-mediated source;
`0x03B28A` RAM-mediated source; `0x03B2FE` sibling-call effect;
`0x03C07C` caller argument; `0x03D5AE` RAM-mediated entity record; and
`0x03E61A` inherited caller argument.

The bounded `0x00D54A` audit narrows, but does not close, its source blocker.
The exact `0x00F80E` helper saves and restores all address registers, so it
cannot produce `A4`. In the shared `0x00D406` body, `0x00D42E` sets
`A4 = entry A1 + 4`; `0x00D542` then loads `A0 = (A4)` before the
`0x00D54A` decompressor call. The consumed source is therefore a longword in
the inherited entry record/RAM field. Without a closed caller or runtime
register capture proving that field's ROM origin, this remains
`INHERITED_A1_FIELD_NOT_ROM_PROVEN` and no bytes are promoted.

The second shared-loader call at `0x00D650` is now bounded more precisely.
The first `0x3820` returns its advanced source and destination in `A0/A1`,
which `0x00D550` and `0x00D552` copy to `A4/A5`. When bit 2 of
`0x00FF16F1` is set, `0x00D64C` and `0x00D64E` restore those post-states as
the second call's `A0/A1`; `0x00D656` and `0x00D658` then retain its new
post-state. This proves `0x00D650` is a sequential continuation of the first
decompression, not an independent selector. It does not prove the first
source longword is a canonical ROM address, nor the exact compressed-stream
boundary consumed by the continuation. The blocker is therefore
`FIRST_STREAM_AND_CONTINUATION_NOT_ROM_PROVEN`; no bytes are promoted.

### Exact `0x00D406` direct-xref census

The separate machine report
`build/m12-gfx-loader-census.json` (`oasis.m68k.m12-gfx-loader-census.v1`,
SHA-256 `3E9F5D9D11AA3B38500859E35449CCEE945CA0BDBA993C50A8F0D461CA40270E`)
scans the canonical ROM for the exact six-byte `JSR abs.l,0x00D406` encoding.
It finds 173 direct call sites. This is a different quantity from the 52
`0x3820` call sites above: `0x00D406` is itself one of the shared loaders.

| relation | uses/calls | result |
| --- | ---: | --- |
| screen descriptor uses | 167 | 163 unique descriptors |
| descriptor `+0x1A` with exact direct `0x00D406` | 146 | direct screen-loader relation |
| descriptor `+0x1A` without exact direct call | 17 unique expected sites | 15 verified continuations; 2 unresolved |
| direct `0x00D406` not matched to a screen descriptor `+0x1A` | 27 | separate loader candidates; no ownership promotion |

The 27 unmatched direct call sites are `0x02CF9C`, `0x02D402`, `0x02DB40`,
`0x02DCF2`, `0x02DD8C`, `0x02DE58`, `0x02DFC2`, `0x02E084`, `0x02E0EE`,
`0x02E1F2`, `0x02E99A`, `0x030080`, `0x031B78`, `0x031C78`, `0x031DE8`,
`0x0320B8`, `0x032166`, `0x033516`, `0x035680`, `0x03697E`, `0x036A08`,
`0x036AC8`, `0x037B62`, `0x0390A4`, `0x0395D8`, `0x039C18`, and `0x039FA0`.
The 17 unique screen-descriptor expected sites without that direct encoding are
`0x02E98C`, `0x03007E`, `0x031B76`, `0x031C76`, `0x031DE6`, `0x0320B6`,
`0x03215E`, `0x03567A`, `0x03697C`, `0x036A06`, `0x036AC6`, `0x033512`,
`0x037B5E`, `0x038FD6`, `0x03959A`, `0x039C0C`, and `0x039F9A`.
The census is xref evidence only: it does not prove the caller's `A1`, a
record boundary, a compressed stream boundary, or source ownership.

The bounded follow-up relation pass adds a closed, non-owning continuation
verification to the same machine report. Fifteen of the 17 missing expected
sites have a nearest exact direct call within 16 bytes: `0x02E99A`, `0x030080`, `0x031B78`,
`0x031C78`, `0x031DE8`, `0x0320B8`, `0x032166`, `0x035680`, `0x03697E`,
`0x036A08`, `0x036AC8`, `0x033516`, `0x037B62`, `0x039C18`, and `0x039FA0`.
The two without such a bounded successor are `0x038FD6` and `0x03959A`.
The existing 68000 slice decoder and a closed verifier for the exact observed
instruction forms establish that all 15 paths reach the exact direct call and
none writes `A1` before it. `0x02E994` has a conditional branch directly to
`0x02E99A`, with fall-through through `0x02E996`; the other 14 verified paths
are straight fall-through. These are
`VERIFIED_SCREEN_DESCRIPTOR_CONTINUATION` relations, not ownership ranges;
they inherit the screen-root descriptor/A1 entry contract and do not promote
bytes.

The two unresolved entries are explicit blockers, not missing scan effort.
`0x038FD6` begins with `MOVEM.L D0/D1/D2/D3/D4,-(A7)` and a long RAM-state
routine with no bounded `0xD406` continuation; closing it requires a parent
caller path or targeted runtime provenance. `0x03959A` also begins with a
code-like `MOVEM.L`/`LEA.L $039DD2,A0` sequence and contains an unbounded sibling
call at `0x0395D8`; separating that sibling call from the descriptor entry
requires caller/A1 provenance. Neither site is promoted.

Five unmatched direct calls have a 26-byte, screen-descriptor-shaped prefix at
`call - 0x1A`, with a nonzero in-ROM pointer and four resource-id bytes below
the closed `0..107` screen-resource domain:
`0x02CF82 -> 0x1F4E64`, `0x02D3E8 -> 0x1FA32A`,
`0x02DCD8 -> 0x208C44`, `0x02E0D4 -> 0x211518`, and
`0x02E1D8 -> 0x2119D2`. The first four descriptor/stream relations are already
owned by earlier bounded transactions; the fifth is a new candidate over an
already-owned stream. None is promoted here: record-family boundaries and the
caller's `A1` still require independent proof.

## Screen-root and resource closure

The root table contains 21 longwords and points to the following finite group
roots: `0x02C6B8`, `0x02E56A`, `0x02F1A4`, `0x02F9B2`, `0x02FF80`,
`0x030218`, `0x030E56`, `0x031B50`, `0x0328D6`, `0x03361C`, `0x0341A4`,
`0x035528`, `0x036286`, `0x037B42`, `0x038AAC`, `0x02C6F2`, `0x038FBA`,
`0x03957C`, `0x039F7A`, `0x039F7C`, and `0x039F7E`.

The exact screen census contains 167 descriptor uses, 163 unique 26-byte
descriptors, and 159 unique streams. Stream boundaries range from
`0x1E7136` through `0x2603D2` and total 337,514 compressed bytes. All stream
intervals are already `LOCAL_ROM_DERIVED_ASSET`; all descriptor intervals
are already `STRUCTURED_DATA_CONFIRMED`. There is no new decoded-payload
commit and no stream promotion in this transaction.

The 108-entry resource pointer table `[0x05CE96,0x05D046)` and its 107 finite
targets were already closed by M12-GFX-2. The adjacent known loader census
was reviewed through `0x37D2`, `0x3820`, `0xD3B2`, `0xD406`, `0xD950`,
`0x2CBC`, `0x2E1E`, and `0x36D4`; only the already closed `0x37D2`/`0xD3B2`
relations produced a new finite Ancient source contract. No arbitrary selector
domain, decoder-validity-only range, or visual inference was promoted.

## Accounting and ambiguity

The table promotion adds 84 `STRUCTURED_DATA_CONFIRMED` bytes and removes one
UNKNOWN interval. No ASM bytes or new resource bytes are added here. The
M12-GFX-2 Carver checkpoint remains B `113/623,036`, F `56/58,789`, and G
`588/988,641`; this typed-data split was not treated as a new static-consumer
recovery sweep. The previous graphics closure already reduced G by 47,389
bytes through exact caller-derived streams; the remaining ten dynamic sources
are still fail-closed.

## Validation limits and next step

Passed locally for the original root-closure transaction: Python compile,
deterministic helper test, canonical ROM identity, full candidate
materialization, byte-for-byte rebuilt-ROM comparison, and the previous
Debug/Release CTest (143/143 each). The bounded `0x00D406` relation update
passes Python compilation, focused tests (4/4), deterministic canonical-ROM
JSON generation, Debug/Release builds, full Debug/Release CTest (144/144
each), the GNU/Linux-equivalent WSL Release build with 143/143 non-size-gate
tests, and `git diff --check`. A full Linux CTest attempt reached the
`project_file_line_limit` test and was stopped after approximately 90 seconds
of `/mnt/c` filesystem scanning; excluding that known slow gate, all 143 tests
passed. The identical source-size gate passed in Windows Debug and Release in
163.05 s and 161.59 s. No source violation was observed.
The second full-layout `vasmm68k_mot` attempt was not
successful because the existing generated baseline layout contains duplicate
labels such as `loc_00B856`; the report records use of the independently
byte-exact baseline rebuilt ROM as a verification fallback. This is not
claimed as a fresh assembler round-trip.

The next graphics step is to resolve the two remaining screen sites and the
five descriptor-shaped records, then return to the ten dynamic `0x3820`
producers. No candidate becomes owned without a proven source and exact
boundary; M13 and ASM-to-C++ migration remain out of scope.

Implementation SHA: `6a773cae39dffefac82bdcff5b357a7122f032ba`.
Exact implementation CI: GitHub Actions run `34660611094` (success).
Final publication SHA: `6a773cae39dffefac82bdcff5b357a7122f032ba`.
Exact final publication CI / publication HEAD CI: GitHub Actions run
`34660611094` (success).
