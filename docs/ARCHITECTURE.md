# Architecture
## M11.58 raw-parameter behavior cluster

The existing `ParentSuffixMachine` is the minimal behavior-cluster boundary:
it composes `RamFlagRoutine` once, preserves each component's resumable token
space and exposes only supplied raw addresses/register state. Its exact
footprint is `FF0010`, `FF0011`, `FF0012`, `FF0013`, `FF0014`, `FF0016`,
`FF0628`, `FF06F2` and the A5-derived `FF001F..FF0021` range when the parent
base is `FF001A`. `oasis_core` owns byte effects and opaque continuations;
the parent/adapter owns the actual memory lifetime, ROM-PC provenance, GPGX
fetch/timing, hardware prefix, full SR, shared epilogue and RTS.

The data gate remains raw: fixed-window bytes have known external writers and
the A5-derived range has unresolved aliases and lifetime. No typed structure or
new cluster wrapper is justified. This is a portable behavior cluster contract,
not a portable subsystem boundary. See ADR-0039 and
`reports/PORTABLE_BEHAVIOR_CLUSTER_M11_58.md`.


## Target architecture

## M11.57 parent-owned suffix helper

`oasis_core` now contains `ParentSuffixMachine` and the minimal
`ParentSuffixContract`. It owns only the five safe-RAM SF writes, structural
RamFlag composition, resumable phase tokens and opaque parent handoff. The
developer-only hybrid adapter owns canonical ROM validation, GPGX fetch/timing,
nested BSR representation, and mapping the handoff to `0x611D6`. The helper
does not know the parent frame, SR restoration, hardware prefix, shared
epilogue or enclosing RTS. See ADR-0038 and
`reports/PARENT_SUFFIX_HANDOFF_M11_57.md`.

## M11.56 parent-frame boundary

The 0x604F0 RamFlag caller suffix is an internal tail, not an independently
callable routine. Its 0x611D6 destination restores 56 saved register bytes
and the parent's SR before returning to the original 0x60004 caller.
Full-SR restoration and parent hardware effects remain adapter concerns;
no third portable routine or new composition interface enters oasis_core.
The selected seven-instruction region ends at 0x60516; the previous 0x60520
decoder budget also contains the beginning of another parent arm.

CallerContinuationObserver is a bounded, opt-in, EMULATED-only read-only
tools/hybrid observer. It records the existing parent prologue through the
selected natural enclosing return; its local validator checks paired traces,
saved-frame ownership and ordered RAM accesses. It supplies evidence only
and never controls guest execution. Dependency direction and the two existing
authoritative core routines are unchanged. See ADR-0037 and
reports/RAMFLAG_CALLER_ROUTINE_M11_56.md.

## M11.55 caller/data boundary

M11.55 adds a developer-only `tools/hybrid` caller-attribution observer. It
records the natural `0x0604BC` entry context and proves one `0x0604F6` caller
region beginning at `0x0604F0` and one `0x060BCC` caller region beginning at
`0x060BC4`. The first region is corrected away from zero data at `0x0604EC`;
the second begins with a `MOVE.W` to hardware register `0x00A11100` before the
RamFlag call. Neither region is a closed portable routine.

The fixed subset of the raw `0x00FF0010..0x00FF0016` window is byte-proven at
offsets 0, 1, 2, 3, 4 and 6; offset 5 and A5-relative effects remain unknown.
No typed structure, caller owner or subsystem boundary is accepted. The
dependency direction is unchanged: core owns only already-proven portable
routine semantics/tokens, while hybrid owns ROM PC/opcode provenance, GPGX
hooks/timing, address provenance, hardware interaction and attribution output.

## M11.54 discovery boundary

M11.54 proves a RamFlag-centered call-graph/raw-memory cluster, but not a
portable subsystem. The two authoritative core routines remain separate
ownership islands because no shared proven structure or edge connects
TableCopyRoutine to RamFlagRoutine. The raw 0x00FF0010..0x00FF0016 window is
shared with bounded callers, but field lifetime, aliasing and meaning are not
closed. The 0x060BCC context also reaches hardware register 0x00A11100.

The current dependency direction remains explicit: oasis_core owns only the
portable TableCopy/RamFlag contracts and opaque continuation tokens;
tools/hybrid owns ROM PC/opcode provenance, GPGX continuation/timing/prefetch,
checkpoint/oracle tooling, accounting and hardware interaction. No typed
structure or subsystem owner is accepted by M11.54.

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

M11.50 also places the proven mechanical primitive contract in `core`. Its
executor knows only generic register roles, byte/word memory effects, CCR/X
rules, DBF low-word semantics, 32-bit address arithmetic and resumable
instruction-boundary results. Instruction tokens are opaque values supplied by
an adapter; core contains no ROM addresses, canonical opcodes, GPGX/libretro
types or checkpoint layout.

M11.51 adds the first complete routine-shaped boundary as
`TableCopyRoutine`. Its semantic implementation receives opaque entry,
continuation and destination tokens plus a portable register/memory machine;
the routine contains no ROM PC dispatch or instruction decoder. The
developer-only hybrid adapter supplies the `0x2D66..0x2D84` mapping, GPGX
prefetch/timing bridge and shadow comparator. The native replacement remains
proven after M11.52 closed the frozen checkpoint identity mismatch. The
generated/interpreter path remains the authoritative oracle/fallback for the
routine's shadow comparison and for all other paths.

M11.52 keeps the ownership split explicit: `oasis_core` owns only portable
routine semantics and continuation tokens, while `tools/hybrid` owns
GPGX-specific instruction fetch, IR/prefetch, cycle, refresh and RTS handoff
reconstruction. A native adapter must enter and finish each represented guest
instruction through the developer-only bridge; a lump-sum routine timing
update is not a valid continuation. Checkpoint canonicalization remains an
identity proof for host representation only and is not a native promotion
escape hatch.

M11.53 adds RamFlagRoutine as a second isolated core contract. It owns only
structured BSET/Scc/LEA/RTS semantics, opaque tokens and resumable boundaries.
The 0x604BC adapter owns canonical bytes, ROM addresses, GPGX prefetch
seeding, per-instruction timing/refresh, block-hook boundary return and return
state. The shared registry allows deterministic coexistence with
TableCopyRoutine; the two core contracts have no shared candidate state or
address assumptions.

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

The M11.41 checkpoint identity adapter and the M11.42 restart evidence remain
strictly developer-only hybrid tooling. They do not alter the native runtime,
introduce an emulator dependency into production, or authorize interpreter
coverage promotion while serialized host representation remains unresolved.

M11.43 keeps this boundary explicit with `gpgx_checkpoint_layout`: pinned
GPGX wholesale-struct layout models and representation spans are isolated in
the hybrid contract library, while `checkpoint_evidence` owns only raw-preserving
hashing and evidence files. The layout table is generated from mirrored field
offsets and ABI assertions; production `oasis_core` and generated game blocks
do not depend on it.

M11.44 adds `interpreter_ledger`, a developer-only consumer of the existing
profile writer and `re_slice_decoder`. It records one evidence row per dynamic
interpreter PC and closes counts exactly; it does not infer indirect CFGs or
hardware behavior. The promoted 0x03A7AE block is emitted by the existing
decoder-to-C++ generator into `generated_blocks_m1144.cpp`; registry metadata
and the generic `basic_block` boundary glue remain separate handwritten
integration code. The final interpreter remainder stays fail-closed for
unverified semantics, unresolved register-based memory and 0xA00003 hardware.

M11.47 extends this developer-only boundary with shared safe-memory semantic
helpers and generated candidate translation units. `recomp_generator` owns
decoder/exact-IR to C++ emission and canonical ROM provenance;
`generated_block_runtime` owns reusable instruction semantics;
`generated_blocks_m1147.cpp` contains generated bodies; and
`generated_blocks_m1147_registry.cpp` contains generated metadata. The
handwritten `basic_block` registry remains generic boundary glue. Primitive
discovery records only exact mechanical copy/clear structure and does not add a
native higher-level replacement or hardware dependency.

M11.48 adds `mechanical_primitive.hpp/.cpp` as a second, still developer-only
layer above the generated instruction blocks. `MechanicalMachine` exposes only
registers, byte/word memory effects, instruction timing hooks, canonical
displacement fetch and boundary reasons; it has no GPGX types or ownership.
`execute_memory_clear` is generic resumable loop semantics, while the registry
is handwritten dispatch/measurement glue containing only the proven loop
metadata. The native adapter maps that interface to `BasicBlockApi`; the shadow
adapter runs a detached snapshot. Each dispatch yields after the current guest
instruction and resumes from the body or DBF PC, so a loop is never an atomic
instantaneous operation. Primitive shadow runs beside the generated/basic-block
oracle, which remains responsible for the full decoded IR/prefetch comparison
and remains the fallback path. This layer is not linked into production.

M11.49 closes the mechanical family using the same developer-only boundary.
The hybrid registry carries canonical body/DBF opcodes, ROM PCs, register roles,
width and continuation metadata; its executor previously contained the generic
resumable body/DBF semantics. Copy remains ordered read-before-write and clear
uses the exact width-specific bus operation. Unsupported operation, width,
displacement and odd-word alignment forms fail closed. Generated M11.47 bodies
remain generated oracle/fallback code.

M11.50 extracts that generic semantic ownership into
`src/core/mechanical_primitive.*`. `oasis_core` owns the portable
`MechanicalMachine`, `MechanicalLoopContract`, `PrimitiveContinuation` and
`PrimitiveExit` contracts plus the single executor implementation. The hybrid
layer owns only the four ROM registry entries, canonical opcode/displacement
validation, GPGX/BasicBlock timing and prefetch adapters, detached shadow
snapshot, comparison, metrics and reporting. The adapter maps hybrid boundary
reasons into the core enum and maps core instruction steps back to the
canonical ROM encodings. A standalone core test links without hybrid,
GPGX or libretro, and CMake plus a source scan enforce the reverse-dependency
boundary.

M11.46 adds `address_provenance` as a separate developer-only observer around
the same GPGX hook and block-registry path. It records runtime fallback PC
counts, top-level data-bus address/width/direction/order and A0–A7 transitions,
then classifies observed addresses against the existing Genesis map. It has no
dependency from `oasis_core`, generated gameplay blocks or handwritten native
game logic. The observer resolves memory evidence only; semantic eligibility,
hardware ordering and generator promotion remain independent gates.

M11.45 keeps this boundary and adds the generated
`generated_blocks_m1145_*.cpp` translation units containing only decoder-owned
candidate bodies and registry fragments. Handwritten runtime helpers in
`generated_block_runtime.cpp`
implement exact operation/size/addressing-mode contracts; the generator emits
the address-specific calls and canonical ROM-byte provenance. The handwritten
registry glue combines the historical and M11.45 spans. Failed timing/flags
candidates, unresolved register-based memory, indirect CFG, decoder gaps and
0xA00003 hardware remain outside the promoted set.

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
