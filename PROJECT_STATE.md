# Project State

CURRENT_MILESTONE: M3 — Reverse-engineer graphics decompression at 0x00003820
CURRENT_TASK: Establish exact routine boundaries, callers/callees, register contract and memory effects for USA reference build
STATUS: ACTIVE
LAST_VERIFIED_RESULT: M2 completed; uploaded USA ROM matches canonical fingerprint and both CI workflows pass
NEXT_ACTION: disassemble a bounded window around 0x3820 from the verified USA ROM and document control flow before writing C++
DO_NOT_WORK_ON: M4+, Thor 2, Saturn support, remaster features
BLOCKERS: none

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
