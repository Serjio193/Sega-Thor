## THOR Evidence Engine design boundary

### Ephemeral graphics resource transaction

`graphics_resource_transaction.py` reduces one bounded observation into a
deterministic resource receipt. It requires the same ROM, run and frame across
the completed `0x3820` call, RAM-to-VRAM publication, SAT tile references and
visible pixels. The receipt is derived runtime evidence, not a second map; it
does not mutate OASROMP1, canonical SQLite, or `SOURCE_OWNED`. Exact same-frame
CRAM palette bytes are retained as a dependency and are not treated as ROM
provenance.
Until emulator instrumentation supplies those linked observations, no live
resource closure is accepted.

The graphics runtime bridge also exposes a separate caller-scoped diagnostic
record for `0x03C07C` → `0x3820`. It checks the immediate JSR and matched stack
return, records entry register context, and accepts an exact physical ROM
boundary only after byte mapping succeeds. A missing target call does not
contribute coverage or change canonical ownership.

The separate bounded `graphics_caller_census` buffer is discovery-only and
does not mutate the graphics transaction ledger or ROM-property authority.
The native `gpgx_graphics_transactions_json` export now wraps the existing
transaction JSON with `caller_census` and row/serialized-byte diagnostics;
the existing managed and Lua string bindings remain unchanged. A host-only
synthetic pipeline test passes. The diagnostic install must keep both raw and
compressed (`.wbx.zst`) core artifacts in sync. Live 1,800-frame acceptance
passes when the refreshed compressed core is loaded; the capture runner treats
the receipt as completion and then closes the still-open EmuHawk process.
Caller-function identity remains unresolved until independently verified; a
return-derived call-site candidate is explicitly unconfirmed.

## M14.2 canonical global evidence authority

The canonical `knowledge.sqlite` generation remains the only knowledge-map
store. Its `emission` table is the ROM partition and `SOURCE_OWNED` authority;
`rom_range`, `rom_object`, `claim`, `relation`, `evidence_ref`, and `conflict`
hold evidence. M14.2 adds derivation/input and map-proposal/operation tables to
that schema. Proposals bind to a parent generation, base map hash, graph hash,
and validator version, but do not directly alter emission or establish
ownership. Runtime occurrence identity must retain capture, epoch, CPU,
address-space/domain, native sequence, and event kind. MASTER V2 preserves the
added tables when serializing and materializing canonical knowledge.

## Structural consumer-family evidence plane

`src/tools/thor_evidence/static_family_evidence.py` emits the independent
`STATICALLY_ENUMERATED_CONSUMER_SOURCE` ledger for exact members of a closed
selector/table/consumer contract. It binds ROM identity, contract entries,
source-artifact hashes, and the exact physical range union. Its derived view
supports deterministic replay, overlap-aware union accounting, and independent
contribution removal.

This plane does not write or overlay the runtime property bitmap, contribute to
`TOTAL_PROVEN`, update canonical SQLite, or affect `SOURCE_OWNED`. It does not
claim that every entry executed in a runtime scenario and assigns no graphics
or audio semantics. Runtime-causal classes such as
`AUDIO_STRICT_CAUSAL_SOURCE` and `GRAPHICS_VRAM_CAUSAL_SOURCE` remain separate
and unchanged.

Variable-length families use the versioned
`static_family_variable.py` contribution model. A family owns root records,
exact descriptor records, and half-open stream intervals linked by explicit
`root -> descriptor -> stream` provenance edges. Each resource carries its
consumer/decoder contract, boundary evidence, ROM identity, and source-artifact
hashes. `gfx_screen_roots_structural_closure.py` builds the 159-member
`GFX_SCREEN_ROOTS` family from identical Debug/Release decoder censuses and
replays contribution removal against the prior structural view. This extends
only the structural evidence plane; the fixed-size v1 validator remains
unchanged so its accepted source hashes continue to verify.

Selector-indexed descriptor tables use
`static_family_table_variable.py`. The schema records the complete selector
domain and descriptor census, filters family members by exact descriptor
fields, and binds each root→descriptor→stream path to a shared parser boundary
rule. For `AUDIO_MODE0_BANK_RESOURCES`, the descriptor's little-endian length
is loaded by the normal Z80 parser and the decoder loop exits when its byte
counter reaches zero. This closes exact static half-open intervals without
claiming runtime execution of every selected entry.

Signed-relative sentinel lists use the versioned
`static_family_sentinel_list.py` contribution model. Each instance carries its
source table references and selector slot, resolved signed-relative target,
record width/count, sentinel key location, included sentinel-byte count, ROM
identity, and source hashes. The shared view dispatcher validates this schema
without merging it into runtime properties or ownership. `AUTO13_BCEA` closes
its finite slot census through independent regeneration and sentinel-bound
interval replay.

Absolute pointer tables with decoder-terminated streams use the separate
`static_family_pointer_table.py` v4 model. It records every table slot,
big-endian target mapping, exact decoder-consumed end, source hashes, and the
table-entry→pointer→reader→decoder→stream provenance chain. The
`GFX3820_INDEXED_POINTER_TABLE` adapter regenerates this census twice and only
admits all 107 non-null entries when both canonical enumerations agree. Its
view builder validates and unions the existing v1/v2/v3 contributions alongside
v4, so contribution removal rebuilds the prior view without changing any other
evidence authority.

Descriptor-selected variable-length streams use
`static_family_descriptor_stream.py` v3. Each resource stores its exact
descriptor and stream half-open intervals, decoder-consumed end, source hashes,
and explicit root→descriptor→stream→consumer→boundary provenance. The
`GFX_DESCRIPTOR_STREAMS_AUTO49` adapter checks the descriptor pointer field and
regenerates every end with independent Python and native decoders. The schema
is structural-only and does not touch runtime properties, `TOTAL_PROVEN`,
`SOURCE_OWNED`, or canonical SQLite.

## AUTO65 multi-chain campaign boundary

`src/tools/thor_evidence/auto65_chain.py` makes a normalized causal chain the
primary investigation unit. Nodes and typed edges are compared structurally;
the SHA-256 fingerprint is only an index. `auto65_campaign.py` selects a new
scenario only after the persisted AUTO64 graph reaches a fixed point, ingests
one cheap broad discovery, and creates a bounded investigation DAG from the
observed chains.

The campaign reuses a known prefix and schedules only a new tail. Exact
replays become `KNOWN_NEW_INSTANCE` and are cancelled before static/runtime
work. Shared prefixes are clustered, their dependency proof is reused by all
children, and one capture records every investigation that consumed it. Static
evidence is queried before a focused capture; an equivalent capture is rejected
by a fingerprint containing scenario, start state, input range, watch set,
obligations, and evidence lineage. Closure is fail-closed: reset/init roots,
bounded ROM/source facts, or explicit `BOUNDED_UNRESOLVED` frontiers are
required; last-writer or equal-value observations are not sufficient.

This is developer-only M12 tooling. It does not write SOURCE_OWNED, production
runtime code, ROM/assets, or a general emulator, and it persists campaign state
under ignored `build/thor-evidence/auto65`.

## AUTO66 multi-scenario campaign boundary

`src/tools/thor_evidence/auto66_campaign.py` reuses AUTO65 chain identity and
investigation primitives while adding a repository-driven scenario planner.
It fingerprints the complete deterministic scenario file, rejects equivalent
captures before launching BizHawk, emits `NEED_NEW_SCENARIO` at AUTO64/AUTO65
fixed point, ranks the remaining scenario pool from coverage/frontier and
scenario metadata, and persists the automatic transition and per-scenario
metrics. It may consume a sealed report for deterministic receipt regeneration,
but does not treat that replay as new runtime evidence. The tool remains
developer-only and cannot write SOURCE_OWNED or production runtime state.

## AUTO67 live opportunistic RE boundary

`src/tools/thor_evidence/auto67_live.py` is the single developer-only live
launcher and Dispatcher for human gameplay sampling. It owns a fixed rolling
runtime window, one atomic claim authority, bounded per-worker mailboxes and
configurable worker counts of 1/2/4/8/16/32/64. It intentionally has no raw
event FIFO: when workers are busy, Lua overwrites unclaimed observations and
the Dispatcher later chooses from the current window. Lua callbacks only
normalize and append minimal events; static/provenance work remains outside
BizHawk. The same launcher publishes a bounded status snapshot and serves the
operator dashboard, which reads snapshots without participating in claims,
workers, or persistence. `live_opportunistic.lua` is developer-only and never
promotes ownership or mutates guest state.

Operator performance repair: `auto67_status.py` runs file publication in a
separate daemon using one replaceable scalar-status slot and a nonblocking
try-lock to copy at most 16 recent investigations plus 24 transitions per
worker. HTTP reads an immutable byte snapshot. Slow UI/disk updates are dropped;
full investigation records stay with the dispatcher and its final artifact.
The default operator view is now `auto67_window.py`, a separate native Tk child
process consuming the same replaceable snapshot file. It renders metrics, workers and
bounded transition history without HTTP, raw events, claims or persistence.
Browser serving remains an explicit compatibility mode (`--view-mode browser`).
`--capture-mode burst` explicitly opts into sparse discovery (64 callback
entries every 30 frames, removing the hook between bursts). The default
`continuous` preserves the prior every-16th-write sampling policy. Neither
policy proves causal completeness. The Worker keeps only the bounded
`recent_investigations` deque (16 entries); it does not retain an unbounded
investigation history or decide global novelty, known, proven, duplicate, merge,
or reject status. Worker results are factual outcomes such as
`EVIDENCE_OBSERVED`, `CAPTURE_UNAVAILABLE`, `DECODE_FAILED`, and
`EVIDENCE_MATERIALIZED`. Persistence status is a compatibility value only and
the downstream sink cannot block return to `IDLE`.

AUTO67.2 separates continuous hunting from scarce focused capture. Free workers
periodically scan the current rolling window and may claim fresh evidence
without a capsule; a focused BizHawk capture is optional and bounded by the
configured slot count (default one). Capture-slot exhaustion therefore does
not block quick checks or analysis, and no old-event queue is introduced.
Worker stages distinguish `HUNT`, `QUICK_CHECK`, `WAITING_CAPTURE_SLOT`,
`CAPTURING`, and `ANALYZING`; a returned worker immediately hunts again.

AUTO67.3 attaches `auto67_persistence.py` to the same existing SQLite evidence
sidecar as a persistent worker-chain store. Worker completion submits one
immutable bounded chain descriptor through `put_nowait`; one background writer
transactionally inserts the SHA-256 of its canonical causal payload or updates
exact-duplicate metadata. Frame, epoch, worker and lease are provenance only
and cannot create a new chain hash. The live path does not infer relations,
structure, ownership or SOURCE_OWNED from a seed; the former knowledge-yield
analyzer is outside the acceptance path. The native view reports only runtime
health plus objective chain-store growth (`unique`, `session new`, exact
duplicates, writes, errors, unresolved and rooted). The sidecar is disabled
unless `--chain-db` (legacy alias `--knowledge-db`) is supplied.

### AUTO67.occurrence-only Dispatcher semantics

The Dispatcher leases only current, unleased `RollingWindow` items to free
workers. Branch and context fingerprints are not mailbox or Worker semantics;
`REJECT_ACTIVE_CLAIM`, `active_collisions`,
and `investigation_merges` remain obsolete compatibility fields and stay zero
on this path. The exact stored item is protected by its
`dispatch_state=LEASED` marker, and each worker has one mailbox and one
`LEASED`/`WORKING` task at a time.

Runtime occurrence identity comes from the capture source's monotonic
`epoch + seq` pair (`live_opportunistic.lua`). The Dispatcher adds a bounded
`window_item_id` for the exact current-window object. Investigation and lease
identities include that runtime occurrence identity, while persistence may
still aggregate identical semantic chain hashes later; the Dispatcher does not
perform that merge. Capsule limits remain `CAPSULE_COUNT=16` and
`MAX_LIVE_CAPTURES=4`; exhaustion produces the existing
`WAITING_CAPTURE_SLOT` path, not branch suppression. There is no raw-event
backlog or per-worker queue.

### AUTO67 minimal Worker mailbox

Each worker mailbox contains one detached message with `event`,
`investigation_id`, `lease_id`, optional `capsule_id`/`capture_status`, and a
bounded `dispatch_trace`. The event copy is authoritative for
`epoch`, `seq`, `occurrence_id` and `window_item_id`; mailbox-level duplicates,
worker IDs, seeds, branch/context values and capsule lease aliases are absent.
`investigation_id` is derived only from occurrence and window-item identity.
The same `lease_id` is used for capsule wait, decode, predecessor decode and
persistence provenance. Worker diagnostics retain factual worker and
occurrence IDs plus bounded materialization counts.

### AUTO67 Cartographer seam

The Worker materializes capture data into an explicit local-chain result and
submits it through the existing bounded `LiveMapSink` queue when `--map-db` is
enabled. Without that option the Worker still records local diagnostics and no
output queue is created. The
downstream `auto67_cartographer.py` adapter accepts only the existing
predecessor-produced `REGISTER_REACHING_DEFINITION` step when its producer and
consumer PCs, A4/A5 register, occurrence epochs and complete/non-intervening
interval evidence are valid. It emits stable global `ROM_INSTRUCTION` nodes
and a `REGISTER_REACHING_DEFINITION:<register>` edge, then calls the existing
MAP-1 `Cartographer.merge()`; runtime occurrence and lease identities remain
local provenance only. All accepted steps in one chain are canonicalized into
one stable bundle before merge. Unproven chains stay diagnostics and produce no
map frontier. The `LiveMapSink` writer thread creates an explicit in-memory
SQLite Cartographer, consumes the bounded local-chain queue, and snapshots
only cached metrics. On shutdown it backs up that RAM graph to the retained
session-map SQLite file; the canonical GLOBAL map is not opened during
gameplay. After the writer stops, `auto67_runner.py` invokes the deterministic
offline merger, which copies GLOBAL to `*.merge.tmp.sqlite`, imports the session
under `session-map:<session_graph_hash>`, validates the graph hash, fsyncs and
atomically replaces GLOBAL. A failed merge leaves the original GLOBAL bytes
untouched.

### AUTO67 current-event ring boundary

The Python `RollingWindow` is a current-event container only: it owns
`capacity`, a `deque(maxlen=capacity)` and an `overwrites` count. It does not
track proof, semantic retention, Worker results, novelty, Cartographer state or
frontiers, and it exposes no dead convenience API. Its snapshot reports only
`capacity`, `utilization` and `overwrites`; `dispatch_state=LEASED` remains on
the exact stored item as the DISPATCHER-1 redispatch guard.

The Lua `live_opportunistic.lua` ring is a separate transport window across the
file/poll boundary. It owns `ring_start`, `ring_count`, `capacity` and
`overwritten`, emits current events in order, and preserves `epoch`, `seq` and
`occurrence_id`. The two rings are intentionally separate and neither is a
pending-event FIFO or an unbounded backlog. The predecessor/prehistory ring is
outside this boundary.

### AUTO67 pre-dispatch transport boundary

`auto67_transport.py::PreDispatchTransport` consumes the replaceable Lua
status snapshot before Python calls `Dispatcher.ingest`. It keeps one identity
cursor `(epoch, seq, occurrence_id)`, synthesizes the occurrence identifier
only for legacy records that omit it, and returns unseen records directly to
the caller. Duplicate, stale, or malformed records are counted and dropped;
raw events are never retained in a transport queue. The legacy `events` and
`discovery` keys are input aliases only. Transport validation does not inspect
known/proven/frontier state and does not choose or claim work.

The same transport cursor consumes the final Lua snapshot after the emulator
process exits and before `Dispatcher.stop()`. This preserves final-only events
while keeping shutdown bounded by the existing worker stop event and capsule
wait timeout.

The live-opportunistic launcher path has no `--state` consumer: its Lua source
does not load a savestate or read `OASIS_LIVE_STATE`, so the dead runner option
and export were removed. Other capture scripts with their own state contracts
remain independent of this path.

## AUTO67.4 frozen capsule worker materialization

`capture/live_capsule.lua` writes versioned O67V v2 capsules with a 24-byte
physical header and 20-byte records that preserve the observed event kind;
the older O67C format remains decoder-readable for historical evidence. The
bounded worker decoder validates magic, version, capsule id, lease id, count,
logical length, physical boundaries and truncation before materialization.
Workers receive separate `runtime_observations`, `causal_facts` and bounded
register/capture diagnostics. Live Worker materialization deliberately omits
`unresolved_frontier` and any prescriptive `next` action; unresolved evidence
is represented by factual `register_provenance.status`, `reason`, `unresolved`,
and `capture_diagnostics`. The existing bounded persistence queue receives only
completed descriptors and preserves occurrence provenance (`epoch + seq`,
`window_item_id`, lease and worker) while downstream canonicalization remains
the only semantic deduplication point. No ownership promotion, SOURCE_OWNED
change, raw-event backlog or AUTO68 dependency is introduced.

## AUTO67.5 observed/causal integrity gate

Materialized schema version 2 separates direct `observed_facts` from
`causal_facts`. Capsule BUS_WRITE_PC records are observation-only; temporal
adjacency, shared addresses and record ordering never create a dependency.
The worker may emit a causal fact only when the bounded static M68K decoder
proves instruction operand/destination semantics. Register/RAM provenance is
kept in factual diagnostics, and `chain_steps` remains empty until an
ordered dependency is independently established. Historical AUTO67.4 rows
remain readable and are not migrated or rewritten.

## AUTO67.6R2 continuous prehistory and register provenance

`auto67_predecessor.py` decodes versioned O67P evidence and resolves only an
exact epoch/sequence/PC register reaching definition. The R2 Lua side installs
one continuously active global `event.on_bus_exec_any` hook, stores compact
epoch/sequence/frame/PC records in a bounded 4096-entry ring, and freezes a
bounded slice at the exact joined BUS_WRITE consumer. A4/A5 values are read
only at that consumer occurrence. Discovery and execution sequence numbers are
joined explicitly; they are not equated. O67P v2 adds ring capacity, wrap,
consumer PC and exact-join metadata while retaining the 28-byte record shape.

BizHawk 2.11.1 supplies a bus value as the second global execution callback
argument rather than a reliable opcode. Therefore R2 uses the runtime PC,
epoch and sequence as execution evidence and decodes instruction bytes from
the canonical ROM during worker resolution. This is recorded as
`producer_opcode_source=STATIC_ROM_PC`; it does not invent a producer PC.
Unsupported register writers, gaps, epoch mismatches and missing consumers
remain fail-closed frontiers. The canary proved A5 from `LEA $00FF134C.L,A5`
at `0x002234` and A4 from `LEA $00C00004.L,A4` at `0x0027BE` into consumer
`0x0027EC`.

The real R2 proof also exposed a runtime limitation: the global Lua hook makes
all 120 sampled frames 50 ms or slower (maximum 679 ms), including frames
before the first lease. Increasing the ring would not address this callback
cost and would increase snapshot work. This remains developer-only and adds no
semantic merging, SOURCE_OWNED change, graph expansion or AUTO68 dependency.

## AUTO67.1 fixed capsule experiment

`auto67_capsule.py` extends the same Dispatcher with exactly sixteen reusable
128 KiB capsule slots and a maximum of four simultaneous targeted captures.
`capture/live_capsule.lua` keeps only a 256-entry discovery ring and targeted
capture metadata in replaceable status snapshots. Capsule commands are a
bounded replaceable control stream; capsule records are frozen to binary only
after capture and never enter the UI payload. Target hooks are budgeted per
frame and removed until the next frame, so a hot PC cannot turn a capsule into
an unbounded callback load. The capsule path remains developer-only and
lossy/causal-incomplete by design; it does not write SOURCE_OWNED or invoke
the production runtime.

ADR-0044 specifies a proposed developer-only M12 evidence sidecar around the
existing controlled BizHawk harness. Immutable capture events, temporal
byte/register versions and scoped relations are distinct models. Python/SQLite
analysis exports non-owning evidence into existing Carver IntervalDB views;
the existing promoters and exact ROM reconstruction remain authoritative.
No runtime/core dependency or executable engine is added by this design.
See [architecture contract](reports/THOR_EVIDENCE_ENGINE_ARCHITECTURE.md) and
[bounded stages](reports/THOR_EVIDENCE_ENGINE_STAGES.md). Milestone status and
the M12 → M13 parity → M14 C++ sequence remain unchanged.

## M11.64 G0 boundary closure

M11 closes the bounded G0 evidence line without creating a portable subsystem.
`A5 = 0xFF001A` has parent-owned natural lifetime through `0x06027E`/`0x060284`;
`0x0623AC` exposes six natural VDP byte-write sites and therefore a hardware
portability boundary; overlapping raw access and unresolved external writers block
typed-data ownership. Static indirect edges and `0x060286` remain explicit debt.
All ROM-PC, GPGX, hardware, timing and parent-continuation facts stay in
developer-only hybrid evidence. See `RE_LEDGER.md` and the M11.64 report.

## M11.63 bounded 0x0623AC callee evidence

M11.63 adds a developer-only observer for the natural callee rooted at
`0x0623AC`. It records four parent continuations, exact A5 preservation,
all natural nested direct/indirect calls, path signatures, ordered effects and
hardware interaction. The static transitive slice retains `INDIRECT_CFG` at
`0x062878`; natural targets are closed, but VDP writes at `0xC00011` keep the
portable call-effect gate blocked. No production/core boundary, typed data,
transaction or subsystem is introduced. See
`reports/CALLEE_0623AC_CONTRACT_M11_63.md`.

# Architecture
## M11.62 bounded 0x061934 callee evidence

M11.62 adds a developer-only observer for the natural callee rooted at
`0x061934`. It records parent entry/return distribution, direct `A5` consumers,
all nested direct/indirect calls, A5 equality and complete natural effects.
The static transitive slice retains `INDIRECT_CFG` at `0x061F60`, while the
bounded natural target set is closed. No production/core boundary, typed data,
transaction or subsystem is introduced. See
`reports/CALLEE_061934_CONTRACT_M11_62.md`.
# Architecture
## M11.61 bounded 0x062AE0 callee evidence

M11.61 adds a developer-only observer for the natural callee rooted at
`0x062AE0`. It proves exact A5 entry/exit equality and records the complete
natural data effects without importing ROM/GPGX state into `oasis_core`. The
static CFG retains an indexed indirect call at `0x062CEC`, so this evidence
does not create a portable callee, transaction or subsystem boundary. The
parent-owned G0 lifetime and raw typed-data gate remain unchanged. See ADR-0042
and `reports/CALLEE_062AE0_CONTRACT_M11_61.md`.

## M11.60 bounded A5 lifetime

M11.60 adds only an opt-in, EMULATED-only developer observer under
`src/tools/hybrid`. It records the raw A5 generation rooted at `0x060182`,
selected register-relative consumers, call handoffs and the parent-owned
`0x06027E` restore. The enclosing region remains parent-owned; no typed shared
memory, subsystem or `oasis_core` boundary is introduced. The existing raw
ParentSuffix + RamFlag cluster and its storage remain separate alternate
producer/consumer paths. See ADR-0041 and
`reports/A5_CONSUMER_LIFETIME_M11_60.md`.

## M11.59 raw ownership boundary

M11.59 keeps the first behavior cluster raw and parent-parameterized. The
all-ROM provenance census proves that the `FF001A` base escapes to the stack,
is reloaded and is post-incremented across a broad range; fixed bytes also
have multiple bounded writers. Therefore no typed shared-memory structure or
new `oasis_core` owner is justified. The developer-only decoder provenance
test records address modes and widths without importing ROM PCs or GPGX types
into core. The 0x60BCC hardware prefix and sibling continuation remain
adapter-owned. See ADR-0040 and `reports/RAW_DATA_OWNERSHIP_M11_59.md`.

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

The GFX3820 externalization prototype extends `resource_loader` to stable
ResourceIDs 1..107 using the accepted exact parser-bounded family manifest at
`assets/gfx3820/manifest.tsv`. It checks optional `<id>.bin` compressed copies
against pinned size/hash, requires full decoder consumption and the pinned
4,096-byte decoded hash, and falls back to the canonical ROM stream on any
override rejection. Explicit `replacements.tsv` rows form a separate
hash-pinned modified-resource mode. These backend/migration statuses are
portability evidence only; they do not alter runtime, structural, SOURCE_OWNED
or canonical SQLite authority and assign no gameplay/visual semantics.

The separate screen-root catalog at `assets/gfx3820_screen_roots/manifest.tsv`
preserves three identities: root path, descriptor address, and unique physical
stream. Its stable file ID is `STREAM_<ROM start>`: 21 roots expand to 167
root/descriptor uses, 163 unique descriptors, and 159 unique streams. The
`screen_resource_loader` resolves paths to streams, checks the pinned manifest
and ROM/source census identities, and reuses `decompress_graphics`. Exact
compressed copies are optional files under `assets/gfx3820_screen_roots/streams`;
missing or invalid copies fall back to the exact ROM range. Decoded sizes vary
from 6,212 to 46,316 bytes. This is a separate portability catalog; it makes no
semantic or natural-selection claim and changes no runtime, structural,
SOURCE_OWNED, or canonical SQLite authority.

The fixed-count record backend is a third, independent portability catalog at
`assets/fixed_count_records/manifest.tsv`. It keeps all 96 active table row IDs
and their explicit aliases while resolving to 78 stable `STREAM_<ROM start>`
files. A shared bounded parser validates the big-endian inclusive count and
exact `2 + 6 * (count + 1)` extent; exact raw copies are optional files under
`assets/fixed_count_records/streams`. Missing, wrong-sized, malformed, or
hash-mismatched files fall back to the canonical ROM span. This catalog does
not assign object/animation/map semantics or alter any ROM evidence authority.

`audio_format_a` is a bounded native translation candidate for the W5 Format-A
Z80 sample decoder. It parses the eight fixed bank descriptor tables into
selector IDs (bank index × 16 + entry), then expands exact length-bounded Mode 0
or Mode 1 byte spans into unsigned PCM samples. The API is not yet wired to the
game's sound-command/playback dispatch. Differential validation is a manual
local-ROM reference step against the independent W5 Python implementation;
the 18-sample W4 DAC witness remains the only runtime sample oracle used here.

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

M12-CARVER-1 adds a developer-only orchestration layer under `src/tools`.
`m12_carver.py` imports the existing complete ROM manifest into one exact
half-open interval database, attaches typed evidence, maintains a directed
provenance graph, and reports UNKNOWN gaps and deterministic campaigns. Its
adapters consume existing evidence-producer reports only. Carver Stage 1 does
not replace a promoter, reclassify a manifest range, create SOURCE_OWNED bytes,
or enter `oasis_core`; conflicts are blocking review records and all ROM and
generated evidence inputs remain external.

M12-CARVER-2 adds `m12_carver_expansion.py` as a developer-only graph-guided
evidence pass. It consumes the confirmed field1 table contract and existing
graphics census, creates typed pointer/resource nodes and candidate evidence,
and records rejection reasons for unclosed raw-resource boundaries. It does
not modify the manifest, replace promoters, or create a runtime dependency.

M12-CARVER-3 adds a developer-only global sweep over the same IntervalDB. It
discovers recognized canonical-ROM evidence, retains artifact repetitions while
deduplicating identical payloads for ingestion, aggregates typed ROM
read/execute spans, and records scenario/checkpoint coverage, parser/consumer
clusters, destination statistics and one blocker class for every remaining
UNKNOWN range. Runtime observation remains non-owning; only existing exact
promotion contracts can change the manifest. Its report is
`docs/reports/THOR_ROM_CARVER_M12_GLOBAL_SWEEP.md`.

M12-CARVER-4 adds a developer-only static consumer recovery pass. It consumes
existing re-slice, xref, candidate, table and M12 promotion artifacts globally,
emits typed code/table/parser references and family-level blocker records, and
reconciles them through the existing IntervalDB fixed point. It never writes
manifest ownership, repeats runtime capture, or replaces an exact promoter.
Its report is `docs/reports/THOR_ROM_CARVER_M12_STATIC_CONSUMER_RECOVERY.md`.

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

M12-CARVER-5 keeps whole-ROM format/container reconstruction in developer-only
tools. `m12_carver_format_reconstruction.py` consumes the canonical manifest,
the local ROM, and previously recorded M12 evidence, then writes typed
fingerprints, bounded grammar candidates, duplicate provenance, and fixed-point
IntervalDB metadata. It never writes manifest ownership: existing exact
promoters remain the only ownership writers, and decoder/entropy/structural
heuristics cannot become SOURCE_OWNED without their exact contracts.

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

## M12 MAP-1 persistent provenance boundary

`src/tools/thor_evidence/cartographer.py` is the first generic persistent global
provenance graph. It is separate from the historical AUTO67 `live_chain` database
and from the in-memory V3/V4 domain graphs. The bounded import path is
`map_sources.py` -> `Cartographer.merge()` -> MAP-1 SQLite. Static entities use
canonical kind/key/scope identities; runtime occurrences and value versions retain
frame/epoch scope. Only explicit PROVEN edges contribute to map metrics. OBSERVED,
unresolved frontiers, and conflicts remain separately stored.

The importer consumes existing CPU/register, SAT/DMA, graphics/resource, and
selector/control JSON evidence. It does not capture events, throttle sensors,
infer causality from adjacency, or write `SOURCE_OWNED`. The map hash is derived
from deterministic sorted graph rows, so replaying the same corpus is a zero-delta
operation.

## M12 AUTO67 live-forward Worker 1A boundary

`tools/bizhawk-native-ring/live_forward_trace.c` and
`live_forward_worker.c` implement a developer-only shared native M68K execution
stream and a single live-forward capture descriptor. `live_forward_flow.c`
classifies the supported 68000 control-flow outcomes. One 4096-record ring is
written from the main M68K execution hooks; a Worker attaches at the next
instruction boundary, snapshots ENTRY state, and copies its exact range into a
preallocated immutable result when it ends. Lua reads completed results in bulk;
there is no per-instruction Lua, managed, or Python callback.

The prototype applies to the BizHawk 2.11.1/GPGX main M68K cartridge path. It
does not instrument the separate Sega CD sub-CPU, does not call Cartographer,
does not require predecessor resolution, and is not linked into `oasis_core` or
the production AUTO67 runner. A checked FLOW_V1 result means only that the
recorded ENTRY, execution range, EXIT, identity, and termination are factual;
it creates no reaching-definition or global provenance claim. The source
patches and acceptance evidence are recorded in
`docs/reports/THOR_M12_AUTO67_LIVE_FORWARD_WORKER_1A.md`.

## M12 AUTO67 live-forward Worker 1B measurement boundary

Worker 1B scales capture descriptors around the same single native M68K
stream. A bounded pending queue admits at most one new capture at a confirmed
instruction boundary, and an ordered active queue retires completed windows;
the instruction hook never writes a separate stream per Worker. A checked
allocation plan accounts for descriptors, private result slots, queues, the
shared ring and a two-pass result wave before the test runtime configures its
Waterbox heap. Natural depth-20 measurements and forced-concurrency depth
measurements are reported separately. Each tested `WORKER_COUNT` must finish
100 serialized lifecycle rounds for every slot. A round is accepted only after
each Worker enters the current epoch, emits a fresh capture ID and generation,
reaches COMPLETE and ANALYZING, passes a complete host-side FLOW_V1 byte audit,
receives its exact ACK, and returns to FREE before the next request. The gate
requires `WORKER_COUNT * 100` completed and validated segments; starting the
pool or observing one successful round is insufficient. Per-slot native
counters prove capture start, completion, first analysis transition and exact
release. The host compares both binary exports and ENTRY/EXIT states after the
CPU stream advances, before it accepts the segment. This remains developer-only
and does not feed Cartographer or production AUTO67.

## M12 AUTO67 live-forward RAM Cartographer 2A boundary

The developer-only `live_forward_cartographer_runtime.py` reuses the Worker
1B runner's completed-segment callback. The callback runs after the independent
host audit has compared immutable metadata and both record exports, and before
the exact ACK is written. The runner independently audits per-Worker lifecycle
receipts. It translates only those
validated FLOW_V1 records into `OBSERVED` M68K instruction/event nodes and
ordered `EXECUTED_NEXT` edges. Stable ROM-scoped structure deduplicates overlap;
per-segment lineage retains run, epoch, Worker, capture, generation, segment
hash, stream/instruction bounds, profile and control-flow outcomes, plus the
occurrence count and bounds for repeated structure. To keep ACK latency bounded,
lineage rows are staged in the same RAM SQLite during runtime, then folded into
MAP-1 rows and hashed once after EmuHawk closes. No dependency,
reaching-definition or SOURCE_OWNED claim is derived.

Each EmuHawk run owns a new SQLite `:memory:` Cartographer and never loads the
master. After EmuHawk exits, SQLite backup writes a retained session file and
the runner checks the saved graph hash against the RAM graph. The separate
`live_forward_archivist.py` validates MAP-1/session schema, ROM identity and
closed-session graph hash, then reuses the atomic session merger. A missing
master is seeded without warning; incompatible sessions or newly created
conflicts fail before replacement. This bridge is independent of the existing
AUTO67 REGISTER_REACHING_DEFINITION adapter and is not part of production
scheduling.

## M12 FLOW_V1 exact ROM range linkage 2B

`live_forward_rom_link_runtime.py` runs one bounded 16-Worker, depth-20,
64-KiB Worker-1B session. Its callback stages only the immutable bytes from
segments already accepted by the host audit and RAM Cartographer. After
EmuHawk exits, `live_forward_rom_link.py` invokes the developer-only
`oasis_re_rom_range_decode` helper, which reuses the bounded M68K decoder and
the explicit 24-bit memory-region resolver. It checks the canonical USA ROM
identity, FLOW opcode, exact decoded instruction bytes and ROM bounds before
adding any range claim. RAM, hardware, unknown and unsupported addresses do
not become ROM ranges; an unsupported ROM decode remains unresolved and stops
the checkpoint from claiming PASS.

One ROM-SHA-scoped `ROM_INSTRUCTION_RANGE` represents each canonical start,
exclusive end and full byte encoding. `OBSERVED` `EXECUTED_FROM_ROM` edges
retain each run/epoch/Worker/capture/generation/record occurrence in edge
lineage, so repeated execution adds evidence without copying the canonical
range node. A segment's terminal instruction also has an
`OBSERVED_NEXT_PC` edge to an address-only node; that edge records the raw
target address and does not assert that the target instruction was captured.
`rom_execution_ranges.json` reports observed execution-byte volume separately
from unique ranges and unique covered bytes. These runtime metrics never alter
`SOURCE_OWNED`. `live_forward_rom_link_audit.py` reconciles every saved
instruction occurrence and terminal `next_pc` against the raw FLOW_V1 segment
records. It independently decodes every captured PC, checks all linked ranges
against canonical ROM bytes and persisted MAP-1 identities, and verifies that
repeated occurrences share one stable range identity. This is an
executed-instruction interval layer, not a full ROM classification map.

## M12 generic recursive ROM closure V1

`src/tools/thor_evidence/generic_recursive_closure.py` is the subsystem-neutral
post-run closure core. It consumes normalized instruction, memory, call and
exact-range facts; derives register and effective-address provenance; emits
typed graph edges, table candidates, call relations, closure queue and capture
gaps; and promotes only caller-supplied byte-identical ranges. Missing facts
are `UNRESOLVED` and remain queued, so an incomplete chain cannot stop
unrelated exact closure. The module has no game-PC rules. Its command-line
entrypoint emits the nine `postrun_*` artifacts; the dynamic SAT selector chain
is a regression fixture, not an engine special case.

## M13 generic recursive closure live integration V1

`live_forward_generic_closure_stage.py` is the post-run adapter between the
semantic stages and ASM closure. It accepts only the sealed FLOW paths owned by
the receipt, materializes a hash-bound normalized corpus, checks the canonical
ROM identity, runs the generic core, and emits closure/gap artifacts. A zero
promotion result is `NO_DELTA` with a deterministic capture-gap ranking and
continues to ASM closure; missing or corrupted corpus/ROM identity is
fail-closed. `generic_closure_handoff.py` keeps this policy out of the core
graph engine and the coordinator owns the stage ordering.

## M13.1 semantic unresolved scheduling

`pipeline_outcomes.py` separates a stage's semantic result from its scheduling
effect. Allowlisted semantic coverage/provenance/acceptance gaps remain
`UNRESOLVED` with their original stop reason, while the scheduler continues to
generic closure and independent downstream analysis. Integrity, identity, and
other non-allowlisted stops remain fatal. The final pipeline can therefore
complete its audits and cleanup while reporting unresolved evidence; the
progress UI and final receipt preserve that distinction rather than relabeling
the semantic gap as PASS.

## M13.2 generic closure gap-driven improvement

The closure engine may add `OBSERVED_NEXT_PC` edges only from normalized
instruction events whose source opcode exactly matches canonical ROM bytes.
These edges record observed transitions, not statically proven branch/call
semantics, and do not establish candidate boundaries or ownership. The live
stage runs closure twice and requires deterministic equality plus a hashed gap
ranking before acceptance. Permanent raw-FLOW reclaim is gated on those
artifacts; absent or altered generic acceptance evidence leaves raw inputs
intact. Missing CPU/domain, register, memory-width and round-trip evidence
remains unresolved and is reported for a future capture.

## M13.3 normalized FLOW evidence contract V2

`generic_flow_normalizer.py` maps every fixed-width native FLOW record into a
versioned event identity and generic collections. CPU identity is taken only
from the captured `cpu_id`; bus width/domain and the causal instruction
sequence are retained, ROM bus reads are made explicit, and M68K opcode bytes
are compared to the canonical ROM without dropping mismatches. Runtime
next-PCs are kept as observed flow facts; JSR/BSR, returns, and indirect JMP
targets are represented only when their encoded opcode family and captured
transition support that relation. The closure core consumes normalized memory,
ROM-read, call, and return facts directly while retaining the legacy v1 input
path for fixtures.

The current native 48-byte record ABI has no per-instruction register payload,
snapshot reference, or complete M68K instruction byte width. Host-audited
Worker segment metadata does preserve exact entry/exit D0-D7, A0-A7, SR and PC
snapshots; V2 links these bounded snapshots to the segment's first/last
instruction identities without pretending they describe intervening
instructions. Missing per-instruction snapshots and decode/width fields stay
explicit capture gaps. This is a partial schema migration only: it does not
establish the M13.3 live acceptance gate or authorize raw evidence
reclamation.

## M12 runtime control provenance 2C — Phase 0 stop

Phase 0 examined the saved, independently audited 2B FLOW_V1 runtime evidence
before considering any register-provenance instrumentation. Its 199,630
instruction occurrences contain 29,087 branch-family events, 290 direct
absolute-long JSRs, 1,760 returns, and no indirect JMP or JSR. The 290 JSRs
encode their destination in the instruction itself; they do not demonstrate a
pointer loaded from ROM or another memory region. The run also contains 863
asynchronous vector-30 exception-event records.

Result: `STOP_NO_RUNTIME_PROVENANCE_GAP` for this bounded saved execution.
No native provenance state, transport record, Lua hook, runtime campaign, or
canonical graph relation was added. This is a run-local negative result, not a
claim that all game scenarios lack indirect transfers. Reconsider a sensor only
after accepted runtime evidence contains an actual indirect JMP/JSR whose
memory origin the current evidence pipeline cannot resolve.

## Runtime ROM property map v1 — developer tooling

`uint16_t[rom_size]` stores additive observed-use bits, with zero reserved for
UNKNOWN. It is separate from the canonical emission partition and
`SOURCE_OWNED`. The versioned checkpoint binds the payload to ROM, schema,
proof-contract, core/build, run, generation, capabilities, and validation
identities. Runtime updates touch memory only; checkpoints and compact range
exports are explicit operations.

The developer library implements dirty-page tracking, supported M68K decoder
span marking, byte-resolved data reads, Z80 fetched-byte validation, exact-copy
RAM origin tags, consumed VDP source marking, cross-run union/rebuild, range
export, and canonical-map overlay without changing existing classes or
ownership. Pinned GPGX hooks now feed the live bitmap for accepted M68K, Z80,
and direct VDP paths. Mode 5 CPU data-port writes additionally qualify only
when one exact physical ROM word is read and a completed same-instruction
`MOVE.W` forwards that value unchanged to the accepted VRAM, CRAM, or VSRAM
port; fills and unsupported source modes fail closed. Their limits and
acceptance evidence are in
`docs/ROM_PROPERTY_MAP.md`.

The Format-A mode-0 audio hook uses the existing Z80 banked-ROM read resolver.
It adds `AUDIO_PAYLOAD_PROVEN` only for an exact byte read at verified decoder
PC `0x080E` or `0x0855` inside either statically closed Format-A resource. It
does not taint registers or promote a whole resource from one read.

The graphics contract records `A0` at entry to verified M68K decompressor
`0x3820` and classifies its exact ROM-backed input interval only after a
successful `RTS`. The new `COMPRESSED_GRAPHICS_SOURCE` bit describes consumed
compressed input; it does not assert direct VRAM provenance. The hook validates
every byte's physical ROM mapping and rejects discontinuity, incomplete calls,
and spans over 64 KiB.

## M14 accepted-artifact ROM property contributions

`src/tools/thor_evidence/m14_consumer_evidence_import.py` and
`m14_gfx_accepted_artifact_import.py` and
`m14_gfx_accepted_artifact_batch.py` are host-side verifiers and sparse ledger
writers. They emit `ACCEPTED_ARTIFACT` claims apart from runtime `OASROMP1`
contributions. A source-aware derived bitmap is rebuilt from the immutable
runtime base plus revalidated ledgers; it is not saved with a fabricated runtime
core identity. W5 support remains restricted to the exact secondary Format-A
mode-0 resource and `AUDIO_PAYLOAD_PROVEN`. The GFX adapter reconstructs one
accepted M12-GFX-2 closure from immutable Git source/report identities and the
accepted AUTO8 consumer record, independently replays its exact Ancient decoder
extent. The batch adapter reuses that same caller-closed proof semantics for
the accepted TOP-20 exact ranges, recording each physical range as an
independent `COMPRESSED_GRAPHICS_SOURCE` ledger. Both adapters validate source
and proof-contract hashes, exact ROM identity, range replay, class delta,
deterministic replay, removal, and read-only canonical SQLite invariance.
Neither enters emulator, worker, or rendering paths.

### ROM Coverage GUI — read-only visualization

`tools/bizhawk-native-ring/rom_coverage_gui.py` is a Tkinter view over the
existing versioned bitmap checkpoint. `rom_coverage_model.py` verifies the
checkpoint checksum, ROM/schema/proof-contract/build identity and property
capabilities before aggregation. An overlay receipt is accepted only with the
exact paired checkpoint hash and identity. The GUI has no path to runtime
hooks, property mutation, canonical map writes, or semantic promotion.

The view keeps bitmap union coverage (`mask != 0`) separate from classification
coverage. M68K/Z80 data-read bits are observations; by themselves their cell
status is `OBSERVED_UNCLASSIFIED`, with separate colors for each read source.
Executed-encoding, direct VDP-source, and proven
audio-payload bits count as classified. The direct VDP-source bits include the
same-instruction ROM-to-port path after its proof-contract extension. Multiple
proven classes render as
`MIXED`, while raw read observations overlapping one proven class do not hide
that class. A run delta is unavailable until a compatible canonical union is
loaded, preventing a missing baseline from being presented as new discovery.
The view also reports newly classified bytes separately from newly covered
bytes, so property refinement remains visible after union coverage plateaus.
This grouping is local to the read-only view and does not alter the property
contract or runtime bitmap.

The LIVE mode polls the selected checkpoint's atomic replacement timestamp
every 500 ms and reads changed snapshots on a separate reader thread. It never
shares the emulator's in-memory property array or blocks its observation
thread. The current capture script writes its bitmap at the end of a capture;
LIVE refresh therefore occurs when an existing publisher replaces the
checkpoint, and does not add a new live publisher to GPGX. Whole-ROM cells are
1 KiB, with click-through 32-byte and 1-byte views. All levels use exact
half-open byte ranges and compute total coverage from `mask != 0`.

## Rolling MASTER runtime address map viewer

`tools/bizhawk-native-ring/rolling_master_map_model.py` validates and reads one
`oasis.m12.rolling-master.v1` snapshot through SQLite `mode=ro`; an optional ROM
file must match its stored SHA-256. The paired GUI groups observed instruction
PCs and execution counts into 1 KiB, 32-byte, and byte cells, with exact
instruction rows on selection. This dataset carries runtime observations and
control edges, not the canonical emission partition or `SOURCE_OWNED`. Empty
cells mean only that the selected runtime master has no PC there. The viewer
cannot write the database or promote semantic, ownership, asset, or player
claims. It remains separate from the OASROMP1 property-checkpoint viewer.

The live Worker Control window continues to display bounded runtime snapshots.
After sealing, `live_forward_postrun_window.py` adds a runtime-map tab that polls
only the run-local rolling MASTER `current.json`, validates the generation path
and SQLite hash, then reads and bins the snapshot on a daemon thread. A pointer
change refreshes the overview; unchanged generations are not rehashed. Large
post-run reports are parsed only when their file identity changes so the 4 Hz
progress display stays responsive. The map is a 2D ROM-address execution heat
overview and does not represent canonical ownership or a literal 3D scene.

## Accepted Z80 driver image source proof

`src/tools/thor_evidence/m14_z80_driver_image_contract.py` validates accepted
M12-AUTO2 upload evidence together with W4 execution/sink corroboration and W5
destination evidence for the exact ROM image `[0x062E38,0x064E38)`. Its contract
`contracts/m14_z80_driver_image_upload_v1.json` defines
`Z80_DRIVER_IMAGE_SOURCE` only in the separate `ACCEPTED_ARTIFACT` evidence
layer. It does not extend runtime `OASROMP1` properties or claim that every
uploaded byte executed. The verifier replays the exact ROM range, reconciles
current artifact ledgers, and reports overlap and potential unique gain without
importing a contribution. Focused tests are in
`tests/m14_z80_driver_image_contract_test.py`; the ranked family inventory is
`docs/reports/THOR_CONSUMER_FAMILY_BACKLOG_20260928.md` with a JSON companion.

## Artifact-only Z80 driver contribution

`src/tools/thor_evidence/m14_z80_driver_image_import.py` consumes the accepted
upload proof and emits `thor.accepted-artifact.property-contribution.v2` for
`Z80_DRIVER_IMAGE_SOURCE`. Its typed property namespace is separate from the
16-bit runtime `OASROMP1` mask; no runtime bit is allocated and
`Z80_EXECUTED_ENCODING` is explicitly excluded. The verifier reconstructs the
prior proven-coverage bitmap from the runtime checkpoint and independently
validated W5/GFX ledgers, then proves exact range replay, unique-byte delta,
removal, and deterministic rebuild. The removable ledger and result receipt
are under ignored `build/m14-consumer-evidence-import/`. Regression coverage is
in `tests/m14_z80_driver_image_import_test.py`.
# M14 sparse graphics VRAM causal artifact layer

`m14_graphics_vram_causal_import.py` validates reproducible bounded
`0x3820` transactions and records exact ROM input ranges under the distinct
`GRAPHICS_VRAM_CAUSAL_SOURCE` accepted-artifact property. The evidence layer
requires replayed decoder output and exact RAM-origin VRAM transfers, and stays
separate from the runtime property ABI, `COMPRESSED_GRAPHICS_SOURCE`, and
canonical ownership. SAT/plane use, palette dependency, and rendered pixels
remain separate proof stages.
# Diagnostic caller census receipt path (2026-09-30)

The developer-only `graphics_caller_census` module serializes bounded
`0x3820` decoder observations. The native graphics-transactions export wraps
the existing transaction JSON and appends `caller_census` plus row/byte
diagnostics; the existing managed and Lua string bindings remain unchanged.
This is diagnostic discovery data only. It does not update ROM properties,
accepted evidence, canonical authority, or `SOURCE_OWNED`. Caller function
identity remains unknown unless independently proved; a return-address-derived
call-site candidate is explicitly marked unconfirmed.

## Proposed path-specific M68K native dispatch (design only)

The verified gameplay helpers at `0x94A2`, `0x8EF8`, and `0x16A46` remain
inactive. Their reversible dispatch proposal, including distinct ABI adapters,
caller gates, ROM fallback, staged effects, and cloned-state reference mode, is
documented in `docs/reports/THOR_M68K_HYBRID_DISPATCH_DESIGN_20261008.md`.
This proposal is not production architecture and adds no runtime wiring. A
future implementation needs a separate decision after exact transfer-source
provenance and emulator continuation requirements are proven.

M68K `0x8EF8` now has an opt-in developer-only HOOK_CPU route in
`oasis_raw_record_live_dispatch`. It is enabled only by `--enable-8ef8`, pins
the canonical ROM, and gates on exact consecutive direct-BSR source PCs plus the
matched stack return. Failed gates continue the original ROM routine. The
runner is not linked into the shipped game and does not activate production
dispatch; cycle/prefetch equivalence and natural gameplay reachability remain
unproven. See `reports/THOR_M68K_8EF8_OPT_IN_LIVE_ROUTE_20261009.md`.

## Replay-native island attempt (2026-10-12)

The deterministic gameplay-session runner can opt into a replay-only typed
adapter for `0x9D70`, gated to caller `0x009144`, the pinned ROM, exact BSR.W,
stack return, contract ID, and bounded RAM inputs. The flag defaults off and
the route uses staged effects before redirecting through the original RTS.
ROM-only replay passes; an enabled one-helper run diverges at frame 2655 after
one native call. Therefore the route is diagnostic-only, no multi-helper
replay-equivalence is established, and production dispatch remains inactive.
See `reports/THOR_REPLAY_NATIVE_ISLAND_ATTEMPT_20261012.md`.
