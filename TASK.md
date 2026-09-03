# Current Task

TASK: M11 — Event/script router investigation
WHY: M10 is complete; the next dependency is one evidence-backed event/script operation.
CURRENT MILESTONE: M11
UNDERSTANDING CONFIDENCE: 70%
STATUS: ACTIVE

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

## M11 Definition of Done
- [x] identify one event/script producer and its data-source evidence;
- [x] identify one bounded dispatch boundary;
- [x] translate one deterministic event/script operation;
- [x] add synthetic tests for the native operation;
- [x] add a local USA-ROM oracle for addresses/bytes/data used by the slice;
- [x] keep dialogue semantics and unproven commands out;
- [x] record unknown fields without invented meanings;
- [x] keep every file <= 500 lines;
- [x] CI green.

## Constraints
- Do not begin M12 or later work.
- Do not label a raw record as enemy/NPC/effect without evidence.
- Do not infer entity fields from common engine conventions; prove them from callers/accesses.
- Reuse M7 `ByteGridView` / terrain gate rather than introducing a second collision model.
- Keep pool iteration, behavior selection and presentation separate.
- Prefer exact integer/fixed-point behavior over floating point.
- Keep the ROM archive local-only and untracked.

## Exact next action
Use the new raw boundary to investigate the caller of the type-8 source or
the downstream handler at `0x7B2A`; do not add a stream parser yet.
