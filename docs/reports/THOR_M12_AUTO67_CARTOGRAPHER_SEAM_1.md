# M12-AUTO67-CARTOGRAPHER-SEAM-1

**Date:** 2026-09-15  
**Baseline:** `199c8d43454d761a2000026a83c20a06f7c4d00d`  
**Classification:** `PASS_AUTO67_CARTOGRAPHER_SEAM`

## Scope and boundary

This checkpoint connects the existing AUTO67 worker result to the existing MAP-1
Cartographer. The upstream path remains `CPU → Lua ring → PreDispatchTransport →
RollingWindow → Dispatcher → one mailbox → Worker`; no Worker global lookup or
classification was added. The Worker now emits one bounded
`oasis.m12.auto67.local-chain.v1` result. The downstream proof gate is the only
place that can create a MAP-1 bundle.

The Worker result contains the occurrence, bounded chain steps, observations,
causal facts, register provenance and capture diagnostics, plus the existing
`investigation_id` and `lease_id`. It contains no `PROVEN`, `NEW`, `DUPLICATE`,
`KNOWN`, `CONFLICT` or `MAP_DELTA` decision.

## Proof gate and durable identity

Only `REGISTER_REACHING_DEFINITION` is supported. A step is accepted only when
producer and consumer PCs are valid, the register is `A4` or `A5`, both
producer/consumer occurrences are present in the same epoch, and
`evidence.complete_interval == true` with
`evidence.intervening_register_write == false`. Temporal adjacency and runtime
observations cannot promote a node.

An accepted step emits two global `ROM_INSTRUCTION` nodes and one
`REGISTER_REACHING_DEFINITION:<register>` edge, all `PROVEN`. Node and edge IDs
are content-addressed from stable ROM PCs/register/scope. Runtime occurrence,
frame, lease, worker and investigation IDs are absent from durable identity.
Lineage is `AUTO67_LOCAL_CHAIN` plus the proof contract/schema and stable
fingerprint. The import identity is `auto67-live:<stable_bundle_hash>`.
Bundles contain no frontiers and do not create live frontiers.

## Merge evidence

The synthetic MAP-1 probe and seam tests produced the following deterministic
results:

| case | delta | graph hash / import |
|---|---|---|
| first valid A5 (`0x002234 → 0x0027EC`) | `new_nodes=2`, `new_edges=1`, promotions/conflicts/frontiers `0` | `fd3dd919d1100c9168706eedd6b3a19a768948f31c2356ffb930956ba52dc896`; `auto67-live:deaaffd8f44ce13ce90a2f87abb41beb3ab45ba3362b65465690f4e55bb42683` |
| exact replay | all delta counts `0` | same graph hash |
| same chain, different occurrence | stable hash/bundle unchanged; zero map delta | no occurrence bloat |
| distinct A4 relation | `new_nodes=0`, `new_edges=1` | `fcb90c7ed1ed1620b04c7ed70d394ab2c750a0e25e45f32850a2788164cc491a` |
| incomplete/intervening/malformed/unresolved | no candidate; no promotion | no frontiers |

Ten repeated occurrences of one stable chain leave exactly two nodes, one edge
and one `map_import` row. Map-only operation (`--map-db` without `--chain-db`) and
map-plus-legacy compatibility both pass. The existing `LivePersistenceSink`
remains the single bounded queue and single `auto67-chain-writer` thread; map
merge and legacy `live_chain` persistence share that writer.

## Tests and validation

- New Cartographer seam tests: **11/11 PASS** (local Worker result, proof accept/reject, first merge/replay, occurrence stability, distinct knowledge, unproven retention, no bloat/frontiers, map-only/compatibility, Worker boundary and one queue/thread).
- Focused AUTO67/MAP-1 regressions: **94/94 PASS**, including mailbox, Dispatcher-1, Worker, Ring, PreDispatchTransport and Walker-1 coverage.
- Windows Debug CTest: **195/195 PASS**.
- Windows Release CTest: **195/195 PASS**.
- Source-limit: PASS; **666** governed files, all at or below 500 lines.
- `git diff --check`: PASS.
- SOURCE_OWNED: `1,475,600 / 3,145,728`, delta `0`; unchanged.
- GNU/Linux-equivalent build/link: **NOT_REQUIRED_PYTHON_ONLY**; no CMake target, link order or portability-sensitive native code changed.
- GitHub CI receipt: **UNAVAILABLE**.

Implementation commit: `a5134a6`.
