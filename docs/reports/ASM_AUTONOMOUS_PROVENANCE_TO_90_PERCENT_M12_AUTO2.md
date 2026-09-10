# M12-AUTO2 — bounded provenance toward a 90% source-owned ROM map

Status: `M12_AUTO2_BLOCKED_BELOW_90_NO_COMPLETE_PROVENANCE_GRAPH`.

This report records the strongest byte-exact M12 checkpoint reached without
starting M13, native gameplay/runtime C++ migration, or emulator expansion.
The result is deliberately below the requested 90% gate because the remaining
ROM bytes do not have independently closed code, data, or asset provenance.

## Identity and checkpoints

| Item | Value |
| --- | --- |
| Canonical ROM size | 3,145,728 bytes |
| Canonical CRC32 | `C4728225` |
| Canonical SHA-1 | `2944910c07c02eace98c17d78d07bef7859d386a` |
| Canonical SHA-256 | `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263` |
| Baseline Git SHA | `1215d0d8ee04f29927bb27160d7e1f39a5c6391b` |
| Final local transaction | `build/m12-auto2-consumer-transaction-d/materialized/manifest.json` |

| Checkpoint | Source-owned bytes | Percentage |
| --- | ---: | ---: |
| M12-AUTO | 279,468 | 8.884048461% |
| Screen descriptor graph | 621,220 | 19.748051961% |
| Exact Z80 upload | 629,412 | 20.008468628% |
| Consumer graph + fixed records | 718,252 | 22.832616170% |

The 90% threshold is 2,831,156 bytes; the final gap is 2,112,904 bytes.

## Final ownership census

| Class | Bytes | Percent |
| --- | ---: | ---: |
| 68000 `CODE_VERIFIED` | 48,906 | 1.554679871% |
| Header/vector ASM | 512 | 0.016276042% |
| Confirmed structured data | 14,010 | 0.445365906% |
| Confirmed alignment padding | 47 | 0.001494090% |
| Local ROM-derived assets | 654,777 | 20.814800262% |
| **SOURCE_OWNED** | **718,252** | **22.832616170%** |
| Remaining `UNKNOWN` blob | 2,427,476 | 77.167383830% |

The asset total consists of the original 107-entry compressed-resource graph,
159 screen-descriptor primary streams, 29 direct 68000 graphics streams, and
the seven streams selected by the bounded `0x03B8DE` table. The structured
data total includes the existing exact ASM-backed data plus eight fixed
1208-byte records. The Z80 8192-byte image is included in the ASM total and is
also recorded as `Z80_SOURCE_OWNED_BYTES=8192`.

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
10. Remaining ROM spans retained as canonical-local-ROM blobs.

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
| `0x03E7F4..0x060000` | 137,228 | mixed static code/data area with unresolved boundaries |
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
repository. Debug MinGW and Release MinGW full CTest each passed 83/83 after
the CMake target split; the GNU/Linux-equivalent build and the five relevant
M12/helper tests passed. The GNU full CTest reached the file-limit test but
was stopped after its mounted-NTFS scan remained active for roughly ten
minutes; the same test passed in both Windows configurations. `git diff
--check`, source-file limits, Python helper checks, and the exact manifest/hash
audit passed.
