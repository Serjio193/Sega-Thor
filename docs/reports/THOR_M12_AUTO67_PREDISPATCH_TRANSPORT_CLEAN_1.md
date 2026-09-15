# M12-AUTO67-PREDISPATCH-TRANSPORT-CLEAN-1R1

**Date:** 2026-09-15  
**Baseline:** `7ad9249bbf6cc2098a249d336ab8f3eea960cf4b`
**Result:** PASS_AUTO67_PREDISPATCH_TRANSPORT

## Scope

The launcher previously used a process-local sequence-only `seen_sequence`
gate while consuming replaceable Lua status snapshots. The new
`PreDispatchTransport` owns that boundary and keeps only one cursor:
`(epoch, seq, occurrence_id)`. It accepts unseen records directly for
`Dispatcher.ingest`, counts and drops duplicate, stale, or malformed records,
and retains no raw event list or queue.

`events` is the current status key and `discovery` remains a legacy input alias.
The transport performs no known/proven/frontier lookup and makes no dispatch
claim. Lua transport rings, Python `RollingWindow`, Dispatcher occurrence
leasing, Worker-clean behavior, predecessor capture, CapsulePool, persistence,
Cartographer, MAP-1, Walker-1, AUTO68, C++, and SOURCE_OWNED are unchanged.

## Proof

The focused test covers replaceable snapshot deduplication, epoch rollover
with sequence restart, malformed/stale rejection, legacy key compatibility,
and the absence of raw backlog or semantic transport state. R1 adds final-only
ingest, periodic/final overlap, bounded shutdown after process exit, explicit
occurrence-id consistency, and the state-option audit. The existing 55 focused
regressions remain green; the combined focused run is **60/60 passed**.

## R1 acceptance

- `final_snapshot_ingested: true` — periodic `18,19,20` followed by final
  `19,20,21,22` reaches Dispatcher as `18,19,20,21,22`, exactly once.
- `final_only_occurrences_proven: true` — `21` and `22` are ingested through
  Dispatcher before the final dashboard publication; they are not UI-only.
- `shutdown_after_final_ingest: PASS` — final consume → Dispatcher ingest →
  `dispatcher.stop`; existing stop event releases a worker waiting on a
  capsule, and all worker threads terminate within the bounded test window.
- `state_option_audit: REMOVED` — `live_opportunistic.lua` has no state
  consumer, so this runner no longer exposes `--state` or exports
  `OASIS_LIVE_STATE`. No savestate loading was added.
- `explicit_occurrence_id_consistency: PASS` — an explicit mismatch is
  rejected as `INVALID`; legacy records synthesize `epoch=<epoch>:seq=<seq>`.

## Validation

- Windows Debug build: PASS.
- Windows Release build: PASS.
- Debug CTest: **193/193 PASS**.
- Release CTest: **193/193 PASS**.
- Source-limit: PASS; all governed source files are ≤500 lines. The new
  transport module is 105 lines and the focused test is 69 lines.
- `git diff --check`: PASS.
- SOURCE_OWNED: unchanged at `1,475,600 / 3,145,728`; delta `0`.
- No ROM, BIOS, savestate, extracted asset, production runtime, or raw-event
  backlog was added.
- Exact remote CI receipt: **UNAVAILABLE**.
- GNU/Linux-equivalent build/link: not required; this change is Python-only
  and does not alter CMake targets, static libraries, link order, or portability
  code.

Implementation SHA: `19f9f84`. The report is a subsequent publication
commit; its final SHA is recorded in the JSON after publication.
