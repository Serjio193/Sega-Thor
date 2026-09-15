# THOR M12 AUTO67 CAPTURE-PERF-1 — hook overhead isolation

- **Classification:** `PASS_GLOBAL_EXEC_ANY_BOTTLENECK`
- **Baseline:** `67a49df5fc49de11e416852a70ccf93580447180`
- **ROM:** `local-roms/Beyond Oasis (USA).md`, SHA-256 `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`
- **BizHawk:** `C:\Dev\SegaThorTools\BizHawk-2.11.1-win-x64\EmuHawk.exe`, version 2.11.1 / GenPlus-gx
- **State:** `Beyond Oasis (U) [!].Genplus-gx.QuickSave1.State`
- **Same Lua:** `src/tools/thor_evidence/capture/live_capsule.lua`
- **Bound:** 60 frames per run, 16 workers, window 256, no map DB, no view.

All four runs used the same ROM, state and frame bound. A/B had capsule mode
OFF; C/D had capsule mode ON with `max-live-captures=16`.

| MODE | GLOBAL EXEC CALLBACKS | TARGET EXEC | TARGET WRITE | FRAME P50 | P95 | P99 | MAX |
|---|---:|---:|---:|---:|---:|---:|---:|
| A minimal live capture | 0 | 0 | 0 | 17,000 µs | 17,000 µs | 26,000 µs | 26,000 µs |
| B global prehistory only | 563,191 (9,386/frame) | 0 | 0 | 621,000 µs | 707,000 µs | 715,000 µs | 715,000 µs |
| C capsule hooks only | 0 | 5,645 | 1,335 | 17,000 µs | 28,000 µs | 42,000 µs | 42,000 µs |
| D combined current | 563,191 (9,386/frame) | 5,645 | 1,791 | 649,000 µs | 723,000 µs | 733,000 µs | 733,000 µs |

The result is `PASS_GLOBAL_EXEC_ANY_BOTTLENECK`: B reproduces 95.7% of D's
p50 frame cost (`621 ms / 649 ms`), while C is effectively A (`17 ms` p50 in
both). Therefore continuous predecessor `event.on_bus_exec_any` is dominant;
capsule targeted hooks contribute only a small residual cost in this run.

The prior LIVE-MAP-1 interpretation is corrected. The `24,007` target PCs are
lookup candidates used by `join_write()` in continuous predecessor mode; they
are **not** 24,007 `targeted_bus_exec` registrations. Also,
`hook_metrics.classes.prehistory_global_exec.calls == 0` is not a callback count:
the predecessor global callback does not call `hook_metrics.callback_start/end`.
The actual global execution-hook volume is `predecessor.records_observed`:
`563,191` callbacks, or `9,386/frame`, in B and D.

Capsule scaling (1/4/16 simultaneous captures) was not run because C was not
materially slower than A, as permitted by the checkpoint.

`SOURCE_OWNED` stayed `1,475,600 → 1,475,600` (delta `0`). No production code,
map database, scheduler, Dispatcher, Worker or Cartographer was changed.

Focused AUTO67/Dispatcher regressions: **109 passed**. Debug/Release CTest was
not rerun because this is documentation-only; the prior production baseline
remains valid. Source-limit is unchanged. `git diff --check` is recorded after
publication.

LIVE-MAP-1 reference: final SHA
`67a49df5fc49de11e416852a70ccf93580447180`; GitHub Actions run `35006780426`,
completed / success.
