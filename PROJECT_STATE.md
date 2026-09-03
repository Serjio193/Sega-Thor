# Project State

CURRENT_MILESTONE: M8 — Player system
CURRENT_TASK: Recover the first verified player movement/update path from the canonical USA ROM and define only the confirmed portable state needed to reproduce it
STATUS: ACTIVE
LAST_VERIFIED_RESULT: M7 completed; screen descriptors, byte-grid addressing, footprint aggregation and terrain movement gate are implemented/tested, and build/reference/probe checks are green
NEXT_ACTION: identify the player entity/update entry point, trace controller input into movement deltas and the confirmed terrain gate, then record the smallest verified player state contract
DO_NOT_WORK_ON: M9+, Thor 2, Saturn support, remaster features, speculative attacks/animation systems
BLOCKERS: none

## M7 verified evidence
- Screen dispatcher `0xC8F0` resolves 16-bit screen IDs through group table `0xC92C` to 26-byte descriptors; native loader has ROM-backed reference tests.
- Auxiliary byte grids at `0xFF1766` / `0xFF1770` use 8-pixel cells with confirmed pointer/stride/shift addressing.
- `0x9C40` is a confirmed single-cell world-grid reader.
- `0x9BF2` aggregates the entity footprint with OR/AND over covered grid bytes; native `ByteGridView` reproduces this contract.
- Low nibble terrain codes map through `0x96E8` / `0x96F8` into movement/height classes.
- `0x938E` is a confirmed directional terrain movement gate: carry set blocks movement, carry clear allows it.
- Synthetic byte-grid/footprint/terrain-gate tests pass.
- Main build-test, reference-ROM workflow and final M7 probe passed on the M7 completion head.

## Confirmed USA reference fingerprint
- Size: 3,145,728 bytes
- CRC32: `c4728225`
- SHA-1: `2944910c07c02eace98c17d78d07bef7859d386a`
- SHA-256: `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`

## Accepted reference policy
- Canonical engineering reference: clean USA retail `Beyond Oasis`.
- Final reconstructed C++ game model is region-independent.
- Europe/Japan are secondary evidence and future data profiles.
