# HISTORICAL — THOR M12 AUTO67.3 — persistent knowledge and real-yield proof

Authoritative final checkpoint: `THOR_M12_AUTO67_3_FINAL_CANONICAL_CHAIN_IDENTITY_CHECKPOINT.md`.

Date: 2026-09-13. Requested base `124c80c0a1c3cef95f7d646f3a2c35ceae472b74`; tested checkout `f95cc7df1fba523527a53dfaf53dc886439bc50e` on `main`.

## Result

PASS for the bounded AUTO67.3 objective. Real BizHawk ran with 16 prestarted workers and the native operator window. The 4800-frame proof run produced 1162 leases and 1162 returns, peak 16 busy/working, and ended with zero busy workers. The bounded persistence queue submitted and drained 1162 descriptors with zero drops. The machine-readable proof is [THOR_M12_AUTO67_3_PERSISTENT_KNOWLEDGE_YIELD.json](THOR_M12_AUTO67_3_PERSISTENT_KNOWLEDGE_YIELD.json).

## Why AUTO67.2 yield was mechanically inflated

The previous session ledger used `event.seq` in its observation key, inserted a generic branch/context/predecessor/successor edge for each return, inferred roots from first-seen branches, and incremented `metrics.new_edges` unconditionally in the worker completion path. Therefore lease count could track “new” contexts/edges even when no durable relation was proven.

AUTO67.3 separates runtime observations, session contexts, persistent observations and proof obligations. Durable identity uses `kind/pc/address` and optional meaningful role fields. `frame`, `epoch` and `seq` are provenance only. No relation is persisted without validated endpoints and a witness; this run therefore reports relations, structures and promotion candidates as zero.

## Real proof metrics

| Metric | Result |
|---|---:|
| Session duration / frames | 82.781 s / 4800 |
| Workers / peak busy | 16 / 16 |
| Leases / returns | 1162 / 1162 |
| Known returns / merged returns | 0 / 0 |
| Active collisions / visible merges | 50 / 50 |
| Duplicate active claims | 0 |
| Runtime observations | 15040 |
| New session contexts | 1162 |
| Persistent observations inserted / deduped | 374 / 788 |
| New proof obligations | 374 |
| Investigations advanced / closed | 0 / 0 |
| Relations / structures / promotion | 0 / 0 / 0 |
| Rolling window | 256 capacity, 14784 overwrites |
| Seed age average / maximum | 4.052 / 16 ms |
| Raw event backlog | 0 / `NONEXISTENT` |

The same-state QuickSave1 replay used the same sidecar twice: run 1 had 209 persistent inserts and 92 deduped results; run 2 had 40 inserts and 193 deduped results, with equal 3760 observed events and zero queue drops. A different existing QuickSave4 produced 5 new persistent observations and 136 deduped results. The difference is expected from the bounded lossy rolling-window sampling; it does not turn temporal fields into durable identity.

## Worker lifecycle proof

The bounded native-window history contains real cycles for W00, W01, W02 and W03. Each has `IDLE → LEASED → WORKING → RETURNING → IDLE`; W00 and W01 also receive later fresh leases on different chain fingerprints. Returned result in this run is `BOUNDED_UNRESOLVED`, and the worker is released regardless of that result. Active collision/merge rejection is visible while no duplicate active claim exists.

## Dispatch and frame evidence

Dispatch `T0→T8` was min/p50/p95/p99/max **16/46/1016/1167/2032 µs**. Claim-lock hold was **5/9/40/69/114 µs**. Individual stage values are in the JSON proof. Frame timing recorded 2896 frames over 16 ms, one over 33 ms and none over 50 ms; the largest was 35 ms at frame 4261 with no active lease. This is not a claim that every 16–33 ms frame is dispatch-caused; the >33 ms outlier had no active lease.

No commit or push was performed. AUTO68 was not started and no SOURCE_OWNED promotion was performed.
