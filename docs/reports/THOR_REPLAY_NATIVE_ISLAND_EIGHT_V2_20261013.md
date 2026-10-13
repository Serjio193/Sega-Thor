# Frozen eight-helper island telemetry V2 (2026-10-13)

## Scope and identity

V2 preserves the complete V1 identity and frozen helper set. V1 remains unchanged historical evidence. This update adds path/cycle telemetry and an explicit coverage matrix; it does not add helper #11, activate production routing, broaden a caller gate, or alter any evidence authority.

The run used the same canonical ROM, GPGX 1.7.6 core, pinned input poll log, cold-load plus captured initial-state restore, and eight-helper set. The new Debug replay completed 3,600 frames in 45.42 seconds. It matched all 3,600 full-frame state records, 60 periodic hashes, 128 architectural sync records, 128 scheduler sync records, Z80/VDP-related artifacts, normalized helper events, external event ordering, and per-helper runtime counters against the prior Debug replay. The input SHA-256 remained `a5b2973a6f26b2c29948263e9aa160f05f0f7acc8e2e879407854d6002e24ef4`; all five trace runs share calls trace SHA-256 `9dc29baece0c8b314e284e0995784116851e258d5ec5078b0da20fee76427aad`.

No trace behavior was changed. The new per-call timing/path census is derived offline from the recorded `STEP_INSTRUCTION` trace. The seven non-E268 helpers have full native-call body traces in this replay. E268 instruction tracing is sampled at 17 of 45,047 native calls; its full-call, IRQ, refresh-event, and resume counts come from the runtime E268 metrics file.

## Unified per-helper telemetry

Cycle limits are measured master-cycle totals from entry through RTS, with the existing normalizer's observed refresh-before-RTS adjustment. `REFRESH_CROSSING_CALLS` for the first seven helpers counts captured calls whose RTS snapshot had the refresh-pending state. This is distinct from a refresh-event counter. Fields without an exact counter are `UNINSTRUMENTED`, never zero.

| Helper | Native / ROM fallback | Rejects (provenance / precondition / contract) | Timing calls / unique vectors | Cycles min–max | Path IDs | Refresh crossings / events | IRQ pause/resume | Yield / resume | Per-helper host time |
|---|---:|---:|---:|---:|---:|---|---|---|---|
| `0x9D70` | 184 / 2 | 2 / 0 / 0 | 184 / 10 | 406–490 | 1 | 2 / UNINSTRUMENTED | UNINSTRUMENTED / UNINSTRUMENTED | UNINSTRUMENTED |
| `0x9D4E` | 113 / 0 | 0 / 0 / 0 | 113 / 13 | 546–630 | 1 | 7 / UNINSTRUMENTED | UNINSTRUMENTED / UNINSTRUMENTED | UNINSTRUMENTED |
| `0x9DB4` | 507 / 0 | 0 / 0 / 0 | 507 / 13 | 406–497 | 1 | 3 / UNINSTRUMENTED | UNINSTRUMENTED / UNINSTRUMENTED | UNINSTRUMENTED |
| `0x9D92` | 24 / 208 | 208 / 0 / 0 | 24 / 7 | 546–637 | 1 | 0 / UNINSTRUMENTED | UNINSTRUMENTED / UNINSTRUMENTED | UNINSTRUMENTED |
| `0x94A2` | 66 / 0 | 0 / 0 / 0 | 66 / 33 | 1848–2534 | 3 | 3 / UNINSTRUMENTED | UNINSTRUMENTED / UNINSTRUMENTED | UNINSTRUMENTED |
| `0x9C40` | 24 / 3 | 3 / 0 / 0 | 24 / 11 | 1820–1911 | 1 | 3 / UNINSTRUMENTED | UNINSTRUMENTED / UNINSTRUMENTED | UNINSTRUMENTED |
| `0x8EF8` | 3 / 11 | 11 / 0 / 0 | 3 / 3 | 672–686 | 1 | 0 / UNINSTRUMENTED | UNINSTRUMENTED / UNINSTRUMENTED | UNINSTRUMENTED |
| `0xE268` | 45,047 / 410,687 | 410,687 / 0 / 0 | 17 sampled / 6 | 1302–1316 sampled | 1 sampled | crossing calls UNINSTRUMENTED; 61,583 events; 45,046 variant calls | 73 / 73 | yield UNINSTRUMENTED / 15,825 | UNINSTRUMENTED |

For the 7 fully traced helpers, route/call telemetry is complete for the pinned replay. E268's path-ID counts, cycle vectors, and min/max are explicitly sample-only; do not scale those samples to its full call count. Whole-replay host times remain diagnostic: ROM-only Debug 43.46 s, island Debug runs 46.52/46.49 s, ROM-only Release 12.17 s, island Release 12.46/12.97 s, plus this fresh Debug replay at 45.42 s. Per-helper host cost and P95 are not measured.

## Path coverage matrix

| Helper | CPP path verified | Natural replay paths | Synthetic oracle paths | Stepper paths | IRQ path observed | Refresh variation observed | Full CFG coverage |
|---|---|---|---|---|---|---|---|
| `0x9D70` | YES | YES | UNKNOWN | YES | UNKNOWN | YES | NO |
| `0x9D4E` | YES | YES | UNKNOWN | YES | UNKNOWN | YES | NO |
| `0x9DB4` | YES | YES | UNKNOWN | YES | UNKNOWN | YES | NO |
| `0x9D92` | YES | YES | PARTIAL | YES | UNKNOWN | NO | NO |
| `0x94A2` | YES | YES | UNKNOWN | YES | UNKNOWN | YES | NO |
| `0x9C40` | YES | YES | PARTIAL | YES | UNKNOWN | YES | NO |
| `0x8EF8` | YES | YES | UNKNOWN | YES | UNKNOWN | NO | NO |
| `0xE268` | YES | YES | UNKNOWN | PARTIAL (17-call step sample) | YES | YES | NO |

`PARTIAL` is intentionally retained for the documented synthetic oracle subsets and sampled E268 instruction paths. No matrix entry upgrades path-specific proof to global proof.

## Clean-build dependency preparation

The machine receipt contains the exact declared runner source list (25 files), `oasis_core` source list (28 files), recursively discovered quoted project-header closure (55 files), focused tests (4 files), CMake inputs, and external dependency identities. The external GPGX source tree is required at the pinned GENPLUS-GX 1.7.6 identity; the canonical ROM remains user-supplied and outside repository history. This is a reproduction recipe, not evidence of a clean-checkout build. V1's source checkout was dirty and no clean checkout was attempted in this task.

The V1 global line-limit blocker remains unrelated: `src/tools/re_slice_decoder.cpp` (506 lines), `src/tools/thor_evidence/audio_mode1_structural_closure.py` (519), and `src/tools/thor_evidence/helper_candidate_index.py` (518). The new telemetry tool is 378 lines and its test is 45 lines.

## Recommendation

Next priority: **clean-checkout reproducibility**. The telemetry now shows 8 observed native path IDs across seven fully traced helpers and one sampled E268 path ID; the largest unresolved observability gaps are per-helper scheduler-yield/IRQ counters and E268's per-call timing path census. First reproduce the frozen set from an isolated, clean source checkout and keep the external ROM/core inputs pinned. Then add missing counters only where a distinct scheduler event can be measured without changing emulation behavior. Do not expand the island or design production activation until that reproducibility/counter surface is established.

`REPLAY_NATIVE_ISLAND_VALIDATED=YES` remains limited to the pinned 3,600-frame development replay. `GLOBAL_HELPER_VERIFICATION=NO`, `PRODUCTION_RUNTIME_ACTIVE=NO`, and `FULL_CFG_COVERAGE=NO` remain unchanged. Runtime total 440,425 B, structural proven 1,266,667 B, SOURCE_OWNED 1,487,672 B; authority deltas 0.

Machine receipt: [THOR_REPLAY_NATIVE_ISLAND_EIGHT_V2_20261013.json](THOR_REPLAY_NATIVE_ISLAND_EIGHT_V2_20261013.json).
