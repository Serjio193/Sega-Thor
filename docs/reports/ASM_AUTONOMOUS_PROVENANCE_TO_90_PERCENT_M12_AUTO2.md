# M12-AUTO34 — exact compressed-resource pointer table

Status: `M12_AUTO34_BELOW_90_CONTINUING`.

AUTO34 adds the wholly UNKNOWN `[0x05CE96,0x05D046)` 108-entry absolute
resource-pointer table (432 bytes). The exact `0x00D3B2` reader uses a
4-byte index and the verified `0x003820` decoder; entry 0 is a sentinel and
entries 1..107 point monotonically to already-owned streams. The transaction
rebuilds the canonical ROM exactly and the map reaches 1,227,770 / 3,145,728
bytes (39.0297571818%).

# M12-AUTO33 — exact 64-entry item label table

Status: `M12_AUTO33_BELOW_90_CONTINUING`.

AUTO33 adds the wholly UNKNOWN `[0x05CC16,0x05CE16)` 64-record item-label
table (512 bytes). Two exact consumers use the byte-selected index and the
8-byte fixed stride; all 512 bytes satisfy the printable-record contract and
the following binary payload remains UNKNOWN. The transaction rebuilds the
canonical ROM exactly and the map reaches 1,227,338 / 3,145,728 bytes
(39.0160242716%).

# M12-AUTO32 — exact fixed-width menu label table

Status: `M12_AUTO32_BELOW_90_CONTINUING`.

AUTO32 adds the wholly UNKNOWN `[0x05CBA6,0x05CBD6)` six-record menu-label
table (48 bytes). The exact `FF185C` switch accepts five one-based cases,
the fallback explicitly selects 5, and consumer `0x003F8E` indexes six
8-byte records through `FF1861 << 3`. The transaction rebuilds the canonical
ROM exactly and the map reaches 1,226,826 / 3,145,728 bytes (38.9997482300%).

# M12-AUTO31 — exact sentinel threshold table

Status: `M12_AUTO31_BELOW_90_CONTINUING`.

AUTO31 adds the wholly UNKNOWN `[0x05D906,0x05D918)` nine-word threshold
table (18 bytes). Consumer `0x010666` advances by the exact 8-byte result
stride and terminates on the ninth-word `0xFFFF` sentinel immediately before
the already-confirmed next table. The transaction rebuilds the canonical ROM
exactly and the map reaches 1,226,778 / 3,145,728 bytes (38.9982223511%).

# M12-AUTO30 — exact state dispatch pointer table

Status: `M12_AUTO30_BELOW_90_CONTINUING`.

AUTO30 adds the wholly UNKNOWN `[0x00DF54,0x00E0B8)` 89-entry absolute
pointer table (356 bytes). Three exact consumers use the same `LEA`, doubled
selector, `-44(A0,D6.W)` longword read, and indirect `JSR`; the table ends
before the already-owned `RTS` at `0x00E0B8`. The pointed-to handler bodies
remain unowned. The transaction rebuilds the canonical ROM exactly and the
map reaches 1,226,760 / 3,145,728 bytes (38.9976501465%).

# M12-AUTO29 — preserved candidate-map code census negative result

Status: `M12_AUTO29_BELOW_90_CONTINUING`.

AUTO29 reconstructs 496 bounded function ranges from the preserved candidate
map and runs the strict transactional auto-promoter. It discovers 534
candidates, but 0 are eligible, 0 are attempted, and 0 are accepted against
the AUTO28 map. No ownership is added. This exhausts the current preserved
automatic code-census evidence set only; it is not a global blocker and does
not justify promoting mixed or unbounded code. The current map remains
1,226,404 / 3,145,728 bytes (38.9863332113%), and the canonical ROM remains
byte-exact.

# M12-AUTO28 — exact 16-entry nibble lookup

Status: `M12_AUTO28_BELOW_90_CONTINUING`.

AUTO28 adds the wholly UNKNOWN `[0x062DC0,0x062DE0)` 16-word lookup (32
bytes). The exact consumer at `0x0624D2` masks `D3` with `0xF`, doubles the
index, and reads the table through a PC-relative base. The transaction is
`build/m12-auto28-nibble-lookup-transaction-a/materialized/manifest.json` and
rebuilds the canonical ROM exactly; neighboring code and mixed payload remain
UNKNOWN.

# M12-AUTO27 — exact event dispatch table

Status: `M12_AUTO27_BELOW_90_CONTINUING`.

AUTO27 adds the wholly UNKNOWN `[0x00532C,0x005378)` 38-entry signed-relative
event dispatch table (76 bytes). The exact dispatcher at `0x00530C` subtracts
`0x1A`, doubles the bounded selector, and applies each signed word relative to
its table entry; selectors 17..37 resolve to the owned `RTS` at `0x00532A`.
The transaction is
`build/m12-auto27-event-dispatch-transaction-a/materialized/manifest.json`;
the canonical ROM remains byte-exact. Surrounding handlers and mixed payload
remain UNKNOWN.

# M12-AUTO26 — exact field-86 callback entrypoints

Status: `M12_AUTO26_BELOW_90_CONTINUING`.

AUTO26 adds 54 bytes from the indirect callback family rooted at `0x00E0BA`.
The dispatcher loads `86(A6)` into `A0` and executes `JSR (A0)`; exact
initializers set `0x10000` and `0x30000`. The ranges
`0x010000..0x010034` and `0x030000..0x030002` round-trip through vasm and the
full transaction remains canonical. The F-line/data-like `0x80000` candidate
is explicitly rejected because no closed code boundary was established.

# M12-AUTO25 — exact PC-relative consumer tables

AUTO25 adds 352 bytes from 11 wholly UNKNOWN ranges selected by exact
PC-relative consumers. Masked byte lookups, sequential DBF record loops,
bounded 3-byte selectors, fixed 10-byte copies, and exact VDP word-copy loops
provide the parser/consumer and half-open boundaries. The transaction is
`build/m12-auto25-pc-tables-transaction-b/materialized/manifest.json` and
rebuilds the canonical ROM exactly.

# M12-AUTO24 — bounded provenance toward a 90% source-owned ROM map

Status: `M12_AUTO24_BELOW_90_CONTINUING`.

AUTO24 adds the caller-backed exact static island `[0x0167BE,0x01685A)`
(156 bytes). Two Ghidra caller xrefs (`0x01672E`, `0x016742`) identify the
function, the bounded decoder accepts the full range, and vasm reproduces it
after normalizing the decoder's incorrect terminal `exg.w D4,A6` spelling to
the independently verified `EXG A4,A6` opcode. No ROM bytes are changed.

The read-only census evaluated 26 caller-backed Ghidra ranges wholly inside
UNKNOWN; 25 were rejected for decoder boundary or unsupported-form failures.
The absolute and signed-relative pointer-density scan found no candidate
window meeting its minimum density threshold. The `0x80000` bank remains
UNKNOWN because its dynamic object-base use does not close a parser or code
container boundary.

AUTO23 adds 296 exact save-serialization bytes after AUTO22. AUTO22 added 58
exact small-table bytes; AUTO21 added 7,238
bytes from the exact CC-B0 relative-target-table contract; AUTO20 added
115,011 table-selected graphics bytes. The current map owns 1,225,734 bytes
(38.9650344849%), with 1,605,422 bytes remaining to the integer 90% threshold.
Canonical ROM identity remains unchanged and no M13/C++ migration has begun.

M12-AUTO18 adds two exact static-consumer graphics streams totaling 1,063
bytes after AUTO17: `0x167E48..0x16821F` and `0x168442..0x168492`. Exact
`LEA → D9A4 → 37D2 → 0x3820` contracts select both streams and local decoding
consumes exactly their boundaries. AUTO17 runtime-correlated streams are
revalidated in the same transaction. Surrounding tables and census-only
candidates remain UNKNOWN. The canonical ROM remains byte-exact and M13/C++
migration remain prohibited.

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
| Baseline Git SHA | `732178432d9698ebff74d82a625633be9e23e71e` |
| Final local transaction | `build/m12-auto24-static-code-transaction-d/materialized/manifest.json` |

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
| Exact 0x3820 graphics consumers | 1,002,451 | 31.8670590719% |
| Table-selected graphics | 1,218,142 | 38.7236913045% |
| CC-B0 relative-target tables | 1,225,380 | 38.9537811279% |
| Exact small tables | 1,225,438 | 38.9556248983% |
| Save serialization slots | 1,225,734 | 38.9650344849% |
| Exact static caller-backed island | 1,225,890 | 38.9699935913% |

The 90% threshold is 2,831,156 bytes; the current gap is 1,605,266 bytes.

## Final ownership census

| Class | Bytes | Percent |
| --- | ---: | ---: |
| 68000 `CODE_VERIFIED` | 51,806 | 1.6468683879% |
| Header/vector ASM | 512 | 0.016276042% |
| Confirmed structured data | 95,117 | 3.0236879985% |
| Confirmed alignment padding | 132,677 | 4.217688243% |
| Local ROM-derived assets | 845,098 | 26.8649419149% |
| **SOURCE_OWNED** | **1,225,890** | **38.9699935913%** |
| Remaining `UNKNOWN` blob | 1,919,838 | 61.0300064087% |

The asset total consists of the original 107-entry compressed-resource graph,
159 screen-descriptor primary streams, 29 direct 68000 graphics streams, the
seven streams selected by the bounded `0x03B8DE` table, five direct chain
continuations, two runtime-correlated streams, and two exact static-consumer
streams read by `0x3820`. The structured
data total includes the existing exact ASM-backed data, eight fixed 1208-byte
records, the 196-byte indexed table, and 98 exact streams totalling 11,210
bytes, the 64-byte nested outer table, 492 bytes of count-bounded nested
groups, and 185 pointer-backed records totalling 1,613 bytes. The one
8-byte unindexed record-shaped span remains UNKNOWN. The structured-data total
also includes the 50-record table at `0x5D046`, the 64-entry `0x0EEE` word
lookup table, the 512-byte bounded byte lookup table, and the 296-byte exact
save-serialization ranges. The Z80 8192-byte
image is included in the ASM total and is also
recorded as `Z80_SOURCE_OWNED_BYTES=8192`. The exact static island adds 156
bytes to the ASM total.

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
| Exact static 0x3820 graphics consumers | `0x167E48..0x16821F`, `0x168442..0x168492` | 1,063 bytes; exact `LEA → D9A4 → 37D2 → 0x3820` chains and local decoder boundaries |
| Full unresolved-region graphics census | `0x064E38..0x141580`, `0x1AD000..0x1E7236`, `0x25FEC2..0x300000` | decoder-complete candidates; only closed consumer/table edges promoted |
| Table-selected graphics | `0x3F306..0x3FF66`, field1 `+4` | 35 wholly UNKNOWN streams, 115,011 bytes; independent census boundary and decompressed-size checks |
| CC-B0 relative-target tables | `0x00CCB0`, 20 unique target windows | 7,238 previously UNKNOWN bytes; exact one-byte selector, two-byte slots, and signed-relative consumer |
| Exact small tables | `0x03E0B8..0x03E0D8`, `0x522E..0x5236`, `0x5CBD6..0x5CBE8` | 58 bytes; exact copy/selector/record loop contracts |
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
repository. AUTO18 helper compilation, regression, runtime-reader/static-chain/pointer
validation and canonical hash checks passed; the full Debug/Release/GNU-equivalent build and
CTest publication gate remains pending for this checkpoint.
## M12-AUTO19 — exact Ancient Music Driver data archive

AUTO19 promotes `0x064E38..0x07D780` as a bounded sound-data container,
adding 100,680 `DATA_KNOWN` bytes. The exact 68000 consumer at `0x060B50`
indexes the table rooted at `0x0638D4`; the deterministic validator checks the
34-entry primary group, 126-entry secondary group, and 17-entry tertiary group.
Only 29/34, 126/126, and 4/17 entries respectively target the promoted data;
the 13 tertiary targets outside the container are rejected. The immediate
`0x07D780..0x080000` run is byte-exact `0xFF` fill and closes the container.
The inherited transaction has zero gaps/overlaps and canonical full-ROM hashes;
the map reaches 1,103,131 bytes (35.0675900777%). This method establishes raw
sound-data ownership only; audio semantics and C++ migration remain outside
M12.

Method-effectiveness update: the sound-table method is POSITIVE for one
bounded 100,680-byte container, while raw pointer targets outside its exact
terminal boundary remain NEGATIVE/REJECTED. Decoder-only census candidates,
unbounded sound tables, and inactive tertiary targets remain UNKNOWN.
# M12-AUTO20 — Table-selected graphics promotion

## Scope

M12 remains provenance-only: preserve the canonical byte-exact ROM, do not
start M13 or C++ migration, and promote only ranges with an exact source
contract and deterministic boundary. AUTO20 evaluates the 99-row resource
table at `[0x3F306,0x3FF66)` and its field1 longwords at row offset `+4`.

## Positive result

Thirty-five unique field1 pointers match independent graphics-census records
with exact compressed boundaries and recorded decompressed sizes. Only streams
wholly inside UNKNOWN manifest entries were selected; the 22 matching census
streams that overlap prior ownership remain excluded. The selected ranges are:

`0x2DC334..0x2DC7F0`, `0x2B15DA..0x2B45D8`, `0x29EE9A..0x2A0922`,
`0x1650DC..0x165433`, `0x1658DC..0x165E12`, `0x165E72..0x16610C`,
`0x1665A0..0x166C3D`, `0x166CFC..0x1670C0`, `0x16722E..0x167707`,
`0x167728..0x16779D`, `0x28D158..0x28EA71`, `0x16378E..0x1638CC`,
`0x1638CC..0x163A69`, `0x163A6A..0x163BDA`, `0x163BDA..0x163D42`,
`0x163D42..0x163EB7`, `0x164070..0x1641D0`, `0x164270..0x16460C`,
`0x29D148..0x29E2D8`, `0x26DCDE..0x26E55C`, `0x26ECD0..0x270584`,
`0x16785A..0x167A6D`, `0x164896..0x164D10`, `0x2DF6A4..0x2E0134`,
`0x27AA2A..0x27B32A`, `0x2DD302..0x2DF25F`, `0x27C206..0x27E112`,
`0x28003E..0x28161F`, `0x161364..0x1621B9`, `0x162470..0x163716`,
`0x2E1870..0x2E2FE6`, `0x2E3AB6..0x2E5FCF`, `0x2EF4F4..0x2F125E`,
`0x168D34..0x16943C`, `0x2F6986..0x2F784F`.

The transaction promotes 115,011 bytes and reaches 1,218,142 / 3,145,728
bytes, or 38.7236913045%. The canonical rebuilt ROM remains CRC32 `C4728225`,
SHA-1 `2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

## Negative boundary

This transaction does not promote raw field1 pointers without a census
boundary, non-graphics table fields, or any range overlapping existing code or
data ownership. Those remain UNKNOWN for a later parser/consumer proof.

# M12-AUTO21 — CC-B0 relative-target tables

AUTO21 promotes 7,238 previously UNKNOWN bytes from the nested tables selected
by `0x00CCB0`. The exact consumer masks the low selector byte, scales it by two,
and adds a signed relative word to the selected group target. This closes a
fixed 256-slot / `0x200`-byte table for each of the 20 unique targets; overlapping
windows are unioned and AUTO15's already-owned selected slots are preserved.
No records reached by the relative pointers are inferred. The transaction
rebuilds the canonical ROM exactly and reaches 1,225,380 bytes (38.9537811279%).

# M12-AUTO22 — Exact small tables

AUTO22 promotes 58 bytes from three exact fixed consumers:
`[0x3E0B8,0x3E0D8)` is the eight-longword copy table selected at `0x03E516`,
`[0x522E,0x5236)` is the eight-byte selector table selected at `0x0051CE`, and
`[0x5CBD6,0x5CBE8)` is the six three-byte source-record table read at `0x005120`.
No adjacent bytes are classified. The canonical ROM remains CRC32 `C4728225`,
SHA-1 `2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
The current map reaches 1,225,438 bytes (38.9556248983%); the integer 90%
threshold still requires 1,605,718 additional bytes.

# M12-AUTO23 — Save serialization slots

AUTO23 promotes 296 bytes from the exact save serializer/checker contract.
The primary range `[0x2025CD,0x2026E1)` is bounded by six stride-2 tag
positions, 130 serialized bytes, and a two-byte checksum. The secondary
promotion is deliberately limited to `[0x2026E1,0x2026F5)`, where the exact
checker and writer establish only six tags and a four-byte value. The later
save/SRAM tail remains UNKNOWN because no exact consumer closes its extent.
The current map reaches 1,225,734 bytes (38.9650344849%); the integer 90%
threshold still requires 1,605,422 additional bytes. Canonical full-ROM
hashes remain unchanged.

# M12-AUTO24 — Exact static caller-backed island

AUTO24 promotes `[0x0167BE,0x01685A)` (156 bytes), wholly inside UNKNOWN in
the AUTO23 manifest. The Ghidra function has caller xrefs at `0x01672E` and
`0x016742`; the exact range decoder accepts the whole island, and vasm
reproduces the ROM after correcting one decoder operand-class spelling. The
terminal bytes `C9 4E` assemble as `EXG A4,A6`; the decoder's `exg.w D4,A6`
text is therefore normalized only in generated ASM. This does not alter the
canonical ROM or assert ownership of neighboring bytes.

The transaction reaches 1,225,890 bytes (38.9699935913%), leaving 1,605,266
bytes to the integer 90% threshold. The full-ROM result remains CRC32
`C4728225`, SHA-1 `2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
