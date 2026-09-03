# Current Task

TASK: M2 — Define and implement reference ROM identification
WHY: All reverse-engineered addresses and tests need one reproducible binary reference before translating original code.
CURRENT MILESTONE: M2
UNDERSTANDING CONFIDENCE: 90% for identification architecture; exact canonical hash policy is under user discussion.
STATUS: USER DECISION REQUIRED

## Proposed acceptance criteria
- choose one canonical engineering ROM revision;
- document known regional/revision variants without committing ROM data;
- identify input by exact byte size;
- parse and validate Sega Mega Drive header fields;
- compute cryptographic whole-file hash;
- compare against a registry of known ROM identities;
- distinguish SUPPORTED / KNOWN_UNSUPPORTED / UNKNOWN / MODIFIED;
- tests use synthetic data and hash primitives, never copyrighted ROM content;
- update REVERSE_ENGINEERING.md, WORKLOG.md, PROJECT_STATE.md and ROADMAP.md with evidence.

## Current evidence
- Existing public `smd_beyondoasis` reverse-engineering/translation work explicitly expects `Beyond Oasis (U) [!]`, making USA the strongest reference candidate for address compatibility.
- Public database evidence identifies USA retail as 3 MiB / CRC32 C4728225.
- Public database evidence identifies Europe retail as 3 MiB / CRC32 1110B0DB.
- Additional retail variants exist for Germany, Spain, Japan and Korea.

## Decision under discussion
Recommended policy: USA retail clean dump is CANONICAL_SUPPORTED. Known clean regional variants are KNOWN_UNSUPPORTED initially. They can become supported only after address/data compatibility is verified.

## Exact next action after decision
Implement `RomIdentity` + registry + SHA-256/CRC32/header validation and tests; then verify canonical hash metadata from independent sources/local user ROM before marking it CONFIRMED.
