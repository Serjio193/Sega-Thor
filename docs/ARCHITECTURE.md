# Architecture

## Target architecture

```text
User-owned ROM
    |
    v
ROM validation / version detection
    |
    +--> data readers / extractors
    |
    v
Game data interfaces
    |
    v
Translated game logic (C++20)
    |
    +--> input
    +--> renderer
    +--> audio
    +--> save system
    |
    v
Platform layer
```

## Layers

### `core`
Platform-independent utilities and ROM access. No gameplay rules.

Responsibilities:
- ROM loading;
- checksums/version identification;
- endian-aware reads;
- diagnostics.

### `genesis`
Minimal compatibility layer for Mega Drive concepts actually used by the game.

Responsibilities:
- original address constants;
- work RAM model where useful during translation;
- VDP/VRAM semantics required by translated routines;
- palette/tile helpers;
- eventually narrow audio compatibility interfaces.

This layer must not quietly become a full console emulator.

### `game`
Translated Beyond Oasis logic.

Responsibilities:
- named translated routines;
- player/world/entities;
- collision;
- spirits;
- scripting/events;
- menus/inventory;
- save semantics.

Every routine translated from assembly should retain a traceable mapping to original ROM address(es).

### `tools`
Developer-only inspection and extraction tools.

Responsibilities:
- ROM inspection;
- graphics decompression validation;
- tile/palette/map exports for local analysis;
- symbol/address reports;
- differential test helpers.

Tools must not require committing extracted assets.

### `platform`
Modern OS/window/input/audio/rendering integration.

This layer should remain isolated from game rules.

### `tests`
Behavioral and regression tests.

Preferred test types:
- unit tests for deterministic translated routines;
- golden/hash tests using locally generated fixtures that do not contain copyrighted data;
- differential tests against known traces/values;
- integration smoke tests.

## Dependency rules
- `game` may depend on `core` and narrow `genesis` interfaces.
- `genesis` may depend on `core` but not `game`.
- `platform` must not own gameplay logic.
- `tools` may inspect all low-level data APIs but should not become a runtime dependency of the game.
- cyclic dependencies are prohibited.

## File-size rule
Every code and documentation file must remain at or below **500 lines**. Split modules by responsibility before reaching the limit.

## Translation strategy
Do not translate all 68000 instructions mechanically into a monolithic CPU state loop. Preferred order:
1. identify routine boundaries and inputs/outputs;
2. understand observable semantics;
3. create a named C++ function with explicit types;
4. preserve address metadata;
5. test against original evidence;
6. refactor only after parity is established.

Temporary register-like translation is allowed when semantics are still unclear, but it should be isolated and documented as transitional code.

## Architecture change policy
Any change that alters layer responsibilities, project direction, major dependencies, rendering strategy, audio strategy, ROM-data policy, or translation approach requires an entry in `docs/DECISIONS.md` before or with the code change.
