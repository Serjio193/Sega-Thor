# M12 AUTO67 live-forward Worker 1B scaling — runtime stop

**Date:** 2026-09-17
**Baseline:** `17780138a6e3d9c67c2bc83c76350c482c6a8140`
**Result:** `STOP_RUNTIME_CPU_EXECUTION_STREAM_STALL`

The recovered musl Waterbox sysroot/toolchain built the 1B GPGX core, and a
coherent BizHawk 2.11.1 Release install passed startup smoke with that exact
WBX. The live natural and forced campaigns then proved repeated per-Worker
lifecycle cycles at the counts below. Every PASS has 100 cycles per Worker,
`WORKER_COUNT * 100` independently host-audited segments, fresh capture IDs and
generations, advancing CPU execution during immutable reread and host audit,
exact ACKs, and reconciled native `FREE -> CAPTURING -> COMPLETE -> ANALYZING
-> FREE` transition counters.

The first runtime correctness stop was `CPU execution stream stopped before
immutable result reread`. It occurred at natural `WORKER_COUNT=128` and forced
`WORKER_COUNT=256`; neither failed count reached 100 cycles per Worker, so
neither is a PASS. No larger count was run, and no optimization was attempted.
This is a measured execution-progress/correctness boundary, not evidence of a
RAM allocation ceiling or a general maximum Worker capacity.

| Phase | Workers | Result | Host-audited segments | Completed cycles per Worker | Peak simultaneous captures |
|---|---:|---|---:|---:|---:|
| Natural | 1 | PASS | 100 | 100 | 1 |
| Natural | 2 | PASS | 200 | 100 | 2 |
| Natural | 4 | PASS | 400 | 100 | 4 |
| Natural | 8 | PASS | 800 | 100 | 8 |
| Natural | 16 | PASS | 1,600 | 100 | 16 |
| Natural | 32 | PASS | 3,200 | 100 | 32 |
| Natural | 64 | PASS | 6,400 | 100 | 64 |
| Natural | 128 | STOP | 8,448 before stop | 66 | — |
| Forced | 2 | PASS | 200 | 100 | 2 |
| Forced | 4 | PASS | 400 | 100 | 4 |
| Forced | 8 | PASS | 800 | 100 | 8 |
| Forced | 16 | PASS | 1,600 | 100 | 16 |
| Forced | 32 | PASS | 3,200 | 100 | 32 |
| Forced | 64 | PASS | 6,400 | 100 | 64 |
| Forced | 128 | PASS | 12,800 | 100 | 128 |
| Forced | 256 | STOP | 8,448 before stop | 33 | — |

Natural PASS receipts contain 12,700 segments total; forced PASS receipts
contain 25,400, for 38,100 proven segments. At both failed counts, the 8,448
segments completed before the stop were individually audited, but fall short
of the required 12,800 / 25,600 respectively. Full per-count receipts, raw
runtime logs, Lua logs and JSONL segment audits remain under
`build/thor-evidence/live-forward-worker-1b/campaign-100cycles-proven/` and
`build/thor-evidence/live-forward-worker-1b/campaign-forced-100cycles-proven/`.
The aggregate receipt is
`docs/reports/THOR_M12_AUTO67_LIVE_FORWARD_WORKER_1B_SCALING.json`.

The installed Worker 1B bundle originally contained an old compressed
`gpgx.wbx.zst` beside the new uncompressed WBX. BizHawk prefers the compressed
sidecar, so the earlier probes had loaded the old core. The old sidecar was
preserved under a backup name in the isolated coherent test install, after
which the newly built core initialized, sealed and completed the full campaign.
The WBX SHA-256 is
`4AC692A115CB5543BB3C2260FC04FDACD2CF5D963DF59C17D87A12A30ADD3CD4` (4,055,592
bytes); the ROM SHA-256 is
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

The build used the pinned BizHawk 2.11.1 baseline
`bdddf4a58aa1a022afb11dc73294a81a5aa7bbd5`, musl submodule
`2063abc4e16c84218757b1db10d3cdf9f36ef3f8`, GPGX submodule
`051d430d3d1b54625f9900c8f152d7f232e06daf`, and LLVM `llvmorg-18.1.8`. The
native Worker implementation remains developer-only; production AUTO67,
predecessor handling, Cartographer and `SOURCE_OWNED` were not changed.

| Check | Result |
|---|---|
| Native synthetic lifecycle regression | PASS: 100 cycles per slot for test pool sizes 1, 2, 4, 8, 16 and 32; deterministic synthetic feed, not BizHawk runtime evidence |
| Full Windows Debug CTest | PASS: 201/201 |
| Full Windows Release CTest | PASS: 201/201 |
| GNU/Linux focused CTest | PASS: 3/3, including native Worker, scaling and Python audit tests |
| Python scaling-audit unit tests | PASS: 4/4, including Windows CRLF and final process-exit drain |
| Python compilation | PASS |
| BizHawk EmuHawk Release build | PASS: 0 errors; one existing SharpCompress NU1902 warning |
| BizHawk Emulation.Cores Release rebuild after diagnostic cleanup | PASS: 0 errors, 0 warnings |
| Recovered Waterbox GPGX Release build | PASS: matching 1B WBX hash above |
| `SOURCE_OWNED` delta | 0 |
| Exact-SHA GitHub Actions | Run after publication; exact-SHA result is recorded in the publication response |

The runtime acceptance is complete through the factual stop conditions above;
counts that failed the per-Worker 100-cycle requirement remain explicitly
unproven. No claim of a RAM-bound maximum is made. The incremental BizHawk and
GPGX patch hashes are recorded in the aggregate JSON receipt.
