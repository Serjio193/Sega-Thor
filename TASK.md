# Current Task

TASK: M9 — Entities/enemies/NPCs
WHY: M8 established the player path and shared movement consumer. The next dependency is the common entity-pool contract before translating representative non-player behavior.
CURRENT MILESTONE: M9
UNDERSTANDING CONFIDENCE: 90%
STATUS: COMPLETE

## Preconditions completed
- M2 canonical USA ROM identification is complete.
- M3 graphics decompression is native and differentially verified.
- M4 local graphics inspection is verified against the reference ROM.
- M5 narrow VDP state model is tested and renderer-independent.
- M6 deterministic runtime/input skeleton is complete.
- M7 screen descriptors, byte-grid addressing, footprint aggregation and terrain movement gate are verified and tested.

- M8 player movement/state slice is accepted locally and by CI.

## M9 Definition of Done
- [x] identify the common entity-pool loops and raw pool descriptors;
- [x] implement bounded raw record access and the confirmed active-record predicate;
- [x] add synthetic tests for pool bounds, stride and active selection;
- [x] add a local USA-ROM oracle for the pool loop constants;
- [x] trace shared record fields consumed by the common movement entry;
- [x] identify one representative non-player behavior from caller/data evidence;
- [x] keep AI, attacks, animation callbacks and spawn semantics out until their dependencies are proven;
- [x] record unknown fields without invented semantic names;
- [x] keep every file <= 500 lines;
- [x] CI green.

## Constraints
- Do not begin M10 or spirits work.
- Do not label a raw record as enemy/NPC/effect without evidence.
- Do not infer entity fields from common engine conventions; prove them from callers/accesses.
- Reuse M7 `ByteGridView` / terrain gate rather than introducing a second collision model.
- Keep pool iteration, behavior selection and presentation separate.
- Prefer exact integer/fixed-point behavior over floating point.
- Keep the ROM archive local-only and untracked.

## Exact next action
M9 is complete. Do not begin M10 or spirits work without explicit authorization.
