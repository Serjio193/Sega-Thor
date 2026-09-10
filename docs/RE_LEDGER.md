# Bounded G0 reverse-engineering ledger

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
