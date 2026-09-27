# Consumer-driven ROM classification — 2026-09-27

**Status:** Partial; exact audio-consumer contract accepted for this live run.
**ROM:** Beyond Oasis USA, 3,145,728 bytes,
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
**Scope:** Add only properties proved at live consumers. Canonical ownership
and the read-only GUI remain unchanged.

## Iteration 1 — direct CPU-to-VDP writes

The hook accepts only one byte-exact ROM-backed word forwarded unchanged by a
completed `MOVE.W` to an accepted Mode 5 VDP data path. It rejects fills and
unproven register or transformed sources. The fresh 3,600-frame run did not
add bytes beyond existing ROM-backed DMA evidence. Classified coverage for
that checkpoint stayed at 172,306 bytes. No property contract was weakened.

## Iteration 2 — Format-A audio consumer

W4 ties Z80 decoder PCs `0x080E` and `0x0855` to the live YM2612 DAC path. W5
independently verifies two Format-A mode-0 resources by deterministic decode
and byte-identical inverse-encode:

| Resource | Exact ROM range |
|---|---|
| Primary | `[0x0BC95C, 0x0BD540)` |
| Secondary | `[0x0BD540, 0x0BF768)` |

The runtime marks only a byte actually read at either approved PC inside those
ranges. It does not promote a sound bank, descriptor, or neighboring byte.
The fresh run read all 8,744 resource bytes, so `AUDIO_PAYLOAD_PROVEN` gained
8,744 bytes. Under the existing GUI KPI, which excludes raw M68K/Z80 read
observations, unique classified coverage rose from 172,306 to 181,050 bytes
(+8,744). Bitmap coverage did not change.

## Fresh run and replay

| Identity | Value |
|---|---|
| Runtime/build ID | `443d5034c2c6f969b1e0ac31915b60323633ad566139a9826d4ee44b4b8c9066` |
| Proof-contract SHA-256 | `a67fbfcd04ebf01bce28516ae602730db4833e0207a160ff76196e78d04c096d` |
| Run ID | `consumer-audio-format-a-20260927-3600-01` |
| Checkpoint SHA-256 | `166479ca950c7152bb261a27a736086116466e19dd9daacd1d11d42500237cc0` |
| Capabilities | `255` |

The replay covers the full ROM in 2,362 ranges with zero gaps and overlaps;
bitmap → ranges → bitmap is exact. The canonical overlay has 4,643 ranges.
Canonical hash stayed
`a901cde397632b9b5153682f683fbab281358883c6bedd0e8da3e3312ae8bc13`;
`SOURCE_OWNED` delta is zero and unauthorized promotions are zero. The
overlay is read-only.

Observed coverage is 3,145,226 / 3,145,728 bytes (99.984042%); 502 bytes remain
unknown. Classified coverage is 181,050 bytes (5.7554%). The largest remaining
observed-unclassified span is `[0x158296, 0x261124)` (1,085,070 bytes). Other
largest spans in this checkpoint are `[0x27A974, 0x300000)` (546,444),
`[0x062B1C, 0x0BD540)` (371,236), `[0x03C9CC, 0x060000)` (144,948), and
`[0x11D9E0, 0x13AD1C)` (119,612). These are byte-map spans, not assertions
that each span is one game resource.

## Parity and performance

Neutral-input per-frame traces matched between the previous direct-VDP build
and the audio-enabled build at 120, 1,200, and 3,600 frames. RAM markers and
critical CPU state matched in each trace. This is a bounded runtime regression
check, not full-game behavioral parity and not clean-baseline parity.

The clean-core benchmark could not be repeated: both the clean standard install
and the separate clean-core clone hung after Waterbox printed `Calling _start()`
and hit the 60-second timeout. The completed three-repeat experiment used the
previous instrumented build for its A mode, so its A→B and A→C values are not
clean-baseline overhead and are not accepted. In that run, the within-build
map-off/map-on means were 840.985 / 690.439 FPS (17.90% decrease); the map-off
sample standard deviation was 112.627 FPS, so this is preliminary and noisy.
No optimization was attempted. A reproducible clean A/B/C measurement remains
open.

## Remaining work

The audio hook adds useful semantic coverage but does not reduce the largest
observed-only span. Direct CPU-to-VDP acceptance also produced no new bytes in
its scenario. The next high-value contract is the verified `0x3820` graphics
decoder: prove the exact live ROM source extent consumed by a completed call,
then add a compressed-graphics property rather than mislabeling compressed
input as direct VRAM data. CRAM/VSRAM, live exact-copy provenance, and
Z80-executed origin remain unproven. The 3600-frame capture was sufficient for
this iteration; no duration extension was made.

Artifacts: `build/runtime-rom-properties-consumer-audio/`, including
`replay-verification.json` and the fresh checkpoint. The clean-core timeout and
raw parity/performance receipts are under its `parity-performance/` folder.

**Continuation:** The later graphics decoder contract adds another 2,239
classified bytes and has a new proof-contract identity. Its fresh checkpoint,
overlay, largest remaining ranges, and current parity/timing are recorded in
`THOR_ROM_PROPERTY_GRAPHICS_CONSUMER_20260927.md`; this earlier audio checkpoint
cannot be merged with it because the contract hashes differ.
