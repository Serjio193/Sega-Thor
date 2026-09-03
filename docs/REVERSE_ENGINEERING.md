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
**Role:** Canonical binary for original addresses, disassembly, traces, and differential verification.

| Field | Value | Confidence |
|---|---|---|
| Title | `Beyond Oasis (USA)` | CONFIRMED |
| Size | `3145728` bytes | CONFIRMED |
| CRC32 | `C4728225` | CONFIRMED |
| SHA-1 | `2944910c07c02eace98c17d78d07bef7859d386a` | CONFIRMED |
| SHA-256 | `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263` | CONFIRMED from uploaded reference |
| Archive member | `Beyond Oasis (USA).md` | CONFIRMED |
| Detector result | `SUPPORTED` | VERIFIED |

**Evidence:**
- user-provided reference ROM was fingerprinted inside GitHub Actions;
- reference-ROM workflow verifies size/SHA-1 and the project detector returns `SUPPORTED`;
- current MAME metadata independently matches size, CRC32 and SHA-1 with status `good`;
- public `smd_beyondoasis` reverse-engineering work targets `Beyond Oasis (U) [!]`.

### Europe retail — known secondary revision
**Role:** Secondary evidence/future data profile; not current address reference.

| Field | Value | Confidence |
|---|---|---|
| Title | `The Story of Thor (Europe)` | HIGH |
| Size | `3145728` bytes | HIGH |
| CRC32 | `1110B0DB` | HIGH |
| Current runtime status | `KNOWN_UNSUPPORTED` | PROJECT POLICY |

### Region policy
- USA addresses are the default notation in reverse-engineering documents.
- Region-specific offsets must not leak into native gameplay code.
- Europe/Japan may be compared to USA as secondary evidence.
- Final C++ game model is region-independent.

## `0x00003820..0x00003B3C` — graphics decompression routine
**Status:** INVESTIGATING; boundaries and major calling convention established, C++ translation not started.

**Semantic confidence:** HIGH. Public ROM-hacking work names `0x3820` `decompress_gfx`; static analysis independently shows literal runs, repeated-byte runs and sliding-window backreferences.

### Boundaries
- Entry: `0x3820` — saves `D0-D2/A2`.
- Fast/format-A return: `0x38CE`.
- Format-B return: `0x3A24`.
- Internal bitstream refill/helper branches extend through `0x3B3A`.
- `0x3B3E` starts an unrelated function with a new full-register prologue.
- Candidate routine byte range is therefore `0x3820..0x3B3C`; confidence HIGH/near-confirmed from complete contiguous disassembly.

### Direct callers
Static byte search in the verified USA ROM found **52 direct absolute `JSR 0x3820` calls** and **0 direct absolute JMPs**.

Representative call sites:
- `0x00C394`
- `0x00D3C8`
- `0x03A7FE`
- `0x03E820`

Full caller-offset list remains reproducible through `.github/workflows/m3-3820-probe.yml`.

### Calling convention
**A0 — compressed source pointer: CONFIRMED.** Representative callers load ROM addresses into A0 immediately before the call:
- `0xC386`: `A0 = 0x16943C`
- `0xD3BC`: A0 starts from table `0x5CE96`, then indexed pointer is loaded into A0
- `0x3A7F0`: `A0 = 0x16943C`
- `0x3E814`: `A0 = 0x141580`

The routine consumes bytes exclusively through `A0@+`/`A0@` in both formats.

**A1 — destination/output pointer: CONFIRMED.** Representative callers load RAM destinations:
- `0xC38C`: `A1 = 0xFF2FA8`
- `0xD3B6`: `A1 = 0xFF2FA8`
- `0x3A7F6`: `A1 = 0xFF316C`
- `0x3E81A`: `A1 = 0xFF2FA8`

The routine writes output through `A1@+`.

**A1 after return — end-of-output pointer: CONFIRMED.** At `0x3A7FC`, caller saves initial A1 in A2; after `JSR 0x3820`, instruction `SUBA.L A2,A1` computes the decompressed byte count, which is then halved for DMA length.

**A0 after return — advanced input pointer: LIKELY/HIGH.** A0 is not preserved and is incremented throughout decompression. Need a caller or dynamic trace that observes the returned value before promotion to CONFIRMED.

### Preserved registers
- Outer routine saves/restores `D0-D2/A2`.
- Format-B path additionally saves/restores `D3/D6/D7`.
- A0/A1 are intentionally not preserved.

### Format dispatch
At `0x3824`, the routine tests byte `source[2]`:
- nonzero → path beginning `0x382C`;
- zero → path beginning `0x38D0`.

Do not assign external algorithm names until evidence supports them.

### Path A: `0x382C..0x38CE`
Observed behaviors:
- compressed block end is calculated from the first two source bytes relative to the current source base;
- literal run copies input bytes to output;
- repeated-byte run reads one input byte and writes it repeatedly;
- backreference run copies from already-produced output (`A2 = A1 - distance`);
- block terminator at `0x38C4` either starts another block or returns.

### Path B: `0x38D0..0x3B3C`
Observed behaviors:
- begins after skipping/consuming header bytes;
- uses a control-bit stream in D7 with D6 as remaining-bit counter;
- literal case at `0x3986` copies `A0@+ -> A1@+`;
- multiple backreference forms calculate distance/length and copy from previous output;
- helper branches `0x3A26..0x3B3A` refill control bits from source;
- terminator at `0x3A16` either begins another block or restores registers and returns at `0x3A24`.

### Callees
No `JSR`/`BSR` occurs inside the candidate routine range. The decompressor is self-contained aside from memory reads/writes.

### Memory contract
- Reads compressed stream from memory addressed by A0.
- Writes sequentially to memory addressed by A1.
- Backreferences read from already-written destination bytes behind A1.
- No hardware-register accesses observed inside the routine.

### Remaining proof before C++ translation
1. obtain at least one independent dynamic execution trace of original routine bytes;
2. record source consumed, output length and output hash for representative call(s), preferably one case from each format;
3. confirm A0 returned advanced;
4. use those traces as regression fixtures;
5. only then implement native C++ translation.

## Existing translated compatibility behavior

### Tile copy to work RAM
Initial C++ compatibility implementation exists, derived from the public `tilecopy_to_ram` macro. Revisit after data interfaces stabilize.

### Tile copy to VRAM
Initial C++ compatibility implementation exists, derived from the public `tilecopy_to_vram` macro. Current VDP model is intentionally minimal.

## ROM identification implementation
**Status:** VERIFIED.

Detector records byte size, Mega Drive header, Sega checksum, CRC32, SHA-1, SHA-256 and classification. Synthetic tests contain no original ROM bytes.
