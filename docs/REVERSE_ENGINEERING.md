# Reverse-Engineering Ledger

This file records what is known about the original Beyond Oasis binary. Do not promote guesses to facts without evidence.

## Known hardware addresses
| Address | Meaning | Confidence | Evidence/status |
|---|---|---:|---|
| `0x00C00000` | Mega Drive VDP data port | CONFIRMED | Present in public ROM-hacking constants and standard hardware mapping |
| `0x00C00004` | Mega Drive VDP control port | CONFIRMED | Present in public ROM-hacking constants and standard hardware mapping |
| `0x00FF0000` | 68000 work RAM base | CONFIRMED | Present in public ROM-hacking constants and standard hardware mapping |

## Reference ROM identity

### USA retail — canonical engineering reference
**Role:** Canonical binary for original addresses, disassembly, traces, and differential verification.

**Confidence:** CONFIRMED for size/CRC32/SHA-1 identity from public preservation/database evidence. SHA-256 will be recorded when independently established from a verified local/reference dump.

| Field | Value | Confidence |
|---|---|---|
| Title | `Beyond Oasis (USA)` | CONFIRMED |
| Size | `3145728` bytes (3 MiB) | CONFIRMED |
| CRC32 | `C4728225` | CONFIRMED |
| SHA-1 | `2944910c07c02eace98c17d78d07bef7859d386a` | CONFIRMED |
| SHA-256 | not yet pinned | UNKNOWN |
| Runtime role | reference evidence/data source, not region-specific gameplay model | ACCEPTED ADR-0005 |

**Evidence:**
- current MAME software-list metadata marks `beyond oasis (usa).bin` as `good`, size `3145728`, CRC `c4728225`, SHA-1 `2944910c07c02eace98c17d78d07bef7859d386a`;
- GameHacking.org independently reports USA / 3M / CRC32 `C4728225`;
- existing public `smd_beyondoasis` work explicitly targets `Beyond Oasis (U) [!]`.

### Europe retail — known secondary revision
**Role:** Secondary evidence/future data profile; not current address reference.

| Field | Value | Confidence |
|---|---|---|
| Title | `The Story of Thor (Europe)` | HIGH |
| Size | `3145728` bytes (3 MiB) | HIGH |
| CRC32 | `1110B0DB` | HIGH |
| Current runtime status | `KNOWN_UNSUPPORTED` | PROJECT POLICY |

### Region policy
- USA addresses are the default notation in reverse-engineering documents.
- Region-specific offsets must not leak into native gameplay code.
- Europe/Japan may be compared to USA to understand regional data/code differences.
- The final C++ game model is region-independent.

## Known game routine candidates

### `0x00003820` — `DecompressGraphics`
**Status:** Identified, not yet translated/verified in this repository.

**Confidence:** HIGH for semantic purpose, incomplete for exact contract.

**Known evidence:**
- public Beyond Oasis ROM-hacking work defines `decompress_gfx` at `0x3820`;
- patched graphics-loading code calls this routine before copying decoded tile data.

**Inputs:** UNKNOWN — must be established from exact disassembly/callers.

**Outputs:** Likely decompressed graphics data in memory; exact destination/length contract is UNKNOWN.

**Side effects:** UNKNOWN.

**Known callers:** To be enumerated from reference ROM/disassembly.

**Required before C++ translation:**
1. identify exact supported ROM revision;
2. capture routine address range;
3. disassemble complete control flow;
4. enumerate callers;
5. document register/memory contract;
6. obtain at least one reproducible input/output trace;
7. implement C++ translation;
8. compare result to reference behavior.

## Existing translated compatibility behavior

### Tile copy to work RAM
**Origin:** public translation-project macro `tilecopy_to_ram`.

**Behavior:** Copies 32-byte Genesis tile blocks from source to a destination in RAM while preserving the original routine's conceptual tile-line grouping.

**Status:** Initial C++ compatibility implementation exists. Must be revisited when exact runtime data interfaces stabilize.

### Tile copy to VRAM
**Origin:** public translation-project macro `tilecopy_to_vram`.

**Behavior:** Programs a VDP write destination and copies tile data through the VDP data port.

**Status:** Initial C++ compatibility implementation exists. Current VDP model is intentionally minimal.

## ROM identification implementation
**Status:** IMPLEMENTED, verification in progress.

Current detector records:
- exact input byte size;
- Mega Drive header signature/titles/product/region/ROM range;
- stored and calculated Sega checksum;
- CRC32;
- SHA-1;
- SHA-256;
- classification: `SUPPORTED`, `KNOWN_UNSUPPORTED`, `MODIFIED`, `UNKNOWN`.

USA `SUPPORTED` matching requires the confirmed 3 MiB size + CRC32 + SHA-1 tuple. No commercial ROM bytes are stored in tests or repository.

## Routine entry template
```text
### `0xAAAAAAAA-0xBBBBBBBB` — Name or `sub_AAAAAAAA`
Status: Identified | Investigating | Translated | Verified
Confidence: LOW | MEDIUM | HIGH | CONFIRMED

Purpose:

Evidence:

Inputs/registers:

Outputs/registers:

Memory reads/writes:

Callers:

Callees:

Control-flow notes:

C++ mapping:

Tests/reference traces:

Unknowns:

Next investigation:
```

## Data-format entry template
```text
### Data structure name
ROM range/source:
Confidence:

Layout:

Consumers:

Evidence:

Unknown fields:
```
