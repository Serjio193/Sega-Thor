# Project State

CURRENT_MILESTONE: M2 — Identify supported ROM revision
CURRENT_TASK: Define and implement reference ROM identification strategy
STATUS: DISCUSSION / USER DECISION REQUIRED
LAST_VERIFIED_RESULT: C++ bootstrap and project governance are present in branch oasis-cpp-bootstrap
NEXT_ACTION: Choose reference ROM policy, then implement identification by size + header + cryptographic hash
DO_NOT_WORK_ON: M3+, Thor 2, Saturn support, remaster features
BLOCKERS: Reference-ROM policy not yet selected

## Candidate policy under discussion
- Canonical engineering reference: one exact clean ROM revision.
- Recognition layers: file size, Sega header fields/checksum, SHA-256 (primary), SHA-1/CRC32 optional compatibility metadata.
- Additional known revisions may be recognized but marked unsupported until mapped/verified.
