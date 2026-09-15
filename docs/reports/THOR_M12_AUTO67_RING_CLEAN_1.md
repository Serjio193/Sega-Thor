# M12-AUTO67-RING-CLEAN-1 — Minimal current-event ring baseline

**Date:** 2026-09-15`n**Checkpoint implementation SHA:** `e562ab4``n**Baseline:** `f0101b754421915c996d982bed6b60e713c98477``n**Result:** PASS — ring cleanup accepted

## Python ring before/after

Before, `RollingWindow` contained `capacity`, `items`, `overwrites`, and the
obsolete `retained` counter. Its snapshot also exposed `retained` and
`max_utilization`, where the latter was only a duplicate of capacity; the class
also exposed an unused `current()` convenience method.

After, the class owns exactly:

```text
RollingWindow {
    capacity
    items: deque(maxlen=capacity)
    overwrites
}
```

`append()` evicts the oldest item automatically when full and increments only
`overwrites`. Snapshot output is exactly `capacity`, `utilization`, and
`overwrites`. No semantic state, proof retention, Worker result, novelty,
Cartographer state, frontier, or MAP delta is stored.

The exact stored item still carries `dispatch_state=LEASED`; this remains the
DISPATCHER-1 guard against redispatching the same current event. `epoch`, `seq`,
`occurrence_id`, and `window_item_id` are unchanged.

## Two-ring boundary

```text
BizHawk -> Lua transport ring -> status snapshot -> Python ingest
        -> Python RollingWindow -> Dispatcher -> Workers
```

The Lua `live_opportunistic.lua` ring remains a separate bounded transport
window across the file/poll boundary. It uses `ring_start`, `ring_count`,
`capacity`, and `overwritten`, overwrites oldest entries, preserves order, and
emits `epoch`, `seq`, and `occurrence_id`. The Python ring remains the
Dispatcher's current-event window. Neither ring is a pending-event FIFO or an
unbounded backlog; the predecessor/prehistory ring is outside this checkpoint.
No Lua code was rewritten because its existing contract is sound.

## Removed and retained fields

Removed exactly:

- `RollingWindow.retained` and `rolling_window.retained`;
- `rolling_window.max_utilization`;
- unused `RollingWindow.current()`;
- no replacement counter was introduced.

Retained compatibility data and consumers:

| Field | Consumer and reason |
|---|---|
| `dispatch_state=LEASED` | Dispatcher exact stored-item redispatch guard |
| `epoch`, `seq`, `occurrence_id` | Lua/Python occurrence identity and provenance |
| `window_item_id` | Dispatcher exact current-window item identity |
| `capacity`, `utilization`, `overwrites` | bounded ring status/dashboard and tests |
| Lua `ring_start`, `ring_count`, `overwritten` | transport ordering, utilization, overwrite diagnostics |

No semantic field is added to either live event ring.

## Focused proof A–F

`tests/thor_evidence_auto67_ring_clean_test.py` provides deterministic coverage:

| Test | Proof | Result |
|---|---|---|
| A | Python capacity 4, events 0..5 produce 2,3,4,5 and `overwrites == 2` | PASS |
| B | A leased item remains marked `LEASED`; another current item remains eligible | PASS |
| C | Sixteen same-branch occurrences remain independent with distinct identities | PASS |
| D | Ring object has only the three structural fields; no semantic state or `current()` | PASS |
| E | Lua transport fixture proves bounded count, oldest eviction, order, overwrite count, and occurrence identity; source audit confirms ring contract | PASS |
| F | Architecture/source audit proves Lua transport window and Python Dispatcher window stay separate with no backlog | PASS |

Worker-clean regressions and DISPATCHER-1 regressions pass unchanged. Focused
Ring + Worker-clean + AUTO67 + DISPATCHER-1 + capsule/materializer/persistence
coverage totals 50 tests, all passing.

## Validation

- Debug build: PASS (`build-current-debug`).
- Release build: PASS (`build-current-release`).
- Debug CTest: `192/192 PASS` (sequential rerun; a parallel run exposed the
  known flaky `oasis_re_m12_static_code_helpers`).
- Release CTest: `192/192 PASS`.
- Source-limit: PASS, 661 governed files ≤500 lines (inventory 673);
  `auto67_live.py` is 489 lines and the new test is 109 lines.
- `git diff --check`: PASS.
- No raw-event backlog or queue was added; Worker-clean and Dispatcher
  occurrence-only behavior remain intact.
- `SOURCE_OWNED` is unchanged: no manifest/ownership file is in the
  implementation diff, and no ROM, BIOS, extracted asset, or production runtime
  file was added. Existing untracked analysis/runtime artifacts remain unadded.
- GNU/Linux-equivalent build was not required for this Python-only ring cleanup;
  no CMake target, static library, link order, or portability code changed.
- Exact GitHub CI status for the checkpoint SHA: **UNAVAILABLE**.

The implementation commit is `e562ab4`; the report commit is the subsequent
publication commit. `HEAD == origin/main` is verified after publication.
