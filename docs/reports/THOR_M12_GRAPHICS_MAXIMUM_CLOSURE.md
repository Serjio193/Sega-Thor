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

`0x00D54A` helper output; `0x00D650` register value; `0x02DB52` caller
argument; `0x02F6A0` caller argument; `0x03B236` RAM-mediated source;
`0x03B28A` RAM-mediated source; `0x03B2FE` sibling-call effect;
`0x03C07C` caller argument; `0x03D5AE` RAM-mediated entity record; and
`0x03E61A` inherited caller argument.

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

Passed locally: Python compile, deterministic helper test, canonical ROM
identity, full candidate materialization, byte-for-byte rebuilt-ROM comparison,
Debug/Release builds, full Debug/Release CTest (143/143 each), the
GNU/Linux-equivalent WSL Release build and helper CTest, and `git diff --check`.
The second full-layout `vasmm68k_mot` attempt was not
successful because the existing generated baseline layout contains duplicate
labels such as `loc_00B856`; the report records use of the independently
byte-exact baseline rebuilt ROM as a verification fallback. This is not
claimed as a fresh assembler round-trip.

The next graphics step is to close one of the ten dynamic producers only when
its upstream descriptor/register/RAM chain proves an exact ROM source. If all
ten remain blocked after that bounded review, the next subsystem is the
non-graphics UNKNOWN census; M13 and ASM-to-C++ migration remain out of scope.

Implementation SHA and exact final CI are added after the focused change is
committed and published.
