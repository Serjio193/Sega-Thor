# ADR-M14.7B-GENERATION-COMPACTION — One current canonical database pair
**Status:** Accepted for M14.7B canonical-generation storage.
**Date:** 2026-09-26

**Context:** Every accepted capture published a complete `master.sqlite` and
`knowledge.sqlite` pair while retaining all ancestors. After 50 unique W3
blobs, 86 committed generations occupied 30,811,365,376 bytes, so each new
capture reduced free space despite receipt-bound raw cleanup.

**Decision:** Keep `current.json` as the sole authoritative pointer to one full
database pair. Preserve each committed generation's existing receipts and add
a sealed `lineage.json` containing exact parent identity, before/after logical
hashes, source/session identity and experiment-receipt references. Serialize
publishers, the canonical MASTER V2 snapshot reader, and GC with one process
lock. `generation-gc --plan` is read-only;
`--verified-only` revalidates the current pair, SQLite integrity/FK checks,
receipt chain and map-hash transitions, then unlinks only the exact two known
database paths in verified current-lineage ancestors. Keep unsealed staging,
orphan generations, ROMs, raw captures and all non-database generation files.
Do not add a second full-map database or map-history store.

**Consequences:** Queries use the one current aggregate map; transaction and
experiment receipts preserve logical lineage and replay references. Historical
full databases are no longer immediate rollback targets; recovery to a prior
state requires replay from its source capture/accepted bootstrap. Current-map
publication remains atomic, and compaction runs only after pointer commit and
current-map reopen/self-check. The fixture verifies G0→G3 compaction, unchanged
canonical query signatures and one full database pair through ten sequential
transactions.

**Evidence:** `src/tools/thor_evidence/knowledge_generation_gc.py`,
`tests/rom_knowledge_pipeline_test.py`, and
`docs/reports/THOR_M14_7B_GENERATION_COMPACTION.json`.
# ADR-M12-W5C-EXACT-AUDIO-RESOURCE-OWNERSHIP-PROMOTION-V1 — Exact Audio Resource Ownership, Partition Split, and Canonical Promotion
**Status:** Accepted for M12 W5c
**Date:** 2026-09-21

**Context:**
Following W5 round-trip verification, two Mode 0 audio resources (`AUDIO_RESOURCE_FORMAT_A_0001` at `0x0BD540..0x0BF768`, 8,744 bytes, and `AUDIO_RESOURCE_FORMAT_A_0002` at `0x0BC95C..0x0BD540`, 3,044 bytes, combined 11,788 bytes) were proven byte-identical to canonical ROM without original nibble/byte dependence in the encoder. The pre-promotion canonical ROM knowledge map holds emission range `0x0B8000..0x0BF768` (30,568 bytes) as unowned `UNKNOWN` (`source_owned = 0`). Under ADR-0044, exact promoters alone may update ownership under their current gates.

**Decision:**
1. **Ownership Eligibility Qualification**:
   - Qualify exact ownership eligibility for `AUDIO_RESOURCE_FORMAT_A_0001` (8,744 bytes) and `AUDIO_RESOURCE_FORMAT_A_0002` (3,044 bytes) under claim type `RECONSTRUCTION_VERIFIED` (`STATIC_VERIFIED`).
   - Confirm boundaries, format, deterministic decode, semantic inverse encode, byte-identical round-trip, encoder independence, and canonical ROM identity.
2. **Canonical Emission Partition Split**:
   - Authorize canonical knowledge tooling (`w5_audio_promote.py`) to split canonical emission range `0x0B8000..0x0BF768` into three exact disjoint intervals:
     * `[0x0B8000, 0x0BC95C)` (18,780 bytes): unpromoted `UNKNOWN`, `source_owned = 0`, artifact `blobs/0B8000_0BC95C.bin`.
     * `[0x0BC95C, 0x0BD540)` (3,044 bytes): `AUDIO_RESOURCE_FORMAT_A_0002`, `SOUND_DATA_CONTAINER_CONFIRMED`, `DATA_KNOWN`, `source_owned = 1`, `DATA`, artifact `data/audio_0BC95C.bin`.
     * `[0x0BD540, 0x0BF768)` (8,744 bytes): `AUDIO_RESOURCE_FORMAT_A_0001`, `SOUND_DATA_CONTAINER_CONFIRMED`, `DATA_KNOWN`, `source_owned = 1`, `DATA`, artifact `data/audio_0BD540.bin`.
3. **Canonical Claims and Proof Evidence**:
   - Create `rom_object` entries (`AUDIO_DATA`), `claim` entries (`SOURCE_OWNED = true, STATIC_VERIFIED`, `SOURCE_CLASS`, `FORMAT_SPEC`), and `evidence_ref` entries linking W5 acceptance proofs (`w5_audio_format_spec.json`, `w5_audio_resources.json`, `w5_roundtrip_receipt.json`, `w5_witness_alignment.json`).
4. **Idempotence and Non-Duplication**:
   - Re-running promotion on an already-promoted map produces `NEW_SOURCE_OWNED_BYTES = 0` and leaves hashes and partitions byte-for-byte identical.
5. **Strict Scope Prohibition**:
   - Descriptor tables (`0x0B8000..0x0B8050`), bank padding (`0x0BF768..0x0C0000`), Mode 1 resources, unverified neighbouring candidate resources, and whole audio banks remain strictly unpromoted as `UNKNOWN` or pre-existing padding until covered by independent reconstruction proof.
6. **SOURCE_OWNED Accounting**:
   - Increment `SOURCE_OWNED` from `1,475,600` to `1,487,388` bytes (`+11,788` bytes, 47.2827911377% of ROM).

**Consequences:**
1. Canonical knowledge map `docs/reports/THOR_M12_CANONICAL_ROM_KNOWLEDGE_MAP_2D.json` and its report are regenerated with updated hashes and metrics.
2. Partition coverage remains complete across all 3,145,728 bytes with 0 gaps, 0 overlaps, and 0 out-of-bounds rows.
3. Checkpoint emitted: `PASS_EXACT_AUDIO_RESOURCE_OWNERSHIP_V1`.

# ADR-M12-W5-EXACT-AUDIO-RESOURCE-FORMAT-ROUNDTRIP-V1 — Non-Linear Delta-PCM (Format A), Bank Descriptor Tables, Canonical IR, and Byte-Identical Round-Trip
**Status:** Accepted for M12 W5
**Date:** 2026-09-21

**Context:**
Following the W4 Level-3 causal witness from early-boot banked ROM reads (`0x0BEDE4..0x0BEDED`) to YM2612 DAC writes (`0x4001`), the exact audio resource format, true resource boundaries, decoding logic, encoding logic, and hardware alignment must be proven end-to-end without guesswork or promotional semantics.

**Decision:**
1. **Exact Consumer & Decoder Routines**:
   - The Z80 sound driver is located at ROM `0x062E38..0x064E38` (copied to Z80 RAM `0x0000..0x1FFF` during M68K boot).
   - Sound command `0x11` at `0x064F` invokes descriptor parser and bank switcher at `0x0704..0x076D`.
   - Bank switching writes 9 bits sequentially into Genesis bank latch `0x6000` via routine `0x0A0B..0x0A40`.
   - Mode 0 sample loop resides at `0x080D` (high nibble) and `0x0854` (low nibble), with loop advancement at `0x0880` (`INC HL; DEC BC`) and termination check (`OR B, C; JP Z, 0x0892`).
   - Hardware DAC emitter resides at `0x0968..0x0980`, writing to YM2612 port 0 register 0x2A (`(IY+0)=0x2A; ADD A, D; LD D, A; LD (IY+1), A`).
2. **Exact Descriptor Table Layout**:
   - Each of the 8 32KB audio banks (ROM `0x080000..0x0B8000`) begins with an 80-byte table (`0x8000..0x804F`) containing 16 5-byte entries.
   - Entry structure: bytes 0..1 = 16-bit logical start address (`0x8000..0xFFFF`); bytes 2..3 = 16-bit compressed byte length; byte 4 = playback mode (0 = standard, 1 = hold/interpolate via `0x0999`).
   - Compressed audio data starts immediately following the table at offset `0x8050` and packs contiguously until the end of the last resource, followed by `0xFF` bank padding.
3. **Format Semantics (AUDIO_FORMAT_A)**:
   - Initial accumulator `D = 0x80` (128 decimal, unsigned 8-bit PCM midpoint / silence).
   - Lookup table at Z80 RAM `0x0008..0x0016`:
     `[0, +1, +2, +6, +12, +24, +48, +96, -96, -48, -24, -12, -6, -2, -1]`.
   - Nibble 0: repeat previous delta `E` for 3 consecutive sample ticks.
   - Non-zero nibbles: load delta from table, add to accumulator `D`, emit sample.
4. **Exact Boundaries for Primary Resource**:
   - `AUDIO_RESOURCE_FORMAT_A_0001`: Bank 0x17, Entry 7 (`0x0BD540..0x0BF768`, 8,744 bytes, mode 0).
   - Strictly encloses W4 witness cluster `0x0BEDE4..0x0BEDED` at byte offset 6,308.
   - Decoded sample at byte offset 6,308 low nibble matches observed W4 DAC write `0x78` exactly.
5. **Canonical IR & Byte-Identical Round-Trip**:
   - Pure Intermediate Representation (`AudioResourceIR` / `AudioToken`) represents tokenized delta events and decoded PCM samples without storing original binary blobs.
   - Deterministic encoder reconstructs original byte stream with 100% byte-identical SHA-256 match for primary resource (`AUDIO_RESOURCE_FORMAT_A_0001`), secondary resource (`AUDIO_RESOURCE_FORMAT_A_0002`), and across multiple audio banks.
6. **SOURCE_OWNED Reconciliation**:
   - All Gates A through J verified green.
   - Manifest promotion delta deferred (`SOURCE_OWNED_DELTA = 0`) to preserve ADR-0044 manifest invariants until a full audio bank re-partitioning manifest is formally authorized.
   - Checkpoint emitted: `PASS_EXACT_AUDIO_RESOURCE_ROUNDTRIP_V1`.

**Consequences:**
1. Complete deterministic codec implemented in `src/tools/thor_evidence/w5_audio_decode.py` and `w5_audio_encode.py`.
2. All 9 required W5 acceptance artifacts emitted in `build/m12-w5-acceptance/`.
3. 18 comprehensive tests pass in `tests/w5_audio_format_test.py`.
4. Zero promotional or musical names used.

# ADR-M12-W4-ACTIVE-AUDIO-RESOURCE-ANALYSIS-V1 — Four Causal Levels, Last-Writer Memory Handoff, Port-Local Audio Sinks, and Neutral Audio Resource Classification
**Status:** Accepted for M12 W4
**Date:** 2026-09-21

**Context:**
Connecting M68K sound commands to Z80 sound driver execution, Z80 RAM state, and physical audio outputs (YM2612 FM synthesis and PSG) requires strict evidentiary separation between temporal coincidence, memory dependencies, instruction dataflow, and strict end-to-end causal chains. Hardware audio interfaces require port-local latching semantics (YM2612 Part 1: 0x4000->0x4001; Part 2: 0x4002->0x4003) and exact hardware register recognition (DAC data 0x2A, enable 0x2B; PSG 0x7F11). Candidate audio resources in canonical ROM must be classified strictly with neutral taxonomy (AUDIO_COMMAND_CANDIDATE, AUDIO_TABLE_CANDIDATE, AUDIO_PATCH_CANDIDATE, AUDIO_RESOURCE_RANGE_CANDIDATE) without premature promotional music semantics (SONG, TRACK, INSTRUMENT).

**Decision:**
1. **Four Causal Levels**:
   - Level 0 (`TEMPORAL_ASSOCIATION`): Temporal ordering ($T_1 < T_2$) within same execution context `(run_id, epoch)`. Necessary but not sufficient for causality.
   - Level 1 (`MEMORY_DEPENDENCY`): Exact last-writer rule. M68K writes to Z80 window ($A00000..$A01FFF), Z80 reads matching value at $T_{read} > T_{write}$, and no intervening write occurs. Yields `OBSERVED_HANDOFF` (`DERIVED_EXACT`).
   - Level 2 (`DATAFLOW_DEPENDENCY`): Exact Z80 instruction dataflow tracking across registers (A, B, C, D, E, H, L, IX, IY), memory, stack, and shadow registers. Unsupported opcodes terminate tracking conservatively without speculation.
   - Level 3 (`STRICT_CAUSAL_CHAIN`): Provenance chain connecting verified source (M68K handoff or banked ROM read) to hardware audio sink through unbroken Level 2 dataflow (`DERIVED_EXACT`).
2. **Port-Local YM2612 Latching Semantics**:
   - Part 1: Port 0x4000 latches register address for Port 0x4001.
   - Part 2: Port 0x4002 latches register address for Port 0x4003.
   - Cross-port pairing (e.g. 0x4000 pairing with 0x4003) is strictly prohibited. An address write overwrites any previous unconsumed latch on that port.
   - YM2612 register 0x2A is DAC data, 0x2B is DAC enable. Port 0x7F11 is PSG.
3. **Input Evidence Authenticity**:
   - Evidence records must exist in raw binary input files processed by the active pipeline run to be classified as `OBSERVED`. Historical records or prior run references absent from the active binary stream remain `WITNESS_CANDIDATE` (`HYPOTHESIS`).
4. **Spatial Cluster Span vs. Resource Boundary**:
   - Spatial span `[min_addr..max_addr]` of observed ROM reads defines an `OBSERVED_ROM_CLUSTER`, not an exact resource boundary. Resource ranges remain `HYPOTHESIS` until bounded by static or runtime closure proofs.
5. **Neutral Resource Taxonomy**:
   - All candidate audio structures must use strictly neutral types (`AUDIO_COMMAND_CANDIDATE`, `AUDIO_TABLE_CANDIDATE`, `AUDIO_PATCH_CANDIDATE`, `AUDIO_RESOURCE_RANGE_CANDIDATE`). Promotional labels (`SONG`, `TRACK`, `INSTRUMENT`, `VOICE`, `SAMPLE`) are strictly forbidden until behavioral parity is established.

**Consequences:**
1. Analysis pipeline (`w4_audio_pipeline.py`) emits 9 verified acceptance artifacts under `build/m12-w4-acceptance/`.
2. Over 7,168 M68K->Z80 handoffs verified with exact last-writer matching and 0 intervening writes.
3. 2,048 YM2612 register writes reach Level 3 Strict Causal Chain with exact Z80 dataflow provenance.
4. Zero promotional or unverified music semantics introduced into the repository.

# ADR-M12-W3-CANONICAL-CROSS-CPU-TIMELINE-V1 — Canonical Cross-CPU Temporal Identity and Causal Closure
**Status:** Accepted for M12 W3
**Date:** 2026-09-21

**Context:**
Dual-CPU co-capture records concurrent execution of M68000 and Z80 processors.
To guarantee sound, non-speculative cross-CPU timeline analysis and causal relations
(such as 68K writes to Z80 RAM followed by Z80 reads and YM2612 audio register writes),
the exact semantics, source, units, frame reset behavior, and tie-breaking rules of
`master_time` across both CPU cores must be mathematically verified and strictly enforced.

**Decision:**
1. **Master Time Source and Units**:
   - `m68k.cycles` (`core/m68k/m68kcpu.c`) scales instruction cycles by `MUL = 7` (Genesis Master Clock / 7 = 7.67 MHz M68K clock).
   - `Z80.cycles` (`core/z80/z80.c`) scales instruction cycles by `15` in `cc_op` tables (Genesis Master Clock / 15 = 3.58 MHz Z80 clock).
   - Both cycle counters are directly expressed in the exact same physical unit: **Genesis Master Clock cycles** (3420 mcycles per scanline, 896,040 mcycles per standard NTSC frame). The conversion ratio is 1:1.
2. **Temporal Semantics and Canonical Identity**:
   - At every frame boundary, `core/system.c` executes `m68k.cycles -= mcycles_vdp` and `Z80.cycles -= mcycles_vdp`.
   - Therefore, `master_time` is strictly `FRAME_RELATIVE` (rebasing to ~0 each frame).
   - The canonical timeline key is defined as `(run_id, epoch, frame, master_time)`. Comparing `master_time` alone across different frames is strictly prohibited.
3. **Cross-CPU Tie-Breaking Rule**:
   - If M68K and Z80 share `(frame, master_time)`, the relation is defined as `SAME_MASTER_TIME_UNORDERED`.
   - `stream_sequence` reflects host thread scheduling slices and CANNOT be used to prove causal precedence between same-timestamp events.
   - Causal interactions require `STRICT_BEFORE` temporal precedence (`T_write < T_read`).
4. **Timeline Integrity and Isolation**:
   - Cross-run and cross-epoch events are strictly disjoint; joining or comparing events across differing `run_id` or `epoch` raises an error.
5. **Retained Segment Metric**:
   - `retained_segment_data` is defined as `CUMULATIVE_BYTES_PROCESSED` (cumulative audit throughput streamed to disk), not resident live memory. Live heap remains strictly bounded at 160 MiB.

**Consequences:**
1. Authoritative cross-CPU sorting in `w3_cross_cpu_timeline.py` respects frame precedence first, followed by master cycles.
2. Tests A–G pass with 100% verification, preventing speculative causality, timestamp collisions, and cross-run contamination.
3. Memory behavior remains proven `BOUNDED_OSCILLATING` under sustained gameplay.

# ADR-M12-W3-SUSTAINED-10MIN-PROFILE-V1 — 10-Minute Sustained Gameplay Memory Stability and Safety Verification
**Status:** Accepted for M12 W3
**Date:** 2026-09-21

**Context:**
Following the configuration of the 2,097,152-slot default ring buffer (128 MiB slot storage, 96 MiB raw),
it was necessary to prove that the default ring capacity is safe under prolonged, real gameplay emulation
rather than brief, open-and-immediately-close acceptance campaigns. The system was subjected to a sustained
Beyond Oasis emulation session for at least 10 minutes with 128 Workers and depth 100, sampling process
working set, private bytes, native heap, ring allocation, worker counts, and record throughput every 5 seconds.

**Decision:**
1. Maintain the startup default ring capacity at 2,097,152 slots.
2. Formally evaluate memory behavior using linear regression slope over sustained emulation (minutes 1 to 10).
3. Require 0 retention failures, 0 dropped captures, 0 identity collisions, 0 stale ACKs, and 0 runtime errors
   under full 128-Worker continuous turnover.
4. Verify graceful END GAME shutdown via IPC and confirm post-cleanup return to steady-state.

**Consequences:**
1. Under 610.5 seconds of continuous Beyond Oasis gameplay (283,648 audited segments, 828,506,454 records produced,
   395 ring wraps), Private Bytes remained strictly bounded between 301.35 MB and 306.72 MB with a slope of
   -0.3350 MB/min, and Working Set remained bounded between 466.85 MB and 470.18 MB with a slope of +0.2237 MB/min.
2. Memory behavior was classified as `BOUNDED_OSCILLATING`, proving the absence of memory leaks, unbounded queues,
   or growth runaway.
3. Retention failures remained exactly 0 throughout all 395 wraps.
4. Upon triggering END GAME, final analysis completed without spikes (peak Private Bytes 303.93 MB), and the emulator
   process terminated cleanly with code 0, releasing all resources.
5. The 2,097,152-slot default capacity is verified as safe, robust, and permanent for the W3 architecture.

# ADR-M12-W3-LARGE-BOUNDED-RING-ACCEPTANCE-V1 — Large Bounded Ring and Final W3 Acceptance
**Status:** Accepted for M12 W3
**Date:** 2026-09-21

**Context:**
Dual-CPU co-capture (M68K + Z80) at scale increases stream record throughput.
Under 128 active workers and deep chains (depth 100, 512, 1000), the initial 16,384-slot
transport ring required frequent wraps (1,783 wraps per 100 rounds at depth 20).
To ensure robust retention without dynamic reallocation or converting the ring into an
unbounded log, a bounded large ring with constant-time hot-path indexing was needed.

**Decision:**
1. Set the default startup ring capacity to 2,097,152 slots (48-byte records, 96 MiB raw,
   128 MiB slot storage). Keep capacity startup-configurable via `oasis_lf_set_ring_capacity()`
   and command-line flag `--ring-capacity`.
2. Prohibit dynamic resizing during emulation: the ring remains strictly bounded cyclic transport.
3. Optimize hot-path record append with power-of-two bitmask indexing:
   `(sequence - 1u) & (cap - 1u)` replaces division/modulo operations on the hot path.
4. Scale worker result buffers with capture depth: 256 KiB for depth 100, 512 KiB for depth 512,
   and 1024 KiB for depth 1000, ensuring `depth_limit_endings` are achieved without premature
   buffer exhaustion.
5. Invariant preservation: ring expansion does NOT alter Worker admission or capture-depth semantics.
   Cross-CPU ordering strictly uses `(run_id, epoch, frame, master_time)`.

**Consequences:**
1. Ring turnover dropped by 137x (from 1,783 wraps to 13 wraps per 100 rounds at depth 20).
2. Official acceptance at 128 Workers x Depth 100 passed 12,800/12,800 segments with 0 retention
   failures and 0 overwrites.
3. Deep-chain stress tests at Depth 512 (82.1M records) and Depth 1000 (138.2M records) passed
   with 100% depth-limit completion, 0 retention failures, and 0 invalid captures.
4. Frame timing under dual-CPU recording remained within normal emulation latency (p50 = 16-17 ms).

# ADR-M12-W3-Z80-COCAPTURE-CROSS-CPU-TIMELINE-V1 — Z80 Co-Capture and Cross-CPU Timeline
**Status:** Accepted for M12 W3
**Date:** 2026-09-21

**Context:**
M12 Workers had M68K instruction and bus capture (W1), resource classification (W2),
and frame coherence (W2.1), but lacked Z80 co-capture. Cross-CPU interactions
(e.g., M68K writing to Z80 RAM, Z80 reading and writing to YM2612/PSG) could not
be ordered on a common timeline, and Z80 banked ROM accesses could not resolve
to physical addresses without external context. The 32-byte record was insufficient
to losslessly represent common emulated time and multi-byte Z80 instructions.

**Decision:**
1. Upgrade `oasis_lf_record` to 48 bytes with explicit lossless fields:
   `stream_sequence`, `instruction_sequence`, `master_time`, `pc`, `address`,
   `value`, `kind_flags`, `cpu_id`, `length_or_width`, `domain`, `reserved`,
   and `auxiliary`.
2. Keep M68K and Z80 `instruction_sequence` strictly independent monotonic counters.
3. Use frame-relative master cycles (`master_time`) as the authoritative common
   timing basis across M68K and Z80.
4. Make ring capacity configurable at initialization (default 16,384 slots).
5. Capture ordered `BANK_REGISTER_CHANGE` events and resolve physical Genesis/ROM
   addresses directly into the record `auxiliary` field for all banked ROM reads.
6. Prohibit higher-level audio semantics (no SONG, TRACK, INSTRUMENT, VOICE,
   SAMPLE, NOTE, MUSIC_RESOURCE).
7. Validate cross-CPU causal relations only under strict temporal precedence
   (T1 < T2 < T3) and matching addresses/values without speculative inference.

**Consequences:**
Native trace hooks cleanly capture both M68K and Z80 instruction and bus events.
Cross-CPU timeline sorting by `master_time` produces deterministic temporal ordering.
The C test suite (`oasis_live_forward_z80_test`) covers all 22 required test cases
A–V. Python tools provide lossless decoding, bank tracking, physical address
resolution, timeline construction, and primitive event classification.

# ADR-M12-R11-EVENT-DRIVEN-SUBPROCESS-WAIT-1 — Event-driven Stage 7 waits
**Status:** Accepted for M12 R11
**Date:** 2026-09-20

**Context:** Stage 7 and Stage 8 external tools were launched through a
bounded runner that checked `Popen.poll()` only after a 250 ms sleep. The
retained R10 workload showed 400 external decoder invocations, so the polling
quantization was material to short child processes even though decoder
semantics were correct.

**Decision:** Replace process-completion polling with a blocking
`Popen.wait()` waiter thread and completion event. Keep dedicated stdout and
stderr drain threads, bounded joins, the existing timeout classifications,
and terminate-then-kill shutdown. A separate heartbeat thread may publish
progress but cannot determine child completion; callback failures are returned
to the caller after bounded cleanup.

**Consequences:** Fast children return at process-exit latency instead of a
250 ms quantum. The Stage 7 worker pool, candidate ordering, proof,
promotion, decoder contract, and temporary-output boundaries are unchanged.
The checkout exposes no public cancellation API, so no cancellation contract
was altered. The same sealed input produced identical request, unsupported,
promotion, and SOURCE_OWNED results while reducing Stage 7 wall time.

**Evidence:** `src/tools/thor_evidence/stage7_subprocess.py`,
`tests/stage7_subprocess_test.py`, the R10 sealed receipt, and the three R11
same-input optimized receipts under `build/thor-evidence/r11-event-wait-stage7`.

# ADR-M12-R5-IN-PROCESS-POSTRUN-COORDINATOR-1 — In-process Stage 1–9
**Status:** Accepted for M12 R5
**Date:** 2026-09-19

**Context:** R4 established ROM + MASTER V2 as the fail-closed startup
authority, but post-run still crossed a runtime/subprocess/status-file boundary.

**Decision:** Add `PostRunCoordinator` with explicit `PostRunContext`,
`PostRunResult`, and in-memory stage events. Normal Stage 1–9 execution runs in
one non-UI background thread owned by the runtime process. JSON status/report files
are derived diagnostics only; the legacy subprocess launcher remains available
for shadow/regression comparison. Disk-backed FLOW and Stage 5/6/7/8 truth
semantics are unchanged. Atomic diagnostic snapshots retry transient Windows
reader locks.

**Consequences:** Closing the analysis window cannot terminate coordinator work;
UI consumers may resubscribe to the event stream. Stage 7/8 external tools remain
bounded subprocesses. A replay of an already-absorbed run remains fail-closed
with `STOP_KNOWLEDGE_IMPORT_NONIDEMPOTENT`.

**Evidence:** `tools/bizhawk-native-ring/live_forward_postrun_coordinator.py`,
`tests/live_forward_postrun_coordinator_test.py`, and fresh runtime campaign
`build/thor-evidence/live-worker-control-2h/campaign-desktop-20260919-231221-978`.

# ADR-M12-R4-MASTER-STARTUP-AUTHORITY-1 — MASTER-only initialization
**Status:** Accepted for M12 R4
**Date:** 2026-09-19

**Context:** R3 made MASTER V2 the read authority for canonical, provenance and
outcome state, but normal startup still had no explicit fail-closed contract.

**Decision:** Add `master_startup_authority.py` as the sole startup validator.
It resolves one atomic pointer, verifies the MASTER file/logical/section hashes,
ROM identity, lineage, canonical emission partition, SOURCE_OWNED, provenance,
outcomes and absorption history, then exposes the existing canonical and outcome
views in memory. Legacy state is never opened on the production path. The
`--legacy-shadow-compare` flag is the explicit comparison boundary. Missing or
invalid authority stops with `STOP_MASTER_STARTUP_UNAVAILABLE` or
`STOP_MASTER_STARTUP_INTEGRITY` and never falls back to legacy files.

**Consequences:** A MASTER-only environment can initialize and prepare a
deterministic N+1 candidate descriptor without legacy persistent files. The
current runtime transport and Stage 5–9 semantics remain unchanged; deletion of
legacy artifacts is deferred.

**Evidence:** `build/thor-evidence/master-v2/current.json`,
`build/thor-evidence/master-v2/r4-startup-report.json`, and
`tests/master_startup_authority_test.py`.

# ADR-AUTO67-CARTOGRAPHER-SEAM-1 — Worker local chain to MAP-1
**Status:** Accepted for AUTO67
**Date:** 2026-09-15

**Context:** Clean AUTO67 Worker materialization produced bounded evidence but
was written only to the legacy live-chain sidecar. MAP-1 already owns stable
provenance graph identity and merge authority.

**Decision:** Add one downstream local-chain adapter and proof gate. Accept all
valid predecessor-produced `REGISTER_REACHING_DEFINITION` steps, construct
stable `ROM_INSTRUCTION` nodes and register-specific edges, and call the
existing Cartographer merge. Keep one bounded `LiveMapSink` queue/writer.
Cartographer is created, used and closed by that writer thread; snapshots
return cached graph metrics. Runtime occurrence, lease, frame and worker IDs
remain local evidence and never define global graph identity.

**Consequences:** Worker never sees map state or novelty results. Exact stable
replays and different occurrences share an import identity; incomplete steps do
not discard valid siblings and no-proof chains are not counted as map drops.
`--map-db` enables durable map output; without it the Worker still produces
local diagnostics and no output queue. Historical descriptor persistence is
isolated for offline AUTO67.3/AUTO67.4 consumers. No frontiers, scheduler or
upstream capture changes are introduced.

**Evidence:** `docs/reports/THOR_M12_AUTO67_CARTOGRAPHER_SEAM_1.md`.
# ADR-AUTO67-RAM-SESSION-MAP-1 — RAM session map and offline GLOBAL merge
**Status:** Accepted for AUTO67
**Date:** 2026-09-15

**Decision:** Keep the canonical GLOBAL SQLite unopened during runtime. The
existing bounded queue and single writer use an in-memory SQLite Cartographer;
clean shutdown backs it up to a retained session file. A deterministic offline
merge imports that session into a temporary GLOBAL copy under
`session-map:<session_graph_hash>`, validates, fsyncs and atomically replaces
GLOBAL. Replace or validation failure preserves the original GLOBAL bytes.

**Consequences:** Runtime map writes cannot stall on GLOBAL I/O, repeated
session imports are idempotent, and shutdown remains bounded by the existing
writer join and Dispatcher stop event. No new queue, capture path or Cartographer
identity is introduced.

**Evidence:** `docs/reports/THOR_M12_AUTO67_RAM_SESSION_MAP_1.md`.

# ADR-AUTO67-MAILBOX-CLEAN-1 — minimal Dispatcher-to-Worker lease message
**Status:** Accepted for AUTO67
**Date:** 2026-09-15

**Context:** The occurrence-only Dispatcher still copied branch, context,
seed, worker and duplicate lease identity into each mailbox task. Those fields
were not consumed by the Worker boundary and made the lease contract appear
to carry scheduling semantics.

**Decision:** Keep one detached `event` copy, one occurrence/window-derived
`investigation_id`, one `lease_id`, optional capsule/capture fields and the
bounded dispatch trace. The Worker consumes those IDs directly; CapsulePool
metadata and persistence retain their factual worker/lease provenance. Remove
mailbox branch/context/seed/worker/occurrence aliases and legacy profiler names
without changing selection order, capsule limits, or shutdown mechanisms.

**Consequences:** Final-only and same-branch occurrences remain independently
leasable, capsule identity mismatches fail closed, and shutdown can release a
worker waiting on a stopped emulator through the existing stop event. No queue
or backlog is introduced.

**Evidence:** `docs/reports/THOR_M12_AUTO67_MAILBOX_CLEAN_1.md`.

# ADR-0065 — chain-primary bounded AUTO65 campaign
**Status:** Accepted for AUTO65
**Date:** 2026-09-13

**Context:** AUTO64 reaches a useful fixed point with one unresolved upstream
queue. Repeating its QuickSave1 address-focused capture would not scale and
would violate the M12 evidence boundary.

**Decision:** Use one scheduler-selected, materially different bounded
gameplay scenario for a cheap broad discovery. Normalize each observed chain
into typed nodes and edges, compare structure before fingerprints, reuse known
prefixes, cluster shared dependencies, and batch investigations. Static
evidence precedes focused runtime; anti-repeat fingerprints reject equivalent
captures. A second encounter is persisted as `KNOWN_NEW_INSTANCE` and is
cancelled. Closure remains fail-closed and ownership promotion remains under
the existing M12 contracts.

**Consequences:** AUTO65 can support 100+ pending chains and process smaller
batches without becoming a serial address queue. A single broad capture can
serve many investigations, while unresolved activity remains an explicit
frontier. The campaign remains developer-only and non-owning.

**Evidence:** `docs/reports/THOR_M12_AUTO65_MULTI_CHAIN_CAMPAIGN.md`.

# ADR-0042 — Keep 0x062AE0 natural evidence developer-only
**Status:** Accepted for M11.61
**Date:** 2026-09-10

**Context:** The natural callee crossed by M11.60 G0 preserves A5 exactly and
has a complete observed safe-RAM/G0-relative/stack effect set. Its static CFG
also contains an unresolved indexed JSR at `0x062CEC` and multiple latent RTS
exits.

**Decision:** Record only the natural preservation/effect contract with a
developer-only observer and regression test. Classify the static boundary as
`INDIRECT_CFG`; retain the M11.60 parent-lifetime transaction gate and typed
data block. Do not recurse into a subsystem or add a production abstraction.

**Consequences:** One G0 callee dependency is reduced to an exact natural
fact. The latent indirect path and other G0 callees remain explicit blockers.

**Evidence:** `docs/reports/CALLEE_062AE0_CONTRACT_M11_61.md`.

# ADR-0041 — Keep the 0x060182 A5 lifetime parent-owned
**Status:** Accepted for M11.60
**Date:** 2026-09-10

**Context:** The natural generation written by `0x060182 LEA FF001A,A5` has a
bounded CFG and exact +0/+4 consumers, while the +7 arm is dead. Its endpoint
is the parent's `0x06027E MOVEM` restore. Calls crossed while G0 is live do not
yet have complete whole-callee effect proofs, and M11.59 retains broader raw
aliases and external writers.

**Decision:** Record the generation with a developer-only hybrid observer and
decoder regression test. Classify it as `A5_LIFETIME_MERGES_WITH_PARENT` and
keep the transaction and typed-data gates fail-closed. Do not add a type,
subsystem, production helper, or native routine.

**Consequences:** The exact consumer/lifetime evidence is reusable for one
future callee-preservation closure. Parent frame, GPGX, timing, ROM-PC and
raw-storage ownership remain outside `oasis_core`.

**Evidence:** `docs/reports/A5_CONSUMER_LIFETIME_M11_60.md`.

# ADR-0040 — Preserve raw ownership boundary after M11.59 census
**Status:** Accepted for M11.59
**Date:** 2026-09-10

**Context:** M11.58 left the typed-data gate open because fixed bytes had
external writers and the A5-derived range had unresolved alias/lifetime
evidence. M11.59 found three A5 materializers; one path saves and reloads A5
and post-increments it through a broad range, while other routines consume
multiple derived offsets. The sibling 0x60BCC writers also retain a hardware
prefix.

**Decision:** Keep `ParentSuffix + RamFlag` raw and parent-parameterized. Do
not add a typed structure, shared-memory owner, subsystem wrapper or
0x60BCC promotion. Permit only a standalone decoder provenance regression
test; keep ROM/GPGX, hardware ordering, lifetime and continuation evidence in
developer-only tooling and the parent adapter.

**Consequences:** The aliasing boundary is proven negative for typed
replacement while the behavior-cluster contract remains valid. M11.60 must
close one bounded A5 consumer/lifetime contract before revisiting typed data.

**Evidence:** `docs/reports/RAW_DATA_OWNERSHIP_M11_59.md`.

# ADR-0039 — Keep the first behavior cluster raw and parent-parameterized
**Status:** Accepted for M11.58
**Date:** 2026-09-10

**Context:** M11.57 proved a parent-owned suffix helper that composes the
portable RamFlagRoutine. M11.58 audited its exact raw footprint and bounded
ROM/runtime provenance. The fixed `FF0010..FF0014` bytes have known external
writers, the flag addresses are used by other bounded code, and the
`FF001A + 5..7` derived range has unresolved alias and lifetime boundaries.

**Decision:** Treat the existing ParentSuffix + RamFlag composition as the
portable behavior-cluster contract, retaining raw address parameters and each
component's independent continuation tokens. Do not add a typed shared-memory
structure, opaque replacement wrapper, subsystem owner or new routine. Keep
parent frame/SR/hardware/epilogue/RTS ownership and all ROM/GPGX provenance in
the parent or developer-only hybrid adapter.

**Consequences:** The cluster can be tested and shadow/native-proven without
inventing gameplay meaning or hiding unresolved ownership. M11.59 must close
one dominant alias/lifetime and external-writer blocker before any typed data
replacement is considered. `PORTABLE_SUBSYSTEM_BOUNDARY_PROVEN` remains false.

**Evidence:** `docs/reports/PORTABLE_BEHAVIOR_CLUSTER_M11_58.md`.

# ADR-0031 — Portable mechanical primitive layer in `oasis_core`
**Status:** Accepted for M11.50
**Date:** 2026-09-09

**Context:** M11.49 proved one generic resumable executor for four exact
copy/clear plus DBF loops, but its implementation still lived in the
developer-only hybrid library. The semantics were reusable; the registry,
canonical bytes, GPGX bridge, shadow comparator and evidence were not.

**Decision:** Move only the generic mechanical contract and executor into
`oasis_core`. Use opaque instruction tokens, a portable machine interface and
explicit continuation state. Keep ROM PCs, candidate names, canonical opcodes
and displacement, GPGX/BasicBlock timing/prefetch adapters, shadow snapshots,
metrics and reporting in `tools/hybrid`. Add a standalone core test and a
repository-visible dependency-boundary check. Do not add new primitive forms,
gameplay semantics or hardware modeling.

**Consequences:** There is one implementation of copy/clear/DBF semantics and
it is independently linkable without GPGX/libretro. The hybrid adapter remains
responsible for hardware/bus mapping and canonical provenance. The extracted
layer preserves the M11.49 shadow/native identity; future native routines may
compose these operations only after separate evidence closes their contracts.

**Affected files/milestones:** `src/core/mechanical_primitive.*`, hybrid
adapter/registry, standalone core and boundary tests, CMake and M11.50 report.

# ADR-0030 — Metadata-driven mechanical primitive family
**Status:** Accepted for M11.49 developer-only hybrid tooling
**Date:** 2026-09-09

**Context:** M11.48 proved one resumable `CLR.B`/`DBF` loop. M11.47 also
identified two byte copy loops and one word clear loop, but the family
abstraction, exact source/destination and width contracts, and coexistence
behavior had not been proven.

**Decision:** Generalize the architecture-neutral primitive executor around a
metadata-only `MechanicalLoopContract`. Promote only the exact two
`MOVE.B (A2)+,(A1)+` plus DBF forms and `CLR.W (A0)+` plus DBF form that pass
deterministic vectors, generated-oracle shadow, existing bus/timing/refresh
contracts and unchanged native identity. Keep ROM PCs in registry metadata,
keep generated code as oracle/fallback, use explicit ordered byte copy and
width-specific clear writes, and fail closed on unsupported forms. Do not
broaden hardware emulation or add a production dependency.

**Consequences:** Four mechanical loops share one reusable implementation and
preserve exact 32-bit address, DBF, CCR/X, bus ordering and interruption
behavior. Native execution represents 42,384 guest instructions while the
generated-plus-mechanical translated-equivalent count remains 6,241,765;
checkpoint/video, yields, resumptions and CPU equivalence remain unchanged.
The promoted family has no hardware-visible access; the existing hardware
boundary remains unchanged. Extraction is a future boundary, not part of M11.49.

**Affected files/milestones:** `mechanical_primitive.*`, runner report,
synthetic primitive test, M11.49 governance and evidence report.

# ADR-0029 — Resumable native mechanical primitive layer
**Status:** Accepted for M11.48 developer-only hybrid tooling
**Date:** 2026-09-09

**Context:** M11.47 proved repeated safe-memory copy/clear structures but did
not define a higher-level replacement contract. The first replacement must
preserve instruction-boundary timing, refresh, event/interrupt behavior and
continuation state, while remaining independent of GPGX internals.

**Decision:** Add a small architecture-neutral `MechanicalMachine` interface
and resumable primitive registry in `src/tools/hybrid/mechanical_primitive.*`.
Promote only the exact `CLR.B (A5)+` plus `DBF D0` loop at `0x061266`/`0x061268`.
Dispatch metadata identifies the proven ROM loop; the implementation executes
one guest body/DBF pair at a time and fails closed on canonical-byte or bridge
contract mismatch. Keep generated blocks and generic basic-block glue separate;
retain the generated path as the shadow oracle and fallback for all other PCs.
Do not add gameplay meaning, hardware emulation or a production dependency.

**Consequences:** The native path represents 3,780 guest instructions through
1,890 resumable clear iterations and preserves 86 synthetic/observed
mid-operation yields in shadow evidence. The unchanged 600-frame proof retains
checkpoint/video/CPU identity, 150,135 total yields, 288 resumptions and zero
fallback/hardware accesses. The three other discovered loops remain unpromoted.

**Affected files/milestones:** M11.48 mechanical primitive API, runner/CMake,
synthetic primitive test, governance and evidence report.

# ADR-0028 — Bounded safe-memory semantic tranche and mechanical primitives
**Status:** Accepted for M11.47 developer-only hybrid tooling
**Date:** 2026-09-09

**Context:** M11.46 resolved runtime memory classes for a bounded set of
interpreter PCs. Seven high-payoff rows were observed only in safe main RAM,
but their exact instruction semantics and bus effects still required proof.
Repeated copy/clear loops also appeared mechanically structured, while their
native higher-level replacement contract was not yet needed.

**Decision:** Independently verify only the exact seven forms required by the
M11.47 safe-memory tranche, emit them through the decoder-owned mechanical
generator and shared runtime helpers, and certify each through the existing
instruction-boundary GPGX shadow and unchanged 600-frame native gates. Keep
hardware-reachable, mixed, unresolved, indirect-CFG and decoder rows
fail-closed. Record copy/clear structures as mechanical future replacement
candidates only; do not replace loops or assign gameplay semantics in M11.47.

**Consequences:** Seven generated ranges remove exactly 42,047 interpreter
executions, raising translated execution to 96.1933% while preserving the
authoritative checkpoint/video identity, bus/timing/event contract and zero
hardware-visible native accesses. The remaining ledger closes at 247,008
executions. Generated bodies, registry metadata, shared helpers and handwritten
boundary glue remain separate.

**Affected files/milestones:** M11.47 hybrid generator/runtime, generated
block registry, semantic regression test and M11.47 reports.

# ADR-0027 — Observe runtime addresses without promotion
**Status:** Accepted for M11.46 developer-only hybrid tooling
**Date:** 2026-09-09

**Context:** M11.45 left 149,678 interpreter executions in 635
`UNKNOWN_WITH_EVIDENCE` register-address PCs. Static decoding could not prove
their runtime memory classes, while the existing GPGX path already exposes
instruction and top-level data-bus boundaries.

**Decision:** Add a separate observer mode around the existing GPGX hook and
M11.45 block registry. Record PC counts, data-bus address/width/direction/order
and A-register transitions, and classify only observed addresses using the
existing Genesis address contract. Keep mapper-dependent cartridge SRAM
unproven when address alone is insufficient. Do not promote semantics, broaden
hardware emulation, add address-specific execution bodies or connect the
observer to production runtime.

**Consequences:** Three independent processes matched the M11.45 identity and
metrics. Runtime classes were resolved for 147,847 executions; 1,831
address-computation-only executions remain exact-evidence unresolved. The result
is `BOUNDED_RUNTIME_ADDRESS_PROVENANCE_PROVEN`; M11.45 history and all prior
negative gates remain intact.

# Architecture Decision Log

Use this file for decisions that can redirect architecture, dependencies, scope, or reverse-engineering strategy.

## ADR-0026 — Bounded semantic closure with independent shadow vetoes
**Status:** Accepted for M11.45 developer-only hybrid tooling
**Date:** 2026-09-09

**Context:** M11.44 left 560,968 interpreter executions. The highest-payoff
decoder-backed semantic/proven candidates could exceed the 236,530 execution
threshold needed for a 95% result, but exact GPGX timing and flags remained
unproven for new forms.

**Decision:** Select only decoder-owned rows with proven static memory class and
exact mechanical support, excluding register-based/other memory, hardware,
indirect CFG, decoder gaps and unknown runtime addresses. Verify exact helpers
with deterministic vectors, generate all candidate bodies mechanically, and let
the existing per-boundary shadow gate veto candidates independently. Do not add
candidate-specific timing constants or hardware behavior.

**Consequences:** 551 candidates passed 6,199,718 shadow comparisons and native
proof, removing 271,913 executions. LSR.W, ROR.W and CMPI.B were retained as
fail-closed shadow rejections. The resulting 95.5453% translated share proves
`REMAINING_ATTRIBUTION_AND_DYNAMIC_COVERAGE_95_PROVEN`; the final ledger still
accounts for every 289,055 interpreter execution.

## ADR-0025 — Promote only the canonical 0x03A7AE generated block
**Status:** Accepted for M11.44 developer-only hybrid tooling
**Date:** 2026-09-09

**Context:** After M11.43 restored the authoritative checkpoint identity, the
M11.42 restart gate reproduced the frozen 661,916 interpreter remainder. The
hottest pair, 0x03A7AE `TST.W ($00FF1654).L` and 0x03A7B4 `BNE.W`, retained the
M11.35 IR/prefetch rejection (`actual 0x4E73`, expected `0x4A79`).

**Decision:** Re-run the exact decoder-owned pair through the current generic
timing/refresh, instruction-boundary, checkpoint-canonicalization and GPGX
shadow contracts. Promote it only as generator output with no address-specific
semantic body or timing constant. Keep all other remaining forms fail-closed,
retain 0x060BA4 as hardware-visible fallback, and close the complete remainder
with a per-PC ledger rather than using an `other` bucket.

**Consequences:** The historical rejection is preserved but classified obsolete.
The pair passed 100,948 per-instruction shadow comparisons with zero divergence;
the unchanged 600-frame native proof preserves checkpoint/video/CPU/boundary
identity and removes 100,948 interpreter executions. Final translated share is
91.3548%, below the 95% gate, so the result is
`REMAINING_INTERPRETER_ATTRIBUTION_PROVEN_SEMANTICS_BLOCKED`.

## ADR-0024 — Canonicalize only the proven pinned-GPGX representation layout
**Status:** Accepted for M11.43 developer-only hybrid tooling
**Date:** 2026-09-09

**Context:** M11.42 found that the M11.41 adapter left a YM2612 host pointer at
serialized offset `140734`. The pinned DLL's raw state also showed its Z80
`daisy` pointer at `144576`. The available external `state.c` source and stale
build objects were not byte-for-byte synchronized with that DLL: source-only
arithmetic predicts an earlier Z80 position, while raw cart mapping and pointer
evidence prove the pinned binary position.

**Decision:** Model the raw-saved YM2612/Z80 structs with machine-checked x64
`sizeof`/`offsetof` assertions and construct one non-overlapping representation
span table from those fields. Guard `STATE_SIZE=0xfd000` and version
`GENPLUS-GX 1.7.6`; reject unknown values. Canonicalization copies the raw
buffer and clears only the table's 55 host-pointer, one function-pointer and
111 ABI-padding spans. It does not clear semantic bytes or broaden hardware
serialization. The active USA cart path is separately serialized field-by-field;
optional SVP wholesale state is out of scope for this active baseline.

**Consequences:** The complete contract produces the new authoritative aggregate
`251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d` across five
current proofs and current/exact-historical/M11.41 raw replay. M11.41's
`c9236218...` remains historical and is superseded because its adapter erased
semantic bytes. M11.42 PHASE 2 remains unstarted.

## ADR-0023 — Keep M11.42 closed on checkpoint identity mismatch
**Status:** Accepted for M11.42 gate governance
**Date:** 2026-09-09

**Context:** The M11.41 repair was expected to reproduce authoritative
checkpoint aggregate `c9236218...` from commit `5c19e22`. Two fresh runs matched
all execution and video metrics but produced `d5de401c...`. Raw evidence shows
the M11.41 canonicalization leaves a host-pointer byte at serialized offset
`140734`.

**Decision:** Treat the restart as blocked. Do not classify the remaining
interpreter executions, broaden semantic forms, generate candidates, run shadow
promotion, broaden hardware behavior or weaken the checkpoint identity gate.
Repair and independently re-prove the developer-only identity contract in a
separate bounded task before resuming M11.42.

**Consequences:** M11.39, M11.40 and M11.41 evidence remain preserved; no
coverage or attribution result is claimed from this run. Production/native
architecture is unchanged.

## ADR-0022 — Deterministic developer-only checkpoint identity
**Status:** Accepted for M11.41 developer-only hybrid tooling
**Date:** 2026-09-09

**Context:** M11.40 matched ROM, GPGX, video, instruction and boundary metrics
but could not reproduce the historical checkpoint aggregate. Byte evidence
showed that GPGX v1.7.6 wholesale-serialized host pointers and ABI padding in
the YM2612 and Z80 contexts. The first mismatch was frame 60, state offset
`140654`, inside `FM_SLOT.DT`.

**Decision:** Preserve the complete raw `retro_serialize()` buffer in ignored
developer evidence, but compute the authoritative checkpoint identity from a
copy with only the proven pointer/padding spans cleared for the recognized
`STATE_SIZE=0xfd000` format. Reject unknown state sizes. Keep the existing
frame cadence and aggregate order; retain all semantic state bytes and record
both raw and authoritative per-record hashes.

**Consequences:** Checkpoint identity is deterministic across the current and
exact historical checkout: the repaired 600-frame aggregate is
`c9236218f55fb18f7f1d5095e4970b25f228de588bd03bd7cbbffccc2e225fd1`. Raw
evidence remains available for future audits but is not tracked. This does not
start M11.40 PHASE 2 and does not add a production emulator dependency.

## ADR-0021 — Bounded hot-path profile and exact multi-block promotion
**Status:** Accepted for M11.39 developer-only hybrid tooling
**Date:** 2026-09-09

**Context:** M11.38 proved generic instruction-boundary continuation, but
661,916 interpreter executions remained after the exact M11.38 registry. The
next coverage step needed a dynamic-payoff ranking without becoming an
indefinite discovery pass or weakening the exactness gate.

**Decision:** Emit a deterministic interpreter-PC profile from the existing
M11.38 registry run, rank by dynamic instruction executions, and consider a
bounded maximum of 40 decoder-owned ranges. Promote only candidates whose exact
forms pass the independent semantic harness, whose bodies are generated from
canonical ROM provenance, and whose every instruction boundary passes the
existing GPGX shadow contract. Keep indirect control unresolved and hardware
visible candidates in fallback unless an existing bridge already proves them.

The M11.39 set contains `[0x000380,0x0003A0)` and `[0x03A864,0x03A868)`.
The first required one new exact semantic form, `ADD.W (An)+,Dn`; the second
reused the verified direct Bcc form. Generated code and metadata remain
separate from handwritten helper/registry glue, and promotion remains offline.

**Consequences:** Native translated dynamic share rose from 63.5261% to
89.7991% with exact 600-frame checkpoint/video, CPU, interrupt and boundary
evidence. The remaining profile is reported conservatively; no latent function
boundaries or indirect target sets are claimed. M11.40 remains a separate
milestone and is not implemented here.

## ADR-0020 — Generic instruction-boundary yields for rejected multi-instruction blocks
**Status:** Accepted for M11.38 developer-only hybrid tooling
**Date:** 2026-09-08

**Context:** M11.37 rejected four mechanically generated two-instruction ranges
because natural execution showed interrupt interleaving inside their ranges.
GPGX owns interrupt service, trace handling, cycle/refresh state and the VDP/Z80
schedule; treating the ranges as atomic would change observable ordering.

**Decision:** Make generated execution instruction-granular at the bridge
boundary. After each guest instruction, the generated body asks one generic
GPGX-owned boundary callback and returns `BlockExit` with the exact next PC,
reason and executed count when continuation is required. The shadow adapter
compares registers, SR, PC, prefetch, RAM/bus effects, cycle/refresh and
interrupt-visible fields at every boundary. Native continuation re-enters the
same generated block at the exact instruction entry PC; GPGX performs any
pending interrupt before the continuation. Do not add candidate-specific
timing, a second scheduler, atomic blocks, runtime JIT or production linkage.

**Consequences:** The four frozen M11.37 ranges pass the per-boundary shadow
gate and native 600-frame equivalence; 274 naturally observed continuations
resumed after actual GPGX interrupt service. The capability is conservative and
instruction-granular, and further coverage still requires a separate milestone.

## ADR-0019 — Controlled dynamic block promotion remains offline and generic
**Status:** Accepted for M11.37 developer-only hybrid tooling
**Date:** 2026-09-08

**Context:** M11.36 made the GPGX timing/refresh boundary authoritative. M11.37
needed to expand coverage beyond six entries without turning discovery into a
runtime JIT or trusting generated code by construction.

**Decision:** Use one bounded natural trace, an explicit candidate queue,
independent semantic verification of the used Bcc/DBcc/TST forms, existing
decoder-owned mechanical generation, and the same GPGX shadow gate before
promotion. Store block boundaries and instruction counts as generated metadata;
the registry dispatches through that metadata rather than a candidate-specific
hard-coded list. Reject a candidate if its block crosses an observed interrupt
or hardware-visible boundary. Keep all discovery, generated bodies and GPGX
bridge code in developer-only tooling; do not add runtime JIT, whole-ROM
translation or ROM-byte coverage claims.

**Consequences:** Sixteen new single-instruction natural blocks passed the
full shadow/native gate. Four multi-instruction candidates remain unpromoted
because the trace showed interrupt interleaving; a hardware-visible candidate
was rejected fail-closed. Further expansion requires a new bounded milestone.

## ADR-0018 — GPGX post-instruction bridge comparison boundary
**Status:** Accepted for M11.36 developer-only hybrid tooling
**Date:** 2026-09-08

**Context:** M11.35 compared a prediction sampled before a direct TST entry with
an execution event observed at the next instruction boundary. GPGX rebases its
frame-relative `m68k.cycles` and `refresh_cycles` counters at frame end, so the
first mismatch was `896114/896268` versus `74/228` even though the instruction
effect itself was exact.

**Decision:** Keep GPGX as the timing, refresh, hardware and interrupt oracle.
Expose one generic post-instruction hook from the normal interpreter after
semantic and cycle/refresh advancement, before the next scheduler/frame
transition. Close shadow comparison there. Keep the existing entry hook for
block dispatch. A translated block remains valid only for bounded code whose
accesses cannot expose a GPGX-observable boundary; interrupt polling and trace
handling remain outside generated semantics and are never crossed atomically.

**Consequences:** No baseline subtraction, magic constant, candidate-address
patch or parallel timing engine is needed. The old M11.35 negative result stays
historical. The existing M11.33 plus three M11.35 blocks pass the full shadow
and native 600-frame gate; future blocks still require the same exact contract.

## ADR-0017 — Fail-closed demand-driven block promotion gate
**Status:** Accepted for M11.35 developer-only pilot
**Date:** 2026-09-08

**Context:** M11.33 generated three proven blocks and M11.34 independently
verified their used instruction semantics. The next bounded experiment needs a
repeatable way to select naturally executed blocks without turning the project
into a whole-ROM recompiler or trusting generated output by construction.

**Decision:** Permit only one bounded natural trace, exact decoder-owned block
IR, independent vectors for newly required forms, provenance-bound generated
bodies and full GPGX shadow comparison before promotion. Keep candidate bodies
and discovery evidence in developer-only tooling. If semantic or runtime shadow
evidence fails, do not promote the candidate and retain interpreter fallback.
Do not add runtime JIT, automatic trust, or whole-game ranking in this pilot.

**Consequences:** M11.35 may end in a negative gate result while preserving
useful discovery, semantic and generator evidence. The first runtime blocker is
recorded rather than hidden by running native execution. Any future promotion
must establish the missing cycle/refresh and interrupt-boundary contract in
a separately bounded task.

**Result:** `DEMAND_DRIVEN_PROMOTION_RUNTIME_BLOCKED` at `0x3A85E`, with
`actual_cycles=74 expected_cycles=896114 actual_refresh=228
expected_refresh=896268`.

## ADR-0016 — Test-only independent M68K semantic oracle
**Status:** Accepted for M11.34 verification tooling
**Date:** 2026-09-08

**Context:** M11.33 established decoder-owned generated execution, but shared
decoder, emitter and helper assumptions could agree while encoding an incorrect
68000 rule.

**Decision:** Verify only the seven currently emitted M11.33 combinations with
a small deterministic reference model transcribed from the Motorola/NXP 68000
Programmer's Reference Manual. Keep it in tests/developer tooling, separate
from the decoder, emitter and production targets; do not broaden the instruction
surface to make the verification table larger.

**Consequences:** A mismatch must remain visible as a first-mismatch vector and
be fixed only with sufficient evidence. Passing this checkpoint establishes
independent verification of the bounded used subset, not a general CPU core or
whole-ROM semantic claim.

**Reference:** `https://www.nxp.com/docs/en/reference-manual/M68000PRM.pdf`

## ADR-0015 — Decoder-owned generated basic-block bodies
**Status:** Accepted for developer-only migration tooling
**Date:** 2026-09-08

**Context:** M11.32 proved three manually written basic-block bodies, while
the next milestone must remove routine-specific instruction authoring without
changing the proven GPGX timing and hardware boundary.

**Decision:** M11.33 uses the existing decoder/exact IR to generate C++ calls
to a small hybrid instruction-helper boundary. Generated functions retain the
guest PC, opcode and decoded assembly as provenance. The initial generated set
is exactly the three M11.32 blocks; unsupported forms fail closed. The oracle
prediction path remains separate until M11.34 verifies common semantics.

**Consequences:** Adding an ordinary supported block can use generator output
rather than a handwritten body, while the hybrid bridge, GPGX state and
generated artifact remain developer-only. This does not create a production
CPU emulator or authorize semantic promotion.

**Affected files/milestones:** `src/tools/hybrid/`, `CMakeLists.txt`, M11.33.

## ADR-0014 — Developer-only GPGX basic-block replacement boundary
**Status:** Accepted for migration experiments only
**Date:** 2026-09-08

**Context:** The M11.30 atomic override skipped GPGX instruction-body timing
and diverged in serialized VDP/sound state. A bounded proof needs native
mechanical execution while preserving GPGX's own fetch, memory bus, prefetch,
cycle and refresh behavior.

**Decision:** Add one developer-only block callback immediately before GPGX's
opcode dispatch. A registered block may perform its exact bounded operations
through helper functions implemented inside GPGX, then return control to the
normal CPU loop. The initial registry contains only `0x2D66`, `0x604BC` and
`0x61032`, each with an explicit state/effect contract. This is not a generic
M68K replacement engine and is not linked by production Sega-Thor targets.

**Consequences:** The three-block 600-frame proof can reuse GPGX hardware and
timing semantics and has passed with exact state/video equivalence. Every new
block still requires an independent shadow proof; uncertain side effects must
fail closed. The bridge, ROM PCs, emulated memory and GPGX remain confined to
developer tooling. See `docs/reports/BASIC_BLOCK_RECOMPILATION_TIMING_M11_32.md`.

## ADR-0001 — Native C++ reimplementation, not a general emulator
**Status:** Accepted

**Context:** The goal is to make Beyond Oasis run natively on modern systems while preserving original behavior.

**Decision:** Translate game routines and implement only the Mega Drive hardware semantics required by the game. Do not build a general 68000/Mega Drive emulator as the main architecture.

**Consequences:**
- more reverse-engineering work up front;
- clearer native game code long term;
- hardware compatibility layer must remain narrow;
- address/routine mappings must be preserved for traceability.

## ADR-0002 — User-supplied ROM owns commercial data
**Status:** Accepted

**Decision:** The repository never contains the original ROM or extracted commercial assets. Runtime/tools read a locally supplied ROM.

**Consequences:**
- Git repository remains source-only;
- CI tests use synthetic/non-copyrighted fixtures;
- local developer tools may export ignored files for inspection.

## ADR-0003 — Fidelity before enhancements
**Status:** Accepted

**Decision:** Reproduce original gameplay and rendering semantics before widescreen, HD, QoL, remaster behavior or Story of Thor 2 work.

## ADR-0004 — 500-line hard file limit
**Status:** Accepted

**Decision:** Source and project documentation files must not exceed 500 lines.

**Reason:** Keep modules understandable for humans and AI agents, discourage monoliths, make review and context retrieval reliable.

## ADR-0005 — USA reference binary, region-independent reconstructed game
**Status:** Accepted
**Date:** 2026-09-03

**Context:** The project aims to reconstruct the complete game so the native C++ runtime can target desktop, mobile, and future consoles. Regional retail binaries differ, while existing public reverse-engineering work and known addresses are based on the USA `Beyond Oasis` release.

**Decision:**
- Use the clean USA retail `Beyond Oasis` binary as the canonical engineering reference for addresses, traces, and differential verification.
- The reconstructed game model must be region-independent and must not embed USA ROM addresses in gameplay code.
- Europe and Japan are secondary evidence sources and future data profiles, not separate game implementations.
- ROM-specific offsets belong only in extraction/reverse-engineering metadata.
- Runtime gameplay code consumes normalized game data structures rather than raw ROM addresses.


**Alternatives considered:**
- Europe as primary reference: rejected because current public address knowledge targets USA.
- Supporting all regions equally from the beginning: rejected because it multiplies binary-diff work before core behavior is understood.
- Building a USA-only final runtime: rejected because it conflicts with the portable reconstruction goal.

**Consequences:**
- address annotations default to USA reference addresses;
- later regional support maps region-specific data to the same C++ game model;
- regional differences are documented rather than forked into separate engines;
- reference identity must be established before translating substantial 68000 routines.

**Affected files/milestones:** M2 onward, all reverse-engineering documentation and extraction code.

## ADR-0006 — Separate reassembly exactness from execution trust
**Status:** Accepted
**Date:** 2026-09-06

**Context:** M11.14 `CODE_VERIFIED` proved exact decoder/assembler bytes but did
not prove that each reconstructed range is executed code. Treating those facts
as one classification could make weak structural evidence appear trusted.

**Decision:** Keep the existing full-ROM ownership and ASM artifacts, while
reporting an evidence ladder: `ASM_ROUNDTRIP_EXACT`, `CODE_STATIC_SUPPORTED`,
`CODE_EXECUTED` and `BEHAVIOR_VERIFIED`. Static support requires an independent
anchor, vector/startup provenance, dynamic evidence, or an exact incoming xref
from an already trusted caller. Direct-caller counts and Ghidra boundary
agreement alone do not raise trust. The automatic promoter records a successful
round trip at the lowest level unless explicit evidence qualifies it.

**Consequences:** Existing ranges can be downgraded without changing bytes or
the exact full-ROM rebuild. Audits must retain artifact identity, ROM identity,
entry/range linkage and concrete xref sources. Missing dynamic evidence remains
unknown and is never synthesized.

## ADR-0007 — Natural runtime evidence is range-local and fail-closed
**Status:** Accepted
**Date:** 2026-09-06

**Context:** M11.16 needs to distinguish a naturally observed program counter
from a forced hook, a stale report, or a report whose provenance artifact is
missing. Dynamic reachability of one routine must not silently trust callers,
callees, or adjacent ranges.

**Decision:** Accept only `DYNAMIC_NATURAL` evidence whose ROM identity,
scenario, artifact hash, target address and audited half-open range all match.
Reuse of an existing artifact is preferred to rerunning an emulator. Missing
or forced evidence remains non-promoting, and trust changes are local to the
observed range. A pre-run selection report is mandatory for bounded target
passes.

**Consequences:** Dynamic passes can end with a reachability-limited result
without weakening static exactness. Historical runtime claims without a
retained artifact are explicitly reported as unaccepted context. Callers and
callees require their own evidence.

## ADR-0008 — Structured data classifications are explicit and non-heuristic
**Status:** Accepted
**Date:** 2026-09-06

**Context:** The full-ROM split contains millions of bytes whose apparent
shape is not proof of data. M11.17 needs a useful data inventory while keeping
unknown bytes and code/data boundaries honest.

**Decision:** Accept `DATA_STRUCTURE_SUPPORTED` only when an exact range,
element width, deterministic count/end, canonical byte identity and a proven
consumer or parser are present. Use `DATA_REGION_SUPPORTED` for a bounded
non-code region whose field semantics are incomplete. A trusted data range
creates a conflict record on code overlap and vetoes future code promotion;
weak or unknown data hypotheses do not veto.

**Consequences:** Structured data can be reported without replacing the
blob-backed full-ROM representation. Resource payloads remain unknown until
their own compressed boundaries are proven, and code/data conflicts cannot be
resolved silently.

## ADR-0009 — Bounded native controlled-screen platform seam
**Status:** Accepted
**Date:** 2026-09-06

**Context:** M11.18 requires the first interactive native vertical slice while
reconstructed-source and broad RE expansion are frozen. The repository has
portable runtime/game logic but no window or input backend.

**Decision:** Add a small `oasis_platform` adapter. On Windows it owns a native
Win32 window, keyboard polling, focus-loss clearing and scaled software-DIB
presentation. The game layer owns the deterministic fixture, movement and
software rasterization. Non-Windows builds keep a compile-only unavailable
adapter until a concrete backend is justified.

**Consequences:** The core/game path remains platform-independent and has no
new third-party dependency. The first playable runtime is Windows-only; this
is an explicit bounded limitation, not a claim of cross-platform GUI support.
The screen geometry is synthetic and cannot be used as evidence for an
original room.

**Affected files/milestones:** `src/platform/`, `src/game/controlled_screen.*`,
`src/game/render/`, M11.18.

## ADR-0010 — Address-level GPGX manual runtime evidence
**Status:** Accepted
**Date:** 2026-09-07

**Context:** The instrumented Genesis Plus GX core now provides persistent
manual-realtime executed-PC bitmaps for the canonical Beyond Oasis ROM. This
is stronger evidence than static reachability for individual instruction
starts, but it does not establish function boundaries, semantics, or complete
range execution.

**Decision:** Add `GPGX_MANUAL_REALTIME` as a provenance-bound evidence source
that records only `CODE_EXECUTED_AT_ADDRESS` facts. The importer must verify
the canonical ROM identity, bitmap size and metadata/file hashes, retain
unsupported decoder results, and reject odd, out-of-range or data-conflicting
addresses. Evidence is duplicate-safe by ROM, source and capture identity.
It may report existing-range coverage, but it must not promote or otherwise
change range classifications.

**Alternatives considered:** Promoting an entire static range from one
executed PC was rejected because it invents boundaries and branch coverage.
Reclassifying trusted data on runtime overlap was rejected because conflicts
must remain explicit and fail-closed. Replay or input automation was rejected
because this source is specifically manual realtime capture.

**Consequences:** Runtime execution evidence is available to later trust
analysis at address granularity. Unknown executed addresses remain separately
reported as `RUNTIME_EXECUTED_UNKNOWN`; `RUNTIME_DATA_CONFLICT` is retained as
a critical diagnostic. No gameplay code, `main`, ROM or extracted asset is
changed.

**Affected files/milestones:** `src/tools/gpgx_import_gpgx_coverage.cpp`,
`CMakeLists.txt`, M11.19.

## ADR-0011 — Developer-only hybrid migration experiment
**Status:** Accepted
**Date:** 2026-09-07

**Context:** The user explicitly replaces M11.27 manual ID3 hunting with a
bounded hybrid execution PoC for the already translated `0x3820` decompressor.

**Decision:** A developer-only adapter may observe natural GPGX calls, run
existing C++ translations on copied bounded inputs and compare their effects.
Its dispatch states are `EMULATED`, `SHADOW_NATIVE`, `NATIVE_OVERRIDE`. Shadow
always preserves the original CPU result. Override must fail closed until all
CPU, memory, return and execution-timing effects needed by continuation are
proven; output equivalence alone cannot authorize it.

**Consequences:** GPGX remains an external developer tool. No production target
may depend on the emulator, its CPU context, original PCs or its RAM layout
through this experiment. Exactly one routine is in scope; no emulator rewrite,
AI generation, routine expansion or manual gameplay search is authorized.

## ADR-0012 — Target-specific developer-only native override boundary
**Status:** Accepted
**Date:** 2026-09-07

**Context:** M11.29 requires one native override whose CPU/hardware contract is
simpler than `0x3820`, while preserving the original GPGX execution path and
avoiding a generic replacement engine.

**Decision:** Use the naturally executed `0x2D66` leaf as the single override
target. Its adapter captures and proves only its bounded registers, full SR,
source/output RAM and saved stack window. On override it applies those effects
and uses GPGX's existing `m68k_set_reg(PC)` transition rather than reimplementing
prefetch or IR state. The adapter stays in developer-only hybrid tooling.

**Consequences:** The 600-frame neutral scenario proves one native replacement
and exact checkpoint/video continuation. The proof does not generalize to
other routines; `0x3820` remains blocked by its separate timing/CCR.X/prefetch
contract. Production targets and dependencies remain unchanged.

## ADR template
Copy this block for new decisions:

```text
## ADR-NNNN — Title
Status: Proposed | Accepted | Superseded | Rejected
Date: YYYY-MM-DD

Context:

Decision:

Alternatives considered:

Consequences:

Affected files/milestones:
```

## ADR-0010 — Minimal developer-only hybrid replacement registry
**Status:** Accepted for migration experiments only
**Date:** 2026-09-07

**Context:** M11.29 proved one target-specific override. M11.30 tests repeatability
across a small natural batch without making the emulator hook a production
runtime dependency or a generic M68K replacement engine.

**Decision:** Keep a small registry mapping explicit ROM PCs to target-owned
`EMULATED`, `SHADOW_NATIVE` and `NATIVE_OVERRIDE` adapters. Each adapter owns
its exact state/effect contract; the registry only routes hook events and
aggregates metrics. Return/prefetch transitions use GPGX's internal bridge
state. Timing and hardware phase are not synthesized by the registry.

**Consequence:** Multiple routines can share the migration boundary, while
unsafe candidates fail closed. The M11.30 batch shadow is clean, but override
promotion remains blocked until the exact GPGX bus-refresh/VDP/sound phase
contract is proven. Production Sega-Thor code remains emulator-free.

## ADR-0013 — Comparative projects are method references only
**Status:** Accepted
**Date:** 2026-09-08

**Context:** M11.31 compares public Streets of Rage 2/3 disassembly,
extraction and static-recompilation projects with the existing Beyond Oasis
evidence. The comparison found no public exact-ROM reassembly proof and no
evidence sufficient to establish shared programmer style, binary modules or
sound-driver lineage.

**Decision:** Transfer only the bounded evidence workflow: canonical identity,
address-preserving maps, explicit unknown/indirect-entry ledgers, natural
evidence gates, and separation of generated/mechanical output from native
helpers. Do not import SoR labels, RAM/object/audio assumptions, generated CPU
recompiler machinery or external code/data. Keep sound lineage as a separate
specialized investigation.

**Consequences:** M11.32, if started, remains a localized Beyond Oasis pass
using existing decoders/contracts. Similarity claims stay separated into
programmer style, binary structure, RE method and sound lineage. No production
runtime dependency changes.

**Affected files/milestones:** M11.31 report and governance documents; no
production source or ROM data.
# ADR-0032 — Keep first portable native routine shadow-only pending identity proof
**Status:** Accepted for M11.51
**Date:** 2026-09-09

**Context:** M11.29 supplied complete structural evidence for the bounded
`0x2D66..0x2D84` table-copy leaf, and M11.50 supplied the portable mechanical
ownership boundary. A structured `TableCopyRoutine` can now be extracted, but
the current pinned GPGX authoritative replacement does not preserve the frozen
M11.50 checkpoint aggregate even though video and isolated accounting match.

**Decision:** Keep the portable routine and zero-divergence shadow adapter as
developer-visible evidence, but do not promote it as authoritative execution.
The hybrid generated/interpreter path remains the oracle/fallback. Any future
promotion must independently close CPU, RAM, VDP, sound, interrupt,
timing/refresh and continuation identity against the frozen 600-frame run.
No timing constant, hardware behavior or gameplay meaning is invented to force
the gate.

**Consequences:** `oasis_core` demonstrates the next abstraction boundary
without weakening behavioral parity. The milestone result is
`PORTABLE_NATIVE_ROUTINE_SHADOW_PROVEN_REPLACEMENT_BLOCKED`; the next task must
explain the exact identity mismatch before another authoritative replacement.

**Affected files/milestones:** `src/core/table_copy_routine.*`, the 2D66
developer adapter and tests, M11.51 governance and report.

## ADR-0033 — Per-instruction hybrid continuation is required for native routine promotion
**Status:** Accepted for M11.52
**Date:** 2026-09-09

**Context:** M11.51's portable `TableCopyRoutine` shadow matched, but its
authoritative native adapter changed four canonical checkpoint bytes after
frame 120. The adapter's routine-level lump-sum cycle update produced the same
exit cycle count as the interpreter while missing intermediate GPGX refresh
sampling. The shadow comparison stopped before the scheduler-visible state
where the mismatch appeared.

**Decision:** Keep portable routine semantics in `oasis_core`. Extend the
developer-only hybrid bridge with instruction fetch and per-instruction
begin/finish callbacks, and require the adapter to reconstruct the exact
opcode/extension stream, DBF continuation decision, MOVEM dynamic timing and
RTS prefetch state. Do not use checkpoint canonicalization or aggregate-hash
exceptions to promote a native routine. Promotion requires paired per-frame
canonical checkpoint identity, video identity, continuation timing and
execution accounting.

**Alternatives considered:** Keep the lump-sum adapter; rejected because it
misses refresh/scheduler semantics. Canonicalize the four bytes; rejected
because RAM, sound and Z80 interrupt fields are semantic. Add a candidate-local
checkpoint patch or a full CPU emulator; rejected by project scope and evidence
rules.

**Consequences:** The 2D66 routine is promoted with a generic hybrid
continuation contract, while the generated/interpreter path remains the
oracle/fallback for other routines. Future routine adapters must prove their
own post-return and serialized-state boundary.

**Affected files/milestones:** `src/tools/hybrid/replacement.hpp`,
`src/tools/hybrid/gpgx_bridge.c`, `src/tools/hybrid/runner.cpp`, the 2D66
adapter/test and M11.52 evidence/governance.
# ADR-0034 — Second portable native routine contract
Status: Accepted for M11.53
Date: 2026-09-09

Context: M11.52 proved that native replacement requires per-instruction GPGX
continuation. M11.53 had one complete natural candidate, 0x604BC, whose old
adapter used a lump-sum timing handoff.

Decision: Extract only the structured BSET/Scc/LEA/RTS semantics of
0x604BC..0x604E6 into oasis_core with opaque tokens, portable registers and
memory, and explicit resumable boundaries. Keep ROM metadata, canonical
opcode/extension validation, GPGX fetch/begin/finish, prefetch/refresh,
boundary sampling, block-hook continuation return and RTS return state in
tools/hybrid. Promote only after
paired dual-routine checkpoint/video identity and separate accounting.

Consequences: TableCopyRoutine and the second routine coexist through the
existing registry without shared candidate state. No gameplay meaning,
hardware behavior, subsystem abstraction or second timing model is added.
The old lump-sum adapter path is removed for 0x604BC; generated execution
remains the oracle/fallback.

Affected files/milestones: src/core/ram_flag_routine.*, the 0x604BC adapter
and regression test, replacement accounting, M11.53 report.

# ADR-0035 — Stop at the first proven native routine cluster
Status: Accepted for M11.54
Date: 2026-09-09

Context: M11.52 and M11.53 authoritatively proved two portable native routine
contracts, TableCopyRoutine at 0x2D66..0x2D84 and RamFlagRoutine at
0x604BC..0x604E6. M11.54 reproduced the dual proof twice and audited exact
caller/callee and raw-memory evidence.

Decision: Classify the RamFlag direct callers at 0x604F6 and 0x60BCC, together
with the raw 0x00FF0010..0x00FF0016 window, as a CALL_GRAPH_CLUSTER and
MEMORY_STRUCTURE_CLUSTER. Do not call it a gameplay subsystem or move callers
into oasis_core until caller CFG, data ownership and hardware ordering close.
Keep TableCopy and RamFlag as separate ownership islands. Do not add a third
routine or typed data in M11.54.

Consequences: The architectural result is PORTABLE_ROUTINE_CLUSTER_PROVEN,
not FIRST_PORTABLE_SUBSYSTEM_BOUNDARY_IDENTIFIED. The next proposed task is a
bounded RamFlag caller/data closure audit. The existing dependency direction
is preserved: oasis_core owns portable semantics/tokens, while tools/hybrid
owns ROM provenance, GPGX continuation, oracle/accounting and hardware.

Affected files/milestones: M11.54 governance documents and
reports/NATIVE_ROUTINE_CLUSTER_M11_54.md; no production source change.

# ADR-0036 — Keep RamFlag caller regions developer-only after M11.55 closure
Status: Accepted for M11.55
Date: 2026-09-09

Context: M11.54 had static-only caller edges for `0x0604F6` and `0x060BCC`.
M11.55 added a natural-entry observer and reproduced the unchanged dual-native
proof twice. The observer deterministically attributes one `0x0604F6` entry
and three `0x060BCC` entries, while exact bounded slices expose continuation,
sibling-call, A5-relative memory and hardware-boundary gaps.

Decision: Accept `RAMFLAG_CALLER_CONTRACTS_PROVEN` only as a bounded caller-
region result. Keep attribution, ROM byte provenance, GPGX timing/hooks,
address provenance and the `0x00A11100` hardware interaction in
`tools/hybrid`. Treat the `0x0604F0` path and `0x060BC4` path as partial
contracts, not portable routines. Do not create a typed structure for
`FF0010..FF0016`; offset `FF0015`, A5-relative effects, lifetime and aliasing
remain unknown. Do not implement a caller or subsystem unless a future task
closes its complete continuation and memory/hardware contract.

Consequences: Dynamic caller provenance is now reproducible and fail-closed for
unknown caller classes without widening `oasis_core`. The exact
`0x060BC4` hardware-prefix/RamFlag-suffix ordering is recorded, but the wider
caller remains boundary-blocked. `PORTABLE_ROUTINE_CLUSTERS` remains 1 and
`PORTABLE_SUBSYSTEM_BOUNDARIES` remains 0. M11.56 must choose one falsifiable
continuation or sibling/data closure and is not executed by this decision.

Affected files/milestones: caller attribution observer/test, extracted hybrid
library wrapper, M11.55 governance documents and report; no production source
change.

# ADR-0037 — Keep the parent-owned 0x604F0 suffix internal
Status: Accepted for M11.56
Date: 2026-09-09

Context: Exact natural evidence identifies the previously unknown 0x611D6
destination as the shared saved-register/SR epilogue of the 0x60004/0x6042A
parent. 0x604F0 has no ordinary standalone return at A7; it inherits 58
bytes of parent state. The old decoder budget also includes another arm.

Decision: Record THIRD_ROUTINE_NOT_A_STANDALONE_ROUTINE with dominant blocker
ENCLOSING_ROUTINE_BOUNDARY. Do not manufacture a third routine by attaching
the shared restore/RTS tail or absorbing the parent's hardware prefix.
Retain full-SR/event continuation as partial. Keep the bounded read-only
observer and local evidence validator in tools/hybrid; do not change core,
RamFlag semantics, typed data or subsystem ownership.

Consequences: Structural continuation ownership is now proven for the natural
path, while a portable independent entry contract is still absent. Inventory
remains two authoritative routines and one cluster, with no subsystem boundary.
A future M11.57 may test an explicit parent/suffix handoff contract, preserving
the distinction between a portable internal helper and a complete routine.

Affected files/milestones: caller continuation observer/validator/test,
M11.56 report and governance. No production implementation.
# ADR-0038 — Parent-owned portable suffix helper
**Status:** Accepted for M11.57
**Date:** 2026-09-10

**Context:** M11.56 proved that `0x604F0` is an internal fallthrough carrying
the parent frame into a shared epilogue, so it cannot be promoted as a third
standalone routine. Its hardware-free suffix nevertheless has a closed
entry/exit contract and composes with the already-proven RamFlag routine.

**Decision:** Add one small architecture-neutral `ParentSuffixMachine` core
executor with opaque phase and parent-continuation tokens. It owns only the
five safe-RAM SF writes and structural RamFlag call. Keep ROM addresses,
canonical bytes, GPGX fetch/timing, nested BSR representation and the mapping
to `0x611D6` in the developer-only hybrid adapter. Keep parent frame/SR,
hardware, shared epilogue and RTS ownership with the parent.

**Consequences:** The exact suffix can be tested and resumed independently
without inventing gameplay meaning or a subsystem boundary. The native helper
is counted as `PORTABLE_INTERNAL_HELPER`; it is never called a third routine.
Full-SR and hardware behavior remain outside the helper and require a separate
milestone if they are ever considered.

**Affected files/milestone:** `src/core/parent_suffix.*`,
`src/tools/hybrid/candidate_parent_suffix.*`, `tests/parent_suffix_test.cpp`,
M11.57 report and hybrid accounting.

# ADR-0044 — Scoped temporal provenance sidecar for M12

**Status:** Accepted design contract; implementation stages not completed
**Date:** 2026-09-12

**Context:** Controlled BizHawk capture is working, but repeated manual
RAM/register/ROM dependency reconstruction duplicates work. Existing Carver
IntervalDB owns the ROM partition and non-owning evidence reports; its graph
does not encode instruction instances, memory versions or capture completeness.
Callback PC and lagged input observations already demonstrate false-proof risks.

**Decision:** Design a developer-only Python/SQLite sidecar that separates
immutable observations, temporal byte/register versions and scoped relations.
Use demand-driven slicing, explicit data/address/control dependencies,
coverage certificates and proof-obligation scheduling. Reuse the controlled
harness and M12 static analyzers. Export non-owning ROM evidence to Carver;
existing exact promoters alone may update ownership under their current gates.
V1 must recover the known canary with minimal RAM/register semantics, not
merely import an expected graph or log A372 followed by UNKNOWN.

**Consequences:** No monolithic rewrite, new production dependency, graph
server or general symbolic/emulator implementation. Repeats establish
reproducibility rather than independent semantic proof. Savestate roots are
scope boundaries, not invented reset provenance. Hardware visibility and
static completeness have separate gates. V0 capability validation precedes
V1 implementation; no milestone completion or ownership change is implied.

**Evidence/design:** `docs/reports/THOR_EVIDENCE_ENGINE_ARCHITECTURE.md` and
`docs/reports/THOR_EVIDENCE_ENGINE_STAGES.md`; verified baseline manifest
contains 1,475,368 SOURCE_OWNED bytes. ADR-0043 remains authoritative for the
M12 ASM → M13 rebuilt parity → M14 systematic C++ sequence.

# ADR-0045 — V0.1 raw completion and launch identity gate

**Status:** Accepted bounded repair; V1 remains gated
**Date:** 2026-09-12

**Context:** The V0 gate review reproduced sealing of a raw prefix ending at a
valid epoch, incorrect collector attribution, and capability conclusions that
survived missing evidence. A sealed transport must not be mistaken for a
complete execution or a causal proof.

**Decision:** Require a strict `thor.evidence.raw.v0.1` header/footer envelope
and a completed local launch receipt before normalization. The receipt binds
the actual Lua collector, normalizer, harness, emulator/core/config, watch-plan
order, mode/reverse flag, ROM/state and scenario. Report capabilities are
derived from runtime witnesses or hashed BizHawk API source receipts; absent or
contradictory evidence yields UNKNOWN/ERROR. Historical V0 captures remain
valid only as historical evidence when their execution identity cannot be
reconstructed.

**Consequences:** V0.1 can establish a truthful capture boundary and a local
FF13CC precondition matrix without implementing causal provenance. Generic bus
width, overlap, IRQ, same-value completeness and input causality remain explicit
frontiers. No ownership mutation, ROM discovery or production dependency is
introduced.

**Evidence:** `docs/reports/THOR_EVIDENCE_ENGINE_V0_1.md`, the V0.1 receipt,
raw-envelope and report tests, and the three repeated bounded captures under
ignored `build/thor-evidence/v0`.

# ADR-0046 — Local FF13CC V1-gate remains fail-closed

**Status:** Accepted bounded gate; V1 remains blocked
**Date:** 2026-09-13

**Context:** The accepted V0.1 capture proves ordered exact-hook observations
around one `A372` execution, but does not provide dense instruction coverage or
an interruption boundary. Treating an exact hook miss as absence of an
overlapping writer would create the causal false proof the V1 gate is intended
to prevent.

**Decision:** Add only a local gate certificate and adversarial validator. The
checked static `MOVE.L D2,(A5)+` form, selected ordered pairing and four-byte
temporal representation may be certified. Writer completeness requires an
explicit dense instruction list with `NO_MEMORY_WRITE`, `WRITE_DISJOINT`, or
`WRITE_TARGET_OVERLAP`; any `UNKNOWN_MEMORY_EFFECT`, omitted target coverage,
or unresolved interruption blocks the gate. One-byte overlap and same-value
writes remain writes. No provenance graph, general last-writer engine,
register propagation or ownership action is introduced.

**Consequences:** The local result is `PARTIAL / V1 BLOCKED`: pairing is
closed, while dense writer coverage and interruption remain explicit frontier
items. A future V1 authorization must supply those evidence classes before any
causal claim.

**Evidence:** `docs/reports/THOR_EVIDENCE_ENGINE_V1_GATE.md`,
`src/tools/thor_evidence/v1_gate.py`, and the persisted negative fixtures.

# ADR-0043 — Complete ASM reconstruction before systematic C++ migration
**Status:** Accepted for M12.0
**Date:** 2026-09-10

**Context:** The existing exact reassembly proof is a local-ROM-backed split
with 13,550 exact 68000 ASM bytes and 3,132,178 blob bytes. The prior roadmap
would have started Inventory/UI/Save as a portable C++ milestone before the
executable ROM was represented as complete assembler/source.

**Decision:** Rebase the project sequence to ROM -> complete reassemblable ASM
-> rebuilt-ROM runtime parity -> systematic ASM-to-portable-C++ migration.
Define ASM_CODE_COMPLETE, ASM_ROM_MAP_COMPLETE, ASM_REASSEMBLY_BYTE_EXACT and
ASM_REBUILT_ROM_BOOT_PROVEN as separate gates. Keep current native C++ proofs
preserved and set CPP_MIGRATION=PAUSED_PENDING_ASM_COMPLETION. Do not begin
M12.1 implementation in this decision.

**Consequences:** The former Inventory/UI/Save proposal is no longer the
immediate M12 milestone. Executable code cannot be hidden in local blobs;
unknown data and copyrighted payloads may remain local only under deterministic
tooling and explicit classification. M13 owns rebuilt-ROM boot/runtime parity
and M14 owns systematic C++ migration.

**Evidence:** docs/reports/ASM_COMPLETION_CENSUS_M12_0.md.
# ADR-0047 — Close only local dense FF13CC coverage before V1

**Status:** Accepted for V1-GATE-COVERAGE
**Date:** 2026-09-13

**Context:** The prior gate had a proven A372 pairing but exact-address hooks
could not establish that every instruction in the local interval was observed.
It also lacked a local interruption boundary. BizHawk 2.11.1 exposes
`event.on_bus_exec_any`, so a bounded dense capture can answer those two
questions without implementing provenance.

**Decision:** Use one bounded dense capture (with one corrected retry only for
capture serialization) from the proven `A370` pre-boundary through the `A374`
post-fetch boundary. Decode every captured execution instance with the existing
checked ROM decoder, compute concrete memory-write ranges from the pre-execution
register snapshot, retain all target-overlapping and same-value writes, and
fail closed on unknown effects or discontinuity. Treat `A374` as a boundary
witness, not as an unobserved post-boundary instruction.

**Consequences:** The selected interval has complete local execution coverage,
one concrete `FF13CC..FF13CF` writer and a continuous `A370 -> A372 -> A374`
control-flow boundary. This remains a local capability certificate. It does
not establish global IRQ behavior, causal input provenance, a last-writer
engine, a provenance graph, or V1 readiness by itself.

**Evidence:** `docs/reports/THOR_EVIDENCE_ENGINE_V1_GATE_COVERAGE.md`.
# ADR-0048 — First V1 provenance slice is engine-derived and canary-bounded
**Status:** Accepted for V1 FF13CC canary
**Date:** 2026-09-13

**Context:** V0 and the local V1 gate established sealed transport, checked
static forms, dense execution coverage and explicit capability frontiers. The
next authorized step is one causal query for the existing FF13CC canary.

**Decision:** Build provenance only from the checked ROM-bound static producer
and sealed dynamic witnesses. Represent temporal value versions, register
bit-slices, execution instances and dependencies with explicit VALUE, ADDRESS
and CONTROL roles. Persist the derived certificate in a separate SQLite
sidecar table. Reject unknown transforms, address-only edges, PC heuristics,
ROM low-byte substitution and cross-epoch merges. Keep access width, overlap,
same-value writer completeness, IRQ/exception and input-read capabilities
explicitly UNKNOWN.

**Consequences:** The engine can explain the first `FF13CC` value-version as
ROM high24 plus incremented `D5.low8`, and can be re-imported idempotently. The
certificate is not a general last-writer proof and does not authorize V2 or
source ownership.

**Evidence:** `docs/reports/THOR_EVIDENCE_ENGINE_V1_FF13CC_CANARY.md`.
# ADR-0049 — Reusable V2 RAM byte versions remain coverage-gated
**Status:** Accepted for V2 RAM provenance
**Date:** 2026-09-13

**Context:** V1 proved one FF13CC output through a bounded engine-derived
chain, but its RAM output versions were still local to the canary builder.

**Decision:** Generalize only RAM byte temporal versions and write operations.
Represent byte/word/long writes in big-endian physical order, preserve
previous versions and explicit pre-capture roots, isolate restore epochs, and
require a trace-bound complete coverage certificate for `PROVEN` last-writer
results. Return explicit frontiers for gaps, unsupported transforms and
conflicts. Extend the existing SQLite sidecar; do not create another store.
Reuse the primitive for the V1 canary and leave the held-out FF188A query at
`INCOMPLETE_CAPTURE` when existing coverage is insufficient.

**Consequences:** V2 can answer bounded RAM byte last-writer queries without
numeric-value or address-only identity collapse. Register/control provenance,
global coverage, IRQ/input causality, ROM RE and V3 remain separate gates.

**Evidence:** `docs/reports/THOR_EVIDENCE_ENGINE_V2_RAM_PROVENANCE.md`.

# ADR-0050 — V2.1 verified coverage and V1 RAM identity repair
**Status:** Accepted for V2.1 soundness repair
**Date:** 2026-09-13

**Context:** The independent V2 gate audit reproduced false promotion from
metadata-only coverage, future-version temporal queries, partial-write ghosts,
SQLite identity splicing and a V1 adapter path that discarded overlaps.

**Decision:** Keep the V2 byte engine and SQLite sidecar, but separate
unverified coverage claims from verified certificates. A certificate must be
content-bound to a contiguous capture basis, receipt, epoch, byte scope,
decoder/rule and execution instances. Keep immutable initial roots separate
from current state, validate complete write ranges before mutation, reject
inconsistent imported identities atomically, and expose the V1 target through
the concrete V2 byte versions and operation.

**Consequences:** The bounded FF13CC canary remains the only promoted query.
The historical FF188A evidence remains a negative frontier. Global coverage,
IRQ/input/DMA and V3 provenance remain outside this repair.

**Evidence:** `docs/reports/THOR_EVIDENCE_ENGINE_V2_1_SOUNDNESS_REPAIR.md`.

# ADR-0051 — V3 register/control provenance build skeleton
**Status:** Accepted for V3 BUILD
**Date:** 2026-09-13

**Context:** V2.1 re-audit left known soundness defects open, while the next
authorized milestone requires reusable register, execution and local control
structures before stabilization.

**Decision:** Add a developer-only V3 skeleton with temporal D/A register
slices, checked local M68K rules, explicit execution instances, bounded
call/return and local control facts, typed dependency roles, RAM interop,
canonical graph/export and explain APIs. Extend the existing SQLite sidecar;
do not create a second database. New generalized claims default to
OBSERVED/PROVISIONAL/UNKNOWN/CONFLICT and no V2 path is widened.

**Consequences:** V3 can represent the FF13CC and one existing execution
fixture, including partial register writes and address/control roles. The
engine is not a sound causal proof system. V2.1 defects remain in the known
defect ledger and V4 is prohibited until the V3 build gate is published.

**Evidence:** `docs/reports/THOR_EVIDENCE_ENGINE_V3_REGISTER_CONTROL_BUILD.md`.

# ADR-0052 — V4 ROM/resource/hardware cross-domain build bridge
**Status:** Accepted for V4 BUILD
**Date:** 2026-09-13

**Context:** V3 provides temporal register/control structure but has no shared
domain model for canonical ROM roots, resource transforms, DMA or video-memory
targets. V5 needs a graph boundary that can consume these identities without
turning bounded observations into ownership or causal proof.

**Decision:** Extend the existing developer-only engine with typed ROM,
constant, external, RAM/register and hardware roots; checked resource
transform contracts including the existing `0x3820`/Ancient interop; bounded
VRAM/CRAM/VSRAM/SAT and DMA nodes; role-preserving cross-domain edges; and
transactional SQLite persistence in the existing sidecar. New relations remain
OBSERVED/PROVISIONAL/UNKNOWN/CONFLICT by default and SOURCE_OWNED promotion is
forbidden.

**Consequences:** V4 can assemble ROM→resource→hardware and RAM/register→DMA
paths for later static/differential scheduling. It does not establish DMA
timing, same-frame VDP publication, complete aliases, IRQ causality or proof
system soundness.

**Evidence:** `docs/reports/THOR_EVIDENCE_ENGINE_V4_ROM_RESOURCE_HARDWARE.md`.

# ADR-0053 — V5 bounded static request and non-owning Carver bridge
**Status:** Accepted for V5 BUILD
**Date:** 2026-09-13

**Context:** V4 provides cross-domain runtime/resource identities, while the
Carver already has deterministic static analyzers and an ownership-preserving
IntervalDB. The next stage needs a machine-readable handoff without allowing
static hints to become SOURCE_OWNED bytes.

**Decision:** Add bounded trace/ROM/range-bound static requests, validated
responses with structure/domain/boundary certificates, runtime-seeded query
generation, typed static graph merge and Carver evidence export. Static output
is always evidence-only; requests and responses retain unresolved frontiers and
never carry automatic ownership or promotion transactions.

**Consequences:** V5 can route runtime seeds into existing static analyzers and
return deterministic Carver-compatible evidence while preserving the exact
manifest ownership metric. Indirect-CFG completeness, parser closure and
promotion remain separate gates.

**Evidence:** `docs/reports/THOR_EVIDENCE_ENGINE_V5_STATIC_CARVER_BRIDGE.md`.

# ADR-0054 — V6 isolated multi-scenario differential
**Status:** Accepted for V6 BUILD
**Date:** 2026-09-13

**Context:** V5 can route bounded static evidence but cannot compare controlled
neutral repeats with motivated alternate arms without risking cross-capture
temporal splicing.

**Decision:** Add scenario identities bound to environment, trace and arm;
retain an independent graph per scenario; classify exact normalized fact
repetition as observation-only; and reject graph edges whose endpoints leave
their scenario. Differential exports must contain no causal claims or automatic
ownership changes.

**Consequences:** V6 supplies deterministic COMMON and scenario-specific
manifests while preserving event provenance references and scenario isolation.
Repeated address/value observations remain non-causal until a later bounded
capability closes their writer and timing semantics.

**Evidence:** `docs/reports/THOR_EVIDENCE_ENGINE_V6_MULTI_SCENARIO_DIFFERENTIAL.md`.

# ADR-0055 — V7 bounded automatic frontier scheduling
**Status:** Accepted for V7 BUILD
**Date:** 2026-09-13

**Context:** V6 can compare isolated scenarios but has no deterministic way to
select the next unresolved evidence request while preventing unbounded tracing.

**Decision:** Add a frontier inventory ranked by information gain, confidence,
cost and risk; generate only bounded requests with explicit evidence classes;
and enforce a two-pass non-progress exhaustion policy. Whole-ROM trace requests
are rejected and scheduler output never promotes ownership.

**Consequences:** Later stages can drive bounded evidence acquisition and detect
fixed points without conflating scheduling with proof. The scores remain a
heuristic and must be evaluated against a real held-out frontier in V8.

**Evidence:** `docs/reports/THOR_EVIDENCE_ENGINE_V7_FRONTIER_SCHEDULER.md`.

# ADR-0056 — V8 held-out frontier evaluation
**Status:** Accepted for V8 BUILD
**Date:** 2026-09-13

**Context:** V7 scheduling needs a real unknown frontier evaluation to show that
the assembled machine can obtain bounded static and runtime structure without
silently resolving the capability.

**Decision:** Evaluate the existing canonical `0x03BDA6/0x03BDD8` frontier using
the existing static analyzer and a ROM-bound runtime receipt. Join results only
through an explicit UNKNOWN relation when the runtime observation does not close
the static termination condition; never promote ownership.

**Consequences:** V8 demonstrates useful machine-derived structure and preserves
the unresolved stream boundary. It does not authorize causal provenance,
same-value writer completeness or semantic labeling.

**Evidence:** `docs/reports/THOR_EVIDENCE_ENGINE_V8_HELD_OUT_FRONTIER.md`.

# ADR-0057 — V9 bounded operational M12 cycle
**Status:** Accepted for V9 BUILD
**Date:** 2026-09-13

**Context:** V4–V8 provide domain, static, differential, scheduling and
held-out-frontier components, but no single reproducible workflow persists and
exports their state.

**Decision:** Add a one-cycle orchestrator and small CLI that import a sealed
capture into the existing SQLite sidecar, seed and request bounded frontiers,
merge evidence through the non-owning static bridge, and emit deterministic
manifests including persistent-store and Carver state. Reject trace splicing and
keep ownership delta at zero.

**Consequences:** The assembled machine can run and audit one bounded M12 cycle.
Production UX, retries, causal soundness and stabilization remain post-V9 work.

**Evidence:** `docs/reports/THOR_EVIDENCE_ENGINE_V9_OPERATIONAL_INTEGRATION.md`.

# ADR-0058 — stabilization P0 evidence and persistence trust boundary
**Status:** Accepted for stabilization
**Date:** 2026-09-13

**Context:** The assembled V2/V3/V9 machine still allowed fabricated coverage
tags, semantically inconsistent RAM output rows and an under-validated V2
certificate branch to reach stronger trust states.

**Decision:** Require content-attested coverage events with checked execution
semantics; validate RAM output cardinality, producer/address/value/predecessor
relationships before SQLite insertion; and route V2 canary validation through a
dedicated identity and coverage validator. Missing or malformed evidence fails
closed and SOURCE_OWNED remains unchanged.

**Consequences:** P0 false-proof and persistence paths are bounded by explicit
regressions. The attestation is a lineage contract for validated capture input;
it does not authorize ownership promotion or resolve causal frontiers.

**Evidence:** Stabilization P0 regressions in
`tests/thor_evidence_v2_ram_test.py` and `tests/thor_evidence_v1_canary_test.py`.

# ADR-0059 — explicit V1/V2 target bridge during stabilization
**Status:** Accepted for stabilization
**Date:** 2026-09-13

**Context:** The canary result carried the legacy V1 target graph and concrete
V2 RAM byte outputs in separate namespaces. A validator that inspected only
the V2 branch could accept a detached legacy explanation.

**Decision:** Emit and validate one identity-checked `causal_bridge` that binds
the legacy FF13CC value-version, the concrete MOVE_LONG RAM operation, all four
byte-version outputs, and the shared write witness. Missing or detached bridge
data fails closed.

**Consequences:** The bounded canary has an explicit V1-to-V2 handoff. This
does not infer input causality, access-width capabilities outside the checked
operation, or whole-program provenance.

**Evidence:** `test_ram_target_requires_v1_v2_bridge` and the dedicated
`canary_validation.py` validator.

# ADR-0060 — completed epoch boundary is part of coverage attestation
**Status:** Accepted for stabilization
**Date:** 2026-09-13

**Context:** Event hashes and contiguous sequence numbers alone could still
describe a truncated prefix that had been relabeled as a complete coverage
interval.

**Decision:** A verified coverage basis must carry an attested `EPOCH_END` with
`data.reason == COMPLETE`; missing completion, NOTE substitution, unknown
effects, alias or overlap markers fail closed.

**Consequences:** The coverage contract distinguishes a sealed complete epoch
from a syntactically coherent fragment. The contract still does not claim
global writer completeness or causal input provenance.

**Evidence:** `test_coverage_rejects_note_substitution_and_unknown_effect`.

# ADR-0061 — RAM SQLite root/output semantic separation
**Status:** Accepted for stabilization
**Date:** 2026-09-13

**Context:** RAM persistence validated hashes and output references but did not
fully reject a root byte being repurposed as a write output.

**Decision:** Import requires root versions to have an allowed root origin and
no predecessor; operation-bound versions must be WRITE_OPERATION outputs at
the operation temporal sequence. Coverage intervals and addresses are also
shape-checked before insertion.

**Consequences:** SQLite cannot silently reinterpret initial state as a writer
or accept malformed coverage bounds. Transaction rollback remains the failure
boundary.

**Evidence:** `test_sqlite_rejects_root_as_write_output`.

# ADR-0062 — preserve explicit FF13CC capability frontier
**Status:** Accepted for stabilization
**Date:** 2026-09-13

**Context:** A `PROVEN` FF13CC target could lose its capability frontier if
validation ignored the explicit UNKNOWN list.

**Decision:** The bounded canary validator requires the five declared
capabilities (`access_width`, `overlap_range`, `same_value_writers`,
`irq_exception`, `input_reads`) to remain present and UNKNOWN, and rejects an
input-causal edge or PC heuristic.

**Consequences:** The canary can prove its checked byte write while retaining
the unclosed interrupt, input and broader memory frontiers.

**Evidence:** `test_ram_target_requires_v1_v2_bridge` and the RAM validator.

# ADR-0063 — held-out receipt type strictness
**Status:** Accepted for stabilization
**Date:** 2026-09-13

**Context:** The held-out runtime join coerced arbitrary truthy values with
`bool(...)`, so a malformed string such as `"false"` could become OBSERVED.

**Decision:** Runtime receipts require exact boolean reachability, nonnegative
frame/sequence integers, nonempty string addresses and backend/scenario fields.
Malformed receipts fail before the UNKNOWN join is evaluated.

**Consequences:** The 03BDD8 frontier remains UNKNOWN when the receipt is
incomplete or malformed; no static extent or ownership promotion is inferred.

**Evidence:** malformed-receipt cases in `tests/thor_evidence_v8_test.py`.

# ADR-0064 — duplicate capture imports are idempotent in the manifest
**Status:** Accepted for stabilization
**Date:** 2026-09-13

**Context:** SQLite import is idempotent, but the V9 operational manifest could
list the same trace more than once after a repeated capture call.

**Decision:** Keep one trace identity per operational cycle while allowing the
underlying transactional import to remain idempotent.

**Consequences:** Replays cannot look like independent captures in the
manifest. A different trace is still rejected as a splice.

**Evidence:** duplicate capture assertion in `tests/thor_evidence_v9_test.py`.
# ADR-0066 — Repository-driven multi-scenario campaign

Date: 2026-09-13

Status: Accepted for M12 developer-only evidence tooling.

AUTO66 reuses AUTO65's normalized chain, novelty, prefix and closure engine.
The new layer owns only scenario-pool fingerprinting, fixed-point transition,
equivalent-capture rejection, bounded discovery dispatch and per-scenario
accounting. Scenario files are the authority for deterministic inputs and
watch configuration; Luna does not provide an address or manually choose a
chain. A sealed report may be replayed to repair a persisted receipt, but that
operation is not counted as new runtime evidence. This preserves the
ROM-loader/evidence/contract boundary and keeps knowledge non-owning.

The first accepted campaign stops after the repository's two useful scenarios:
the AUTO65 capture is the fixed-point starting point, and the scheduler selects
the distinct idle scenario, which adds one bounded ROM-activity branch. A new
savestate or gameplay state is required before further scenario coverage.

# ADR-0067 — Live opportunistic RE machine and snapshot operator view

Date: 2026-09-13

Status: Accepted for M12 developer-only evidence tooling.

AUTO67 replaces scenario-driven runtime dispatch with current human-gameplay
sampling. The emulator publishes a fixed rolling window of minimal events;
unclaimed observations are overwritten rather than placed in a persistent raw
event queue. One Dispatcher atomically performs preliminary identity, known
coverage and active-claim checks, then leases work through one mailbox per
worker. Investigations and worker leases are separate, so WAITING_RUNTIME,
KNOWN, MERGED, PROVEN, BLOCKED and EXHAUSTED work releases its worker.

The operator view is served by the same launcher and reads bounded status
snapshots only. UI publication is outside callbacks and does not hold the
Dispatcher claim lock while workers run. Live observation remains non-owning;
M12 promotion contracts and canonical byte-exact reconstruction remain
authoritative.

Evidence: `tests/thor_evidence_auto67_test.py` and
`docs/reports/THOR_M12_AUTO67_LIVE_OPPORTUNISTIC_RE.md`.

2026-09-13 operator-test correction: the original implementation serialized
all investigations and its dashboard displayed static transition examples.
The performance repair limits UI publication to recent rows and real bounded
worker history, with file/HTTP work in a separate publisher. Burst capture is
opt-in because lost writes can break causal chains; continuous capture already
samples every 16th write and is not a complete trace either. No task dispatch,
observed-PC identity or worker return counter constitutes a causal proof.
AUTO67's current worker path records unresolved observations rather than
executing the full provenance engine. Completion of that integration is a
separate outstanding task; this repair does not silently change the RE model.

## AUTO67.1 — Fixed frozen capsule experiment

Date: 2026-09-13

AUTO67.1 keeps the existing rolling-window and claim authority but adds a
developer-only pool of exactly sixteen reusable 128 KiB capsules. At most four
capsules can be live targeted captures at once. A capsule freezes before
analysis, is released on every result including KNOWN/MERGED and unresolved
outcomes, and returns to FREE for reuse. BizHawk receives bounded targeted
hooks with a per-frame callback budget; discovery remains a replaceable ring,
not a raw-event FIFO or per-event UI stream. Capsule records are immutable
after freeze and are not part of dashboard payloads.

This is an explicit lossy experiment: a 128 KiB capsule and callback budget
cannot establish causal completeness or chain closure. The existing AUTO67
dashboard is extended with capsule status and worker history rather than a
second UI. No production runtime, SOURCE_OWNED bytes, or AUTO68 work is
introduced.

AUTO67.1 operator correction: the default view is a separate native
`auto67_window.py` child process consuming the same replaceable status snapshot.
It is kept outside callbacks, Dispatcher claims, workers and knowledge
persistence. The prior HTTP dashboard remains available only when explicitly
requested with `--view-mode browser`; this keeps the operator UI choice from
changing the live RE data path.

## AUTO67.2 — Continuous hunt with scarce focused capture

The live worker pool must keep hunting current bounded rolling-window evidence
while focused BizHawk capture slots are occupied. A capsule is optional for a
lease: when the configured focused-capture limit is reached, the worker takes
the fresh candidate through quick check and analysis instead of waiting behind
the hook. The default focused-capture count is one; higher counts remain an
explicit measurement setting. Sampling remains lossy and never proves absence;
no raw-event backlog is permitted. The decision is based on the AUTO67.2 real
run: 16 workers reached peak busy 16, recorded 169 hunt-success assignments
after returns, and produced no frame over 33 ms with one focused slot.

## AUTO67.3 — Persistent chain store first through the existing sidecar

Date: 2026-09-13

Status: Accepted for M12 developer-only evidence tooling.

The AUTO67.2 in-memory yield ledger was not durable evidence: its generic
observation/context/edge counters could increase once per lease, and the live
worker completion path incremented `new_edges` unconditionally. It is disabled
from the acceptance path. AUTO67.3 keeps the runtime, capsule and dispatcher
architecture unchanged and establishes persistent completed-chain storage
before any DB-growth or semantic-yield analysis.

Completed workers hand a compact descriptor to one bounded nonblocking queue.
Only its background writer opens the existing `Store` connection and extends
the same SQLite sidecar with live session/context/observation/obligation rows.
The AUTO67.3 acceptance path is now chain-store-first. The former
knowledge-yield analyzer and seed/context durable classifications are disabled;
before worker execution the Dispatcher rejects only an active claim (plus its
normal already-dispatched window token). A completed worker result becomes an
immutable canonical chain record. Its SHA-256 hash excludes frame, epoch,
worker, lease and session identifiers, while those values remain provenance.
Exact hashes update bounded observation metadata in `live_chain`; the chain body
is never duplicated and similar chains are not semantically merged. Unresolved
chains are valid records. The native window reports objective store growth only:
unique chains, session new, exact duplicates, writes/errors, unresolved and
rooted counts. No raw-event backlog, second database, dispatcher SQLite call,
or SOURCE_OWNED promotion is introduced.
# ADR-0067.4 — bounded frozen capsule materialization
**Status:** Accepted for AUTO67.4
**Date:** 2026-09-14

**Context:** AUTO67.3 proved that the focused Lua capsule contained richer
runtime records than the seed-only worker descriptor, but the authoritative
worker path did not read the frozen body.

**Decision:** Version the existing capsule file as O67V v2, retain a bounded
O67C decoder for historical files, validate capsule/lease identity in the
worker, and materialize only observed runtime records into separate evidence
fields. Persist new descriptors as `MATERIALIZED_CHAIN`; preserve historical
rows as `SEED_ONLY`. The unresolved frontier remains explicit.

**Consequences:** The existing dispatcher, hot workers, bounded queue and
SQLite sidecar remain in place. No semantic merging, ownership promotion,
SOURCE_OWNED change, raw-event backlog or AUTO68 work is introduced.

**Evidence:** `docs/reports/THOR_M12_AUTO67_4_FROZEN_CAPSULE_REAL_WORKER_CHAIN.md`.

## ADR-0067.5 — Separate observed runtime facts from causal dependencies

**Status:** Accepted for AUTO67.5

**Context:** AUTO67.4 placed additional BUS_WRITE_PC capsule observations in a
field named `causal_facts`. Those records establish that a PC/address pair was
observed, but do not establish a producer, register, RAM-version or control
dependency.

**Decision:** Version new materialized payloads as schema 2. Store capsule
observations in `runtime_observations` and direct witnessed pairs in
`observed_facts`. Emit `causal_facts` only from generic, fail-closed static
M68K instruction semantics. Do not create `chain_steps` without an ordered
dependency proof; leave register/RAM provenance in `unresolved_frontier`.
Historical AUTO67.4 databases remain read-only and interpretable.

**Consequences:** The 0x0027EC canary proves a MOVE memory-write source and
destination-address dependency from the ROM opcode, but not the A4/A5 register
origins. The unrelated 0x06009A seed remains unresolved because its opcode is
not a supported memory-writing MOVE. No semantic merging, SOURCE_OWNED change,
ownership promotion or AUTO68 work is introduced.

**Evidence:** `docs/reports/THOR_M12_AUTO67_5_CAUSAL_INTEGRITY_GATE.md`.

## ADR-0067.6R — Targeted register predecessor evidence

**Status:** Accepted for AUTO67.6R

**Context:** AUTO67.5 proved generic instruction source and destination
semantics but intentionally left A4/A5 register provenance unresolved. The
existing focused BizHawk hooks already provide the minimum mechanism for a
bounded predecessor slice.

**Decision:** Add a versioned O67P v1 sidecar only for investigations whose
static facts require A4/A5. Reuse `event.on_bus_exec_any` and targeted
`emu.getregister` reads, retain only a bounded predecessor ring, and resolve a
register only when epoch, sequence, consumer occurrence, interval completeness
and static producer semantics all agree. Reject overwrite, gap, truncation,
unsupported decode and identity mismatch. Do not migrate historical AUTO67.4
data, merge graphs, change SOURCE_OWNED or begin AUTO68.

**Consequences:** The worker can emit the first register reaching-definition
step when a complete real interval exists. The AUTO67.6R real run currently
fails closed because the canary predecessor interval is incomplete; no producer
PC is inferred. The bounded negative result is authoritative until a later
task changes capture scheduling explicitly.

**Evidence:** `docs/reports/THOR_M12_AUTO67_6R_TARGETED_REGISTER_PREDECESSOR.md`.
# ADR-0067.6R2 — one global bounded prehistory ring
**Status:** Accepted as AUTO67.6R2 checkpoint; performance follow-up required
**Date:** 2026-09-14

**Context:** AUTO67.6 targeted predecessor hooks started too late to recover
the history before a BUS_WRITE consumer. A per-investigation execution-hook
fan-out was already known to damage BizHawk responsiveness.

**Decision:** Use one continuously active developer-only global
`event.on_bus_exec_any` hook with a bounded 4096-record compact ring. At an
exact discovery/execution join, freeze one bounded O67P v2 slice and read only
the requested A4/A5 values at the consumer. Keep discovery sequence and
execution sequence separate. Because BizHawk's second callback argument is a
bus value, resolve v2 instruction semantics from canonical ROM bytes at the
captured runtime PC; do not treat that callback value as an opcode.

**Consequences:** AUTO67.6R2 can prove the first real generic reaching-
definition steps without a per-worker hook or raw-event backlog. The canary
proves A4/A5 reaching definitions. The real run also measures a hard negative:
the global Lua callback produces 120/120 frames over 50 ms and a 679 ms maximum,
so increasing ring capacity is not an optimization and smooth gameplay is not
accepted by this checkpoint. No semantic merge, SOURCE_OWNED change or AUTO68
work is implied.

**Evidence:** `docs/reports/THOR_M12_AUTO67_6R2_CONTINUOUS_PREHISTORY.md` and
its machine-readable JSON proof.

## ADR-0067.6R3 — existing low-overhead source audit
**Status:** Negative checkpoint; no source accepted
**Date:** 2026-09-14

**Context:** AUTO67.6R2 proves the A4/A5 reaching-definition contract through a
bounded global Lua execution ring, but its real QuickSave1 run is not
gameplay-safe. Before adding capture machinery, existing BizHawk TraceLogger,
native GPGX hooks, hybrid block translation and helper/plugin paths were
audited.

**Decision:** Do not increase the ring and do not add a new source in R3. The
only connected complete source is `event.on_bus_exec_any`, and it fails the
performance gate. BizHawk TraceLogger exposes no machine-readable bounded
stream. The existing GPGX `HOOK_CPU`/block hook is a separate developer-only
libretro path without BizHawk QuickSave1 integration; existing hybrid blocks
are fixed translation proofs, not a generic ordered runtime history. A native
BizHawk helper/plugin would be new architecture and is outside this checkpoint.

**Consequences:** R2 semantic correctness remains accepted, while normal
gameplay performance remains blocked. No AUTO68, semantic merging,
`SOURCE_OWNED`, chain depth, database schema, or runtime architecture changes
are introduced. The next source must attach to the authoritative BizHawk path
and preserve exact block/order/branch/interrupt fail-closed evidence.

**Evidence:** `docs/reports/THOR_M12_AUTO67_6R3_LOW_OVERHEAD_PREHISTORY.md`.

## ADR-0067.6R3B — targeted producer-triggered execution burst
**Status:** Accepted as a positive bounded feasibility checkpoint
**Date:** 2026-09-14

**Context:** AUTO67.6R2 proved the reaching-definition contract with a global
execution ring, but its always-on Lua callback was not gameplay-safe. The
targeted-burst hypothesis keeps the global hook absent during idle periods and
opens it only around the two known static producer PCs.

**Decision:** Add a developer-only bounded burst source with two targeted
producer hooks, one active global hook slot, a 64-record maximum, exact stop at
consumer `0x0027EC`, and fail-closed budget exhaustion. Retain at most 16
flushed O67P v2 slices. Do not increase the continuous ring, merge graphs,
change SOURCE_OWNED, redesign workers, or begin AUTO68. Do not expose a path
for a bounded slice unless the sidecar was actually flushed.

**Consequences:** The real QuickSave1 gate proved both generic reaching
definitions for the canary while reducing frame maximum from the R2 679 ms
failure to 23 ms, with zero frames over 50 ms. The bounded proof retains only
16 sidecars; exact joins beyond that retention are not advertised as persisted
paths. The targeted source is accepted for this feasibility checkpoint and is
not a claim of full-ROM capture coverage.

**Evidence:** `docs/reports/THOR_M12_AUTO67_6R3B_TARGETED_BURST_FEASIBILITY.md`
and its machine-readable JSON proof.

## ADR-0067.6R3C — generic static register-writer targeting
**Status:** Negative checkpoint; generic full-ROM targeting rejected by the
performance gate
**Date:** 2026-09-14

**Context:** R3B proved the bounded burst semantics around two producer PCs,
but its runtime target list was not yet generated from the full existing
static writer set. R3C required removing those runtime literals and measuring
the generic static candidate set before accepting a broader hook installation.

**Decision:** Reuse `register_writes()` to enumerate statically proven A4/A5
writer candidates and generate the bounded runtime target file. Keep the Lua
burst source fail-closed and producer-agnostic. Stop the generic acceptance
gate after a real 500-hook prefix reached 63 ms maximum frame time and one
frame over 50 ms. Do not install the remaining 30,042 hooks, redesign the
capture path, increase SOURCE_OWNED, merge graphs, or begin AUTO68.

**Consequences:** The generic configuration path is proven to be generated and
contains both canary writer PCs without Lua hard-coding. Generic gameplay
acceptance is negative at the measured bounded scale. The earlier R3B semantic
proof remains valid and is not replaced by an unsafe full-ROM run.

**Evidence:** `docs/reports/THOR_M12_AUTO67_6R3C_GENERIC_REGISTER_WRITER_TARGETING.md`
and its machine-readable JSON.

## ADR-MAP-1 — Persistent global provenance cartographer

**Decision:** Add a small generic SQLite graph beside (not inside) the historical
AUTO67 persistence schema. Import only already-proven machine-readable evidence;
canonicalize static ROM entities, retain runtime occurrence/value-version scope,
and keep OBSERVED/PROVEN/UNRESOLVED/CONFLICT distinct.

**Consequences:** CPU/register, SAT/DMA, graphics/resource, and selector/control
evidence can share one deterministic persistent graph with lineage and frontiers.
Legacy proof databases remain readable and untouched. No semantic merging,
runtime capture, or SOURCE_OWNED promotion is introduced.

**Evidence:** `docs/reports/THOR_M12_MAP_1_GLOBAL_PROVENANCE_CARTOGRAPHER.md` and
`tests/thor_evidence_map1_test.py`.

## ADR-WALKER-1 — Bounded two-window sparse advancement

**Decision:** Accept ASM Walker only as a bounded developer-only transport for
validated blocks. The first experiment uses boundary guards around one
straight-line body, disables `event.on_bus_exec_any` while that body executes,
and re-enables it for one bounded successor window. Cartographer receives
stable static identities; run, restore, capture, and fragment identities remain
in the runtime evidence and are required by join validation.

**Consequences:** The QuickSave1 checkpoint proved one real off-body interval,
one non-oracle W2 dependency, positive first merge, zero-growth identical
replay, and fail-closed rejection of five false joins. This does not establish
ROM-wide scaling or exact data-writer attribution. Any future block must supply
its own static contract and remain bounded; a missing contract is a frontier.

**Evidence:** `docs/reports/THOR_M12_WALKER_1_TWO_WINDOW_SPARSE_ADVANCEMENT.md`,
its JSON proof, and `tests/thor_evidence_walker1_test.py`.

## ADR-DISPATCHER-1 — Occurrence-only AUTO67 leasing

**Decision:** Remove branch-level active suppression from the AUTO67
Dispatcher. A `branch_fingerprint` remains diagnostic metadata, while
`dispatch_state=LEASED` on the exact current `RollingWindow` item is the only
event-level redispatch guard. Free workers may therefore lease simultaneous
runtime occurrences with identical kind/address/PC.

**Identity:** `live_opportunistic.lua` supplies monotonic `epoch + seq` as the
runtime occurrence identity. The Dispatcher adds a bounded `window_item_id`
for the exact stored window item; investigation and lease identifiers include
both identities. This is runtime occurrence identity only and does not assert
causal sameness or semantic novelty.

**Consequences:** The existing one-mailbox-per-worker model and capsule limits
(`16` total, `4` live) remain unchanged. Capture-slot exhaustion still uses
`WAITING_CAPTURE_SLOT`; no raw-event backlog or per-worker queue is added.
Compatibility metrics for active collisions, duplicate active claims, and
pre-dispatch merges remain present but obsolete and are no longer incremented.

**Evidence:** `docs/reports/THOR_M12_DISPATCHER_1_OCCURRENCE_ONLY.md`, its JSON
counterpart, and `tests/thor_evidence_dispatcher1_test.py`.
# ADR-AUTO67-WORKER-CLEAN-1 — Keep Worker factual and bounded
**Status:** Accepted for M12 AUTO67
**Date:** 2026-09-15

**Context:** AUTO67.occurrence-only leasing already made the Dispatcher the
single claim authority, but the Worker still retained an unbounded investigation
map and assigned semantic novelty/status during processing. That mixed evidence
collection with downstream interpretation and allowed live materialization to
emit prescriptive unresolved frontiers.

**Decision:** Keep only the bounded 16-entry `recent_investigations` deque.
Worker outcomes are factual capture/decode/materialization observations;
`known_during_work` and Worker global KNOWN/PROVEN/DUPLICATE/MERGED decisions
are removed. Capsules are temporary resource containers released by id and no
longer carry semantic flags. Live materialization retains observed and proven
causal facts plus bounded register/capture diagnostics, while omitting
`unresolved_frontier` and `next`. Persistence remains an optional downstream
sink and occurrence provenance is preserved in descriptors.

**Consequences:** Worker completion and capsule release are independent of
persistence availability. Existing Dispatcher occurrence identity, exact-item
LEASED guard, rolling window, mailbox shape, and capsule limits remain unchanged.
Semantic deduplication and any future frontier scheduling stay downstream.

**Evidence:** `docs/reports/THOR_M12_AUTO67_WORKER_CLEAN_1.md`.
# ADR-AUTO67-RING-CLEAN-1 — Current-event rings have no semantic state
**Status:** Accepted for M12 AUTO67
**Date:** 2026-09-15

**Context:** After Worker cleanup, the Python `RollingWindow` still exposed a
`retained` counter and a misleading `max_utilization` snapshot field. The
current-event ring must remain a temporary bounded scheduling window, separate
from the Lua transport ring and from the predecessor ring.

**Decision:** Remove `RollingWindow.retained`, `rolling_window.retained`,
`max_utilization`, and the unused `current()` method. Keep only capacity,
items and overwrite count, while preserving exact-item `dispatch_state=LEASED`
and all occurrence identity fields. Leave the Lua ring implementation intact
because its bounded overwrite-oldest/order/identity contract already passes the
focused fixture audit.

**Consequences:** The Python ring cannot be mistaken for proof retention or a
backlog. Lua remains the transport window and Python remains the Dispatcher
current-event window; neither ring is merged or enlarged. Dispatcher, Worker,
CapsulePool, Cartographer, MAP-1, Walker-1, AUTO68, SOURCE_OWNED and C++ are
unchanged.

**Evidence:** `docs/reports/THOR_M12_AUTO67_RING_CLEAN_1.md`.

# ADR-AUTO67-PREDISPATCH-TRANSPORT-CLEAN-1 — Identity cursor at the status boundary
**Status:** Accepted for M12 AUTO67
**Date:** 2026-09-15

**Context:** The launcher previously tracked only one process-local `seq` while
reading replaceable Lua snapshots. That left epoch rollover implicit and made
the pre-dispatch transport contract depend on ad-hoc runner code.

**Decision:** Put snapshot consumption in `PreDispatchTransport`. Keep one
identity cursor `(epoch, seq, occurrence_id)`, accept only unseen monotonic
records, and drop duplicate, stale, or malformed records. Support `events` and
legacy `discovery` as input aliases. Return records immediately to Dispatcher;
do not retain raw events, perform semantic lookup, or claim work in transport.

**Consequences:** Epoch changes cannot be lost when sequence numbers restart,
and replaceable snapshot loss remains intentional and bounded. Dispatcher,
Lua rings, predecessor capture, Worker, CapsulePool, Cartographer, MAP-1,
SOURCE_OWNED and production runtime behavior remain unchanged.

**Evidence:** `docs/reports/THOR_M12_AUTO67_PREDISPATCH_TRANSPORT_CLEAN_1.md`.

# ADR-M14.2C — Fused evidence projects as canonical object references
**Status:** Accepted for M14.2C
**Date:** 2026-09-25

**Context:** The persisted M14.2B graph contains exact independent gameplay
and Sprite/SAT evidence joined on an existing canonical ROM instruction
object. The path proves a runtime DMA-emitter reference but supplies no new
ROM data boundary or class.

**Decision:** Store the projection as a `GLOBAL_EVIDENCE_REFERENCE` claim and
its source refs in the existing canonical knowledge SQLite child generation.
Bind each proposal to its generation, map hash, emission hash, graph hash,
exact relations, fusion derivation, object/range, and emission owner. Do not
change range partition, classifications, emission, or SOURCE_OWNED. Hypothesis
and unresolved evidence cannot create canonical claims.

**Consequences:** Global object queries show the graph's source evidence and
the canonical map owner together. M14.2C records multi-source map progress
without inventing resource semantics or ownership. A future boundary or class
promotion still needs its own exact proof and normal promotion path.

## ADR-M14.2B — Reuse canonical SQLite for cross-source evidence fusion
**Status:** Accepted for M14.2B
**Date:** 2026-09-24

**Context:** Accepted gameplay RAM/SAT, Sprite/SAT, VDP/DMA, and normalized FLOW
artifacts describe different views of the same execution, but the canonical
ROM map already owns stable ROM object identities, evidence references, typed
relations, derivations, conflicts, and map proposals.

**Decision:** Add thin adapters and global queries over the existing canonical
`knowledge.sqlite`. Resolve each ROM PC to an existing canonical object;
connect exact RAM-shadow→DMA and DMA→hardware-SAT relations through their
shared DMA-emitter object; preserve RAM/VRAM addresses as typed relation
attributes; and retain source artifact hashes, analyzer/version, capture, and
original truth in the existing evidence tables. Keep map proposals dry-run
only and require emission and SOURCE_OWNED invariance.

**Consequences:** Multiple analyzers and captures can attach evidence to one
static ROM object, while normalized FLOW occurrences remain M14.2A-scoped.
The persisted graph and `WHY` query can expose a path that no one subsystem
report contains alone. The historical selector→ROM-table→renderer chain stays
deferred where current accepted artifacts lack exact rows. No new database,
map application, promotion, or ownership change is introduced.

**Evidence:** M14.2B-R acceptance receipt under ignored
`build/thor-evidence/m14-2b-final-forward/` and its order-independent reverse
rebuild.

# ADR-M12-MAP-DRIVEN-EXECUTED-ASM-CLOSURE-2F — Audited static ownership promotion
**Status:** Accepted for the developer-only 2F checkpoint
**Date:** 2026-09-19

**Context:** The canonical 2G knowledge map contains runtime-executed M68K
instruction objects whose ROM bytes remain unowned. Runtime execution proves
that an instruction ran, but does not by itself prove an ASM ownership range,
its boundaries, or closed control flow.

**Decision:** Derive candidate islands only from exact mapped instruction
ranges and observed adjacent `EXECUTED_NEXT` relations. Promote a range only
after the bounded decoder exactly matches every mapped instruction, all
control-flow exits are closed by the interval or existing verified code, the
generated 68000 ASM round-trips byte-exactly in vasm, and an independent audit
reconstructs both the ownership delta and full canonical ROM. Candidate-to-
candidate control-flow dependencies must also close; unresolved candidates
remain blockers. Canonical map refresh uses the existing 2D importer and 2G
Archivist pipeline, with `SOURCE_OWNED` treated as an explicit expected base.

**Consequences:** 2F promoted only `[0x002AA4,0x002ACE)` (42 bytes, 14
instructions) and left 24 of 25 candidate islands blocked by unresolved
control-flow. New ownership is `STATIC_VERIFIED`; existing runtime claims and
relations remain unchanged. No CPU hooks, runtime campaigns, Worker/FLOW,
AUTO67, predecessor logic, or scaling semantics are changed.

**Evidence:** `docs/reports/THOR_M12_MAP_DRIVEN_EXECUTED_ASM_CLOSURE_2F.md` and
its compact JSON receipt.
# ADR-M12-POSTRUN-PROGRESS-HEARTBEAT-2I1 — Truthful post-run completion
**Status:** Accepted for M12 2I.1
**Date:** 2026-09-19

**Context:** The compact 2I analyzer wrote only a stage name and the post-run
window treated every `PASS...` report as full completion. Long SQLite/FLOW
work therefore looked frozen and could display `ANALYSIS COMPLETE` while
later pipeline stages had not run.

**Decision:** Publish an atomically replaced JSON snapshot at a bounded 4 Hz
heartbeat. Each of the nine existing stages has an explicit state and factual
N/M counters when a denominator exists; unknown totals use indeterminate UI
animation. The UI derives heartbeat age and progress age independently,
keeps raw JSON behind DETAILS, and reports compact-master-only output as
`PARTIAL ANALYSIS COMPLETE`. Backend failure is terminal and does not kill or
depend on the UI process.

**Consequences:** A reopened window can restore the latest stage and counters
from disk, closing the window does not stop the analyzer, and a stale backend
is visible instead of an endless spinner. FLOW_V1, Worker lifecycle/scaling,
END GAME boundary behavior, rolling-master semantics, canonical truth, 2E/2F
promotion rules, and SOURCE_OWNED are unchanged.

**Evidence:** `tests/live_forward_postrun_progress_test.py` and
`docs/reports/THOR_M12_END_GAME_ROLLING_MASTER_2I.md`.

# ADR-M12-END-GAME-ROLLING-MASTER-2I — Graceful close and compact post-run master
**Status:** Accepted for the developer-only 2I compact orchestration pass
**Date:** 2026-09-19

**Decision:** The separate Worker Control window writes one atomic `END_GAME`
sentinel. Lua consumes it only between complete Worker rounds and exits through
the supported EmuHawk client path. After the sealed receipt, a background
post-run process validates the raw/index hashes and writes a compact SQLite
rolling generation containing deduplicated instruction-PC facts, control edges,
terminal facts, occurrence counts and run provenance. A hash-bound `current.json`
pointer is replaced only after SQLite integrity validation.

The compact pass keeps raw FLOW and the segment index until the canonical
Cartographer/Archivist/decoder refresh is independently available. It does not
change FLOW_V1, Worker lifecycle/scaling semantics, SOURCE_OWNED or production
AUTO67.

**Evidence:** `docs/reports/THOR_M12_END_GAME_ROLLING_MASTER_2I.md` and the
ignored rolling generation under `build/thor-evidence/`.

# ADR-M12-CANONICAL-ROM-KNOWLEDGE-MAP-2D — Separate ranges, facts and emission
**Status:** Accepted for the 2D checkpoint
**Date:** 2026-09-18

**Context:** Accepted M12 reconstruction classifications, exact emission
intervals, Carver hypotheses and 2B runtime linkage existed in separate
artifacts. AUTO60/Carver reported 1,427,873 `SOURCE_OWNED` bytes while the
accepted current manifest reported 1,475,600 bytes; runtime occurrence
lineage also must remain out of canonical object identity.

**Decision:** Build a deterministic, ROM-SHA-scoped SQLite knowledge layer
with separate emission intervals, canonical ROM ranges/objects, claims,
relations and evidence references. Reconcile AUTO60 to AUTO61 through the
accepted GFX-2/GFX-MAX/AUTO61 manifests before importing current ownership.
Keep Carver findings as `HYPOTHESIS`; import each of the 330 exact 2B
instruction ranges once and reference its occurrence lineage through saved
artifact hashes. An `EXECUTED_NEXT` relation requires captured M68K instruction
objects at both endpoints; exception-event edges remain in source evidence but
are not miscast as instruction adjacency. Preserve terminal `next_pc` facts
as address-only `OBSERVED_NEXT_PC`. No runtime campaign, hook, ownership
promotion, or Worker/FLOW/AUTO67 change is part of this checkpoint.

**Consequences:** The map covers all 3,145,728 ROM bytes with a disjoint exact
emission partition, while UNKNOWN remains explicit. Stable object identity
depends on ROM hash, byte bounds and type, not run/Worker/capture identity.
Raw session/FLOW data and the working SQLite database remain local; the
committed receipt provides deterministic structure, evidence references,
metrics and hashes for independent review. This checkpoint does not complete
the broader M12 reconstruction milestone or begin M13.

**Evidence:** `docs/reports/THOR_M12_CANONICAL_ROM_KNOWLEDGE_MAP_2D.md`, its
JSON receipt and `tests/rom_knowledge_map_test.py`.

# ADR-M12-AUTO67-LIVE-FORWARD-WORKER-1A — One shared stream, one live Worker
**Status:** Accepted for the bounded developer-only 1A prototype
**Date:** 2026-09-17

**Context:** The existing AUTO67 native snapshot freezes an earlier execution
window and its Worker input can lack a later native instruction stream. The
first live-forward proof must start at the CPU's current instruction boundary
without waiting for predecessor resolution or adding callbacks to each
instruction.

**Decision:** In the isolated BizHawk 2.11.1/GPGX developer build, record the
main M68K stream once into a bounded native ring. A single Worker attaches at a
future instruction boundary, stores full ENTRY/EXIT CPU state, counts observed
control-flow transitions, and seals an exact copied range on depth, memory,
retention, CPU-stop, or capture-error conditions. The result is immutable and
is validated outside the instruction hot path before it is called
`READY_FOR_CARTOGRAPHER`.

**Consequences:** This proves a local factual FLOW_V1 segment only. The separate
Sega CD sub-CPU remains unsupported; predecessor resolution, Cartographer,
production AUTO67, `SOURCE_OWNED`, and the project roadmap do not change. A
multi-Worker or Cartographer-adapter design needs a later explicit checkpoint;
1A stops here.

**Evidence:** `docs/reports/THOR_M12_AUTO67_LIVE_FORWARD_WORKER_1A.md` and its
JSON receipt.

# ADR-M12-AUTO67-LIVE-FORWARD-WORKER-1B — Bounded multi-Worker scaling
**Status:** Accepted for the developer-only 1B scaling checkpoint
**Date:** 2026-09-17

**Context:** Worker 1A proves one capture descriptor against a shared M68K
stream. Capacity measurements require several different overlapping windows,
while keeping the CPU-side instruction recorder single-copy and the configured
result budget bounded.

**Decision:** Replace the fixed 64-entry singleton lifecycle with a dynamically
allocated, preflighted descriptor/result pool and ordered pending/active queues.
At most one pending descriptor attaches at each confirmed instruction
boundary. Instructions continue to be recorded once in the same shared ring;
each result is copied only into its own bounded slot at completion. Use the
same 64-KiB result budget in both phases and vary depth only for forced
concurrency. Reject a request before allocation when the checked native,
Waterbox, host-transport or process memory budget does not fit. The experiment
is scoped to the explicitly requested counts through 100,000; that scope ceiling
is not reported as a measured capacity limit.

**Consequences:** Phase A reports natural `CAPTURING`, completed and occupied
peaks independently. Phase B reports forced simultaneous capacity separately.
Every tested `WORKER_COUNT` must complete 100 serialized lifecycle cycles on
each Worker. Each segment requires a fresh capture ID and generation, a live
execution epoch, COMPLETE and ANALYZING states, full host validation of the
binary record segment, an exact ACK, and a verified return to FREE before the
next cycle. The gate is `WORKER_COUNT * 100` completed and validated segments;
a pool startup or one successful capture is never a PASS. Depth/memory
terminations remain visible bounded results; a slot is never reused before its
exact ACK. Per-slot native counters prove capture start, completion, first
ANALYZING transition and release across every generation. The host exports each
segment twice while CPU execution advances and compares metadata, ENTRY/EXIT
state and every binary record before accepting it. The single shared M68K stream
remains the only per-instruction trace.
Production AUTO67, predecessor handling, Cartographer, `SOURCE_OWNED` and the
project roadmap remain unchanged. Stop after recording the first factual
natural saturation or forced/resource/correctness limit.

**Evidence:** `docs/reports/THOR_M12_AUTO67_LIVE_FORWARD_WORKER_1B_SCALING.md`
and its JSON receipt.

# ADR-M12-AUTO67-LIVE-FORWARD-CARTOGRAPHER-2A — RAM execution map and Archivist
**Status:** Accepted for the developer-only 2A checkpoint
**Date:** 2026-09-17

**Context:** Worker 1B proves immutable FLOW_V1 execution windows and lifecycle
identity, but does not persist observed execution structure. MAP-1 already owns
stable node/edge identity, lineage union and proof-status ordering.

**Decision:** Add a separate live-forward adapter after the 1B host audit and
before its exact ACK. It accepts only validated FLOW_V1 record sequences,
creates ROM-scoped instruction/event nodes and `OBSERVED` `EXECUTED_NEXT` edges,
and records each execution's run/epoch/Worker/capture/generation/hash/bounds
lineage. The RAM Cartographer stages per-segment lineage in its in-memory SQLite
and folds it into MAP-1 rows once after emulator exit, before graph hashing and
backup; no full-graph hash or proof-component scan runs before each ACK. Each
runtime starts with a fresh RAM Cartographer and never reads the master. After
EmuHawk exits, SQLite backup saves a retained session; a separate
Archivist validates MAP-1/session identity and uses the existing atomic merger
to seed a missing master or merge a compatible session. New merge conflicts
fail before atomic replacement.

**Consequences:** Repeated structure and overlapping Worker windows merge into
one graph while each observation remains in lineage; distinct targets remain
branch alternatives. Runtime facts remain `OBSERVED`, cannot self-promote to
`PROVEN`, and do not change SOURCE_OWNED. The proof adapter and production AUTO67
path remain independent. Worker/native/Lua instruction hooks are unchanged.

**Evidence:** `tests/live_forward_cartographer_test.py` and the two-run
`M12-AUTO67-LIVE-FORWARD-CARTOGRAPHER-2A` receipt.

# ADR-M12-ROM-RANGE-LINKAGE-2B — Exact executed ROM instruction ranges
**Status:** Accepted for the developer-only 2B checkpoint
**Date:** 2026-09-17

**Context:** Worker 1B FLOW_V1 records preserve the runtime PC and opcode, while
Cartographer 2A preserves the observed execution graph. Neither currently
proves which canonical ROM bytes encode each captured instruction or records
the terminal `next_pc` as an address-only fact.

**Decision:** Reuse the existing bounded M68K decoder and explicit 24-bit
memory-region resolver in a developer-only native batch helper. After one
bounded 1B run completes, project supported records into stable,
ROM-SHA/start/end/full-bytes `ROM_INSTRUCTION_RANGE` nodes and
`OBSERVED` `EXECUTED_FROM_ROM` edges. Preserve run/epoch/Worker/capture/
generation/record identity in edge lineage, and represent each segment's
terminal `next_pc` through an `OBSERVED_NEXT_PC` edge to an address node only.
Non-ROM locations remain unlinked; unsupported exact lengths remain unresolved
and prevent a PASS result. Export observed and unique byte coverage separately
from source ownership, and independently reconcile every saved instruction
occurrence and terminal fact against raw records, canonical bytes, a fresh
bounded decode and persisted MAP-1 identities.

**Consequences:** Runtime evidence can identify exact executed instruction
encodings without claiming complete ROM classification, `SOURCE_OWNED`, or
`PROVEN` source knowledge. FLOW_V1 raw records and 2A graph facts remain
unchanged. No Worker/native/Lua hot-path instrumentation, 1B scaling semantics,
production AUTO67, predecessor logic or roadmap status changes.

**Evidence:** `tests/live_forward_rom_link_test.py`,
`tools/bizhawk-native-ring/live_forward_rom_link_audit.py`, and
`docs/reports/THOR_M12_ROM_RANGE_LINKAGE_2B.md` (full runtime and saved-result
acceptance passed).

# ADR-M12-ARCHIVIST-CANONICAL-KNOWLEDGE-PIPELINE-2G — Atomic post-run bridge
**Status:** Accepted for the developer-only 2G checkpoint
**Date:** 2026-09-18

**Context:** Accepted closed MAP-1 sessions could be merged by Archivist and
canonicalized by 2D, but the handoff was manual. Updating the Archivist master
and canonical knowledge DB separately could expose a half-published checkpoint.

**Decision:** Keep Cartographer, Archivist, and 2D responsibilities separate.
Archivist returns a hash-bound merge receipt without inline lineage. A
delta-oriented adapter validates the session/master graph chain and translates
only supported factual MAP-1 records to stable ROM-SHA/range/type identities;
runtime coordinates and occurrences remain evidence locators. Build the
master and knowledge databases as one ignored immutable generation, replay the
import to prove idempotence, independently audit it, then atomically publish a
single `current.json` pointer to the pair. A failed import or audit leaves the
previous pointer and generation untouched. Unsupported factual types stop with
`UNMAPPED_FACT_TYPE`; runtime observations cannot change SOURCE_OWNED or the
emission partition. 2E pointer, offset, and selected table relations are
accepted only from exact-byte synthetic fixtures until a canonical witness
exists.

**Consequences:** Canonical knowledge can be refreshed automatically after a
closed session with rollback-safe publication. No CPU/runtime callback,
Worker/FLOW behavior, production AUTO67, predecessor semantics, ownership, or
emission classification changes. Synthetic relation fixtures remain isolated
from the canonical Beyond Oasis map.

**Evidence:** `tests/rom_knowledge_pipeline_test.py`,
`docs/reports/THOR_M12_ARCHIVIST_KNOWLEDGE_PIPELINE_2G.md`, and its compact JSON
receipt.

# ADR-M12-LIVE-WORKER-CONTROL-WINDOW-2H — Snapshot-only operator window
**Status:** Accepted for the developer-only 2H checkpoint
**Date:** 2026-09-18

**Context:** The accepted 1B runtime already owns exact Worker transitions,
per-Worker lifecycle counters, and the authoritative native allocation
planner, while the accepted 2G launch provides canonical Beyond Oasis
execution. Operators need live, legible status and a way to persist a future
configuration without changing a running pool.

**Decision:** Keep the operator window in a separate process. It reads one
replaceable, bounded status snapshot and writes only a deterministic next-run
configuration plus a replaceable preview request. The current launcher
captures its configuration once before EmuHawk starts. During execution Lua
may call only the existing native memory planner and a new read-only Worker
status query at a bounded rate; it may not resize, reallocate, switch
generations, or wait for settings. The launcher remains responsible for
system/process memory sampling and measured evidence-size rate, and uses the
same resource-budget calculation for preview and startup rejection.

**Consequences:** Closing or slowing the window cannot back-pressure the
runtime; Worker presentation uses native lifecycle state and current
control-flow depth instead of a guessed progress value. Arbitrarily large
positive decimal requests can be saved without UI clamping, then fail closed
at exact native representability, allocation-plan, or process-budget checks.
No CPU hook, Worker/FLOW semantics, production AUTO67, predecessor behavior,
Cartographer/Archivist/2D/2E semantics, SOURCE_OWNED, or emission changes.

**Evidence:** `tests/live_worker_control_test.py` and
`docs/reports/THOR_M12_LIVE_WORKER_CONTROL_WINDOW_2H.md` with its compact JSON
receipt. Five real canonical-ROM campaigns completed 6,400 independently
audited segments; window-off/on timing, current-run configuration immutability,
and next-launch application are recorded in the report.

# ADR-AUTO67-PREDISPATCH-TRANSPORT-CLEAN-1R1 — Final snapshot closes the cursor
**Status:** Accepted for M12 AUTO67
**Date:** 2026-09-15

**Context:** The initial transport cleanup consumed periodic status snapshots
but left the final replaceable Lua snapshot outside the transport cursor. It
also retained a sequence-only state option on the live-opportunistic runner,
and accepted mismatched explicit occurrence identifiers.

**Decision:** Consume `lua_final` with the same `PreDispatchTransport` instance
after emulator exit and before `Dispatcher.stop()`. Validate explicit
`occurrence_id` as exactly `epoch=<epoch>:seq=<seq>`; synthesize it only for
legacy records. Remove `--state` and `OASIS_LIVE_STATE` from this runner path
because `live_opportunistic.lua` has no such consumer. Keep shutdown on the
existing stop event and bounded capsule wait.

**Consequences:** Final-only records reach Dispatcher exactly once, shutdown
does not wait for new emulator data, and identity mismatches fail closed. No
ring, Dispatcher, Worker, capture, persistence, graph, ownership, or C++ path
is redesigned.

**Evidence:** `docs/reports/THOR_M12_AUTO67_PREDISPATCH_TRANSPORT_CLEAN_1.md`.

# ADR-M14.4-CANONICAL-ROM-GENERIC-ASM-CLOSURE — Reuse decoder and SQLite authority
**Status:** Accepted for the M14.4 evidence campaign
**Date:** 2026-09-25

**Context:** M14.3 ranked 27 UNKNOWN intervals with exact executed-instruction
evidence but lacked canonical-byte CFG closure and assembler roundtrip. The
project already owns the M68K decoder/emitter and canonical `knowledge.sqlite`
schema; an additional evidence database would duplicate authority.

**Decision:** Verify the canonical ROM size and SHA before bounded byte reads.
Use exact runtime and reconstruction instruction claims plus typed exact
canonical-reference/ASM-CFG relation targets, with their source evidence
references, to seed the existing decoder. Merge recursive CFG results with overlap/exit checks,
and require the existing ASM emitter plus vasm to reproduce each contiguous
extent. Persist exact results only as parent-bound map proposals in an isolated
copy of the authoritative SQLite generation. Apply no map operation and do not
change SOURCE_OWNED in this campaign. Any later ownership change must continue
through the existing Stage7 promotion path.

**Consequences:** Global evidence, provenance, and proposal identities remain
in the existing canonical SQLite schema. UNKNOWN extents that remain open,
unsupported, or non-roundtripping remain unclassified; proposal rows do not
change canonical emission or ownership.

**Evidence:** `src/tools/re_cfg_closure.*`,
`src/tools/thor_evidence/rom_generic_asm_closure.py`, and the M14.4 report.

# ADR-M14.5-EXACT-ASM-MAP-ADOPTION — Apply validated classification in canonical SQLite
**Status:** Accepted for M14.5
**Date:** 2026-09-25

**Context:** M14.4 persisted its exact ASM closure as a parent-bound proposal.
The project already has an authoritative canonical ROM SQLite generation and
Stage7 ownership promotion contract. A parallel map/evidence store would split
truth and complicate graph-to-map reconciliation.

**Decision:** Reproduce the tracked M14.4 proposal, independently validate its
operations and component proofs, then apply accepted classification operations
to deterministic child copies of the same SQLite generation. Preserve prior
range/evidence identities and record split lineage through existing proposal,
evidence and derivation tables. Expose component-scoped proofs in the existing
global object view. ASM classification remains non-owning; ownership can change
only through the existing Stage7 path.

**Consequences:** Exact proven extents may become canonical ASM while the
enclosing UNKNOWN ranges remain conservatively split. Unsupported references
stay rejected independently. Graph and map continue to share the established
SQLite authority, and `SOURCE_OWNED` is unchanged when Stage7 prerequisites are
absent.

**Evidence:** M14.5 implementation and `THOR_M14_5_EXACT_ASM_MAP_ADOPTION` report.

# ADR-M14.6-RECONCILE-EXACT-ASM-REFERENCES — Keep Stage7 as the ownership authority
**Status:** Accepted for M14.6
**Date:** 2026-09-25

**Context:** M14.5 correctly rejected six M14.4 exact CFG references because
their source instruction objects were absent from the proposal's parent
generation. The accepted M14.5 child now contains those exact instructions,
but remains non-owning. Rejected graph links must not be confused with
caller evidence or used to bypass Stage7.

**Decision:** Reconcile each reference into a deterministic child of the same
canonical SQLite generation only after resolving its original proposal proof
references, matching the M14.5 component CFG and roundtrip proofs, and checking
both endpoint instruction bytes against the canonical ROM. Reuse the existing
Stage7 selector and split predicate for eligibility reporting. Never derive a
caller relation from an intra-component CFG edge; change ownership only after
the complete existing Stage7 and full-ROM audit path succeeds.

**Consequences:** The six previously missing references become auditable
canonical graph edges across two components / 158 bytes. Five exact components
remain ineligible without static caller evidence; the missing Stage7
reconstruction artifact set is independently reported. No parallel database,
promotion path, or ownership accounting rule is introduced.

**Evidence:** `src/tools/thor_evidence/rom_knowledge_stage7_closure.py` and
`docs/reports/THOR_M14_6_EXACT_ASM_SOURCE_OWNERSHIP_CLOSURE.json`.

# ADR-M14.7-STATIC-ENTRY-PROOF — Record exact fallthrough entry evidence without widening Stage7
**Status:** Accepted for M14.7
**Date:** 2026-09-25

**Context:** Five exact, non-owning M14.5 ASM components had runtime entry
observations but no static entry or caller facts. The accepted M68K decoder
treated absolute-word JSR/JMP operands as indirect even though the encoding
provides an exact sign-extended target. Static references must be grounded in
canonical ROM bytes and exact instruction boundaries, while Stage7 retains its
existing direct-caller selector.

**Decision:** Correct absolute-word JSR/JMP target decoding and scan only
accepted source-owned 68K ASM emission intervals, anchored at their map
boundaries. Admit a `STATIC_VERIFIED_ENTRY` claim only when decoded direct
control flow or semantics-proven fallthrough reaches the exact accepted entry
object. Store proof edges, canonical-ROM evidence refs and derivation inputs in
the existing SQLite generation. Do not reinterpret fallthrough as a direct
caller for the Stage7 selector; construct no full-ROM manifest unless that
selector admits a component.

**Consequences:** Static entry truth is independently queryable from runtime
observations. Exact entry facts that do not satisfy Stage7's caller gate remain
non-owning, and no unknown bytes are classified or promoted by this decision.

**Evidence:** `src/tools/re_static_xref_scan.cpp`,
`src/tools/thor_evidence/rom_knowledge_static_entry.py`, and the M14.7 report.


# ADR-M14.7A — Preserve execution witnesses independently of structure

**Status:** Accepted for bounded developer-side session fusion.

**Context:** MAP-1 exported structure but omitted its existing exact session
occurrence table; AUTO67 dependency bundles discarded local occurrence lineage.

**Decision:** Reuse the existing optional native-event table in MAP-1 master,
union by accepted scoped native identity, reject core-payload contradictions
before atomic publication, and seal new session event evidence independently.
Keep AUTO67 dependency hashes structural and add occurrence witnesses. Derive
ordered paths from existing witnesses; do not create a second canonical store,
trie authority or ownership path. same prefix is not same full chain.

**Consequences:** Replay preserves exact counts; historical maps without exact
events remain readable but cannot supply fabricated trajectories. Capture
windows remain bounded; retained evidence grows with observations. Canonical
knowledge schema and all truth/ownership rules remain unchanged.


# ADR-M14.7B — Fail-closed raw retention and evidence completeness

**Status:** Accepted for report-only audit and seal evaluation; complete
lossless normalization and replay integration remain open.

**Context:** M14.7A preserves native runtime occurrences and path lineage, but
the current MAP-1 event projection omits raw-format fields and does not account
for each rejected or unresolved input event. Existing pipeline receipts prove
their own analysis stages, not semantic completeness of the raw capture.

**Decision:** Treat each raw format separately. Fixed-width FLOW_V1/W3 V2
records use a lossless field envelope that retains every source record and
locator, with accepted/unresolved/rejected/duplicate accounting and original
byte-hash replay. A raw binary can be marked disposable only when a
format-, extractor-, validator- and generation-bound normalized artifact and complete accounting exist and an
independent replay without raw matches graph, path, occurrence and unresolved
identities while preserving rare branch witnesses and capture gaps. Normalized
evidence has a separate gate requiring equivalent replay from the sealed
session store. Missing or stale proof means KEEP with a machine-readable
reason. The implementation audits and writes separate artifacts; it never
deletes files.

**Consequences:** Existing PASS receipts do not imply delete safety, and no
current raw file is automatically reclaimed. Canonical knowledge, MAP-1
schemas, truth labels, SOURCE_OWNED and capture behavior remain unchanged. A
future deletion command requires a separate milestone and fresh seal check.

## M14.7B continuation — one canonical map and consumable captures

**Status:** Accepted policy; implementation and historical corpus ingestion
remain in progress.

**Context:** A permanent raw or normalized archive would duplicate knowledge
authority. The existing Archivist pipeline already publishes one validated
canonical generation through an atomic current-generation pointer.

**Decision:** Keep that existing map as the sole long-term knowledge store.
Each experiment closes with a compact receipt binding raw identity, complete
event accounting, map generation/hash before and after, map self-check and
unchanged `SOURCE_OWNED`. `MERGED` requires a new generation;
`NO_NEW_KNOWLEDGE` and `INVALID` require the map to remain unchanged, with an
explicit invalid reason for the latter. Only closed receipts may authorize
deletion. Cleanup is explicit, verifies receipt and raw identity, and accepts
only paths contained in the exact audited root. Temporary envelopes and
session artifacts share the consumable lifecycle; they do not become a second
evidence authority.

**Consequences:** Old captures without a reader remain open until bounded
investigation classifies them. Receipt tooling does not itself imply that a
capture was ingested. No historical artifact may be removed until its own
validated receipt exists. Runtime observations remain observations and do not
promote truth or ownership.

## M14.7B real capture closure — bounded evidence accepted

**Status:** Accepted for one indexed FLOW_V1 evidence bundle; the remaining
corpus stays open for per-experiment ingestion.

**Context:** Synthetic lifecycle tests established receipt and cleanup policy,
but did not prove real format ingestion, full canonical lineage, or queries
after real raw deletion. The first selected bundle contained both M68K and Z80
events, overlapping worker windows, terminal address facts and exact raw byte
locators.

**Decision:** Preserve every accepted CPU-specific runtime occurrence in the
existing canonical evidence architecture. Link only M68K instructions to the
M68K ROM; keep Z80 instructions as runtime observations. Treat indexed
overlap duplicates as already-known event accounting while retaining their
window provenance. Define runtime paths by stable capture-window identity and
retain per-event source offsets separately. Use the canonical pipeline's
staged generation and atomic current-pointer replacement, and calculate
`SOURCE_OWNED` from the parent map rather than a constant. Issue an exact
closed receipt only after independent segment/ROM checks, full map integrity
and path queries, complete accounting and unchanged ownership. Delete only
the receipt's selected raw and named temporary staging files; retain the
segment index and byte-identical historical pass copies.

**Consequences:** This proves one real bounded `CAPTURE → ONE MAP → RAW
DELETED` lifecycle. It does not change the source campaign's
`STOPPED_FRAME_LIMIT` result, prove whole-game reachability, or classify other
captures as `NO_NEW_KNOWLEDGE`. Remaining content-identical files remain
separate physical paths until their experiment identities are reconciled.
Runtime evidence stays non-owning.

**Evidence:** `docs/reports/THOR_M14_7B_REAL_CAPTURE_CLOSURE.md`, its exact
receipt under `docs/reports/m14-7b-closure-receipts/`, and generated
pre/post-cleanup map checks under `build/thor-evidence/m14-7b-real-capture/`.
## ROM Coverage and canonical evidence map integration

**Status:** Accepted for a read-only visualization integration.

**Context:** The ROM Coverage Map reports byte-level property masks from accepted runtime checkpoints. The persistent M14 knowledge map stores ROM-addressed objects, claims, relations, evidence references and conflicts. These answer different questions and can overlap: a byte can have observed coverage while its semantic evidence remains unresolved, and a knowledge-map claim must not silently become a byte-property proof.

**Decision:** Present both maps in one GUI as independent layers bound to the same exact ROM SHA-256 and size. Preserve the property bitmap as the cell fill; show canonical evidence status as a separate outline and expose its supporting objects/claims in the range detail. Read the atomically published canonical `current.json` pointer and its SQLite generation using read-only access. In LIVE mode, refresh on pointer replacement from a background loader. Count bytes as a union within each evidence status; statuses may overlap, and these counts do not alter property coverage. The viewer must never write the canonical generation, change proof status, or promote `SOURCE_OWNED`.

The evidence loader prefers the promoted `oasis.m12.master-v2-startup.current.v1` pointer at the project runtime path and resolves the M14 Archivist pointer only when the MASTER pointer is absent. An explicit pointer argument overrides discovery. MASTER V2 is checked against the exact ROM identity, file and logical hashes, section hashes, generation ID, and a contained relative master path before any claims are shown. Claim statuses absent from the viewer's known status vocabulary remain in a separate unmapped-status overlay; they are not translated into `OBSERVED_RUNTIME` or another known class.

**Consequences:** Accumulation remains the canonical pipeline's responsibility; the GUI shows the latest accepted generation when it is published. A different ROM identity, invalid generation path, or database hash mismatch fails closed. The two layers can be compared spatially without conflating observed coverage with semantic classification.

The LIVE view also reports checkpoint read state and property-byte growth since the viewer began watching. Lack of a checkpoint change is presented as waiting, not as proof that an emulator or Worker has stalled. Polling and checkpoint parsing remain outside emulator callbacks; this display adds no per-frame capture work.

At byte-level view, an explicit click opens a bounded read-only provenance trace. It follows exact object, claim, relation, evidence-reference, source-artifact and derivation identities; an execution-path segment is displayed only when the linked relation has retained runtime evidence. A derivation output is labeled as a recorded extraction reference only when it includes both a path and SHA-256; the GUI does not validate the external file. Missing provenance remains visible. Trace indexes are built by the background loader, while expensive detail expansion is excluded from redraw and hover.

## Live-forward session progress sidecar

**Status:** Accepted for display-only live progress.

**Context:** The 2A/2B driver admits host-audited FLOW segments to an in-memory
Cartographer as the Worker runs but writes the closed MAP-1 session database
after emulator shutdown. The ROM Coverage GUI previously had no way to show
that accepted-segment growth while the session remained open.

**Decision:** Publish a small atomic sidecar at a bounded host polling cadence.
It carries session/ROM identity, lifecycle state, admitted/expected/rejected
segment counts, MAP-1 node/edge counts, and the session's unchanged
`SOURCE_OWNED` value. The GUI reads the sidecar read-only and displays it only
when its loaded ROM identity matches exactly. Progress is explicitly the
fraction of configured run segments admitted, never a percentage of ROM
coverage. The sidecar is telemetry, not a map generation or proof; final MAP-1
serialization and ROM-link audit remain post-run gates.

**Consequences:** The GUI can show that the session map is growing without
reading a changing SQLite database or adding work to emulator callbacks.
Malformed, stale, failed, or mismatched sidecars remain visibly rejected or
waiting. The canonical generation, property bitmap, and ownership publisher
remain unchanged.

## M14.7B W3 lineage bridge

**Status:** Accepted for deterministic producer-contract reconstruction only.

**Context:** W3 V2 record bytes contain event fields but omit run, epoch, Worker,
capture, and segment identity. Historical waves can still have a complete
companion `segment-audits.jsonl` and run receipt. The producer writes one wave
per round and appends each Worker export in ascending worker order.

**Decision:** Reconstruct boundaries from the receipt-bound audit rows and the
deterministic producer order only when all expected Worker IDs occur once,
record counts cover the raw wave exactly, every contiguous raw window hash
matches its audited segment hash, and the event stream bounds match. Persist
this reconstruction as a temporary, raw-hash and audit-hash-bound sidecar for
new captures. Reject incomplete, ambiguous, missing, or tampered evidence; do
not infer from timestamps, sequence proximity, filenames alone, or partial
byte matches. Keep W3 payload format and existing readers unchanged.

**Consequences:** A legacy wave with complete audited companion evidence can
retain the existing MAP-1 occurrence and path lineage through the existing
Cartographer. A wave without that evidence remains OPEN until independently
classified; it is never assigned guessed capture identity. SOURCE_OWNED and
truth classes remain unchanged.

## ADR-M14.7B-NORMALIZED-V2 — One-map record accounting

**Status:** Accepted for the first historical normalized-generic v2 artifact.

**Context:** The producer emits `records[]` as its primary normalized event
stream plus several secondary arrays containing projections of those same
events. Counting every array would duplicate input accounting. The existing
canonical publisher accepts MAP-1 sessions and owns the staged generation
transaction, while normalized v2 lacks capture-window identity needed to
invent MAP-1 runtime paths.

**Decision:** Stream only primary `records[]`. Preserve one durable
`evidence_ref` per source ordinal in the existing canonical map, with an exact
artifact SHA, run/epoch/sequence identity, record type, outcome, and original
fields. Attach only exact M68K instruction facts to already-existing canonical
instruction objects after validating the opcode against the accepted ROM.
Records without a supported canonical representation remain queryable as
`UNRESOLVED`; malformed records are `REJECTED`. Apply evidence only to the
publisher's staged child database through `KnowledgeStore`, then use the normal
independent audit, atomic pointer replacement, and generation GC. Stream the
existing canonical logical-hash encoding to keep memory bounded.

**Consequences:** `SOURCE_OWNED`, emission, object boundaries, and accepted ROM
identity remain unchanged. Source stream order and record provenance survive
deletion of the normalized JSON. The adapter does not claim capture-window or
runtime-path reconstruction because v2 records omit that identity. The first
artifact is not disposable until complete accounting, map self-check,
provenance queries, replay, and the existing closed-only cleanup lifecycle all
pass.
# ADR-M12-END-GAME-POSTRUN-2I.2 — fail-closed stages 5–9

**Status:** Accepted for developer-only post-run orchestration.

**Decision:** Reuse the existing 2I.1 progress publisher and invoke one
post-run coordinator after rolling-master compaction. The coordinator may
construct a closed MAP-1 session only from a sealed, hash-checked ordered FLOW
spool, and may publish a new canonical generation only through an atomic staged
master/knowledge pair with idempotent replay and unchanged SOURCE_OWNED and
emission hashes. Control provenance inspects ordered FLOW; aggregate rolling
counts cannot supply predecessor evidence. ASM closure, full ROM audit, and
cleanup remain fail-closed until an accepted runner and all required inputs are
present. A STOP retains raw evidence and leaves later stages pending.

**Consequences:** Normal END GAME now enters the real stage 5–9 path and shows
an exact STOP reason when the current tree lacks a required accepted input.
The UI does not claim completion from a displayed stage list. Worker/Lua/FLOW
runtime semantics and production ownership remain unchanged.
# ADR-M12-END-GAME-POSTRUN-2I.2a — terminal state coherence

**Status:** Accepted for developer-only post-run orchestration.

**Decision:** All terminal transitions use one progress finalizer. It clears
the active stage, terminalizes the current stage, blocks later mandatory
stages after STOP/ERROR, persists backend and exception metadata, and writes a
single atomic snapshot. Heartbeat staleness remains non-terminal while the
backend PID is alive. Malformed terminal snapshots are repaired before reuse.

**Consequences:** `COMPLETE`, `PARTIAL_COMPLETE`, `STOPPED` and `FAILED` can
never coexist with an ACTIVE stage. Raw evidence and accepted generations are
retained on STOP/ERROR. This change affects status orchestration and UI display
only; Worker/FLOW/Lua semantics and 2F reconstruction remain unchanged.

# ADR-M12-INTERACTIVE-PREFLIGHT-POSTRUN-GATE — unsealed runs do not analyze

**Status:** Accepted for developer-only interactive orchestration.

**Decision:** Launch the post-run analyzer only after an interactive runtime
has emitted a sealed terminal stop outcome. A resource, native-allocation, or
host-transport preflight rejection is a visible Worker Control result, but it
does not create a Stage 1/9 analyzer window. The rejection receipt records the
exact reason and `RUN_NOT_SEALED`.

**Consequences:** A failed start cannot be misreported as a post-run analysis
failure. The resource reserve and fail-closed allocation policy are unchanged;
only the secondary UI orchestration is gated. Worker, Lua, FLOW, and production
runtime semantics remain unchanged.

# ADR-M12-DESKTOP-MEMORY-FLOOR — one GiB host reserve

**Status:** Accepted for developer-only Worker Control launch preflight.

**Decision:** Reserve one GiB of currently available physical RAM for the host
when calculating the desktop Worker Control native allocation budget. Keep the
existing native/process/core caps and reject when the measured remainder cannot
hold the requested plan. The evidence disk reserve remains independent.

**Consequences:** A 32/64 GiB desktop with a few GiB currently free can start
a small or medium requested Worker plan; severe host pressure still fails
closed before EmuHawk starts. No Worker, Lua, FLOW, or production semantics
change.
# ADR-M12-STAGE5-COMPACT-SESSION — deterministic post-run MAP-1 refresh

**Status:** Accepted for developer-only post-run analysis.

**Decision:** Build one closed MAP-1 session from ordered FLOW in a run-specific
analysis directory. Use compact lineage occurrence summaries, exact ROM-range
decoding, and atomic canonical generations. Validate the accepted rolling-master
proof separately; preserve the canonical emission partition and `SOURCE_OWNED`
bytes exactly, failing closed on identity, ROM, count, or ownership conflicts.

**Consequences:** Repeated observations remain auditable without materializing
millions of duplicate lineage entries. `EXECUTED_NEXT` is emitted only for
adjacent instruction records; terminal next-PC observations remain separate.
Worker/Lua/native scaling semantics are unchanged.
# ADR-M12-STAGE7 — Current-generation MAP-driven executed ASM closure

Date: 2026-09-19

The post-run pipeline consumes the accepted canonical generation directly and
recomputes executed, not-fully-owned M68K candidate islands from current map
objects and observed runtime relations. A candidate is promotable only after
exact range decoding, complete control-flow closure, vasm byte round-trip, and
an independent full-ROM rebuild. Dependency closure is fail-closed; unresolved
islands remain blocked records. Promotion publishes a new immutable generation
and updates the canonical pointer atomically. Stage 8 starts automatically and
rebuilds the resulting split independently. No runtime campaign or production
Worker/FLOW semantics are part of this decision.

# ADR-M12-STAGE6-HANDOFF — receipt-bound control provenance lifecycle

Date: 2026-09-19

Stage 6 must consume the exact sealed receipt and ordered FLOW artifact already accepted by Stage 5. Its result is structured as PASS, NO_DELTA, STOP, or ERROR, including run identity, raw FLOW hashes, relation counters, generation, and failure details. NO_DELTA completes Stage 6 and immediately starts Stage 7; it is not a pipeline terminal state. The existing terminal finalizer handles STOP/ERROR so no terminal snapshot may retain an ACTIVE Stage 6. The Worker, Lua, native pool, and Stage 7 semantics remain unchanged.

# ADR-M12-STAGE7-PREFLIGHT — lineage-bound emission partition handoff

Date: 2026-09-19

Stage 7 preflight must compare the current canonical map with a materialized
emission/ownership manifest selected from the explicit generation parent chain.
A historical path or latest/glob selection is not an authority. The preflight
records both authority hashes and exact category totals, fails closed on any
range, ROM, generation, or ownership mismatch, and keeps UNKNOWN fields outside
the emission partition. This preserves the Stage 7 gate while allowing a
post-2F canonical generation to reuse its own coherent materialized split.
Worker, Lua, native scaling, predecessor, and FLOW semantics are unchanged.

# ADR-M12-STAGE6-STOP — unresolved provenance is not an integrity failure

Date: 2026-09-19

Stage 6 may observe real indirect consumers whose register predecessor or
transform cannot be resolved within bounded evidence. Those events remain in
the diagnostic receipt and count as unresolved/unsupported; they do not stop
the pipeline globally. Global STOP is reserved for ROM identity, FLOW
continuity, impossible occurrence identity, or equivalent evidence-integrity
failures. The result contract remains PASS/NO_DELTA/STOP/ERROR and the exact
receipt is published before terminalization.

# ADR-M12-STAGE9-ABSORPTION — permanent reclaim only after semantic proof

Date: 2026-09-19

Stage 9 may permanently delete run-scoped raw FLOW and reconstruction artifacts
only after the exact run is present in the accepted rolling master, all prior
stages and ROM identities pass, and a streaming semantic-equivalence audit
reconciles every instruction, relation and terminal fact against compact
master rows. Preparation writes an explicit delete manifest and an atomic
absorption receipt before deletion; deletion uses direct filesystem unlink with
bounded WinError 32 retry and no recycle-bin path. Replay consumes the durable
manifest/receipt idempotently, while any missing protected state or failed
post-delete integrity check stops closed and never reports cleanup PASS.

The rolling master, canonical map, ownership/emission state, Stage 5–8
receipts, final report/status and ROM are protected. Worker/FLOW runtime
semantics and SOURCE_OWNED truth are unchanged.

# ADR-M12-R1 — Shadow self-contained MASTER V2

**Status:** Accepted for shadow validation only.

**Decision:** Encode the accepted rolling master, canonical map/knowledge and
Stage 5–9 semantic outcomes into a deterministic, section-hashed MASTER V2
container while leaving all legacy pointers and artifacts authoritative. The
container is independently decoded and compared against a streamed semantic
projection of the legacy inputs. No runtime or cleanup path reads MASTER V2.

**Reason:** R2–R10 require a self-contained authority before replacing the
legacy split state. R1 proves the representation, deterministic size and
corruption detection without changing runtime truth semantics or risking the
accepted `PASS_ABSORBED_RAW_PERMANENT_RECLAIM_V1` path.

**Consequences:** The current accepted master remains the only authority. The
shadow currently measures `1,272,891,699` bytes for run `1789872670`; later
migration stages must account for this size before changing authority.

## ADR-M12-R2 — MASTER V2 canonical read authority (2026-09-19)

**Decision:** Use one `MasterCanonicalView` access layer as the canonical read
source for map, ownership and emission data. The R2 pointer selects a verified,
deterministic MASTER V2 shadow generated from the accepted legacy state. Stage 5
materializes a local scratch generation through the API; Stages 6–8 keep their
existing consumers and semantics. Legacy canonical files remain present and
unchanged for hash comparison only.

**Rationale:** This changes exactly one authority boundary while avoiding a
representation rewrite, runtime capture change, or independent MASTER V2
parsers in individual stages. Fail-closed pointer verification prevents a stale
shadow from becoming canonical. The R2 shadow is not itself committed as a
GiB-scale artifact.

**Validation:** Two real generations were byte-identical and semantically equal
to the current legacy state; API counts, `SOURCE_OWNED`, ownership/emission,
materialized SQLite integrity and targeted Stage 5–9 regressions passed.
# ADR-M12-R3-MASTER-OUTCOME-AUTHORITY-1 — durable provenance and outcomes
**Status:** Accepted for M12 R3
**Date:** 2026-09-19

**Context:** R2 made MASTER V2 the canonical map/ownership/emission read
authority, but Stage 6 provenance, Stage 5–9 receipts and absorbed-run proof
still lived in external JSON/SQLite artifacts.

**Decision:** Add independently hashed MASTER V2 `provenance`, `outcomes` and
`absorption_history` sections and expose them through `MasterOutcomeView` and
`MasterProvenanceView`. Legacy receipts remain shadow comparison inputs. A
missing or corrupt section raises `STOP_MASTER_OUTCOME_UNAVAILABLE`; the read
layer never falls back silently to legacy JSON.

**Consequences:** Accepted provenance, terminal stage outcomes, lineage,
semantic-equivalence proof, deleted-byte totals and deletion mode remain
available after raw FLOW deletion. Runtime capture, Stage 5–9 execution and
SOURCE_OWNED semantics are unchanged. Legacy receipt removal is deferred to a
later checkpoint.

**Evidence:** `build/thor-evidence/master-v2-shadow-r3/r3-acceptance.json`.

## ADR-M12-RUN-CONTRIBUTION-RECONCILIATION — run-scoped V2 idempotency

Date: 2026-09-20

**Decision:** Add a hashed `run_contributions` section to MASTER V2 and a
run-scoped contribution boundary. A contribution is applied once by `run_id`
and its logical hash; an identical replay returns `NO_DELTA_ALREADY_APPLIED`,
while a differing contribution stops with `STOP_RUN_CONTRIBUTION_CONFLICT`.
Occurrence deltas are retained even when a fact already exists globally.
`VERIFY_ONLY` and forensic replay do not mutate counters, legacy generations,
or raw evidence. Historical lineage membership alone remains ambiguous.

**Consequences:** The current V2 startup pointer now verifies 54 ledger entries
without changing canonical, provenance, Stage 5–9, Worker/FLOW or
`SOURCE_OWNED` semantics. Six sealed runs are classified `NOT_COMMITTED` and
are eligible only for a later single-run replay; 28 runs remain `AMBIGUOUS`.
No historical raw/session data is deleted and no mass replay is started.

## ADR-M12-SINGLE-V2-RUNTIME-WRITE-AUTHORITY — emulator post-run promotion

Date: 2026-09-20

**Decision:** Interactive Worker runs launched with the desktop control flow
must use MASTER V2 startup and the in-process post-run coordinator. Stage 5–9
may create only a campaign-local rolling scratch materialized from V2. The
sealed contribution is calculated before cleanup, then the completed Stage 9
result is written through an atomic V2 candidate/pointer replacement. The
legacy persistent `rolling-master-2i` tree is not a write authority.

**Consequences:** Future emulator runs append to the single V2 authority while
preserving Stage 5–9 truth semantics and cleanup behavior. The old subprocess
post-run mode is rejected for interactive runs so it cannot recreate a legacy
rolling master. Legacy canonical shadow verification remains available only
for explicit historical fixtures; current V2 startup does not require those
files.

## ADR-M12-R6-STAGE5-IN-MEMORY-CARTOGRAPHER — Stage 5 RAM session boundary

Date: 2026-09-20

**Decision:** Production Stage 5 uses an explicit `Stage5SessionMemory` backed
by SQLite `:memory:` for semantic FLOW aggregation and MAP-1 construction.
The sealed FLOW spool remains the input authority. Exact decoder exchange files
are OS-managed ephemeral inputs/outputs only; they are not persistent Stage 5
evidence. Canonical candidate/master files remain the durable outputs.

**Consequences:** Normal R6 runs report zero Stage 5 SQLite bytes and zero
persistent Stage 5 temporary files, release the in-memory session before Stage
6–9 continues, and preserve the accepted MASTER V2, ownership/emission,
provenance and cleanup semantics. The old disk session path remains available
only in a named legacy shadow module for comparison and regression.

## ADR-M12-R7-FLOW-IN-MEMORY-HANDOFF — Ordered FLOW without persistent spool

Date: 2026-09-20

**Decision:** The interactive R7 path transfers sealed ordered FLOW segments
through a bounded in-memory queue. One runtime owns the handoff; a consumer
feeds Stage 5 and control-provenance streaming consumers, and the coordinator
closes the RAM session only after the queue drains. The existing disk FLOW
implementation remains available as an explicit legacy/shadow path.

**Consequences:** R7 reports explicit segment/record/hash accounting,
backpressure, and zero FLOW disk reads/writes. Stage 5–9 truth semantics,
canonical refresh, MASTER V2 promotion, ownership, emission and cleanup rules
remain unchanged. The in-memory Cartographer connection is safe across the
runtime consumer and post-run coordinator thread boundary.

## ADR-M12-R8-BOUNDED-STAGE7-DECODE — profile-first analysis optimization

Date: 2026-09-20

**Decision:** Parallelize only the independent external Stage 7 candidate
decodes with a bounded four-worker executor. Keep candidate proof, round-trip
validation, promotion and graph/master merge sequential and ordered exactly as
before. Publish timing and R7 handoff counters in `analysis_performance`.

**Consequences:** The accepted R7 truth boundary is unchanged: Stage 5/6/7/8
meaning, candidate ordering, fail-closed errors, `SOURCE_OWNED`, emission,
cleanup and MASTER V2 authority remain the same. The measured median post-run
wall time fell from `85.656 s` to `60.407 s` (`29.48%`), while Stage 7 fell
from `51.906 s` to a `25.141 s` median (`51.57%`). Runtime coverage can still
produce new facts; the second optimized campaign added 57 instructions and 66
relations with zero source-owned delta.

**Evidence:** R8 baseline and optimized campaign reports under
`build/thor-evidence/live-worker-control-2h/`, plus the focused decode and
post-run coordinator regression tests.

## ADR-M12-R9-MEASURED-DECODE-DEDUP — no cache when requests are unique

Date: 2026-09-20

**Decision:** Measure complete Stage 7 external decode request identity before introducing memoization. The measured accepted workload had 139 unique keys for 139 requests (0% duplication), so no completed-result cache, in-flight coalescing table, eviction policy or persistent cache is added. Instead, reuse the immutable initial/final SQLite snapshot when no promotion can have changed it; retain fresh scans after every promotion.

**Consequences:** Decoder key telemetry remains available in the Stage 7 receipt, including request counts, unique keys, duplicate rate, external wall, pool occupancy and key construction cost. Snapshot reuse preserves candidate ordering, proof, promotion, Stage 8, ownership and MASTER semantics while reducing fresh full-run median Stage 7 wall from `28.906 s` to `16.711 s`. R7/R8 in-memory FLOW and zero-disk guarantees remain unchanged.

**Evidence:** R9 reference and optimized campaigns under `build/thor-evidence/live-worker-control-2h/`, direct sealed-generation profile, and `tests/stage7_decode_test.py` metric/key regressions.

## ADR-M12-R10-STAGE7-DECODER-CONTRACT — explicit range decoder role

Date: 2026-09-20

**Decision:** Keep the Stage 5 PC-list decoder and Stage 7 bounded-range
decoder as separate executable roles. Stage 7 requires the existing
`oasis_re_assemble_range.exe` contract
`ROM START END ASM_OUTPUT JSON_OUTPUT`; startup validation requires its exact
`oasis.stage7.range.v1` capability marker. The PC-list helper remains the
Stage 5 `PCS_INPUT TSV_OUTPUT` tool and cannot be substituted silently.

**Consequences:** Invocation/configuration failures are fail-closed with a
decoder-contract, input, output or tool classification and are no longer
reported as unsupported M68K instructions. Stage 7 still owns bounded decode,
candidate ordering and proof handoff; Stage 8 retains independent full-ROM
assembly/round-trip validation. Cleanup validates both the pre-closure input
generation and the promoted current generation when Stage 7 advances canonical
ownership.

**Evidence:** R10 canonical-ROM fixtures, capability probes, the sealed
`campaign-r10-contract-20260920-1` report, and
`tests/stage7_decoder_contract_test.py`.

## ADR-M12-W1-NATIVE-BUS-EVENT-SIDEBAND

Date: 2026-09-20

**Decision:** Extend the existing 32-byte native ordered ring with explicit
`EVENT` records for 68K data reads, data writes and frame boundaries. The event
subtype occupies otherwise unused kind-flag bits. Bus events use the existing
record fields for address/value and pack value high bits, width, mapped domain
and CPU into `auxiliary`. Opcode and extension fetch helpers are excluded;
32-bit top-level accesses produce one event. Frame events use instruction
sequence zero and carry a monotonic per-epoch frame number.

**Consequences:** Existing instruction/exception records and FLOW projection
remain binary-compatible. Host consumers must preserve raw event records but
filter them when deriving instruction-only control-flow, ROM-link and
Cartographer facts. Domains are emulator memory-map facts only; no sprite,
music, DMA or other semantic resource classification is introduced. The
instrumentation remains developer-only and does not change `SOURCE_OWNED`.

**Evidence boundary:** Native unit/parser tests and source-level GPGX syntax
checks pass. Real WBX build, gameplay counts, retention measurements and
`PASS_NATIVE_BUS_EVENT_SIDEBAND_V1` remain pending until the Waterbox sysroot
and isolated runtime campaign are available.

## ADR-M12-W2-ACTIVE-RESOURCE-CLASSIFICATION

Date: 2026-09-20

**Decision:** Interpret immutable W1 bus events in a separate bounded Worker
analysis layer. Primitive labels are mapped-memory facts only. Instruction
relations require exact ordered event association and a closed decoded
instruction rule; equal values alone are insufficient. VDP register/control
and CPU DMA programming are decoded from observed writes, while internal DMA
payload, resource ownership, sprite identity, song and instrument semantics
remain explicitly unclaimed. S1–S8 inputs are read-only and cross-links require
coherent run/frame/range identity.

**Consequences:** W2 produces deterministic analysis artifacts, bounded
candidates and fail-closed gaps without modifying the native capture path,
ring ABI, Worker scheduling, FLOW semantics, S1–S8 artifacts or
`SOURCE_OWNED`. `OBSERVED` and `DERIVED_EXACT` remain distinct, and
`HYPOTHESIS` is not emitted by this checkpoint.

**Evidence:** `docs/reports/THOR_M12_WORKER_ACTIVE_RESOURCE_CLASSIFICATION_V1.md`
and `build/m12-w2-acceptance/w2-active-resource-classification.json`.

## ADR-M12-W2.1-FRAME-COHERENT-WORKER-EVIDENCE

Date: 2026-09-20

**Decision:** Anchor each accepted Worker segment with `entry_frame` and
`exit_frame` snapshots of the already-authoritative W1 `frame_number`. Equal
snapshots are single-frame only when no contained boundary contradicts them;
changed snapshots require contained FRAME_BOUNDARY markers for exact
partitioning and otherwise remain unresolved. Frame identity is the tuple
`(run_id, epoch, frame)`.

**Consequences:** The fixed 32-byte raw record and native bus semantics remain
unchanged. W2 events, relations, VDP/DMA facts and candidates carry exact
frame identity when the segment proof permits it, otherwise `UNRESOLVED`.
S1–S8 inputs remain read-only and cross-run links are explicitly forbidden.
No graphics meaning, Z80 implementation, Worker scheduling change or
`SOURCE_OWNED` change is introduced.

**Evidence:** `docs/reports/THOR_M12_WORKER_FRAME_COHERENT_EVIDENCE_V1.md`
and `build/m12-w2-1-acceptance/w2-frame-coherent-evidence.json`.

# ADR-M13.2-GENERIC-OBSERVED-CFG-AND-RAW-RETENTION

**Date:** 2026-09-23

**Status:** Accepted for the M13.2 developer-only closure pipeline.

**Context:** The M13.1 closure received millions of normalized FLOW rows but no
decoded instructions, memory facts, or exact range inputs. It emitted no graph
facts, and raw FLOW was deleted before method gaps could be independently
replayed. Existing normalized rows preserve a PC/opcode/next-PC subset, but omit
CPU/domain identity and register snapshots.

**Decision:** The generic closure core may materialize an observed PC-to-next-PC
edge when and only when the source opcode matches canonical ROM bytes. It must
retain an explicit observed-edge truth label; it must not infer static branch
semantics, candidate extents, subsystem meaning, or SOURCE_OWNED changes from
that edge. The live stage must run twice deterministically and emit a hashed
gap ranking. Permanent raw-FLOW reclamation must require successful generic
closure, the ranking artifact, and deterministic replay acceptance.

**Consequences:** Missing instruction identity, CPU/domain, registers, bus
width/value/order, and byte-round-trip boundaries remain unresolved. Observed
CFG recovery can improve evidence navigation but by itself has no measurable
ROM ownership payoff. A failed or missing acceptance artifact prevents cleanup.

**Evidence:** `docs/reports/THOR_M13_2_GENERIC_CLOSURE_GAP_DRIVEN_IMPROVEMENT_V1.md`
and the replay artifacts beneath `build/thor-evidence/live-worker-control/`.

# ADR-M13.3-VERSIONED-GENERIC-FLOW-NORMALIZATION

**Date:** 2026-09-23

**Status:** Accepted as an additive, partial normalization contract; M13.3
capture acceptance remains open.

**Context:** M13.2 showed that the closure adapter discarded fields already
present in native FLOW V2, including CPU ID, bus width/domain, stream and
instruction sequence, and next-PC. The fixed 48-byte record does not carry a
per-instruction register snapshot or complete M68K instruction bytes/width.

**Decision:** Preserve all available fields through `oasis.m13.normalized-
generic-corpus.v2`, materialize generic instruction, memory, ROM-read,
control-flow, call, return, and indirect-target collections, and classify
captured M68K opcodes against the canonical ROM without dropping mismatches.
Use only native `cpu_id` for CPU identity. Carry the exact Worker segment
entry/exit register snapshots from host-audited metadata and link them to the
first/last instruction identities; never imply they describe intervening
instructions. Missing per-instruction snapshots, decode, width, and
address-space classifications remain explicit unresolved fields. Generic
closure consumes normalized memory, ROM-read, call, and return facts while
maintaining compatibility with v1 synthetic fixtures.

**Consequences:** Available evidence is no longer lost at normalization, but
the register reaching-definition contract is incomplete until native
per-instruction bounded snapshots and references are captured. V2 status alone
does not pass M13.3 acceptance and cannot release raw FLOW/index files. No
ownership promotion or production runtime change is implied.

**Evidence:** M13.3 normalizer tests, live generic-stage tests, and the
preserved validation capture receipt (when emitted).
# ADR-M14.2A — Canonical Global Evidence Graph Persistence
**Status:** Proposed; safe-admission acceptance remains incomplete
**Date:** 2026-09-24

**Decision:** Add derivation and map-proposal records to the existing canonical
ROM knowledge SQLite database. Keep `emission` as the sole ROM partition and
`SOURCE_OWNED` authority. Runtime captures remain scoped evidence; proposals
are bound to exact parent/map/graph/validator identities and are not canonical
facts. MASTER V2 must preserve all added rows. This decision does not authorize
graph fusion or ownership changes.

**Acceptance limitation:** Runtime evidence import has not yet been migrated
to admit individual scoped occurrence identities; therefore M14.2A safe
admission is not complete.

**M14.2A safe-admission completion:** The live importer now validates the
existing scoped runtime occurrence identity and creates one evidence reference
per occurrence, with individual claim/relation support references. The session
occurrence log remains source-session evidence; canonical `knowledge.sqlite`
retains the references. Focused replay, overlap-window, CPU/domain, persistence,
and emission invariance acceptance passed. This does not fuse subsystems or
change emission ownership.
# ADR-ROM-PROPERTY-MAP-V1 — Direct observed-property map

**Status:** Accepted for developer tooling; emulator runtime acceptance open
**Date:** 2026-09-27

**Context:** Required raw FLOW retention makes direct facts expensive to use
for bounded ROM classification. The canonical emission map also records a
different concern: reconstructed/source-owned partitioning.

**Decision:** Add an independent dense `uint16_t` runtime property map. Its
bits represent only evidence-backed use of exact physical ROM bytes. Zero is
UNKNOWN. Checkpoints bind the map and per-run contributions to exact ROM,
schema, proof-contract, core/build, run, generation, capability, and validation
identity. Compatible contributions may be OR-merged and can be excluded and
replayed. Ranges are exported as a full-ROM interval partition and may be
overlaid on the existing canonical map without changing canonical class,
truth, or ownership. No GPU, raw event archive, or semantic promotion is
required for these direct facts.

The byte-exact property and rejection contracts live in
`src/tools/thor_evidence/runtime_rom_properties_contract.json`; the checkpoint
identity pins its SHA-256. Emulator callbacks must prove backing and completion
before calling low-level map operations. Unsupported cases stay zero.

**Consequences:** Checkpoint, merge, exporter, M68K decoder-backed span
primitive, Z80 fetched-byte validation, exact-copy RAM origins, and a
canonical overlay are implemented as developer tooling. The tools do not yet
establish that the emulator calls these primitives. Audio payload promotion,
live callback acceptance, behavioral parity, and runtime performance remain
unproven. This ADR does not change ROM reconstruction, emission authority, or
`SOURCE_OWNED` gates.

## ADR-ROM-PROPERTY-VDP-DIRECT-MOVE — Same-instruction VDP source

**Status:** Accepted for implementation; live acceptance pending
**Date:** 2026-09-27

**Context:** The direct DMA adapter identifies physical ROM sources but cannot
cover a CPU data-port write where one instruction reads a ROM word and forwards
it directly to VRAM, CRAM, or VSRAM. General register tracking is not required
for this bounded case.

**Decision:** Reuse the existing VDP source bits. On Mode 5 CPU data-port
writes, retain at most one exact ROM-backed 16-bit data read in the current
M68K instruction. Set the matching VDP bit only after the write reaches the
accepted hardware data path and the instruction completes as a memory-source
`MOVE.W` whose value is unchanged. Reject other opcodes, address-register and
data-register sources, immediate sources, extra reads, mismatches, incomplete
instructions, and DMA-fill trigger writes. The contract hash changes so old
and new contributions cannot be merged without an explicit migration.

**Consequences:** This adds no property bit, canonical-map mutation, or generic
CPU taint engine. It covers only a direct same-instruction ROM-to-port path.
Register-mediated paths, RAM copies, graphics decompression, and audio remain
separate proof contracts. Live capture, parity, and performance results are
required before runtime acceptance.

## ADR-ROM-PROPERTY-W5-AUDIO-CONSUMER — Exact Format-A Decoder Read Classification

**Status:** Accepted; fresh 3600-frame runtime capture and exact replay passed
**Date:** 2026-09-27

**Context:** W4 established a live banked-ROM → Z80 decoder → YM2612 DAC chain.
W5 independently closed two Format-A mode-0 resource ranges with a
byte-identical decode/inverse-encode roundtrip. Generic Z80 data reads and
sound-bank membership alone do not prove audio payload use.

**Decision:** Reuse `AUDIO_PAYLOAD_PROVEN` only for byte-verified banked-ROM
reads at decoder PCs `0x080E` and `0x0855` whose physical ROM offsets lie in
`[0x0BC95C,0x0BD540)` or `[0x0BD540,0x0BF768)`. Mark only the exact byte read.
Reject descriptors, other PCs, and all other banked ROM addresses. Bind these
fixed inputs to the proof-contract hash.

**Consequences:** This adds no property bit, whole-resource promotion, generic
register taint, or canonical ownership change. Existing checkpoints use the
prior proof contract and cannot be merged with contributions under this rule.

## ADR-ROM-PROPERTY-3820-COMPRESSED-GRAPHICS — Completed decoder input span

**Status:** Implemented; fresh runtime replay passed, coverage gain is bounded
**Date:** 2026-09-27

**Context:** The ROM has a verified `0x3820` graphics decompressor. Its `A0`
argument is the compressed input pointer and returns immediately after consumed
input. The routine has no nested call. Direct VDP source bits cannot describe
compressed input without overstating what the hardware consumed.

**Decision:** Add `COMPRESSED_GRAPHICS_SOURCE`. Capture `A0` only when the live
M68K instruction stream enters exactly `0x3820`. At its completed `RTS`, accept
only a nonempty span of at most 64 KiB whose every byte resolves to contiguous
physical ROM offsets. Mark exactly `[A0_entry,A0_return)`. Exceptions,
unsupported mappings, other entry points, and incomplete calls fail closed.

**Consequences:** This adds one independent property bit and changes the proof
contract identity. It does not imply direct VRAM use, change canonical
ownership, track transformed RAM output, or promote all statically known
resources. Older checkpoints remain valid only under their own contract hash.
# DEV-BRANCH-CONSOLIDATION-2026-09 — Persistent branch policy

**Status:** Accepted
**Date:** 2026-09-28

**Context:** Parallel M12/M14 implementation and evidence work had accumulated
multiple local branches and worktrees with overlapping changes. That made it
hard to tell which runtime, map, and test changes represented the current
project state.

**Decision:** Keep `main` as the consolidated project line and
`evidence/rom-coverage-map` as the only second branch. Do implementation and
game behavior changes directly on `main`. Use the evidence branch only for new
ROM coverage observations, proof artifacts, and the minimal reproducible
checks needed to validate those findings; do not develop gameplay or runtime
features there. After review, merge accepted evidence into `main`, advance the
evidence branch to the new `main` commit, and publish both. Do not create other
development branches. When old branch history is superseded, preserve useful
commits in `main` without reactivating stale task instructions.

**Consequences:** Existing local M12/M14 implementation and evidence histories
are reconciled before obsolete branch references are removed. Existing
evidence artifacts and dirty worktree contents remain preserved during
consolidation. Only `main` and `evidence/rom-coverage-map` remain locally and
on the remote. Evidence returns to the consolidated mainline after each
accepted finding, so the second branch is ready for the next evidence-only
change.
