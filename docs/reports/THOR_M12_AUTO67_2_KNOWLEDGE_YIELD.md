# HISTORICAL — THOR M12 AUTO67.2 — Knowledge yield / anti-churn live proof

Authoritative final checkpoint: `THOR_M12_AUTO67_3_FINAL_CANONICAL_CHAIN_IDENTITY_CHECKPOINT.md`.

## Result

The real BizHawk run completed successfully with 16 prestarted workers and the
native operator window. It ran 18,000 frames for 308.031 seconds and produced
4,356 leases and 4,356 returns. The measured session classification is
`PRODUCTIVE` under the bounded session-ledger rule: 4,356 useful leases, zero
no-gain leases, and 100% useful in the last 100, last 1,000, and whole-session
windows.

This is session knowledge accounting, not a persistent Knowledge DB result:
the current AUTO67 worker path has no connected persistent Knowledge DB. The
machine proof therefore reports `persistent_db_delta_available: false` and
does not claim durable database promotion.

## Acceptance metrics

| Metric | Evidence |
|---|---:|
| Session / frames | 308.031 s / 18,000 |
| Workers / peak busy | 16 / 16 |
| Leases / returns | 4,356 / 4,356 |
| Exact unresolved duplicate rejects before worker | 51,701 |
| Active collision/merge rejections | 215 / 215 |
| Duplicate active claims | 0 |
| Rolling-window overwrites | 56,016 |
| Average / maximum seed age | 4.026 ms / 16.000 ms |
| Raw backlog | 0 / `NONEXISTENT` |
| Emulator | PASS: return code 0 and responsive during run |

All completed worker results were `BOUNDED_UNRESOLVED`; the yield ledger
classified them as 1,016 `NEW_ROOT` and 3,340 `NEW_CONTEXT`. There were no
`KNOWN_COMPLETE`, `MERGED_NO_NEW_FACT`, `INVESTIGATION_ADVANCED`, `PROVEN`, or
`CONFLICT` worker returns in this capture. Merge activity was visible as 215
pre-dispatch active-collision rejections.

## Worker lifecycle proof

The global bounded transition ring retained 500 real transitions. Consecutive
duplicate states are compressed only for checking the pattern; no transition
is simulated. Four workers have the required complete lifecycle:

- W00: `LEASED → WORKING → RETURNING → IDLE` on chain
  `c413be6a1ac32d06a33f219d`, then the same cycle on fresh chain
  `9bb5a2352d7bb641259563c0`; two cycles are present.
- W01: `LEASED → WORKING → RETURNING → IDLE` on fresh chain
  `dc89dd24e2042e4a15d18995`.
- W02: `LEASED → WORKING → RETURNING → IDLE` on fresh chain
  `6500b6ce1366c2bd896677f4`.
- W03: `LEASED → WORKING → RETURNING → IDLE` on fresh chain
  `30cfef0041754e9cc3109616`.

The operator view receives the same replaceable snapshot and shows worker id,
state, stage, result, yield class, chain, seed age, and bounded recent
transitions. It has no raw-event backlog and cannot claim work.

## Dispatch and frame observations

Dispatch stage statistics are in microseconds and are copied verbatim into the
JSON proof. The aggregate T0→T8 path was min/p50/p95/p99/max
`18/185/2374/7665/10438`; claim-lock hold was
`0/16/76/128/292`. T7→T8 worker start was
`3/42/2038/7606/10303`.

The run recorded 10,324 frames over 16 ms, 32 over 33 ms, and 4 over 50 ms.
The largest measured frame was 3,683 ms at frame 5,686 with no associated
lease. This proof therefore demonstrates worker recycling and anti-churn
accounting, but it does not claim that the remaining frame-time outlier is
caused by dispatch; its recorded lease association is empty.

## Files and validation

- Machine-readable proof: `docs/reports/THOR_M12_AUTO67_2_KNOWLEDGE_YIELD.json`
- Full runtime artifact: `build/auto67-knowledge-yield-final.json`
- Native operator snapshot: `build/auto67-knowledge-yield-final.view.json`
- Added bounded session ledger: `src/tools/thor_evidence/auto67_yield.py`
- Regression: `19/19` AUTO67 tests passed; Python compilation and `git diff --check` passed.
- No commit or push was made.
