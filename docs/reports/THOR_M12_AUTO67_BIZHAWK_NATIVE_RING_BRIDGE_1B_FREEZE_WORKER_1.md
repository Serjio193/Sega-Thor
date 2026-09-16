# M12 AUTO67 BizHawk Native Ring Bridge 1B — Freeze to Worker

**Baseline:** `d13a0b028a3991212f2d29dddda70fb182a05554`

**Classification:** `STOP_NATIVE_RESOLVER_OUTPUT_MISSING`

The real native freeze, snapshot admission, capsule decode, Worker lease and
slot-release path passed. The checkpoint stops at the required resolver gate:
the natural no-input runtime produced no Worker event for which the existing
resolver requested a register, so there is no factual producer result to
publish. This is not classified as a PASS.

## Artifact and runtime

The run used the same experimental native GPGX artifact as the accepted core
ring proof: `f6bb758083d1c88873a067ebcdf047a7fedc18d4a3a427b37f6c60693c64e2b8`.
The canonical ROM hash was
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`. The
isolated BizHawk managed-host build succeeded in Release with zero errors; it
exposes the test-only synchronous snapshot call through Lua. No savestate or
input was used. Production AUTO67 and the authoritative BizHawk installation
were not changed.

The supplemental managed-host patch SHA-256 is
`328b87d7ba61ceae5836a85d96d85d81e1141f9c1b9f0a6643def0aa6d45e5cc`; it
contains only the `GenesisLuaLibrary.cs` bridge used by the successful build.

The bounded natural run completed 1,800 frames in 34.672 seconds with 16
Workers. Of 3,496 transported occurrences, all 3,496 received a native freeze,
were dispatched, and completed. The 16-slot snapshot pool peaked at 5 slots;
invalid, missing, and pool-full counts were zero, and the pool ended empty.
Sixteen capsule-backed Worker materializations received the native snapshot
through the existing predecessor adapter. For all 16, the Worker-start record
hash matched the frozen hash, the live native sequence had advanced beyond the
snapshot, and the slot was released after Worker completion.

## Concrete occurrence receipt

This is the complete factual path up to the first missing gate:

| Stage | Evidence |
|---|---|
| Occurrence | `epoch=0:seq=0`, `BUS_WRITE_PC`, event PC `0x00026C`, address `0xFFF62A` |
| Freeze and identity | snapshot `NR-0000-0000000000-0000000000001456`; epoch 0; retained sequences 1–1456; count 1456 |
| Actual native contents | 16-byte GPGX records; ordered range validated; worker-start SHA-256 `14afd215ce41bcc67b4b89455e072f067d8c2491958d0504ca92e98da8f83f7c`; final record PC `0x00026A`, opcode `0x2D00` |
| Live ring and immutability | live latest was 502,156 at Worker start; frozen tuple hash matched; immutable snapshot remained intact |
| Worker | worker 0; the capsule's predecessor decode used this exact snapshot identity; slot release after Worker completion: `true` |
| Resolver gate | `resolver_executed=false`; status `NOT_REQUIRED`; requested registers `[]`; resolver capture absent |
| Output | `RESOLVER_NOT_REQUIRED`; producer not established |

The occurrence PC from the Lua bus callback (`0x00026C`) did not equal the last
native ring PC (`0x00026A`). The ROM decoder reports the event PC opcode as
unsupported `0x51CE`. Across all 16 materialized Worker results, the existing
resolver was entered zero times and resolved/unresolved register counts were
both zero. Other bus-write occurrences had either unsupported static opcodes
or address mismatches. Thus the captured native history reached the Worker,
but the required concrete resolver result was not produced.

The existing continuous prehistory-join mode was tried once to localize the
missing PC join. That diagnostic reached frame 184 in about 120 seconds and had
zero exact joins among the 192 bus-write events retained in the last status
snapshot. It was stopped as an incomplete diagnostic and is not counted as a
passing runtime test.

## Measured path cost

Measurements below are from the 16 capsule-backed real Worker receipts. Native
API read/copy/compress/freeze p50 values were 0.3 µs, 3.7 µs, 178.4 µs and
188.8 µs respectively. The synchronous host-method freeze metric includes
compression and ends before Base64/JSON response serialization; the native ring
read plus copy itself measured 4.0 µs p50. Worker materialization p50 was 71.0
µs (maximum 2.1073 ms), using the same nearest-rank percentile calculation as
the project profiler; it included no resolver call. Resolver duration is
therefore **not available**, rather than zero.

## Scope and verification

The Lua native-history ring, 4,096-record capacity, RollingWindow, Dispatcher,
Worker, Capsule limits, predecessor resolver semantics, Cartographer, MAP-1,
SOURCE_OWNED and C++ were left unchanged. `SOURCE_OWNED` remains
`1,475,600 / 3,145,728` (delta 0).

Focused native-snapshot tests passed 6/6; snapshot-admission regressions passed
10/10. Full Debug CTest passed 198/198. Full Release CTest passed 198/198 on a
serial repeat; the project `project_file_line_limit` test passed. `git diff
--cached --check` passed. The committed receipt uses
`PENDING_EXTERNAL_VERIFICATION` for GitHub CI so it does not claim a CI run for
its own commit before that run exists.

Changed files are limited to the native snapshot adapter and opt-in probe
runner, capsule snapshot wiring, Lua freeze helper/status, CTest registration,
focused tests, file map/worklog, and the native-ring developer README/host patch
and this report pair. No Worker, Dispatcher, Cartographer, C++, or ownership
source was changed.
