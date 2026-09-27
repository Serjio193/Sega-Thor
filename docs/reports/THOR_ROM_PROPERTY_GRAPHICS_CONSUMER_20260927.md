# Live compressed-graphics consumer classification — 2026-09-27

**Status:** `PASS_SEAL_REPLAY`; overall classification remains partial.
The hook adds a proof class for the compressed input actually consumed by the
verified live `0x3820` decoder. It does not label compressed input as direct
VRAM data.

## Contract

At each M68K instruction boundary, the runtime recognizes only exact entry PC
`0x3820` and records `A0`. At the decoder's completed `RTS`, it reads the
returned `A0` and accepts only a nonempty span no larger than 64 KiB. Every
logical source byte must resolve to the next contiguous physical ROM offset.
An exception, non-ROM mapping, discontinuity, oversized span, other entry PC,
or incomplete call receives no property. A successful call marks exactly the
consumed interval as `COMPRESSED_GRAPHICS_SOURCE`.

The proof contract binds the exact decoder boundary, the original-ROM
`A0`-advance postcondition, and the local decoder implementation hashes. No
output-buffer origin is inferred, and no VDP-source property is added by this
contract.

## Sealed run

| Identity | Value |
|---|---|
| ROM SHA-256 | `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263` |
| ROM size | 3,145,728 bytes |
| Runtime/build ID | `89064cd571a5e96dc262aac848cf9a18d40b1011b071c8352e1147e651079f14` |
| Proof-contract SHA-256 | `c88eb4dcc273b55681bc1d0fc04e483b4d200d292b054488f7c87348b2c07842` |
| Schema / capabilities | `thor.rom-properties.v1` / `511` |
| Run ID | `consumer-graphics-3820-20260927-3600-01` |
| Frames | 3,600 deterministic menu/directional input |
| Checkpoint SHA-256 | `5a87455d0a240654812d7009689ba8adf281e6e422f4c545738212dadf605208` |

Both GPGX Release and Debug Waterbox builds succeeded. The corresponding core
hashes are in `build/runtime-rom-properties-consumer-graphics/runtime-build-manifest.json`.

## Replay and coverage

Range export has 2,375 ranges covering the full ROM, with zero gaps and
overlaps. Bitmap → ranges → bitmap is exact. The canonical overlay has 4,646
ranges, preserves hash
`a901cde397632b9b5153682f683fbab281358883c6bedd0e8da3e3312ae8bc13`, keeps
`SOURCE_OWNED` at 1,487,672 bytes (delta zero), and reports zero unauthorized
promotions.

| Metric | Before (audio checkpoint) | After (graphics checkpoint) | Delta |
|---|---:|---:|---:|
| Observed coverage | 3,145,226 | 3,145,226 | 0 |
| Classified coverage | 181,050 | 183,289 | +2,239 |
| Unknown | 502 | 502 | 0 |
| `COMPRESSED_GRAPHICS_SOURCE` bytes | 0 | 2,239 | +2,239 |

The original largest observed-only interval `[0x158296,0x261124)` intersected
the new proven inputs by 1,453 bytes. It remains a large unresolved region.
The largest remaining contiguous observed-unclassified spans in this run are:

| ROM span, half-open | Bytes |
|---|---:|
| `[0x180C9F,0x261124)` | 918,661 |
| `[0x27A974,0x300000)` | 546,444 |
| `[0x062B1C,0x0BD540)` | 371,236 |
| `[0x03C9CC,0x060000)` | 144,948 |
| `[0x11D9E0,0x13AD1C)` | 119,612 |

These are bitmap runs, not semantic resource boundaries. The fresh run proves
only the seven exact decoder input spans in its property bitmap; no bytes were
promoted from static resource membership alone.

## Parity and performance

Neutral-input traces matched the previous instrumented build at 120, 1,200,
and 3,600 frames. At 3,600 frames the final state markers are
`m68k_ram_fnv1a=0c0b556e` and `z80_bus_0000_1fff_fnv1a=e6a598fa`. This checks
the runtime hook's lack of behavioral side effects under neutral input; it is
not full-game behavioral parity.

Three-repeat Release timing used the same host, ROM, cold-boot start, neutral
input, 1,200 frames, disabled VSync/throttling, and normalized emulator
settings:

| Mode | Mean FPS | Sample SD | Runs |
|---|---:|---:|---|
| B — instrumented, map off | 912.003 | 42.645 | 960.000; 878.477; 897.532 |
| C — instrumented, map on | 738.373 | 15.316 | 746.269; 748.130; 720.721 |

The measured B→C decrease is 19.04%. A clean baseline was not measured: both
clean-core installations timed out after `Calling _start()`. Therefore clean
instrumentation overhead (A→B) and total overhead (A→C) remain unknown. No
optimization was attempted.

## Next proof gap

The completed live decoder contract raises unique classified coverage by
2,239 bytes, but does not materially shrink the large observed-only regions.
The current deterministic scenario reaches only seven compressed-resource
calls. More input time on this same scenario has no demonstrated value. The
next useful capture must exercise a different known graphics-loading scene or
consumer path while keeping the run bounded. RAM-copy provenance, CRAM/VSRAM,
Z80 execution origin, and most of the largest observed-only range remain open.
