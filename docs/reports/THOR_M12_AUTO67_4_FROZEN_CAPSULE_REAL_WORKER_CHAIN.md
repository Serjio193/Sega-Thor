# THOR M12 AUTO67.4 — Frozen Capsule to Real Worker Chain

Status: **PASS**. This is a bounded developer-only proof, not a semantic chain
closure or ownership promotion.

## Change boundary

The existing AUTO67 path remains in place. `live_capsule.lua` now writes O67V
v2 capsules: a 24-byte physical header, a logical 64-byte header accounting,
and 20-byte records (`sequence`, `frame`, `address`, `pc`, `kind_code`). The
decoder accepts historical O67C files but requires O67V v2 for new worker
materialization. It validates magic, version, capsule id, lease id, record
count, logical length, physical length, and truncation.

The worker decodes the frozen body and produces separate
`runtime_observations`, `causal_facts` with `RUNTIME_CAPSULE` evidence, and an
explicit unresolved frontier. It does not infer semantic closure. New rows
are marked `MATERIALIZED_CHAIN`; historical rows remain `SEED_ONLY`.

The only bug found during the real run was a stale `FREE` status snapshot
overwriting a Python-held lease. `CapsulePool.sync()` now rejects a status
whose lease differs from the active lease, including stale `lease_id=null`.
This is a handoff guard, not an architecture change.

## Real BizHawk proof

Command inputs: canonical Beyond Oasis ROM, the existing QuickSave1 state, 720
frames, 16 workers, 16 capsule slots, and a new proof database:

| Item | Result |
|---|---:|
| workers configured / peak working | 16 / 16 |
| leases / completed returns | 336 / 320 |
| active collisions / merges | 331 / 331 |
| duplicate active claims | 0 |
| decoded capsule records / decoder errors | 49,486 / 0 |
| materialized worker results / causal facts | 304 / 557 |
| queue drops / DB errors | 0 / 0 |
| rolling window | 256 / 256, 2,000 overwrites |
| average / maximum seed age | 0.207 / 0.796 seconds |
| frame maximum / over 50 ms | 33 ms / 0 |
| raw event backlog | NONEXISTENT |
| BizHawk return code | 0 |

The bounded run ended with 16 final in-flight capsules because the emulator
stopped at the frame limit; 320 worker results were returned and persisted
before shutdown. No decoder error or queue drop occurred.

## Canary

The real database contains a `MATERIALIZED_CHAIN` for:

```text
BUS_WRITE_PC  pc=0x0027EC  address=0xC00004
```

Its chain hash is
`162bd4d300c3201edeade495edbfad78a775bc392527b63dab0708377aeb7af8`.
The associated frozen file was
`build/auto67-4-short-proof3.capsules/capsule-07-L000000AE.bin`, decoded as
O67V v2, 208 records, 4,184 physical bytes. The persisted canonical payload
contains 208 runtime observations, 7 runtime-supported causal facts, and an
explicit `UNKNOWN` frontier requiring static decode and register provenance.

An unrelated `BUS_WRITE_PC` seed uses the same generic dispatcher, capsule,
decoder, materializer and persistence path. One concrete row is
`BUS_WRITE_PC pc=0x06009A address=0xFF0B82`, hash
`02e55c1d4c82d871cf906d12ab3fac61c486fa607650113a9b7edcbeab2465a5`, with
30 decoded runtime observations. The proof database contains 151 unique
materialized rows and 16 seed-only rows in total.

## Database and historical preservation

The proof database is
`build/thor-evidence/auto67-4/short-proof3.sqlite`. It contains 167 unique
`live_chain` rows: 151 `MATERIALIZED_CHAIN` and 16 `SEED_ONLY`. The
`live_chain.chain_hash` primary key prevents duplicate hash rows; 153 exact
duplicate observations were updated rather than inserted.

The previous AUTO67.3 database was inspected through a SQLite read-only URI:
`build/thor-evidence/auto67-3/persistent-chain-proof-v3.sqlite` still has 264
rows and its original nine-column `live_chain` schema. It was not opened by
the migrating `Store` and was not modified.

Five actual short canonical payloads from the new SQLite proof, plus the full
canary metadata, are in the machine-readable companion:
`THOR_M12_AUTO67_4_FROZEN_CAPSULE_REAL_WORKER_CHAIN.json`.

## Validation

- `tests/thor_evidence_auto67_4_test.py`: 6 passed.
- `tests/thor_evidence_auto67_3_test.py`: 3 passed.
- `tests/thor_evidence_auto67_1_test.py`: 10 passed.
- `tests/thor_evidence_auto67_test.py`: 9 passed.
- Python compilation for the changed AUTO67 modules: PASS.
- CTest AUTO67/AUTO67.1/AUTO67.3/AUTO67.4 helpers: 4 passed.
- Debug and Release CMake builds: PASS.
- Source-file limit: PASS, 643 governed files at or below 500 lines.
- `git diff --check`: PASS.

No ROM, savestate, extracted asset, production runtime code, SOURCE_OWNED
record, semantic merge, raw-event backlog, or AUTO68 work was added.
