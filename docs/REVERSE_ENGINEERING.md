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

## M8 — player input and movement slice
**Status:** IMPLEMENTED as a portable slice; full animation/entity callback semantics remain INVESTIGATING.

### Player entity selection
- `0x0013D6..0x00142E` initializes the main entity at work RAM `0xFF19E8` and writes entity type `2` before calling `0x8D06` — **CONFIRMED**.
- The main entity pool uses 21 records with stride `0xBC`; `0x008EB2..0x008ED0` iterates that pool — **CONFIRMED**.

### Controller normalization and direction mapping
- `0x00217C..0x002188` calls the controller reader at `0x2992`; normalized port state is stored beginning at `0xFF165C`, with the movement nibble in `0xFF165E` — **CONFIRMED**.
- `0x0085E2` masks `0xFF165E` with `0x0F`, dispatches through the table at `0x85FA`, and returns direction plus fixed-point deltas — **CONFIRMED**.
- Cardinal vectors are `(+0x36000,0)`, `(-0x36000,0)`, `(0,+0x30000)`, `(0,-0x30000)`; diagonal vectors are `(+/-0x2A000,+/-0x25800)` — **CONFIRMED** from `0x8624..0x86AE`.
- The native mapping preserves the original low-nibble behavior, including opposite-direction collapse and diagonal combinations; unsupported combinations resolve to no movement — **VERIFIED by synthetic tests**.

### Movement and collision contract
- `0x008F12..0x00938C` is the shared active-entity movement update; main-pool records enter at `0x8F22` — **CONFIRMED**.
- X/Y fixed-point deltas are accumulated in entity fields `+0x72/+0x76`; integer positions are committed to `+0x08/+0x0C` — **CONFIRMED**.
- `0x009BF2` aggregates the entity footprint, and `0x00938E` is called before an axis commit; carry set takes the blocked path — **CONFIRMED**.
- Native `PlayerState::try_move` reuses `ByteGridView::aggregate_world_square` and `evaluate_terrain_gate`. Rendering, animation scripts and the unknown entity callback at `+0x22` are intentionally outside this slice.

## Existing translated compatibility behavior

### Tile copy to work RAM
Initial C++ compatibility implementation exists, derived from the public `tilecopy_to_ram` macro. Revisit after data interfaces stabilize.

### Tile copy to VRAM
Initial C++ compatibility implementation exists, derived from the public `tilecopy_to_vram` macro. Current VDP model is intentionally narrow.

## ROM identification implementation
**Status:** VERIFIED.

Detector records byte size, Mega Drive header, Sega checksum, CRC32, SHA-1, SHA-256 and classification. Synthetic tests contain no original ROM bytes.
