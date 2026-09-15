# M12-AUTO67-CARTOGRAPHER-SEAM-1R1

**Date:** 2026-09-15
**Baseline:** `335bb289fecd773740555f11c4b0a6a0a8554fe9`
**Classification:** `PASS_AUTO67_CARTOGRAPHER_SEAM`

## Scope

This revision repairs only the remaining seam correctness issues. The upstream
`CPU → Lua ring → PreDispatchTransport → RollingWindow → Dispatcher → mailbox →
Worker` path, local-chain Worker role, proof contract type, CapsulePool,
predecessor resolver, MAP-1 identity semantics, Walker-1 and SOURCE_OWNED are
unchanged.

## Complete local-chain ingest

`candidate_bundle()` scans every `chain_steps` entry, retains every valid
`REGISTER_REACHING_DEFINITION_V1` step, deduplicates nodes and emits all stable
edges in deterministic order. Invalid steps remain in the Worker local-chain
diagnostics and do not block valid siblings.

The two-step proof contains:

- `0x2234 --A5--> 0x27EC`
- `0x27BE --A4--> 0x27EC`

The candidate contains **3 unique `ROM_INSTRUCTION` nodes** and **2 `PROVEN`
edges**, with relations `REGISTER_REACHING_DEFINITION:A4` and
`REGISTER_REACHING_DEFINITION:A5`.

The stable hash covers the complete sorted accepted dependency set. Runtime
occurrence, frame, lease, worker and investigation metadata are excluded. The
same A4+A5 set in reversed step order and a different runtime occurrence yields
the same bundle, hash and `auto67-live:<stable_bundle_hash>` import reference.
Exact replay has all durable delta fields zero, unchanged graph hash and one
`map_import` row; the different occurrence also has zero durable delta.

## Drop semantics and thread ownership

A no-proof local chain increments `chains_without_accepted_proof` and leaves
`map_fragments_dropped` at zero. Only actual bounded queue loss increments
`map_fragments_dropped`; the queue-full regression produces one local-chain drop
without adding overflow storage. Normal acceptance runs report zero dropped map
fragments.

`LivePersistenceSink` now receives only `map_db` and ROM SHA. Its sole writer
thread creates, uses, merges and closes Cartographer. Cartographer uses normal
SQLite thread checks. `snapshot()` returns cached graph hash and map metrics
under the existing sink lock and performs no Cartographer or SQLite query. There
is still exactly one bounded queue and one writer thread. Map-only and
map-plus-legacy-chain modes both pass.

## Reported proof fields

- `final_snapshot_ingested: true`
- `final_only_occurrences_proven: true`
- `shutdown_after_final_ingest: PASS`
- `state_option_audit: REMOVED`
- `explicit_occurrence_id_consistency: PASS`

## Tests and validation

- Cartographer seam tests: **18/18 PASS**.
- Focused AUTO67/MAP-1 regressions: **101/101 PASS**.
- Windows Debug CTest: **195/195 PASS**.
- Windows Release CTest: **195/195 PASS**.
- Source-limit: PASS; **666** governed files, all at or below 500 lines.
- `git diff --check`: PASS.
- SOURCE_OWNED: `1,475,600 / 3,145,728`, delta `0`; unchanged.
- GNU/Linux-equivalent build/link: **NOT_REQUIRED_PYTHON_ONLY**.
- GitHub CI in this committed report: **PENDING_EXTERNAL_VERIFICATION**; the
  prior receipt is intentionally not rewritten into a new SHA. The previous
  SEAM-1 receipt `34965486208` belongs to baseline SHA
  `335bb289fecd773740555f11c4b0a6a0a8554fe9`.

Implementation commit: `509d815a2f087a6d8560481013f5ebbe5837189b`.
