# M12 AUTO67 BizHawk Native Ring Bridge 1C — Worker Input Trace

**Baseline:** `a22bef0a3445a0d1cbdc347023a25ee7737bd32f`

**Classification:** `PASS_WORKER_INPUT_PRESERVED`

The same concrete native-snapshot occurrence was observed at mailbox write,
Worker mailbox read, and the existing `materialize(...)` call. Every requested
field compared exactly at both boundaries. This proves that the Worker neither
changed the event nor substituted its snapshot on this path. It does not repair
or reclassify the earlier 1B resolver-output gate.

## Runtime

The isolated GPGX native-ring artifact was
`f6bb758083d1c88873a067ebcdf047a7fedc18d4a3a427b37f6c60693c64e2b8`; canonical
ROM SHA-256 was `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
The natural no-input run completed 1,800 frames in 34.219 seconds. No savestate
was loaded. All 4,976 native-snapshot occurrences were frozen and completed;
there were zero missing or invalid snapshots. The observer retained 48 bounded
BUS_WRITE candidates and selected the complete preferred-PC trace below.

## Concrete three-point trace

`EVENT_PC` and `NATIVE_LAST_PC_AT_FREEZE` are deliberately listed separately;
this checkpoint makes no claim about which PC is correct.

| Trace point | Worker / investigation | Occurrence | Kind | Event PC | Address | Event epoch / seq | Snapshot identity | Snapshot epoch; first–latest; count | Frozen-records SHA-256 | Normalized full-event SHA-256 |
|---|---|---|---|---|---|---|---|---|---|---|
| `DISPATCH_INPUT` | `0` / `INV-AUTO67-3fa1ddc8026d374661c6ec61` | `epoch=0:seq=0` | `BUS_WRITE_PC` | `0x00026C` | `0xFFF62A` | `0 / 0` | `NR-0000-0000000000-0000000000001456` | `0; 1–1456; 1456` | `14afd215ce41bcc67b4b89455e072f067d8c2491958d0504ca92e98da8f83f7c` | `8767d86ffe98e36090f87f99c2e63b0f646b5b5ac6d1df186452c375839ab88b` |
| `WORKER_RECEIVED` | `0` / `INV-AUTO67-3fa1ddc8026d374661c6ec61` | `epoch=0:seq=0` | `BUS_WRITE_PC` | `0x00026C` | `0xFFF62A` | `0 / 0` | `NR-0000-0000000000-0000000000001456` | `0; 1–1456; 1456` | `14afd215ce41bcc67b4b89455e072f067d8c2491958d0504ca92e98da8f83f7c` | `8767d86ffe98e36090f87f99c2e63b0f646b5b5ac6d1df186452c375839ab88b` |
| `MATERIALIZER_INPUT` | `0` / `INV-AUTO67-3fa1ddc8026d374661c6ec61` | `epoch=0:seq=0` | `BUS_WRITE_PC` | `0x00026C` | `0xFFF62A` | `0 / 0` | `NR-0000-0000000000-0000000000001456` | `0; 1–1456; 1456` | `14afd215ce41bcc67b4b89455e072f067d8c2491958d0504ca92e98da8f83f7c` | `8767d86ffe98e36090f87f99c2e63b0f646b5b5ac6d1df186452c375839ab88b` |

At freeze, the same snapshot's final native record was
`NATIVE_LAST_PC_AT_FREEZE = 0x00026A`, opcode `0x2D00`; the event PC used by the
materializer was `EVENT_PC = 0x00026C`. The trace does not infer a correction or
preference between them.

The observer compares worker id, investigation id, occurrence identity, event
kind/PC/address/epoch/sequence, snapshot identity/epoch/bounds/count, a SHA-256
recomputed from the frozen record tuple, and a canonical SHA-256 of the complete
`task["event"]`. All 15 fields were equal for `DISPATCH_INPUT ==
WORKER_RECEIVED` and `WORKER_RECEIVED == MATERIALIZER_INPUT`.

## Regression and validation

The new regression starts the real `Dispatcher` and Worker, sends the event
through `NativeSnapshotAdmission` and the actual mailbox, then uses a bounded
fixture capsule status to release the ordinary Worker capture wait. The test
does not replace or bypass the mailbox transfer or materializer call.

Focused native-snapshot tests passed 7/7. Debug and Release builds passed;
Debug CTest passed 198/198 and Release CTest passed 198/198, including
`project_file_line_limit`. `git diff --check` passed. `SOURCE_OWNED` remains
1,475,600 / 3,145,728 bytes (delta 0). No event PC, decoder, resolver, required
register set, Worker semantics, Dispatcher scheduling, Cartographer, global
map, frontier, native ring, snapshot-pool size, or C++ code was changed.

The committed JSON records `PENDING_EXTERNAL_VERIFICATION` for its own
post-push CI. The exact final commit SHA and GitHub Actions run are returned in
the publication result.
