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
- no copyrighted ROM/assets are committed in development code.

## M1 — Project governance and documentation — DONE
Goal: make development deterministic for human and AI contributors.

Completed:
- `AGENTS.md` mandatory workflow;
- architecture document;
- canonical file map;
- hard 500-line source/document limit;
- automatic CTest line-limit check;
- development rules;
- decision log;
- worklog;
- reverse-engineering ledger;
- task/AI handoff template;
- README entry points to all governance documents.

## M2 — Identify supported ROM revision — DONE
Goal: establish one reproducible reference binary.

Accepted reference:
- USA retail `Beyond Oasis` is the canonical engineering binary;
- final reconstructed game model remains region-independent;
- Europe/Japan are secondary evidence/future data profiles.

Verified implementation:
- exact byte-size reporting;
- Mega Drive header parsing;
- Sega checksum calculation/validation;
- CRC32, SHA-1 and SHA-256 calculation;
- known-ROM classification;
- `SUPPORTED / KNOWN_UNSUPPORTED / MODIFIED / UNKNOWN` states;
- synthetic tests;
- CLI fingerprint output;
- GitHub Actions build/test workflow;
- dedicated reference-ROM verification workflow.

Confirmed USA reference:
- size `3145728`;
- CRC32 `C4728225`;
- SHA-1 `2944910c07c02eace98c17d78d07bef7859d386a`;
- SHA-256 `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`;
- uploaded archive member `Beyond Oasis (USA).md`;
- detector result `SUPPORTED`;
- CI and reference-ROM workflow both green after ROM loader fix.

## M3 — Reverse-engineer graphics decompression at `0x00003820` — NEXT
Goal: translate the first substantial original routine to tested C++.

Tasks:
- obtain/disassemble exact routine boundaries from the verified reference ROM;
- document registers, inputs, outputs, memory effects and callers;
- record control flow;
- obtain reproducible reference behavior;
- produce readable C++ translation only after the contract is understood;
- compare translated output against original behavior/reference evidence.

Acceptance criteria:
- routine is documented in `REVERSE_ENGINEERING.md`;
- exact boundaries and direct control flow are established;
- C++ implementation has tests;
- assumptions are explicitly marked;
- behavior is verified against reference evidence;
- CI passes.

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
