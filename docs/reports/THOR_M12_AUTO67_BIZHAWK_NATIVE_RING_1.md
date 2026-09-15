# M12-AUTO67-BIZHAWK-NATIVE-RING-1 — Same continuous prehistory, native GPGX ring

**Baseline:** `3654f5220ea83ea60c4b991f5e6659fb7a14390c`
**Classification:** `STOP_CUSTOM_BIZHAWK_BUILD_FAILURE`
**Scope:** experimental BizHawk 2.11.1 patch and bounded validation. The normal AUTO67 harness was not changed.

## Implemented patch

The exact inspected BizHawk source is tag `2.11.1`, commit
`bdddf4a58aa1a022afb11dc73294a81a5aa7bbd5`; its Genesis-Plus-GX submodule is
`051d430d3d1b54625f9900c8f152d7f232e06daf`.

Files patched by the reproducible recipe:

* BizHawk `src/BizHawk.Emulation.Cores/Consoles/Sega/gpgx64/LibGPGX.cs`
* BizHawk `waterbox/gpgx/Makefile`
* BizHawk `waterbox/gpgx/cinterface/cinterface.c`
* Genesis-Plus-GX `core/m68k/m68kcpu.c`
* Genesis-Plus-GX new `core/debug/trace_ring.h` and `trace_ring.c`

Patch files and SHA-256:

* `tools/bizhawk-native-ring/bizhawk-2.11.1-native-ring.patch`:
  `A6E303140D5A85E11C604A0CFDFAD42413395C49E4A70B1415C1AA8A627609E5`
* `tools/bizhawk-native-ring/genesis-plus-gx-native-ring.patch`:
  `139C2A15D27DDDED659BED964349076A3E2EFC91B8B43BDFBD350AA8200D85EB`

The parent and nested patches both apply cleanly to their exact source commits.
No BizHawk binary is committed. Custom binary hash: **UNAVAILABLE** because the
required Waterbox compiler was absent.

## Ring contract

Each record is a fixed 16-byte native structure:

```c
uint64_t sequence;
uint32_t pc;
uint16_t opcode;
uint16_t reserved;
```

Capacity is 4096 records. At the M68K execution point the patch saves the
pre-fetch PC, fetches the 16-bit opcode, then performs a sequence increment and
one ring-slot write. The hot path has no allocation, strings, disassembly,
managed object, Lua call, register/RAM/VDP mutation, branch, interrupt, timing,
or savestate-visible state change.

The bulk ABI is:

```text
gpgx_native_trace_ring_latest()
gpgx_native_trace_ring_count()
gpgx_native_trace_ring_copy(start_sequence, destination, capacity)
gpgx_native_trace_ring_reset()
```

Copy returns records in sequence order, clamped to the retained window. The
managed declarations are an evidence-format adapter surface only; no current
AUTO67 launcher or Lua path selects this custom core.

## Required comparison table

| MODE | PER-INSTRUCTION LUA CALLBACK | RECORDS/FRAME | BULK COPIES | P50 | P95 | P99 | MAX | SLICE HASH | PROOF |
|---|---|---:|---:|---:|---:|---:|---:|---|---|
| A — stock BizHawk, no global prehistory | NO | 0 | 0 | 17 ms | 17 ms | 26 ms | 26 ms | n/a | prior 60-frame control |
| B — stock BizHawk, continuous `event.on_bus_exec_any` | YES | 9,386.52 | 0 | 621 ms | 707 ms | 715 ms | 715 ms | stock slice available in prior receipt | prior 60-frame control |
| C — patched BizHawk, native ring | NO (source path) | not run | not run | not run | not run | not run | not run | not run | blocked by custom build failure |

A and B are the existing bounded 60-frame QuickSave1 control on the canonical
USA ROM. C was not falsely measured: BizHawk's Waterbox build stopped before
compilation because `waterbox/sysroot/bin/musl-clang` and `musl-gcc` are
absent; the C# project also requires unavailable .NET Framework 4.8 reference
assemblies. Consequently no custom binary, parity hash, reaching-definition
comparison, or performance gate result is claimed.

The standalone host compiler test exercised ring insertion, 4096-record wrap,
ordered bulk copy, and reset; result: **PASS**. This does not substitute for
the required emulator parity run.

## Parity and evidence status

Required matched run was not completed. Therefore:

* `STOCK_LUA_SLICE_HASH = N/A`
* `NATIVE_RING_SLICE_HASH = N/A`
* `parity_interval = NONE`
* `reaching_definition_comparison = NOT_RUN`
* `STOP_NATIVE_RING_EVIDENCE_DIVERGENCE` was not asserted.

No targeted-burst mode, proof logic, predecessor resolver, Worker, Dispatcher,
Cartographer, Lua transport ring, RollingWindow, capsule, Session Map, offline
merge, MAP identity, or SOURCE_OWNED path was changed.

## Validation

* Native ring standalone compile and wrap/reset test: PASS.
* Parent patch clean-apply check: PASS.
* Genesis-Plus-GX nested patch clean-apply check: PASS.
* BizHawk native build: BLOCKED — missing Waterbox `musl-clang/musl-gcc`.
* BizHawk C# project build: BLOCKED — missing .NET Framework 4.8 reference assemblies.
* Focused AUTO67 regressions: not rerun; no Sega-Thor production code changed.
* Debug/Release CTest: not rerun; no Sega-Thor production code changed.
* Source-limit: PASS for the changed Sega-Thor files; only documentation, patch,
  and recipe files were added.
* `SOURCE_OWNED = 1,475,600 / 3,145,728`, delta 0.
* `git diff --check = PASS`.
* `github_ci = PENDING_EXTERNAL_VERIFICATION`.
* `final_sha =` publication commit returned with this receipt.

Stop here. The stock BizHawk capture backend remains authoritative until a
toolchain-equipped parity and performance run proves the native ring.