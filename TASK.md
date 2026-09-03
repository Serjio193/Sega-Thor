# Current Task

TASK: M11 — Event/script router investigation
WHY: M10 is complete; the next dependency is evidence-backed script/event behavior beyond the already verified raw router slice.
CURRENT MILESTONE: M11
MILESTONE UNDERSTANDING CONFIDENCE: 70%
CURRENT SLICE UNDERSTANDING CONFIDENCE: 95%
SLICE MODE: IMPLEMENTATION_ALLOWED
STATUS: COMPLETE

## Confidence evidence
- The completed M11 raw producer/router slice had >=90% slice confidence before implementation because its type gate, field transfer, bounded handler ranges, synthetic tests, USA-ROM oracle and CI were independently verified.
- The **current** follow-up slice now has exact raw inputs, outputs and side effects for `0x7B2A -> 0x60004 -> 0x609C6/0x60D10 -> 0x62CC`; the producer caller itself remains an independent unknown.
- Current-slice confidence is 95% for this bounded downstream contract. No semantic audio/dialogue label is assigned.

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
- [x] fully bound the relevant downstream handler contract;
- [x] record exact addresses, inputs, outputs and side effects;
- [x] raise CURRENT SLICE UNDERSTANDING CONFIDENCE to >=90% with written evidence;
- [x] define implementation-specific criteria for the bounded raw trace.

## Current follow-up implementation criteria
- preserve the masked result from the first raw driver call;
- issue raw selector `0x0008` only for sentinel `0x01FF`;
- expose the verified `FF17B8`, current-record and `0x62CC` cleanup effects;
- keep driver, audio, dialogue and progression meanings unassigned.

## Constraints
- Do not begin M12 or later work.
- Do not label a raw record as enemy/NPC/effect without evidence.
- Do not infer entity fields from common engine conventions; prove them from callers/accesses.
- Do not add a generic stream parser without direct evidence that the current slice requires it.
- Keep pool iteration, behavior selection and presentation separate.
- Prefer exact integer/fixed-point behavior over floating point.
- Keep the ROM archive local-only and untracked.

## Exact next action
M11 bounded event-router slice is complete and verified. Stop here and await
an explicit next-milestone instruction; do not begin M12.
