# M12-AUTO9 — bounded provenance toward a 90% source-owned ROM map

Status: `M12_AUTO9_BLOCKED_BELOW_90_NO_COMPLETE_PROVENANCE_GRAPH`.

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
| Baseline Git SHA | `9f7a065ea693bb487aa90a5ad0570ad81625ed54` |
| Final local transaction | `build/m12-auto9-erased-alignment-transaction-b/materialized/manifest.json` |

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

The 90% threshold is 2,831,156 bytes; the current gap is 1,903,378 bytes.

## Final ownership census

| Class | Bytes | Percent |
| --- | ---: | ---: |
| 68000 `CODE_VERIFIED` | 48,906 | 1.554679871% |
| Header/vector ASM | 512 | 0.016276042% |
| Confirmed structured data | 29,825 | 0.948111216% |
| Confirmed alignment padding | 132,677 | 4.217688243% |
| Local ROM-derived assets | 715,858 | 22.756512960% |
| **SOURCE_OWNED** | **927,778** | **29.493268331%** |
| Remaining `UNKNOWN` blob | 2,217,950 | 70.506731669% |

The asset total consists of the original 107-entry compressed-resource graph,
159 screen-descriptor primary streams, 29 direct 68000 graphics streams, the
seven streams selected by the bounded `0x03B8DE` table, and five direct chain
continuations. The structured
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
- No edge was created from a decoder coincidence alone to an owned asset.

## Methods and outcomes

| Method | Scope | Outcome |
| --- | --- | --- |
| Existing exact 68000 reassembly + caller gate | 496 bounded Ghidra intervals | 48,906 ASM bytes retained across the accumulated map |
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
| Full unresolved-region graphics census | `0x064E38..0x141580`, `0x1AD000..0x1E7236`, `0x25FEC2..0x300000` | decoder-complete candidates; only closed consumer/table edges promoted |
| Runtime ROM-reader correlation | existing GPGX evidence | corroboration only; no destination/boundary proof for unknown spans |
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
| `0x2025C9..0x211F7A` | 63,921 | fixed-record tail/save continuation not independently closed |
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
repository. Fresh Debug, Release, and GNU-equivalent builds passed; full CTest
passed `90/90` in all three configurations after the padding helper was
registered. `git diff --check`, source file limits, Python helper checks, and
the independent manifest/hash audit passed.
