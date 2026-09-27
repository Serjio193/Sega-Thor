# M12 native bus-event sideband W1 acceptance

Checkpoint: `PASS_NATIVE_BUS_EVENT_SIDEBAND_V1`

Scope was limited to W1 native bus-event sideband and real runtime acceptance.
No W2 classification or semantic labels were added. The canonical ROM was
`Beyond Oasis (USA).md`, SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

## Source and build

The source audit started from BizHawk `bdddf4a58aa1a022afb11dc73294a81a5aa7bbd5`
and GPGX `051d430d3d1b54625f9900c8f152d7f232e06daf`, then applied 1A, 1B, W1
sideband and observability patches into a fresh materialization at
`/home/serji/m12-w1-source-audit-20260920-d`. All required source mechanisms
were present: 68K BUS_READ/BUS_WRITE hooks, FRAME_BOUNDARY hook, domain
resolver, 32-byte event schema, ring changes, and four frame-observability
bindings. The canonical patch files are under
`tools/bizhawk-native-ring/`.

The restored toolchain was copied from the repository-supported Waterbox musl
build at `/home/serji/bizhawk-2.11.1/waterbox/sysroot`, musl commit
`2063abc4e16c84218757b1db10d3cdf9f36ef3f8`, Ubuntu-24.04 WSL2 clang
`18.1.3 (1ubuntu1)`, libcxx tag `llvmorg-18.1.8`. The installed
`waterbox/sysroot/bin/musl-clang` hash is
`9125791649f7cb2409b80f8282c20c2336e80c70f0ba820e08c4e251a78dc14f`.

Debug WBX: 4,040,568 bytes,
`D383E01B6E4C59D0C119414F45EB8C42806EBB36E6AEC8F40B867C5766DF6FC7`.
Release WBX: 5,678,048 bytes,
`9CDBC48EA4FD410924E9E5E1A82BA441DDCD87B9DD3F8E6E97C397F042EE31E2`.
Both builds passed; `nm -u` had zero unresolved symbols beyond the normal
`_DYNAMIC` entry. The exported
`gpgx_live_forward_latest_frame_boundary_record` symbol was present in both
artifacts. The managed Release host build passed with one existing NU1902
SharpCompress warning.

## Micro runtime

The real Release WBX was installed in the isolated
`C:\Dev\SegaThorTools\BizHawk-m12-w1-frame-api-20260920` install. The probe
receipt is
`build/m12-w1-acceptance/sideband-probe-frame-api-100-preroll10-1mb/receipt.json`.
After a 10-frame preroll, the API returned `FRAME_BEFORE=279050` and
`FRAME_AFTER=309196` after one capture advance (`FRAMES=11`), proving a real
FRAME_BOUNDARY record. The exported capture contained 3,290 records: 1,695
instruction records and 1,595 BUS_READ records. Stream sequences were
contiguous, every bus event had a nonzero causing instruction sequence, and
zero event records carried the FETCHED flag. The no-preroll companion probe
captured 198 BUS_WRITE records. The 128-Worker gameplay receipt below adds
32-bit and both-direction/domain coverage.

## 128-Worker gameplay acceptance

Receipt: `build/m12-w1-acceptance/campaign-128x100-final-counted/final-counted-receipt.json`.

| Measure | Result |
|---|---:|
| Workers / depth / cycles | 128 / 100 / 100 |
| Validated segments | 12,800 / 12,800 |
| Instruction records | 4,029,639 |
| BUS_READ records | 1,342,733 |
| BUS_WRITE records | 1,634,287 |
| Frame records inside bounded slices | 0 |
| Total exported records / bytes | 7,007,510 / 224,240,320 |
| 8 / 16 / 32-bit bus events | 1,390,606 / 1,067,121 / 519,293 |
| ROM / 68K RAM / Z80 window / VDP events | 6,461 / 2,914,939 / 13,824 / 41,796 |
| Fetch-flagged event records | 0 |
| Capture invalid / dropped / stale / collision | 0 / 0 / 0 / 0 |
| Ring overwrite failures | 0 |
| Ring retention failures / runtime errors | 0 / 0 |
| Shared ring wraps (not failures) | 6,852 |
| Wall / Worker wall time | 36.422 s / 29.914 s |

The real 30-second END GAME receipt is
`build/m12-w1-acceptance/campaign-128x100-endgame/continuous-128-depth100/count-128/receipt.json`.
It stopped with `STOPPED_END_GAME` after 1,663 frames and 12,544 audited
segments; each of 128 Workers completed 98 cycles, with no invalid, dropped,
stale, identity, retention, memory-limit or unsupported termination.

## Control-flow regression

The pre-W1 install and the W1 install were run against the same ROM and 100
depth-100 segments. Receipt:
`build/m12-w1-acceptance/semantic-ab/comparison.json`. After excluding EVENT
records, equality was `true`, with 100 compared segments and zero mismatches.
The comparison included instruction sequence, PC, opcode, next PC, branch
taken/not-taken flags, exceptions, terminal facts and segment entry/exit
boundaries.

## Runtime witnesses

Domains are numeric schema values: `0=ROM`, `1=68K_RAM`, `2=Z80_WINDOW`,
`3=VDP`. Values below are exact first witnesses from the earlier callback-counted
W1 receipt `build/m12-w1-acceptance/campaign-128x100-counted/counted-receipt.json`;
they are not classified as SPRITE or MUSIC.

| Event | stream_sequence | instruction_sequence | PC | address | value | width | domain |
|---|---:|---:|---:|---:|---:|---:|---|
| ROM data/operand BUS_READ | 1,740,816 | 1,155,493 | `0x456` | `0x45E` | `0x307A` | 32 | ROM |
| 68K RAM BUS_WRITE | 1,740,278 | 1,155,183 | `0x1F76` | `0xFF131E` | `0x360E` | 16 | 68K_RAM |
| VDP mapped BUS_WRITE | 3,516,096 | 2,334,292 | `0x229A` | `0xC00004` | `0x8164` | 16 | VDP |
| Z80-window BUS_WRITE | 1,740,711 | 1,155,451 | `0x60490` | `0xA00017` | `0xFF` | 8 | Z80_WINDOW |
| Z80-window BUS_READ | 1,740,696 | 1,155,440 | `0x60312` | `0xA00005` | `0x00` | 8 | Z80_WINDOW |

The known shadow-SAT RAM address `0xFF13CC` was not present in the retained
first-witness set; no shadow-SAT claim is made.

## Measured cost and ownership

The paired 100-segment W1/pre-W1 performance receipt is
`build/m12-w1-acceptance/semantic-ab/comparison.json`. W1 wall time was
11.563 s versus 11.578 s pre-W1; Worker wall time was 5.006 s in both runs,
or 19.98 validated captures/s. W1 generated 6,152,766 native records and
1,819,744 captured segment bytes; pre-W1 generated 4,080,947 native records
and 1,064,704 captured segment bytes. The W1 captured slices contained
55,967 records / 1,790,944 bytes versus 32,372 / 1,035,904 pre-W1. All
capture-failure counters were zero. Frame timing was measured, not projected:
baseline p50 17 ms, recorder p50 17 ms, Worker p50 16 ms; W1 maxima were
18/17/17 ms.

`SOURCE_OWNED` remained `1,475,600` bytes with delta `0`. No ROM, asset,
commit, push or W2 classification was added by this acceptance.
