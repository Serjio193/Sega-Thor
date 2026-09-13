# HISTORICAL — THOR M12 AUTO67 — persistent chain store first

Authoritative final checkpoint: `THOR_M12_AUTO67_3_FINAL_CANONICAL_CHAIN_IDENTITY_CHECKPOINT.md`.

Status: **PASS for the bounded persistent-store acceptance proof.**

The AUTO67 acceptance path no longer uses the in-memory knowledge-yield
analyzer or seed/context classifications. Before worker execution it rejects
only an active claim and the normal already-dispatched rolling-window token.
Every completed worker result becomes a canonical chain descriptor and is sent
through a bounded nonblocking queue to a background transactional writer in the
existing Evidence Engine SQLite sidecar.

## Replay proof

Three real BizHawk 2.11.1 runs used 16 prestarted workers and the native
operator window:

| Run | Frames | Leases/returns | Unique inserts | Exact duplicates | Queue drops | Max frame |
|---|---:|---:|---:|---:|---:|---:|
| QuickSave1, run 1 | 1200 | 3368/3368 | 214 | 3154 | 0 | 32 ms |
| QuickSave1, run 2, same DB | 1200 | 3353/3353 | 39 | 3314 | 0 | 32 ms |
| QuickSave4, same DB | 1200 | 3346/3346 | 11 | 3335 | 0 | 37 ms |

The second same-scenario run rejected 98.84% of completed results as exact
duplicates. The different savestate added new hashes, proving that the store
does not reject all later work. The final sidecar contains 264 unique
unresolved chain records across three sessions; no rooted chain was fabricated.
Reopening the DB after the processes exited found zero orphan chain records and
maximum `times_observed=705`.

Canonical identity is SHA-256 over observed causal facts and optional explicit
chain steps/frontier/root. Wall-clock time, worker, lease, session, frame,
epoch and sequence remain provenance and cannot change the chain hash. Similar
chains are not semantically merged. An unresolved chain is retained as a valid
record. Old AUTO67/AUTO67.1/AUTO67.2 artifacts contained no complete canonical
worker-chain bodies, so historical import recovered and imported **0** records.

## Runtime health

All three runs returned successfully. Each had zero duplicate active claims,
zero queue drops, zero DB write errors and a nonexistent raw-event backlog.
Peak busy workers was 16. The largest observed frame was 37 ms, with zero
frames over 50 ms. Seed age stayed below 32 ms maximum. Capsule reuse remained
bounded and active capsules were released.

The native operator displays runtime health and objective chain-store growth:
unique chains, session new, exact duplicates, writes/errors, unresolved and
rooted. It does not display `PRODUCTIVE`, `NEW_ROOT` or `NEW_CONTEXT`.

## Validation and scope

`tests/thor_evidence_auto67_test.py`, `tests/thor_evidence_auto67_1_test.py`
and `tests/thor_evidence_auto67_3_test.py` pass (`22/22`). The persistent
writer is outside the BizHawk callback path; the callback publishes only the
existing bounded runtime snapshot. AUTO68, SOURCE_OWNED promotion, semantic
merge and dispatch redesign were not started. No commit or push was performed.

Machine-readable evidence is in
`THOR_M12_AUTO67_PERSISTENT_CHAIN_STORE_FIRST.json`.
