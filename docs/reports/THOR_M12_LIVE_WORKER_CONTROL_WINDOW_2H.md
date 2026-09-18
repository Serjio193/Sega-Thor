# M12 Live Worker Control Window 2H

**Checkpoint:** `PASS_LIVE_WORKER_CONTROL_WINDOW_V1`

**Baseline:** `36dc7e6acc61eb534e4c5269ed30c3b875d54d75`

**Runtime:** canonical Beyond Oasis ROM, SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`
**Native GPGX artifact:** `gpgx.wbx`, SHA-256
`C8309FFAFA5545BA6CC4C5A4F41420F15F222F6F4275302D648F1EDFF02AD1F7`

## Result

The separate `THOR WORKER CONTROL` process reads a bounded, atomically
replaced status snapshot at approximately 4 Hz. It displays Worker lifecycle
and chain progress, system and Thor process working sets, the exact native
plan and next-run free-memory projection, and evidence growth measured from
the evidence tree. The UI writes only the deterministic next-run config; the
launcher captures its chosen configuration before EmuHawk starts. The native
planner and startup resource preflight remain authoritative, with no silent
clamp. The native status query is read-only and runs through the existing
bounded control/status path, not a per-instruction callback.

Five real campaigns passed:

| Campaign | Config | Window | Completed / independently audited | Worker cycles | Worker p50 / max | Retention failures | Runtime errors |
| --- | ---: | --- | ---: | ---: | ---: | ---: | ---: |
| `campaign-no-window` | 16×20 | Off | 1,600 / 1,600 | 100 each | 16 / 17 ms | 0 | 0 |
| `campaign-window` | 16×20 | On | 1,600 / 1,600 | 100 each | 16 / 17 ms | 0 | 0 |
| `campaign-live-save-16x20` | 16×20 | On | 1,600 / 1,600 | 100 each | 16 / 20 ms | 0 | 0 |
| `campaign-next-run-8x21` | 8×21 | On | 800 / 800 | 100 each | 16 / 17 ms | 0 | 0 |
| `campaign-live-restart-8x21` | 8×21 | On | 800 / 800 | 100 each | 16 / 17 ms | 0 | 0 |

All 6,400 segment audit files matched their receipt SHA-256. Each run has 100
valid segments per Worker, fresh capture IDs, and generations 1–100 per
Worker. The independent audit reported zero identity conflicts, unsupported
decode occurrences, opcode mismatches, and non-ROM occurrences. Native
metrics reported zero retention failures, drops, invalid captures, stale
ACKs, and identity collisions.

The mid-run configuration proof started at 16×20, saved 8×21 while the run
was active, and then confirmed that current remained 16×20 through all 1,600
segments. A following launch read exactly 8×21 from persistent config and
completed 800/800; a separate restart of the same saved config also completed
800/800. The deterministic A–O tests additionally prove arbitrarily large
positive decimal values are preserved by config and rejected only by exact
planner/preflight constraints, without UI clamping.

## Performance and live status

The paired 120-frame timing samples showed `PERF_WORKER` at 16 ms p50 and
17 ms max both with the window disabled and enabled. `PERF_BASELINE` was
17/17 ms without the window and 17/19 ms with it; `PERF_RECORDER` was
17/18 ms in both. This millisecond-resolution sample shows no measured Worker
timing regression; it does not establish zero overhead. The host audit
observed CPU stream progression in every campaign (for the paired runs:
10,530,746 records without the window and 9,270,659 with it; separate run
variation makes these totals unsuitable as a throughput comparison).

The final saved-config restart snapshot reported:

| Metric | Observed value |
| --- | ---: |
| Current Workers / depth | 8 / 21 |
| Saved next-run Workers / depth | 8 / 21 |
| Current native Worker allocation | 723,200 bytes |
| System RAM total / available | 34,121,834,496 / 13,351,038,976 bytes |
| Thor process working set | 336,490,496 bytes |
| Projected next-run native allocation | 723,200 bytes |
| Projected next-run free RAM | 13,316,695,808 bytes |
| Evidence tree / measured rate | 1,477,014 bytes / 81,983.459 bytes per second |
| Projected evidence growth | 0.275 GiB/hour |
| Snapshot update frequency | 3.91 Hz |
| Separate UI process working set | 33,669,120 bytes |
| Worker captures started / completed | 800 / 800 |
| Audited segments / retention failures / runtime errors | 800 / 0 / 0 |

Thor process memory is the sum of distinct host launcher, EmuHawk PID, and UI
PID working sets; native Worker allocation is already inside the EmuHawk
process and is not added a second time. The projected next-run free value
releases current-process working sets before subtracting the projected
allocation. Evidence rate is an observed short-run average, displayed to one
decimal GiB/hour in the main window; it is not a long-term storage forecast.

The runtime started the separate UI process and published live worker/memory
snapshots. Its data process stayed responsive at the measured cadence while
all Workers progressed. Automated A–O coverage includes save/load, next-run
application, current-run immutability, no clamp, exact preflight rejection,
RAM reconciliation, evidence growth and stationary cases, missing snapshots,
and slow/closed-window isolation. No pixel-level screenshot review was part
of this runtime proof.

## Validation and boundaries

- Focused 2H tests: 17/17 PASS.
- Debug and Release builds: PASS; Debug and Release CTest: 208/208 each.
- GNU/Linux-equivalent build and CTest: PASS; CTest 208/208.
- Python compilation, source file limit, and `git diff --check`: PASS.
- GPGX WBX build and isolated BizHawk Release host build: PASS. The host build
  retained two existing NU1902 SharpCompress advisories.
- `SOURCE_OWNED`: 1,475,600 bytes before and after; delta 0.
- No production AUTO67, predecessor, Worker 1B scaling, FLOW_V1,
  Cartographer/Archivist, 2D/2E, or emission semantics changed.
- SQLite session/master maps, raw logs, and segment audit evidence stay local
  under ignored `build/thor-evidence/live-worker-control-2h/`; only compact
  source, tests, patches, and reports are publication inputs.

Per-campaign receipt, independent audit, and raw segment file paths are
listed in the compact JSON receipt and remain available locally under the
ignored evidence directory. The broader M12 reconstruction milestone remains
active.
