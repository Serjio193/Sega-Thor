# M12-AUTO67-LEGACY-CHAIN-CLEAN-1

**Date:** 2026-09-15
**Baseline:** `59cc7c311d8391d7f2c06eb00af5c8cd16243fa3`
**Classification:** `PASS_AUTO67_CARTOGRAPHER_ONLY_OUTPUT`

## Architecture change

Before:

```text
Worker → legacy descriptor → one queue → writer ├─ Cartographer
                                              └─ Store/live_chain
```

After:

```text
Worker → LOCAL CHAIN → one bounded queue → one writer → Cartographer → GLOBAL PROVEN MAP
```

The Worker now submits the local-chain object directly. The queue item has no
`chain_hash`, `canonical_payload`, `record_class` or `BOUNDED_UNRESOLVED` wrapper.
Observed and materialized local-chain diagnostics remain intact; Worker does not
classify or query the map.

`LiveMapSink` is the sole active output object. `--map-db` is the only durable
live-output option. Without it, no sink, queue or Cartographer is created and
bounded Worker diagnostics still complete normally. With it, Cartographer is
created, merged and closed by the single writer thread using normal SQLite
thread checks; snapshots return cached metrics only.

The old descriptor/Store implementation is retained only as
`auto67_legacy_persistence.py` for historical offline consumers:
`tests/thor_evidence_auto67_3_test.py`,
`tests/thor_evidence_auto67_3_identity_audit.py` and
`tests/thor_evidence_auto67_4_test.py`. It is not imported by the AUTO67 live
runner, Worker, window or map sink.

## Removed legacy surface

Removed from the active path: `--chain-db`, `--knowledge-db`, `Store`,
`begin_live_session`, `record_live_chain`, `live_session`, `live_chain`, legacy
throughput accounting (`set_runtime_leases`, `runtime_leases`,
`chains_per_1000_leases`), `chain_store` result naming and the obsolete
`known_or_merge_visible` visualization field. The active result/snapshot field
is `map_sink`.

## Preserved seam proof

The frozen R1 proof remains unchanged: the valid steps
`0x2234 --A5--> 0x27EC` and `0x27BE --A4--> 0x27EC` produce 3 unique
`ROM_INSTRUCTION` nodes and 2 `PROVEN` edges. Reversed step order, replay and a
different occurrence retain the same stable identity and zero durable replay
delta. A no-proof local chain is processed with `map_fragments_dropped == 0`;
actual bounded queue loss increments that counter once. No live frontiers are
created.

## Validation

- Focused AUTO67/MAP-1/mailbox/Worker/Dispatcher/Ring/transport/capsule/
  predecessor/materializer/Walker-1 tests: **103/103 PASS**.
- Debug CTest: **195/195 PASS**.
- Release CTest: **195/195 PASS**.
- Source-limit: PASS; **667** governed files, all ≤500 lines.
- `git diff --check`: PASS.
- SOURCE_OWNED: `1,475,600 / 3,145,728`, delta `0`; unchanged.
- GNU/Linux-equivalent build/link: **NOT_REQUIRED_PYTHON_ONLY**.
- Committed report CI: **PENDING_EXTERNAL_VERIFICATION**. The prior receipt
  `34965486208` is explicitly for baseline
  `335bb289fecd773740555f11c4b0a6a0a8554fe9`, not this checkpoint.

Implementation commit: `07fe196cb40fa9bbdde635998f473b2d68fcd854`.
