# Runtime ROM Properties — controlled-input expansion

**Date:** 2026-09-27
**Status:** `PASS_SEAL_REPLAY`; classifier expansion remains partial
**Scope:** Extend the accepted corrected-runtime evidence with a longer
deterministic input run. No runtime source, property bit, schema, proof
contract, or canonical ownership was changed.

## Sealed identity

| Identity | Value |
|---|---|
| ROM SHA-256 | `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263` |
| ROM size | 3,145,728 bytes |
| Runtime/build ID | `e202add134725f7460c2a67d866d0b2e1c97ba5f9175ca5b4f76a4e961640889` |
| Release core SHA-256 | `dcd39613aef68a945f29927ead467326f6ac892d92f924466ee6f4b5f3b0f630` |
| Classifier schema | `thor.rom-properties.v1` |
| Proof contract SHA-256 | `10da481265de188d7a11049f34d24dd33f3b0e34b5ff797d12541ee241356c78` |
| Run ID | `scenario-menu-start-directional-20260927-10000-01` |
| Frames / capabilities / state | 10,000 / 255 / `PARTIAL` |
| Checkpoint SHA-256 | `80c30f8f19f16b40eaeef34581f12208201bca7e53c9952e7493879372aedff2c` |
| Input-script SHA-256 | `293f732e27b807678730e734e0ea77b8ea82a4a016532325332e4f7605c2e304` |

The run used the corrected Release core and the same deterministic schedule as
`menu-start-directional-v1`: periodic Start/A/C inputs during the first 1,200
frames followed by the eight-phase directional/A/B/C pattern.

## Replay and overlay

| Check | Result |
|---|---|
| Range export | PASS, 2,468 ranges |
| Address partition | Full ROM, 0 gaps, 0 overlaps |
| Bitmap → ranges → bitmap | Exact |
| Canonical overlay | PASS, 4,732 ranges |
| Canonical hash before / after | `a901cde397632b9b5153682f683fbab281358883c6bedd0e8da3e3312ae8bc13` / same |
| `SOURCE_OWNED` before / after | 1,487,672 / 1,487,672 bytes |
| Ownership delta / unauthorized promotions | 0 / 0 |

The overlay receipt is paired with the exact checkpoint checksum. The overlay
was computed as a read-only split of canonical and property boundaries; no
canonical map or ownership record was written.

## Proven properties observed

| Mask | Bytes | Properties |
|---:|---:|---|
| `0x00` | 502 | UNKNOWN |
| `0x04` | 2,757,879 | M68K_DATA_READ |
| `0x05` | 56,426 | M68K_DATA_READ + M68K_EXECUTED_ENCODING |
| `0x0C` | 76,009 | M68K_DATA_READ + Z80_DATA_READ |
| `0x14` | 254,912 | M68K_DATA_READ + VDP_VRAM_SOURCE |

Total bitmap coverage is 3,145,226 / 3,145,728 bytes (99.984042%). The GUI
presentation groups 311,338 bytes (9.897168%) as classified and 2,833,888
bytes (90.086873%) as observed but not yet classified. The 502 zero-mask
bytes (0.015958%) remain UNKNOWN; zero does not mean unused.

Compared with the accepted 3,600-frame checkpoint, this run adds no newly
covered bytes because both runs already observed the same bitmap union. It
adds 139,032 bytes with at least one proven class. The larger read footprint
does not establish semantic ownership or payload type.

## Observation-only range triage

The largest exact `M68K_DATA_READ`-only ranges in this run are:

| ROM range | Bytes |
|---|---:|
| `0x158296–0x261123` | 1,085,070 |
| `0x28CF3A–0x2FFFFF` | 471,238 |
| `0x03C9CC–0x05FFFF` | 144,948 |
| `0x062B1C–0x080009` | 120,046 |
| `0x11D9E0–0x139C9B` | 115,388 |

These contiguous read spans are prioritization evidence only. They do not prove
that the bytes are code, graphics, audio, or one semantic object. Follow-up
work should connect each span to a closed consumer operation or completed
instruction proof before adding a class.

## Remaining proof gaps

- No `Z80_EXECUTED_ENCODING` bytes were accepted. The run proves banked Z80
  data reads only; fetched bytes without exact ROM origin remain unclassified.
- No `VDP_CRAM_SOURCE` or `VDP_VSRAM_SOURCE` bytes were observed.
- Live RAM-copy provenance remains unaccepted. Only exact copies may retain
  origin; unsupported transformations must remain UNKNOWN.
- No `AUDIO_PAYLOAD_PROVEN` bytes were observed. Audio remains outside the
  base architecture completion gate.
- This is controlled-input runtime evidence, not full-game behavioral parity.
  The prior bounded parity/performance results remain in
  `THOR_RUNTIME_ROM_PROPERTIES_RESEAL_20260927.md`; this run changed no runtime
  code and did not repeat those measurements.
