# M12-AUTO67-WORKER-CLEAN-1 — Minimal bounded evidence Worker baseline

**Date:** 2026-09-15`n**Baseline:** `3ee9bc807165aee54f90ff9f7575209713e40204``n**Result:** PASS — bounded Worker cleanup accepted

## Scope and acceptance

The change is limited to the AUTO67 Worker path. Existing DISPATCHER-1
semantics remain the contract: current `RollingWindow`, exact stored-item
`dispatch_state=LEASED` guard, source `epoch + seq` occurrence identity plus
`window_item_id`, one mailbox per Worker, 16 Workers as the supported scale,
`CAPSULE_COUNT=16`, `MAX_LIVE_CAPTURES=4`, and no raw-event backlog. Cartographer,
MAP-1, Walker-1, AUTO68, SOURCE_OWNED, C++, hooks, queues, and seed identity
were not redesigned.

## Before and after graphs

```text
BEFORE (baseline)
Lua source -> RollingWindow -> Dispatcher occurrence lease -> one mailbox
  -> Worker -> unbounded investigations{} + known_during_work/status decisions
  -> materializer with unresolved_frontier/next -> optional persistence
  -> capsule release(result) carrying known/merged/proven/bounded_unresolved
```

```text
AFTER (this checkpoint)
Lua source -> bounded current RollingWindow -> Dispatcher occurrence lease
  -> one mailbox -> Worker factual capture/decode/materialize outcome
  -> recent_investigations[16] + factual register/capture diagnostics
  -> optional downstream persistence (occurrence provenance preserved)
  -> capsule release(capsule_id), resource state only
```

## Code delta

Removed:

- `Dispatcher.investigations` unbounded map and full-snapshot expansion;
- Worker `known_during_work` override and Worker global KNOWN/PROVEN/
  DUPLICATE/MERGED/blocked/exhausted decisions and counters;
- Capsule semantic fields `known`, `merged`, `proven`, `bounded_unresolved`;
- unused `known_early_release` and `merge_early_release` metrics;
- live persistence serialization of `unresolved_frontier` and its prescriptive
  `next` field; the historical non-live materializer contract remains readable.

Retained:

- bounded `recent_investigations` (`deque(maxlen=16)`), occurrence/lease/
  `window_item_id` provenance, and bounded worker transition history;
- O67V capsule validation, predecessor decoding, static M68K MOVE decoding,
  `required_registers`, observed-versus-causal separation, fail-closed decode,
  and `chain_steps` only where predecessor evidence proves them;
- `CAPSULE_COUNT=16`, `MAX_LIVE_CAPTURES=4`, temporary capsule release and the
  existing one-mailbox Dispatcher path;
- downstream canonical semantic deduplication in the existing SQLite sidecar.

Worker outcomes are factual: `EVIDENCE_OBSERVED`, `EVIDENCE_CAPTURED`,
`CAPTURE_UNAVAILABLE`, `DECODE_FAILED`, and `EVIDENCE_MATERIALIZED`. Persistence
continues to use compatibility status `BOUNDED_UNRESOLVED`; it is not a Worker
semantic verdict. Sink errors are counted and cannot prevent Worker return to
`IDLE` or capsule release.

## State, frontier, and capsule field audit

| Area | Before | After |
|---|---|---|
| Worker history | `investigations{}` grew with completions; full snapshot copied it | `recent_investigations` max 16; both snapshots bounded |
| Worker status | input `known_during_work` could replace factual result | ignored for semantics; factual `outcome` only |
| Live unresolved data | materialized `unresolved_frontier` with `next` | `register_provenance` status/reason/unresolved plus `capture_diagnostics.missing_evidence`; no `next` |
| Capsule fields | semantic flags plus resource state | resource/capture metadata only; `release(capsule_id)` |
| Persistence | optional downstream writer, frontier serialized | same writer and queue; no new live frontier payload; occurrence provenance retained |

An unresolved live result is therefore diagnostic evidence, not a future task or
frontier scheduling instruction. Proven static causal facts remain durable in
the materialized payload.

## Tests A–G and regressions

`tests/thor_evidence_auto67_worker_clean_test.py` covers:

| Test | Evidence | Result |
|---|---|---|
| A | 40 completions leave no `investigations` attribute and exactly 16 retained entries | PASS |
| B | two `known_during_work` events both complete as `EVIDENCE_OBSERVED`; no Worker known metric | PASS |
| C | capsule release resets only resources, omits semantic fields, and reuses the slot | PASS |
| D | unsupported static decode keeps factual diagnostics, omits frontier and `next` | PASS |
| E | proven `INSTRUCTION_SOURCE_MEMORY` causal fact survives live materialization | PASS |
| F | identical branch occurrences receive distinct occurrence/window identities and leases | PASS |
| G | failing persistence sink cannot block Worker return/release; error is counted | PASS |

Existing AUTO67, DISPATCHER-1, capsule, materializer, and persistence suites
also pass. The focused set is 38 tests. Full CTest has 191/191 PASS in both
Debug and Release. The after-run bounded snapshot measurement for 40 completions
was 72,266 JSON bytes with 16 investigations; Python traced current/peak memory
was 150,348/150,692 bytes. The baseline map had O(completions) retained
investigation memory; the new Worker retention is O(16 + bounded transitions,
window, and capsule metadata).

## Validation and boundaries

- Debug configure/build: PASS (`build-current-debug`).
- Release configure/build: PASS (`build-current-release`).
- Debug CTest: PASS, 191/191.
- Release CTest: PASS, 191/191.
- `cmake -P tests/check_file_limits.cmake`: PASS, 660 governed files ≤500
  lines (inventory 672); `auto67_live.py` is 496 lines and the new test 163.
- `git diff --check`: PASS.
- SOURCE_OWNED: unchanged; no ROM, BIOS, extracted asset, or production/runtime
  file was added. Existing untracked runtime/analysis artifacts remain unadded.
- GNU/Linux-equivalent build/link: not required for this Python-only behavior
  change; no CMake target, static library, link order, or portability code was
  changed. Exact remote CI receipt: **UNAVAILABLE** in this environment.
- Cartographer and MAP-1 are deliberately not connected to this Worker cleanup.

The next step is downstream review of the bounded factual records; this
checkpoint does not promote ownership or claim chain closure.
