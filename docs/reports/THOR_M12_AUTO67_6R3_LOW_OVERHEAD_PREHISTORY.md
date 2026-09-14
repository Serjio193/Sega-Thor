# THOR M12 AUTO67.6R3 — Low-Overhead Prehistory Source Audit

Date: 2026-09-14
Baseline: `b13a14add1f574f8ea6e5f1a6dae103cf999257c`
Result: **NEGATIVE — no existing low-overhead BizHawk source accepted**

## Scope and stop gate

This checkpoint audits existing execution sources before adding another capture
mechanism. The scope is only the AUTO67.6R2 A4/A5 reaching-definition contract:
exact consumer occurrence, epoch identity, contiguous producer-to-consumer
interval, no intervening register write, static producer semantics, and
fail-closed gaps or unsupported writers.

No AUTO68 work, graph merge, chain-depth expansion, `SOURCE_OWNED` change, or
ring-capacity increase was made. The R2 implementation and its positive canary
proof remain unchanged. The stop gate is negative because every existing
low-overhead candidate is either not connected to BizHawk QuickSave1 or does not
provide the exact ordered runtime evidence required by the contract.

## Candidate audit

| Existing source | What it can provide | Measurement / inspection | Decision |
|---|---|---|---|
| BizHawk `event.on_bus_exec_any` | Exact per-instruction PC order and the R2 canary interval | Real 120-frame QuickSave1 run: 1,126,282 callback records, 9,385 records/frame average, p50 frame 623 ms, max 679 ms, 120/120 frames over 50 ms | **Reject**: complete but not gameplay-safe |
| Targeted BizHawk `event.on_bus_exec` hooks | Selected-PC evidence only | Real six-target predecessor diagnostic: max 778 ms, 59 frames over 50 ms. One-slot normal run: max 25 ms, but no predecessor capture was installed and no chain step was possible | **Reject**: either stalls or lacks the required interval |
| BizHawk TraceLogger | Operator trace window | Installed 2.11.1 Lua contract exposes only `client.opentracelogger()`; no Lua/native machine-readable record stream or bounded handoff is exposed to this repository | **Reject**: no usable evidence source |
| Existing Evidence Engine / capsule path | Sparse discovery, bounded capsules, worker-side materialization | It receives status/capsule results after capture; it does not receive complete block order or all instruction boundaries | **Reject**: cannot prove R2 interval completeness |
| External GPGX `HOOK_CPU` block hook | Native instruction/block boundary in a separate libretro core | Existing developer-only bridge and `cpu_block_hook` are present in the external checkout; prior 600-frame shadow/native proofs complete with exact comparisons, but use cold deterministic libretro scenarios, not BizHawk QuickSave1. The current repository has no BizHawk bridge, state import, or bounded prehistory export for it | **Reject for AUTO67.6R3**: not the authoritative live path |
| Existing hybrid basic-block translation | Fixed developer-only translated blocks and shadow comparisons | Existing M11 proofs cover selected static blocks and preserve GPGX timing/hardware ownership; they do not log generic executed block identity/order, branch boundaries, interrupts, or missing-block failures for AUTO67 | **Reject**: insufficient generic evidence |
| Native helper/plugin path | Potential place for a cheap ring | No existing BizHawk plugin/helper integration was found; adding one would be a new architecture, not reuse | **Reject / out of scope** |

## Why increasing the ring is not a repair

The R2 ring was bounded at 4096 records and overwrote 1,122,186 older records
in the accepted 120-frame proof. The measured cost was the Lua callback executed
for every instruction, including the first frame before any lease. A larger ring
would retain more data but would not reduce callback frequency; it would also
increase snapshot/freezing work. This is therefore not a capacity problem.

## R2 parity retained

The real R2 QuickSave1 canary remains the only accepted reaching-definition
proof:

```text
0x002234  LEA $00FF134C.L,A5  ->  0x0027EC MOVE.W (A5),-4(A4)
0x0027BE  LEA $00C00004.L,A4  ->  0x0027EC MOVE.W (A5),-4(A4)
```

The consumer occurrence was exact, both intervals were contiguous, no
intervening writes were observed, and producer semantics were statically
decoded from the canonical ROM. The performance acceptance was not met:
`max=679 ms`, `p50=623 ms`, and `>50 ms=120/120`.

## Exact mechanism boundaries

The installed BizHawk API definition is
`C:\Dev\SegaThorTools\BizHawk-2.11.1-win-x64\Lua\_docs_luacats\event.d.lua`.
It labels `event.on_bus_exec_any` CPU-intensive and exposes callback arguments
`(addr, val, flags)`. The only TraceLogger Lua entry in
`client.d.lua` is `client.opentracelogger()`, which opens the UI and does not
publish records to Python or a bounded ring.

The native block hook is outside the BizHawk process. It is implemented in the
external developer-only checkout at
`C:\Github\Genesis-Plus-GX-instrumented\core\debug\cpuhook.{c,h}` and
`core\m68k\m68kcpu.c`, with the repository-side bridge in
`src/tools/hybrid/gpgx_bridge.c`. It is useful evidence that a native hook is
technically possible, but it is not an existing AUTO67 live source. Connecting
it to BizHawk would require a new upstream integration and new exact
block/order/interrupt contracts, both outside this checkpoint.

## Tests and artifacts

- Existing R2 real BizHawk/native-window proof: return code 0; 16 workers,
  120 frames; raw backlog `NONEXISTENT`; queue drops and DB errors `0`.
- R3 rerun: `python -m unittest discover -s tests -p
  'thor_evidence_auto67*.py' -q` — 48 passed.
- R3 rerun: direct `cmake -P tests/check_file_limits.cmake` — passed,
  647 governed files <=500 lines.
- R3 rerun: Release CTest executable set — 67/67 passed when the stale
  `project_file_line_limit` registration was excluded; the direct check above
  passed independently.
- The stale Debug CTest directory `build-m1157-debug-gpgx` is incomplete: 46
  tests were `Not Run` because Debug executables are absent. This is a build
  directory limitation, not a source failure; no rebuild was needed for this
  documentation-only checkpoint.
- `git diff --check` — passed.
- The focused R3 audit found no source or test implementation to change.
- No code, database, emulator, ROM, or user artifact was modified.

This is a deliberate negative result. The next implementation, if separately
authorized, must supply a native/block-level source attached to the actual
BizHawk execution path and preserve the R2 fail-closed reconstruction contract.
