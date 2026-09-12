# M12-GFX-MAX — Maximum Graphics Closure Checkpoint

Status: bounded graphics fixed point for the currently proven loader graph;
the overall graphics system is not claimed complete while ten dynamic
`0x3820` producers and remaining non-screen loader paths remain source-blocked.

## Result

This pass closes the last unowned root of the already proven screen graphics
family and one exact descriptor-family candidate:

| range | bytes | classification | proof |
| --- | ---: | --- | --- |
| `0x00C92C..0x00C980` | 84 | `STRUCTURED_DATA_CONFIRMED` / `SCREEN_GROUP_POINTER_TABLE` | `0x00C8F0` selects 21 fixed big-endian longword roots |
| `0x02E1D8..0x02E1EE` | 22 | `STRUCTURED_DATA_CONFIRMED` / `GRAPHICS_DESCRIPTOR_RECORD` | repeated 22-byte family, exact `+4` stream pointer, IDs `78..81`, exact stream `0x2119D2..0x211F79` |

The candidate manifest is byte-exact to the canonical local ROM. No ROM,
decoded graphics, PNG, CRAM, SAT, or generated payload is tracked.

| metric | M12-GFX-2 after | M12-GFX-MAX candidate | delta |
| --- | ---: | ---: | ---: |
| SOURCE_OWNED bytes | 1,475,262 | 1,475,368 | +106 |
| SOURCE_OWNED percent | 46.8973159790% | 46.9006856283% | +0.0033696493 pp |
| UNKNOWN bytes | 1,670,466 | 1,670,360 | -106 |
| UNKNOWN ranges | 758 | 758 | 0 |

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

The direct sibling-helper census is separate from the 52 `0x3820` caller
count. It is complete for the three proven helper targets below:

| helper | direct calls | bounded body | classification | ROM-source result |
| --- | ---: | --- | --- | --- |
| `0x00D950` | 12 | `[0x00D950,0x00D9A4)` / 84 bytes | VDP transfer, local `0x00D962` | none; helper-only |
| `0x002CBC` | 4 | `[0x002CBC,0x002CE4)` / 40 bytes | VDP fill | none; helper-only |
| `0x002E1E` | 24 | `[0x002E1E,0x002E78)` / 90 bytes | RAM/state, local `0x002F6E` | none; not a ROM loader |

These 40 direct xrefs are fully accounted for by exact canonical call
encodings and body fingerprints. They do not change the 52-caller resource
accounting or SOURCE_OWNED totals.

The source-set census is 17 `EXACT_ROM_ADDRESS`, 2
`FINITE_TABLE_DERIVED_SET`, 23 `PARAMETERIZED_SEQUENTIAL_FAMILY`, and 10
`UNRESOLVED_PARAMETERIZED_FAMILY`. The published closure contains 34 unique
caller-derived resource starts, 82,861 unique resource bytes, and 47,389
newly promoted bytes. Its ten blockers are unchanged:

`0x00D54A` inherited entry field; `0x00D650` sequential first stream;
`0x02DB52` D406 post-source; `0x02F6A0` D406 post-state; `0x03B236`
`A5+4` field; `0x03B28A` `A3` argument; `0x03B2FE` `A4` argument;
`0x03C07C` inherited `A1`; `0x03D5AE` RAM-mediated entity record; and
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

The local slice containing `0x02DB52` is also bounded. At `0x02DB40` it calls
`0x00D406`; `0x02DB46` then loads `A0` from `0x00FF17AA`, the post-source
field written by `0x00D65A`, and `0x02DB4C` sets `A1 = 0x00FF2FA8` before the
`0x02DB52` call. This proves a `0x00D406` post-source continuation rather
than an independent caller argument. The incoming `D0` copied to `A0` at
`0x02DB3C` is still inherited, and the exact source/boundary remains
unproven; the blocker is `D406_POST_SOURCE_NOT_ROM_PROVEN` and no bytes are
promoted.

The adjacent `0x02F6A0` slice has the same bounded predecessor relation.
`0x02F67C` calls `0x00D406`; `0x02F692` and `0x02F698` then load `A0` and
`A1` from `0x00FF17AA` and `0x00FF17AE`, the post-source/post-destination
fields written by `0x00D65A` and `0x00D660`, before `0x02F6A0` calls
`0x003820`. The source entering `D406` and the exact stream boundary remain
unproven, so this is `D406_POST_STATE_NOT_ROM_PROVEN`; no bytes are promoted.

The `0x03B1D0` family has one shared output and three bounded source arms.
At `0x03B236`, `A0 = 4(A5)`; at `0x03B28A`, `A0 = A3`; and at `0x03B2FE`,
`A0 = A4`; every arm sets `A1 = 0x00FF316C`. The sibling helpers before
the third arm are also bounded: `0x002CBC` does not write `A4`, and
`0x00D950` saves/restores data registers and `A2` while its `0x00D962` body
does not write `A4`. The remaining blockers are therefore exactly
`A5_FIELD_NOT_ROM_PROVEN`, `A3_ARGUMENT_NOT_ROM_PROVEN`, and
`A4_ARGUMENT_NOT_ROM_PROVEN`; no source bytes are promoted.

The `0x03C07C` call is reached from the exact `0x03BF86` initialization path.
At `0x03C074`, `A0` is loaded from the direct ROM literal `0x00172168`, then
`A1` is copied only to `A3` before `0x003820`; the preceding `0x03C956` helper
does not write `A1`, and `0x03BF86` preserves all address registers. The
remaining blocker is therefore `A1_INHERITED_NOT_ROM_PROVEN`. The direct
source literal is recorded, but no resource bytes are promoted because the
destination and complete source-to-output contract remain unproven.

The final two dynamic sites add no promotable source relation from the exact
retained slices. `0x03D5AE` begins with `JSR 0x003820` and then writes only
entity-state fields at `0x00FF19A2`, `0x00FF19A6`, `0x00FF19AA`, and
`0x00FF19AC`; no local `A0`/`A1` definition closes the RAM-mediated entity
source. `0x03E61A` likewise calls `0x003820` before any new `A0`/`A1`
definition; its later `0x03E662` and `0x03E704` arms load the already closed
`0x0016943C` stream into `0x00FF2FA8`. The first call remains
`CALLER_ARGUMENT_NOT_ROM_PROVEN`, and no bytes are promoted.

### Exact `0x00D406` direct-xref census

The separate machine report
`build/m12-gfx-loader-census-extended.json` (`oasis.m68k.m12-gfx-loader-census.v1`,
SHA-256 `55B7A55F6C67017996C859350E2A18841BAC8C7D9A84BF6128B965A84DAED479`)
scans the canonical ROM for the exact six-byte `JSR abs.l,0x00D406` encoding.
It finds 173 direct call sites. This is a different quantity from the 52
`0x3820` call sites above: `0x00D406` is itself one of the shared loaders.

| relation | uses/calls | result |
| --- | ---: | --- |
| screen descriptor uses | 167 | 163 unique descriptors |
| descriptor `+0x1A` with exact direct `0x00D406` | 146 | direct screen-loader relation |
| descriptor `+0x1A` without exact direct call | 17 unique expected sites | 17 verified continuations; 0 unresolved |
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
The two longer continuations are `0x038FD6 -> 0x0390A4` and
`0x03959A -> 0x0395D8`.
The existing 68000 slice decoder and a closed verifier for the exact observed
instruction forms establish that all 15 paths reach the exact direct call and
none writes `A1` before it. `0x02E994` has a conditional branch directly to
`0x02E99A`, with fall-through through `0x02E996`; the other 14 verified paths
are straight fall-through. These are
`VERIFIED_SCREEN_DESCRIPTOR_CONTINUATION` relations, not ownership ranges;
they inherit the screen-root descriptor/A1 entry contract and do not promote
bytes. The exact developer-only `re_slice_decoder` CFG reports cover both
longer entries with zero unsupported instructions before the call, no return
before the call, and all reachable paths reaching the exact D406 target. Their
pre-call byte hashes are
`53A38241102D56692BAA4176D467ED204EBC2EF6A6FBB21E429F7E1C6820CB71` and
`D4D24EB99DB0998278F3AEC1B4BA0735DC3299F4A59AED089CE6781EC9A258D`.
The exact paths write no `A1`; both relations remain non-owning.

Five unmatched direct calls have a 26-byte, screen-descriptor-shaped prefix at
`call - 0x1A`, with a nonzero in-ROM pointer and four resource-id bytes below
the closed `0..107` screen-resource domain:
`0x02CF82 -> 0x1F4E64`, `0x02D3E8 -> 0x1FA32A`,
`0x02DCD8 -> 0x208C44`, `0x02E0D4 -> 0x211518`, and
`0x02E1D8 -> 0x2119D2`. The first four descriptor/stream relations are already
owned by earlier bounded transactions. The fifth now has an independent
22-byte family contract: IDs `0x4E..0x51` are in the closed screen domain, the
`+4` pointer is the exact stream start `0x2119D2`, and the graphics census
closes that stream at `0x211F79` with 18,712 decompressed bytes. The narrow
candidate promotion adds only `0x02E1D8..0x02E1EE`; it does not promote the
four-byte gap before the D406 call or infer caller `A1` from it.

### Exact `0x0037D2` wrapper-family census

The developer-only wrapper census
`build/m12-gfx-37d2-census.json` (`oasis.m68k.m12-gfx-37d2-census.v1`)
finds all seven direct absolute `JSR 0x0037D2` sites. The wrapper's exact
internal `BSR 0x003820` at `0x0037D8` is therefore accounted for as a sibling
loader edge rather than being hidden by the 52-site absolute `0x3820` census.
Its SHA-256 is
`EC247A5304BE12B458515CB7CE081A2FF54A1EA0C9DED51AEC7FBEC8F975068E`; a
deterministic repeat produced the identical hash.

Five sites have an exact local setup of `LEA source,A0`, `LEA destination,A1`,
`MOVE.W #value,D0`, and `JSR 0x0037D2`. Their deterministic Ancient source
spans are already wholly `LOCAL_ROM_DERIVED_ASSET`; this census adds no bytes:

| wrapper call | source | end | compressed bytes | D0 | destination |
| ---: | ---: | ---: | ---: | ---: | ---: |
| `0x03C0DE` | `0x1744EE` | `0x17502A` | 2,876 | `0x4B00` | `0x00FF2FA8` |
| `0x03C2BE` | `0x18CCD0` | `0x18CF97` | 711 | `0x7080` | `0x00FF2FA8` |
| `0x03C632` | `0x18EC26` | `0x18F214` | 1,518 | `0x6A40` | `0x00FF2FA8` |
| `0x03CA40` | `0x1911EA` | `0x191F09` | 3,359 | `0x2580` | `0x00FF2FA8` |
| `0x03CD54` | `0x19911A` | `0x199CBA` | 2,976 | `0x6400` | `0x00FF2FA8` |

The remaining direct calls are explicit negative evidence: `0x00D9B2`
inherits its source through `A6` in `0x00D9A4`, while `0x02F6B6` inherits the
post-source state of the preceding `0x00D406`. Neither creates a new exact ROM
source set. The five direct spans total 11,440 already-owned bytes; promotion
is zero and the canonical ROM remains unchanged.

### Exact `0x00D3B2` indexed-loader/root census

The developer-only indexed-loader census
`build/m12-gfx-d3b2-census.json` (`oasis.m68k.m12-gfx-d3b2-census.v1`)
finds all seven direct absolute `JSR 0x00D3B2` sites. Every call has an exact
`MOVE.W #selector,D0`, `MOVE.W #destination,D1` setup immediately before the
call:

| call site | selector | D1 VRAM word address |
| ---: | ---: | ---: |
| `0x02CFAA` | `3` | `0x4000` |
| `0x02CFB8` | `4` | `0x5000` |
| `0x02D410` | `0x57` | `0x4000` |
| `0x032174` | `0x23` | `0x4000` |
| `0x032182` | `0x24` | `0x5000` |
| `0x032884` | `0x23` | `0x4000` |
| `0x032892` | `0x24` | `0x5000` |

The census hash is
`DDBA1EF1EC7D5B89830DA74A0835BC6506D8BBB7C2283440AD5BCE8B32653895`; a
deterministic repeat produced the identical hash. The proven root table is
`[0x05CE96,0x05D046)`: 108 four-byte entries, entry 0 null, and finite child
indices 1..107. All 107 child pointers match exact existing
`LOCAL_ROM_DERIVED_ASSET` manifest spans totaling 238,087 compressed bytes.
The selected indices resolve to five already-owned spans:
`3: [0x1AE1A8,0x1AE8AA)`, `4: [0x1AE8AA,0x1AF033)`,
`35: [0x1BF148,0x1BF953)`, `36: [0x1BF954,0x1BFFAA)`, and
`87: [0x1DBD46,0x1DC4DE)`. This closes the indexed loader/root family without
adding ownership; table shape and selector validity were not used alone for
promotion.

### Exact `0x0036D4` wrapper census

The developer-only wrapper census
`build/m12-gfx-36d4-census.json` (`oasis.m68k.m12-gfx-36d4-census.v1`)
finds all six direct absolute `JSR 0x0036D4` sites:
`0x03D25E`, `0x03D2D8`, `0x03D512`, `0x03DD70`, `0x03DF14`, and `0x03E544`.
Every caller has the exact local form `MOVE.W #value,D0`,
`MOVE.L #0x60000003,A5`, then the wrapper call. The D0 values are `0x20`,
`0x4000`, `0x20`, `0x20`, `0x20`, and `0x20`.

The bounded body `[0x0036D4,0x00372A)` is 86 bytes. It sets the shared RAM
destination `0x00FF2FA8`, calls `0x003820` at `0x0036EA`, then calls
`0x002CBC` at `0x003720` before its exact `RTS`. Its source `A0` is inherited
from the caller and is not ROM-proven; this closes the wrapper as a classified
RAM-mediated graphics consumer with blocker `CALLER_A0_NOT_ROM_PROVEN`, not as
a new source range. Promotion is zero.

## Screen-root and resource closure

The root table contains 21 longwords and points to the following finite group
roots: `0x02C6B8`, `0x02E56A`, `0x02F1A4`, `0x02F9B2`, `0x02FF80`,
`0x030218`, `0x030E56`, `0x031B50`, `0x0328D6`, `0x03361C`, `0x0341A4`,
`0x035528`, `0x036286`, `0x037B42`, `0x038AAC`, `0x02C6F2`, `0x038FBA`,
`0x03957C`, `0x039F7A`, `0x039F7C`, and `0x039F7E`.

The exact screen census contains 167 descriptor uses, 163 unique 26-byte
descriptors, and 159 unique streams. Stream boundaries range from
`0x1E7136` through `0x2603D2` and total 337,514 compressed bytes. All stream
intervals are already `LOCAL_ROM_DERIVED_ASSET`; the candidate descriptor is
now also `STRUCTURED_DATA_CONFIRMED`. There is no new decoded-payload commit
and no stream promotion in this transaction.

The 108-entry resource pointer table `[0x05CE96,0x05D046)` and its 107 finite
targets were already closed by M12-GFX-2. The adjacent known loader census
was reviewed through `0x37D2`, `0x3820`, `0xD3B2`, `0xD406`, `0xD950`,
`0x2CBC`, `0x2E1E`, and `0x36D4`. The direct `0x37D2` census closes its
seven-call evidence set: five exact source spans are already owned and two
paths remain inherited/post-state blocked. The new `0x36D4` census closes its
six-call wrapper set as RAM-mediated with inherited-A0 source. No arbitrary
selector domain, decoder-validity-only range, or visual inference was promoted.

The remaining proven sibling-helper census is now also complete for the
directly reachable `0x00D950`, `0x002CBC`, and `0x002E1E` targets. It finds 12,
four, and 24 direct calls respectively, for 40 exact xrefs. Their bounded
bodies are `[0x00D950,0x00D9A4)` (84 bytes), `[0x002CBC,0x002CE4)` (40 bytes),
and `[0x002E1E,0x002E78)` (90 bytes), with deterministic body fingerprints in
`build/m12-gfx-sibling-census.json` (`oasis.m68k.m12-gfx-sibling-census.v1`,
SHA-256 `25A787BDF61088BCD18870AD3293FDDF78F0F5B4C1D8E0B55CCF87EB53E099FC`).
`0x00D950` is a VDP-transfer helper with local `0x00D962`, `0x002CBC` is a
VDP-fill helper, and `0x002E1E` is a RAM/state helper with local `0x002F6E`.
None contains a ROM source operand or a `0x003820` decompressor edge. These
three xref sets are therefore classified and closed as non-owning helpers;
promotion remains zero.

## Accounting and ambiguity

The table promotion adds 84 and the candidate promotion adds 22
`STRUCTURED_DATA_CONFIRMED` bytes. The candidate splits one remaining UNKNOWN
interval, so the overall UNKNOWN-range count returns to the M12-GFX-2 count
while UNKNOWN bytes fall by 106. No ASM bytes or new resource bytes are added
here. The M12-GFX-2 Carver checkpoint remains B `113/623,036`, F `56/58,789`, and G
`588/988,641`; this typed-data split was not treated as a new static-consumer
recovery sweep. The previous graphics closure already reduced G by 47,389
bytes through exact caller-derived streams; the remaining ten dynamic sources
are still fail-closed.

## Validation limits and next step

Passed locally for the original root-closure transaction: Python compile,
deterministic helper test, canonical ROM identity, full candidate
materialization, and byte-for-byte rebuilt-ROM comparison. This continuation
and descriptor-candidate update passes Python compilation, focused helpers
(`6/6` existing graphics assertions plus `3/3` new sibling/helper assertions),
deterministic canonical-ROM JSON generation, Debug/Release builds,
full Windows Debug/Release CTest (`149/149` each, including the source-size
gate), the WSL build with the seven relevant graphics CTest helpers (`7/7`), and
`git diff --check`. The candidate materialization rebuilt the canonical ROM
byte-for-byte with CRC32 `C4728225`,
SHA-1 `2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
A full Linux CTest attempt previously reached the
`project_file_line_limit` test and was stopped after approximately 90 seconds
of `/mnt/c` filesystem scanning; the current Linux build and three relevant
graphics helpers pass. The Windows source-size gate passed in Debug and
Release after the CMake registration was kept at the existing 500-line limit.
No source violation was observed.
The second full-layout `vasmm68k_mot` attempt was not
successful because the existing generated baseline layout contains duplicate
labels such as `loc_00B856`; the report records use of the independently
byte-exact baseline rebuilt ROM as a verification fallback. This is not
claimed as a fresh assembler round-trip.

The screen continuation sites are now all closed, and the five descriptor-
shaped records are accounted for: four were already owned and the fifth is
promoted under the exact 22-byte contract above. The next graphics step is to
obtain new caller-closed or targeted register evidence for the ten dynamic
`0x3820` producers and the 27 unmatched non-screen `0x00D406` candidates.
Existing exact static slices have reached evidence exhaustion for the ten
dynamic producers; no candidate becomes owned without a proven source and
exact boundary. The directly reachable sibling helpers covered by the census
are now classified and should not be reopened without new source evidence.
M13 and ASM-to-C++ migration remain out of scope.

Implementation SHA: `3fe64956be986bfe3e76f3d6a4251abba4525865`.
Exact implementation CI: GitHub Actions run `34667380039` (success).
Final publication SHA: `3fe64956be986bfe3e76f3d6a4251abba4525865`.
Exact final publication CI / publication HEAD CI: GitHub Actions run
`34667380039` (success).
