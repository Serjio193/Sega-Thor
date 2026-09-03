# Current Task

TASK: M2 — Identify and verify the canonical reference ROM
WHY: All reverse-engineered addresses and differential tests require one reproducible binary reference before translating original 68000 code.
CURRENT MILESTONE: M2
UNDERSTANDING CONFIDENCE: 98%
STATUS: ACTIVE / IMPLEMENTED, FINAL VERIFICATION PENDING

## Accepted architecture
- Canonical engineering reference: clean USA retail `Beyond Oasis`.
- Final reconstructed game/runtime model: region-independent C++.
- Europe/Japan: secondary reverse-engineering evidence and future data profiles.
- Region-specific offsets stay out of gameplay code.
- See ADR-0005.

## Definition of Done
- [x] choose canonical engineering ROM revision;
- [x] document known regional/revision strategy without committing ROM data;
- [x] identify input by exact byte size;
- [x] parse Mega Drive header fields;
- [x] calculate and validate Sega checksum;
- [x] calculate CRC32;
- [x] calculate SHA-1;
- [x] calculate SHA-256;
- [x] compare against known identity metadata;
- [x] distinguish `SUPPORTED / KNOWN_UNSUPPORTED / MODIFIED / UNKNOWN`;
- [x] add synthetic tests for standard CRC32/SHA-1/SHA-256 vectors and header/checksum behavior;
- [x] add CI workflow for build + CTest;
- [ ] observe CI result for current branch revision;
- [ ] optionally record SHA-256 for a verified clean USA reference dump when independently established;
- [ ] update milestone to DONE after final branch verification.

## Confirmed reference evidence
USA retail reference:
- size: `3145728` bytes;
- CRC32: `C4728225`;
- SHA-1: `2944910c07c02eace98c17d78d07bef7859d386a`;
- MAME status: `good`;
- existing public reverse-engineering project targets `Beyond Oasis (U) [!]`.

Current code only marks USA as `SUPPORTED` when size + CRC32 + SHA-1 all match.

## Tests/evidence
- Local reconstructed bootstrap build before SHA-1 strengthening: CMake build passed; smoke + ROM identity tests passed.
- Hash tests use public standard test vectors and synthetic ROM data only.
- GitHub Actions workflow added to verify the actual repository branch.

## Exact next action
Check CI on the current branch. If green, mark M2 DONE and move to M3: establish exact boundaries/call contract for USA routine `0x00003820` before writing its C++ translation.
