# Project State

CURRENT_MILESTONE: M4 — Local asset inspection tool
CURRENT_TASK: Build a local-only graphics inspection path using the verified 0x3820 C++ decompressor
STATUS: ACTIVE
LAST_VERIFIED_RESULT: M3 completed; native C++ decompressor matches original 68000 traces for both compression formats and CI is green
NEXT_ACTION: add local ROM asset inspection command, Genesis 4bpp tile decoding, and safe local-only output
DO_NOT_WORK_ON: M5+, Thor 2, Saturn support, remaster features
BLOCKERS: none

## M3 verified evidence
- Original routine range: `[0x00003820, 0x00003B3E)`.
- Direct absolute callers found: 52 `JSR`, 0 `JMP`.
- Input `A0`: compressed source pointer; output `A1`: destination pointer advanced to output end.
- Format selector: `source[2] != 0` selects command stream; zero selects bit stream.
- Native C++ implementation: `src/game/graphics_decompress.cpp`.
- Synthetic decompression tests pass.
- ROM-backed differential verification passes for both formats:
  - `0x16943C`: consumed 1217, output 3072, SHA-256 `65e99e74020fedbdcb97c8249a5ccfe540aca5bb5d29bfb260352cd6f388c31a`;
  - `0x1894EA`: consumed 112, output 128, SHA-256 `167d4e5409f6b075b3b6f2bc61dbb747e8d8c857e8699745184ddf48d83bcda9`.

## Confirmed USA reference fingerprint
- Size: 3,145,728 bytes
- CRC32: `c4728225`
- SHA-1: `2944910c07c02eace98c17d78d07bef7859d386a`
- SHA-256: `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`
- Uploaded archive member: `Beyond Oasis (USA).md`
- Detector result: `SUPPORTED`

## Accepted reference policy
- Canonical engineering reference: clean USA retail `Beyond Oasis`.
- Final reconstructed C++ game model is region-independent.
- Europe/Japan are secondary evidence and future data profiles.
