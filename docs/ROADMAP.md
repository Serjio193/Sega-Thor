# Development Roadmap

The roadmap is ordered. Do not skip ahead unless a blocking dependency is documented.

## Status legend
- `NEXT` — active milestone
- `TODO` — planned
- `BLOCKED` — cannot proceed until documented dependency is resolved
- `DONE` — acceptance criteria met

## M0 — Repository bootstrap — DONE
Acceptance criteria:
- C++20/CMake project exists;
- ROM loader exists;
- minimal memory/VDP scaffolding exists;
- smoke test exists;
- no copyrighted ROM/assets are committed.

## M1 — Project governance and documentation — NEXT
Goal: make development deterministic for human and AI contributors.

Acceptance criteria:
- `AGENTS.md` mandatory workflow exists;
- architecture documented;
- canonical file map documented;
- 500-line file limit documented;
- development rules documented;
- decision log exists;
- worklog exists;
- reverse-engineering ledger exists;
- README points contributors to these documents.

## M2 — Identify supported ROM revision — TODO
Goal: establish one reproducible reference binary.

Tasks:
- identify exact Beyond Oasis revision/region used for initial work;
- record file size and cryptographic hashes locally without committing ROM;
- add ROM identification code;
- reject/flag unsupported revisions clearly;
- document header fields and relevant address assumptions.

Acceptance criteria:
- tool/runtime can identify the supported reference ROM;
- tests cover version detection using non-copyrighted synthetic fixtures where possible.

## M3 — Reverse-engineer graphics decompression at `0x00003820` — TODO
Goal: translate the first substantial original routine to tested C++.

Tasks:
- obtain/disassemble exact routine boundaries from the reference ROM;
- document registers, inputs, outputs, memory effects and callers;
- record control flow;
- produce readable C++ translation;
- compare output against original routine behavior/traces.

Acceptance criteria:
- routine is documented in `REVERSE_ENGINEERING.md`;
- C++ implementation has tests;
- assumptions are explicitly marked;
- behavior is verified against reference evidence.

## M4 — Local asset inspection tool — TODO
Goal: inspect original graphics without committing assets.

Tasks:
- CLI command for local ROM;
- decompression invocation;
- Genesis 4bpp tile decoding;
- palette decoding;
- local-only export format such as PNG/debug dump.

Acceptance criteria:
- user can locally inspect selected graphics from their ROM;
- generated files are ignored by Git;
- repository contains no extracted commercial assets.

## M5 — VDP data model and rendering primitives — TODO
Goal: represent game-visible tile/sprite operations without full-console emulation.

Tasks:
- document needed VDP semantics;
- tile planes;
- sprite attributes;
- palette state;
- scrolling and priority semantics actually used by the game.

Acceptance criteria:
- known scenes/primitives can be reconstructed from local ROM data;
- compatibility layer remains narrow and documented.

## M6 — Runtime frame/input skeleton — TODO
Goal: establish deterministic update/frame loop and input abstraction.

Acceptance criteria:
- stable frame stepping;
- controller input abstraction;
- deterministic test mode;
- no gameplay logic buried in platform layer.

## M7 — World/map/collision foundations — TODO
Goal: decode room/map structures and collision semantics.

Acceptance criteria:
- documented data formats;
- a room can be loaded from local ROM;
- collision queries have tests.

## M8 — Player system — TODO
Goal: translate movement, animation state, attacks and relevant state machine.

Acceptance criteria:
- player behavior is mapped to original routines/data;
- core behavior passes differential/regression tests.

## M9 — Entities/enemies/NPCs — TODO
Goal: translate common entity framework and representative behaviors.

## M10 — Spirits — TODO
Goal: reproduce spirit summoning, targeting, abilities and interactions.

## M11 — Scripts/events/dialogue — TODO
Goal: reproduce game progression and event semantics.

## M12 — Inventory/UI/save — TODO
Goal: menus, inventory, item behavior and compatible save semantics.

## M13 — Audio — TODO
Goal: faithful music/SFX playback with the narrowest viable compatibility strategy.

Audio architecture requires an ADR before implementation.

## M14 — Full-game parity — TODO
Goal: complete game from start to credits with regression coverage.

Acceptance criteria:
- full playthrough possible;
- known differences are documented;
- critical progression blockers resolved.

## M15 — Portability and packaging — TODO
Targets initially:
- Windows;
- Linux;
- macOS.

Additional platforms are later decisions.

## M16 — Optional enhancements — TODO
Only after base parity:
- resolution/scaling options;
- improved controller UX;
- widescreen experiments;
- QoL options;
- rendering enhancements.

Enhancements must be optional and must not replace the faithful mode.

## Deferred
**The Story of Thor 2 / The Legend of Oasis** is explicitly deferred until the first project reaches a mature parity milestone.
