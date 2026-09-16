# M12-AUTO67-NATIVE-RING-BRIDGE-1A-ADMISSION-1X - bounded 1:1 snapshot admission

Baseline: 9937f291132fd59cccc3eaf5571a0c30ba07e968
Classification: PASS_NATIVE_SNAPSHOT_ADMISSION_1X

This is a developer-only admission experiment. Production AUTO67 was not
switched. The CPU history ring remains 4096 records; no native bridge, final
transport, Worker architecture, Cartographer, global map, resolver, frontier or
SOURCE_OWNED path was changed.

## Architecture invariant and static audit

The probe freezes every concrete runtime occurrence and never examines
usefulness, novelty, known/proven/duplicate/conflict state, branch fingerprints,
global-map state, Cartographer results or frontier state before Worker dispatch.
The existing Dispatcher remains the claim authority and its existing order is
preserved. The exact stored occurrence dispatch_state=LEASED guard remains
unchanged. Compatibility counter REJECT_ACTIVE_CLAIM is not consulted by
admission and remains obsolete.

The mechanical audit passed with:

    dispatcher_admission_forbidden_tokens=[]
    existing_order_is_preserved=true
    obsolete_active_claim_counter_in_admission=false
    snapshot_freeze_reads_only_identity_and_slot=true
    global_map_or_cartographer_in_admission=false
    semantic_filter_absent=true

## CPU history ring and snapshot pool

Continuous native recording remains enabled with the existing 4096-record
history ring. The diagnostic pool has exactly 16 immutable snapshot slots for
16 Workers, ratio 1.0x. It is not a pending-event FIFO. The existing
RollingWindow and existing Dispatcher are used for current context and
selection; the diagnostic pool stores only occurrence-scoped frozen metadata.

FREEZE occurs synchronously before the occurrence is handed to Dispatcher.
Each admitted occurrence carries (epoch, seq), occurrence_id,
snapshot_identity, snapshot_epoch, first_sequence, latest_sequence, and count.
The snapshot is a frozen value object. After FREEZE, the live ring may continue
recording without changing the retained snapshot.

A 16-occurrence burst produced this receipt:

| metric | value |
|---|---:|
| occurrences_seen | 16 |
| occurrences_frozen | 16 |
| snapshots_dispatched | 16 |
| snapshots_completed | 16 |
| snapshot_pool_capacity | 16 |
| current_snapshot_depth | 0 |
| peak_snapshot_depth | 16 |
| snapshot_pool_full_count | 0 |
| snapshot_pool_full_duration (s) | 0.0 |
| occurrences_without_snapshot | 0 |
| workers_busy current / peak | 0 / 16 |
| workers_idle current / minimum | 16 / 0 |
| worker_leases / returns | 16 / 16 |
| RollingWindow overwrites | 8 |

The occupancy histogram sampled depths 0..16 and reached depth 16. Every slot
was released only after its corresponding Worker return. The immutable
snapshot test mutated the source occurrence after FREEZE and observed identical
snapshot identity, epoch, sequence bounds and count. A deliberate seventeenth
freeze while all 16 slots were occupied returned no replacement and recorded
SNAPSHOT_POOL_FULL; no silent drop or overwrite occurred.

Occurrence age at Worker lease was measured for all 16 occurrences (p50 0 ms,
maximum 0.999 ms). Occurrence age at Worker start was measured for all 16
(p50 6.742 ms, maximum 6.742 ms). Frozen-snapshot age at Worker start was
measured for all 16 (p50 1.369 ms, maximum 1.369 ms).

## Current event-source sampling audit

live_opportunistic.lua remains unchanged. In continuous mode its exact
mechanical policy is write_stride=16 (the burst default is 1), with bounded
overwrite-oldest transport storage. Emitted occurrence classes are
RAM_WRITE_SAMPLE for every sixteenth bus-write callback and one FRAME_PC per
frame. This source sampling policy is not a semantic useful/not-useful filter.

## Classification and validation

The 1:1 gate passes: no semantic pre-worker selector, immutable occurrence
snapshot identity, exact 16-slot capacity, zero pool saturation, zero snapshot
capacity loss, and unchanged Dispatcher/Worker semantics. The 1.5x / 24-slot
experiment is not started.

Focused snapshot-admission tests: 10/10 PASS.
Debug CTest: 197/197 PASS. Release CTest: 197/197 PASS.
Source-limit: PASS; 674 governed files, all at or below 500 lines.
git diff --check: PASS.
SOURCE_OWNED: 1,475,600 / 3,145,728, delta 0; unchanged.
Debug and Release CTest, source-limit, git diff --check, and exact GitHub CI are
recorded at publication. The committed report keeps github_ci:
PENDING_EXTERNAL_VERIFICATION until the publication SHA has its external
receipt.

Stop after this checkpoint.
