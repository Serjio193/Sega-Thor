# M12-AUTO23 save-serialization provenance checkpoint

AUTO23 is `build/m12-auto23-save-slots-transaction-b/materialized/manifest.json`.
It promotes `[0x2025CD,0x2026E1)` as the primary save slot: six stride-2 tag
bytes, 130 serialized bytes, and a two-byte checksum are closed by the exact
reader/writer pair at `0x001DDC`/`0x001E46`. It promotes only the confirmed
secondary prefix `[0x2026E1,0x2026F5)`, where `0x001EDE` checks six tags and
`0x001F26` writes the four-byte value. The later save/SRAM tail remains
UNKNOWN because no exact extent is established. Full-ROM hashes remain
canonical and no C++ migration is involved.

# M12-AUTO22 exact small-table provenance checkpoint

AUTO22 is `build/m12-auto22-exact-small-tables-transaction-a/materialized/manifest.json`.
It adds three exact fixed tables: `[0x3E0B8,0x3E0D8)` selected by the eight-
longword copy at `0x03E516`, `[0x522E,0x5236)` selected by the fixed selector
consumer at `0x0051CE`, and `[0x5CBD6,0x5CBE8)` consumed as six three-byte
records from `0x005120`. No adjacent bytes are included; full-ROM hashes remain
canonical.

# M12-AUTO21 CC-B0 relative-target-table provenance checkpoint

AUTO21 is `build/m12-auto21-ccb0-target-tables-transaction-b/materialized/manifest.json`.
The exact selector contract at `0x00CCB0` masks the low selector byte, scales
it by two, and adds a signed relative word to the selected group target. This
closes 256 two-byte slots (`0x200` bytes) for each of the 20 unique target
values. Overlapping windows are unioned and existing AUTO15 slots remain
owned by their earlier contract. No nested records are inferred.

# M12-AUTO19 Ancient Music Driver data archive

The AUTO19 transaction
`build/m12-auto19-sound-data-transaction-a/materialized/manifest.json` adds
100,680 `DATA_KNOWN` bytes in `0x064E38..0x07D780`. The exact consumer at
`0x060B50` indexes the `0x0638D4` offset table. Machine-readable validation
records 34 primary entries (29 active in the promoted range), 126 secondary
entries, and 17 tertiary entries (4 active); targets outside the terminal
boundary are not promoted. The following `0xFF` alignment run is independently
preserved. The source-owned map reaches 1,103,131 bytes (35.0675900777%); the
canonical ROM hashes and zero-gap/zero-overlap manifest remain exact.

# M12-AUTO18 exact 0x3820 graphics consumers

The AUTO18 transaction
`build/m12-auto18-graphics-consumers-transaction-d/materialized/manifest.json`
adds 1,063 `LOCAL_ROM_DERIVED_ASSET` bytes in
`0x167E48..0x16821F` and `0x168442..0x168492`. Exact static consumers at
`0x02D444`, `0x02E204`, and `0x02D416` load those sources and call `0x00D9A4`;
`0x00D9A4` calls `0x0037D2`, and the `0x0037D8` direct branch reaches the exact
graphics decoder `0x003820`. Local decoder source consumption is 983 and 80
bytes. The AUTO17 runtime streams are revalidated; no decoder-only census
candidate without this graph is promoted.

# M12-AUTO17 runtime-correlated graphics streams

The AUTO17 transaction
`build/m12-auto17-runtime-graphics-transaction-d/materialized/manifest.json`
adds 13,166 `LOCAL_ROM_DERIVED_ASSET` bytes in two exact ranges:
`0x15E052..0x160E19` and `0x2119D2..0x211F79`. Canonical GPGX ROM-reader
correlation identifies PC `0x003830` as the first reader for both ranges; the
runtime execution evidence confirms that PC as executed code in the exact
`0x3820` graphics decoder. The local decoder consumes the same boundaries,
and static big-endian pointer literals occur at `0x03F60A`, `0x043880`,
`0x043B6A`, `0x0461E6`, `0x046224`, and `0x02E1DC`. The surrounding records,
tables, and adjacent payload remain UNKNOWN; no decoder-only census candidate
was promoted.

# M12-AUTO16 exact runtime-correlated probe slices

The AUTO16 transaction reaches 988,222 bytes (31.4147313436%) by adding nine
canonical-equal ASM probe slices totalling 1,430 bytes. The slices are
runtime-correlated at PC level and remain bounded to their decoder-reported
intervals; adjacent mixed code/data and semantic behavior remain UNKNOWN.
Canonical full-ROM hashes remain exact and no C++ migration is authorized.

# M12-AUTO15 CC-B0 selected relative-pointer slots

The AUTO15 transaction closes 15 unique 16-bit slots (30 bytes) selected by
constant-D0 callers of `0x00CA24`. Each signed relative pointer resolves to an
even canonical-ROM target after the closed group table. Dynamic callers and
nested table extents remain UNKNOWN; the map reaches 986,792 bytes
(31.3692728678%).

# M12-AUTO14 CC-B0 group pointer table

The exact consumer at `0x00CCB0` selects a 32-entry longword pointer table at
`0x04371E` with `high_byte << 2`. The closed table `[0x04371E,0x04379E)` adds
128 source-owned bytes, bringing the map to 986,762 bytes (31.3683191935%).
Nested target subtables remain UNKNOWN; no selector range beyond the physical
32-entry table is asserted.

# Bounded G0 reverse-engineering ledger

## M12-AUTO13 BCEA field3 sentinel-list edge

The M12-AUTO13 transaction reaches 986,634 bytes (31.364250183%) of the
canonical ROM and preserves the exact ROM hashes. The exact BCEA selector
transform produces offsets `-20`, `0`, `20`, and `40` from 30 field3 bases in
the `0x58000..0x5D046` container. Signed relative pointers, 44-byte positive
rows, and negative-key sentinels close 100 list views, merged into five ranges
totalling 7,516 bytes. The rest of the container remains UNKNOWN; no semantic
field names are asserted and no C++ migration is authorized.

## M12-AUTO12 exact code continuation

The M12-AUTO12 local transaction reaches 979,118 bytes (31.125322978%) after
adding 1,092 exact ASM bytes to AUTO11. Twelve of twelve caller-backed Ghidra
candidates were accepted after systemic DIVU/DIVS/SBCD decoder normalization;
the canonical ROM hashes remain exact. No C++ migration is authorized.

## M12-AUTO11 count-bounded record-stream family

The M12-AUTO11 transaction reaches 978,026 bytes (31.090609233%) of the
canonical ROM, preserving the exact ROM hashes. Exact selectors at `0x8D06` and
`0x8DA4` use the `0x3F2FA` table base and the shared record consumers at
`0xAB82`, `0xAF02`, `0xB15E`, and `0xB28E` read a leading count followed by
six-byte records. This closes the 99-record table `[0x3F306,0x3FF66)` and 78
unique count-bounded stream views; overlapping views are merged only across
bytes covered by an accepted view. The additions are 3,168 table bytes and
46,858 stream bytes. No stream semantics beyond the raw parser contract are
asserted, and no C++ migration is authorized.

## M12-AUTO10 exact code continuation

The M12-AUTO10 transaction reaches 928,000 bytes (29.500325521%) of the
canonical ROM and preserves the exact ROM hashes. Two new caller-backed 68000
islands, `[0x00F0EC,0x00F10C)` and `[0x00B28E,0x00B34C)`, add 222 exact ASM
bytes. The automatic promoter now carries all ASM-backed baseline entries,
including header/vector ASM, through trials. Twelve other eligible candidates
remain rejected as unsupported exact IR. The threshold still requires
1,903,156 additional bytes. No M13 or C++ migration is authorized.

## M12-AUTO9 erased alignment padding

The M12-AUTO9 transaction promotes 927,778 bytes (29.493268331%) of the
canonical ROM, preserving the exact ROM hashes. Sixteen new ranges totaling
132,630 bytes are complete `0xFF` fill, at least 256 bytes each, and end on
4 KiB ROM alignment boundaries. They are classified only as
`ERASED_ROM_ALIGNMENT_PADDING`; zero-filled, mixed, and decoder-only spans
remain UNKNOWN. The integer 90% threshold still requires 1,903,378 additional
bytes. No M13 or C++ migration is authorized.

## M12-AUTO8 direct-graphics chain continuation

The M12-AUTO8 transaction promotes 795,148 bytes (25.277074178%) of the
canonical ROM, preserving the exact ROM hashes. Five new streams totaling
21,016 bytes are closed by exact sequential `0x3820` consumers and decoder
boundaries: `[0x18955A,0x18CC6E)` and `[0x18D01B,0x18EB1F)`. The chain also
revalidates already-owned anchors at `0x172168`, `0x1894EA`, and `0x18CF98`;
they are not double-counted. The threshold still requires 2,036,008 additional
bytes. No M13 or C++ migration is authorized.

## M12-AUTO7 direct-graphics boundary

The M12-AUTO7 transaction promotes 774,132 bytes (24.608993530%) of the
canonical ROM, preserving the exact ROM hashes. Seven non-overlapping streams
totaling 40,065 bytes are closed by direct `0x3820` consumers and sequential
decoder-advanced `A0`: `[0x150000,0x1503D3)`, four streams spanning
`[0x152340,0x15335A)`, and two streams spanning `[0x180C56,0x1894EA)`.
The fifth adjacent call after the first chain is decoder-rejected and remains
UNKNOWN. The threshold still requires 2,057,024 additional bytes. No M13 or
C++ migration is authorized.

## M12-AUTO6 fixed-stride-table boundary

The M12-AUTO6 transaction promotes 734,067 bytes (23.335361481%) of the
canonical ROM, preserving the exact ROM hashes. The new structured-data edge
is `[0x5D046,0x5D686)`: 50 records of 32 bytes selected by exact consumers at
`0x00D72C`, `0x010086`, `0x0100AA`, and `0x039100`. Each consumer uses the same
literal base, `index << 5`, and an eight-longword copy. The end is the start of
the independently confirmed `0x0EEE` word table. No semantic field names or
adjacent UNKNOWN spans are included. The threshold still requires 2,097,089
additional bytes. No M13 or C++ migration is authorized.

## M12-AUTO5 exact lookup-table boundary

The M12-AUTO5 transaction promotes 732,467 bytes (23.284498851%) of the
canonical ROM, preserving the exact ROM hashes. The word table
`[0x5D686,0x5D706)` is a bounded 64-entry table consumed by `0x00302E`; the
largest observed caller count is `0x3F`, so the 128-byte boundary is exact.
The byte table `[0x5D706,0x5D906)` is selected by exact consumers at
`0x010684`, `0x01108C`, `0x0161FA`, and `0x030200`; the latter masks the index
with `0x1FE`, closing the 512-byte boundary. The two tables add 640 bytes of
confirmed structured data. The threshold still requires 2,098,689 additional
bytes. No M13 or C++ migration is authorized.

## M12-AUTO4 nested level-table boundary

The M12-AUTO4 transaction promotes 731,827 bytes (23.264153798%) of the
canonical ROM, preserving the exact ROM hashes. The new structured-data edge
is the outer table `[0x5D918,0x5D958)` selected by exact 68000 lookup sites at
`0x4D58`, `0x4DCC`, `0x4E2C`, `0x4F00`, `0x4F12`, and `0x4F4A`. Every non-empty
group has a 16-bit count and exact boundary `start + 2*(count+2)`; the groups
are contiguous through `0x5DB44` and contribute 492 bytes. Their 186 non-zero
inner pointers resolve to 185 unique records with printable bodies and exact
NUL terminators, contributing 1,613 bytes through `0x5E1A0`, immediately
before all-`FF` filler. The 8-byte unindexed record-shaped span
`0x5E0FE..0x5E106` remains UNKNOWN. The threshold still requires 2,099,329
additional bytes. No M13 or C++ migration is authorized.

## M12-AUTO3 indexed script-table boundary

The M12-AUTO3 transaction promotes 729,658 bytes (23.195203145%) of the
canonical ROM, preserving the exact ROM hashes. The new structured-data edge
is the 16-bit table `[0x051514,0x0515D8)` selected by the exact parser at
`0x00C326`; its caller `0x00C2EC` enumerates 98 indices (`0x1B..0x1D` are
explicitly skipped), and every table target resolves monotonically to a
NUL-terminated stream. The 98 streams occupy `[0x0515FC,0x0541FD)` as
11,210 non-overlapping bytes; the table adds 196 bytes. All control bytes in
the streams are accepted by the parser's observed dispatch set. The skipped
indices remain included only as table-addressed records with the same closed
parser/boundary contract; no text semantics are asserted.

The exact transaction is
`build/m12-auto3-script-transaction-b/materialized/manifest.json`, and the
full evidence is in
`docs/reports/ASM_AUTONOMOUS_PROVENANCE_TO_90_PERCENT_M12_AUTO2.md`. The 90%
threshold still requires 2,101,498 bytes. No M13 or C++ migration is
authorized.

## M12-AUTO2 source-owned map boundary

The current local M12-AUTO2 transaction promotes 718,252 bytes (22.832616170%)
of the canonical ROM. It preserves the exact ROM hashes while adding the Z80
upload, screen graph, direct graphics consumers, the `0x03B8DE` table consumer
graph, and eight fixed 1208-byte parser records. The exact report is
`docs/reports/ASM_AUTONOMOUS_PROVENANCE_TO_90_PERCENT_M12_AUTO2.md`.

The 90% threshold requires 2,831,156 bytes; 2,112,904 additional bytes still
lack independently closed provenance. Decoder census coincidences and raw
pointer literals without a closed parser/consumer edge remain UNKNOWN. No M13
or C++ migration is authorized.

## M12-AUTO source-owned map boundary

The M12-AUTO checkpoint promotes 181 caller-backed exact 68000 islands for
23,430 bytes beyond M12.5, 107 pointer-table-selected compressed resource
streams for 238,087 bytes, and 47 deterministic alignment bytes. Combined
source ownership is 279,468 bytes (8.884048461%) of the canonical ROM. The
combined materialization has zero gaps/overlaps and canonical CRC32/SHA-1/
SHA-256. The exact resource proof is `src/tools/re_resource_boundary_scan.cpp`:
each stream is consumed by the existing graphics decompressor and ends at or
before the next table pointer; one-byte gaps are classified as alignment.

The remaining 2,866,260 bytes stay blob-backed. The largest unresolved ranges
are `0x062D6C..0x1AD000` (1,352,340 bytes) and
`0x1E7236..0x300000` (1,150,410 bytes). No global code/data/resource
provenance exists for those spans; no asset or executable ownership is
claimed. This is the current M12 global evidence blocker. No C++ migration or
M13 work is authorized.

## M12.2 ASM promotion boundary

M12.2 transactionally promotes thirteen exact source-owned intervals inside
`0x00DE00..0x00E338`: `0x00DEEC..0x00DF52`, `0x00E0B8..0x00E0BA`,
`0x00E0BA..0x00E0F4`, `0x00E0F4..0x00E0FE`, `0x00E106..0x00E140`,
`0x00E268..0x00E2A2`, `0x00E2A2..0x00E2BA`, `0x00E2D4..0x00E2F0`,
the four observed case arms `0x00E302..0x00E308`, `0x00E308..0x00E30C`,
`0x00E30C..0x00E310`, `0x00E310..0x00E316`, and the shared tail
`0x00E332..0x00E338`. They total 366 bytes and reassemble exactly.

The remaining 970 bytes are `UNRESOLVED_BOUNDARY`; `POSSIBLE_CODE_BYTES=0`
and `UNKNOWN_DATA_BYTES=0`. Exact indirect JSR sites at `0x00DF1A`,
`0x00E0E4`, and `0x00E130` are recorded evidence, not unresolved range
boundaries. The indirect JMP dispatch at `0x00E2F0` and its continuation remain
blob-backed. No interval is a typed data structure or portable routine.

The recomputed P0 queue proposes only `0x006516..0x0083D4` for M12.3
(7,870 bytes, 25 observed PCs, 0 static xrefs); it is not started.

## M12.1 ASM promotion boundary

M12.1 transactionally promotes only six exact source-owned intervals inside
`0x06042A..0x0611F4`: `0x06042A..0x060484`, `0x060490..0x0604B0`,
`0x060B50..0x060CDA`, `0x0611D6..0x0611E0`, `0x0611E0..0x0611EA`, and
`0x0611EA..0x0611F4`. They total 546 bytes and are backed by exact local
assembler output plus full-ROM exactness. The remaining 2,984 bytes are kept
as UNKNOWN blobs because dispatch/case and continuation boundaries are not
closed. The gap census is `POSSIBLE_CODE_BYTES=0`, `UNKNOWN_DATA_BYTES=0`,
`UNRESOLVED_BOUNDARY_BYTES=2,984`; these categories are not additional
ownership claims. No range below is a typed data structure or portable routine.

The legacy global census marked `0x06042A`, `0x0611DC`, and `0x0611E6` as
unsupported status-register forms. The current exact decoder normalizes these
`MOVE SR` encodings, and the affected slices reassemble exactly. This resolves
the decoder limitation only; it does not close the remaining target CFG.

One M12.2 proposal is recorded for `0x00DE00..0x00E338` (1,336 bytes, 31
observed PCs, 87 static xrefs). It was not started.

## M12.0 ASM completion boundary

M12.0 changes project sequencing, not the M11 G0 evidence. The authoritative
full-ROM materialization is the local M11.15 manifest: 203 exact 68000 ASM
ranges totaling 13,550 bytes, 136 canonical-local-ROM blob ranges totaling
3,132,178 bytes, 0 gaps and 0 overlaps. M11.17 independently classifies 10
bounded data ranges totaling 1,164 bytes, but they remain blob-backed and are
not source-owned in the exact rebuild.

The 8 blob ranges intersecting bounded observed execution evidence are P0
until split and promoted transactionally. The highest concentration is
0x06042A..0x0611F4 (3,530 coarse bytes, 76 unique observed PCs and 77 static
xrefs); this is the single M12.1 target. All other unclassified ranges remain
P3 code/data/asset ambiguity. No range is promoted by this ledger.

## M11.64 closure

Status: `G0_PORTABILITY_BOUNDARY_PROVEN_M11_LINE_CLOSED`.

G0 is the parent-owned region addressed by `A5 = 0xFF001A`. M11.60 proves its observed lifetime from `0x060182` materialization through parent restore at `0x06027E` and return at `0x060284`. M11.61–M11.63 close the natural evidence cluster while retaining static, hardware, ownership, and continuation blockers. This ledger is deliberately conservative: a natural trace does not become a whole-static contract, and raw storage does not become a typed object.

| Item | Classification | Current evidence |
|---|---|---|
| `0x060182` G0 materialization | `PROVEN_NATURAL_CONTRACT` | A5 equals `0xFF001A` on the bounded natural path. |
| `0x062AE0` consumer | `PRESERVATION_PROVEN_EFFECTS_BLOCKED` | Natural preservation observed; latent `0x062CEC` indirect CFG blocks whole-static closure. |
| `0x061934` consumer | `PROVEN_NATURAL_CONTRACT` | Natural entries/effects closed; latent `0x061F60` remains static-only debt. |
| `0x0623AC` consumer | `HARDWARE_BOUNDARY` | Six byte-write PCs to VDP `0x00C00011`, 407 events. |
| `0x060286` continuation | `NOT_YET_CLOSED` | Deferred; not executed in M11.64. |
| Parent restore/return | `PROVEN_NATURAL_CONTRACT` | `0x06027E` restore followed by `0x060284 RTS`. |
| External writers/aliasing | `UNRESOLVED` | Typed ownership is blocked. |
| Interrupt and unresolved runtime effects | `NATURAL_ZERO` | Zero events in the bounded natural evidence. |
| Broader `0x061258` lifetime | `NOT_YET_CLOSED` | Outside bounded G0 interval. |

## Required gates

A register/lifetime: parent-owned and exact on observed natural entries/returns. B call/effect: natural cluster substantially closed but whole-static debt remains. C hardware: VDP dependency proven at the source PCs, with no abstraction layer. D typed-data: overlapping/shared raw access and external-writer closure are absent, so typed promotion is blocked.

`0x060286` would not change the architecture: `NO_ARCHITECTURAL_DECISION_CHANGE`. Keep all deferred edges and ownership questions explicit until new evidence is independently collected.
# M12-AUTO20 — Table-selected graphics provenance

| Family | Exact source contract | Boundary proof | Result |
|---|---|---|---|
| table-selected graphics | `0x3F306..0x3FF66`, field1 `+4`, rows 15, 17, 23, 33-50, 53-57, 60-61, 63, 65, 67-69, 73-74, 77, 79, 89, 94-95 | Independent deterministic graphics census; every promoted range wholly UNKNOWN; full-ROM vasm equality | 35 streams, 115,011 bytes, `LOCAL_ROM_DERIVED_ASSET` |
