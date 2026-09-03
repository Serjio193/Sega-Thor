# Reverse-Engineering Ledger

This file records what is known about the original Beyond Oasis binary. Do not promote guesses to facts without evidence.

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

**Semantic confidence:** CONFIRMED for decompression behavior and call contract. Public work labels the entry `decompress_gfx`; independent static and dynamic analysis establishes the implementation details below.

### Boundaries
- Entry: `0x3820`.
- Format-A return: `0x38CE`.
- Format-B return: `0x3A24`.
- Internal bit-refill branches extend through `0x3B3A`.
- `0x3B3E` begins an unrelated function with a new register-save prologue.
- Exact half-open routine range: `[0x3820, 0x3B3E)`.

### Direct callers
Verified USA ROM contains **52 direct absolute `JSR 0x3820` calls** and **0 direct absolute JMPs**. Representative callers include `0x00C394`, `0x00D3C8`, `0x03A7FE`, and `0x03E820`.

### Calling convention
- `A0`: compressed source pointer — CONFIRMED.
- `A1`: destination pointer — CONFIRMED.
- On return, `A0` points immediately after consumed compressed data — CONFIRMED by dynamic trace.
- On return, `A1` points immediately after decompressed output — CONFIRMED statically by caller `0x3A7FE` and dynamically by trace.
- `D0-D2/A2` are preserved by the outer routine.
- Format B additionally preserves `D3/D6/D7`.
- No hardware-register access or nested subroutine call occurs inside the decompressor.

### Format dispatch
At `0x3824`, byte `source[2]` selects the format:
- nonzero: command-stream format A;
- zero: bitstream format B.

### Format A
Observed command families:
- literal byte runs;
- repeated-byte runs;
- sliding-window backreferences into already-produced output;
- chained `0b011xxxxx` extensions that continue the preceding backreference;
- block-length framing and a byte terminator deciding whether another block follows.

The low 5 bits of a chained extension encode its copy count; zero behaves as 256 because the original uses byte decrement followed by word-sized DBF semantics.

### Format B
Observed behavior:
- each block begins with three skipped header bytes followed by an LSB-first control bit stream;
- literal tokens copy one source byte;
- several distance/length backreference forms copy previous output;
- distance `1` is a special repeated-byte run;
- distance `0` ends the current compressed block, followed by a byte marker deciding whether another block follows;
- control bits refill from two source bytes when exhausted.

### Native mapping
Implementation:
- `src/game/graphics_decompress.hpp`
- `src/game/graphics_decompress.cpp`

Tests:
- `tests/graphics_decompress_test.cpp` — synthetic literals, RLE, backreferences, extension chains and bitstream cases;
- `tests/graphics_decompress_reference.cpp` — local verified-ROM differential verifier.

### Original 68000 oracle traces
The exact original bytes `[0x3820,0x3B3E)` were executed in an isolated M68K Linux/QEMU harness. This is reference verification only; final runtime does not execute original 68000 code.

| Format | Caller | ROM source | Source consumed | Output bytes | Output SHA-256 |
|---|---:|---:|---:|---:|---|
| A | `0x00C394` | `0x16943C` | 1217 | 3072 | `65e99e74020fedbdcb97c8249a5ccfe540aca5bb5d29bfb260352cd6f388c31a` |
| B | `0x03C276` | `0x1894EA` | 112 | 128 | `167d4e5409f6b075b3b6f2bc61dbb747e8d8c857e8699745184ddf48d83bcda9` |

The native C++ implementation matches both traces exactly on source bytes consumed, output size, and output SHA-256. ROM-backed GitHub Actions verification and ordinary CTest/CI are green.

## Existing translated compatibility behavior

### Tile copy to work RAM
Initial C++ compatibility implementation exists, derived from the public `tilecopy_to_ram` macro. Revisit after data interfaces stabilize.

### Tile copy to VRAM
Initial C++ compatibility implementation exists, derived from the public `tilecopy_to_vram` macro. Current VDP model is intentionally minimal.

## ROM identification implementation
**Status:** VERIFIED.

Detector records byte size, Mega Drive header, Sega checksum, CRC32, SHA-1, SHA-256 and classification. Synthetic tests contain no original ROM bytes.
