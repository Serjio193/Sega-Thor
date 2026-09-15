# M12-AUTO67-PREDISPATCH-TRANSPORT-CLEAN-1

**Date:** 2026-09-15  
**Baseline:** `466736e6ee1a4a45f957b423ff08f3499cdc3a04`  
**Implementation SHA:** `e54bf12`  
**Result:** PASS — bounded pre-dispatch transport cleanup

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
and the absence of raw backlog or semantic transport state. Ring-clean,
Worker-clean, AUTO67, DISPATCHER-1, capsule, materializer, and persistence
regressions remain green: **55 tests passed**.

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

The implementation SHA is `e54bf12`; the report is a subsequent publication
commit. After publication, `HEAD == origin/main` is verified.
