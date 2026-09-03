# Current Task

TASK: M11 — Event/script router investigation
WHY: M10 is complete; the next dependency is evidence-backed script/event behavior beyond the already verified raw router slice.
CURRENT MILESTONE: M11
MILESTONE UNDERSTANDING CONFIDENCE: 70%
CURRENT SLICE UNDERSTANDING CONFIDENCE: 70%
SLICE MODE: EVIDENCE_GATHERING_ONLY
STATUS: ACTIVE

## Confidence evidence
- The completed M11 raw producer/router slice had >=90% slice confidence before implementation because its type gate, field transfer, bounded handler ranges, synthetic tests, USA-ROM oracle and CI were independently verified.
- The **current** follow-up slice does not yet have enough evidence to identify the type-8 producer caller or assign semantics to downstream helper `0x60004` / handler `0x7B2A`.
- Therefore current-slice confidence remains below 90% and no new production C++ behavior may be added for that follow-up until additional ROM/caller/data evidence raises it to >=90%.

## Preconditions completed
- M2 canonical USA ROM identification is complete.
- M3 graphics decompression is native and differentially verified.
- M4 local graphics inspection is verified against the reference ROM.
- M5 narrow VDP state model is tested and renderer-independent.
- M6 deterministic runtime/input skeleton is complete.
- M7 screen descriptors, byte-grid addressing, footprint aggregation and terrain movement gate are verified and tested.
- M8 player movement/state slice is accepted locally and by CI.
- M9 common entity-pool framework and representative callback-dispatch path are accepted locally and by CI.
- M10 spirit raw entry, slot/dispatch and target-query slice is accepted locally and by CI.

## Completed M11 initial slice
- [x] identify one event/script producer and its data-source evidence;
- [x] identify one bounded dispatch boundary;
- [x] translate one deterministic event/script operation;
- [x] add synthetic tests for the native operation;
- [x] add a local USA-ROM oracle for addresses/bytes/data used by the slice;
- [x] keep dialogue semantics and unproven commands out;
- [x] record unknown fields without invented meanings;
- [x] keep every file <= 500 lines;
- [x] CI green.

## Current follow-up acceptance gate
Before any new production C++ is added for the current follow-up:
- [ ] identify the direct/indirect caller or data path that reaches the type-8 producer, **or** fully bound the relevant downstream helper/handler contract;
- [ ] record exact addresses, inputs, outputs and side effects;
- [ ] raise CURRENT SLICE UNDERSTANDING CONFIDENCE to >=90% with written evidence;
- [ ] only then define implementation-specific acceptance criteria and write native behavior.

Until those boxes are satisfied, allowed work is reverse engineering, probes, evidence capture and documentation only.

## Constraints
- Do not begin M12 or later work.
- Do not label a raw record as enemy/NPC/effect without evidence.
- Do not infer entity fields from common engine conventions; prove them from callers/accesses.
- Do not add a generic stream parser without direct evidence that the current slice requires it.
- Keep pool iteration, behavior selection and presentation separate.
- Prefer exact integer/fixed-point behavior over floating point.
- Keep the ROM archive local-only and untracked.

## Exact next action
Investigate the type-8 source caller and/or downstream `0x7B2A -> 0x60004` contract. Gather evidence only until the current slice reaches >=90% confidence; do not extend production C++ before then.
