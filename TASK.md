# Current Task

TASK: M8 — Player system
WHY: M7 established tested world-grid and terrain collision primitives. The next dependency is recovering the original player's update/movement contract before translating attacks or animation state.
CURRENT MILESTONE: M8
UNDERSTANDING CONFIDENCE: 91%
STATUS: ACTIVE

## Preconditions completed
- M2 canonical USA ROM identification is complete.
- M3 graphics decompression is native and differentially verified.
- M4 local graphics inspection is verified against the reference ROM.
- M5 narrow VDP state model is tested and renderer-independent.
- M6 deterministic runtime/input skeleton is complete.
- M7 screen descriptors, byte-grid addressing, footprint aggregation and terrain movement gate are verified and tested.

## M8 Definition of Done
- [ ] identify the player entity/update entry point and document its call path;
- [ ] trace controller input bits into player movement intent without guessing field names;
- [ ] connect player movement to the confirmed M7 terrain/collision gate;
- [ ] identify the minimal position, velocity/delta, facing/direction and movement-state fields actually used by the verified path;
- [ ] implement a portable player movement/state slice only after the raw behavior is evidenced;
- [ ] add deterministic synthetic tests for movement and blocked movement;
- [ ] add at least one ROM-backed reference/oracle check for translated player behavior where practical;
- [ ] keep attacks, animation, inventory and enemy behavior out until their dependencies are proven;
- [ ] record unknown fields without invented semantic names;
- [ ] keep every file <= 500 lines;
- [ ] CI green.

## Constraints
- Do not begin M9.
- Do not infer player fields from common engine conventions; prove them from callers/accesses.
- Reuse M7 `ByteGridView` / terrain gate rather than introducing a second collision model.
- Separate input intent, movement resolution and presentation/animation.
- Prefer exact integer/fixed-point behavior over floating point.

## Exact next action
Locate the player entity/update routine in the USA ROM, then trace reads of controller state and the first calls into the movement cluster around `0x8F00..0x9D00`.
