# AGENTS.md — mandatory project instructions

This file is the first document any AI agent or contributor must read before changing the repository.

## Mission
Reimplement **Beyond Oasis / The Story of Thor** as a portable C++20 project using a user-supplied original ROM for copyrighted data. The long-term target is a native implementation, not a general-purpose Mega Drive emulator.

## Non-negotiable rules
1. Do not change the project goal without an explicit architecture decision recorded in `docs/DECISIONS.md`.
2. Do not add ROMs, extracted copyrighted assets, BIOS files, or commercial game data to the repository.
3. Keep every source/document file at **500 lines or fewer**. Split before exceeding the limit.
4. Prefer small modules with one responsibility.
5. Do not implement unrelated features while the current roadmap milestone is unfinished.
6. Every meaningful code change must have a corresponding entry in `docs/WORKLOG.md`.
7. Every architectural change must have an ADR-style entry in `docs/DECISIONS.md`.
8. Every reverse-engineered routine must record its original address, evidence, assumptions, and test status in `docs/REVERSE_ENGINEERING.md`.
9. Update `docs/FILE_MAP.md` whenever files/directories are added, removed, renamed, or their responsibility changes.
10. Update `docs/ROADMAP.md` when milestone status changes.
11. Tests are required for translated deterministic routines whenever practical.
12. Preserve original game behavior first. Enhancements, widescreen, HD assets, QoL and modernization belong after behavioral parity.
13. Never silently invent unknown game behavior. Mark unknowns explicitly and gather evidence.
14. Never replace reverse engineering with a full CPU emulator unless the decision is explicitly approved and documented.
15. Keep commits focused. One conceptual task per commit whenever practical.

## Required workflow for every task
Before coding:
1. Read `docs/PROJECT_VISION.md`.
2. Read `docs/ROADMAP.md` and identify the active milestone.
3. Read `docs/ARCHITECTURE.md`.
4. Read `docs/FILE_MAP.md`.
5. Read the latest entries in `docs/WORKLOG.md` and `docs/DECISIONS.md`.
6. State the concrete task and acceptance criteria in the worklog.

During coding:
1. Work only on the active task.
2. Keep files below 500 lines.
3. Add/update tests alongside translated logic.
4. Record reverse-engineering discoveries immediately.
5. Avoid speculative refactors unrelated to the active task.

After coding:
1. Build the project.
2. Run tests.
3. Update `docs/WORKLOG.md` with results and remaining unknowns.
4. Update `docs/FILE_MAP.md` if structure changed.
5. Update roadmap status if a milestone moved.
6. Record any architectural decision.

## Definition of done
A task is not done merely because code compiles. It is done when:
- implementation exists;
- behavior is tested or explicitly marked unverified;
- documentation is updated;
- architecture/file map remains accurate;
- no file exceeds 500 lines;
- no copyrighted ROM/assets were committed;
- next step is clearly stated.

## Priority order
When priorities conflict, use this order:
1. Legal repository hygiene.
2. Behavioral fidelity to the original game.
3. Reproducible evidence and tests.
4. Clear architecture and maintainability.
5. Portability.
6. Performance.
7. Enhancements.

## Current active direction
The current technical direction is:
ROM loader → reverse-engineering database → graphics decompression (`0x00003820`) → asset inspection tools → rendering primitives → game systems translated one by one.

Do not jump directly to remaster features, Story of Thor 2, a new engine, or unrelated emulator work while this path is active.
