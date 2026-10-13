# Three-helper instruction-stepped replay island (2026-10-13)

## Result

`REPLAY_NATIVE_ISLAND_VALIDATED = YES` for the pinned 3,600-frame development replay and exactly these caller-gated paths: `0x9D70` from `0x009144`, `0x9D4E` from `0x00909E`, and `0x9DB4` from `0x0092CC`. Production activation remains NO; global helper verification remains NO. The detailed per-run identity, component checks, path timing and metrics are in the adjacent JSON receipt.

## Validation

A fresh ROM-only baseline passed, followed by all three single-helper runs, all three pair combinations, and the triple. Every route used the same ROM, core, runner build, config, initial state lineage, 3,600 polls and effective-input hash. Across each route, all 3,600 full-frame canonical CPU/scheduler/observable-memory records matched baseline; architectural and scheduler sync traces (128 each), 60 observed landmarks, 1 MiB Z80 RAM snapshots, normalized helper events (804), other normalized events and ordered external events matched. The timing-path census matched baseline. No divergence or native mismatch occurred. Raw serializer SHA is diagnostic only.

Triple counts: `0x9D70` native 184, provenance rejects 2 and ROM fallbacks 2; `0x9D4E` native 113; `0x9DB4` native 507. Total native calls were 804. The two `0x9D70` rejected entries were left on the ROM path. No precondition rejects occurred.

## Timing and implementation notes

The session observed one natural branch path for each helper, with multiple cycle-sequence variants caused by instruction timing and refresh/scheduler yields. Exact path IDs, instruction sequences, cycle sequences, total-cycle variants and refresh-before-RTS values are retained in the JSON receipt. These are observed replay paths, not a claim that every CFG branch was naturally exercised. The stepper charges at instruction boundaries and leaves refresh, `cycle_end`, scheduler yields and resume to the core mechanism.

Differential debugging corrected the `0x9D4E` operation at `0x9D54`: opcode `0xDE6E` is `ADD.W`, not subtraction. At `0x9D58`, `LSL.W #3` has a dynamic shift charge; omitting it caused a 42-unit cycle lag on the tested core ratio. After correction, the single, pair and triple replay matrix passed.

## Status boundaries

This establishes replay-native equivalence only for this pinned development replay/configuration and the three exact caller paths. It does not promote any helper to global verification, does not activate production dispatch, and does not change runtime, structural, `SOURCE_OWNED`, or canonical SQLite authorities. Raw serializer hashes may remain unstable due already classified non-semantic serialized fields; canonical state and the explicit scheduler/component traces are the acceptance surface.

## Next priority

Rank the remaining replay-covered, path-specific verified helpers for instruction-step conversion. `0x9D92` is the next candidate to assess, gated to its accepted caller path; do not infer replay equivalence for callers outside the pinned session evidence.
