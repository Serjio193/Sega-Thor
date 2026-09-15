# THOR M12 AUTO67 RAM SESSION MAP 1

- Classification: `PASS_AUTO67_RAM_SESSION_MAP`
- Baseline: `20da49279e09ee2f1c5954aa0c4cae8d77e3d6c3`
- Implementation: `874250f` (report publication follows)
- Scope: existing bounded Worker → LiveMapSink queue → one writer; no capture, Dispatcher, Worker, Cartographer identity, capsule, predecessor, C++, MAP-1 or SOURCE_OWNED redesign.

## Acceptance

`LiveMapSink` now creates `Cartographer.in_memory()` and never opens GLOBAL during runtime. On the existing bounded shutdown join it backs up the RAM database to the retained session SQLite through `<session>.tmp` and fsync/atomic replace. `auto67_runner.py` reads `lua_final`, ingests `transport.consume(lua_final)` before `dispatcher.stop()`, stops the map writer, then calls the offline merger.

The offline merger copies an existing GLOBAL (or creates an empty temporary graph), imports the session under `session-map:<session_graph_hash>`, validates the resulting graph hash, fsyncs `global.merge.tmp.sqlite`, and atomically replaces GLOBAL. A failed replace or validation removes only the temporary copy; the original GLOBAL remains byte-for-byte unchanged.

Required final-only transport proof:

```
periodic: epoch=1 seq 18,19,20
final:    epoch=1 seq 19,20,21,22
Dispatcher ingest: 18,19,20,21,22
```

The overlap 19/20 is rejected by the single `PreDispatchTransport` cursor; 21/22 are returned by `consume(lua_final)` and ingested before shutdown, so they are not dashboard-only.

## Test evidence

`tests/thor_evidence_auto67_ram_session_map_test.py` covers empty GLOBAL, mixed old/new, duplicate and repeated session imports, byte-exact failed replace, RAM/disk semantic parity, bounded shutdown after final save, state audit, final-only ingest and lifecycle ordering. The same-session second import reports zero new nodes/edges and leaves the import count and graph hash unchanged.

Focused AUTO67/Dispatcher regressions: `115 passed`.
Debug CTest: `196/196 passed`.
Release CTest: `196/196 passed`.
`git diff --check`: PASS.
Source-code line-limit CTest: PASS.
SOURCE_OWNED: unchanged at `1,475,368 / 3,145,728` (delta 0).

## Runtime and performance

A 60-frame canonical Beyond Oasis (USA) QuickSave1 BizHawk run completed with return code 0 in 70.187 s. It produced and retained `build/auto67-ram-session.sqlite`; GLOBAL was absent both before runtime and after runtime, and was created only by the post-shutdown merge. The runtime map sink reported `session_save_status=PASS`, `global_merge_status=PASS`, zero queue drops/write errors/conflicts, and zero accepted proof candidates for this short capture.

The deterministic 64-merge microbenchmark (`build/m12-ram-session-map-benchmark.json`) measured RAM 29.30 ms total (p50 0.482 ms) versus file-backed SQLite 378.34 ms total (p50 5.946 ms), 12.91x lower merge time in RAM. This isolates map persistence overhead and does not change capture behavior.

## State and identity audits

`state_option_audit: REAL_CONSUMER:src/tools/thor_evidence/capture/live_capsule.lua` — its top-level `state_path` block calls `savestate.load(state_path, true)`. The AUTO67 runner has no `--state` option and does not export `OASIS_LIVE_STATE`; `live_opportunistic.lua` has no such consumer. No savestate-loading change was made.

`explicit_occurrence_id_consistency: PASS` — `PreDispatchTransport._identity` requires explicit `occurrence_id == epoch=<epoch>:seq=<seq>`, rejects mismatches as invalid, and synthesizes the identifier only when legacy records omit it. Identity uses epoch/seq only.

## Publication

Final report JSON: `docs/reports/THOR_M12_AUTO67_RAM_SESSION_MAP_1.json`.
