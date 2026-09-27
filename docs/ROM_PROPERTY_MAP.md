# Runtime ROM Property Map

## Purpose and authority

The runtime property layer stores independently proven facts about how
physical ROM bytes were used. Its in-memory authority is a dense
`uint16_t[rom_size]`; a zero mask means no property has been proven. Zero does
not mean unused. This layer is observational evidence and does not replace the
canonical emission partition or authorize `SOURCE_OWNED` changes.

The current schema is `thor.rom-properties.v1`. Its proof contract is the
byte-exact `runtime_rom_properties_contract.json`, SHA-256
`c88eb4dcc273b55681bc1d0fc04e483b4d200d292b054488f7c87348b2c07842`.
Checkpoints bind the map to the ROM SHA-256 and size, schema, proof-contract
hash, exact core/build identity, run ID, generation, capability mask, and validation state. The versioned binary
payload is little-endian and protected by SHA-256. A checkpoint is written to
a sibling temporary file, flushed, and atomically replaced. No checkpoint,
JSON, SQLite, allocation, or disk access belongs in a runtime observation hook.

## Property contracts

Bits are independent and may coexist. A classifier may set a bit only when all
positive conditions hold. Missing, ambiguous, or contradictory evidence leaves
the bit unset.

| Bit | Property | Positive proof | Must reject |
|---:|---|---|---|
| 0 | `M68K_EXECUTED_ENCODING` | A completed 68000 instruction has an exact encoding span from the pinned decoder contract; every byte maps to this physical ROM | PC-only coverage, opcode-only capture, prefetch alone, `next_pc - pc`, exception entry without completed instruction, uncertain mapping or incomplete decode |
| 1 | `Z80_EXECUTED_ENCODING` | A completed Z80 instruction has every prefix/opcode/operand byte recorded with origin proven byte-by-byte to physical ROM | PC delta, RAM fetch without retained exact ROM origin, unsupported/repeated prefix overflow, ambiguous bank or incomplete fetch |
| 2 | `M68K_DATA_READ` | A completed CPU data read resolves to a bounded physical ROM byte span | Instruction fetch, I/O/unmapped reads, bus address without backing-store proof, invalid width or wrap ambiguity |
| 3 | `Z80_DATA_READ` | A completed Z80 data read resolves to physical ROM bytes through the current bank mapping | Z80 logical address alone, Z80 RAM, I/O, or a handler result without a ROM backing identity |
| 4 | `VDP_VRAM_SOURCE` | Exact ROM bytes are consumed by accepted VRAM DMA writes, or by a completed same-instruction `MOVE.W` from one exact ROM word to an accepted VDP data-port write | DMA command alone, fill, VRAM copy, register/immediate/transformed source, multiple data reads, mismatched value, incomplete instruction, or unconsumed bytes |
| 5 | `VDP_CRAM_SOURCE` | Exact ROM bytes are consumed by accepted CRAM DMA writes, or by a completed same-instruction `MOVE.W` from one exact ROM word to an accepted VDP data-port write | Inferred color semantics, register/immediate/transformed source, multiple data reads, mismatched value, incomplete instruction, or unconsumed bytes |
| 6 | `VDP_VSRAM_SOURCE` | Exact ROM bytes are consumed by accepted VSRAM DMA writes, or by a completed same-instruction `MOVE.W` from one exact ROM word to an accepted VDP data-port write | Incomplete command, unresolved source, register/immediate/transformed source, multiple data reads, mismatched value, incomplete instruction, or unconsumed bytes |
| 7 | `AUDIO_PAYLOAD_PROVEN` | A versioned Thor consumer contract proves that exact ROM bytes supply payload consumed by the sample decoder or another explicitly accepted payload reader | Sound-bank membership, arbitrary Z80 read, descriptor/control parameter, driver execution, or temporal proximity to an audio sink |
| 8 | `COMPRESSED_GRAPHICS_SOURCE` | Exact ROM bytes in the `A0` input extent of a completed call to the verified `0x3820` graphics decompressor, with contiguous byte-exact ROM origin | Different entry point, incomplete call, empty/oversized span, non-ROM or discontinuous origin, or inference of direct VRAM semantics |

The implemented Format-A mode-0 audio contract accepts only exact banked-ROM
bytes read at Z80 PCs `0x080E` or `0x0855` inside the independently verified
resource spans `[0x0BC95C,0x0BD540)` and `[0x0BD540,0x0BF768)`. It marks only
the byte read at that runtime event. The W5 decoder/inverse-encoder roundtrip
and W4 YM2612 DAC witness establish the resource and consumer; descriptors,
other decoder PCs, and neighboring bank data remain unclassified.

The `0x3820` graphics contract records `A0` on entry and only marks
`[A0_entry,A0_return)` after the decoder returns via `RTS`. It checks every
source byte against contiguous physical ROM mapping and caps one resource at
64 KiB. The result means compressed graphics input was consumed; it does not
claim that those bytes were copied directly to VRAM or that every decoded
output byte was displayed.

These labels mean source bytes participated in a proven operation. They do not
claim that every source bit survived byte swap, hardware masks, conversion, or
consumer decoding unchanged. A future UI may display broader labels such as
CODE, VIDEO, PALETTE, or AUDIO without changing these proof bits.

## ROM physical offset resolver

A resolver returns either `(ROM_SHA256, physical_offset, span_length)` or
`UNRESOLVED`. It must check the active address map for every byte, reject
handler-backed/non-ROM memory, account for bank state at each access, and split
spans at mapping boundaries. Arithmetic must be overflow checked and bounded
by `rom_size`. A numerically in-range CPU address is not sufficient evidence
of ROM backing. A copied byte retains ROM origin only through an explicitly
supported copy operation; any unsupported write or transformation clears it.

## Core and merge compatibility

`core_build_id` is the SHA-256 of a canonical build manifest containing the
emulator source revision, integration patch digest, built core binary digest,
compiler/toolchain identity, and build flags. Merges require exact equality of
ROM hash/size, schema ID, proof-contract hash, core/build ID, capability mask,
and validation state. Run ID and generation
identify a contribution and therefore differ across compatible runs. There is
no implicit compatibility between emulator revisions. An explicit migration
must validate the old and new proof contracts and emit a new contribution.

Each run remains a separately saved contribution. A union is derived by
replaying the selected contribution files, so an invalid run can be excluded
and the union rebuilt. Property OR is valid only after identity and payload
validation. No semantic inference is performed by merge.

## Current implementation boundary

The reusable map, dirty-page tracking, checked range roundtrip, checkpoint
codec, atomic checkpoint save, strict identity validation, contribution merge,
hex-dump import, and range export are implemented. The low-level
`apply_validated` operation trusts its caller; proof remains the responsibility
of the runtime adapters.

The pinned GPGX runtime now has exact M68K consumed-prefetch encoding capture,
direct M68K data-read resolution with byte-for-byte backing checks, actual Z80
opcode/operand fetch capture, direct Z80 bank-read observation, and direct
68K-bus-to-VDP DMA source hooks after `vdp_bus_w`. A consumer-side Mode 5 hook
also accepts only a completed same-instruction M68K `MOVE.W` with one exact
ROM-backed word read whose value reaches the VDP data port unchanged. The VDP
consumer supplies the accepted VRAM/CRAM/VSRAM destination code; fill-trigger
writes are rejected. Runtime acceptance remains scenario-limited. VDP DMA
source bits are accepted in the 10,000-frame controlled run; direct CPU-port
source acceptance is tracked separately in the consumer-hook iteration report.
Z80 RAM-copy provenance, indirect register source, compressed-output VDP
provenance, and audio remain outside this direct path.

The live map is dumped outside the CPU hot path through bounded Lua chunks.
Checkpoint creation, range export and canonical overlay run on the host. The
corrected 2026-09-27 run `corrected-live-20260927-120f-01` has a fresh manifest
and checkpoint after the Z80 `ARG16()` PC-advance repair. It round-trips all
3,145,728 bytes with 0 gaps and 0 overlaps; canonical overlay preserves
`SOURCE_OWNED` at 1,487,672 bytes (delta 0), with no canonical promotions.
See `docs/reports/THOR_RUNTIME_ROM_PROPERTIES_RESEAL_20260927.md` for the
identity, checkpoint checksum, parity, performance, and remaining acceptance
work.

A separate 3,600-frame controlled-input run on the same corrected runtime
observed `Z80_DATA_READ` on 40,182 bytes and direct ROM-backed
`VDP_VRAM_SOURCE` on 129,984 bytes. It has its own checkpoint and exact range
replay; the canonical overlay again has zero ownership delta. Z80 executed
encoding, VDP CRAM/VSRAM source, and live RAM-copy provenance remain unproven.

A follow-up 10,000-frame run with the same deterministic menu/directional input
schedule retained the same 502 UNKNOWN bytes and full bitmap coverage, while
expanding the existing accepted properties: 56,426 bytes carry
`M68K_EXECUTED_ENCODING`, 76,009 carry `Z80_DATA_READ`, and 254,912 carry
`VDP_VRAM_SOURCE`. Its 2,468-range export round-trips exactly; the canonical
overlay has zero gaps, overlaps, ownership delta, or unauthorized promotions.
The run added no total covered bytes relative to the 3,600-frame run, but added
139,032 bytes with a proven class. It still proves no Z80 executed encoding,
CRAM/VSRAM source, or audio payload. The independent acceptance record is
`docs/reports/THOR_RUNTIME_ROM_PROPERTIES_EXPANSION_20260927.md`.

## Pinned runtime build environment

The Waterbox compiler is available in the existing WSL2 checkout at
`/home/serji/bizhawk-2.11.1/waterbox/sysroot/bin/musl-clang`. Its musl
submodule is pinned to `2063abc4e16c84218757b1db10d3cdf9f36ef3f8`; the wrapper
SHA-256 is
`9125791649f7cb2409b80f8282c20c2336e80c70f0ba820e08c4e251a78dc14f`. BizHawk
is pinned at `bdddf4a58aa1a022afb11dc73294a81a5aa7bbd5`, GPGX at
`051d430d3d1b54625f9900c8f152d7f232e06daf`, and the compiler is Ubuntu clang
18.1.3 with libcxx `llvmorg-18.1.8`.

On 2026-09-27, the prior core artifact (SHA-256
`9973dac167e69a8d444a71fa4e4f2332fddae1edc3ac37df7364ee0c91f9fa23`) was
rebuilt with property hooks. The initial Release artifact and build manifest
were superseded after parity testing exposed the Z80 `ARG16()` PC-advance bug.
The corrected Debug and Release artifacts are pinned in the fresh manifest;
its build ID is `e202add134725f7460c2a67d866d0b2e1c97ba5f9175ca5b4f76a4e961640889`.
The fresh checkpoint/replay passes, while overall property acceptance remains
partial pending live Z80/VDP paths, provenance, and broader gameplay parity.

## Read-only ROM Coverage GUI

Run `python tools/bizhawk-native-ring/rom_coverage_gui.py <run-checkpoint>`
to open the Defraggler-style block view. Pass `--canonical <union-checkpoint>`
to compare an accepted property union, or choose a canonical union in the UI.
The overlay-receipt button accepts an existing overlay JSON only with the exact
checkpoint whose checksum and identities it records. The optional ROM-name
picker verifies the selected file SHA-256 before displaying its filename; a
checkpoint by itself contains only the ROM digest.

The whole-ROM view maps 1 KiB cells in strict ascending-address row order.
Click drills into 32-byte cells, then byte cells; each range is half-open
internally and displayed with inclusive endpoints in hover details.
Per-property counts can overlap. Total and unknown coverage use bitmap union
truth (`mask != 0` and `mask == 0`), never summed property percentages. Cells
with only M68K or Z80 data-read observations retain distinct observed colors,
while their cell status is `OBSERVED_UNCLASSIFIED`. A cell with both read types
uses MIXED color but remains observed-only in its status. A single proven class keeps its class
color even when raw data-read observations overlap it; multiple proven classes
in a cell use MIXED. `CLASSIFIED` counts bytes with any executed-encoding, VDP-source, or
audio-payload bit. `OBSERVED_UNCLASSIFIED` counts nonzero masks with none of
those bits. This is a GUI presentation grouping; it does not change the bitmap,
schema, proof contract, or runtime interpretation. Tooltip property counts
still show every overlapping bit separately.

Run delta is shown only when both the current checkpoint and a compatible
canonical union are loaded. Without that baseline, the GUI reports the delta
as unavailable instead of treating all current coverage as newly discovered.
It also reports newly classified bytes separately, so a run can show class
refinement even when total covered bytes do not increase.
The legend wraps across rows so every state remains visible at the default
window width.

LIVE mode checks for atomic checkpoint replacement every 500 ms and decodes
changed snapshots on a background reader thread. It reads no runtime shared
memory. Current GPGX capture writes its checkpoint after a bounded capture, so
live refresh requires an existing process to replace the checkpoint. The GUI
cannot modify the property bitmap, canonical map, or ownership.
