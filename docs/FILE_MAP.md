# File Map

This document is the canonical map of repository structure. Update it whenever structure or responsibilities change.

```text
/
├── AGENTS.md                  Mandatory rules for AI agents and contributors
├── CMakeLists.txt             Top-level build configuration and CTest registration
├── PROJECT_STATE.md           Current milestone/task/status for context recovery
├── README.md                  Public project overview and build entry point
├── TASK.md                    Current task, evidence, acceptance criteria, next action
├── .github/
│   └── workflows/
│       └── ci.yml             GitHub Actions build + CTest verification
├── docs/
│   ├── ARCHITECTURE.md        Layering, dependencies, translation strategy
│   ├── DECISIONS.md           Architecture decision records (ADR-style)
│   ├── DEVELOPMENT_RULES.md   Coding, testing and documentation rules
│   ├── FILE_MAP.md            This canonical repository map
│   ├── PROJECT_VISION.md      Goal, scope, non-goals and end-state
│   ├── REVERSE_ENGINEERING.md Address/routine/ROM/data research ledger
│   ├── ROADMAP.md             Ordered milestones and current active milestone
│   ├── TASK_TEMPLATE.md       Mandatory task/session handoff template
│   └── WORKLOG.md             Chronological record of every development step
├── src/
│   ├── main.cpp               CLI entry point and ROM identity report
│   ├── core/
│   │   ├── rom.cpp            ROM file loading/basic title access
│   │   ├── rom.hpp            ROM byte-container API
│   │   ├── rom_identity.cpp   Header/checksum/hash/known-revision identification
│   │   └── rom_identity.hpp   ROM identity models and public API
│   ├── genesis/
│   │   ├── memory_bus.cpp     Minimal Mega Drive address-space compatibility
│   │   ├── memory_bus.hpp     Memory bus interface
│   │   ├── vdp.cpp            Minimal VDP/VRAM compatibility behavior
│   │   └── vdp.hpp            VDP interface/state
│   └── game/
│       ├── symbols.cpp        Known original ROM symbol/address table
│       ├── symbols.hpp        Symbol metadata API
│       ├── translated_routines.cpp  First translated game/utility routines
│       └── translated_routines.hpp  Translated routine declarations
└── tests/
    ├── check_file_limits.cmake Enforces <=500-line rule through CTest
    ├── rom_identity_test.cpp   Synthetic CRC/hash/header/checksum tests
    └── smoke.cpp               Minimal build/runtime smoke test
```

## Planned directories
These directories are approved but should be created only when their milestone begins:

```text
src/tools/          ROM inspection/extraction command implementations
src/game/player/    Player behavior after routine boundaries are understood
src/game/world/     Rooms/maps/collision after formats are documented
src/game/entities/  Enemy/NPC/entity systems
src/game/spirits/   Spirit mechanics
src/game/scripts/   Event/script interpreter or translated semantics
src/audio/          Audio runtime/compatibility layer
src/platform/       Window/input/render/audio platform integration
```

## Rules
- Do not create a generic `utils` dumping ground. Name modules by responsibility.
- Do not create planned directories early merely to make the tree look complete.
- New top-level directories require an architecture decision.
- Keep each file <= 500 lines.
- When a file approaches 400 lines, consider splitting before adding major functionality.
