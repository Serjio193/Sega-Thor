# Project State

CURRENT_MILESTONE: M8 — Player system
CURRENT_TASK: Recover the first verified player movement/update path from the canonical USA ROM and define only the confirmed portable state needed to reproduce it
STATUS: ACTIVE
LAST_VERIFIED_RESULT: M7 completed; screen descriptors, byte-grid addressing, footprint aggregation and terrain movement gate are implemented/tested, and build/reference/probe checks are green
NEXT_ACTION: finish M8 acceptance review and keep the ROM reference workflow local-only
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

## M8 verified evidence
- Main player initialization selects entity slot `0xFF19E8` and type `2` at `0x13D6..0x142E`.
- Controller normalization writes the movement nibble to `0xFF165E`; `0x85E2` maps it to confirmed cardinal/diagonal fixed-point vectors.
- The shared movement cluster commits positions from `+0x72/+0x76` into `+0x08/+0x0C` and gates the footprint through `0x9BF2`/`0x938E`.
- Native player state, movement mapping and terrain-gated update are implemented with synthetic and local ROM-oracle checks.
- The indirect update path is closed: `0x8B22 -> 0x557A -> 0x5670 -> 0x59BA`, with state `0` entering `0x61F6`.
- State `2` stop and state `4` turning/timeout branches are byte-verified: stop clears `+0x4E/+0x52`, writes `+0x2A=0` and returns to state `0`; shared movement owns `+0x72/+0x76` cleanup, while turning uses `+0x16`, `+0x17`, `+0x72/+0x76` and timeout state `0xC`.
- Native `update_movement_state` now mirrors the confirmed state-2 stop and state-4 axis-selection/accumulation rules; presentation callbacks and the `0x64C4` slowdown context remain outside the API.
- The native state driver is integration-tested through the shared terrain consumer; footprint OR mask `+0x6F` is preserved, while the `0x64C4` globals `FF1985`, `FF1984` and bit 4 of `FF16F1` remain lifecycle-investigating.
- The `0x64C4` context boundary is isolated: `FF1985` and `FF1984` have external writers, while bit 4 of `FF16F1` is read without a direct writer in the scanned references.
- Optional native `VelocityAdjustContext` now covers the three confirmed `0x64C4` arithmetic outcomes; unavailable context preserves the existing deterministic path.
- Frame-boundary ordering is covered: state-4 reads the prior footprint mask, then shared movement updates the mask for the next frame.
- The Windows CTest runtime path is configured in CMake; clean-shell Debug and Release runs pass 11/11 without manual `PATH` edits.

## Confirmed USA reference fingerprint
- Size: 3,145,728 bytes
- CRC32: `c4728225`
- SHA-1: `2944910c07c02eace98c17d78d07bef7859d386a`
- SHA-256: `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`

## Accepted reference policy
- Canonical engineering reference: clean USA retail `Beyond Oasis`.
- Final reconstructed C++ game model is region-independent.
- Europe/Japan are secondary evidence and future data profiles.
