# Project State

CURRENT_MILESTONE: M7 — World/map/collision foundations
CURRENT_TASK: Establish the first verified room/map loading path and raw data format from the canonical USA ROM
STATUS: ACTIVE
LAST_VERIFIED_RESULT: M6 completed; one core runtime/input API remains, deterministic input-sequence tests pass, and CI is green
NEXT_ACTION: inspect existing reverse-engineering map/tilemap knowledge, then verify candidate pointers/functions directly against the USA ROM before defining C++ room structures
DO_NOT_WORK_ON: M8+, Thor 2, Saturn support, remaster features
BLOCKERS: none

## M6 verified evidence
- `src/core/runtime.hpp/.cpp` is the single runtime/input source of truth.
- Explicit integer frame stepping; no wall-clock dependency.
- Portable controller snapshot for two ports.
- Identical initial state + input sequence produces identical recorded traces.
- Altering input changes the trace.
- Duplicate experimental `game/runtime` API was removed.
- GitHub CI build/test passed on the M6 head.

## Confirmed USA reference fingerprint
- Size: 3,145,728 bytes
- CRC32: `c4728225`
- SHA-1: `2944910c07c02eace98c17d78d07bef7859d386a`
- SHA-256: `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`

## Accepted reference policy
- Canonical engineering reference: clean USA retail `Beyond Oasis`.
- Final reconstructed C++ game model is region-independent.
- Europe/Japan are secondary evidence and future data profiles.
