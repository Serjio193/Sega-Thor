# M12-AUTO67-BIZHAWK-NATIVE-RING-CORE-1 — corrective actual-core ring proof

**Baseline:** `e4d1c0f2b452134367e3c34a11f091c3ade1cc6f`
**Classification:** `PASS_NATIVE_RING_CORE`

This corrective checkpoint proves the continuous native recorder in the actual
custom GPGX core run. It does not integrate the ring into AUTO67 and does not
implement `NATIVE-RING-BRIDGE-1`, freeze/read transport, or any managed final
transport.

## Scope and exact artifact

A is the locally rebuilt unmodified GPGX from Phase-0A-R1. B is the same source
with the existing native-ring recorder patch applied to `m68kcpu.c`,
`trace_ring.[ch]`, the GPGX Makefile, and the C export surface. The C# LibGPGX
bridge hunk was not applied. A temporary external diagnostic tool called the
existing experimental live-copy exports after timing; it is test-only and is
not production AUTO67 or a transport architecture.

* BizHawk source: `bdddf4a58aa1a022afb11dc73294a81a5aa7bbd5`.
* Genesis-Plus-GX source: `051d430d3d1b54625f9900c8f152d7f232e06daf`.
* BizHawk patch SHA-256:
  `a6e303140d5a85e11c604a0cfdfad42413395c49e4a70b1415c1aa8a627609e5`.
* Genesis-Plus-GX patch SHA-256:
  `139c2a15d27ddded659bed964349076a3e2efc91b8b43bdfbd350aa8200d85eb`.
* Custom artifact: 4,026,624 bytes,
  SHA-256 `f6bb758083d1c88873a067ebcdf047a7fedc18d4a3a427b37f6c60693c64e2b8`.
* Custom test installation:
  `C:\Dev\SegaThorTools\BizHawk-2.11.1-native-ring-core-test`.
* Authoritative Windows installation was not overwritten.

The native record is exactly 16 bytes (`uint64_t sequence`, `uint32_t pc`,
`uint16_t opcode`, `uint16_t reserved`) with capacity 4096. The hot path records
pre-fetch PC and fetched `REG_IR` around the normal opcode fetch and performs no
Lua/managed callback, allocation, I/O, synchronization, map lookup, register or
RAM snapshot, or game-state mutation. The controlled performance Lua script
contains no `event.on_bus_exec_any`.

## Cold-reset emulation parity

A and B were launched independently from clean ROM power-on with the exact
Phase-0A-R1 schedule: steps 1–30 neutral, 31–60 `Right`, 61–300 neutral. The
canonical ROM SHA-256 is
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

| step | frame | PC | A4 | A5 | RAM FF188A | RAM FF188C | VDP writes | DMA writes |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 1 | 2 | 618 | 12582916 | 760 | 0 | 0 | 0 | 0 |
| 10 | 11 | 906 | 12582916 | 0 | 0 | 0 | 43 | 43 |
| 30 | 31 | 912 | 12582916 | 0 | 0 | 0 | 43 | 43 |
| 60 | 61 | 920 | 12582916 | 0 | 0 | 0 | 43 | 43 |
| 120 | 121 | 13038 | 12582916 | 12582912 | 0 | 0 | 92 | 92 |
| 300 | 301 | 13044 | 12582916 | 12582912 | 0 | 0 | 260 | 260 |

The stock and native-ring checkpoint files are byte-for-byte identical.
`FIRST_DIVERGENCE: none`. Result: **EMULATION PARITY PASS**.

## Actual core ring receipt

Immediately before the controlled B run, the diagnostic called
`gpgx_native_trace_ring_reset()`. It then ran exactly 60 frames with the input
schedule 1–30 neutral and 31–60 `Right`; no `event.on_bus_exec_any` was
installed. After the measured interval stopped, and only then, it called the
same core instance's existing experimental live-copy APIs:

* `gpgx_native_trace_ring_latest()` → `latest_sequence = 920003`.
* `gpgx_native_trace_ring_count()` → `count = 4096`.
* `gpgx_native_trace_ring_copy(915908, ...)` → `copied = 4096`.

Therefore `records_total = latest_sequence = 920003`,
`records_per_frame = 920003 / 60 = 15333.3833333333`, and
`count = min(records_total, 4096) = 4096`. The retained first sequence is
`latest_sequence - count + 1 = 915908`; the copied last sequence is `920003`.

All 4096 copied records from the actual GPGX core passed the following checks:

* sequences strictly monotonic and contiguous (`915908..920003`);
* retained first and last sequences match the latest/count rule;
* wrapped retention semantics pass (`latest_sequence > 4096`, capacity 4096);
* every M68K PC is valid and even;
* all 4096 retained PCs were ROM-resident for this receipt, and every
  `record.opcode` equals canonical ROM `[PC:PC+2]` (`mismatches = 0`).

The live-copy read and copy happened after the timer stopped, so they are not
included in the native recorder timing interval. This is actual core data, not
the earlier standalone synthetic host harness. The diagnostic used only the
existing exports `latest`, `count`, `copy`, and `reset`; it did not add a second
cursor, freeze, queue, bridge, or final transport.

## Performance experiment

A and B were each run 12 times with the same exact 60-frame Lua loop and input
schedule. Values below are in-process `os.clock` milliseconds for that current
60-frame loop; no per-instruction Lua callback was present.

| mode | ring | p50 (ms) | p95 (ms) | p99 (ms) | max (ms) | samples |
|---|---:|---:|---:|---:|---:|---:|
| A — rebuilt stock | no | 982.0 | 982.45 | 982.89 | 983 | 12 |
| B — rebuilt native ring | yes, 4096 | 981.5 | 982.45 | 982.89 | 983 | 12 |

`B.p50 / A.p50 = 0.99949x`, below the required `2.0x` gate. The exact
artifact was used for B and the diagnostic read/copy was performed only after
the measured interval.

The historical Lua reference is a separate prior CAPTURE-PERF measurement, not
this current loop and not the native ring receipt: its p50/p95/p99/max were
621/707/715/715 ms and its `9386.5167 records/frame` counts Lua callback
records. It is retained as historical context with its own units and is not
combined with the current core `920003 / 60` instruction-record rate.

## Optional native-core self-state smoke

The custom core independently saved a new state after 120 frames, advanced 20
frames, reloaded its own state, replayed the same 20 `Right` frames, and matched
frame/PC/A4/A5/RAM (`equal=true`). Cross-WBX states were not used.

## Validation

* Custom native-ring Waterbox GPGX build: **PASS**.
* Cold-reset A/B checkpoint equality: **PASS**.
* Actual core latest/count/copy/reset receipt: **PASS**.
* Actual core sequence/order/wrap/PC/opcode-ROM proof: **PASS**.
* Native-core self-savestate smoke: **PASS**.
* Native-ring overhead ratio: **0.99949x**, **PASS**.
* Debug CTest: **195/195 PASS**.
* Release CTest: **194/195 PASS**; existing
  `oasis_re_import_gpgx_coverage_self_test` fails with `invalid JSON at 0:
  missing value`.
* Source-limit: **PASS**.
* `SOURCE_OWNED = 1,475,600 / 3,145,728`, delta 0; unchanged.
* `git diff --check`: **PASS**.
* GitHub CI for the eventual publication SHA: `PENDING_EXTERNAL_VERIFICATION`.

Stop after this checkpoint. `NATIVE-RING-BRIDGE-1` and all freeze/read managed
integration remain future work.
