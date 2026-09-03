# Sega Thor / Beyond Oasis C++

Reverse-engineering and native C++20 reimplementation project for **Beyond Oasis / The Story of Thor** (Sega Mega Drive / Genesis).

## Goal

Reimplement the original game logic in portable C++ while using data extracted/read from a user-supplied original ROM. This repository must not contain copyrighted ROM images or original game assets.

The target is a **native game implementation**, not a general-purpose Mega Drive emulator.

## Mandatory project documents

Before contributing or asking an AI agent to modify code, read:

1. [`AGENTS.md`](AGENTS.md) — mandatory workflow and anti-drift rules.
2. [`docs/PROJECT_VISION.md`](docs/PROJECT_VISION.md) — project idea, scope and non-goals.
3. [`docs/ROADMAP.md`](docs/ROADMAP.md) — ordered development milestones and active direction.
4. [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) — layer boundaries and dependency rules.
5. [`docs/FILE_MAP.md`](docs/FILE_MAP.md) — canonical repository/file responsibility map.
6. [`docs/DEVELOPMENT_RULES.md`](docs/DEVELOPMENT_RULES.md) — C++, testing, reverse-engineering and documentation rules.
7. [`docs/REVERSE_ENGINEERING.md`](docs/REVERSE_ENGINEERING.md) — evidence ledger for addresses, routines and data formats.
8. [`docs/DECISIONS.md`](docs/DECISIONS.md) — architecture decision log.
9. [`docs/WORKLOG.md`](docs/WORKLOG.md) — chronological record of development actions.

### Hard rule

**Every source and project-documentation file must remain at or below 500 lines.** Split modules before exceeding the limit.

Every meaningful development action must be documented in the worklog, and reverse-engineering findings must be recorded in the evidence ledger.

## Current status

Initial bootstrap includes:

- C++20 project;
- ROM loader;
- Mega Drive memory map scaffold;
- VDP/VRAM scaffold;
- known reverse-engineered symbol addresses;
- first translated tile-copy helpers;
- smoke test;
- project governance/architecture documentation.

## Current development direction

The ordered path is:

```text
project rules/governance
    -> identify exact supported ROM revision
    -> reverse-engineer graphics decompression at 0x00003820
    -> local asset inspection tool
    -> VDP/rendering primitives
    -> deterministic runtime/input
    -> world/collision
    -> player
    -> entities
    -> spirits
    -> scripts/events
    -> UI/save
    -> audio
    -> full-game parity
    -> portability
    -> optional enhancements
```

**The Story of Thor 2 is deliberately deferred.**

## Build

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

On Windows with LLVM-MinGW, the CMake test configuration supplies the compiler
runtime DLL directory to CTest automatically.

## Bounded reverse-engineering report

The developer-only `oasis_re_slice` tool reads a locally supplied verified USA
ROM and writes deterministic JSON plus a short text report. Its default target
is the bounded M11 entry `0x60004`:

```bash
oasis_re_slice "Beyond Oasis (USA).bin" slice.json slice.txt
```

The decoder follows direct control flow only within `[0x60004, 0x61204)`;
indirect control flow and unsupported opcode families are reported separately.
It is not an emulator, whole-ROM recompiler or gameplay-runtime dependency.

## ROM policy

Do not commit ROM files or extracted commercial assets. The runtime/tooling operates on a locally supplied, legally obtained ROM dump.

## Known reverse-engineering starting points

- VDP data port: `0xC00000`
- VDP control port: `0xC00004`
- 68000 RAM base: `0xFF0000`
- graphics decompression routine candidate: `0x00003820`

The reverse-engineering ledger is the authoritative location for confidence/status of these findings.
