# Current Task

TASK: M10 — Spirits
WHY: M9 established the common entity framework. The next dependency is one evidence-backed spirit lifecycle slice before expanding into abilities or interactions.
CURRENT MILESTONE: M10
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

## M10 Definition of Done
- [x] identify a summon entry and its caller/data evidence;
- [x] identify spirit slot storage and lifecycle state transitions;
- [x] identify the dispatch path for an active spirit;
- [x] translate one deterministic summon/slot/dispatch slice;
- [x] translate one raw target-query interaction contract; semantic ability names remain unproven;
- [x] add synthetic tests for the native deterministic slice;
- [x] add a local USA-ROM oracle for addresses/bytes/data used by the slice;
- [x] keep rendering, audio and unproven spirit semantics out;
- [x] record unknown fields without invented semantic names;
- [x] keep every file <= 500 lines;
- [ ] CI green.

## Constraints
- Do not begin M11 or later work.
- Do not label a raw record as enemy/NPC/effect without evidence.
- Do not infer entity fields from common engine conventions; prove them from callers/accesses.
- Reuse M7 `ByteGridView` / terrain gate rather than introducing a second collision model.
- Keep pool iteration, behavior selection and presentation separate.
- Prefer exact integer/fixed-point behavior over floating point.
- Keep the ROM archive local-only and untracked.

## Exact next action
Commit and push the raw summon-entry slice, then verify CI. Keep the producer
of raw type `0x16` in `FF19E8` and the semantic identity of the
`0x17A96`/`0x17CA6` path as documented unknowns; do not add behavior without
new evidence.
