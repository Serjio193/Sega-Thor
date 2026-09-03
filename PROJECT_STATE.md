# Project State

CURRENT_MILESTONE: M5 — VDP data model and rendering primitives
CURRENT_TASK: Replace the placeholder VDP scaffold with a narrow, testable model of the Mega Drive video state required by Beyond Oasis
STATUS: ACTIVE
LAST_VERIFIED_RESULT: M4 completed; local asset inspector builds, all 5 CTests pass, and ROM-backed CI verifies 0x16943C -> 3072 bytes -> 96 tiles -> 128x48 PGM
NEXT_ACTION: model VRAM/CRAM/VSRAM and tile attribute decoding, then prove semantics with synthetic tests before adding plane/sprite rendering
DO_NOT_WORK_ON: M6+, Thor 2, Saturn support, remaster features
BLOCKERS: none

## M4 verified evidence
- `oasis_inspect` accepts only the supported USA reference for address-based inspection.
- Native decompressor is used; no 68000 execution is required by the tool.
- Genesis 4bpp tile decoder and 16-color CRAM palette decoder have synthetic tests.
- Generated `.pgm`/`.ppm` output is ignored by Git.
- Reference workflow verified ROM source `0x16943C`:
  - source consumed: `1217`;
  - decompressed bytes: `3072`;
  - complete tiles: `96`;
  - default sheet: `128x48`;
  - PGM header and exact payload size verified.

## Confirmed USA reference fingerprint
- Size: 3,145,728 bytes
- CRC32: `c4728225`
- SHA-1: `2944910c07c02eace98c17d78d07bef7859d386a`
- SHA-256: `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`

## Accepted reference policy
- Canonical engineering reference: clean USA retail `Beyond Oasis`.
- Final reconstructed C++ game model is region-independent.
- Europe/Japan are secondary evidence and future data profiles.
