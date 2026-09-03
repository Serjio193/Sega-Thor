# Current Task

TASK: M3 — Reverse-engineer graphics decompression routine at `0x00003820`
WHY: This is the first substantial original 68000 routine to recover as native C++ and a template for later function-by-function reconstruction.
CURRENT MILESTONE: M3
UNDERSTANDING CONFIDENCE: 95%
STATUS: ACTIVE

## Preconditions completed
- M2 reference ROM identification is complete.
- Uploaded `Beyond Oasis (USA).md` matches canonical USA size/CRC32/SHA-1.
- SHA-256 recorded as `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
- Reference-ROM workflow passes against the uploaded ROM.
- General CI passes after fixing ROM loader construction.

## Definition of Done
- [ ] obtain disassembly around `0x00003820` from the verified USA ROM;
- [ ] determine exact routine start/end boundaries;
- [ ] enumerate direct branch targets and subroutine calls;
- [ ] identify callers where practical;
- [ ] document input registers/pointers;
- [ ] document output registers/pointers;
- [ ] document memory reads/writes and side effects;
- [ ] mark all conclusions with evidence/confidence;
- [ ] obtain at least one reproducible input/output behavior trace or equivalent reference evidence;
- [ ] write readable C++ translation only after the contract is understood;
- [ ] add tests comparing translated behavior to reference evidence;
- [ ] update `REVERSE_ENGINEERING.md` and `WORKLOG.md`;
- [ ] CI green on the completed implementation.

## Constraints
- Do not infer semantic names without evidence.
- Do not start M4 asset tooling before this routine is verified.
- Do not execute original 68000 code in the final runtime as a shortcut.
- Keep files <= 500 lines.

## Exact next action
Disassemble a bounded window around `0x3820` from the verified USA ROM, identify control-flow boundaries, and document the routine contract before writing C++.
