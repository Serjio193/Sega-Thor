# Project State

CURRENT_MILESTONE: M2 — Identify supported ROM revision
CURRENT_TASK: Implement ROM identification by size + header + checksum + hashes
STATUS: ACTIVE
LAST_VERIFIED_RESULT: USA reference / region-independent runtime policy accepted in ADR-0005
NEXT_ACTION: implement and test ROM identity primitives, then confirm exact clean-USA cryptographic metadata
DO_NOT_WORK_ON: M3+, Thor 2, Saturn support, remaster features
BLOCKERS: none for implementation; exact canonical USA SHA-256 remains evidence-pending

## Accepted reference policy
- Canonical engineering reference: clean USA retail `Beyond Oasis`.
- Final reconstructed C++ game model is region-independent.
- Europe/Japan are secondary evidence and future data profiles.
- Recognition layers: byte size, Mega Drive header, Sega checksum, CRC32, SHA-256.
- Known regional revisions may be recognized but remain unsupported until mapped/verified.
- Exact cryptographic metadata is not marked CONFIRMED without independent evidence.
