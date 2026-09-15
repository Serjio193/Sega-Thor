# M12-AUTO67-NATIVE-TRACE-PROBE-1 — Same evidence semantics, different execution backend

**Baseline:** `aa93307723cfdfce8957d8a9f9adc6ba17b842fe`
**Classification:** `STOP_NO_LOW_OVERHEAD_NATIVE_TRACE`
**Scope:** bounded capture-backend probe only. No Sega-Thor production integration.

## Result

No tested backend provides a complete, low-overhead replacement for the current continuous prehistory semantics.

* BizHawk 2.11.1 / GenPlus-gx has `GPGXTraceBuffer`, but it is a `CallbackBasedTraceBuffer`: enabling the sink installs an execute memory callback, calls `GetCpuFlagsAndRegisters()` and disassembly for every occurrence, and appends formatted text. The source comment explicitly says this is significantly slower than a direct implementation. It is C# callback plumbing over the core, not a native buffered M68K ring consumed in batches.
* MAME 0.289 has a native debugger trace and emits an ordered M68K stream, but the tested standard trace is text-only (PC plus disassembly), does not include the required register snapshot, and measured 4.49x overhead. It therefore does not meet the <=3x promising criterion or the complete evidence contract.
* BlastEm was not installed and no build was available without introducing a separate emulator project; it was not integrated or modified.

The required evidence contract remains unchanged: ordered executed M68K PCs, opcode per occurrence, occurrence ordering, enough state for reaching definition, consumer occurrence, and no intervening writer. No static CFG, sampling, prediction, future scheduling, or lossy trace was substituted.

## Measurements

The same canonical ROM was used. Each backend was compared with its own no-trace run; absolute emulator FPS was not compared.

| BACKEND | BASELINE ms/frame | TRACE ms/frame | OVERHEAD | RECORDS/frame | PC | OPCODE | REGISTERS | PARITY |
|---|---:|---:|---:|---:|---|---|---|---|
| BizHawk 2.11.1 / GenPlus-gx, current Lua control | 17.00 | 621.00 | 36.53x | 9,386.52 | YES | YES | YES (selected A4/A5 at joins) | NOT_PROVEN |
| BizHawk 2.11.1 `GPGXTraceBuffer` candidate | not run (mechanically disqualified) | not run | n/a | managed `List<TraceInfo>`, per-exec callback | YES | YES (disassembly/raw bytes in formatted trace) | YES (M68K registers) | NOT_PROVEN |
| MAME 0.289 Genesis native debugger trace | 17.81 | 79.97 | 4.49x | 15,569.45 | YES | PARTIAL (disassembly; raw opcode field not emitted by tested trace) | NO (not emitted by tested standard trace) | NOT_PROVEN |
| BlastEm | unavailable | unavailable | unavailable | unavailable | UNKNOWN | UNKNOWN | UNKNOWN | NOT_RUN |

### BizHawk control receipt

This is the previously completed bounded 60-frame control; the long experiment was not repeated in this checkpoint.

* Executable: `C:\Dev\SegaThorTools\BizHawk-2.11.1-win-x64\EmuHawk.exe`.
* Version/core: BizHawk 2.11.1 / GenPlus-gx.
* Run: 60 frames from the existing QuickSave1 state, headless `view-mode none`; no throttle/vsync requirement.
* Command:

  ```text
  python src/tools/thor_evidence/auto67_runner.py --rom "C:\Github\Sega-Thor\local-roms\Beyond Oasis (USA).md" --emulator "C:\Dev\SegaThorTools\BizHawk-2.11.1-win-x64\EmuHawk.exe" --lua "C:\Github\Sega-Thor\src\tools\thor_evidence\capture\live_capsule.lua" --workers 16 --window 256 --worker-delay 0.003 --poll-interval 0.05 --max-frames 60 --capture-mode continuous --view-mode none
  ```

* No-global-prehistory baseline: p50 17 ms/frame, 0 global records.
* Continuous `event.on_bus_exec_any` prehistory: p50 621 ms/frame, 563,191 ordered records = 9,386.52/frame and 13,489.60 records/sec over the recorded 41.75-second wall run. The ratio is the per-frame p50 ratio 621/17 = 36.53x.
* Records carry ordered PC/opcode occurrences and selected A4/A5 snapshots at pending joins, preserving the current reaching-definition/no-intervening-writer semantics.

### BizHawk native trace inspection

Read-only source checkout: BizHawk tag `2.11.1`, commit `bdddf4a58aa1a022afb11dc73294a81a5aa7bbd5`.

Inspected symbols/files:

* `src/BizHawk.Emulation.Cores/Consoles/Sega/gpgx64/GPGX.ITraceable.cs`: `GPGXTraceBuffer.TraceFromCallback` reads `M68K PC`, calls the disassembler, formats M68K registers, and calls `Put`.
* `src/BizHawk.Emulation.Common/Base Implementations/CallbackBasedTraceBuffer.cs`: the `Sink` setter adds `TracingMemoryCallback` with `MemoryCallbackType.Execute`; the implementation stores a managed `List<TraceInfo>` and is documented as significantly slower than a direct implementation.
* `src/BizHawk.Emulation.Cores/Consoles/Sega/gpgx64/GPGX.IDebuggable.cs`: GenPlus-gx exposes managed memory callbacks and register reads.

Conclusion: the existing BizHawk facility satisfies fields at the formatting layer but still crosses a managed execute callback for each instruction; no native/batched facility was found or benchmarked.

### MAME native trace receipt

* Executable: `C:\Dev\SegaThorTools\Mame-0.289\mame.exe`.
* Driver: `genesis` (Genesis USA NTSC), MAME 0.289.
* Baseline command:

  ```text
  mame.exe genesis -cart "<absolute canonical ROM path>" -bench 1 -video none -sound none -nothrottle -skip_gameinfo
  ```

  Two wall timings were 1.0786885 s and 1.0582449 s per emulated second (average 1.0684667 s; 17.81 ms/frame at 60 Hz).

* Native trace command:

  ```text
  mame.exe genesis -cart "<absolute canonical ROM path>" -debug -debugscript "C:\Github\Sega-Thor\build\mame-trace-bench.cmd" -bench 1 -video none -sound none -nothrottle -skip_gameinfo
  ```

  The script contains `trace build/mame-trace-bench.log,0,noloop` and `go`. Two wall timings were 4.7918868 s and 4.8046682 s (average 4.7982775 s; 79.97 ms/frame). The log contains 934,167 ordered lines for one emulated second (15,569.45/frame; 934,167 records/sec) and is 25,153,240 bytes.

* Example native output starts with ordered PC/disassembly records such as `000214: bne $21c`, `000216: tst.w $a1000c.l`, `00021C: bne $29a`. The tested default trace does not emit a register snapshot or a raw opcode column. A debugger `tracelog` expression can reference opcode-space memory, but that expression is evaluated per trace action and the bounded field-action attempt was aborted because of its runaway overhead; it is not counted as a successful low-overhead result.

### Cross-backend parity

No emulator-specific savestate was reused. The BizHawk QuickSave1 cannot be loaded by MAME. A shared cold-reset/input interval reaching the preferred `0x0027BE ... 0x0027EC` dependency was not established in this bounded probe. Therefore no architectural parity claim or merged evidence is made: `parity_interval = NONE`, `mismatches = []`, and `STOP_BACKEND_EXECUTION_DIVERGENCE` was not asserted.

## Boundaries and acceptance

* Dispatcher, Worker, local-chain schema, proof gate, Cartographer, RAM Session Map, offline merge, current capture backend, Lua transport ring, RollingWindow, capsule limits, predecessor ring, persistence, MAP-1, Walker-1, SOURCE_OWNED, and C++ were untouched.
* `SOURCE_OWNED = 1,475,600 / 3,145,728`, delta 0.
* `final_sha =` publication commit returned with this receipt.
* `github_ci = PENDING_EXTERNAL_VERIFICATION`.
* `git diff --check = PASS` for the report-only publication diff.
* Source-limit: PASS; no governed source files changed (inventory not rerun).
* Focused tests and Debug/Release CTest: not rerun; this is a report-only probe and no production code changed.
* No production integration was attempted. Probe stops here; the current capture backend remains authoritative.
