# Project State

CURRENT_MILESTONE: M6 — Deterministic runtime frame/input skeleton
CURRENT_TASK: Add a platform-independent frame counter and input snapshot with deterministic step semantics
STATUS: ACTIVE
LAST_VERIFIED_RESULT: M5 completed; VRAM/CRAM/VSRAM, tile/plane/sprite raw attributes and bounds are implemented and CI passed
NEXT_ACTION: implement `game/runtime` input snapshot + frame state + deterministic step test
DO_NOT_WORK_ON: M7+, Thor 2, Saturn support, remaster features
BLOCKERS: none

## M5 verified evidence
- 64 KiB VRAM, 128-byte CRAM and 80-byte VSRAM modeled as raw portable state.
- Bounded byte-span writes and big-endian 16-bit read/write operations implemented.
- Standard tile word fields: index, palette, priority, horizontal flip, vertical flip.
- Minimal plane-cell representation uses the same raw tile semantics.
- Standard four-word sprite attribute layout represented without sprite rendering/emulation.
- Full-emulator behavior remains explicitly unsupported in `docs/VDP_MODEL.md`.
- `oasis_vdp` tests raw formats, storage and bounds.
- M5 head passed GitHub Actions build/test.

## Confirmed USA reference fingerprint
- Size: 3,145,728 bytes
- CRC32: `c4728225`
- SHA-1: `2944910c07c02eace98c17d78d07bef7859d386a`
- SHA-256: `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`

## Accepted reference policy
- Canonical engineering reference: clean USA retail `Beyond Oasis`.
- Final reconstructed C++ game model is region-independent.
- Europe/Japan are secondary evidence and future data profiles.
