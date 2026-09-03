# Current Task

TASK: M7 — World/map/collision foundations
WHY: Deterministic runtime/input is now established. The next dependency for native gameplay is understanding how Beyond Oasis stores and loads rooms, map layers and collision data.
CURRENT MILESTONE: M7
UNDERSTANDING CONFIDENCE: 92%
STATUS: ACTIVE

## Preconditions completed
- M2 canonical USA ROM identification is complete.
- M3 graphics decompression is native and differentially verified.
- M4 local graphics inspection is verified against the reference ROM.
- M5 narrow VDP state model is tested and renderer-independent.
- M6 core runtime has explicit integer frame stepping, portable controller snapshots and deterministic replay-style tests.

## M6 completion evidence
- `src/core/runtime.hpp/.cpp` owns the platform-independent runtime step contract.
- controller state supports standard Genesis direction/action/start and extended buttons without platform APIs;
- each call to `RuntimeLoop::step()` produces exactly one numbered `FrameContext`;
- a synthetic input sequence run twice produces identical traces;
- modifying one frame of input changes the trace;
- duplicate experimental `game/runtime` API was removed so there is one source of truth;
- GitHub CI build/test is green.

## M7 Definition of Done
- [ ] identify at least one verified room/map loading path in the USA binary;
- [ ] document relevant ROM pointer/table addresses and call flow;
- [ ] identify map dimensions/layer representation with confidence labels;
- [ ] identify collision representation or explicitly isolate it as a separate unknown structure;
- [ ] create a portable C++ data model only after raw formats are evidenced;
- [ ] load at least one room/map structure from a supported local ROM;
- [ ] add synthetic tests for decoded structures and collision queries once semantics are known;
- [ ] record unknown fields without invented names;
- [ ] keep every file <= 500 lines;
- [ ] CI green.

## Constraints
- Do not translate player behavior yet.
- Do not invent room fields from visual guesses.
- Keep ROM offsets inside reverse-engineering/data-reader code, not gameplay logic.
- Prefer static evidence + caller tracing + local inspection before naming structures.
- Do not begin M8 until M7 acceptance criteria pass.

## Exact next action
Inventory existing public reverse-engineering knowledge for Beyond Oasis map/tilemap pointers, then verify candidate addresses/functions directly against the canonical USA ROM before creating C++ room structures.
