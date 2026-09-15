# M12-AUTO67-MAILBOX-CLEAN-1

**Date:** 2026-09-15  
**Baseline:** `98de373f6f765c688433d78ba78451b22eded1a4`  
**Classification:** `PASS_AUTO67_MAILBOX_CLEAN`

## Scope

This checkpoint repairs only the Dispatcher-to-Worker lease message. Current
selection order, exact `dispatch_state=LEASED` guard, one mailbox per worker,
prehistory ordering, CapsulePool limits (16 total / 4 live), RollingWindow,
predecessor ring, persistence architecture, Cartographer, MAP-1, Walker-1,
SOURCE_OWNED and C++ are unchanged. No queue, retry backlog, scheduler or
capture mechanism was added.

## Mailbox contract

Before, the task carried `event`, `branch`, `context`, `occurrence_id`,
`worker_id`, `seed`, `assigned_ns`, `dispatch_trace`, `investigation_id`,
`capsule_id`, `capsule_lease`, `capture_status` and `lease_id`.

After, the task carries exactly the required core fields:

```text
event, investigation_id, lease_id, dispatch_trace
```

`capsule_id` and `capture_status` remain optional when a capsule is involved.
`event` is created with `dict(event)` and is the authoritative detached copy
of `epoch`, `seq`, `occurrence_id` and `window_item_id`. Mailbox-level
`occurrence_id`, `worker_id`, `seed`, `assigned_ns`, `branch`, `context` and
`capsule_lease` were removed. Worker ID remains in worker diagnostics, capsule
metadata and persistence provenance, where it has a concrete consumer.

`investigation_id` is `INV-AUTO67-` plus a bounded digest of only
`occurrence_id` and `window_item_id`. The Worker reads `task["investigation_id"]`
directly; it does not recompute an ID. The one `lease_id` is either the
claimed capsule lease or the no-capsule lease generated for that occurrence.
The Worker passes that same value to capsule wait, decode, predecessor decode
and persistence. Capsule ownership checks and fail-closed mismatch behavior
remain in CapsulePool.

Worker history now records factual `investigation_id`, `occurrence_id`,
`window_item_id`, `lease_id`, `worker_id`, `outcome` and bounded materialization
counts. Branch/context and seed sequence are not mailbox or Worker semantics.
Dispatch profiler labels were clarified to `T0_T1_candidate_identity_us` and
`T3_T4_event_copy_us`; timestamp boundaries are unchanged.

## Required proofs

- `final_snapshot_ingested: true` — the existing shared PreDispatchTransport
  consumes periodic `18,19,20`, then final `19,20,21,22` before
  `dispatcher.stop()`. Dispatcher receives `18,19,20,21,22` exactly once;
  `21` and `22` arrive through `dispatcher.ingest`, not dashboard publication.
- `final_only_occurrences_proven: true` — transport acceptance and Dispatcher
  observation both include final-only occurrences 21 and 22.
- `shutdown_after_final_ingest: PASS` — after final ingest with the emulator
  stopped, `dispatcher.stop()` sets the existing stop event, releases capsule
  wait, joins all workers within the bounded test window, and leaves no live
  worker thread.
- `explicit_occurrence_id_consistency: PASS` — PreDispatchTransport accepts
  only `occurrence_id == epoch=<epoch>:seq=<seq>`; explicit mismatch is counted
  `INVALID` and dropped. Legacy records without the field synthesize it.
- Same-branch occurrences remain independent through the existing 16-worker
  regression and exact stored-item guard.

## Tests and validation

New mailbox tests cover minimal shape, identity-preserving detached copy,
single investigation/lease propagation through Worker and persistence,
capsule mismatch rejection, no backlog/history map, state-option audit and
shutdown after final ingest. The focused AUTO67 set is **67/67 PASS** (60
pre-existing transport/worker/dispatcher regressions plus 7 mailbox tests),
including the required 16 same-branch occurrence test.

- Windows Debug build: PASS.
- Windows Release build: PASS.
- Debug CTest: **194/194 PASS**.
- Release CTest: **194/194 PASS**.
- Source-limit: PASS; **664** governed files, all at or below 500 lines.
- `git diff --check`: PASS.
- SOURCE_OWNED: `1,475,600 / 3,145,728`, delta `0`; unchanged.
- GitHub CI receipt: **UNAVAILABLE**.
- GNU/Linux-equivalent build/link: **NOT_REQUIRED_PYTHON_ONLY**; no CMake
  target, library, link-order or portability code changed.

The implementation and report are committed and pushed after local validation.
