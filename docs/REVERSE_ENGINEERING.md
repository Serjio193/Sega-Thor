# Reverse-Engineering Ledger

This file records what is known about the original Beyond Oasis binary. Do not promote guesses to facts without evidence.

## Known hardware addresses
| Address | Meaning | Confidence | Evidence/status |
|---|---|---:|---|
| `0x00C00000` | Mega Drive VDP data port | CONFIRMED | Present in public ROM-hacking constants and standard hardware mapping |
| `0x00C00004` | Mega Drive VDP control port | CONFIRMED | Present in public ROM-hacking constants and standard hardware mapping |
| `0x00FF0000` | 68000 work RAM base | CONFIRMED | Present in public ROM-hacking constants and standard hardware mapping |

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

## ROM revision research
**Status:** NOT STARTED.

Record here when M2 begins:
- regional title/header values;
- ROM size;
- checksum fields;
- SHA-1/SHA-256 of developer-owned reference dump (hashes only, never ROM data);
- differences between known revisions;
- which revision defines canonical original addresses.

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
