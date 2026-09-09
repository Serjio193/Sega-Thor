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

The M11.25 resource baseline also keeps a narrow production path for verified
ROM resource extraction, native VDP transfer and structural tile diagnostics.
It consumes the canonical ROM through `resource_loader`, stores bytes only in
the bounded VDP model, and does not expose original work-RAM addresses as
native architecture or assign unproven resource semantics.

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

M11.28 adds `src/tools/hybrid/`, a developer-only GPGX observation boundary.
The optional frontend loads an external instrumented libretro library and
compares one naturally reached routine against existing C++ implementations.
`EMULATED` preserves execution, `SHADOW_NATIVE` compares bounded copied inputs
while the emulator remains authoritative, and `NATIVE_OVERRIDE` fails closed
until its CPU/timing/return contract is proven. The external bridge exposes
only register reads, bounded memory peeks and hook installation. Neither the
bridge nor the frontend is linked into `oasis_core`, `oasis_platform` or `oasis`.
See ADR-0011 and `reports/HYBRID_NATIVE_MIGRATION_POC.md` for the explicit
partial SR contract and interrupt/prefetch blockers.

M11.33 adds a bounded recomp generator in the same developer-only boundary.
It consumes `re_slice_decoder` exact IR and emits C++ instruction-helper calls
with guest address/opcode comments. The generated block artifact is an input
to the hybrid proof only; it is not production game code, a general CPU
interpreter, or a replacement for semantic lifting.

M11.34 adds only a test-only independent semantic reference model and vector
harness beside that boundary. It verifies the seven currently emitted forms
against the Motorola/NXP 68000 specification; it is not linked into production,
does not broaden decoder coverage and does not replace the GPGX timing oracle.

M11.35 adds a bounded offline discovery and promotion gate within the same
developer-only boundary. `DISCOVER_BLOCKS` records naturally entered guest PCs;
the exact decoder classifies new forms; the generator refuses unverified or
unsupported IR; and GPGX remains authoritative for shadow comparison. A failed
candidate is interpreter fallback only. The pilot has no runtime JIT, no
automatic trust, no production dependency, and no native promotion after the
first cycle/refresh and interrupt-boundary divergence at `0x3A85E`.

M11.36 fixes the observer boundary generically. GPGX owns the authoritative
`m68k.cycles` and `refresh_cycles` counters; they are accumulated master-cycle
values in the current frame and are rebased with `mcycles_vdp` at frame end.
The bridge therefore compares shadow state at a GPGX post-instruction hook,
after semantic and timing advancement but before the next scheduler/frame
transition. Normal interrupt polling remains GPGX-owned: `m68k_run` polls at
entry, the post hook is before trace/interrupt handling, and a translated block
is valid only when its bounded accesses cannot expose an intervening hardware
or interrupt boundary. The M11.33 and M11.35 blocks satisfy that observed
contract; no second timing model or candidate-specific correction was added.

M11.37 keeps this boundary data-driven for a bounded offline promotion set.
Natural PCs are discovered first, then exact decoder ranges are classified;
independently verified Bcc/DBcc/TST forms are emitted by the existing
generator, which also emits `GeneratedBlockSpec` metadata consumed by the
developer-only registry. `basic_block_reference` is only a generic state,
prefetch and timing prediction adapter for shadow setup; the semantic gate
remains the independent M68K test oracle and GPGX remains authoritative for
runtime effects. A block is not promoted when its bounded range can cross an
observed interrupt or hardware-visible boundary. The production targets do
not link this registry, generated artifact, external GPGX bridge or any JIT.

M11.38 adds a generic instruction-boundary yield contract for only the four
previously rejected two-instruction ranges. Generated bodies remain mechanical
and return `BlockExit { next_pc, reason, instructions_executed }`; they call the
boundary callback after every instruction and may yield for event, interrupt or
trace handling. The shadow adapter compares fully materialized state at each
boundary, while GPGX retains interrupt service and scheduler ownership. A
continuation is dispatched at the exact next guest PC, so no multi-instruction
block is atomic and no candidate-specific timing or interrupt branch exists in
handwritten glue. The result is developer-only and does not widen production
dependencies.

M11.39 keeps the same boundary and adds a bounded post-M11.38 interpreter
profile. The profile ranks dynamic instruction executions, while exact decoder
ownership supplies candidate ranges; it does not infer functions or indirect
target sets. The selected `ADD.W (An)+,Dn` helper is handwritten semantic glue
covered by the independent test oracle, while `generated_blocks_m1139.cpp` and
the registry metadata are mechanical generator output. The profile writer and
registry remain in `tools/hybrid`; none of these files are linked into
production targets. Hardware-visible candidates remain fallback unless an
existing bridge already proves the ordered effect.

M11.41 keeps checkpoint identity in the same developer-only boundary. The
runner retains complete external serialize buffers as ignored raw evidence,
while `checkpoint_evidence` owns the format-guarded identity adapter. It clears
only proven host-pointer and ABI-padding representation spans from the known
GPGX state format before ordered hashing; semantic machine-state bytes remain
authoritative. No production target, generated block, ROM, asset or emulator
dependency is introduced.

### `platform`
Modern OS/window/input/audio/rendering integration.

This layer should remain isolated from game rules.

M11.18 adds the small `oasis_platform` adapter library. It owns Win32 window,
keyboard polling, focus handling and software-framebuffer presentation. Game
state remains in `oasis_core`; the adapter consumes controller snapshots and
packed pixels. Non-Windows builds retain a compile-only unavailable adapter
until a platform backend is separately justified.

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
Every human-maintained source/build file must remain at or below **500 lines**.
Prose documentation is exempt, as specified in `AGENTS.md`.

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
