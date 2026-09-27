# M12 W2 — Active Resource Classification V1

Checkpoint: `PASS_WORKER_ACTIVE_RESOURCE_CLASSIFICATION_V1`

This is a developer-only, worker-side analysis of immutable W1 native
sideband records. It does not change the W1 native hooks, 32-byte ring ABI,
Worker scheduling, emulator state, S1–S8 artifacts, ROM ownership, or
`SOURCE_OWNED`.

## Inputs and method

The fresh runtime used the accepted Release install
`C:\Dev\SegaThorTools\BizHawk-m12-w1-frame-api-20260920`, whose `gpgx.wbx`
SHA-256 is
`9cdbc48ea4fd410924e9e5e1a82ba441ddcd87b9dd3f8e6e97c397f042ee31e2`, and
the canonical ROM `build/m12-auto2-rom/usa/Beyond Oasis (USA).md`. One Worker
ran depth 100 for 100 audited segments. The runner reused the existing
host-side immutable segment audit callback. W2 sorted only copied rows by
`stream_sequence`; it never rewrote a W1 record.

The deterministic machine-readable output is
`build/m12-w2-acceptance/w2-active-resource-classification.json`.
Its report SHA-256 is
`994dbd3d307516ad72573c6dd6d3c5add66ff9f429ae6ea24dc4bbaef04f726a` and the
canonical sorted retained-record SHA-256 is
`ad870b8d071931d08759b38b0b46689529e1feb800e4c0e2ca14e2af71e0ed96`.

## Runtime receipt

| fact | value |
|---|---:|
| runtime outcome | `PASS` |
| audited segments | 100 |
| instruction records in retained slices | 32,272 |
| bus events | 23,595 |
| frame records in retained slices | 0 |
| retained W1 records | 55,967 |
| retained W1 bytes | 1,790,944 |
| W1 record hash | `see machine-readable report: raw_w1_records_sha256` |
| ring capacity | 4,096 |
| ring wraps | 1,502 |
| ring overwrite count | 0 |
| ring retention failures | 0 |
| capture invalid/dropped | 0 / 0 |
| runtime errors | 0 |

Measured runtime wall time was `11.484 s`; the Worker-reported wall time was
`5.006 s`.

The existing accepted W1 128-Worker campaign remains the authority for the
full 128×100 lifecycle gate, ring overwrite/retention proof, control-flow A/B
equality, and measured W1 native cost. W2 added no native capture changes.

## Primitive classes

| class | count |
|---|---:|
| `ROM_DATA_READ` | 12 |
| `RAM_READ` | 10,208 |
| `RAM_WRITE` | 12,681 |
| `Z80_WINDOW_READ` | 220 |
| `Z80_WINDOW_WRITE` | 440 |
| `VDP_CONTROL_READ` | 0 |
| `VDP_CONTROL_WRITE` | 31 |
| `VDP_DATA_READ` | 0 |
| `VDP_DATA_WRITE` | 3 |
| `YM2612_MAPPED_ACCESS` | 0 |
| `PSG_MAPPED_ACCESS` | 0 |
| `OTHER_BUS_ACCESS` | 0 |

Widths were 8-bit `9,170`, 16-bit `9,673`, and 32-bit `4,752`. Each event
retains `stream_sequence`, `instruction_sequence`, PC, address, value, width,
domain, direction, raw flags, and raw auxiliary data. Events are grouped by
their exact instruction sequence after collection because a valid W1 event
may precede its causing instruction record in stream order.

## Exact relations and bounded candidates

The run emitted 581 `DERIVED_EXACT` MOVE memory-to-memory transfer relations.
Each requires a legal decoded MOVE shape, one ordered read and write bound to
the same instruction, matching width, and matching masked value. Equal values
from unrelated instructions do not create a relation. No `HYPOTHESIS` rows
are emitted. There are 439 deterministic bounded candidates keyed by
primitive, address, width, and domain; they carry no ownership or global
frontier semantics.

## VDP and DMA decoder

Observed CPU-visible VDP activity decoded to 22 register writes, 3 complete
control commands, 3 DMA programming records, and 3 incomplete control-command
cases. The complete commands were all VRAM-targeted code `0x21` commands:

| control stream pair | target | destination | source byte address | length |
|---|---|---:|---:|---:|
| `3485073 -> 3485076` | VRAM | `0xDBC0` | `0xFF13E8` | 64 words / 128 bytes |
| `3733518 -> 3733521` | VRAM | `0x8000` | `0xFF15AC` | 3,152 words / 6,304 bytes |
| `3830745 -> 3830748` | VRAM | `0x6000` | `0xFF15AC` | 1,536 words / 3,072 bytes |

These fields are `DERIVED_EXACT` from the observed command pair and observed
register writes. Every DMA record states `transfer_observed=false` and
`CPU_PROGRAMMING_ONLY`; W1 M68K BUS_READ records are not treated as internal
DMA payload reads.

## Runtime witnesses

These are raw W1 event witnesses interpreted only by primitive class. No
sprite, music, song, or instrument label is assigned.

| witness | stream | instruction | PC | address | value | width | domain |
|---|---:|---:|---:|---:|---:|---:|---|
| ROM data/operand read | 3,565,353 | 2,368,894 | `0x03B7CC` | `0x03BD86` | `0x0000` | 16 | ROM |
| 68K RAM write | 1,740,278 | 1,155,183 | `0x001F76` | `0xFF0BDE` | `0x360E` | 16 | 68K_RAM |
| VDP mapped control write | 3,444,714 | 2,285,245 | `0x002288` | `0xC00004` | `0x8124` | 16 | VDP |
| Z80-window write | 1,740,711 | 1,155,451 | `0x060310` | `0xA00017` | `0xFF` | 8 | Z80_WINDOW |

The accepted shadow-SAT range `0xFF13CC..0xFF13FB` had no write in this
short W2 run. Consequently no shadow-SAT write witness is claimed. The
accepted S1–S8 artifacts were read only; the fresh run had no coherent shared
run/frame/range identity, so the bridge emitted `cross_links=[]`.

Z80-window writes were observed, including the witness above. No audio
handoff candidate occurred in this run, and `song_semantics` and
`instrument_semantics` remain `NOT_CLAIMED`.

## Regression boundary

The W1 checkpoint remains `PASS_NATIVE_BUS_EVENT_SIDEBAND_V1`.
`native_capture_modified_by_w2=false`, `raw_w1_record_schema_modified=false`,
`worker_scheduling_modified=false`, and `source_owned_delta=0`. The accepted
W1 semantic A/B receipt remains the evidence for instruction PCs, opcodes,
next-PC, branch outcomes, exceptions, terminal facts, and segment boundaries.

## Validation

The focused W2 test suite passed `5/5`; Python compilation passed; all new
human-maintained source/test files are below 500 lines. No commit or push was
made, and the dirty worktree was preserved.
