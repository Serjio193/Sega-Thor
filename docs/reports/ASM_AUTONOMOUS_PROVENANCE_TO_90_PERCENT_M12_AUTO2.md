# M12-AUTO17 — bounded provenance toward a 90% source-owned ROM map

Status: `M12_AUTO17_BLOCKED_BELOW_90_INHERITED_BASELINE_EXACTNESS`.

M12-AUTO17 adds two exact runtime-correlated graphics streams totaling 13,166
bytes after AUTO16: `0x15E052..0x160E19` and `0x2119D2..0x211F79`. Existing
canonical GPGX ROM-reader correlation identifies the executed `0x003830` PC,
inside the exact `0x3820` decoder, as the first reader for both ranges. The
local decoder consumes exactly the observed boundaries, and independent static
pointer literals select each range. Surrounding tables and decoder-census-only
candidates remain UNKNOWN. The canonical ROM remains byte-exact and M13/C++
migration remain prohibited.

This report records the strongest byte-exact M12 checkpoint reached without
starting M13, native gameplay/runtime C++ migration, or emulator expansion.
The result remains deliberately below the requested 90% gate because the remaining
ROM bytes do not have independently closed code, data, or asset provenance.

## Identity and checkpoints

| Item | Value |
| --- | --- |
| Canonical ROM size | 3,145,728 bytes |
| Canonical CRC32 | `C4728225` |
| Canonical SHA-1 | `2944910c07c02eace98c17d78d07bef7859d386a` |
| Canonical SHA-256 | `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263` |
| Baseline Git SHA | `b0cb5d94d9aff75c1ac1430a996006816b9bce7a` |
| Final local transaction | `build/m12-auto17-runtime-graphics-transaction-d/materialized/manifest.json` |

| Checkpoint | Source-owned bytes | Percentage |
| --- | ---: | ---: |
| M12-AUTO | 279,468 | 8.884048461% |
| Screen descriptor graph | 621,220 | 19.748051961% |
| Exact Z80 upload | 629,412 | 20.008468628% |
| Consumer graph + fixed records | 718,252 | 22.832616170% |
| Indexed script table + streams | 729,658 | 23.195203145% |
| Nested level table + records | 731,827 | 23.264153798% |
| Exact lookup tables | 732,467 | 23.284498851% |
| Fixed-stride 50-record table | 734,067 | 23.335361481% |
| Direct graphics consumers | 774,132 | 24.608993530% |
| Direct graphics chain continuation | 795,148 | 25.277074178% |
| Erased alignment padding | 927,778 | 29.493268331% |
| Exact code continuation | 928,000 | 29.500325521% |
| Count-bounded record streams | 978,026 | 31.090609233% |
| Exact code continuation after record streams | 979,118 | 31.125322978% |
| Exact BCEA field3 sentinel lists | 986,634 | 31.364250183% |
| Physical CC-B0 group pointer table | 986,762 | 31.3683191935% |
| Constant-D0 CC-B0 selected relative-pointer slots | 986,792 | 31.3692728678% |
| Exact runtime-correlated probe slices | 988,222 | 31.4147313436% |
| Runtime-correlated graphics streams | 1,001,388 | 31.8332672119% |

The 90% threshold is 2,831,156 bytes; the current gap is 1,829,768 bytes.

## Final ownership census

| Class | Bytes | Percent |
| --- | ---: | ---: |
| 68000 `CODE_VERIFIED` | 51,650 | 1.6419092814% |
| Header/vector ASM | 512 | 0.016276042% |
| Confirmed structured data | 87,525 | 2.7823448181% |
| Confirmed alignment padding | 132,677 | 4.217688243% |
| Local ROM-derived assets | 729,024 | 23.1750488281% |
| **SOURCE_OWNED** | **1,001,388** | **31.8332672119%** |
| Remaining `UNKNOWN` blob | 2,144,340 | 68.1667327881% |

The asset total consists of the original 107-entry compressed-resource graph,
159 screen-descriptor primary streams, 29 direct 68000 graphics streams, the
seven streams selected by the bounded `0x03B8DE` table, five direct chain
continuations, and two runtime-correlated streams read by `0x3820`. The structured
data total includes the existing exact ASM-backed data, eight fixed 1208-byte
records, the 196-byte indexed table, and 98 exact streams totalling 11,210
bytes, the 64-byte nested outer table, 492 bytes of count-bounded nested
groups, and 185 pointer-backed records totalling 1,613 bytes. The one
8-byte unindexed record-shaped span remains UNKNOWN. The structured-data total
also includes the 50-record table at `0x5D046`, the 64-entry `0x0EEE` word
lookup table, and the 512-byte bounded byte lookup table. The Z80 8192-byte
image is included in the ASM total and is also
recorded as `Z80_SOURCE_OWNED_BYTES=8192`.

AUTO9 additionally owns 132,630 bytes as exact erased alignment padding in 16
complete `0xFF` runs. The runs are accepted only when they are at least 256
bytes long, end on a 4 KiB ROM boundary, and do not overlap an existing owned
range. No zero-filled or mixed range is promoted by this rule.

AUTO10 additionally owns 222 bytes as two exact caller-backed 68000 islands.
The automatic promoter attempted 14 eligible candidates and accepted two;
the remaining 12 were rejected as unsupported exact IR. Its source mapping now
preserves all baseline entries whose emitted artifact type is ASM, including
the header/vector entry.

AUTO11 additionally owns the exact 99-record table and 46,858 bytes of
count-bounded six-byte record streams. The parser contract is raw and
structural; no semantic field names are asserted.

AUTO12 additionally owns 1,092 exact ASM bytes from 12/12 caller-backed
candidate slices after systemic DIVU/DIVS/SBCD decoder normalization fixes.

AUTO13 additionally owns 7,516 bytes from 100 BCEA-selected field3 list views.
The selector offsets are exactly `-20`, `0`, `20`, and `40`; each accepted view
follows a signed relative pointer, 44-byte positive rows, and a negative-key
sentinel. Only the five merged ranges are promoted; the rest of the field3
container remains UNKNOWN.

AUTO14 additionally owns 128 bytes for the physical 32-entry longword pointer
table at `0x04371E..0x04379E`, selected by the exact `0x00CCB0` high-byte
shift. Nested target subtables remain UNKNOWN. The local transaction uses
`INHERITED_BASELINE_FULL_ROM` exactness because no external vasm executable is
installed; canonical full-ROM equality and inherited ASM/blob byte equality
passed. A fresh assembler round-trip remains required when vasm is available.

AUTO15 additionally owns 15 unique 16-bit relative-pointer slots (30 bytes)
selected by exact constant-D0 callers of `0x00CA24`. Every slot resolves to an
even valid canonical-ROM target through the AUTO14 group table. Dynamic callers,
unproven low-byte bounds, and nested table extents remain UNKNOWN. The local
transaction uses `INHERITED_BASELINE_FULL_ROM` exactness because no external
vasm executable is installed; canonical full-ROM equality, inherited ASM/blob
equality, and all 30 new canonical slices passed. A fresh assembler round-trip
remains required when vasm is available.

AUTO16 additionally owns nine exact runtime-correlated 68000 probe slices
totalling 1,430 bytes. The local decoder metadata, ASM artifacts and binary
artifacts agree on every slice, and each slice contains at least one observed
instruction start in the canonical runtime evidence. This PC-level
corroboration does not assign gameplay semantics or close adjacent mixed
ranges. The local transaction uses `INHERITED_BASELINE_FULL_ROM` exactness
because no external vasm executable is installed; canonical full-ROM equality
and every new canonical slice passed. A fresh assembler round-trip remains
required when vasm is available.

## Provenance graph

Nodes:

1. Canonical ROM identity and byte span `[0x000000,0x300000)`.
2. Exact 68000 code islands accepted by the existing round-trip/caller gate.
3. Exact Z80 upload loop at `0x06134E` and image `[0x062E38,0x064E38)`.
4. Original 107-entry graphics pointer table `[0x05CE96,0x05D046)`.
5. Screen group table `0x00C92C` and 26-byte descriptors.
6. Direct graphics consumers at `0x3820` and `0x37D2`.
7. Bounded 16-byte table `0x03B8DE`, selector `0x03A9EE`, consumer
   `0x03B1D0`, and the exact graphics decoder.
8. Fixed-record parser at `0x00129A`/copy routine `0x0012E8`.
9. Eight fixed records `[0x200009,0x2025C9)`.
10. Indexed script table `[0x51514,0x515D8)` and 98 NUL-terminated streams
    `[0x515FC,0x541FD)`.
11. Nested level table `[0x5D918,0x5D958)`, contiguous groups through
    `0x5DB44`, and 185 pointer-backed records through `0x5E1A0`.
12. Exact lookup tables `[0x5D686,0x5D706)` and `[0x5D706,0x5D906)`.
13. Fixed-stride table `[0x5D046,0x5D686)` and its four exact consumers.
14. Seven direct graphics streams selected by exact consumers and A0 continuation.
15. Five direct graphics chain continuations selected from three exact chain
    anchors and sequential A0 advancement.
16. Remaining ROM spans retained as canonical-local-ROM blobs.
17. Exact erased-alignment runs accepted by the `0xFF`/minimum-length/4 KiB
    boundary contract.
18. Two exact caller-backed 68000 islands accepted by the automatic
    reassembly/vasm/full-ROM gate.
19. Exact `0x3F2FA` selector family → 99 fixed-stride records → shared
   count/DBF consumer → 78 stream views, merged only across covered bytes.
20. BCEA field3 bases → exact selector transform → signed relative list pointer
    → 44-byte rows → negative-key sentinel; five merged list ranges.
21. Exact `0x00CCB0` high-byte selector → physical 32-entry longword table
    `[0x04371E,0x04379E)`; nested target subtables remain UNKNOWN.
22. Constant-D0 callers → exact CC-B0 group/index arithmetic → 15 unique
   signed relative-pointer slots; dynamic callers remain UNKNOWN.
23. Runtime-observed probe entries → decoder-bounded exact ASM/binary slices;
   adjacent mixed code/data remains UNKNOWN.

Edges:

- 68000 loader → exact Z80 image → Z80 source-owned ASM.
- Resource table → pointer → graphics decoder → consumed source boundary →
  local ROM-derived asset.
- Screen group table → descriptor relative pointer → descriptor primary
  stream → graphics decoder → local ROM-derived asset.
- `0x03A9EE` table selector → 16-byte records at `0x03B8DE` → `0x03B1D0` →
  graphics decoder → seven additional local ROM-derived assets.
- Record parser/copy routine → eight fixed 302-longword records → confirmed
  structured data.
- `0x00C2EC` index loop → `0x0051514` relative-offset table → `0x00C326`
  stream parser → 98 exact NUL-terminated structured-data streams.
- Exact 68000 nested lookup sites → outer table → count-bounded group → inner
  relative pointer → bounded NUL-terminated record family.
- Exact lookup consumers → bounded word/byte table ranges.
- Four exact fixed-stride consumers → 50-record table `[0x5D046,0x5D686)`.
- Direct `0x3820` consumers and decoder-advanced `A0` → seven exact streams.
- Three exact chain anchors and sequential `0x3820` consumers → five new exact
  continuation streams.
- Complete UNKNOWN `0xFF` runs with minimum length and 4 KiB end boundary →
  confirmed erased alignment padding.
- Existing ASM-backed baseline entries and new exact candidate slices →
  automatic trial materialization and full-ROM equality.
- Exact record selectors and shared six-byte `DBF` consumers → count-bounded
  structured stream ranges.
- BCEA selector transform and field3 signed-relative list slots → exact
  sentinel-terminated 44-byte-row structured ranges; surrounding container
  bytes remain UNKNOWN.
- Exact `0x00CCB0` selector and 32 validated longword slots → bounded physical
  group pointer table; nested target subtables remain UNKNOWN.
- Exact constant-D0 callers and the closed CC-B0 arithmetic → 15 unique
  selected 16-bit relative-pointer slots; other low-byte slots remain UNKNOWN.
- Runtime PC evidence plus canonical-equal probe ASM/binary artifacts →
  bounded exact 68000 slices; no semantic behavior is inferred.
- No edge was created from a decoder coincidence alone to an owned asset.

## Methods and outcomes

| Method | Scope | Outcome |
| --- | --- | --- |
| Existing exact 68000 reassembly + caller gate | 508 bounded Ghidra intervals | 51,650 ASM bytes retained across the accumulated map, including AUTO16 exact probe slices |
| Original pointer-table graphics parser | `0x05CE96..0x05D046` | 107 streams, already in baseline |
| Screen descriptor parser + decoder | `0x00C92C` groups and 167 descriptors | 159 streams and exact 26-byte descriptors |
| Exact Z80 upload proof | `0x06134E`, 0x2000-byte copy | 8,192 source-owned Z80 ASM bytes |
| Direct `JSR → 0x3820/0x37D2` consumer scan | 52 direct call sites | 29 accepted stream ranges |
| `0x03B8DE` table/consumer proof | `0x03A9EE → 0x03B1D0` | 7 accepted stream ranges |
| Fixed-record parser proof | `0x00129A`, `0x0012E8` | 8 × 1,208-byte structured records |
| Indexed script parser proof | `0x00C2EC`, `0x00C326`, `0x0051514` | 196-byte table + 11,210-byte streams |
| Nested level-table proof | `0x004D4E`, `0x004F48`, lookup sites `0x4D58..0x4F4A` | 64-byte outer table + 492-byte groups + 1,613-byte records |
| Exact lookup-table proof | `0x00302E`, `0x010684`, `0x01108C`, `0x0161FA`, `0x030200` | 128-byte word table + 512-byte byte table |
| Fixed-stride table proof | `0x00D72C`, `0x010086`, `0x0100AA`, `0x039100` | 50 × 32-byte records, 1,600 bytes |
| Direct graphics proof | `0x003278`, `0x004E8..0x0050A`, `0x003356`, `0x00335C` | 7 streams, 40,065 bytes |
| Direct graphics chain proof | `0x03C074`, `0x03C276..0x03C286`, `0x03C5CA..0x03C5E6` | 5 new streams, 21,016 bytes; 3 existing anchors revalidated |
| Erased alignment proof | 16 UNKNOWN ranges across the canonical ROM | 132,630 exact `0xFF` bytes ending on 4 KiB boundaries |
| Automatic exact-code continuation | `0x00F0EC..0x00F10C`, `0x00B28E..0x00B34C` | 222 caller-backed ASM bytes; 12 unsupported candidates rejected |
| Fixed record table and count-bounded streams | `0x3F2FA`, `0x3F306..0x3FF66`, `0xAB82`, `0xAF02`, `0xB15E`, `0xB28E` | 3,168 table bytes + 46,858 merged stream bytes |
| BCEA field3 list proof | `0x00BCEA`, field3 bases, selector offsets `-20/0/20/40` | 100 views merged into five ranges, 7,516 bytes |
| CC-B0 group pointer-table proof | `0x00CCB0`, `0x04371E..0x04379E` | 32 validated longword slots, 128 bytes; nested subtables not promoted |
| CC-B0 selected-slot proof | 16 exact constant-D0 callers of `0x00CA24` | 15 unique signed relative-pointer slots, 30 bytes; dynamic slots not promoted |
| Exact runtime-correlated probe slices | `0x008F12..0x009330`, `0x03B1D0..0x03B358`, `0x060090..0x060286`, `0x061232..0x061328` | 9 canonical-equal ASM/binary slices, 1,430 bytes; PC-level corroboration only |
| Runtime-correlated graphics streams | `0x15E052..0x160E19`, `0x2119D2..0x211F79` | 13,166 bytes; exact `0x3820` runtime reader, matching local decoder boundaries, and static pointer literals |
| Full unresolved-region graphics census | `0x064E38..0x141580`, `0x1AD000..0x1E7236`, `0x25FEC2..0x300000` | decoder-complete candidates; only closed consumer/table edges promoted |
| Runtime ROM-reader correlation | existing GPGX evidence | two ranges closed only where runtime reader, local decoder boundary, and static pointer evidence agree |
| Beta-ROM differential | canonical vs beta | evidence only; not ownership proof |
| Monotonic absolute-pointer scan | large unresolved intervals | no complete independent pointer graph |

## Remaining blocker

The largest remaining unknown intervals are:

| Range | Bytes | Current blocker |
| --- | ---: | --- |
| `0x064E38..0x141580` | 902,984 | mixed unresolved code/data; no closed continuation or container boundary |
| `0x25FEC2..0x2E2FE6` | 536,868 | post-screen payload lacks a complete consumer/resource graph |
| `0x03E7F4..0x051514` | 77,088 | mixed static code/data area with unresolved boundaries |
| `0x0541FD..0x05D918` | 38,683 | mixed static code/data area with unresolved boundaries |
| `0x1ED5EC..0x200009` | 76,317 | data family boundary and parser continuation not closed |
| `0x2025C9..0x2119D2` | 62,473 | fixed-record tail/save continuation not independently closed |
| `0x143E76..0x15457A` | 67,332 | stream neighbors and mixed data/code boundary unresolved |

The census found many valid decompressor outputs, but a valid decoder result
at an arbitrary offset is not sufficient: it may be nested, accidental, or
overlap executable/data bytes. Those candidates remain evidence-only unless a
pointer table, parser, consumer, and non-conflicting boundary are all closed.
Promoting the remaining blob as `dc.b`, an undifferentiated asset, padding, or
possible code would violate the M12 acceptance rules and would make the
percentage meaningless.

## Next path toward 100%

1. Recover the parser and full record boundary for the post-screen descriptor
   families around `0x03Fxxx..0x04Fxxx`.
2. Close the mixed 68000 continuation around `0x064E38..0x141580` with exact
   source round-trips and caller/continuation evidence.
3. Establish independent ownership for uncompressed or non-graphics payloads;
   graphics decoder census alone cannot classify them.
4. Re-run the same transactional materializer after each bounded positive
   graph, preserving the canonical ROM hashes.
5. Stop only when the integer source-owned threshold is reached; then run the
   full Debug/Release/GNU-equivalent and hygiene gates before any later
   milestone decision.

## Validation and hygiene

The local transaction has zero manifest gaps and overlaps and reconstructs the
canonical ROM exactly. Generated ROMs, extracted assets, census JSON, and
transaction directories remain local ignored build evidence. No ROM, BIOS,
commercial asset, secret, or production C++ migration was added to the
repository. AUTO17 helper compilation, regression, runtime-reader/pointer
validation and canonical hash checks passed; the full Debug/Release/GNU-equivalent build and
CTest publication gate remains pending for this checkpoint.
