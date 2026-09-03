# Development Roadmap

The roadmap is ordered. Do not skip ahead unless a blocking dependency is documented.

## Status legend
- `NEXT` — active milestone
- `TODO` — planned
- `BLOCKED` — cannot proceed until documented dependency is resolved
- `DONE` — acceptance criteria met

## M0 — Repository bootstrap — DONE
C++20/CMake bootstrap, ROM loader, minimal memory/VDP scaffolding and smoke test established.

## M1 — Project governance and documentation — DONE
Mandatory AI workflow, architecture/file map, 500-line rule, decision/research/work logs and task handoff are enforced.

## M2 — Identify supported ROM revision — DONE
Canonical engineering reference is USA retail `Beyond Oasis`; final C++ model remains region-independent.

Confirmed USA fingerprint:
- size `3145728`;
- CRC32 `C4728225`;
- SHA-1 `2944910c07c02eace98c17d78d07bef7859d386a`;
- SHA-256 `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

## M3 — Graphics decompression `0x00003820` — DONE
Verified native C++ translation of original routine `[0x3820, 0x3B3E)`.

Reference vectors:
- format A `0x16943C`: `1217 -> 3072`, SHA-256 `65e99e74020fedbdcb97c8249a5ccfe540aca5bb5d29bfb260352cd6f388c31a`;
- format B `0x1894EA`: `112 -> 128`, SHA-256 `167d4e5409f6b075b3b6f2bc61dbb747e8d8c857e8699745184ddf48d83bcda9`.

## M4 — Local asset inspection tool — DONE
`oasis_inspect` performs supported-ROM validation, native decompression, Genesis 4bpp tile decoding and local PGM/PPM export. Generated commercial data remains local/ignored.

## M5 — VDP data model and rendering primitives — DONE
Verified:
- 64 KiB VRAM, 128-byte CRAM and 80-byte VSRAM storage;
- bounded byte-span and big-endian word access;
- standard tile pattern-name decoding;
- minimal plane-cell and raw four-word sprite attribute decoding;
- explicit unsupported/full-emulator semantics list;
- no SDL/GPU dependency in core;
- VDP synthetic tests and CI green.

M5 reference probe additionally found 196 raw `0xC00000` data-port references and 33 `0xC00004` control-port references in the USA ROM. The `0x2BA0` routine writes 40 zero words after a VDP control setup, providing direct game evidence consistent with 80-byte VSRAM usage.

## M6 — Runtime frame/input skeleton — DONE
Goal: deterministic portable frame stepping without wall-clock or platform dependencies.

Verified:
- portable controller bit state for two ports;
- explicit `FrameContext` with integer `frame_index` and immutable input snapshot;
- `RuntimeLoop::step()` advances exactly one frame per call;
- deterministic test runs the same input sequence twice and requires identical traces;
- changed input produces a different trace;
- no SDL/OS dependency in runtime core;
- duplicate experimental frame API removed rather than maintaining parallel abstractions;
- current CI green.

## M7 — World/map/collision foundations — NEXT
Goal: decode room/map structures and collision semantics from the reference ROM before translating player behavior.

Tasks:
- locate candidate room/map data tables and their readers;
- identify pointer tables versus packed structures using code + ROM evidence;
- document one room/area data path from table entry to consumed fields;
- identify collision representation and query/update routines;
- implement only confirmed portable data structures;
- build a local ROM-backed room inspector when enough format is proven.

Acceptance criteria:
- at least one room/map structure documented with evidence and confidence levels;
- a confirmed room/map record can be loaded from local ROM without executing 68000 code;
- collision representation has a tested portable query API or milestone remains ACTIVE;
- reverse-engineering ledger/worklog updated;
- files remain <= 500 lines;
- CI green.

## M8 — Player system — TODO
Goal: translate movement, animation state, attacks and relevant state machine.

## M9 — Entities/enemies/NPCs — TODO
Goal: translate common entity framework and representative behaviors.

## M10 — Spirits — TODO
Goal: reproduce spirit summoning, targeting, abilities and interactions.

## M11 — Scripts/events/dialogue — TODO
Goal: reproduce game progression and event semantics.

## M12 — Inventory/UI/save — TODO
Goal: menus, inventory, item behavior and compatible save semantics.

## M13 — Audio — TODO
Goal: faithful music/SFX playback with the narrowest viable compatibility strategy. Audio architecture requires an ADR before implementation.

## M14 — Full-game parity — TODO
Goal: complete game from start to credits with regression coverage.

## M15 — Portability and packaging — TODO
Initial targets: Windows, Linux and macOS. Additional platforms are later decisions.

## M16 — Optional enhancements — TODO
Only after base parity: scaling/resolution, controller UX, widescreen experiments, QoL and rendering enhancements. Faithful mode remains available.

## Deferred
**The Story of Thor 2 / The Legend of Oasis** is explicitly deferred until the first project reaches a mature parity milestone.
