# AGENTS.md — mandatory project instructions

This file is the first document any AI agent or contributor must read before changing the repository.

## Mission
Reimplement **Beyond Oasis / The Story of Thor** as a portable C++20 project using a user-supplied original ROM for copyrighted data. The long-term target is a native implementation, not a general-purpose Mega Drive emulator.

## Non-negotiable rules
1. Do not change the project goal without an explicit architecture decision recorded in `docs/DECISIONS.md`.
2. Do not add ROMs, extracted copyrighted assets, BIOS files, or commercial game data to the repository.
3. Keep executable/source-code files at **500 lines or fewer**. This includes C/C++ source and headers, tests, developer tooling, and build scripts such as CMake. Documentation, project instructions, worklogs, ADRs, task/state files, and other prose/reference Markdown are exempt from the numeric limit. Large documentation should remain well structured and may be split when navigation or responsibility improves, but must not be split merely to satisfy a line count.
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
16. Do not push an implementation commit until the locally available CI-equivalent validation is green. At minimum run the relevant Debug and Release builds/tests, `git diff --check`, the source-code file-limit check, and a GNU/Linux-equivalent build or link check when the change affects CMake targets, static libraries, link order, portability, or toolchain-sensitive code. If the exact CI toolchain is unavailable locally, record that limitation before push and avoid claiming CI readiness.
17. Historical SDK/toolchain material is evidence, not authority. For any task involving historical assemblers, linkers, compilers, development kits, preserved source trees or SDK fingerprints, read and follow `docs/RE_TOOLCHAIN_GUIDE.md`. Do not claim Ancient used a candidate toolchain without project-specific evidence and do not fetch/commit proprietary or leaked SDK binaries.

## Required workflow for every task
Before coding:
1. Read `docs/PROJECT_VISION.md`.
2. Read `docs/ROADMAP.md` and identify the active milestone.
3. Read `docs/ARCHITECTURE.md`.
4. Read `docs/FILE_MAP.md`.
5. Read the latest entries in `docs/WORKLOG.md` and `docs/DECISIONS.md`.
6. If the task uses historical SDK/toolchain/compiler/library evidence, read `docs/RE_TOOLCHAIN_GUIDE.md` before drawing any conclusion from it.
7. State the concrete task and acceptance criteria in the worklog.

During coding:
1. Work only on the active task.
2. Keep files covered by the source-code size policy below 500 lines; documentation is exempt from the numeric limit.
3. Add/update tests alongside translated logic.
4. Record reverse-engineering discoveries immediately.
5. Avoid speculative refactors unrelated to the active task.

After coding:
1. Build the project in the relevant Debug and Release configurations.
2. Run tests and CI-equivalent local validation, including GNU/Linux-equivalent linking when the change is toolchain-sensitive.
3. Run `git diff --check` and the project source-code file-limit check before push.
4. Update `docs/WORKLOG.md` with results, local-toolchain limitations and remaining unknowns.
5. Update `docs/FILE_MAP.md` if structure changed.
6. Update roadmap status if a milestone moved.
7. Record any architectural decision.
8. Push only after the available pre-push validation is green; do not use GitHub Actions as the first compile/link test for a work-in-progress implementation.

## Definition of done
A task is not done merely because code compiles. It is done when:
- implementation exists;
- behavior is tested or explicitly marked unverified;
- documentation is updated;
- architecture/file map remains accurate;
- no executable/source-code file covered by the size policy exceeds 500 lines;
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
