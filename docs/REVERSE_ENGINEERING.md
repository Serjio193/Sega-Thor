# M12-AUTO56 overlapping PC-relative word-table provenance

AUTO56 promotes only the previously UNKNOWN prefix `[0x062DA8,0x062DC0)`
(24 bytes) of the exact 16-word table `[0x062DA8,0x062DC8)`. The consumer at
`0x061A22` resolves the base through `LEA ($1384,PC),A2`, masks the selector
with `0x0F`, doubles it, and reads a word at `0(A2,D0.W)`, closing exactly
16 two-byte entries. The final four entries overlap the already confirmed
AUTO28 table `[0x062DC0,0x062DE0)`; AUTO56 records the overlap as evidence and
does not re-promote or relabel those bytes. No semantic meaning is assigned to
the word values or adjacent data.

The deterministic developer-only helper is
`src/tools/re_m12_pc_word_overlap_promote.py`, with regression coverage in
`tests/re_m12_pc_word_overlap_test.py`. The materialized ROM remains
byte-exact to the canonical hash; no C++ migration is started.

# M12-AUTO55 bounded 0x03B8DE descriptor table

AUTO55 promotes only `[0x03B8E2,0x03B95E)` because the preceding four bytes
are already owned by AUTO47. The PC-relative `LEA` at `0x03A9EE` resolves to
`0x03B8DE` using the 68000 PC base; `0x03A9F2` shifts the selector by four,
and the exact control family bounds it to eight records (`0..7`, 16 bytes
each). The table consumer contracts at `0x03A91C`, `0x03A9E2`, and `0x03AA18`
independently confirm the selector/count family. All eight records remain raw
structured data; no field semantics or adjacent bytes are inferred.

The deterministic developer-only helper is
`src/tools/re_m12_table_03b8de_promote.py`, with regression coverage in
`tests/re_m12_table_03b8de_test.py`. The materialized ROM is byte-exact to the
canonical hash; no C++ migration is started.

# M12-AUTO52 direct loader graphics streams

AUTO52 closes three direct loader chains. Literals at `0x03CA32`, `0x03CD46`,
and `0x03DD62` identify starts `0x1911EA`, `0x19911A`, and `0x19D6A0`; the
existing graphics decompressor contracts close the streams at `0x191F09`,
`0x199CBA`, and `0x19EC4C`. The 11,883 promoted bytes are classified as
`GRAPHICS_COMPRESSED_STREAM` and remain local ROM-derived assets, not portable
C++ code. The transaction preserves the canonical ROM hash and has no gaps,
overlaps, or conflicts.

# M12-AUTO51 static stream pointers

AUTO51 closes nine static-pointer graphics streams (28,000 bytes): four
table-backed streams at `0x176340`, `0x1794CA`, `0x17C700`, and `0x17E610`,
plus direct/repeated-pointer streams at `0x1698FE`, `0x196300`, `0x0541FE`,
`0x163EB8`, and `0x165434`. Each pointer literal and decompressor end is
recorded by `src/tools/re_m12_static_stream_pointer_promote.py`; candidates
without a closed pointer/boundary contract remain UNKNOWN.

# M12-AUTO50 multi-resource families

AUTO50 closes a repeated 22-byte descriptor family near `0x02DAA4`, a
13-record 16-byte table at `0x03DC22`, and three direct loader streams. It
promotes 76,019 exact bytes. The gap `0x207595..0x207738` is deliberately not
owned because the family evidence does not establish its boundary.

# M12-AUTO49 descriptor-backed graphics streams

AUTO49 promotes ten streams selected by repeated 22-byte descriptors near
`0x02CBA2`, totaling 72,284 bytes. The descriptors' pointer field and the
exact decompressor terminator independently close each half-open range. The
nearby `0x1F66A0..0x1F683C` stream-like candidate remains UNKNOWN.

# M12-AUTO48..46 dispatch families

AUTO48 closes a PC-relative eight-word table at `0x00E2F2` with its exact
selector and shared continuation, adding 36 bytes. AUTO47 closes a
multi-dispatch family and table, adding 1,026 bytes. AUTO46 closes a static
dispatch family, adding 246 bytes. These are exact ASM/structured-data
promotions; unresolved indirect targets are not assigned ownership.

# M12-AUTO45 exact indexed offset table

AUTO45 promotes `[0x00AD56,0x00AD76)` (32 bytes) as
`INDEXED_WORD_OFFSET_TABLE`. The exact consumer family is closed by
`LEA.L ($FFFFFB7A,PC),A5` at `0x00B1DA`, which resolves to `0x00AD56`, and
the selector sequence at `0x00B242`/`0x00B248`: `ANDI.W #$0F00,D4`,
`LSR.W #7,D4`, then `MOVE.W 0(A5,D4.W),D4`. The resulting offsets are the
sixteen even values `0..30`, so the table boundary is exactly 16 big-endian
words. The next decoded instruction at `0x00AD76` is retained outside the
promotion.

The data-only transaction is
`build/m12-auto45-indexed-offset-table-transaction-c/materialized/manifest.json`.
It reuses the canonical baseline rebuild and therefore preserves the exact
ROM hashes. No semantics are assigned to the table values beyond their
consumer-defined word-offset role, and no C++ migration is started.

# M12-AUTO44 runtime count-bounded record regions

AUTO44 promotes 27 previously UNKNOWN regions totaling 1,732 bytes as
`RUNTIME_COUNT_BOUNDED_6BYTE_RECORD_STREAMS`. Readers `0x00AF16` and
`0x00ABB4` load each region's count word and tile the complete interval as
`2 + 6 * (count + 1)` bytes per stream; runtime correlation boundaries and
reader execution evidence agree. The other 24 matching regions (1,346 bytes)
were already owned by AUTO11 and are not double-counted.

# M12-AUTO43 small caller-backed runtime-observed exact routine

AUTO43 promotes `[0x0083F8,0x00846C)` (116 bytes) as
`CALLER_BACKED_RUNTIME_EXACT_ASM_ROUNDTRIP`. Exact `BSR` caller `0x006230`
targets the entry immediately after `RTS` `0x0083F6`; all 33 instruction
starts are runtime-observed and decoded, ending at `RTS` `0x00846A`.
Independent vasm output matches every canonical-ROM byte.

# M12-AUTO42 caller-backed runtime-observed exact routine

AUTO42 promotes `[0x03B358,0x03B486)` (302 bytes) as
`CALLER_BACKED_RUNTIME_EXACT_ASM_ROUNDTRIP`. Exact `BSR` callers at
`0x03A900` and `0x03AA2A` target the entry; all 73 instruction starts in the
bounded interval are runtime-observed and decoded, and conditional branches
remain inside the interval before terminal `RTS` `0x03B484`. Independent
vasm output matches every canonical-ROM byte.

# M12-AUTO41 runtime-observed exact routine range

AUTO41 promotes `[0x03AE74,0x03B092)` (542 bytes) as
`RUNTIME_OBSERVED_EXACT_ASM_ROUNDTRIP`. The runtime evidence observes all
100 decoded instruction starts in the bounded range; the previous instruction
at `0x03AE72` is `RTS`, and the routine ends at `RTS` `0x03B090`.
Independently assembled vasm output matches every canonical-ROM byte. The
range remains developer-only provenance and does not begin C++ migration.

# M12-AUTO40 runtime-observed exact routine

AUTO40 promotes `[0x00B79A,0x00B852)` (184 bytes) as
`RUNTIME_OBSERVED_EXACT_ASM_ROUNDTRIP`. The complete decoded 68000 routine
ends at `RTS` `0x00B850`; its entry `0x00B79A` is present in the canonical
`GPGX_MANUAL_REALTIME` execution evidence, with 52 observed instruction
starts and no non-decoded runtime fact in the bounded interval. Independently
assembled vasm output is byte-exact against the canonical ROM. The transaction
is local-only evidence and does not begin C++ migration.

# M12-AUTO39 exact relative selector table

AUTO39 promotes `[0x15A9A6,0x15A9B0)` as
`RELATIVE_SELECTOR_OFFSET_TABLE_5X16`. Consumer `0x003EFA` derives the only
five selector offsets (`0,2,4,6,8`) from the bounded menu-state branches, then
uses `ADDA.W (A0),A0` to resolve each word relative to its own table field.
The exact values resolve to `0x15A9B0`, `0x15A9C4`, `0x15A9DE`, `0x15A9FE`,
and `0x15AA24`; those payloads are not promoted by this table-only proof.

# M12-AUTO38 exact menu offset table and record streams

AUTO38 promotes `[0x15B9D4,0x15BAC2)` as
`MENU_OFFSET_TABLE_AND_COUNT_BOUNDED_RECORD_STREAMS`. The four relative
offset words are `0x0008`, `0x00D4`, `0x00DA`, and `0x00E0`; they select streams
starting at `0x15B9DC`, `0x15BAAA`, `0x15BAB2`, and `0x15BABA`. Their counts are
`0x21`, `0`, `0`, and `0`, so the shared parser at `0x00B730` closes the
contiguous range with `2 + 6 * (count + 1)` bytes per stream. Callers at
`0x004AF2`, `0x004B08`, and `0x004B18` provide the exact selection contracts;
the next byte at `0x15BAC2` remains the independent AUTO36 graphics boundary.

# M12-AUTO37 exact 64-byte enum lookup

AUTO37 promotes `[0x05CE56,0x05CE96)` as `BYTE_ENUM_LOOKUP_TABLE`. Reader
`0x007A6C` loads the selector from `FF1976`, indexes all 64 bytes, and compares
against enum values through `4`; the preceding branch closes the selector at
`0..0x3F`, while `0x05CE96` is the next confirmed pointer table.

# M12-AUTO36 exact menu graphics streams

AUTO36 promotes three independent direct-consumer streams as
`DIRECT_MENU_GRAPHICS_STREAM`: `[0x15BAC2,0x15C238)` (1910 bytes),
`[0x15C238,0x15CA9B)` (2147 bytes), and `[0x15CA9C,0x15CEA0)` (1028 bytes).
Consumers `0x004966`, `0x004974`, and `0x004982` each load the corresponding
ROM address and call the verified decoder at `0x003820`; the local decoder
reproduces 3200, 3200, and 2656 output bytes respectively. The standalone
`0x15CA9B` byte is `0xFF` and remains UNKNOWN.

# M12-AUTO35 exact bit-7 lookup table

AUTO35 promotes `[0x05CE16,0x05CE56)` (64 bytes) as a
`BYTE_BIT7_LOOKUP_TABLE`. Consumer `0x00F61C` masks the word at `50(A6)` to
`0..63`, then tests bit 7 at the corresponding byte offset. The following
64-byte payload remains UNKNOWN; no neighboring bytes are included.

# M12-AUTO34 exact compressed-resource pointer table

AUTO34 promotes `[0x05CE96,0x05D046)` (432 bytes) as a
`COMPRESSED_RESOURCE_POINTER_TABLE`. Reader `0x00D3B2` uses a 4-byte index,
entry 0 is a zero sentinel, and entries 1..107 are strictly increasing
absolute pointers from `0x1AD000` through `0x1E6EDA`, all targeting already
owned decoder streams. The next confirmed table begins at `0x05D046`; no
target-stream bytes or semantic names are inferred by this promotion.

# M12-AUTO33 exact 64-entry item label table

AUTO33 promotes `[0x05CC16,0x05CE16)` (512 bytes) as a
`FIXED_WIDTH_ITEM_LABEL_TABLE` containing 64 printable 8-byte records.
Consumers `0x0041A6` and `0x0041CA` double the selected byte and shift left
three before indexing the table. The exact table hash and printable-record
contract close the range; the following binary payload remains UNKNOWN.

# M12-AUTO32 exact fixed-width menu label table

AUTO32 promotes `[0x05CBA6,0x05CBD6)` (48 bytes) as a
`FIXED_WIDTH_MENU_LABEL_TABLE` containing six 8-byte records. Consumer
`0x003F8E` forms `FF1861 << 3` and indexes the table; the exact switch at
`0x003DCC` accepts values 1..5 and the alternate path writes 5, closing the
effective selector at 0..5. The neighboring `0x05CBD6` data uses a different
format and remains the boundary.

# M12-AUTO31 exact sentinel threshold table

AUTO31 promotes `[0x05D906,0x05D918)` (18 bytes) as a
`SENTINEL_TERMINATED_THRESHOLD_TABLE`. Consumer `0x010666` compares each
post-incremented word, returns on `BLS`, increments its result by 8, and
therefore terminates on the exact ninth-word `0xFFFF`; the next confirmed
table begins at `0x05D918`. No neighboring mixed bytes are included and the
full-ROM rebuild remains byte-exact.

# M12-AUTO30 exact state dispatch pointer table

AUTO30 promotes `[0x00DF54,0x00E0B8)` (356 bytes) as an
`ABSOLUTE_STATE_DISPATCH_POINTER_TABLE`. Consumers at `0x00FD70`, `0x00FDF4`,
and `0x00FF7E` perform `LEA 0x00DF54`, double `D6`, read
`MOVEA.L -44(A0,D6.W),A0`, and execute `JSR (A0)`. The physical container is
89 exact longwords and ends immediately before the already-owned `RTS` at
`0x00E0B8`; the pointed-to handler bodies are not promoted. The canonical
ROM hash and full-ROM rebuild remain exact.

# M12-AUTO29 preserved candidate-map code census

AUTO29 adds no ownership. `src/tools/re_candidate_map_to_ghidra.py` rebuilds a
developer-only `oasis.m68k.ghidra-map.v1` from the preserved candidate-map
`ghidra_range` fields, yielding 496 bounded function ranges. Running the
strict transactional promoter against the AUTO28 manifest discovers 534
candidates but finds 0 eligible, 0 attempted, and 0 accepted candidates.
This is a negative result for the preserved code-census evidence set, not a
global blocker: no mixed or unbounded code is promoted, and data/parser
contracts remain the next provenance path.

# M12-AUTO28 exact nibble lookup

AUTO28 promotes `[0x062DC0,0x062DE0)` (32 bytes) as a
`PC_RELATIVE_NIBBLE_LOOKUP_TABLE`. The consumer at `0x0624D2` forms the table
base with `LEA 08EC(PC),A2`, masks `D3` with `0xF`, doubles the index, and
executes `MOVE.W 0(A2,D3.W),D0`. The promoter hash-checks all 16 canonical
word values, requires wholly UNKNOWN ownership, and verifies an exact full-ROM
rebuild. The consumer's surrounding code and neighboring mixed payload remain
UNKNOWN.

# M12-AUTO27 exact event dispatch table

AUTO27 promotes `[0x00532C,0x005378)` (76 bytes) as a
`SIGNED_RELATIVE_EVENT_DISPATCH_TABLE`. The exact owned dispatcher at
`0x00530C` clears `D0`, reads the event byte, subtracts `0x1A`, rejects the
negative result, doubles the selector, forms the table base with
`LEA 8(PC,D0.W),A0`, and performs `ADDA.W (A0),A0` before `JSR (A0)`. The
table contains 38 signed words: selectors 0..16 resolve into the bounded
handler cluster and selectors 17..37 resolve to the owned `RTS` at `0x00532A`.
The promoter validates the canonical ROM hash, the complete signed-offset and
target contract, wholly UNKNOWN ownership, zero gaps/overlaps, and a full-ROM
vasm rebuild. Handler bodies and the surrounding mixed region remain UNKNOWN.

# M12-AUTO26 exact field-86 callback entrypoints

AUTO26 promotes `[0x010000,0x010034)` and `[0x030000,0x030002)` as exact
callback code. `0x00E0BA` loads field `86(A6)` into `A0` and executes
`JSR (A0)`; bounded initializers set `0x10000` and `0x30000`. The 52-byte
callback and 2-byte `RTS` stub each reproduce the canonical bytes through
developer-only vasm assembly. The data-like/F-line candidate at `0x80000`
remains UNKNOWN because no closed code boundary was established.

# M12-AUTO25 exact PC-relative consumer tables

AUTO25 promotes 352 bytes from exact PC-relative consumers at `0x0086D6`,
`0x009322/0x0093B0/0x0094FE/0x0095CC`, `0x00A354/0x00A360`,
`0x00B48C/0x00B49E/0x00B524`, `0x01FBDC/0x01FBE8`, `0x0292BE`,
`0x02ADE/0x03C0B0`, and their corresponding closed ranges. Masked lookup
indices, exact sequential DBF loops, fixed copy counts, or terminal code
boundaries establish each half-open range. No surrounding mixed payload or
decoder-only candidate is included.

# M12-AUTO24 exact static caller-backed island

AUTO24 closes `[0x0167BE,0x01685A)` as a 156-byte exact static code island.
The Ghidra census records caller xrefs at `0x01672E` and `0x016742`; a bounded
exact decoder accepts the complete range with no gap, overlap, unsupported
form, or unresolved dispatch, and vasm reproduces the canonical bytes. The
decoder currently prints the terminal `C9 4E` as `exg.w D4,A6`; independent
68000 opcode assembly identifies the exact spelling as `EXG A4,A6`, so the
developer-only promoter normalizes only that generated line before assembly.
No ROM byte is changed and no wider neighboring range is inferred.

# M12-AUTO23 save serialization slots

AUTO23 closes two save-related ranges without treating the surrounding SRAM
image as owned data. The primary range `[0x2025CD,0x2026E1)` is exactly the
stride-2 extent established by the six-byte tag loop, 130-iteration payload
loop, and two-byte checksum in the routines at `0x001DDC` and `0x001E46`.
The secondary range is limited to `[0x2026E1,0x2026F5)`: the checker at
`0x001EDE` consumes six stride-2 tags and the writer at `0x001F26` emits four
stride-2 value bytes. Bytes after `0x2026F5` remain UNKNOWN; a save/SRAM
address alone is not a container boundary. The exact canonical ROM hashes
remain unchanged.

# M12-AUTO22 exact small tables

The AUTO22 transaction closes three fixed data units from exact consumer
contracts: `[0x3E0B8,0x3E0D8)` is copied as eight longwords by `0x03E516`,
`[0x522E,0x5236)` is read as eight selector bytes by `0x0051CE`, and
`[0x5CBD6,0x5CBE8)` is read as six three-byte records by `0x005120`. These
are bounded table facts only; surrounding mixed regions remain UNKNOWN.

# M12-AUTO21 nested CC-B0 target tables

The exact consumer at `0x00CCB0` selects one of the 32 longword targets from
`[0x04371E,0x04379E)`, masks the low selector byte, scales it by two, and
reads a signed relative word before adding it to the entry address. Therefore
each selected target has a closed 256-slot / `0x200`-byte table contract. AUTO21
promotes only the union of the 20 unique windows, splitting around prior
AUTO15 slot ownership. The target records reached by those relative words are
not promoted.

# M12-AUTO19 Ancient Music Driver data-archive provenance checkpoint

The AUTO19 transaction is
`build/m12-auto19-sound-data-transaction-a/materialized/manifest.json`. It
promotes exactly `0x064E38..0x07D780` (100,680 bytes) as
`SOUND_DATA_CONTAINER_CONFIRMED`. The exact 68000 consumer at `0x060B50`
loads the offset table rooted at `0x0638D4`; the transaction validates the
34-entry primary group, 126-entry secondary group, and 17-entry tertiary group
with respectively 29, 126, and 4 targets inside the bounded data container.
The remaining 13 tertiary targets are outside the container and are rejected.
The next range `0x07D780..0x080000` is byte-exact `0xFF` fill, providing the
terminal boundary. This is a raw sound-data ownership contract, not an audio
semantic or C++ migration claim. The full-ROM hashes remain canonical.

# M12-AUTO18 exact 0x3820 graphics-consumer provenance checkpoint

The AUTO18 transaction is
`build/m12-auto18-graphics-consumers-transaction-d/materialized/manifest.json`.
It adds two UNKNOWN streams, `0x167E48..0x16821F` (983 bytes) and
`0x168442..0x168492` (80 bytes). The first is selected by exact consumers at
`0x02D444` and `0x02E204`; the second is selected at `0x02D416`. Each calls
`0x00D9A4`, whose exact helper calls `0x0037D2`, and `0x0037D8` has the exact
direct branch to `0x003820`. The local graphics decoder consumes exactly 983
and 80 bytes. AUTO17 runtime-correlated streams are revalidated in the same
transaction. Surrounding payload remains UNKNOWN; canonical hashes are
unchanged and no C++ migration is started.

# M12-AUTO17 runtime-correlated graphics-stream provenance checkpoint

The AUTO17 transaction is
`build/m12-auto17-runtime-graphics-transaction-d/materialized/manifest.json`.
It closes two UNKNOWN ranges, `0x15E052..0x160E19` and
`0x2119D2..0x211F79`, totaling 13,166 bytes. Existing GPGX ROM-reader
correlation records each range as first-read by PC `0x003830`, inside the
already bounded exact graphics decoder at `0x3820`; the independent runtime
execution evidence contains `CODE_EXECUTED_AT_ADDRESS` for that PC. The local
graphics inspector consumes exactly 11,719 and 1,447 bytes respectively.
The ROM also contains five independent pointer literals for the first range
and one for the second. This closes source ownership of the exact compressed
stream bytes only; the surrounding payload and descriptor/table semantics
remain UNKNOWN. Canonical full-ROM hashes remain unchanged and no C++
migration is started.

# M12-AUTO16 exact runtime-correlated probe-slice provenance checkpoint

The AUTO16 transaction is
build/m12-auto16-exact-probe-transaction-b/materialized/manifest.json. It
closes nine exact 68000 probe slices totalling 1,430 bytes:
0x008F12..0x009330 in six bounded slices, 0x03B1D0..0x03B358,
0x060090..0x060286, and 0x061232..0x061328. Each slice has a
canonical-equal local binary, decoder-bounded ASM and at least one observed
instruction start in the canonical runtime evidence. The map reaches 988,222
source-owned bytes (31.4147313436%) with canonical full-ROM hashes unchanged.
This is PC-level corroboration only; it does not assign gameplay semantics or
close adjacent mixed ranges. The transaction uses inherited-baseline exactness
because no external vasm executable is installed; no C++ migration is started.

# M12-AUTO15 CC-B0 selected relative-pointer-slot provenance checkpoint

The AUTO15 transaction is `build/m12-auto15-ccb0-selected-transaction-d/materialized/manifest.json`.
It closes 15 unique 16-bit slots (30 bytes) selected by exact constant-D0
callers that jump to `0x00CA24`. Each slot is reached after the closed
`0x04371E..0x04379E` group table and resolves as a signed relative pointer to
an even canonical-ROM target. Dynamic callers and nested table extents remain
UNKNOWN. The transaction reaches 986,792 source-owned bytes
(`31.3692728678%`) with canonical full-ROM hashes unchanged. No C++ migration
is started.

# M12-AUTO14 CC-B0 group pointer-table provenance checkpoint

The AUTO14 transaction is `build/m12-auto14-ccb0-transaction-c/materialized/manifest.json`.
It adds 128 bytes for the physical 32-entry longword table
`[0x04371E,0x04379E)` selected by the exact consumer at `0x00CCB0`. All table
entries are even valid canonical-ROM targets; nested target subtables and
semantic selector bounds remain UNKNOWN. The transaction reaches 986,762
source-owned bytes (`31.3683191935%`) with canonical full-ROM hashes
unchanged. No C++ migration is started.

# M12-AUTO13 BCEA field3 list provenance checkpoint

The AUTO13 transaction is `build/m12-auto13-field3-transaction-f/materialized/manifest.json`.
It adds 7,516 bytes in five merged ranges from 100 list views selected by the
exact routine at `0x00BCEA`. The routine transforms the field3 selector into
offsets `-20`, `0`, `20`, and `40`, reads a signed relative pointer from each
slot, advances positive rows by 44 bytes, and stops at a negative key sentinel.
The 30 field3 bases come from the exact 99-record table at
`0x03F306..0x03FF66`; the remaining `0x58000..0x5D046` container bytes remain
UNKNOWN. This is a raw structural contract: semantic field names and adjacent
records are not asserted. Full-ROM equality and canonical hashes remain exact;
no C++ migration is started.

# M12-AUTO10 exact code continuation checkpoint

The AUTO10 transaction is `build/m12-auto10-code-transaction-b/manifest.json`.
It promotes two new exact, caller-backed 68000 islands totaling 222 bytes:
`0x00F0EC..0x00F10C` and `0x00B28E..0x00B34C`. The automatic promoter first
reassembles each candidate and then verifies the complete ROM; 12 additional
eligible candidates are rejected as unsupported exact IR. A compatibility fix
now carries every baseline entry with `emitted_artifact_type=asm`, including
the M12 header/vector entry, through candidate trials and final materialization.
The canonical ROM hashes remain unchanged. No C++ migration is started.

# M12-AUTO9 erased-alignment provenance checkpoint

The current local M12 transaction reaches 927,778 bytes (29.493268331%) of
source-owned ROM while preserving the canonical ROM SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`. AUTO9
adds 16 non-overlapping UNKNOWN ranges totaling 132,630 bytes. Each range is
entirely `0xFF`, is at least 256 bytes, begins at an UNKNOWN boundary or after
non-fill, and ends on a 4 KiB ROM alignment boundary:
`0x0551B6..0x058000`, `0x05E1A0..0x060000`, `0x07D780..0x080000`,
`0x0879F9..0x088000`, `0x08FA1E..0x090000`, `0x097C60..0x098000`,
`0x09FC3A..0x0A0000`, `0x0A7C85..0x0A8000`, `0x0AF54A..0x0B0000`,
`0x0B7B45..0x0B8000`, `0x0BF768..0x0C0000`, `0x144A04..0x150000`,
`0x16F9CA..0x170000`, `0x1A4DC6..0x1AD000`, `0x25FEC2..0x260000`, and
`0x2FDD91..0x300000`. They are classified as
`ERASED_ROM_ALIGNMENT_PADDING`, not executable code or game data. Zero-filled,
mixed, and decoder-only candidates remain UNKNOWN. The exact transaction is
`build/m12-auto9-erased-alignment-transaction-b/materialized/manifest.json`;
the helper is `src/tools/re_m12_padding_promote.py` with regression
`tests/re_m12_padding_promote_test.py`. Full evidence is in
`docs/reports/ASM_AUTONOMOUS_PROVENANCE_TO_90_PERCENT_M12_AUTO2.md`.

# M12-AUTO8 direct-graphics chain provenance checkpoint

The current local M12 transaction reaches 795,148 bytes (25.277074178%) of
source-owned ROM while preserving the canonical ROM SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`. Exact
consumer evidence now includes five new direct graphics continuation streams
totaling 21,016 bytes: `[0x18955A,0x18CC6E)` and `[0x18D01B,0x18EB1F)`,
selected by exact `0x3820` consumers and sequential decoder-advanced `A0`; the
anchors at `0x172168`, `0x1894EA`, and `0x18CF98` were already owned. The
bounded 50-record, 32-byte-stride
table `[0x5D046,0x5D686)` selected by consumers at `0x00D72C`, `0x010086`,
`0x0100AA`, and `0x039100` remains included. The following 64-entry word lookup table
`[0x5D686,0x5D706)` and 512-byte indexed lookup table
`[0x5D706,0x5D906)`, with direct consumers at `0x00302E`, `0x010684`,
`0x01108C`, `0x0161FA`, and `0x030200`. The nested table at `0x5D918`, its exact
count-bounded group region `[0x5D958,0x5DB44)`, and 185 unique
pointer-backed NUL-terminated records in `[0x5DB44,0x5E1A0)`. The earlier
`0x51514` indexed script table, the
`0x00C2EC` loop over indices `0x00..0x61` excluding `0x1B..0x1D`, and 98
NUL-terminated streams resolved by `0x00C326`, in addition to the 68000 Z80
upload at `0x06134E`, direct
`0x3820`/`0x37D2` graphics calls, the `0x03B8DE` 16-byte table selected by
`0x03A9EE` and consumed by `0x03B1D0`, and eight 1208-byte records copied by
`0x0012E8`. Decoder census candidates without a closed table/consumer edge
remain UNKNOWN. Full evidence is in
`docs/reports/ASM_AUTONOMOUS_PROVENANCE_TO_90_PERCENT_M12_AUTO2.md`.

# M12-AUTO5 exact lookup-table boundary

The exact transaction is
`build/m12-auto5-lookup-transaction-b/materialized/manifest.json`. The word
table `[0x5D686,0x5D706)` is exactly 64 `0x0EEE` words and is read by
`0x00302E`; callers include a `DBF` count of `0x3F`. The byte table
`[0x5D706,0x5D906)` is exactly 512 bytes; consumers index it after an explicit
`ANDI.W #$1FE` at `0x030200` and equivalent doubled-angle accesses at
`0x010704`, `0x0110E6`, and `0x01626A`. No bytes after `0x5D906` are included.

# M12-AUTO7 direct-graphics boundary

The exact transaction is
`build/m12-auto7-direct-graphics-transaction-b/materialized/manifest.json`.
The direct consumers at `0x003278`, `0x004E8`, `0x004F2`, `0x004FC`, `0x0050A`,
`0x003356`, and `0x00335C` select or continue the seven listed streams. The
local decoder reported source lengths 979, 2,145, 182, 1,700, 95, 73, and
34,891 bytes respectively. The adjacent fifth call after the `0x004E8..0x0050A`
chain was decoder-rejected and remains UNKNOWN.

# M12-AUTO6 fixed-stride-table boundary

The exact transaction is
`build/m12-auto6-fixed-stride-transaction-a/materialized/manifest.json`. The
range `[0x5D046,0x5D686)` is exactly 50 records of 32 bytes. Consumers at
`0x00D72C`, `0x010086`, `0x0100AA`, and `0x039100` all use the literal ROM base
`0x5D046`, select with `index << 5`, and copy eight longwords. The upper
boundary is the start of the independently confirmed `0x0EEE` word table;
no semantic field names or neighboring UNKNOWN bytes are asserted.

# M12-AUTO4 nested level-table boundary

The exact transaction is
`build/m12-auto4-level-transaction-a/materialized/manifest.json`. The outer
table `[0x5D918,0x5D958)` contributes 64 bytes, its contiguous non-empty
count-bounded groups `[0x5D958,0x5DB44)` contribute 492 bytes, and 185 unique
inner-pointer records contribute 1,613 bytes. The 186 non-zero inner edges
resolve to those 185 records. One 8-byte record-shaped span at
`0x5E0FE..0x5E106` is not pointer-backed by the closed graph and remains
UNKNOWN. No semantic level/text interpretation is required for ownership.

# M11.64 G0 portability boundary closure

The consolidated bounded ledger is `docs/RE_LEDGER.md`; the full evidence report
is `docs/reports/G0_PORTABILITY_BOUNDARY_M11_64.md`. G0 parent-owned lifetime is
proven, the natural 0x0623AC VDP dependency is proven, and typed-data ownership
is blocked. `0x060286`, latent `0x062CEC`/`0x061F60`/`0x062878`, external
writers/aliasing and broader `0x061258` lifetime remain unresolved. No new
callee was reverse-engineered.

# M11.63 — 0x0623AC natural callee contract — CONFIRMED NATURAL / HARDWARE BLOCKED

The bounded slice rooted at `0x0623AC` decodes 352 instructions in 135 blocks,
86 direct branches, 16 direct calls and one unresolved indirect call at
`0x062878`; static status is `INDIRECT_CFG`. Natural G0 execution has 1,944
entries and returns across parent call sites `0x060234`, `0x060242`, `0x060250`
and `0x060276`, each 486. A5 equality is 1,944/1,944. Nested direct calls
return 1,297/1,297 and indirect calls return 12/12; indirect targets are
`0x0621F8`, `0x062218`, `0x062900` and `0x06293A`.

The complete natural effect set is 24,993 events: G0-relative 2,483,
safe-RAM 17,361, ROM reads 104, stack 4,638 and hardware 407. The hardware
set is six byte writes to VDP register `0x00C00011`; no interrupts or unresolved
runtime addresses occur. The natural result is
`CALLEE_0623AC_BLOCKED_HARDWARE`; A5 is `A5_PRESERVED_EXACT`, while the static
indirect boundary remains separate evidence. Full evidence:
`reports/CALLEE_0623AC_CONTRACT_M11_63.md`.

# M11.62 — 0x061934 natural callee contract — CONFIRMED

The bounded transitive slice rooted at `0x061934` decodes 475 instructions in
156 blocks with 105 direct branches, 12 direct calls and one unresolved
indirect call at `0x061F60`; static status is `INDIRECT_CFG`. Natural G0
execution has 2,916 entries and returns across six parent call sites (486 each).
A5 equality is 2,916/2,916. Direct consumers `0x06193C` (+0 read) and
`0x061946` (+4 write) occur 2,430 and 1,361 times. All 2,655 direct and 13
indirect nested calls return with equal A5; indirect targets are
`0x06211A`, `0x06202C`, `0x062048` and `0x0620B0`. The complete natural effect
classes are G0-relative, safe-RAM, ROM-read and stack; hardware and unresolved
active effects are zero. The narrow result is
`CALLEE_061934_NATURAL_CONTRACT_PROVEN`; the static indirect boundary is
reported separately. Full evidence:
`reports/CALLEE_061934_CONTRACT_M11_62.md`.
# Reverse-Engineering Ledger
This file records what is known about the original Beyond Oasis binary. Do not promote guesses to facts without evidence.

## M12.4 — ROM-start ownership — `M12_4_ROM_START_PROMOTION_PARTIAL_EXACT`

Against baseline `3e667c304382c5bce81d2e5a4ff75751139db314`, the bounded
`0x000000..0x0007C4` region is classified as mixed ROM structure. The fixed
vector table `0x000000..0x000100` and fixed Genesis header
`0x000100..0x000200` are explicit `dc.l`/`dc.b` source ownership; vector
entries are not executable instructions. The reset vector is `0x00020E`.

Closed code ownership is `0x000200..0x00020E`, `0x00020E..0x00029C`,
`0x000308..0x00045A`, and `0x0006F6..0x0007C4`, materialized as eight exact
ranges totaling 700 bytes. The startup table `0x00029C..0x000308` is
`STRUCTURED_DATA_CONFIRMED` from the reset routine's PC-relative `MOVEM` and
postincrement consumers. The range at `0x00045A` stops before unresolved
indirect `JSR (A1)`; the remaining intervals are `0x00045A..0x00045E`
UNRESOLVED_BOUNDARY, `0x00045E..0x0004C6` UNKNOWN_DATA, and
`0x0004C6..0x0006F6` POSSIBLE_CODE.

Per-range vasm (`-m68000 -no-opt -Fbin`) and full-ROM exactness passed. `MOVE
USP` is represented by exact `dc.w` because this vasm build reverses the
`0x4E6x` encoding for its USP operand spelling. Deterministic locks cover
vector/header boundaries, startup ranges, target/full exactness, and manifest
gaps/overlaps. Full report:
`docs/reports/ASM_PROMOTION_000000_0007C4_M12_4.md`.

## M12.3 — P0 region `0x006516..0x0083D4` — PARTIAL EXACT

The M12.3 transaction promotes 696 bytes in fourteen exact, non-overlapping
`68000_CODE_CONFIRMED` intervals. Evidence combines the 25 observed PCs in
`0x0082FC..0x00832C`, Atlas-local verified entries at `0x007A28` and
`0x0082AE`, the direct caller at `0x007F98`, direct CFG edges, exact
fallthrough, neighboring ASM, and vasm byte round trips. The reported zero
static xrefs is scoped to the priority subregion; it does not negate the
containing `0x0082F8` entry or its observed interior execution.

The decoder/reassembler now covers dynamic bit operations, including
`BSET D1,(A1,D0.W)` at `0x007B24` and `BSET D0,($00FF0DBA).L` at `0x007BEC`.
The exact byte-immediate `0xFF` extension case is retained with raw `dc.w`
words because vasm normalizes that encoding when written as a mnemonic. These
are encoding facts; no gameplay semantics are inferred. The remaining 7,174
bytes stay `UNRESOLVED_BOUNDARY` because their entry, continuation, or
neighboring dispatch ownership is not independently closed. Full evidence:
`reports/ASM_PROMOTION_006516_0083D4_M12_3.md`.

## M12.2 — P0 region `0x00DE00..0x00E338` — PARTIAL EXACT

The M12.2 transaction promotes 366 bytes in 13 exact, non-overlapping
`68000_CODE_CONFIRMED` intervals. Evidence combines 31 observed PCs, 87 static
incoming xrefs, direct branches/calls, fallthrough, surrounding ASM, decoder
output and vasm 1.8g byte round trips. The bounded routines at
`0x00DEEC`, `0x00E0BA`, `0x00E106`, `0x00E268`, and `0x00E2A2` retain their
indirect JSR sites as explicit exits. The `0x00E2D4` prefix stops immediately
before the unresolved indirect JMP at `0x00E2F0`.

The repeated `0xD140` words are normalized as `ADDX.W D0,D0`; `0xC2C0` as
`MULU.W D0,D1`; and the repeated `0xB141` words as exact `EOR.W D0,D1`.
These are decoder/reassembly facts, not gameplay semantics. The remaining 970
bytes stay blob-backed as `UNRESOLVED_BOUNDARY`; no `POSSIBLE_CODE`, structured
data, padding, or `UNKNOWN_DATA` ownership is claimed. Full evidence:
`reports/ASM_PROMOTION_00DE00_00E338_M12_2.md`.

# M12.1 — P0 region 0x06042A..0x0611F4 — PARTIAL EXACT PROMOTION

The M12.1 transaction promotes six exact bounded ASM slices totaling 546
bytes: `0x06042A..0x060484`, `0x060490..0x0604B0`, `0x060B50..0x060CDA`,
`0x0611D6..0x0611E0`, `0x0611E0..0x0611EA`, and `0x0611EA..0x0611F4`.
The two larger slices have prior execution evidence (`CODE_EXECUTED`); the
other four are statically supported exact slices. The current decoder handles
the three legacy `MOVE SR` forms at `0x06042A`, `0x0611DC`, and `0x0611E6`.

Each promoted slice has exact local vasm reassembly, no unsupported instruction
record, and no unresolved memory reference. Direct branch targets, external
continuations and returns are retained as evidence; no gameplay semantics or
portable C++ routine is inferred. The remaining 2,984 bytes stay UNKNOWN
blob-backed because their dispatch/case and linear/CFG boundaries are not
closed. Full report: `reports/ASM_PROMOTION_06042A_0611F4_M12_1.md`.

# M11.61 — 0x062AE0 callee contract — CONFIRMED NATURAL / STATIC NEGATIVE

The callee rooted at `0x062AE0` has six static RTS exits (`0x062B1A`,
`0x062C36`, `0x062CBC`, `0x062D0A`, `0x062D62`, `0x062D6A`), one direct nested
BSR at `0x062B4E -> 0x062D4C`, and one unresolved indexed JSR at `0x062CEC`.
The expanded bounded CFG is therefore `INDIRECT_CFG`. In the M11.60 natural
G0 distribution only the early `0x062B1A` return occurs: 486 entries and
486 returns, with A5 entry/exit equality 486/486 and no nested-call executions.

The complete observed data set is 2,302 effects: safe-RAM `FF0013` reads,
G0-relative `FF001A` reads and `FF001E` writes, A4/A6-derived safe-RAM byte
effects, and stack return-longword reads. Hook types inside the interval are
execution/data only; no hardware event occurs. Three paths occur with counts
310, 175 and 1. The natural result is
`CALLEE_A5_PRESERVATION_PROVEN_EFFECTS_BLOCKED`: preservation and natural
effects are proven, while the latent indexed CFG prevents an all-static
whole-callee effect claim. Full evidence:
`reports/CALLEE_062AE0_CONTRACT_M11_61.md`.

# M11.60 — bounded A5 consumer/lifetime closure — CONFIRMED

G0 is the value materialized by `0x060182 LEA.L $FF001A,A5`. Exact bounded
CFG evidence gives predecessor `0x060170 -> 0x060182`, a natural always-taken
`0x06018E BCC.W 0x0601D4`, and a parent-owned continuation ending at
`0x06027E MOVEM.L (A7)+,D0-D7/A0-A6` before `0x060284 RTS`. The fallthrough
arm containing `0x06019E CMP.B 7(A5),D0` and `0x0601A6 MOVE.B D0,7(A5)` is
dead because `TST.B` clears C. Live selected consumers are `0x06193C
BTST.B #0,0(A5)` (+0 read, 10 natural events) and `0x061946 MOVE.B D7,4(A5)`
(+4 write, 1,361 events); `0x061998 MOVE.B 4(A5),(A4)+` is static but was not
reached in the 600-frame trace. No G0 spill, reload, arithmetic or direct A5
overwrite was observed. The endpoint is `A5_LIFETIME_MERGES_WITH_PARENT`.

The opt-in developer-only observer produced three byte-identical traces with
486 generations and 486 parent restores. Five thousand eight hundred thirty-two
direct call entries and callee entries were observed; the simple return pairer
records 4,860 returns and is not treated as whole-call preservation proof.
The raw transaction is therefore `BOUNDED_A5_TRANSACTION_BLOCKED_PARENT_LIFETIME`;
the relation to M11.58 is `ARE_ALTERNATE_PRODUCER_CONSUMER_PATHS`. The typed
gate remains `TYPED_DATA_BLOCKED_OVERLAPPING_ACCESS`. Full evidence:
`reports/A5_CONSUMER_LIFETIME_M11_60.md`.

# M11.59 — raw data alias/lifetime closure — CONFIRMED NEGATIVE

Result: `RAW_DATA_ALIASING_BOUNDARY_PROVEN_TYPED_DATA_BLOCKED`. The all-ROM
census confirms 60 exact fixed absolute/LEA references among 66 raw candidates
for `FF0010..FF0016`, `FF0628` and `FF06F2`. A5=`FF001A` is materialized at
`060182`, `060434` and `061258`; the latter saves A5 on the stack, clears via
`(A5)+` for `0x762` iterations and reloads it. Separate consumers use
`0(A5)`, `4(A5)`, `5(A5)` and `7(A5)`, proving overlapping alias/lifetime
boundaries. Sibling writers `60BD2/60BDC/60BE2/60BE8/60BEE` match the raw
zero-write order but follow an A11100 hardware prefix and remain
`HARDWARE_ORDERED_WRITER` contexts. Candidate groups are A
`SHARED_STATE_WINDOW`, B `ALIASING_PREVENTS_BOUNDARY`, C/D
`INDEPENDENT_SCALAR`, E `INSUFFICIENT_EVIDENCE`. No typed structure or
production routine was promoted. Full evidence: `reports/RAW_DATA_OWNERSHIP_M11_59.md`.

# M11.58 — portable behavior cluster raw-data census — CONFIRMED

The proven composition is `ParentSuffix -> RamFlagRoutine -> ParentSuffix`
with an opaque parent handoff. Ordered effects are byte writes at `FF0012`,
RamFlag read-modify-writes at `FF0628`/`FF06F2`, writes to the supplied
`FF001A + 5..7` range, `FF0016`, then `FF0010`, `FF0011`, `FF0013` and
`FF0014`. The exact bounded access PCs, widths and ordering are recorded in
`reports/PORTABLE_BEHAVIOR_CLUSTER_M11_58.md`.

Ownership census: `FF0010..FF0014` are shared with known external callers;
`FF0628`/`FF06F2` are addressed by known code outside the composition;
`FF001A + 5..7` have unresolved alias and lifetime boundaries; `FF0016` has a
bounded writer but no exclusivity proof. No access is hardware-visible inside
the core cluster. Classification:
`PORTABLE_BEHAVIOR_CLUSTER_CONTRACT_PROVEN_REPLACEMENT_BLOCKED` with
`TYPED_DATA_CONTRACT_BLOCKED_SHARED_WRITERS_UNRESOLVED_ALIAS_LIFETIME`.
No typed data, subsystem or new routine is promoted.

# M11.57 — parent-owned suffix helper — CONFIRMED

The exact sequence `604F0 SF.B`, `604F6 BSR 604BC`, `604FA/60500/60506/6050C
SF.B`, `60512 BRA 611D6` is a hardware-free parent suffix. Its entry requires
the parent-established A5=`FF001A` and an even A7; it consumes no saved-frame
bytes. Its ordered writes are `FF0012`, `FF0010`, `FF0011`, `FF0013`, `FF0014`,
plus the existing RamFlag writes. A7 is restored by the composed RamFlag RTS;
full SR, stack banks, shared epilogue and final RTS remain parent-owned.
The portable contract and all 17 represented instruction boundaries are
independently tested, with a structural RamFlag composition and opaque parent
continuation token. Natural shadow and native 600-frame evidence are recorded
in `reports/PARENT_SUFFIX_HANDOFF_M11_57.md`.

# M11.56 — 0x604F0 parent-frame ownership — CONFIRMED bounded negative

Result: THIRD_ROUTINE_NOT_A_STANDALONE_ROUTINE. Exact natural predecessor
is 0x604EA (BSET), not a standalone call. Parent entry 0x60004 is reached
from JSR at 0x41E, with return 0x424 at A7=FF0BEA. The 0x6042A/0x60430
prologue saves SR and D1-D7/A0-A6, leaving a 58-byte frame at FF0BB0.
0x604F0 inherits that frame; its top longword 7FF0FFFF is saved D1.
The shared epilogue 0x611D6..0x611E0 sets D0=0, restores 14 registers,
performs a pinned GPGX extra word read at FF0BE8, restores full SR=2114,
and returns at 0x611DE to 0x424 with A7=FF0BEE. Classification:
ENCLOSING_ROUTINE_CONTINUATION, using a shared tail, not an independent tail call.

Boundary correction: selected seven instructions occupy [0x604F0,0x60516).
The old budget [0x604F0,0x60520) also contains a different arm entered by
0x60480 -> 0x60516 and cuts an instruction at 0x6051C. Parent-scope CFG
finds 15 branches plus fallthrough to 0x611D6 and another entry at 0x611D8.
Global external/indirect entry exclusion is UNKNOWN, not inferred from the
bounded decoder's zero unresolved count. The SF forms are unconditional
zero writes; this corrects the generic unknown-condition description in M11.55.

Paired EMULATED runs produce byte-identical parent/entry/exit register,
stack, cycle/refresh and journal evidence: 21 selected path instructions and
32 ordered main-RAM data accesses. Parent-established A5=FF001A resolves
the natural output writes to FF001F/20/21. Saved-frame bytes and returns
are independently validated. Broader aliasing and arbitrary entry are UNKNOWN.
Whole-parent prefix expansion encounters A11100/A00003/C00011 hardware via
0x604E6 -> 0x611EA; full-SR/event portability remains SEMANTICS_PARTIAL.
No 0x60BCC expansion, production translation, typed data or subsystem claim.

Tests: observer regression, local independent validator with three negative
controls, Debug/Release/GCC-UCRT CTest 68/68 each and exact pre/post dual-native
gates. Assumptions/complete instruction ledger, timing, addresses and remaining
unknowns: reports/RAMFLAG_CALLER_ROUTINE_M11_56.md. Translation status: STOP.

# M11.55 — RamFlag caller and shared-data contract closure
STATUS: RAMFLAG_CALLER_CONTRACTS_PROVEN.

The unchanged dual-native 600-frame proof reproduced twice with exact state
checkpoint `251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d`,
video `5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`,
6,488,773 total instructions, 6,488,699 interpreter instructions, 34
TableCopy instructions, 40 RamFlag instructions, zero fallback/divergence and
one yield/resumption. The new developer-only observer is deterministic and
byte-identical across runs.

Every natural entry to `0x0604BC` was dynamically paired with its direct BSR.W
site and stacked return: ordinal 1 is `0x0604F6 -> 0x0604BC` with return
`0x0604FA`; ordinals 2–4 are `0x060BCC -> 0x0604BC` with return `0x060BD0`.
All records have `unknown_count=0`, caller PCs `0x0604F0` and `0x060BC4`,
and include A7, frame, cycles/refresh and all D/A/SR values. The `0x0604F0`
label corrects M11.54's provisional `0x0604EC`: ROM bytes at `0x0604EC..EF`
are zero data.

Exact slices prove 7 instructions/1 block/1 direct call/1 direct branch for
`[0x0604F0,0x060520)`, with edge `0x0604F6 -> 0x0604BC` and exit
`0x060512 -> 0x0611D6`. The `0x060BC4..0x060CDA` block has 84 instructions;
the wider `[0x060B50,0x060E50)` slice has 111 instructions, 9 blocks, 11
direct calls, 6 direct branches, zero unresolved control-flow edges and 25
unresolved memory references. It is therefore a bounded caller region, not a
closed routine.

Address provenance proves fixed byte writes to `FF0010`, `FF0011`, `FF0012`,
`FF0013`, `FF0014` and internal `FF0016`; `FF0015` has no fixed absolute access
in the audited paths. The `0x060BD8` access to `FF001A`, A5-relative writes,
field lifetime, aliasing and type remain unknown. The typed shared-data gate is
blocked.

Raw decoding proves `0x060BC4` writes word zero to hardware register
`0x00A11100` before the RamFlag call. Runtime provenance observes that hardware
access at `0x060BC4` only, so the exact handoff has an adapter-owned hardware
prefix and portable RamFlag/data suffix. The whole caller region remains
hardware-boundary incomplete because sibling calls and memory effects are not
closed. No caller, typed structure or subsystem is promoted. Full evidence:
`reports/RAMFLAG_CALLER_DATA_CLOSURE_M11_55.md`.

# M11.54 — Native routine cluster and first subsystem boundary discovery
STATUS: PORTABLE_ROUTINE_CLUSTER_PROVEN.

The unchanged M11.53 dual-native 600-frame baseline reproduced twice with
checkpoint aggregate 251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d,
video 5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58,
6,488,773 total instructions, 6,488,699 interpreter remainder, 34 TableCopy,
40 RamFlag, zero fallback/divergence and one RamFlag yield/resumption.

Exact static provenance proves 0x2D58 -> 0x2D66 (return 0x2D5C) and
0x604F6 -> 0x604BC (return 0x604FA), 0x60BCC -> 0x604BC (return 0x60BD0).
Dynamic target counts are 1 and 4, but caller PCs were not captured; those
edges remain STATIC_PROVEN rather than BOTH. The RamFlag callers share only
the raw byte window 0x00FF0010..0x00FF0016. Its field lifetime, aliasing and
meaning are unknown, and 0x60BCC is hardware-coupled through 0x00A11100.
No edge or shared structure connects the two authoritative native routines.

No typed structure, gameplay meaning or subsystem boundary is promoted.
Nearby 0x61032 and 0x3820 remain continuation-blocked by routine-specific
contracts; 0x6121A remains hardware-blocked. Full discovery evidence is in
reports/NATIVE_ROUTINE_CLUSTER_M11_54.md.

# M11.53 — Second portable native routine
STATUS: SECOND_PORTABLE_NATIVE_ROUTINE_PROVEN.

The bounded USA-ROM leaf 0x604BC..0x604E6 is a complete routine contract:
ten instructions, one CFG block, one RTS at 0x604E4, no calls, indirect
edges or loops, four natural invocations and only bounded main-RAM flag/output
plus caller-stack effects. BSET updates only Z from the tested pre-operation
bit; the three postincrement and one absolute SF writes are zero-byte writes.
oasis_core owns the portable structured executor and opaque continuation
tokens. The developer-only adapter owns exact bytes, fetch/begin/finish,
prefetch/IR, refresh/boundary and RTS state.

Independent core vectors and an adapter fetch/finish regression pass. A paired
600-frame dual native proof with 0x2D66 matches the frozen checkpoint
aggregate/video identity and closes 6,488,773 as 6,488,699 interpreter + 34
TableCopy + 40 second-routine instructions. Nearby candidates remain
CONTINUATION_BLOCKED (0x61032 and 0x3820), HARDWARE_BLOCKED (0x6121A),
SEMANTICS_BLOCKED (broad graphics slices) or ROUTINE_CONTRACT_PARTIAL
(isolated M11.47 forms). No subsystem boundary or gameplay meaning is
assigned.

The authoritative native adapter uses the shared block-hook continuation path:
the unchanged 600-frame run recorded one event boundary yield and one resume
at opaque continuation token 0x604DA, with zero fallback or divergence. The
natural shadow remained 5/5 with zero divergence.

# M11.52 — Native routine checkpoint mismatch root-cause closure
STATUS: `FIRST_PORTABLE_NATIVE_ROUTINE_PROVEN` for the bounded
`TableCopyRoutine` at `0x002D66..0x002D84`.

M11.51's four canonical checkpoint differences were not representation noise.
At frame 120 the bytes were serialized offsets `3060`, `144468`, `144482` and
`144558`. Offset 3060 is work RAM `0xFF0BE4`; 144558 is `Z80_Regs.iff1` at
the pinned Z80 base `144504 + 54`; 144468 and 144482 are in the pinned sound
semantic region between the YM2612 and Z80 objects, with exact PSG field names
left unresolved because the M11.43 source/object layout comparison found a
124-byte sound-layout discrepancy. None is within a host representation span.

Temporal evidence showed equal entry/exit cycles (`193626` to `196454`) but
M11.51 native refresh ending at `194704` instead of the reference `196622`.
The adapter had collapsed 34 represented 68000 instructions into one cycle
delta and one refresh update. The portable routine's register, stack, output
and return semantics were not the cause. The hybrid adapter now drives each
instruction through GPGX fetch/begin/finish callbacks, including extension
words, DBF taken/not-taken continuation, MOVEM dynamic timing and RTS
prefetch state. Paired 600-frame evidence matches the frozen canonical
checkpoint aggregate and video with exact execution accounting. Full evidence
is in `reports/NATIVE_ROUTINE_MISMATCH_M11_52.md`.

# M11.51 — First portable native routine reconstruction
STATUS: `PORTABLE_NATIVE_ROUTINE_SHADOW_PROVEN_REPLACEMENT_BLOCKED`.

The selected structural routine is `0x002D66..0x002D84`, entered at `0x2D66`
and exited by `RTS` at `0x2D82`, with continuation `0x2D84`. Exact decoding
contains ten instructions, three basic blocks and one local direct `DBF`
back-edge from `0x2D7A` to `0x2D78`; there are no calls, indirect edges,
unresolved control flow or unsupported instructions. The structural name is
`TableCopyRoutine`; no gameplay role is assigned.

The proven entry contract captures A6 as a bounded ROM/RAM source, an even
caller stack A7 with a four-byte return address, D7/A3 as the MOVEM-preserved
registers and full SR including CCR.X. The first two source bytes are an
unsigned destination offset and DBF count. The routine consumes
`2 + 2*(count+1)` source bytes, writes `count+1` sequential words at
`0xFF134C + sign_extend_word(offset)`, advances A6, restores D7/A3, consumes
the return address with RTS, and changes only N/Z/V/C from the final word while
preserving the rest of SR. The exact ordered save/output writes and 12-byte
stack window were independently checked against the M11.29 natural shadow.
No VDP, Z80, I/O, indirect call, self-modifying or other hardware access is
reachable in this closed slice.

`oasis_core` now implements this contract through opaque adapter tokens and a
portable register/memory machine. It is structured routine-level C++, not a
PC/opcode interpreter; ROM mapping and GPGX timing remain outside core. The
synthetic differential oracle covers zero and multi-word terminal cases,
CCR.X/N/Z behavior, exact ordered writes, interruption before DBF resume,
address arithmetic and invalid entry rejection. The CFG contract test and the
unchanged 600-frame shadow proof pass with zero divergence: one natural call,
34 represented guest instructions, 13 word iterations, zero yields and zero
resumptions in the natural call.

The isolated authoritative native candidate also represents 34 instructions
and closes its accounting at 6,488,773 (`6,488,739` interpreter plus `34`
native routine). Video remains the frozen
`5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`, but the
checkpoint aggregate is
`ae8887f5b32a4973a8243775612d68b891b588f6f5dc693558a5a8a2489e5403`, not the
frozen M11.50
`251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d`.
Therefore the routine is shadow-proven but not authoritative; the generated /
interpreter path remains the oracle and fallback. Nearby candidates remain
classified `HARDWARE_BLOCKED` (`0x6121A`), `SEMANTICS_BLOCKED` (broad graphics
slices), or timing/bus-contract blocked (`0x604BC`, `0x61032`); none is
implemented here.

# M11.50 — Portable mechanical primitive layer extraction
STATUS: `PORTABLE_MECHANICAL_PRIMITIVE_LAYER_PROVEN`.

M11.50 changes ownership only; it does not discover or promote a new ROM
routine. The four M11.49 contracts remain the exact evidence-backed family:
two byte copies and byte/word clears followed by DBF. `oasis_core` now stores
only generic operation/width/register roles, opaque body/loop/continuation
tokens, a portable machine interface and explicit continuation state. The
core executor preserves the previously proven read-before-write ordering,
postincrement and 32-bit wrap, DBF low-word decrement with upper-word
preservation, CCR N/Z/V/C with X preservation, instruction-boundary yields,
resume validation and odd-word fail-closed behavior.

ROM PCs, canonical opcode/displacement validation, GPGX/BasicBlock timing and
prefetch, detached shadow snapshots, hardware-address accounting, registry
provenance and report metrics remain in `tools/hybrid`. A source scan rejects
hybrid/GPGX/libretro names and all four ROM PCs/opcode encodings from
`src/core`; CMake keeps `oasis_core` free of link dependencies. The standalone
core test uses synthetic tokens unrelated to Beyond Oasis addresses.

Full ownership and validation evidence is in
`reports/PORTABLE_MECHANICAL_PRIMITIVE_LAYER_M11_50.md`.

# M11.49 — Mechanical primitive family closure
STATUS: `NATIVE_MECHANICAL_PRIMITIVE_FAMILY_PROVEN`.

The M11.48 resumable body/DBF contract generalizes to four exact ROM forms.
Canonical bytes are `12DA` at `0x003A0C` and `0x00389E`, `4258` at `0x0003F0`
and `421D` at `0x061266`; each is followed by `DBF` with displacement `FFFC`.
The exact loop continuations are `0x003A12`, `0x0038A4`, `0x0003F6` and
`0x06126C`. The copy forms use source A2, destination A1 and counters D2/D0;
the clear forms use A0/A5 and D0. Dynamic native counts are respectively
7,613, 7,124, 4,565 and 1,890 body iterations, with equal DBF counts.

The shared primitive proof establishes source-before-destination byte-copy
ordering, postincrement timing, DBF low-word decrement/upper-word preservation,
32-bit address wrap, exact CCR N/Z/V/C behavior with X preservation, and the
single width-2 clear bus operation. The copy candidates had no hardware-visible
access in the native proof and the full ordered shadow matched; no hardware
contract was broadened. Odd word addresses are outside
the reachable proven contract and fail closed. Synthetic vectors cover overlap,
wrap, interruption, word width and unsupported forms. All four loops are now
`PROMOTED_FAMILY_MEMBER`; the M11.47 isolated `0x00026A`, `0x06193C` and
`0x061954` forms remain generated-oracle-only because no mechanical loop
contract was proven. Full evidence is in
`reports/MECHANICAL_PRIMITIVE_FAMILY_M11_49.md`.

# M11.48 — First proven native mechanical primitive replacement
STATUS: `FIRST_NATIVE_MECHANICAL_PRIMITIVE_PROVEN`.

M11.47 proved four mechanical loop shapes but left all higher-level replacement
unpromoted. M11.48 selected the safest complete contract:
`0x061266: 421D / CLR.B (A5)+`, followed by `0x061268: 51C8 FFFC /
DBF D0,0x061266`, with continuation `0x06126C`. Setup evidence at `0x061260`
loads `D0` with `0x761`, yielding 1,890 body iterations and 1,890 DBF
executions. Runtime writes are byte stores to main RAM
`0x00FF001A..0x00FF077B`, stride one, with no reads, overlap or hardware
accesses observed.

The replacement contract is exact and resumable: CLR writes zero, increments
the 32-bit address register with wrap and sets N/Z/V/C while preserving X;
DBF decrements only the low counter word, preserves the upper word, and either
branches to `0x061266` or continues at `0x06126C`. Timing, refresh, canonical
prefetch and boundary reasons flow through the existing generic machine
interface. The generated/basic-block path remains the oracle/fallback. The
primitive's own detached comparator checks registers, IR, timing, boundary
state and ordered writes; concurrent generated shadow checks the complete
decoded IR/prefetch state.

Synthetic mid-iteration event and interrupt vectors prove that the operation
does not complete atomically. The unchanged 600-frame shadow compares 3,780
primitive guest instructions with zero divergence and records 86 yields and
86 resumptions. Native proof preserves the M11.47 checkpoint/video identity,
full CPU equivalence, 150,135 yields and 288 resumptions with zero fallback or
hardware-visible accesses.

The other M11.47 structures remain unpromoted: copy loops at `0x003A0C` and
`0x00389E` still need complete source/destination/control contracts; word clear
at `0x0003F0` still needs the selected word-bus/alignment/wrap contract. No
gameplay role is assigned to any primitive.

# M11.47 — Safe-memory semantic closure and primitive discovery
STATUS: `SAFE_MEMORY_SEMANTIC_CLOSURE_PROVEN`.

The M11.46 runtime-address ledger identified a bounded safe-memory tranche.
Exact ROM forms `MOVE.L D0,-(A6)` at `0x00026A`, byte postincrement copy at
`0x003A0C` and `0x00389E`, `CLR.W (A0)+` at `0x0003F0`, `BTST.B #0,0(A5)` at
`0x06193C`, `MOVE.B #$FF,(A4)+` at `0x061954` and `CLR.B (A5)+` at `0x061266`
were independently verified and mechanically generated. Their dynamic count
is exactly 42,047. Generated bodies preserve canonical ROM bytes and exact
decoder provenance; shared helpers contain the semantic implementation and
unsupported forms remain fail-closed.

The pinned GPGX evidence for `MOVE.L D0,-(A6)` is two 16-bit writes, high word
at the decremented address plus two followed by low word at the decremented
address. Register arithmetic wraps at 32 bits while the observed bus address
is 24-bit. This is an exact form-specific fact, not a family-wide claim.

Natural adjacent loop evidence supports mechanical `MEMORY_COPY` candidates
`[0x003A0C,0x003A0E)` + `DBF 0x003A0E` and
`[0x00389E,0x0038A0)` + `DBF 0x0038A0`, with byte stride one and observed
main-RAM source/destination regions. It also supports `MEMORY_CLEAR` candidates
at `0x0003F0` + `DBF 0x0003F2` (word stride two) and `0x061266` + `DBF
0x061268` (byte stride one). These loops are not atomic and no gameplay role
is assigned. The isolated store/test forms and `0x00026A` lack a complete
higher-level contract and remain `INSUFFICIENT_EVIDENCE` for replacement.

The unchanged native proof translated 6,241,765 of 6,488,773 guest
instructions, leaving 247,008 interpreter executions. The exhaustive ledger in
`reports/REMAINING_INTERPRETER_ATTRIBUTION_M11_47.md` closes every remaining
executed PC exactly; unresolved register-address rows remain
`UNKNOWN_WITH_EVIDENCE`, hardware rows remain blocked, and no hardware
behavior was inferred.

# M11.46 — Runtime address provenance and memory-class resolution
STATUS: `BOUNDED_RUNTIME_ADDRESS_PROVENANCE_PROVEN`.

The M11.45 baseline was reproduced twice after adding a developer-only
observer. Three independent 600-frame provenance runs matched the authoritative
checkpoint/video identity and all M11.45 metrics. The observer uses the existing
GPGX `HOOK_M68K_E`, `HOOK_M68K_R/W` and `HOOK_M68K_POST` events. It records each
fallback PC count, effective bus address, width, direction, ordered bus
sequence, and A0–A7 before/after transition. Immediate instruction reads are
outside the top-level R/W callbacks, so no fetch heuristic is used.

The M11.45 final ledger has 635 `UNKNOWN_WITH_EVIDENCE` register-address PCs
with 149,678 executions. All 635 PCs were observed with exact matching counts.
Runtime memory classes were proven for 147,847 executions: 112,490 safe-memory,
14,085 hardware-only, and 21,272 mixed. Eighteen address-computation-only rows
remain unresolved exact evidence for 1,831 executions; they have no data-bus
event for this observation path. No semantic or hardware promotion follows
from address knowledge. Full evidence is in
`reports/RUNTIME_ADDRESS_PROVENANCE_M11_46.md`.

## M11.45 — Bounded semantic closure and final interpreter Pareto
STATUS: `REMAINING_ATTRIBUTION_AND_DYNAMIC_COVERAGE_95_PROVEN`.

The M11.44 baseline was reproduced twice with the canonical ROM, pinned GPGX
DLL and authoritative checkpoint aggregate. Ranking the complete final ledger
selected 551 decoder-owned exact single-instruction ranges with a dynamic upper
bound of 271,913 executions, above the 236,530 threshold. The selection excluded
register-based/other memory, hardware-visible, indirect-CFG, decoder and unknown
runtime-address classes.

Independent deterministic vectors verified only the exact forms needed by the
selected rows: BTST immediate absolute-long/data-register, MOVEQ, MOVE.W,
ADD/SUB.W, ADDQ.W, SUBQ.B/W, ANDI.B/W, OR.W, ADDI.B, SUBI.B, BCLR.L, MOVEA.L,
plus existing proven forms. The mechanical generator emits only decoder-owned
canonical ROM bytes and exact helper calls; unsupported forms fail closed. The
existing boundary shadow vetoed LSR.W at 0x0038E0 (+14 cycles), ROR.W at
0x06115A (+112 cycles) and CMPI.B at 0x0038AA (X flag divergence). These
rejections remain interpreter fallback and are not hidden by the coverage gate.

The promoted set passed 6,199,718/6,199,718 per-instruction shadow comparisons
with zero divergence. The unchanged 600-frame native run preserved checkpoint
`251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d`, video
`5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`, CPU/RAM/
VDP/sound and event/interrupt continuation identity. Final metrics are
6,488,773 total, 6,199,718 translated, 289,055 interpreter, 580 registered
ranges, 149,059 yields, 288 interrupted resumptions, zero fallback entries,
zero hardware-visible accesses and 95.5453% translated share. The complete
final ledger closes exactly at 289,055 in
`reports/REMAINING_INTERPRETER_ATTRIBUTION_M11_45.md`; `0x060BA4` remains
hardware-boundary blocked. The historical 0x03A7AE rejection is preserved but
remains obsolete after the repaired bridge/boundary/canonicalization contract.

## M11.44 — Remaining interpreter attribution and bounded promotion
STATUS: `REMAINING_INTERPRETER_ATTRIBUTION_PROVEN_SEMANTICS_BLOCKED`.

The M11.43 authoritative identity restart was reproduced twice from
`f71c92eecc128ef3eb2ffac835ec9f9dfd6bde94`: 6,488,773 total guest
instructions, 5,826,857 translated, 661,916 interpreter, 28 registered ranges,
140,065 boundary yields, 274 interrupted resumptions, zero original starts
inside translated ranges, checkpoint aggregate
`251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d`, and video
hash `5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`.

`interpreter_ledger` uses the existing profile and exact slice decoder to emit
one row per executed PC. The pre-promotion profile closes exactly at 661,916;
the final post-promotion profile closes exactly at 560,968. Each row retains
opcode bytes, exact decoded form when available, decoder range, bounded static
predecessor/successor evidence, semantic/generator state, memory class,
hardware visibility, history and a terminal blocker. Register-based unresolved
addresses are explicitly `UNKNOWN_WITH_EVIDENCE`; no generic `other` bucket is
used. See `docs/reports/REMAINING_INTERPRETER_ATTRIBUTION_M11_44.md` and
`docs/reports/REMAINING_INTERPRETER_ATTRIBUTION_FINAL_M11_44.md`.

The required historical rejection at 0x03A7AE is still preserved, but is
obsolete under the current bridge/boundary/canonicalization contract. Its
generator output for `TST.W ($00FF1654).L` followed by `BNE.W` passed 100,948
per-instruction shadow comparisons with zero divergence and the unchanged
native scenario. The final run has 29 ranges, 5,927,805 translated
instructions, 560,968 interpreter instructions and 91.3548% translated share.
The 0x060BA4 `0xA00003` access remains fallback because the existing shared bus
contract does not prove its ordered hardware behavior. No other candidate was
promoted.

## M11.43 — Complete GPGX checkpoint canonicalization contract
STATUS: `CHECKPOINT_CANONICALIZATION_COMPLETED_NEW_AUTHORITATIVE_IDENTITY`.

The pinned GPGX v1.7.6 wholesale state contract is recorded in
`reports/CHECKPOINT_CANONICALIZATION_M11_43.md`. Machine-checked x64 models
prove `FM_SLOT=80`, `FM_CH=400`, `YM2612=3576` and `Z80_Regs=88`, with the
pinned-DLL serialized YM base at `140652` and Z80 base at `144504`. The adapter
clears only 55 host-pointer spans, one function-pointer span and 111 ABI-padding
spans. Five current proofs and raw replay from current, exact M11.39 and M11.41
evidence agree on new aggregate `251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d`.
The older `c9236218...` aggregate is preserved as superseded historical output
because its adapter erased semantic bytes. No interpreter attribution or 95%
coverage work was started.

## M11.42 — Restart gate blocked by incomplete checkpoint canonicalization
STATUS: `M11.42_BASELINE_BLOCKED_CHECKPOINT_CANONICALIZATION_INCOMPLETE`.

From committed baseline `5c19e22`, two unchanged 600-frame
`BASIC_BLOCK_NATIVE` runs reproduced the M11.39 execution metrics and video but
not the M11.41 authoritative checkpoint aggregate. The expected
`c9236218f55fb18f7f1d5095e4970b25f228de588bd03bd7cbbffccc2e225fd1` was
`d5de401ceb64da875219d3ca2564160b954d55a217f4bb47ff0dc204c547ae36`.

Opt-in raw evidence compared one fresh run to preserved M11.41 evidence. The
first raw difference is frame 60, offset `140654`, in the first
`FM_SLOT.DT` host pointer. Applying the committed M11.41 canonicalization still
leaves offset `140734`, the first byte of the next serialized `FM_SLOT.DT`
pointer. The M11.41 assumed YM2612 offset/slot stride therefore leaves host
representation in the authoritative hash. No guest semantic or interpreter
execution attribution is inferred. PHASE 2 and later gates are invalid until
this identity defect is repaired and re-proven.

See `docs/reports/REMAINING_INTERPRETER_ATTRIBUTION_M11_42.md`.

## M11.41 — Checkpoint identity provenance and reproduction repair
STATUS: `CHECKPOINT_IDENTITY_SERIALIZATION_BUG_PROVEN`; restart gate
`CHECKPOINT_BASELINE_IDENTITY_RESTORED`.

The developer-only checkpoint pipeline in `src/tools/hybrid/runner.cpp` takes
full `retro_serialize()` buffers at frames 60 through 600 and hashes the
lowercase per-record state hashes in ordinal order. The raw state evidence
adapter is `src/tools/hybrid/checkpoint_evidence.cpp`; it records the complete
buffers only under the ignored `OASIS_CHECKPOINT_EVIDENCE` opt-in.

The external GPGX v1.7.6 serializer saves `YM2612` and `Z80_Regs` by raw
`sizeof` copies. `FM_SLOT.DT`, `FM_CH` connection fields, and Z80 `daisy` /
`irq_callback` are host pointers; ABI padding is also part of those wholesale
copies. The first independently observed difference was checkpoint ordinal 0,
frame 60, state offset `140654`, byte 3 of the first `FM_SLOT.DT` pointer
(state offset `140651` is the YM2612 context start). This representation byte
is not gameplay-visible state and explains why matching ROM, DLL, video and
instruction metrics did not imply matching raw checkpoint hashes.

`checkpoint_identity_hash()` now copies the recognized `STATE_SIZE=0xfd000`
buffer and clears only the proven pointer/padding spans. It rejects another
state size, leaves the raw buffer unchanged, and retains all semantic bytes in
the identity. Three current and two exact historical-checkout repaired runs
produce authoritative aggregate
`c9236218f55fb18f7f1d5095e4970b25f228de588bd03bd7cbbffccc2e225fd1` with the
same video and M11.39 execution metrics. M11.40 PHASE 2 was not performed.

## M11.40 — Remaining interpreter attribution and 95% coverage gate
STATUS: `M11.40_BASELINE_BLOCKED_CHECKPOINT_IDENTITY_MISMATCH`. The milestone
stopped before attribution, semantic expansion, generation, shadow or native
promotion because the reproduced checkpoint aggregate did not match the
historical M11.39 identity.

The current run used the same canonical USA ROM SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`, external
GPGX DLL SHA-256
`140c00fc7475cf22ca22415d65dfc7ffc66523de128454f9994e7ab77d9826fd`, 28-range
registry and 600-frame neutral scenario. It reproduced `6,488,773` total,
`5,826,857` translated and `661,916` interpreter instruction executions,
89.7991% translated share, the M11.39 video hash, 140,065 boundary yields and
274 interrupted resumptions. Two independent current runs agree on checkpoint
aggregate `fffe59fcdbed7fdac8ef22badb4f7236b8619459fed27c9931e0f93906549052`,
but M11.39 recorded
`20217e10565c51b571db6a4e474aa40854ea6c22277ba14c46ea97c4b1c60a04`. The
cause of the historical/current difference is unknown with present evidence;
no interpreter PC was promoted and no complete M11.40 ledger is claimed.
See `docs/reports/REMAINING_INTERPRETER_ATTRIBUTION_M11_40.md`.

## M11.39 — Hot-path multi-block coverage expansion
STATUS: `HOT_PATH_DYNAMIC_COVERAGE_80_PROVEN` for two new developer-only
decoder-owned ranges. This is dynamic instruction coverage, not ROM-byte or
whole-ROM coverage.

The unchanged canonical USA 600-frame run used ROM SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`, external
GPGX source commit `d60d079934977aa6973e220d123533387159f66e` and DLL SHA-256
`140c00fc7475cf22ca22415d65dfc7ffc66523de128454f9994e7ab77d9826fd`.
The M11.38 registry left 2,366,711 interpreter executions. The bounded profile
ranked 2,152 remaining PCs; it did not discover indefinitely or invent
function/indirect-target ownership.

The selected range `[0x000380,0x0003A0)` contains exactly sixteen contiguous
`ADD.W (A0)+,D0` instructions and has direct fallthrough to the preserved
`DBF D2,[0x000380]` range at `0x0003A0`. The new exact semantic form was
independently checked with four edge vectors covering word carry/overflow,
N/V/Z/C/X, post-increment and address wrap. The selected range contributed
1,572,608 translated dynamic instructions across 122,886 entries.

The selected range `[0x03A864,0x03A868)` is exactly one `BNE.W -> 0x03A85E`
instruction. Its Bcc semantic form was already independently verified. A
broader attempted window was rejected by the generator as an incomplete
decoder range; no guessed neighboring instruction was included. It contributed
132,187 translated dynamic instructions.

Both generated bodies are provenance-bound to the canonical ROM and use the
generic M11.38 per-instruction yield/resume contract. Full shadow completed
5,826,857/5,826,857 comparisons with zero divergence; native completed 600/600
frames with exact checkpoint/video hashes, zero starts inside translated ranges,
zero hardware-visible accesses and 274 interrupted continuations. The remaining
661,916 interpreter executions and conservative blocker Pareto are recorded in
`docs/reports/HOT_PATH_MULTI_BLOCK_COVERAGE_M11_39.md`.

## M11.38 — Interrupt-safe multi-instruction block execution
STATUS: `INTERRUPT_SAFE_MULTI_INSTRUCTION_BLOCKS_PROVEN` for exactly the four
M11.37 rejected developer-only ranges. The original M11.37 rejection remains
in `docs/reports/CONTROLLED_DYNAMIC_COVERAGE_M11_37.md`.

| range | exact instructions | M11.37 natural count | resolved boundary |
| --- | --- | ---: | --- |
| `[0x0032EE,0x0032F6)` | `TST.W ($00FF1658).L`; `BNE.S -> 0x0032EE` | 1,121,997 | after `0x0032EE`, before `0x0032F4` |
| `[0x03A9AC,0x03A9B4)` | `TST.W ($00FFAFAE).L`; `BNE.S -> 0x03A9BC` | 248,291 | after `0x03A9AC`, before `0x03A9B2` |
| `[0x03A9B4,0x03A9BC)` | `TST.B ($00FF0BFD).L`; `BNE.S -> 0x03A9CA` | 248,290 | after `0x03A9B4`, before `0x03A9BA` |
| `[0x03A9CA,0x03A9D4)` | `TST.W ($00FF1654).L`; `BNE.W -> 0x03A9AC` (`FFDA`) | 248,290 | after `0x03A9CA`, before `0x03A9D0` |

M11.37 recorded the interrupt interleaving at range level; each frozen range
has one and only one internal instruction boundary, so the exact conservative
yield ordinal is uniquely the boundary shown above. TST absolute-long and Bcc
short/word are already independently verified semantic forms. The mechanical
generator emits resumable bodies and the registry contains no candidate-specific
timing or interrupt logic. The four-candidate shadow gate recorded
4,122,062/4,122,062 per-boundary comparisons with zero divergence; native
execution recorded 274 exact continuations after actual GPGX interrupt service.
See `docs/reports/INTERRUPT_SAFE_MULTI_INSTRUCTION_BLOCKS_M11_38.md` for the
identity-bound run evidence and per-candidate counts.

## M11.37 — Controlled dynamic coverage expansion
STATUS: `CONTROLLED_DYNAMIC_COVERAGE_EXPANSION_PROVEN` for the bounded
developer-only registry; this is not ROM-byte coverage or a whole-ROM claim.

The unchanged cold-reset neutral 600-frame scenario was run against canonical
USA ROM SHA-256 `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`,
external GPGX source commit
`d60d079934977aa6973e220d123533387159f66e` and DLL SHA-256
`9b345293c239805cbfe22bb3c582e7d42164a701ba2c50e1934e1a8ef80b2ec8`.
The EMULATED baseline recorded 6,488,773 interpreter instruction executions,
2,188 unique interpreter PCs, 600 video frames, checkpoint hash
`b8e1e07908d75e9c8b21f3ed661352a7005c51dcf3120e2502cc0f73788a4bd4` and video
sequence hash `5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`.
The six-entry historical registry is the before basic-block-entry baseline;
the promoted registry contains 22 entries, of which 16 are new.

The bounded candidate queue considered 61 generator-eligible natural entry
PCs. Each row is recorded in
`docs/reports/CONTROLLED_DYNAMIC_COVERAGE_M11_37.md` with observed count,
decoded form/control flow and gate status. All promoted forms were already
covered by the independent M11.34/M11.35 semantic harness. Four multi-
instruction candidates were not promoted after shadow exposed interrupt
interleaving inside their ranges; `0x060BA4` was rejected because its TST.B
touches hardware-visible `0xA00003`; the historical `0x03A7AE` mismatch remains
unpromoted. No candidate-specific timing correction or second CPU model was
introduced.

The final shadow run completed `388308/388308` comparisons with zero divergence
and full CPU equivalence. Native promotion completed the same 600 frames and
translated `388314` guest instructions out of `6488773` (`5.9844%`), leaving
`6100459` interpreter executions and 12 fallback entries. There were zero
original starts inside translated blocks and zero hardware-visible accesses.
Native checkpoint and video hashes exactly matched the EMULATED baseline.
Generated bodies and metadata are mechanical output; handwritten registry
glue is separate. The production target remains free of ROM/assets, GPGX,
emulator binaries and generated run evidence.

## M11.36 — GPGX timing / refresh bridge contract
STATUS: `GPGX_TIMING_REFRESH_BRIDGE_PROVEN` and
`DEMAND_DRIVEN_BLOCK_PROMOTION_PROVEN` for the existing six-entry developer
registry only.

The M11.35 first mismatch was an incompatible observer epoch, not an
instruction timing disagreement. In external GPGX source commit
`d60d079934977aa6973e220d123533387159f66e`, `m68k.cycles` and
`m68k.refresh_cycles` are accumulated master-cycle counters for the current
frame. `system.c` subtracts `mcycles_vdp` from both at frame end. The old
entry/next-entry comparison consequently observed
`actual_cycles=74 actual_refresh=228` after a rebase while the same candidate's
pre-rebase expected state was `896114/896268`; the difference is exactly
`mcycles_vdp=896040`.

The corrected bridge compares at a generic GPGX post-instruction boundary,
after opcode fetch, semantic execution and `USE_CYCLES`/refresh advancement,
before the next scheduler/frame transition. Interrupt polling remains owned by
GPGX: `m68k_run` checks at entry and no registered block crossed an observed
interrupt or hardware-visible access. This is an absolute-state comparison;
no entry-baseline subtraction or candidate-specific delta is used.

Bounded A–G evidence for `0x3A85E` (`TST.W ($00FF1654).L`, exit `0x3A864`)
found exact PC, IR, prefetch, SR, RAM read and timing state through the
post-instruction point. The old observer was the first point to change epoch.
The final shadow gate then completed `185975/185975` with zero divergence for
the preserved M11.33 blocks and only the existing three M11.35 candidates.
Native promotion of that same set completed 600 frames with exact checkpoint
and video hashes, `185975` promoted entries, zero original-body starts, zero
interrupt crossings and zero hardware accesses. The full identity and evidence
are in `docs/reports/GPGX_TIMING_REFRESH_BRIDGE_M11_36.md`.

## M11.35 — Demand-driven block promotion pilot
STATUS: `DEMAND_DRIVEN_PROMOTION_RUNTIME_BLOCKED`. This is developer-only
natural-execution evidence, not a whole-ROM coverage claim.

The cold-reset neutral 600-frame GPGX trace used the same external identity as
the M11.33/M11.34 regression: instrumented source commit
`d60d079934977aa6973e220d123533387159f66e`, DLL SHA-256
`ba6c10fdb1fe421e1e38c88b70ce18b0d2a122f4472f477b8a1a7aba14fce274`, and the
local canonical USA ROM. `DISCOVER_BLOCKS` recorded 2,188 unique guest PCs.

The bounded final shortlist and exact decoded forms are:

| entry | bytes/form | exit | classification | gate |
| --- | --- | --- | --- | --- |
| `0x3A85E` | `4A79 00FF 1654` — `TST.W ($00FF1654).L` | `0x3A864` | `NEW_VERIFICATION_REQUIRED` | shadow blocked, first divergence |
| `0x3A8BA` | `4A79 00FF 1654` — `TST.W ($00FF1654).L` | `0x3A8C0` | `NEW_VERIFICATION_REQUIRED` | not reached after first blocker |
| `0x3A88C` | `4A39 00FF 0BFD` — `TST.B ($00FF0BFD).L` | `0x3A892` | `NEW_VERIFICATION_REQUIRED` | not reached after first blocker |

The independent semantic harness passed the new TST edge vectors and the
generator emitted provenance-bound bodies only for exact supported forms. Bcc
and DBF candidates were explored as pilot alternatives but did not satisfy the
runtime gate because of timing/interrupt-boundary divergence. `0x3A7AE` was
also rejected after an IR/prefetch mismatch (`actual 0x4E73`, `expected 0x4A79`).
Candidates involving indirect control flow or unsupported exact IR were
rejected without widening the slice.

The final shadow stopped at the first selected entry with:
`FIRST_DIVERGENCE block=0x3a85e timing actual_cycles=74 expected_cycles=896114
actual_refresh=228 expected_refresh=896268`.
No M11.35 candidate is promoted; failed candidates remain interpreter fallback.
This result does not establish native equivalence, full CPU equivalence, or any
new production architecture. Detailed evidence is in
`docs/reports/DEMAND_DRIVEN_BLOCK_PROMOTION_M11_35.md`.

## M11.34 — Independent M68K semantic core verification
STATUS: `M68K_SEMANTIC_CORE_INDEPENDENTLY_VERIFIED` for the exact used subset;
all other exact-IR forms remain `UNVERIFIED`.

The frozen M11.33 mechanical surface contains exactly these seven used
combinations: `MOVEM.L <register-list>,-(A7)`, `CLR.W Dn`, `MOVE.B (A6)+,Dn`,
`LEA.L (abs.L),A3`, `ADDA.W D7,A3`, `MOVE.W (A6)+,(A3)+`, and `ADD.L D1,D2`.
There are no supported-but-unused emitter combinations at this checkpoint;
all other exact-IR forms fail closed in the generator.

The independent semantic reference is the Motorola/NXP 68000 Family
Programmer's Reference Manual:
https://www.nxp.com/docs/en/reference-manual/M68000PRM.pdf
The test-only reference model is separate from the decoder and emitter and is
not a production dependency. Twenty-three vectors independently established
result values, D/A effects, PC and length, SR/CCR, effective-address ordering,
memory width/order, MOVEM mask ordering and boundary behavior. No semantic
disagreement remains in the used subset.

Decode/provenance checks independently assert opcode identity, exact operation
and operands, instruction size, extension-word count, effective-address values,
next-PC continuity and generated guest-PC/opcode comments for all three blocks.
The unchanged M11.33 external GPGX proof remains the separate timing/state gate.

## M11.32 — Basic-block recompilation timing proof
STATUS: `BASIC_BLOCK_RECOMP_TIMING_PROVEN`.

The existing deterministic scenario naturally entered `0x2D66` once,
`0x604BC` four times and `0x61032` nine times. Exact block contracts include
all registers, PC/SR, prefetch state, cycles/refresh and ordered bounded
memory effects. Shadow comparisons passed 14/14 with zero divergence. Native
execution reused GPGX's own fetch/bus/timing helpers and skipped the original
body for all 14 calls; serialized checkpoints and video matched EMULATED.
This is migration evidence only. It does not promote a production emulator,
general recompiler, or a claim about other ROM routines. Full identities and
the per-block instruction contracts are in
`docs/reports/BASIC_BLOCK_RECOMPILATION_TIMING_M11_32.md`.

## M11.31 — Comparative method transfer
STATUS: `PARTIAL_TRANSFERABILITY`.

Public Streets of Rage references were inspected at pinned revisions and remain
external methodology evidence only. `gsaurus/sor-disassemblies` is an IDA
database/generated-assembly reference (`SPECIALIZED_ONLY`), while
`gsaurus/sor_pancakes` is a graphics/data hacking toolkit (`SPECIALIZED_ONLY`).
The RuiNelson `RageDecompiler`/recompilation workflow is the strongest process
reference (`PARTIAL_TRANSFERABILITY`): preserve ROM addresses, maintain an
explicit code/data/unknown map, record unresolved indirect entries, iterate
from natural evidence, and separate generated code from hand-written helpers.
It does not establish exact ROM reassembly, shared programmer style, shared
binary modules or Beyond Oasis sound-driver lineage.

The bounded dry-run on the existing `0x2D66` target followed that sequence and
reproduced the M11.29 evidence without importing external labels, RAM formats,
audio assumptions or runtime code. This is a workflow confirmation, not a new
ROM fact or a reason to expand RE tooling. Full source/commit inventory and the
separate conclusions are in
`docs/reports/COMPARATIVE_DISASSEMBLY_METHOD_TRANSFER_M11_31.md`.

## M11.30 — Small batch candidates
STATUS: `SHADOW_PROVEN_OVERRIDE_BLOCKED`.

`0x604BC..0x604E6` is a ten-instruction direct-RTS leaf. It sets two bounded
RAM flag bytes, emits three Scc bytes through `A5+5` and one absolute RAM byte,
then returns with `A0`, `A6`, `A7`, PC and the Z flag mechanically determined.
It has four natural calls in the 600-frame neutral scenario, no observed
interrupt and no hardware access. Shadow comparisons are 4/4 clean.

`0x61032..0x610C8` is a fully decoded short direct-RTS RAM/table transform. It
updates bounded fields relative to `A6`, performs one bounded indirect ROM/RAM
read and has nine natural calls, no observed interrupt and no hardware access.
Shadow comparisons are 9/9 clean with full SR checks. Its exact write list and
register contract are implemented in the developer-only candidate adapter.

Both candidates pass shadow in the natural batch with `0x2D66`. Native-side
register/RAM effects and body skipping work, but serialized VDP/sound state
diverges because the skipped instructions' GPGX bus-refresh and hardware phase
contract is not available through the minimal boundary. Override promotion is
blocked; no timing engine or production dependency is implied.

## M11.29 — Minimal safe native override target
STATUS: `HYBRID_NATIVE_OVERRIDE_MINIMAL_PROVEN`.

The existing deterministic 600-frame neutral PC bitmap contains `0x2D66` and
its direct static caller `0x2D58` is decoded. The exact range is
`[0x2D66,0x2D84)`, 10 instructions ending in `RTS`, with no nested or indirect
control flow. The body saves/restores `D7/A3`, consumes two bytes through `A6`,
uses `D7` as a bounded `DBF` count, and writes words sequentially through
`A3 = 0xFF134C + sign_extended(first_byte)`. It does not access
VDP/Z80/I/O or its own code; the observed neutral call reads its source from
canonical ROM through `A6` and writes only the fixed RAM destination and saved
register stack footprint. `DBF` does
not alter CCR. The final `MOVE.W` derives N/Z/V/C and X is preserved, so the
full SR result is mechanically bounded. This is the selected target for the
bounded M11.29 shadow/override proof.

The executed `0x6121A` leaf was rejected because its `MOVE.B` writes target
`0xC00011` in the VDP range. `0x62CC` and `0xA8DA` were rejected because the
same deterministic 600-frame bitmap has no target PC for either routine.

The target-specific shadow comparison observed one natural call with zero
divergences, including full SR, exact MOVEM stack write order, output bytes,
register deltas and RTS return. Native override then made one call, skipped all
original target-body instruction starts, and matched the EMULATED 600-frame
checkpoint and video sequences. No interrupt was observed while inside the
routine. Evidence and identities are recorded in
`docs/reports/HYBRID_NATIVE_OVERRIDE_MINIMAL_POC.md`.

## M11.28 — Natural hybrid shadow of `0x3820`
STATUS: `HYBRID_SHADOW_PROVEN_OVERRIDE_BLOCKED`.

Six distinct natural startup calls cover both known formats. Three clean
shadow runs compare existing native/mechanical output with GPGX, including
source consumption, D/A register effects, stack saves/restores, RTS return and
SR mask `0xFFEF`. Final X is explicitly unmodeled. GPGX's predecrement MOVEM
bus order is low word then high word. Six external interrupts during calls
preserve the suspended routine registers/CCR and do not touch its required
footprint. Their timing/device effects stay outside the native contract.

All 600 video frames and 10 whole-emulator state checkpoints match EMULATED.
No override or body skip is claimed. The original executes authoritatively;
instruction timing, interrupt scheduling, X and prefetch/IR return behavior
must be established before replacement. Exact source/build/artifact hashes,
captured bounds, assumptions, tests and excluded preliminary captures are in
`reports/HYBRID_NATIVE_MIGRATION_POC.md`. Manual ID3 hunting is stopped.

## M11.24 — Bounded resource contract `0x02CFAA -> 0xD3B2`
STATUS: `RESOURCE_CONTRACT_ID3_VERIFIED`.

The natural executed-PC evidence reaches `0x02CFAA`, `0xD3B2` and `0x3820`.
At `0x02CFA2`, the caller selects resource ID `3`; `0x02CFAA` calls the
indexed loader. The exact table entry `0x05CEA2` contains `0x001AE1A8`, and
the existing native decompressor consumes `0x702` bytes through exclusive
end `0x1AE8AA`. The next table entry independently contains that end pointer.

The loader decodes to `0xFF2FA8..0xFF3FA8` (`0x1000` bytes). The native output
SHA-256 is
`36bbea13cb564ab194dd438b1fe75076525525068b57f2f02a4c0142630dc277`.
An independent mechanical translation of the original `0x3820` algorithm
produces the same consumed length, output length and bytes. Immediately after
the call, `0xD3B2` queues a bounded DMA descriptor: source word address
`0x7F97D4` (`0xFF2FA8 >> 1`), caller-supplied destination `D1=0x4000`, and
`0x800` words. This is a neutral transfer fact, not a semantic resource name.

No adjacent routine or resource meaning was promoted. The complete bounded
contract, provenance and remaining semantic unknown are recorded in
`docs/reports/RESOURCE_CONTRACT_ID3.md`.

## M12-AUTO14 — CC-B0 group pointer table
STATUS: `GROUP_POINTER_TABLE_04371E_CONFIRMED`.

The exact consumer at `0x00CCB0` loads `0x04371E`, shifts the high-byte
selector by two address bits, and reads a 32-entry longword pointer table
through `[0x04371E,0x04379E)`. All 32 canonical values are even ROM targets.
Only the table bytes are source-owned; nested target subtables and records are
not promoted. The developer-only transaction preserves the canonical ROM
identity and records the table contract in its promotion report.

## M11.22 — Bounded static classification for `0x060BB6-0x060BC4`
STATUS: `BOUNDED_REGION_060BB6_STATIC_SUPPORTED`.

The exact bounded unit was re-verified against the canonical ROM and retained
M11.19 runtime evidence: 8/8 instruction starts are byte-exact, decoded and
observed. The exact bounded decoder and structural explorer corroborate the
four required local edges, including the fallthrough entry at `0x060BAE`, the
loop-back at `0x060BC2`, the alternate entry at `0x060BAA`, and the immediate
successor call at `0x060BCC`.

Only the inclusive unit `0x060BB6-0x060BC4` changed from its prior unknown
state to `CODE_STATIC_SUPPORTED`. Its eight address facts remain
`CODE_EXECUTED_AT_ADDRESS`. `boundary_status` is
`LIKELY_INTERNAL_BLOCK`; routine identity, whole-routine promotion and
adjacent-range changes are explicitly absent. Provenance and deterministic
regression results are in
`docs/reports/RUNTIME_REGION_060BB6_CLASSIFICATION.md` and
`build/m11-22-gpgx-bounded-classification.json`.

## M11.21 — Bounded runtime region `0x060BB6-0x060BC4`
STATUS: `RUNTIME_REGION_060BB6_STRUCTURALLY_UNDERSTOOD`.

The retained M11.19 manual-realtime bitmap contains all eight target
instruction-start PCs. Exact ROM decoding gives a coherent local loop:
`0x060BAE` falls through to `0x060BB6`, the six-NOP sequence reaches
`0x060BC2`, `0x060BC2` branches to `0x060B90`, and `0x060BAA` can enter at
`0x060BC4`. `0x060BC4` falls through to `0x060BCC`, whose direct call targets
the existing candidate-map `CONFIRMED` leaf `0x0604BC`. The surrounding
`0x060B8C -> 0x06121A` call is also exact and its target is globally observed.

The bounded fragment is assessed as `LIKELY_INTERNAL_BLOCK`; no whole routine
boundary or semantic name is claimed. The evidence is retained as eight
`CODE_EXECUTED_AT_ADDRESS` facts only. It does not promote the full range to
`CODE_STATIC_SUPPORTED`, and no trust classification changed. See
`docs/reports/RUNTIME_REGION_060BB6.md` for the listing, edge provenance,
rank-1 comparison and unknowns.

## M11.19 — GPGX executed-PC evidence import
STATUS: `GPGX_RUNTIME_EXECUTION_EVIDENCE_HIGH_VALUE`.

The developer-only `oasis_re_import_gpgx_coverage` importer accepted the
validated manual-realtime GPGX capture for the canonical USA ROM
(`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`). The
persistent global bitmap contributed 14,732 unique even instruction-start
addresses; the latest session contributed 1,447 new addresses. The importer
stored 14,732 address-level `CODE_EXECUTED_AT_ADDRESS` facts, retained 94
`DECODE_UNSUPPORTED` results, and reported 12,698 executed addresses without
an existing static classification as `RUNTIME_EXECUTED_UNKNOWN`.

Canonical identity, bitmap size and capture hashes were verified. No
runtime/data conflicts were found. The anchors `0x3820`, `0x62CC`, `0x9BF2`,
`0xD3B2` and `0x6121A` were observed; `0xA8DA` was not. No range-level trust
classification changed and no function boundary or semantic claim is made.
The deterministic artifact and human report are
`build/gpgx_runtime_execution_evidence.json` and
`docs/reports/GPGX_RUNTIME_EXECUTION_TRUST.md`.

## M11.20 — Runtime-executed unknown prioritization
STATUS: `RUNTIME_UNKNOWN_PRIORITIZATION_HIGH_VALUE`.

The M11.19 address evidence was grouped into 9,012 neutral
`RUNTIME_EXECUTED_REGION`s containing 12,698 PCs. The deterministic shortlist
does not treat contiguity as a function boundary and does not change any
classification. It identified 525 regions with runtime evidence plus strong
existing static/candidate corroboration; the highest overall region is
`0x000374..0x0003A0`, while the highest-ranked corroborated region is
`0x060BB6..0x060BC4`.

Top-five bounded slices use the exact decoder's instruction records and only
local direct control flow. Unsupported, indirect, conflict and external
boundaries are retained as stops. The report checks `0x62CC`, `0x9BF2` and
`0xD3B2` independently; they are already `CODE_STATIC_SUPPORTED` and are not
selected by hardcoded priority. The observed systemic gap is Ghidra overlap
without trusted/static corroboration, not a demonstrated decoder defect.
No semantic names, mass classifier repair, runtime capture or trust promotion
was performed. See `docs/reports/RUNTIME_EXECUTED_UNKNOWN_PRIORITY.md`.

## M11.17 — Structured data classification
STATUS: `STRUCTURED_DATA_HIGH_VALUE`.

The bounded classifier accepted nine `DATA_STRUCTURE_SUPPORTED` ranges (the
vector table, two terrain tables, two pointer tables and four 26-byte screen
descriptors) plus the weaker fixed ROM header `DATA_REGION_SUPPORTED`. Every
accepted range has an exact ROM slice hash, width, deterministic count/end and
at least one consumer or parser reference. Pointer targets were validated
against the canonical ROM. The `0x5CE96` table's null entry is preserved, but
compressed payload boundaries remain unresolved and are not classified.

The classifier intersects every candidate with the M11.15 code ranges and
emits explicit conflict records; this pass found none. The future promotion
gate rejects only explicit trusted-data overlap and leaves weak/unknown data
hypotheses non-blocking. See `docs/STRUCTURED_DATA_CLASSIFICATION.md` and the
ignored local JSON report for per-range hashes and parsed elements.

## M11.16 — Targeted dynamic code confirmation
STATUS: `TARGETED_DYNAMIC_REACHABILITY_LIMITED`.

Five critical ranges were selected before inspection: `0x3820`, `0x62CC`,
`0x9BF2`, `0xA8DA` and `0xD3B2`. The retained natural corpus contains no
accepted target-hit artifact for these ranges, so no classification changed.
The prior M11.8 prose count of 13 hits for `0x3820` has no retained JSON
artifact and is not promoted to dynamic evidence. The existing `0x6121A`
positive control remains valid: two matching natural reports use the canonical
ROM hash and record two hits at frame 113, with exact range linkage and
`DYNAMIC_NATURAL` classification. No new emulator run or scenario was added.

Per-target blockers are `EVIDENCE_ARTIFACT_MISSING` for `0x3820`,
`CALLER_NOT_REACHED` for `0x62CC`, `SCENARIO_COVERAGE` for `0x9BF2` and
`0xA8DA`, and `CONTROL_FLOW_GAP` for `0xD3B2`. Dynamic trust remains local;
forced evidence cannot promote and caller/callee trust is not propagated.
The exact full-ROM split and the M11.15 trust counts remain unchanged. See
`docs/TARGETED_DYNAMIC_CONFIRMATION.md` for the bounded selection and hashes.

## M11.15 — Evidence integrity audit and classification trust repair
STATUS: `EVIDENCE_TRUST_NEEDS_FIXUPS` for the post-M11.14 reconstructed layout.
The 203 exact ASM ranges were individually reassembled against the canonical
ROM and linked to candidate/Ghidra entry and range evidence. Exact bytes alone
now map to `ASM_ROUNDTRIP_EXACT`; static support requires an independent anchor,
an exact incoming edge from an already trusted caller, vector/startup evidence,
or dynamic execution. Dynamic execution and behavior are never inferred from
round-trip success.

The audit result is 197 `ASM_ROUNDTRIP_EXACT`, 5 `CODE_STATIC_SUPPORTED`, 1
`CODE_EXECUTED` (`0x6121A`) and 0 `BEHAVIOR_VERIFIED`. Every record retains ROM
identity, source artifact SHA-256, entry/range consistency, incoming xref source
addresses, caller trust, beta/dynamic fields, data conflicts, boundary status
and unresolved concerns. One known provenance mismatch remains: Ghidra ends
`0x3820` at `0x38D0`, while the independently confirmed range ends at `0x3B3E`.
This bounded issue is recorded rather than hidden. The full-ROM split remains
byte-perfect; no new promotion was performed.

## M11.14 — Automated promotion large batch II
STATUS: AUTO_PROMOTION_BATCH2_HIGH_VALUE for bounded byte reproduction only.
The post-M11.13 manifest and prior report were used; 89 new candidates and 2
systemic-fix retries were attempted, with 73 accepted. ASM coverage rose from
6,462 to 13,550 bytes and reject windows stayed stable. Remaining blockers are
clustered by form in the batch report; no semantic ownership was inferred.

The byte-immediate emitter preserves noncanonical extension words but leaves
forms vasm cannot encode rejected. The repeated cmp.w mismatch is retained as
ASM_ENCODING pending a safe general encoding rule. No address-specific or raw
opcode override was introduced.

## M11.13 — Automated promotion scale pass
STATUS: AUTO_PROMOTION_SCALE_HIGH_VALUE for bounded byte reproduction only.
The post-M11.12 manifest was the input; 100 deterministic candidates were
attempted and 84 accepted after exact slice/full-ROM checks. ASM coverage rose
from 2,632 to 6,462 bytes. Rejections were 14 unsupported exact-IR forms and
2 slice mismatches; no data ownership or semantic names were inferred.

The shared emitter now prints immediate ori/andi/eori to CCR with .b, the syntax
accepted by vasm for the 68000 CCR encoding. This is a general operand-kind
rule with a regression test, not an address-specific override.

## M11.12 — Automated blob-to-source promotion PoC
STATUS: `AUTO_PROMOTION_HIGH_VALUE` for bounded byte reproduction only. The
promotion tool does not assign semantic names or classify unknown data.

`re_auto_promote.py` consumes the existing candidate-map/mass-verification
evidence and the M11.11 manifest. Deterministic ranking selected 25 of 210
eligible candidates without a new hardcoded address list. Twenty-one passed
the existing decoder/ASM emitter, vasm `-m68000 -no-opt -Fbin`, exact slice
comparison and transactional full-ROM comparison. Four were retained as
UNKNOWN after bounded unsupported-form or assembler blockers. No handwritten
opcode overrides were used.

The final manifest has 46 `CODE_VERIFIED` and 42 `UNKNOWN` entries, zero gaps or
overlaps, ASM coverage 2,632 bytes (0.0836690267%) and local-ROM blobs 3,143,096
bytes (99.9163309733%). The rebuilt 3,145,728-byte ROM matches canonical
CRC32 `C4728225`, SHA-1 `2944910c07c02eace98c17d78d07bef7859d386a` and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
Rejected trials are rolled back; semantic ownership remains UNKNOWN. See
`docs/AUTO_BLOB_PROMOTION.md` for the accepted/rejected table and reproduction.

## M11.11 — Full-ROM split reassembly baseline
STATUS: `FULL_ROM_SPLIT_EXACT` for byte reproduction only. This baseline does not
classify unknown bytes or assign semantic names.

The trusted code set is exactly the 25 M11.10 ranges, including the M11.9
controls. Every other canonical-USA byte is emitted as an `UNKNOWN` blob sourced
at build time from the hash-verified local ROM. The deterministic manifest spans
`[0x000000,0x300000)` with 50 entries, 25 code and 25 blob ranges, zero gaps and
zero overlaps. No `DATA_KNOWN` or `CONFLICT` range was inferred.

The full layout reassembles to 3,145,728 bytes and matches the canonical ROM
byte-for-byte. Hashes are CRC32 `C4728225`, SHA-1
`2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
ASM coverage is 1,846 bytes (0.0586827596%); 3,143,882 bytes (99.9413172404%)
remain explicit local-ROM blobs. The M11.9 controls, M11.10 corpus, expanded
split and legacy split are rerun before the full comparison. See
`docs/FULL_ROM_SPLIT_BASELINE.md` for commands and metrics.

## M11.10 — Diverse reassembly coverage expansion
STATUS: `DIVERSE_REASSEMBLY_HIGH_VALUE` for the bounded encoding experiment;
runtime ownership and gameplay meaning remain UNKNOWN.

The 25 selected ranges below come from the frozen mass/explorer candidate set.
Each has a confirmed local ROM boundary, contiguous decoder coverage and an
exact ASM/vasm round trip in the M11.10 runner. Structural labels are retained
from the candidate evidence; they are not semantic names.

| Range | Evidence label | Instructions/bytes | Test status |
| --- | --- | ---: | --- |
| `0x07C4..0x07E2` | MODERATE_STATIC | 9/30 | MATCH |
| `0x08A2..0x08B6` | MODERATE_STATIC | 6/20 | MATCH |
| `0x0D5E..0x0D82` | MODERATE_STATIC | 7/36 | MATCH |
| `0x0E80..0x0EC2` | MODERATE_STATIC | 17/66 | MATCH |
| `0x0F32..0x0F7E` | MODERATE_STATIC | 23/76 | MATCH |
| `0x1108..0x1112` | MODERATE_STATIC | 4/10 | MATCH |
| `0x12E8..0x1300` | MODERATE_STATIC | 8/24 | MATCH |
| `0x2B6E..0x2B8A` | MODERATE_STATIC | 4/28 | MATCH |
| `0x2B8A..0x2BA0` | MODERATE_STATIC | 3/22 | MATCH |
| `0x2D66..0x2D84` | MODERATE_STATIC | 10/30 | MATCH |
| `0x3820..0x3B3E` | VERIFIED_BOUNDED_CODE | 306/798 | MATCH |
| `0x4A92..0x4AD0` | MODERATE_STATIC | 13/62 | MATCH |
| `0x62CC..0x62E4` | STRONG_STATIC | 6/24 | MATCH |
| `0x64C4..0x6516` | MODERATE_STATIC | 21/82 | MATCH |
| `0x8504..0x8530` | MODERATE_STATIC | 10/44 | MATCH |
| `0x85C4..0x85E2` | MODERATE_STATIC | 6/30 | MATCH |
| `0x8CAC..0x8CD0` | MODERATE_STATIC | 9/36 | MATCH |
| `0x94A2..0x94D2` | STRONG_STATIC | 20/48 | MATCH |
| `0x99B8..0x99D6` | MODERATE_STATIC | 11/30 | MATCH |
| `0x9BF2..0x9C40` | MODERATE_STATIC | 32/78 | MATCH |
| `0xA8DA..0xA8F0` | MODERATE_STATIC; multiple_entry_overlap | 10/22 | MATCH |
| `0xB730..0xB79A` | MODERATE_STATIC | 34/106 | MATCH |
| `0xC90E..0xC92C` | MODERATE_STATIC | 11/30 | MATCH |
| `0xCECC..0xCEEA` | MODERATE_STATIC | 6/30 | MATCH |
| `0xD3B2..0xD406` | STRONG_STATIC | 18/84 | MATCH |

The corpus totals 606 instructions and 1,846 bytes. Its practical inventory is
105 distinct operation/width/source-kind/destination-kind forms, all 105 exact
in this bounded sample. The expanded split and the legacy M11.9 mixed split
`[0x1108,0xA8F0)` both match. Mismatches encountered while broadening support
were decoder metadata, IR operand metadata, ASM branch-expression formatting,
and assembler optimization; all were repaired systemically. No routine-specific
encoding patch or raw `dc.w` was used. See `REASSEMBLY_POC.md` for reproduction
and the machine-readable form inventory.

## M11.9 — Reassemblable disassembly pipeline
CONFIRMED: canonical local ROM -> existing decoder typed operands -> deterministic
ASM -> vasm gives five exact slices: [0x3820,0x3B3E), [0x62CC,0x62E4),
[0xA8DA,0xA8F0), [0x1108,0x1112), [0x2B6E,0x2B8A). Respectively these contain
306/6/10/4/4 instructions and 798/24/22/10/28 bytes: 330 instructions, 882 bytes.
No handwritten opcode overrides or code-byte directives were needed.

0x1108 and 0x2B6E were selected from the existing clean LEAF mass evidence:
MODERATE_STATIC, BOUNDARY_AGREES, no failure reasons, indirect flow or known
data/overlap conflict. Current bounded decoding confirms contiguous ranges and
RTS terminals. The former has indirect/displacement reads and a shift; the
latter has absolute-long writes and long immediates. Their semantic meanings
and runtime evidence remain UNKNOWN. A8DA's old mass `multiple_entry_overlap`
flag is retained; exact selected-byte agreement does not resolve global ownership.
Its four `(A5)+` destinations, including `MOVE.W D2,(A5)+`, are preserved exactly.
No new runtime evidence is claimed for A8DA/62CC. 0x3820 retains its established
decompressor meaning and prior vector/runtime evidence.

At 0x389C, opcode 0x95C1 is SUBA.L D1,A2. The existing decoder's overly broad
SUBX no-extension mask omitted its EA metadata. Excluding size-code 3 from that
mask restores decoder-owned operands. Address-arithmetic long EA width and
MOVEM word EA width are now retained; synthetic tests cover these distinctions.
ASM and future emitters can consume the same typed operands without another EA
decoder. Supported family recognition alone does not imply exact-emitter support;
unhandled forms are rejected rather than output as raw opcode patches.

The local split covers only [0x1108,0xA8F0): 38,888 bytes, including four UNKNOWN
gaps totaling 38,006 bytes extracted at build time from the canonical ROM.
It compares exactly. All generated ASM/IR and commercial blobs remain ignored.
vasm 1.8g with `-m68000 -no-opt -Fbin` is output-compatible for this experiment;
without `-no-opt`, 0x3820 shrinks to 794 bytes. Ancient's assembler is UNKNOWN.
See `REASSEMBLY_POC.md` for provenance, assumptions, per-range tests and the
first-difference example. Result: `REASSEMBLABLE_DISASM_POC_HIGH_VALUE`.
Next recommendation: A, expand to 25–50 verified routines; not implemented.

## M11.6.2 — Static translation trust repair
Status: `STATIC_TRANSLATION_TRUST_RESTORED` for the bounded developer-only
PoC; this is not a general 68000 translation claim.

Canonical-USA ROM slice decoding confirms the complete A8DA sequence:
`0x0C45 0x0050` (`CMPI.W #$50,D5`), `0x640E` (`BCC.S 0xA8EE`), `0x5245`
(`ADDQ.W #1,D5`), `0x3AC2` (`MOVE.W D2,(A5)+`), `0x3005`
(`MOVE.W D5,D0`), `0xD044` (`ADD.W D4,D0`), `0x3AC0`
(`MOVE.W D0,(A5)+`), `0x3AC3` (`MOVE.W D3,(A5)+`), `0x3AC1`
(`MOVE.W D1,(A5)+`) and `0x4E75` (`RTS`). The fall-through effect is four
ordered word writes at the incoming A5, A5+2, A5+4 and A5+6, followed by an
A5 delta of 8; D5 receives only the word ADDQ result and D0 receives the word
ADD result while their upper halves remain unchanged. The BCC early path is
the two-instruction compare/branch path when CCR.C is clear.

The six-instruction 0x62CC slice is confirmed as `MOVEQ #0,D0`, two
`MOVE.L D0,(d16,A6)` writes at offsets `0x4E` and `0x52`, two
`MOVE.W #0,(d16,A6)` writes at `0x2A` and `0x04`, then `RTS`. MOVE/MOVEQ
clear N/Z/V/C according to their width and preserve X; the used ADD/ADDQ
forms set X together with carry. These are the only CCR semantics added.

The former M11.6 Case B evidence is explicitly invalidated: its C++ routine
treated the memory operands as register moves, and its fixture compared no
meaningful memory writes while expecting that same incorrect model. The new
test derives expected addresses, widths, values, register effects and CCR
from the decoded instruction list without calling `mechanical_A8DA` as an
oracle. Runtime capture remains unavailable under the separate M11.6.1/
M11.7/M11.8 evidence boundary.

## M11.8 — Natural reachability recovery for `0x62CC`
Status: `ROOT_CAUSE_ADVANCED`; no natural target or direct caller was recovered.

The M11.8 scenario is a new hardware-reset, natural-input experiment, not a
forced checkpoint. Its 25 one-frame controller events cover title/start,
movement, attack/use and interaction/room hypotheses. The developer-only
BizHawk probe now records the input schedule, every frame-boundary PC, selected
RAM bytes and exact target-hit counts. It never writes PC, registers, CCR, RAM,
ROM or savestate data. Per-target register/stack snapshots are bounded to the
first eight hits so a high-frequency address cannot make the diagnostic itself
the dominant workload.

The full canonical-USA run executed 1800/1800 frames and watched 43 targets:
the 33 static incoming encodings, the player/event owner candidates, `0x3820`
and `0x60004`. It also sampled 21 RAM bytes. The only target hits were the
positive control `0x3820` (13) and startup/control entry `0x60004` (5). Every
one of the 33 incoming PCs and `0x62CC` itself had zero hits. The frame samples
were live and stateful, with transitions through `0x32EE/0x32F4` and
`0x3A8E8/0x3A8EE`; this narrows the current blocker to the natural transition
from startup/system scheduling into the player/event owner band. It does not
prove that `0x62CC` is globally unreachable, and frame-boundary PCs are not
substitutes for exact branch outcomes.

All 33 incoming sites were ranked from their bounded local slices as follows.
The ranking is a natural-reachability priority, not a recovered semantic
function name:

| Rank | Sites | Static reason and next natural hypothesis |
| --- | --- | --- |
| 1 | `0x5850` | Direct `BSR.W 0x62CC` in the bounded `0x557A` player-owner path; highest-value gameplay entry. |
| 2 | `0x61FE`, `0x62EC` | `BCC.W` edges after `BSR.W 0x85E2`; test after a real player-state update and capture CCR.C. |
| 3 | `0x7AC2`, `0x7B60` | Direct event/entity-side transfer; `0x7B60` follows the `0x7B2A`/`0x7B3C` event gate. |
| 4 | `0x635A`, `0x646E`, `0x673E`, `0x748A`, `0x78DE`, `0x7A16` | Conditional edges with a nontrivial handler/cleanup body; plausible state-handler alternatives but no dynamic owner. |
| 5 | `0x6C0E` | Direct `BSR.W 0x62CC` followed by a handler body; natural only after its unknown producer is reached. |
| 6 | `0x5CDC`, `0x5F6C`, `0x5F8E`, `0x784E`, `0x796C`, `0x7CC2` | Unconditional transfers, but their enclosing callers are not recovered; useful as exact hooks after an owner appears. |
| 7 | `0x5F36`, `0x6620`, `0x66A4`, `0x6732`, `0x6C40`, `0x6E64`, `0x6FDE`, `0x7054`, `0x7060`, `0x725C`, `0x72D0`, `0x73D2`, `0x757E`, `0x760C`, `0x7924` | Short conditional/return or cleanup sites; lowest first-search priority until their containing routine is naturally observed. |

The next experiment is prepared, not forced: sweep button hold/edge timing
around the observed startup/transition helpers `0x6135E`,
`0x32EE` and `0x3A8E8`, retain exact hooks for the ranked sites, and preserve
the input/frame/RAM report. This is the concrete advancement beyond M11.7's
generic `CALLER_NOT_REACHED`.

## M11.7 — Single-target reachability root cause for `0x62CC`
Status: `CALLER_NOT_REACHED` under the two existing natural scenarios.

The target is a six-instruction leaf `[0x62CC,0x62E4)` with no calls,
indirect flow or unsupported instruction:
`MOVEQ #0,D0; MOVE.L D0,0x4E(A6); MOVE.L D0,0x52(A6); MOVE.W #0,0x2A(A6);`
`MOVE.W #0,0x04(A6); RTS`. Bounded static slices confirm the local player
paths `0x61FE -> 0x62CC` and `0x62EC -> 0x62CC`, the `0x5850 -> 0x62CC`
call, and the event path `0x7B60 -> 0x62CC`. A complete even-address ROM
branch-reference scan found 33 direct branch/call encodings to `0x62CC`:
`0x5850`, `0x5CDC`, `0x5F36`, `0x5F6C`, `0x5F8E`, `0x61FE`, `0x62EC`,
`0x635A`, `0x646E`, `0x6620`, `0x66A4`, `0x6732`, `0x673E`, `0x6C0E`,
`0x6C40`, `0x6E64`, `0x6FDE`, `0x7054`, `0x7060`, `0x725C`, `0x72D0`,
`0x73D2`, `0x748A`, `0x757E`, `0x760C`, `0x784E`, `0x78DE`, `0x7924`,
`0x796C`, `0x7A16`, `0x7AC2`, `0x7B60` and `0x7CC2`. This is target-local
incoming evidence, not a claim that all 33 addresses have recovered semantic
function boundaries.

The cheap boot-path check confirms a direct main-loop call `0x8B2E -> 0x557A`,
but the bounded natural observations did not execute `0x8B22`, `0x8B2E`,
`0x557A`, `0x59B8`, `0x61F6`, `0x62E4`, `0x7B2A`, any of the 33 incoming
addresses, or `0x62CC`. Neutral hardware reset ran 300 frames; the existing
`120:Start` run ran 1800 frames. Both had zero hits for every incoming address
and for the target. The nearest observed shared raw entry was `0x60004`
(2 neutral hits, 5 `120:Start` hits), reached from the already-known
`0x611EE`/`0x6121A` path; it is not a direct incoming edge to `0x62CC`.

The static conditions are therefore requirements, not observed failures:
the player branches at `0x61FE` and `0x62EC` are `BCC.W`, requiring CCR.C=0
after `0x85E2`; the event branch at `0x7B3C` is `BEQ.S`, requiring the first
`0x60004` result after `ANDI.W #$1FF,D0` to equal `0x01FF`. Neither branch PC
was reached, so no runtime registers, CCR, RAM read or taken/not-taken result
may be asserted. The event producer `0x609C6` statically constructs D0 from
RAM bit checks, but it was also not reached. No single RAM writer or callback
installation is proven as the root cause.

Minimum natural requirement: reach at least one target-owned incoming routine;
for the player route this means naturally entering `0x557A`, selecting
`0x61F6` or `0x62E4` through `0x59B8`, and returning CCR.C=0 from `0x85E2`;
for the event route it means reaching `0x7B2A` and satisfying `0x7B3C`.
Next recommendation: A — build one minimal natural scenario that causes the
missing state. No such scenario is implemented in this checkpoint.

## M11.6.1 — Runtime capture fixup for static translation
Status: existing-scenario runtime capture unavailable; the static B/C fixtures
remain static evidence only.

The existing hardware-reset scenario and canonical USA ROM were reused. The
developer-only natural probe gained `OASIS_TARGET_ADDRESSES`, an observation
override for the exact selected targets `0xA8DA,0x62CC`; it does not write
emulator state or alter inputs. The neutral hardware-reset run was bounded at
300 frames and the existing `120:Start` run at 1800 frames. Both reports have
zero hits for both targets, `target_reached=false` and no entry snapshot.

Both B and C therefore have result
`RUNTIME_CAPTURE_UNAVAILABLE_EXISTING_SCENARIOS`. There are no accepted
register/CCR/memory/PC/return captures, no runtime replay comparisons and no
natural invocation count to report. The three decision predicates in the task
do not include the case where both routines are unavailable: `PARTIAL` requires
at least one confirmed B/C capture. No translation failure is inferred. The
only next action is recommendation B, a separately authorized small bounded
set of natural gameplay scenarios; no emulator, interpreter, production
runtime or M12 work is authorized here.

## M11.6 — Verified static translation PoC
Status: VERIFIED as a bounded developer-only experiment. No production runtime
or CPU model changed, and no ROM/trace/savestate entered Git.

Case A is the confirmed decompressor `[0x3820,0x3B3E)`, 306 decoded
instructions. A fixed compiled C++ mechanical output reproduced the existing
native decompressor for the two local USA-ROM vectors: `0x16943C` consumed
1217 bytes and produced 3072 bytes; `0x1894EA` consumed 112 bytes and produced
128 bytes. The existing original-ROM output hashes remain the oracle:
`65e99e...` and `167d4e...` as recorded in the M3 ledger.

Case B is `0xA8DA`, a current mass/explorer clean leaf with 10 instructions,
no nested call, indirect flow, device address or unsupported instruction. Its
bounded path is `CMPI.W #$50,D5; BCC; ADDQ.W; MOVE.W`/`ADD.W` register
operations through `RTS`. Two normalized static-state fixtures (fall-through
and early branch) matched register/CCR expectations. No BizHawk runtime
capture for this leaf is claimed.

Case C is `0x62CC`, a current mass/explorer clean leaf with 6 instructions.
It writes four bounded offsets `0x04`, `0x2A`, `0x4E` and `0x52` from `A6`,
after `MOVEQ #0,D0`, and returns without unknown control flow. One normalized
RAM-state fixture matched ordered write records and final memory. No BizHawk
runtime capture for this leaf is claimed, and the fixture does not claim a
gameplay meaning for the routine.

The PoC state model contains only D0-D7, A0-A7, CCR and a bounded big-endian
memory span. It has no fetch/decode loop. Unsupported opcode input produces an
explicit STOP result. First divergence is reported by register, CCR, ordered
write record or memory byte. This is evidence for a verification aid, not a
general recompiler or an authorization to translate additional routines.

## M11.5 — Ant reachability diagnostic PoC v1
Status: VERIFIED as a bounded developer-only reachability experiment. The
canonical USA ROM was checked with BizHawk 2.11.1 on the migrated environment.
The fresh bounded explorer contained 35 unresolved `INDIRECT_FLOW` frontiers.
After excluding the three previously accepted dynamic sources, ten contexts
were selected deterministically: `0x0790`, `0x5328`, `0x59B8`, `0x85F8`,
`0xA322`, `0xA332`, `0xA680`, `0xA690` and two separate owners of `0xA7E2`.

Existing scenario inventory used for reachability: hardware-reset neutral
`natural_reset_idle_v1` / `natural_idle_to_6121a_v1` with a 300-frame bound;
hardware-reset `120:Start` with an 1800-frame bound; and the existing boot-trace
scenario, which was inspected but does not provide frontier-PC coverage. The
watch-only diagnostic mode changed target observation only; it did not invent
inputs or alter RAM, registers, flags, ROM or checkpoints.

The reachability matrix had 18 cells (two scenarios across ten contexts, with
the duplicate `0xA7E2` watch deduplicated). Every cell was `NOT_REACHED` and no
first-frame/register snapshot exists for the sample. Both historical negative
controls, `0x0790` and `0x5328`, remained unreached. Static cross-checks found
valid explored code, matching ROM bytes, plausible owning entries and matching
indirect forms for all ten contexts; no `STATIC_FRONTIER_SUSPECT` was assigned.

The environment positive control was a fresh natural ant job for the known
frontier `0x045A`: source reached at frame 113, sequence 1734712, and the
natural indirect target was `0x307A` after 104103 ms. This distinguishes
scenario coverage failure from a BizHawk/ROM/PC-hook regression. No sampled
frontier had a matched scenario, so no sampled target-resolution retest or
dynamic merge was performed. The correct result is
`SCENARIO_COVERAGE_INSUFFICIENT`, not proof of global unreachability.

Cost comparison is bounded and approximate: the previous blind queue cost
about 104 seconds per ant frontier, while two shared reachability runs covered
the ten contexts in about 65 seconds total. This avoids an estimated 16+ minutes
of blind `NOT_REACHED` work for this sample. The next authorized recommendation
is to create a small bounded set of new natural gameplay scenarios; stop before
resuming the blind queue, adding workers, or beginning M12.

## M11.5 — Single-worker sequential ant queue PoC v1
Status: VERIFIED as a bounded developer-only queue experiment. One frozen queue
of five existing explorer `INDIRECT_FLOW` frontiers was processed strictly
sequentially with one BizHawk 2.11.1 process at a time. No production runtime,
CPU emulator, scheduler, parallel worker or forced state was added.

Selection: the fresh USA explorer baseline contained 35 indirect-flow frontiers.
The neutral 1800-frame reachability report observed `0x045A`, `0x61F60` and
`0x62878`; the deterministic policy ranked those first by first frame and then
source identity, and filled the bounded queue with stable-address fallbacks
`0x0790` and `0x5328`. The queue schema is
`oasis.m68k.re-ant-queue.v1`; each item references an existing
`oasis.m68k.re-ant-job.v1` and the queue is frozen at creation.

Queue identity: `queue-0x4C23AB2632531710`, initial JSON SHA-256
`DC3FC952C6B7682E7F8E28F0180F4C1E3AC9A5D2B55187F92B1E55AE71D97EA6`.
Lifecycle is `AVAILABLE -> CLAIMED -> RESOLVED`, or a bounded failure through
`FAILED_RETRYABLE`/`FAILED_FINAL`; only one item can be claimed. Tests cover
stale-claim recovery, resolved-duplicate suppression and frozen-queue refusal.

Runs A/B used the same frozen queue and restarted the single worker between
jobs. Results were identical after normalization, including target, frame,
instruction bytes and observed D/A/SR registers. The normalized result-set
SHA-256 was
`3B38333E9688208096CDA5D92178CFA0F01FBD32A15514E3A2AA9B9FE2657BFE`.
Three jobs resolved naturally: `0x045A -> 0x307A` at frame 113,
`0x61F60 -> 0x6211A` at frame 424 and `0x62878 -> 0x62900` at frame 424.
The two fallback jobs were `NOT_REACHED` and finalized `FAILED_FINAL`; no
target was invented. A worker-instance poll showed at most one BizHawk process.

Merge: the five results were batch-validated once; only the three
`DYNAMIC_NATURAL` observations were accepted and the explorer was rerun once.
Instruction bytes changed `60916 -> 61506`, decoded instructions
`19623 -> 19765`, discovered entries `504 -> 508`, entries processed
`537 -> 541`, unresolved indirects `35 -> 32`, and frontiers `148 -> 145`.
Dynamic edges increased `0 -> 3`, retaining source entry/PC, frontier ID, job
ID, result hash, backend and scenario provenance. The fallback failures remain
valid bounded reachability evidence, not proof of semantic absence.

Performance: worker execution totaled 520627 ms in A and 517110 ms in B;
three accepted edges correspond to approximately 0.346 and 0.348 edges/minute.
The two fallback jobs consumed the bounded neutral budget and were not retried.
Queue metrics were `selected=5`, `claimed=5`, `attempted=5`, `resolved=3`,
`not_reached=2`, `timeout=0`, `failed_retryable=0`, `failed_final=2`,
`nondeterministic=0`, `duplicate_jobs_avoided=0`, and `unique_dynamic_edges=3`.
No multi-target PC was observed. The initial queue did not contain an already
resolved duplicate; the suppression rule is nevertheless exercised by the
CI-safe queue test.
Checkpoint policy: stop here. Any parallelism, scheduler, larger queue,
savestate farm, random input or M12 work requires a separately authorized task.

## M11.5 — Single-ant closed-loop PoC v1
Status: VERIFIED as one developer-only natural evidence loop. No gameplay
meaning is assigned to the observed target, and no production runtime code or
CPU emulator was added.

Selection: the current ROM-wide explorer exposed 35 `INDIRECT_FLOW` frontiers.
The preferred `0xA7E2` was not reached by the existing natural reset/Start
scenarios, so it was rejected. A bounded neutral hardware-reset selection probe
reached `source_entry=0x020E`, `source_pc=0x045A`, bytes `4E 91` (`JSR (A1)+`)
at frame 114. Its stable frontier identity is
`size=3145728;fnv1a64=EA6BB7880F4BB247:0x0000020E:0x0000045A:INDIRECT_FLOW:address_indirect:computed target is unresolved`.

Job: `ant-0x43919998981C2FF`, schema `oasis.m68k.re-ant-job.v1`, canonical USA
ROM SHA-256 `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`,
BizHawk `2.11.1`, scenario `natural_reset_idle_v1`, hardware reset, neutral
input only, no checkpoint, limits 3000000 instructions/300 frames. The job
requires `NATURAL_OBSERVED`; it does not write registers or flags.

Runs A and B used the identical job. Both were `RESOLVED`: A1 and the observed
next PC/indirect target were `0x0000307A`, at frame 113 and sequence 1734712,
with result hash `0x21238399`. All normalized evidence fields matched;
wall-clock execution was about 51.5 seconds per run and was excluded from the
deterministic hash.

Merge: result schema `oasis.m68k.re-ant-result.v1` was accepted as
`DYNAMIC_NATURAL` (the result-side name for the job's `NATURAL_OBSERVED`
requirement). Wrong frontier, ROM mismatch, forced/state-guided evidence and
hash divergence are rejected by tests. The explorer edge retains
`evidence_class=DYNAMIC_NATURAL` plus the frontier ID, job ID, result hash,
backend and scenario; the job/result retain the full ROM and register
provenance.

Static rerun, using the same USA/Beta Atlas inputs as baseline, changed the
following metrics: instruction bytes `60916 -> 61396` (+480), decoded
instructions `19623 -> 19729` (+106), discovered entries `504 -> 506` (+2),
entries processed `537 -> 539` (+2), direct calls `986 -> 1000` (+14),
fallthroughs `17538 -> 17637` (+99), unresolved indirect edges `35 -> 34`
(-1), and frontiers `148 -> 147` (-1). One dynamic edge was present after the
merge, and the selected frontier was absent. This proves the closed loop and
has positive structural ROI; it does not prove the target's game semantics.

Checkpoint policy: hard stop after this one job. Do not add a scheduler,
second frontier job, swarm, savestate replay, forced-state path or M12 feature.

## M11.5 — Recursive structural exploration v1
Status: VERIFIED as developer-only structural evidence tooling. The explorer
does not execute a CPU, infer gameplay semantics, modify Atlas classification
or generate production C++.

The implementation is oasis_re_explore. It calls the existing
re_slice_decoder for instruction/operand decoding and direct target
resolution, uses re_atlas typed data intervals as traversal guardrails, and
uses re_candidate_map records as provenance input. re_program, re_cfg_audit
and re_reachable_closure were not copied; their relevant evidence models and
deterministic ordering conventions were reused. The worklist creates new
entry candidates only for direct call targets and independent seeds; internal
branch targets remain paths in the current decoded slice.

Seed tiers are deterministic: TIER_0 vectors and confirmed project/control
entries; TIER_1 Atlas static direct-call targets; TIER_2 Ghidra functions with
xrefs; TIER_3 other Ghidra functions; TIER_4 is reserved for future
heuristics. Every seed retains source, source reference, initial confidence
and priority. Existing confirmed entries remain evidence anchors; Ghidra-only
records are never promoted to CONFIRMED.

The persistent output schema is oasis.m68k.re-explore.v1. It records
instruction/data/pointer/conflict ranges, multiple owners, entry analysis
states, direct call/jump/conditional/DBcc/fallthrough edges, explicit return
and terminal stops, compact machine-readable frontiers and quantitative
coverage. A stable frontier identity combines the ROM identity, source entry,
source PC, blocker type, context and reason; it is not a random UUID.

Raw baseline: two fresh Ghidra 12.1.3 headless imports of the canonical USA
ROM (size 3145728, SHA-256
eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263) produced
identical 390972-byte raw exports, SHA-256
613A3AA6DEB8D2DCF994C82ADC6A6939B7D5F27AF67A51D05C16F090D60A5315.
The existing candidate-map/mass pipeline was rerun against the fresh export
and the approved local Beta ROM: candidate JSON SHA-256
9ACD162CE078C2D31C108BA480F0306DA75D6B8FFDFF2D008A1DAE8253263A9B,
mass JSON SHA-256
6B9A6A366C0CEC047C1616A5840C8DA59D7AC89DD75021A0240F4F8E89D69C0D.
Counts and top failure clusters matched the prior Ghidra-shaped measurement;
the prior reconstructed input itself is not present, so byte-level comparison
against it is unavailable.

Bounded corpus: 15 entries were analyzed before wide expansion. The six
current call-edge anchors were recovered: 0x60004 -> 0x6042A (direct jump
encoding), 0x60B8C -> 0x6121A, 0x60D4A -> 0x6121A,
0x611EE -> 0x6121A, 0x60BCC -> 0x604BC and
0xD3B2 -> 0x3820. RTS/RTE terminated paths. 0xA7E2 remained an explicit
indirect frontier. Known Atlas tables were guarded and not decoded as code.

ROM-wide measurement: the bounded gate passed, so one experimental run was
allowed. It processed 537 entries from 547 tiered seed records, decoded
19623 instructions and produced 986 direct-call, 592 direct-jump, 2551
conditional/DBcc, 17538 fallthrough and 35 unresolved-indirect edges.
Coverage was 60916 instruction bytes, 432 pointer-data bytes, 6 probable-data
bytes, 0 conflict bytes and 3084374 unclassified bytes (98.05%). The
unclassified percentage is not a percentage of unreversed gameplay code.
The ROM also contains graphics, audio, tables, maps, strings, padding and
other data.

Entry results were 451 analyzed, 25 BLOCKED_INDIRECT, 28
BLOCKED_UNSUPPORTED, 0 BLOCKED_DATA and 0 conflicts. There were 148
frontiers: 81 DECODE_FAILURE, 35 INDIRECT_FLOW and 32 UNSUPPORTED records
after stable identity deduplication. The full representative addresses are
emitted in JSON/text, not copied into this ledger.

The measured top frontier classes are: (1) decode/boundary failures, 81,
estimated medium effort and medium false-positive risk; (2) unresolved
indirect flow, 35, estimated high effort and high risk; (3) unsupported
decoder coverage, 32, estimated medium effort and low/medium risk. These are
prioritization hypotheses based on frontier counts, not claimed byte gains.
STATIC_BYTES_UNLOCKED_PER_FIXED_BLOCKER was not fabricated because no decoder
or boundary fix was implemented in this checkpoint.

Two ROM-wide runs using the fresh A/B exports were byte-identical:
JSON SHA-256 8CD0C9B669786C76C16FF8E276A4314B5789925371153EB181783FBE8181F8DE;
human summary SHA-256
E1E314077F300AE71AD3B6362865243CE6C917C48C18733786AD133EB5687460.
Wall-clock is intentionally CLI-only and excluded from serialized identity.

Exact next step: stop before the dynamic ant/scheduler PoC. Any next
checkpoint must consume this frontier format with one bounded emulator probe;
do not broaden the decoder, translate leaves or start M12 here.

## M11.5 — Mass structural verification pass v1
**Status:** VERIFIED as developer-only measurement tooling; structural classes
are not semantic confidence and no new routine meaning was inferred.

`oasis_re_mass_verify` consumes the normalized
`oasis.m68k.re-candidate-map.v1` records, the existing Atlas and the canonical
USA ROM. For each entry it performs a bounded entry decode, conservative
reachable-flow walk, direct-reference signal check, Ghidra-boundary comparison,
Atlas data/code overlap check and reuse of existing Beta/dynamic flags. It
records `BOUNDARY_AGREES`, `BOUNDARY_SHORTER`, `BOUNDARY_LONGER`,
`BOUNDARY_UNKNOWN` or `BOUNDARY_CONFLICT`, then assigns one structural class:
`STRONG_STATIC`, `MODERATE_STATIC`, `WEAK_STATIC`, `INDIRECT_FLOW`,
`UNSUPPORTED`, `BOUNDARY_CONFLICT`, `DATA_CONFLICT`, `DECODE_FAILURE` or
`UNKNOWN`. The pass does not run BizHawk/MAME and does not modify
`src/game/`, `src/core/` or `src/genesis/`.

The 534-entry local measurement covered the previous 483 `GHIDRA_ONLY`, 39
`STATIC_SUPPORTED`, 11 `CONFIRMED` and 1 `CONFLICT` records. Previous
`GHIDRA_ONLY` outcomes were: 262 `MODERATE_STATIC`, 176
`BOUNDARY_CONFLICT`, 17 `INDIRECT_FLOW`, 10 `UNSUPPORTED` and 18
`WEAK_STATIC`. Previous `STATIC_SUPPORTED` outcomes were: 11
`STRONG_STATIC`, 5 `BOUNDARY_CONFLICT`, 4 `DATA_CONFLICT`, 7 `UNSUPPORTED`
and 12 `WEAK_STATIC`.

Leaf analysis found 234 leaves: 208 clean, 7 with unsupported decoder
coverage, 13 with indirect flow, 6 with boundary conflict and 0 terminal
failures. Failure clusters are per-entry, non-exclusive: boundary longer than
Ghidra 185, multiple-entry overlap 82, unsupported opcode 54, unknown terminal
44, indirect flow 31, call-target-only 27, no direct xref 24, boundary shorter
9, unsupported addressing 7, decode failure 5, known data overlap 4,
boundary conflict 1 and confirmed-code overlap 1. The three highest measured
fix classes are boundary continuation heuristic (277 affected; medium/high
effort; medium risk), decoder coverage (61; medium; low risk), and static
edge recovery (51; medium; medium risk). Affected counts are aggregated from
the current report and are not a claim of semantic improvement.

All 11 confirmed control anchors were retained. Five passed the batch
heuristic and six were reported as `HEURISTIC_MISS_ON_CONFIRMED`; none was
downgraded. The control result is specifically a checker-quality signal.

The raw external Ghidra v3 export is not present in this checkout. Therefore
the local 534-entry bake-off reconstructed a Ghidra-shaped input from the
previous normalized candidate-map evidence. Its hashes prove deterministic
serialization of this measurement, not a fresh independent Ghidra export.
The canonical export remains the previously recorded
`613A3AA6DEB8D2DCF994C82ADC6A6939B7D5F27AF67A51D05C16F090D60A5315` input.
Mass JSON A/B SHA-256 is
`1F5B31C2789D62752FC6CE0F8E5977357914573116965FCA96A02DA55FA296BC`; human
report A/B SHA-256 is
`E1F9E895E245BFB6A504B1D34891D2E12B5CE2F6A04F43D848BA37E87A062BAF`.

**Exact next step:** restore the raw Ghidra export and rerun this pass before
implementing exactly one systemic fix. Do not translate leaves, add runtime
tracing, investigate `0x611EA`, or begin M12 in this checkpoint.

## M11.5 — Ghidra-to-Atlas candidate integration
**Status:** VERIFIED as developer-only prioritization tooling. Ghidra discovery
remains candidate evidence; no new entry is promoted to project truth.

The normalized model is `oasis.m68k.re-candidate-map.v1`. It consumes the
deterministic external Ghidra v3 export (schema
`oasis.m68k.ghidra-map.v1`, 496 functions and 438 candidates, export SHA-256
`613A3AA6DEB8D2DCF994C82ADC6A6939B7D5F27AF67A51D05C16F090D60A5315`) and
merges it with the existing Atlas, bounded static call/reference evidence,
the already-recorded Beta correspondence and documented BizHawk observations.
No new broad trace or whole-ROM scanner was run.

Merge behavior is conservative and deterministic. Function and candidate
records with the same entry are merged; Atlas entries and known static/dynamic
addresses are retained even when Ghidra missed them. The classification
priority is: existing confirmed project evidence -> `CONFIRMED`; code/data or
boundary disagreement -> `CONFLICT` (except a known confirmed entry retains
`CONFIRMED` with boundary conflict metadata); independent static support ->
`STATIC_SUPPORTED`; runtime-only observation -> `DYNAMIC_OBSERVED`; otherwise
`GHIDRA_ONLY`. Dynamic flags remain visible when stronger evidence selects a
different classification. A Ghidra range containing a known table start is a
code/data conflict; unknown table extents are not invented.

The score is explainable: dynamic +40, known direct target +20, known direct
caller/call-site +10, Beta exact/structural/changed +10/+8/+4, LEAF/SHALLOW/
COMPLEX +10/+6/-8, no observed indirect flow +4, exact Atlas entry +5,
boundary/code-data conflict -50 and Ghidra-only without xrefs -5. Ties sort by
ascending address. This is prioritization only, not confidence.

The real USA/Beta merge produced 534 unique entries: 11 `CONFIRMED`, 39
`STATIC_SUPPORTED`, 0 `DYNAMIC_OBSERVED`, 483 `GHIDRA_ONLY` and 1 `CONFLICT`.
Complexity counts are LEAF 234, SHALLOW 172, COMPLEX 90 and UNKNOWN 38.
The top-10 non-confirmed audit retained the existing runtime call-site facts
and identified `0x611EA` as the highest-ranked new bounded Ghidra function with
static support and no recorded conflict. This is the recommended next target,
not work started by this checkpoint.

Two fresh runs were byte-identical: normalized JSON SHA-256
`5C17F6A735DC715B18CD5A5E8FA34F876CAD5CEB5D4511C80547E2C8720A22AC` and
human report SHA-256
`9E5A5884FE05B816C0265535529E79E2C336FBB4CE176D60AF57F522DAFD9C84`.
Remaining unknowns are Ghidra boundary correctness, indirect target recovery,
unobserved callers and routine semantics.

## M11.5 — bounded downstream runtime resolution at `0x60BFA` / `0x60C08`
**Status:** VERIFIED as scenario-only runtime evidence, developer-only. Static
global unresolved status is unchanged and no semantic field names are assigned.

Canonical-USA static verification found:

| PC | bytes | instruction | mode | base | displacement | width | direction |
|---|---|---|---|---|---:|---:|---|
| `0x60BFA` | `16 28 00 01` | `MOVE.B 1(A0),D3` | `d8(A0)` | A0 | 1 | 1 | read |
| `0x60C08` | `14 28 00 01` | `MOVE.B 1(A0),D2` | `d8(A0)` | A0 | 1 | 1 | read |

The frozen `start_pulse_120` / `120:Start` hardware-reset scenario under
BizHawk 2.11.1 reaches both targets at frame 423. The earlier stack consume
produces A0=`0x0006F8AE` at `0x60BD2`, but A0 changes before these targets;
the old value is therefore not a target-base invariant.

| PC | sequence | SR | relevant D registers | A0 | A7 | effective address | class | raw byte |
|---|---:|---|---|---|---|---|---|---|
| `0x60BFA` | 891 | `0x00002704` | D2=`0x00000100`, D3=`0x00000000` | `0x0006F8B0` | `0x00FF0BAC` | `0x0006F8B1` | ROM | `0x13` |
| `0x60C08` | 892 | `0x00002704` | D2=`0x00000000`, D3=`0x00070BCD` | `0x0006F8B2` | `0x00FF0BAC` | `0x0006F8B3` | ROM | `0x00` |

Full snapshots remain in the report. D0-D7 at `0x60BFA` are
`54,6F8AE,100,0,940F0EEE,60000000,0,FFFF`; at `0x60C08` they are
`54,6F8AE,0,70BCD,940F0EEE,60000000,0,FFFF`. A0-A7 are respectively
`6F8B0,C00,FF316C,FF136C,0,FF001A,FF06F2,FF0BAC` and
`6F8B2,C00,FF316C,FF136C,0,FF001A,FF06F2,FF0BAC`.

Both targets are `runtime_resolved_for_scenario` with
`resolution_scope=scenario_only`, confidence `CONFIRMED`, and
`static_global=false`. Two fresh reports are byte-identical: JSON SHA-256
`CF092C8B91BD2FDA858E3E165A75D3A891F8B90997D6F3E839A65FD053C97D91`; human
trace SHA-256 `34717649BB6DA2C389180A994DE226715F51739036A742BDCDD6B573B7FDE0C4`.
The relevant previous unresolved count is two target rechecks; two are now
scenario-resolved and zero are globally resolved. Global invariance is not
proven. Writer callback width and semantic role remain UNKNOWN.

## M11.5 — bounded runtime stack-value provenance for the `0x60B8C` path
**Status:** VERIFIED as bounded runtime evidence, developer-only, with writer
callback width UNKNOWN. No semantic role is assigned to the observed value.
The additive report is `oasis.m68k.re-stack-runtime-provenance.v1` and runs
outside `oasis_core` against the canonical USA ROM
(`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`) in
BizHawk 2.11.1. It reuses hardware reset, `start_pulse_120` and raw input
`120:Start`; it stops after the first post-`0x60BD0` continuation.

The same controlled run reaches every requested boundary:
`0x60B8C`, `0x6121A`, `0x60B90`, `0x60BCC`, `0x604BC`, `0x604E4`, `0x60BD0`,
`0x60BFA` and `0x60C08`. At `0x60B8C` the raw A7 is
`0x00FF0BA8`. Immediately before `0x60BCC`, P=A7 is
`0x00FF0BA8` and the big-endian longword at memory[P] is
`0x0006F8AE`. USA bytes at the call site are `61 00 F8 EE`, which decode as
`BSR.W 0x604BC` with return address `0x60BD0`.

The observed stack timeline is exact for this run: caller A7=P; BSR places
return `0x60BD0` at P-4 and callee entry A7 becomes `0x00FF0BA4`; the callee
return snapshot at `0x604E4 RTS` preserves that entry A7; the next caller
instruction `0x60BD0` has A7=P and reads the same `0x0006F8AE`; the following
`0x60BD2` snapshot has A0=`0x0006F8AE` and A7=`0x00FF0BAC`. This distinguishes
the BSR return slot from the separately consumed memory[P] longword without
assigning meaning to either value.

Bounded writer capture watched only `[P,P+4)`. The USA oracle confirms
`0x60B66` bytes `2F 08` (`MOVE.L A0,-(A7)`). BizHawk bus-write callbacks
reported callback PC `0x60B68`, correlated by the exact instruction hook to
`0x60B66`, with writes at `0x00FF0BAA`=`0x0000F8AE` and
`0x00FF0BA8`=`0x00000006`. The callback API exposes no width, so both widths
remain UNKNOWN; the final four bytes are reconstructed as
`0x0006F8AE` from the ordered concrete-range writes and pre-consume memory.
No MAME probe was needed, and no global writer or value invariant is claimed.

The JSON and human-readable reports were repeated from fresh runs. Their
SHA-256 values are respectively
`EFB30EEBF3EE0CEE929A02075088D684A2900B0DAFA192BC00390E484A846D0D` and
`4239B4182758FAEA49C56524953632C7568A52C508EF4E5BFFDCE6995872F7AC`, equal
between A/B. `0x60BFA` and `0x60C08` are recorded only as reached plus raw
boundary events; their memory references are not resolved. The result is a
concrete value for this scenario, not a static constant. Debug/Release/GNU
CTest, USA oracle, file-limit and diff-check pass; GitHub Actions CI run
`33874638457` for the focused implementation commit passed.

## M11.5 — bounded natural reachability search for `0x60B8C` / `0x60D4A`
**Status:** VERIFIED for one bounded natural caller path, developer-only. This
is not a complete instruction trace, emulator, input interpretation or
semantic conclusion.

The experiment reused `src/tools/re_bizhawk_natural_scenario.txt` and the
existing BizHawk 2.11.1 Lua probe against the canonical USA ROM
(`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`). Every
variant started from `hardware_reset`; the additive report remains schema
`oasis.m68k.natural-reach.v1` and the probe remains outside production runtime.
Search family: `natural_reach_60b8c_60d4a_v1`.

The bounded search tested neutral baseline, then stopped at the first primary
hit, `start_pulse_120` with one raw event `120:Start` and max horizon 1800.
It reached `0x60B8C` at frame 423 after 424 frame advances. Watched target
counts were:

| watched address | exact count |
|---|---:|
| `0x60B8C` | 3 |
| `0x60D4A` | 0 |
| `0x6121A` | 5 |
| `0x611EE` | 2 |

The first primary caller snapshot has PC `0x60B8C`, A7 `0x00FF0BA8` and
stack window `[0x00FF0B88,0x00FF0BE8)`, exactly the bounded
`[A7-0x20,A7+0x40)` window requested for this experiment. A following
`0x6121A` watched event is paired with raw caller `0x60B8C`; its observed
longword matches the statically computed BSR return address `0x60B90`.
These are register/stack observations only; no value receives a semantic
name.

Bounded input minimization tested the same one-event pulse at frames 119 and
121; both reached `0x60B8C` at frame 423. The neutral baseline, which removes
the only event, did not reach either primary caller. The successful input is
therefore minimal by event/button count within this bounded check, while
nearby timing is observationally equivalent. The exact `start_pulse_120` run
was repeated twice: JSON SHA-256
`20AA010BAECFE696A119D431A7EE6562074848219DD9C08A16D00BE3BBD994F2` and
trace SHA-256
`66F0095A195A9899789F08D0D4E8C5CF45EEFDDEE97EEE14B8A24516A9FB2271` matched.

The report contains 438 ordered events, but frame-boundary samples and exact
watch hooks do not establish a full instruction count or basic-block trace.
`0x60D4A` was not dynamically observed. The meaning of `Start`, the meaning
of the routines/data, complete callee/return state, and any unobserved control
flow remain UNKNOWN. No broader search was performed after the first success.

## M11.5 — bounded dynamic caller discrimination of 0x6121A
**Status:** VERIFIED bounded raw execution evidence, developer-only. The frozen
`natural_idle_to_6121a_v1` scenario remains unchanged: BizHawk 2.11.1,
hardware reset, neutral input, canonical USA ROM SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`, horizon
`max_frames:300`.

Static caller table, verified against the local canonical USA ROM:

| call-site | bytes | instruction | displacement | target | size | return address |
|---|---|---|---:|---|---:|---|
| `0x60B8C` | `61 00 06 8C` | `BSR.W` | `+0x068C` | `0x6121A` | 4 | `0x60B90` |
| `0x60D4A` | `61 00 04 CE` | `BSR.W` | `+0x04CE` | `0x6121A` | 4 | `0x60D4E` |
| `0x611EE` | `61 00 00 2A` | `BSR.W` | `+0x002A` | `0x6121A` | 4 | `0x611F2` |

The bounded probe watches only those three call-sites and `0x6121A`. Both
target hits occur at frame 113 and pair with `0x611EE` without an intervening
watched event: `caller seq=113 -> target seq=114`, then `caller seq=115 ->
target seq=116`. Hit 1 has caller A7 `0x00FF0BE6`, target-entry A7
`0x00FF0BE2`, delta `-4`; hit 2 has caller A7 `0x00FF0BAC`, target-entry A7
`0x00FF0BA8`, delta `-4`.

The longword read at target entry `[A7]` is `0x000611F2` for both hits, exactly
matching the statically computed return address for `0x611EE`. The captured raw
register delta for each pair contains only A7; D0-D7, A0-A6 and SR are equal in
the caller and target snapshots. This is instruction-level stack/register
evidence, not an ABI or semantic conclusion. The second pair is recorded as a
second observed caller-target event; whether it represents re-entry remains
UNKNOWN.

Two fresh executions of the identical scenario produce byte-identical caller
reports and traces. The normalized existing importer reports 117 events, 23
unique PCs, 0 inferred basic blocks, 0 inferred branch/call/return/read/write
events and deterministic hash `0x52F951E69F5A7100`. The trace includes frame
samples and exact watched-hook events; it is not a full instruction trace.

The result upgrades only the specific executed edge `0x611EE -> 0x6121A` for
this scenario. `0x60B8C` and `0x60D4A` were not observed, so their static edges
remain dynamically unselected. Because the selected caller is `0x611EE`, the
result is not relevant to the existing `0x60B8C`/`0x60D4A` caller-stack blocker.
The adapter exposes no separate post-instruction hook, so broader hook-timing
and callee/return-state questions remain outside this checkpoint. No semantic
name or function purpose is assigned to `0x6121A`.

## M11.5 — bounded natural reachability of 0x6121A
**Status:** VERIFIED bounded runtime evidence, developer-only. No emulator,
ROM, savestate or generated trace is part of the repository.

The frozen scenario `src/tools/re_bizhawk_natural_scenario.txt` uses schema
`oasis.m68k.emulator-scenario.v1`, ROM SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`, backend
BizHawk 2.11.1, `start_state=hardware_reset`, neutral controller state and
`stop_condition=max_frames:300`. The Lua probe uses real frame advancement and
target execution hooks only; it does not force PC/register/memory state or
patch the ROM.

Two independent runs reached primary target `0x6121A` at frame 113 after 114
frame advances. The exact target hook fired twice. The first-hit entry snapshot
is raw evidence only: PC `0x0006121A`, SR `0x00002714`, D0-D7
`FFFF,7FF0FFFF,FFFF,0,940F0A8C,60000083,0,40`, A0-A7
`00FF06F2,00FF0BFC,00000000,00FF13CC,00C00004,00FF001A,00000000,00FF0BE2`,
and stack window `[0x00FF0BC2,0x00FF0C22)`. No semantic role is assigned to
any register or stack value. The two natural JSON reports and normalized
human-readable trace imports are byte-identical.

Watched secondary addresses `0x60B8C`, `0x60D4A`, `0x60BCC`, `0x60BD0`,
`0x60BFA` and `0x60C08` were not observed in this finite scenario. Static raw
inspection confirms three direct call-sites to the primary target:

| call-site | bytes | displacement | target |
|---|---|---:|---|
| `0x60B8C` | `61 00 06 8C` | `+0x068C` | `0x6121A` |
| `0x60D4A` | `61 00 04 CE` | `+0x04CE` | `0x6121A` |
| `0x611EE` | `61 00 00 2A` | `+0x002A` | `0x6121A` |

The dynamic target callback does not expose the immediately preceding CPU
instruction. `previous_observed_pc=0x6135E` is only the last frame-boundary
sample and is explicitly not caller evidence. Exact transfer source, caller
selection, return state and stack provenance therefore remain UNKNOWN. The
normalized imported trace contains 114 bounded events, 113 frame-boundary PC
samples plus the exact target event(s), and no inferred branch/call/memory edge.
MAME writer provenance was not repeated because the BizHawk target reach was
already reproducible and a second backend would not expose the missing
preceding instruction without expanding scope.

This result proves deterministic natural reachability of the raw target only;
it does not prove routine semantics or resolve any static unresolved reference.

## M11.5 — external emulator boot-trace oracle PoC
**Status:** VERIFIED with real local backends, bounded to `boot_initial` and
developer-only. No emulator binary/source or ROM is part of the repository,
and no fake dynamic trace is used.

`oasis.m68k.emulator-trace.v1` is a developer-only normalized capture layer
outside `oasis_core`. Its neutral importer accepts externally supplied event
records for PC, optional block, branch, direct/indirect control flow, return,
memory access and optional D0-D7/A0-A7/SR snapshots. The normalizer records
ROM/emulator/backend/scenario/stop-condition metadata, deterministic event order and hash, bounded
coverage, direct call edges, indirect targets, reset-vector evidence, and
Atlas-known/Atlas-unknown executed PCs. Branch/call/indirect targets are
separately split into Atlas-known and Atlas-unknown sets. Frame/cycle values
are retained as non-deterministic fields and excluded from the hash; optional
register snapshots are deterministic hash input.

The static reset-vector reader is limited to the first eight ROM bytes:
initial A7 is the longword at offset 0 and initial PC is the longword at offset
4. Agreement with a first observed PC is reported only when an external trace
is supplied; it is not inferred from static bytes alone. The accepted input
header is `oasis.m68k.external-trace.v1`, followed by metadata lines such as
`scenario=boot_initial` and event lines such as
`event seq=0 pc=0x00000100 kind=instruction block=0x00000100 size=4`.

The real bake-off used already-installed MAME and BizHawk against the ignored
canonical USA ROM. Exact results follow; no emulator binary/source or ROM is
part of the repository.
| backend | exact executable/version | boot capture | register/memory evidence | replay |
|---|---|---|---|---|
| BizHawk primary | `D:\Program Files\BizHawk-2.11.1-win-x64\EmuHawk.exe`, `2.11.1` | 512 instruction events, 640 total events | D0-D7/A0-A7/SR and 128 bus writes through Lua hooks | A/B equal, `0x5CCA6906FAA6A219` |
| MAME secondary | `D:\Program Files\Mame\mame.exe`, `0.289` | 512 instruction events | D0-D7/A0-A7/SR through debugger trace; separate write watchpoint | A/B equal, `0xC2B1C053D1E43D76` |

Capability record (tested means exercised locally; untested means not claimed):

| capability | BizHawk 2.11.1 | MAME 0.289 |
|---|---|---|
| CLI launch / Genesis ROM load | tested | tested |
| unattended execution / automated shutdown | tested (`--chromeless`, `client.exitCode`) | tested (`-video none`, debugger `quit`) |
| scripting / Lua | tested Lua | debugger command script tested; Lua not used |
| M68000 PC | tested | tested |
| D0-D7, A0-A7, SR | tested | tested |
| execution hook/breakpoint | tested instruction hook | tested registerpoint |
| memory read hook | untested | untested |
| memory write hook/watchpoint | tested bus write hook | tested RAM watchpoint |
| save-state load/save | untested | untested |
| deterministic stepping / fixed stop | tested `emu.frameadvance`, 512 callbacks | tested fixed 512-event registerpoint |
| stdout/file trace export | tested Lua file export | tested debugger trace file |

MAME working command:
`mame.exe genesis -cart "Beyond Oasis (USA).bin" -homepath mame-home -video none -sound none -nothrottle -skip_gameinfo -debug -debugscript re_mame_boot_trace.cmd -seconds_to_run 10`.
BizHawk working command:
`EmuHawk.exe --chromeless --lua="re_bizhawk_boot_trace.lua" "Beyond Oasis (USA).bin"`.
The Lua script uses `event.on_bus_exec_any`, `event.on_bus_write`, register
reads and `emu.frameadvance`, then exits at exactly 512 instruction callbacks.
MAME's debugger script uses a fixed registerpoint and trace action; its
separate writer probe caught a 16-bit write at address `0x00FFFFFE`, with the
stopped instruction PC `0x0000026A`. The instruction-trace adapters currently
emit no normalized branch/call/return/read events, so those report counts are
zero and are not inferred from opcode text. Save-state APIs and read hooks were
not probed. The MAME trace begins at `0x214`; BizHawk begins at `0x26C`; both
are after reset PC `0x20E`, so reset/bootstrap transitions remain unknown.
Neither trace observed `0x6121A`, `0x60B8C` or `0x60D4A`. No semantic Atlas
entries were added.

## M11.5 — bounded caller-stack provenance before `0x60BCC`
**Status:** bounded implementation and USA oracle verified. No semantic role is
assigned to any stack value. The additive schema is
`oasis.m68k.re-caller-stack.v1`, a developer-only report over the existing
reachable `[0x60004,0x61204)` slice.

The call-site is `0x60BCC` in block `[0x60BC4,0x60CDA)` with reachable
predecessor `0x60BA4`. Symbolic entry A7 is `S`; the pre-call value is named
`P` only as the caller's pre-BSR stack pointer. The captured reachable path
contains these bounded stack events: `0x6042A MOVE.W SR,-(A7)` (2 bytes),
`0x60430 MOVEM.L` with mask `0x7FFE` (14 longwords, 56 bytes), and
`0x60B66 MOVE.L A0,-(A7)` (4 bytes). These operations establish depth only;
unknown register/status values are not converted into concrete data.

The USA oracle finds two relevant paths. One crosses direct `BSR.W 0x6121A`
at `0x60B8C`; another reaches the same call-site through `0x60D4A -> 0x6121A`
and a locally proven balanced call. Each unknown callee is outside the
permitted effect set, so its A7 and stack-slot effect is UNKNOWN and the
bounded provenance is invalidated. The later `BSR.W 0x604BC` at
`0x60BCC` uses the previously proven local callee summary only: its explicit
stack delta is zero and RTS restores caller pre-BSR A7. Consequently the
longword read by `0x60BD0 MOVEA.L (A7)+,A0` at `memory[P]` has no proven value;
`0x60BFA` and `0x60C08` remain unresolved. Reachable unresolved remains
`16→16`; the 14 `call_clobber` records are unchanged; speculative resolutions
are zero.

The USA oracle checks raw bytes at `0x6042A`, `0x60430`, `0x60B66`, `0x60B8C`,
`0x60D4A`, `0x60BCC`, `0x60BD0`, `0x60BFA`, and `0x60C08`, plus bounded CFG
and blocker evidence. It passes for the local supported USA ROM. No general
stack emulator, ABI, recursive callee analysis,
dynamic tracing, gameplay runtime integration, or M12 work was added.

## M11.5 — bounded callee-effect audit for `0x60BCC`
**Status:** VERIFIED bounded raw effect; no semantics inferred. The call-site
bytes `61 00 F8 EE` at `0x60BCC` decode as direct `BSR.W 0x604BC`; `0x60BCC`
is not the callee entry. Current decoder evidence bounds the callee as
`[0x604BC,0x604E6)`: one reachable block, one `RTS` at `0x604E4`, no direct
nested calls, indirect flow or unsupported instructions. The three static bit
operations use unresolved `(d16,A6)` references; concrete decoder references
are at `0x604BC -> 0x00FF0628`, `0x604C8 -> 0x00FF06F2` and
`0x604DE -> 0x00FF0016`.

The effect table is raw register evidence: A0 `overwritten_unknown` due to
three `(A0)+` writes; A1-A5 `not_touched`; A6 `overwritten_known`
`0x00FF06F2`; A7 `preserved`. The callee has no explicit stack operation.
Timeline: caller A7=P -> BSR pushes return `0x60BD0` at P-4 -> callee runs with
A7=P-4 -> `RTS` pops that return address and restores P -> caller executes
`0x60BD0 MOVEA.L (A7)+,A0`, reading an unknown longword at P and incrementing
A7 by 4. Therefore `0x60BD0` consumes another stack value, not the BSR return
address. Targets `0x60BFA` and `0x60C08` remain unresolved; 14
`call_clobber` refs and zero speculative resolutions are unchanged.

The additive developer-only report schema is
`oasis.m68k.re-callee-effect.v1`; it includes bounded CFG edges, return sites,
per-register effects, instruction/block-bound memory evidence, unsupported and
indirect categories, stack timeline and the two target rechecks. The USA oracle
checks exact bytes/CFG/effects when the local user-supplied ROM is available;
that ROM was absent in the current workspace. No ABI, recursive callee scan,
emulator, dynamic tracing, runtime behavior, whole-ROM work or M12 was added.
Exact reachable addresses: `604EA`, `60BD8`, `60BFA`, `60C08`, `60C1E`, `60C34`,
`60C4A`, `60C60`, `60C76`, `60C8A`, `60C94`, `60CAA`, `60CC2`, `60D94`, `60DB0`, `60DC8`.
## Known hardware addresses
| Address | Meaning | Confidence | Evidence/status |
|---|---|---:|---|
| `0x00C00000` | Mega Drive VDP data port | CONFIRMED | Public ROM-hacking constants + standard Mega Drive mapping |
| `0x00C00004` | Mega Drive VDP control port | CONFIRMED | Public ROM-hacking constants + standard Mega Drive mapping |
| `0x00FF0000` | 68000 work RAM base | CONFIRMED | Public ROM-hacking constants + standard Mega Drive mapping |
## Reference ROM identity

### USA retail — canonical engineering reference
| Field | Value | Confidence |
|---|---|---|
| Title | `Beyond Oasis (USA)` | CONFIRMED |
| Size | `3145728` bytes | CONFIRMED |
| CRC32 | `C4728225` | CONFIRMED |
| SHA-1 | `2944910c07c02eace98c17d78d07bef7859d386a` | CONFIRMED |
| SHA-256 | `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263` | CONFIRMED from uploaded reference |
| Detector result | `SUPPORTED` | VERIFIED |

### Region policy
- USA addresses are the default notation in reverse-engineering documents.
- Region-specific offsets must not leak into native gameplay code.
- Europe/Japan may be compared to USA as secondary evidence.
- Final C++ game model is region-independent.

## `[0x00003820, 0x00003B3E)` — graphics decompression routine
**Status:** VERIFIED and translated to native C++.

**Semantic confidence:** CONFIRMED for decompression behavior and call contract.

### Boundaries and call contract
- Exact half-open range: `[0x3820, 0x3B3E)`.
- Verified USA ROM contains 52 direct absolute `JSR 0x3820` calls and 0 direct absolute JMPs.
- `A0`: compressed source pointer; returns advanced to immediately after consumed input — CONFIRMED.
- `A1`: destination pointer; returns advanced to immediately after output — CONFIRMED.
- `D0-D2/A2` are preserved; format B additionally preserves `D3/D6/D7`.
- No hardware access or nested subroutine call occurs inside the decompressor.

### Format dispatch
At `0x3824`, byte `source[2]` selects the format:
- nonzero: command-stream format A;
- zero: bitstream format B.

### Format A
Observed command families:
- literal byte runs;
- repeated-byte runs;
- sliding-window backreferences into already-produced output;
- chained `0b011xxxxx` extensions continuing the preceding backreference;
- block-length framing and byte terminator.

### Format B
Observed behavior:
- three-byte block header followed by LSB-first control bits;
- literal tokens;
- multiple backreference forms;
- distance `1` special repeated-byte run;
- distance `0` block terminator;
- 16-bit control-bit refill.

### Native mapping and oracle traces
Implementation: `src/game/graphics_decompress.*`.

| Format | Caller | ROM source | Source consumed | Output bytes | Output SHA-256 |
|---|---:|---:|---:|---:|---|
| A | `0x00C394` | `0x16943C` | 1217 | 3072 | `65e99e74020fedbdcb97c8249a5ccfe540aca5bb5d29bfb260352cd6f388c31a` |
| B | `0x03C276` | `0x1894EA` | 112 | 128 | `167d4e5409f6b075b3b6f2bc61dbb747e8d8c857e8699745184ddf48d83bcda9` |

Native C++ matches original 68000 execution on source consumed, output length and SHA-256 for both oracle cases.

## M7 — indexed compressed-resource table at `0x0005CE96`
**Status:** INVESTIGATING.

### Table structure
- Base used by original code: `0x0005CE96` — CONFIRMED.
- Entry width: 4-byte absolute ROM pointer — CONFIRMED by reader at `0xD3B2`.
- Entry 0: `0x000000` — CONFIRMED; semantic role remains UNKNOWN (likely sentinel/unused).
- Entries 1..107 form a dense run of valid compressed-resource pointers from `0x1AD000` through `0x1E6EDA` — CONFIRMED.
- First entries:
  - index 1 -> `0x1AD000`;
  - index 2 -> `0x1AD9D4`;
  - index 3 -> `0x1AE1A8`;
  - index 4 -> `0x1AE8AA`.
- Sample pointed streams have nonzero `source[2]` and therefore use verified decompressor format A — CONFIRMED for sampled entries.

### Reader at `0x0000D3B2`
**Behavior: CONFIRMED.**

Relevant original sequence:
```text
D3B2 save D0/A0/A1
D3B6 A1 = 0xFF2FA8
D3BC A0 = 0x05CE96
D3C2 D0 <<= 2
D3C4 A0 = *(A0 + D0)
D3C8 JSR 0x3820
...
D404 RTS
```

Therefore incoming `D0` is a 0-based resource-table index and selected entry is decompressed into work RAM `0xFF2FA8`.

### Direct calls to `0xD3B2`
Static absolute-call search found seven direct calls:
- `0x02CFAA`, `0x02CFB8`;
- `0x02D410`;
- `0x032174`, `0x032182`;
- `0x032884`, `0x032892`.

Immediate `D0` values observed immediately before these calls include:
- `3`, `4`;
- `0x57` (87);
- `0x23` (35), `0x24` (36).

This proves the table is selected by stable numeric IDs used in gameplay/scene code. What those IDs mean is still under investigation.

### Developer screen-name list
The canonical ROM contains a developer-facing screen-name list in the same general metadata region:
- `VILLAGE` at `0x05DB4D`;
- `ECAPITAL` at `0x05DB56`;
- `HARBOR` at `0x05DB61`;
- `01-00` at `0x05DC48`;
- `01-05 BOSS` at `0x05DC70`;
- `14-01 KING` at `0x05E110`.

**Current hypothesis:** the `0x5CE96` indexed compressed-resource table may correspond to screen/room resources because its ~107 meaningful entries and stable numeric selectors are compatible with the ROM's developer screen inventory. **Confidence: LIKELY, not CONFIRMED.** Exact index-to-name correlation is the next proof target.

### Structure-processing routine at `0x0000D406`
**Status:** INVESTIGATING; high relevance to world/screen setup.

This routine is called extensively from code in roughly `0x2Cxxx..0x3Axxx` and reads a structured record through `A1`:
- words at offsets `+16`, `+18`, `+20` copied to RAM `0xFF16F4..0xFF16F8`;
- word `+22` -> `D6`;
- word `+24` -> `D7` and RAM `0xFF16F0`;
- long at `+8` -> RAM `0xFF16FA`;
- bytes beginning at `+12` are transformed into derived RAM tables;
- additional state buffers begin at `0xFF1716` and `0xFF173E`.

Do not name the record fields until caller/data evidence establishes semantics.

### Related resource loading at `0xD4C8..`
The same `0x5CE96` table is also indexed by four bytes from RAM `0xFF16FA`; each nonzero index selects and decompresses a resource into separate 4096-byte-spaced destinations beginning at `0xFF3FA8`. This strongly indicates the table contains reusable scene-related graphic/data resources, but exact semantics remain UNCONFIRMED.

### M7 next proof
1. enumerate developer screen names in exact order and compare indices `3`, `4`, `35`, `36`, `87` against table selectors;
2. disassemble the seven `0xD3B2` callers far enough backward to identify their surrounding screen/event identity;
3. decompress representative table entries with native C++ and compare header/structure patterns;
4. only after correlation, introduce a portable room/screen resource loader.

### M12-GFX-MAX screen graphics root closure — CONFIRMED

The screen dispatcher evidence closes the root table boundary
`[0x00C92C,0x00C980)`: `0xC8F0` selects one of exactly 21 big-endian
longword roots from this range, and every root points inside the canonical
ROM. The table byte SHA-256 is
`71889c183a0db91d0ec04d3e6ad40fdc6181fa034fec1c552fbfa007997ac895`.
The raw table is classified as `SCREEN_GROUP_POINTER_TABLE`; no field names
are assigned to the pointed-to descriptors beyond the already confirmed
26-byte structural contract.

The bounded screen-resource census has 167 descriptor uses, 163 unique
descriptors, and 159 unique accepted graphics streams. Their exact compressed
stream intervals cover 337,514 ROM bytes and all are already owned by the
M12 manifest as `LOCAL_ROM_DERIVED_ASSET`; the 4,238 descriptor bytes are
already `STRUCTURED_DATA_CONFIRMED`. This pass therefore adds only the 84-byte
root table and does not duplicate or reclassify the streams.

The complete known `0x3820` census remains 52 static callers: 17 direct ROM
source sets, 2 finite-table-derived sets, 23 parameterized sequential
families, and 10 unresolved dynamic producers. The ten blockers remain
`0x00D54A`, `0x00D650`, `0x02DB52`, `0x02F6A0`, `0x03B236`, `0x03B28A`,
`0x03B2FE`, `0x03C07C`, `0x03D5AE`, and `0x03E61A`; their RAM-mediated,
inherited, helper-output, or sibling-call source values are not ROM-proven.
No additional non-screen loader family gained a finite exact source boundary.

### M12-GFX-MAX `0xD406` continuation verification — CONFIRMED

The exact `0xD406` census finds 17 screen-descriptor positions without the
direct `JSR` at `descriptor+0x1A`. For 15 of them, the existing 68000 slice
decoder plus the closed verifier in
`src/tools/re_m12_gfx_loader_census.py` decodes every intervening instruction
and proves that all paths reach the exact `JSR abs.l,0x00D406` without writing
`A1`: `0x02E98C`, `0x03007E`, `0x031B76`, `0x031C76`, `0x031DE6`, `0x0320B6`,
`0x03215E`, `0x03567A`, `0x03697C`, `0x036A06`, `0x036AC6`, `0x033512`,
`0x037B5E`, `0x039C0C`, and `0x039F9A`. The only branch is `BPL.S` at
`0x02E994`, whose taken edge reaches `0x02E99A` and whose fall-through
`ADDI.W` also reaches it. The remaining two screen sites, `0x038FD6` and
`0x03959A`, remain unresolved because no bounded direct continuation was found.
This confirms loader continuation relations only; it adds no ROM ownership.
Negative evidence is recorded explicitly: `0x038FD6` is a code-like
`MOVEM.L`/RAM-state entry with no bounded `0xD406` continuation, while
`0x03959A` is a code-like `MOVEM.L`/`LEA.L` entry with a separate unbounded
`0x0395D8` sibling call. Both remain blocked on parent/caller `A1` provenance;
neither is promoted.
Focused regression coverage is 4/4, and the canonical census output is
`3E9F5D9D11AA3B38500859E35449CCEE945CA0BDBA993C50A8F0D461CA40270E`.

## M8 — player input and movement slice
**Status:** IMPLEMENTED as a portable movement/state slice; full animation/entity callback semantics remain INVESTIGATING.

### Player entity selection
- `0x0013D6..0x00142E` initializes the main entity at work RAM `0xFF19E8` and writes entity type `2` before calling `0x8D06` — **CONFIRMED**.
- The main entity pool uses 21 records with stride `0xBC`; `0x008EB2..0x008ED0` iterates that pool — **CONFIRMED**.

### Controller normalization and direction mapping
- `0x00217C..0x002188` calls the controller reader at `0x2992`; normalized port state is stored beginning at `0xFF165C`, with the movement nibble in `0xFF165E` — **CONFIRMED**.
- `0x0085E2` masks `0xFF165E` with `0x0F`, dispatches through the table at `0x85FA`, and returns direction plus fixed-point deltas — **CONFIRMED**.
- Cardinal vectors are `(+0x36000,0)`, `(-0x36000,0)`, `(0,+0x30000)`, `(0,-0x30000)`; diagonal vectors are `(+/-0x2A000,+/-0x25800)` — **CONFIRMED** from `0x8624..0x86AE`.
- The native mapping preserves the original low-nibble behavior, including opposite-direction collapse and diagonal combinations; unsupported combinations resolve to no movement — **VERIFIED by synthetic tests**.

### Movement and collision contract
- The main game loop at `0x008B22` calls `0x00557A` (player update) before `0x008E90` (active-entity movement), and then `0x00A196` (sprite/entity scheduling) — **CONFIRMED**.
- `0x00557A` selects `0xFF19E8` and enters `0x005670`; the state dispatch at `0x0059BA` routes entity `+0x04` state `0` to `0x0061F6`, state `2` to `0x0062E4`, and state `4` to `0x006516` — **CONFIRMED**.
- `0x0061F6` reads the normalized direction, writes direction `+0x16`, intent deltas `+0x4E/+0x52`, accumulated deltas `+0x72/+0x76`, and transitions `+0x04` to state `2` — **CONFIRMED**.
- The state-2 branch at `0x0062E4` calls the same direction routine; its no-input path at `0x0062CC` clears `+0x4E/+0x52`, writes `+0x2A=0` and returns `+0x04` to `0`; shared movement owns cleanup of `+0x72/+0x76` — **CONFIRMED**.
- The state-4 branch at `0x006516` maps `+0x16` through `0x83D4`, gates the turn on `+0x17` and the normalized input, and accumulates the selected axis through `+0x72/+0x76` — **CONFIRMED**.
- When state-4 input is absent, `0x006618` compares `FF197E` with `6`; the short path reaches the state-2 stop block, while the timeout path writes `+0x04=0x000C` at `0x006624` — **CONFIRMED**.
- The shared movement routine consumes `+0x72/+0x76` before committing position; `+0x2A` and `+0x26` participate in the animation/state sequence, but their presentation semantics remain **INVESTIGATING**.
- `0x008F12..0x00938C` is the shared active-entity movement update; main-pool records enter at `0x8F22` — **CONFIRMED**.
- X/Y fixed-point deltas are accumulated in entity fields `+0x72/+0x76`; integer positions are committed to `+0x08/+0x0C` — **CONFIRMED**.
- `0x009BF2` aggregates the entity footprint, and `0x00938E` is called before an axis commit; carry set takes the blocked path — **CONFIRMED**.
- Native `update_movement_state` mirrors the confirmed state-2 stop and state-4 axis-selection/accumulation rules; `PlayerState::try_move` then consumes those deltas through `ByteGridView::aggregate_world_square` and `evaluate_terrain_gate`.
- Native `VelocityAdjustContext` mirrors the three confirmed `0x64C4` outcomes and is optional until the external flag lifecycle is reconstructed.
- The shared footprint OR result is written at entity `+0x6F`; state-4 calls `0x64C4` with the retained axis delta, where global flags and the low nibble of `+0x6F` select no scaling, half scaling, or a Y-only half scaling — **CONFIRMED**, exact global lifecycle **INVESTIGATING**.
- `FF1985` is written by several event/control paths including `0x56F2`, `0x57C6` and `0x7E50`, and is read by the player dispatcher and `0x64C4`; it is not a player-owned field — **CONFIRMED**, lifecycle **INVESTIGATING**.
- `FF1984` is cleared/set by the active-entity checks around `0x2D220` and `0x2F250`, including a main-entity test against `+0x6E` bit 5 and `+0x10`; it is an external context flag — **CONFIRMED**.
- Bit 4 of `FF16F1` is read by `0x64C4`; no direct bit-4 writer was found in the scanned ROM references, so its producer remains **UNKNOWN**.
- Rendering, animation scripts and the unknown entity callback at `+0x22` are intentionally outside this slice.

## M9 — common entity pool framework
**Status:** IMPLEMENTED; raw pool iteration and one representative callback-dispatch path are verified. Semantic entity behavior remains outside M9.

### Pool iteration evidence
- `0x008E90` iterates four records from `FF2954` with stride `0x5A`, stores dispatcher `0x8EAA` in `FF193C`, and branches active records to `0x8F12` — **CONFIRMED**.
- `0x008EB2` iterates 21 records from `FF19E8` with stride `0xBC`, stores dispatcher `0x8ECC` in `FF193C`, and branches active records to `0x8F22` — **CONFIRMED**.
- `0x008ED4` iterates six records from `FF2D8C` with stride `0x5A`, stores dispatcher `0x8EEE` in `FF193C`, and branches active records to `0x8F12` — **CONFIRMED**.
- Each loop reads record word `+0x00` and uses `bgt` as the active test; zero and negative values are skipped — **CONFIRMED**.
- The native `EntityPoolView` exposes bounds, raw record spans and this active predicate without assigning enemy/NPC semantics to records.

### M9 boundary
The first M9 slice does not translate AI, attacks, animation callbacks, spawn tables or entity field names beyond the raw offsets required by pool iteration. Those require separate caller/data evidence.

### M9 shared record fields and representative dispatch

- The common movement entry at `0x8F22` reads raw record offsets `+0x9C`,
  `+0x9D`, `+0x2A`, `+0x2C`, `+0x2E`, `+0x30`, `+0x32`, `+0x37`, `+0x38`,
  `+0x72` and `+0x76` — **CONFIRMED** by direct accesses in the shared
  entry. The initializer at `0x8D06` copies two ROM pointers into `+0x26`
  and `+0x22`; their presentation/behavior meanings remain unassigned.
- A representative non-player path is the `FF2954` processing block at
  `0xA6A4`: it scans four `0x5A`-byte records, gates on `+0x00 > 0` and
  bit 2 of `+0x3A`, then reaches `0xA7D4`, which performs the proven
  indirect `jmp (+0x22)`. This is recorded as a raw callback-dispatch
  behavior, not as enemy/NPC AI.
- `EntityRecordView` exposes bounded big-endian word/long reads and the raw
  `+0x22` pointer. It deliberately does not invoke ROM addresses or assign
  semantic names to the remaining fields.

## Existing translated compatibility behavior

## M10 — spirit slot and dispatch trace
**Status:** IMPLEMENTED as a narrow deterministic slice; spirit names, button
meanings, targeting, abilities and presentation semantics remain UNKNOWN.

### Slot storage and lifecycle evidence
- At `0x007BE8`, the event handler subtracts `0x16` from the event code and
  uses the result as a bit index for `0x00FF0DBA` — **CONFIRMED**. The four
  observed event values `0x16..0x19` therefore map to slot bits `0..3`.
- At `0x005202`, the status-display path reads `0x00FF0DBA`; the adjacent
  byte table at `0x00522E` is `12 13 14 15 16 17 18 19` — **CONFIRMED**.
  This establishes four contiguous slot/event entries, but does not prove
  their character names or ability semantics.

### Active dispatch evidence
- `0x0031B80` checks bits `3` and `1` of entity field `+0x41` before entering
  the observed path — **CONFIRMED**. The meanings of these raw input bits are
  intentionally unassigned.
- The path tests bit `1` of `0x00FF0DBA`, then tests bit `0` of
  `0x00FF0DC4` as a one-shot guard — **CONFIRMED**.
- When the slot gate is open, it calls `0x0000C2EC` with selector `0x13`,
  fixed values `D5=0`, `D6=0x4000`, `D7=0x4800`, and sets guard bit 0 —
  **CONFIRMED**.
- It then falls through to queue selector `0x15` through `0x0000CA24`, using
  the player record's `+0x08` position-derived value and `D4=0x18` —
  **CONFIRMED**. The selectors are retained as raw trace values; their
  resource/effect semantics are not promoted beyond what the callers prove.

### Native mapping and oracle
`src/game/spirits/spirit_slots.*` models event-to-slot bit updates, the raw
`0x31B80` gate/selector trace and the `0x7A10`/`0x846C` summon-entry seed
without invoking ROM addresses. Synthetic tests cover inactive input,
unavailable slot, open guard, repeated guarded dispatch and the accepted or
rejected summon-entry gate. `oasis_spirit_slots_reference` checks the USA
bytes at `0x7BE8`, `0x5202`, `0x522E`, `0x31B80`, `0x31BC4`, `0x7A10` and
`0x846C`.

### Target-selection evidence
- `0x17CA6` calls `0xB922` with relative bounds `[-10,-6,10,6,0,4]`, then
  checks the returned record's raw type word against `0x16` at `0x17CE4` —
  **CONFIRMED**.
- `0xB922` scans the 21-record `FF19E8` pool in order, skips an inactive
  record or a record whose pointer equals the owner pointer, applies the
  observed X/Y interval checks and Z containment check, and returns the first
  spatial match — **CONFIRMED**. In the observed path the owner is from
  `FF2D8C`, so equal numeric indices across the two pools are not excluded.
- The native `find_observed_target` preserves the first-match-then-type-check
  behavior. The raw fields used by the query are exposed without semantic
  names; interaction meaning remains UNKNOWN. The ROM oracle also checks the
  query setup at `0x17CA6` and the scan prologue at `0xB922`.
- `0x846C` separately initializes raw type `0x16` in record `FF1AA4`; because
  `0xB922` scans `FF19E8`, this is tracked as a possible related producer but
  not as proof of a summon/ability relationship — **UNKNOWN**.
- The callback path is cross-pool: initializer `0xFFDE` obtains an owner from
  the six-record `FF2D8C` pool via `0xD9F0`, stores callback `0x17A96`, and
  `0x17A96` enters `0x17CA6` when its state word is zero. The query helper
  `0xB922` receives that owner in `A6` but scans target records from
  `FF19E8` — **CONFIRMED**. This corrects the native boundary; it does not
  prove summon or ability names.
- A static scan of field `+0x00` writers found the only literal raw type
  `0x16` write at `0x847C`, targeting singleton record `FF1AA4`. The observed
  `FF19E8` construction sites at `0x11DF0`, `0x27B2A` and `0x2BD20` use other
  literal/table values or data-driven values; no producer of raw type `0x16`
  in `FF19E8` is proven — **UNKNOWN**.
- The additional loader scan confirms that `0xFFDE` consumes a data stream
  into the auxiliary `FF2D8C` pool, while the generic stream loaders at
  `0xFF1A` and `0xFCB8` allocate from `FF1CD8`. These are concrete pool
  boundaries, but they do not identify a producer for raw type `0x16` in
  `FF19E8` or prove that the `0x17A96`/`0x17CA6` callback is a summon —
  **CONFIRMED boundary; UNKNOWN semantics**.
- A separate summon-entry candidate is now byte-backed: `0x7A10` rejects
  caller flag bit 1 and calls `0x846C` only for caller state `+0x30 == 0x18`.
  `0x846C` writes raw type `0x16` to singleton `FF1AA4`, copies caller
  position `+0x08/+0x0C`, writes raw `0x13` to `+0x10`, copies `+0x17` to
  `+0x66` and `+0x14`, writes `0x4F8` to `+0x18/+0x5A`, and clears `+0xA6`
  and `+0xAA` — **CONFIRMED raw entry; summon identity UNKNOWN**. The
  table-derived velocity tail beginning at `0x84B2` remains outside the native
  slice until its complete data contract is recovered.

### Tile copy to work RAM
Initial C++ compatibility implementation exists, derived from the public `tilecopy_to_ram` macro. Revisit after data interfaces stabilize.

### Tile copy to VRAM
Initial C++ compatibility implementation exists, derived from the public `tilecopy_to_vram` macro. Current VDP model is intentionally narrow.

## M11 — raw event producer and router boundary
**Status:** IMPLEMENTED as a bounded raw-data slice; event names, progression,
dialogue and command semantics remain UNKNOWN.

### Producer evidence
- `0x0082AE` calls bounded helper `0xB9EC` with the active `FF19E8` pool and
  a search window built from raw bounds `[-6,+6]` — **CONFIRMED**.
- The first returned record is accepted only when raw type `+0x00` equals
  `0x0008`; the source type is then cleared — **CONFIRMED**.
- The producer composes `FF1976` from source `+0x32` shifted left by eight
  and source byte `+0x52`, then copies source `+0x04` to `FF1978` and source
  long `+0x4E` to `FF197A` — **CONFIRMED**.
- A static USA-ROM scan found no direct literal `BSR` or absolute `JSR` to
  `0x82AE`; its caller may be indirect or data-driven and remains UNKNOWN.
- The source type-8 meaning and resulting event-code meaning are not
  assigned. Internal selection semantics of `0xB9EC` remain outside this
  slice.

### Router evidence
- `0x007A28` tests caller field `+0x37` bit 1, reads byte `FF1976` and
  dispatches bounded raw ranges to `0x7B64`, `0x7BD4`, `0x7BF6`, `0x7BA4`,
  `0x7BE8` and `0x7B84`; zero and values above `0x3F` fall through to
  `0x7A6C` — **CONFIRMED**.
- When the tested bit is clear, control goes to raw address `0x7B28`, an
  immediate `RTS`; the adjacent routine begins at `0x7B2A` — **CONFIRMED**.
- The separate `0x7B2A` routine calls external raw address `0x60004` with constants
  `0x0006` and `0x0008`, masks the result to `0x01FF`, returns on the sentinel
  `0x01FF`, then clears mask `0xFFF9` at `FF17B8` and writes raw `FF0D7E` to
  current-record `+0x06` and `0xFFFF` to `+0x5C` — **CONFIRMED**. The helper
  and field meanings remain UNKNOWN.
- The external entry at `0x60004` contains `BRA.W +0x0424`, whose 68000
  word-displacement target is `0x6042A`; `0x6042C` is the following
  instruction. Its raw command
  dispatcher compares `D0` against values `1..8`, and the command `0x0006`
  branch at `0x60478` reaches `0x609C6` — **CONFIRMED**. That handler starts
  with `D0=0`, builds a raw flag mask from driver RAM bit 4 values and returns
  through the shared driver epilogue at `0x611D8` — **CONFIRMED**.
- The command `0x0008` branch reaches `0x60D10`, which performs raw 68000/Z80
  bus operations and copies `0x0606` bytes from `A01000` to `FF0022` —
  **CONFIRMED**. Driver protocol and audio meaning remain outside M11.
- The command `0x0006` handler at `0x609C6` starts `D0` at zero, sets it to
  `0x01FF` when bit 0 of `FF001A` is set, then ORs bits from bit 4 of eleven
  driver RAM locations before returning through `0x611D8` — **CONFIRMED**.
  The event-side meaning of this mask remains UNKNOWN.
- On the `0x01FF` path, `0x7B2A` calls command `0x0008`, performs the raw
  state writes, then branches to `0x62CC`; `0x62CC` clears current-record
  fields `+0x4E`, `+0x52`, `+0x2A` and `+0x04` — **CONFIRMED**. This closes
  the bounded raw side-effect contract without assigning driver semantics.
- The native `event_router` module exposes the producer transfer, raw
  handler-address mapping and the bounded adjacent `0x7B2A` trace. Synthetic tests and
  the USA-ROM oracle cover the type gate, field composition and dispatch
  boundaries.

### M11 boundary
No generic event-stream parser, dialogue decoder, progression model or
unproven command is introduced. Further work must establish the caller/data
contract around the selected type-8 source or a downstream handler first.

### M11 post-completion RE-acceleration checkpoint — bounded `0x60004` slice
**Status:** VERIFIED as developer-only evidence tooling; no native gameplay
behavior is inferred or added.

- The local USA-ROM tool reads through `Rom::load`, validates the canonical
  fingerprint, and decodes only reachable direct control flow in the explicit
  half-open range `[0x60004, 0x61204)` — **VERIFIED**.
- The bytes `60 00 04 24` at `0x60004` are `BRA.W +0x0424`; the 68000
  word-displacement target is `0x6042A`. `0x6042C` is the following opcode,
  correcting the earlier ledger wording — **CONFIRMED by oracle**.
- The deterministic report contains 801 reachable instructions, 109 blocks,
  72 direct branches, 17 direct calls, 3 absolute ROM refs and 114 absolute
  RAM refs with constants attached — **VERIFIED**. Direct edges include
  `0x60004->0x6042A`, `0x60478->0x609C6`, `0x60488->0x60D10`; indirect and
  unsupported categories remain separate and empty on this reachable slice.
- Schema `oasis.m68k.re-slice.v1` and the human report are sorted/deterministic;
  Debug/Release hashes match. Decoder coverage remains bounded to exercised
  opcode families, never a generic CPU/emulator/recompiler/runtime dependency;
  unsupported instructions and indirect targets stay explicit.

**Open questions:** driver command meanings, audio protocol, event/progression
semantics and the producer caller remain **UNKNOWN**.

### M11.5 second RE-acceleration slice — bounded multi-function report
**Status:** VERIFIED as developer-only tooling; no production C++ behavior or
semantic names were added.
- The local-USA CLI analyzes four existing evidence targets: exact documented
  `[0x3820,0x3B3E)` and `[0xD3B2,0xD406)`, plus bounded-only windows beginning
  at `0x8E90` (`0x120` bytes) and `0xA6A4` (`0x180` bytes). Boundary discovery
  marks a return boundary only when every bounded path is complete; no boundary
  is guessed for the two windows.
- The deterministic `oasis.m68k.re-program.v1` report contains 421 decoded
  instructions in 131 basic blocks, one direct call site and one analyzed
  caller→callee edge (`0xD3B2 -> 0x3820`). Call sites retain caller function,
  basic block and instruction address; indirect/unresolved flow is separate.
- It records 18 confirmed absolute references (including `0x5CE96` and
  `FF2FA8`), 114 unresolved register-based references, one unresolved indirect
  jump at `0xA7E2`, and two unsupported opcode locations. Each memory item is
  bound to function, bounded slice, basic block and instruction.
- The USA oracle reproduces entry bytes, the reader call, pool dispatch edges
  `0x8EA6 -> 0x8F12` and `0x8EC8 -> 0x8F22`, and the raw callback jump boundary.
  Debug/Release JSON hashes match; synthetic tests cover confirmed boundaries,
  caller/callee grouping, bindings, unresolved flow and unsupported addressing.

**Limitations:** the decoder remains a bounded opcode-family decoder. It does
not resolve register-based effective addresses, indirect targets or unknown
function boundaries, and it is not an emulator, whole-ROM discovery pass or
recompiler. The `0x8E90` and `0xA6A4` reports intentionally include only their
explicit windows and must not be read as complete function recovery.

### M11.5 third checkpoint — bounded dynamic trace at `0xA7D4`
**Status:** VERIFIED as a developer-only dynamic evidence PoC; no gameplay
runtime or full emulator was added.

- The isolated scenario uses the static `0xA6A4` slice but starts from the
  controlled/savestate-like PC `0xA7D4`. It initializes raw `A6=FF2954`, the
  record word at `+0x00` to `1`, and the raw pointer at `+0x22` to `0xA7E4`.
- The bounded backend executes exactly `A7D4, A7DA, A7DE, A7E2, A7E4`:
  three static blocks, one not-taken `BEQ` at `A7DA`, two RAM reads, one
  indirect jump and one `RTS`. Relevant `A6`/`A0` snapshots are retained only
  at the unresolved memory/control-flow sites.
- Static/dynamic comparison resolves three prior items: the effective RAM
  addresses at `A7D4` (`FF2954`) and `A7DE` (`FF2976`), plus indirect target
  `A7E2 -> A7E4`. Nine other static unresolved memory references remain
  unobserved and therefore unresolved.
- The deterministic `oasis.m68k.re-trace.v1` JSON and human report retain PC,
  block, branch, call/return, memory and indirect-target evidence. Synthetic
  and USA-ROM oracles reproduce the same five PCs, branch outcome, RAM reads,
  pointer value and resolved target.

**Backend limitation:** this is a bounded scenario interpreter for the exact
five-opcode path, not a general 68000 CPU or Mega Drive emulator. It models no
full call stack, writes, interrupts, peripherals or alternate path; execution
stops explicitly on an unsupported scenario PC. Full-game tracing remains out
of scope.

### M11.5 fourth checkpoint — USA retail versus USA Beta 1994-11-01
**Status:** VERIFIED bounded correspondence only. `oasis_re_diff` reports the
five requested pairs: exact `0x3820->0x37D0`, `0x60004->0x60004`,
`0x82AE->0x825E`, `0x7A28->0x79D8`, and structural `0xA6A4->0xA654` with
changed block ordinal 10. No semantic identity or behavior is inferred.

### M11.5 fifth checkpoint — changed block ordinal 10 detail
**Status:** CONFIRMED raw bounded evidence. Retail `[0xA786,0xA792)` and beta
`[0xA736,0xA742)` are 12-byte blocks; corresponding edges are
`A6BA->A786`/`A66A->A736`, `A78E->A7D4`/`A73E->A784`, and
`A78E->A792`/`A73E->A742`. Only `2F3C0000A6BE` vs `2F3C0000A66E` differs
(`relocation_only`); `4A46` and `6B000044` (condition B) are identical.
Semantics remain unknown.

### M11.5 first bounded ROM Atlas prototype
**Status:** VERIFIED as developer-only aggregation; no semantic names or
runtime behavior were added. Schema: `oasis.m68k.re-atlas.v1`.

- The typed manifest contains 13 entries: code `0x3820`, `0x60004`, `0x7A28`,
  `0x82AE`, `0x8E90`, `0x938E`, `0x9BF2`, `0xA6A4`, `0xD3B2`; tables
  `0x5CE96`, `0x96E8`, `0x96F8`, `0xC92C`. IDs are raw address IDs.
- Exact boundaries are claimed only for `[0x3820,0x3B3E)`,
  `[0xD3B2,0xD406)` and the documented 108-entry table
  `[0x5CE96,0x5D046)`. Other entries expose bounded evidence ends without
  claiming function ownership.
- The report reuses `re_program`/`re_diff`/`re_trace` for 13 call edges,
  function/block-bound refs, beta correspondence and raw A6A4 facts; no
  whole-ROM scan was added. USA/Beta oracle: 13 entries, 1314 confirmed
  classified bytes, 6560 bounded bytes and zero conflicts; native statuses
  remain limited to previously tested paths.
- Atlas-driven `oasis.m68k.re-ranking.v1` ranks all 577 unresolved refs. USA
  result: displacement 446, `0x60004` 424, A6 387, immediate propagation 168,
  dynamic candidates 2 and unsupported decoder items 4.
**Unknown:** table sizes at `0x96E8`, `0x96F8`, `0xC92C`, bounded ownership,
unresolved effective addresses and routine semantics remain unknown. Atlas is
not an emulator, recompiler, whole-ROM map or gameplay runtime dependency.
### M11.5 Ghidra ROM Mapping PoC — canonical USA baseline
**Status:** VERIFIED as a developer-only structural discovery experiment;
decision `GHIDRA_USEFUL_WITH_PROJECT_FIXUPS` (option B). No semantic function
names or gameplay behavior are inferred from this output. Ghidra findings are
candidate evidence only; the existing `oasis_re` and runtime captures remain
the verification authority.

- Input fingerprint: 3145728 bytes, SHA-256
  `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
  Official Ghidra 12.1.3 headless used Temurin JDK 21.0.12.1+1, raw
  BinaryLoader, base `0x000000`, language `68000:BE:32:default`, compiler
  `default`, normal conservative auto-analysis and the developer-only
  `OasisGhidraMap` post-script.
- Export schema is `oasis.m68k.ghidra-map.v1`. The map contains 496 functions,
  438 candidates, 364 direct BSR targets, 106 direct JSR targets and 6
  vector-derived targets. The unresolved-call field is present and empty for
  this Ghidra run; no unresolved target is promoted to a resolved edge.
- Known-entry benchmark over
  `0x3820, 0x7A28, 0x82AE, 0x8E90, 0x938E, 0x9BF2, 0xA6A4, 0xD3B2,
  0x60004, 0x604BC, 0x6121A`: 7 exact function matches, one wrong boundary
  (`0x3820`, Ghidra ended at `0x38D0` versus known `0x3B3E`), one code-only
  entry (`0xA6A4`) and two missed entries (`0x7A28`, `0x82AE`). Thus exact
  function recall is 7/11 (63.6%) and code presence is 9/11 (81.8%).
- Required direct call edges were found at
  `0x60B8C->0x6121A`, `0x60D4A->0x6121A`, `0x611EE->0x6121A` and
  `0x60BCC->0x604BC`; the edges `0x60004->0x6042A` and
  `0xD3B2->0x3820` were missed. Result: 4/6 (66.7%). These are structural
  reference results, not proof of routine semantics.
- Data checks at `0x5CE96`, `0x96E8`, `0x96F8` and `0xC92C` all decoded as
  non-code and had useful xrefs (4, 20, 4 and 2 respectively). Only `0xC92C`
  was recognized as defined data. At `0xA7E2`, Ghidra listed `jmp`, did not
  identify an indirect flow through its API and exposed no reference/target;
  the existing bounded `oasis_re` evidence remains authoritative for the
  indirect dispatch observation.
- A bounded 20-item false-positive sample contained 19 `LIKELY_CODE` and one
  `AMBIGUOUS` item at `0x020E`. Two fresh external Ghidra projects produced
  identical 390972-byte JSON exports, SHA-256
  `613A3AA6DEB8D2DCF994C82ADC6A6939B7D5F27AF67A51D05C16F090D60A5315`.

**Limitations:** Ghidra emitted decompiler warnings around `0x6163E` and
several invalid or unresolved instruction addresses; these were not treated
as semantic evidence. The Windows wrapper rejected the `.md` suffix before
import, so an externally stored byte-identical `.bin` adapter copy was used;
the supplied ROM itself was not changed or copied into a tracked path. No
Ghidra project/database/raw disassembly or ROM was committed. This checkpoint
ends after the option-B decision; M12 is not started.

## ROM identification implementation
**Status:** VERIFIED.

Detector records byte size, Mega Drive header, Sega checksum, CRC32, SHA-1, SHA-256 and classification. Synthetic tests contain no original ROM bytes.
# M12-AUTO20 — Exact table-selected graphics streams

The developer-only `re_m12_table_graphics_promote.py` helper closes 35
graphics streams totaling 115,011 bytes. The source contract is the exact
99-row table `[0x3F306,0x3FF66)`, where field1 is the longword at row offset
`+4`. Each promoted start equals a field1 pointer, and each end equals the
independent deterministic graphics-census boundary with its recorded output
size. Streams that overlap prior ownership remain unpromoted. The transaction
reassembles the full canonical ROM byte-for-byte; no C++ migration is involved.
# M12-AUTO54 PC-island tables

AUTO54 closes 15 PC-relative table/literal ranges totaling 376 bytes. Exact
consumer bytes, repeated record shapes, and the next code/data marker close
each half-open boundary. The promoted objects include the 4x16-byte tables
at `0x01953C`, `0x025C24`, and `0x0288A2`, and three repeated 6-byte status
lookups at `0x03E430`, `0x03E492`, and `0x03E4F4`. Mixed candidates at
`0x0108D0`, `0x01142C`, `0x0230A4`, `0x04BAE`, and `0x061588` remain UNKNOWN.

# M12-AUTO53 PC-relative lookup family

AUTO53 promotes five fixed PC-relative lookup tables totaling 200 bytes. The
consumer contracts close the 4x16-byte records at `0x01E2A0`, the 20-word
table at `0x01FC98`, and three 4x8-byte tables at `0x027DBA`, `0x02959A`, and
`0x02A3E0`. No adjacent bytes are assigned.
# M12-AUTO57 bounded word-transform table provenance

AUTO57 promotes `[0x03BD86,0x03BE06)` (128 bytes) from the AUTO56 manifest as
`BOUNDED_WORD_TRANSFORM_TABLE`. Routine `0x03B7C0` is exact-decode supported:
it reads `(A6)+` once per iteration, transforms the word into the destination
buffer, and closes on `DBF D7,0x03B7CC`. The direct callers at `0x03A7A6` and
`0x03A854` load the table base (the latter resolves PC-relative `0x15DE` to
`0x03BD86`); callers at `0x03A9A8` and `0x03AA50` use the RAM pointer written
by the exact initializer at `0x03A87A`. The largest call sets `D7=0x3F`, so
the exact input extent is 64 words / 128 bytes ending at `0x03BE06`; the
second direct call sets `D7=0x0F`, independently confirming the same word
stride and base. The next bytes are not included merely because they are
adjacent or readable.

Evidence artifacts: `build/gpgx-classified.json` instructions at
`0x03B7C0..0x03B830`, the canonical ROM, and
`src/tools/re_m12_bounded_word_transform_promote.py`. Full-ROM materialization
is recorded in `build/m12-auto57-bounded-word-transform-a/`; the rebuilt ROM
retains the canonical size, CRC32, SHA1 and SHA256. C++ migration remains out
of scope.
# M12-AUTO59 bounded word-copy table provenance

AUTO59 promotes `[0x000472,0x0004B4)` and `[0x0031FC,0x003218)` (94 bytes
total) as `BOUNDED_WORD_COPY_TABLE`. Exact routine `0x002D66` saves registers,
reads one byte for the destination offset, one byte for `D7`, then copies one
word from `(A6)+` per iteration through `DBF D7,0x002D78`. The callers at
`0x0089C4` and `0x003260` load the two table bases directly. Their headers
close the extents at 32 and 13 words respectively. The two tables were
promoted independently from UNKNOWN ranges; no neighboring bytes were used as
ownership evidence.

# M12-AUTO58 exact static routine 0x003260 provenance

AUTO58 promotes `[0x003260,0x0032E8)` (136 bytes) as
`STATIC_EXACT_BOUNDED_ROUTINE`. The routine is exact supported 68000 ASM from
the static probe `build/m12-probe-003260.json`, has a direct control-flow
predecessor at `0x003240`, calls already-owned routines at `0x002D58`,
`0x003820` and `0x002CBC`, and ends at the exact `RTS` at `0x0032E6`. The
isolated wrapper `src/tools/re_m12_static_003260_roundtrip.asm` resolves the
external call labels only for vasm verification; it does not alter the source
artifact used by the ROM map. The inherited full layout has unrelated
pre-existing duplicate labels, so its assembler failure remains recorded and
is not presented as a successful full-layout build.
# M12-AUTO60 contiguous static-island provenance

The AUTO60 audit enumerated exact decoded instruction runs that (1) begin at a
static direct caller target, (2) remain contiguous through supported decoded
instructions, (3) end at an explicit `RTS`, and (4) lie wholly inside UNKNOWN
manifest intervals. Of 33 candidates, 15 passed the range tool and vasm
slice-level byte comparison. Those 15 intervals total 1,158 bytes and are
promoted as `STATIC_DIRECT_CALLER_RTS_ISLAND`; the other 18 remain UNKNOWN.
The complete positive and negative audit is
`build/m12-auto60-island-audit/audit.json`, with one ASM and one binary
artifact per candidate. AUTO60 consumes only entries whose audit status is
`EXACT`, and records their caller lists plus artifact hashes in its promotion
report. This is static source ownership, not a gameplay or C++ migration
claim.

# M12-GFX-MAX bounded 0x00D54A provenance audit

The exact `0x00F80E` helper begins with `MOVEM.L` saving all address
registers and restores the same set before `RTS`; it cannot produce a new
`A4` value for the later `0x00D54A` call. In the shared `0x00D406` body,
`0x00D42E` sets `A4` to `entry A1 + 4`, and `0x00D542` reads `A0` from the
longword at `(A4)` before `JSR 0x00D54A`. This proves the dynamic producer's
source relation as an inherited entry-record field, but not that the field
contains a canonical ROM address on every caller path. The classification is
therefore `INHERITED_A1_FIELD_NOT_ROM_PROVEN`; no executable or resource bytes
are promoted. A caller-closed path or a targeted trace with `A1`/`A0` at the
call is still required.

# M12-GFX-MAX bounded 0x00D650 continuation audit

The second `0x3820` call in the shared `0x00D406` body is a bounded
continuation of the first call. `0x00D550` and `0x00D552` copy the first
decompressor's returned post-source/post-destination `A0/A1` into `A4/A5`.
When bit 2 of `0x00FF16F1` is set, `0x00D64C` and `0x00D64E` restore those
post-states as the `A0/A1` arguments at `0x00D650`; `0x00D656` and `0x00D658`
capture the second post-state. This excludes an independent selector
interpretation, but it does not prove the first source longword is ROM-
originated or establish the exact boundary of the sequential compressed
stream. The classification is therefore
`FIRST_STREAM_AND_CONTINUATION_NOT_ROM_PROVEN`; no bytes are promoted.

# M12-GFX-MAX bounded 0x02DB52 post-source audit

The closed local slice `0x02DB3C..0x02DBB6` calls `0x00D406` at
`0x02DB40`. It then loads `A0` from `0x00FF17AA` at `0x02DB46`, while the
shared loader writes that field from its post-source `A4` at `0x00D65A`, and
sets `A1 = 0x00FF2FA8` at `0x02DB4C` before `JSR 0x003820` at `0x02DB52`.
This proves a post-source continuation of `0x00D406`, not an independent
caller argument. The incoming `D0` copied to `A0` at `0x02DB3C` and the exact
compressed-stream boundary remain unproven, so the classification is
`D406_POST_SOURCE_NOT_ROM_PROVEN`; no bytes are promoted.
