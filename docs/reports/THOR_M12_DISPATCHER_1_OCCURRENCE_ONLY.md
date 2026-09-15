# THOR M12 DISPATCHER-1 — occurrence-only leasing

Status: **PASS_DISPATCHER_OCCURRENCE_ONLY**

Baseline: `92a33c4f71a125a09cc26fc31ae2c14cf07a74a8`

The AUTO67 Dispatcher no longer suppresses a current event because another
leased event has the same `kind + address + pc` branch fingerprint.
`Dispatcher._choose_current` still skips an exact stored window item after its
`dispatch_state` becomes `LEASED`, and the existing one-mailbox-per-worker
claim authority remains unchanged.

Before, one active branch fingerprint caused `REJECT_ACTIVE_CLAIM`, so a
second occurrence was marked merged before a worker saw it. After, two
occurrences with the same fingerprint are independent leases when workers are
free. Branch fingerprints remain diagnostic context only. The compatibility
fields `REJECT_ACTIVE_CLAIM`, `active_collisions`, `duplicate_active_claims`,
and `investigation_merges` are obsolete and are no longer incremented.

Runtime identity is the source-provided monotonic `epoch + seq` pair from
`capture/live_opportunistic.lua`. The Dispatcher adds a bounded
`window_item_id` for the exact stored RollingWindow object. Investigation IDs,
lease IDs, worker mailboxes, and persistence descriptors carry that occurrence
identity; no causal or semantic identity is inferred.

Focused deterministic tests:

| Test | Result |
|---|---|
| A — same branch, different occurrences | PASS: seq 100/101 leased to workers 0/1 |
| B — same stored event once | PASS: `dispatch_state=LEASED` blocks redispatch |
| C — busy worker | PASS: a WORKING worker keeps its mailbox |
| D — 16 workers | PASS: 16 same-fingerprint occurrences lease independently |
| E — identity | PASS: occurrence, investigation, lease, mailbox, and persistence descriptor identities differ |

Capsule resources remain unchanged: `CAPSULE_COUNT=16` and
`MAX_LIVE_CAPTURES=4`. When the four live slots are occupied, the existing
`WAITING_CAPTURE_SLOT` bounded fallback applies; it is not a branch rejection.
The Dispatcher has one mailbox per worker and no raw-event backlog or pending
event queue. `raw_event_backlog_structure` remains `NONEXISTENT`.

Regression coverage in `tests/thor_evidence_auto67_test.py` was updated for
occurrence-only behavior. New focused coverage is in
`tests/thor_evidence_dispatcher1_test.py`, registered as
`oasis_re_thor_evidence_dispatcher1_helpers`.

No Cartographer, MAP-1, Walker-1, frontier scheduling, AUTO68, SOURCE_OWNED,
C++, emulator hook, capsule limit, or raw-backlog change was made.

Validation: focused AUTO67/Dispatcher tests pass (`16/16`); Debug and Release
builds pass; Debug and Release CTest pass (`190/190` each); the source-size
gate and `git diff --check` pass.
GitHub CI for the final commit is **UNAVAILABLE** in this checkpoint because
the repository does not expose a CI result through the local task.
