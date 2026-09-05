# Development Roadmap

The roadmap is ordered. Do not skip ahead unless a blocking dependency is documented.

## Status legend
- `ACTIVE` — current milestone being worked on
- `NEXT` — immediate queued milestone when useful to distinguish it from general TODO work
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
Verified 64 KiB VRAM, 128-byte CRAM, 80-byte VSRAM, bounded byte/word access, standard tile pattern-name decoding, minimal plane/sprite data, explicit unsupported emulator semantics and green tests.

## M6 — Runtime frame/input skeleton — DONE
Portable controller snapshots, explicit integer frame stepping and deterministic replay-style tests are established without wall-clock or platform dependencies.

## M7 — World/map/collision foundations — DONE
Goal: decode room/map structures and collision semantics before translating player behavior.

Verified:
- screen dispatcher `0xC8F0` uses high/low bytes of the screen ID with group table `0xC92C` and resolves 26-byte descriptors;
- multiple USA ROM screen IDs resolve to fixed descriptor addresses and are covered by ROM-backed native tests;
- auxiliary byte-grid descriptors at `0xFF1766` / `0xFF1770` expose backing pointer, row stride and row shift;
- world coordinates map to 8-pixel cells; `0x9C40` is the confirmed single-cell reader;
- `0x9BF2` aggregates OR/AND values across an entity footprint and is reproduced by `ByteGridView`;
- low-nibble terrain codes are mapped by tables `0x96E8` / `0x96F8` into movement/height classes;
- `0x938E` is a confirmed directional terrain movement gate: carry set blocks, carry clear allows;
- portable byte-grid, footprint and terrain-gate tests pass;
- final build-test, reference-ROM workflow and M7 probe are green.

## M8 — Player system — DONE
Goal: translate player movement/state from verified binary behavior before attacks and presentation layers.

Tasks:
- identify player entity/update entry and call path;
- trace controller bits into movement intent;
- connect player movement to the M7 terrain gate;
- identify exact position/delta/facing/movement-state fields used by the path;
- translate the smallest portable movement slice with integer/fixed-point semantics;
- add deterministic movement and blocked-movement tests;
- add ROM-backed behavior evidence where practical;
- only then expand into animation and attacks.

Acceptance criteria:
- player movement/update path documented with addresses and confidence labels;
- portable movement state/update exists without platform/render dependencies;
- collision uses the single M7 world model;
- deterministic tests cover free and blocked movement;
- reverse-engineering ledger/worklog updated;
- files remain <= 500 lines;
- CI green.

Delivered on `main`: the portable movement/state slice, M7 terrain-gated integration,
ROM-backed evidence, raw velocity context boundary, local Debug/Release verification
and successful GitHub Actions CI. The commercial ROM remains local-only.

## M9 — Entities/enemies/NPCs — DONE
Goal: translate the common entity framework and one evidence-backed representative
non-player callback-dispatch path.

Delivered on `main`: raw pool descriptors and bounded record views, the shared
movement field-access contract, a representative `FF2954` dispatch gate, local
Debug/Release verification and successful GitHub Actions CI. AI, attacks,
animation and spawn semantics remain explicitly outside this milestone.

## M10 — Spirits — DONE
Goal: reproduce one evidence-backed spirit lifecycle slice: summon/slot state,
target selection and one ability/interaction contract. The first accepted slice
now includes the caller-gated raw summon-entry seed at `0x7A10 -> 0x846C`,
four event-backed slot bits, raw dispatch and first-match target selection.
Rendering, audio and unproven spirit semantics remain outside the milestone.
The milestone acceptance slice is complete with local Debug/Release verification
and successful GitHub Actions CI; remaining unknown relationships stay documented
as unknown and may be revisited when later evidence requires them.

## M11 — Scripts/events/dialogue — DONE
Goal: reproduce game progression and event semantics. The first task is to
prove one bounded event/script router operation from its ROM data source.

Current slice: the type-8 producer at `0x82AE` transfers raw fields to
`FF1976/FF1978/FF197A`, and `0x7A28` maps the raw event byte to bounded handler
addresses. Event, progression and dialogue meanings remain unknown.

The evidence-backed bounded M11 slice is complete with native tests, local
USA-ROM verification and CI. The post-M11 RE-acceleration checkpoints are also
complete: separate bounded decoder/report tooling covers `0x60004` and the
representative routines `0x3820`, `0x8E90`, `0xA6A4` and `0xD3B2`, plus a
bounded dynamic trace scenario for `0xA7D4..0xA7E4`, without adding gameplay
behavior. A fourth bounded comparison tool fingerprints the user-supplied USA
Beta 1994-11-01 ROM and compares only the requested `0x3820`, `0x60004`,
`0x82AE`, `0x7A28` and `0xA6A4` targets using normalized opcode/CFG signatures.
Unknown dialogue, progression, driver and cross-revision semantics remain
explicitly unimplemented. Full-game tracing, broader similarity search,
whole-ROM discovery, recompilation and M12 remain out of scope until explicitly
authorized.

Any later follow-up remains evidence-gathering only while its implementation-
slice confidence remains below 90%; production C++ must not expand until
caller/data or downstream-handler evidence raises that slice to >=90%.

## M11.5 — Recursive structural exploration — DONE
Goal: automate bounded, provable 68000 structural exploration while retaining
explicit uncertainty frontiers and a reusable ROM address map.

Delivered: developer-only oasis_re_explore reuses re_slice_decoder, re_atlas
and re_candidate_map; it applies tiered provenance seeds, a deterministic
priority worklist, guarded recursive control-flow traversal, explicit
analysis states, compact map ranges and stable blocker identities. Bounded
validation runs before the optional single ROM-wide measurement.

The fresh USA Ghidra 12.1.3 raw export was repeated A/B with identical
390972-byte SHA-256
613A3AA6DEB8D2DCF994C82ADC6A6939B7D5F27AF67A51D05C16F090D60A5315.
The bounded corpus passed: 15 entries and all six current direct-edge
anchors were recovered. The gated ROM-wide evidence run processed 537 entries,
decoded 19623 instructions and emitted 148 frontiers; 451 entries were
analyzed, 25 stopped on indirect flow and 28 on unsupported instructions.
The result is structural evidence only; no Atlas classification or production
runtime behavior was changed. The dynamic ant PoC remains the next separately
authorized checkpoint and is not implemented here.

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
