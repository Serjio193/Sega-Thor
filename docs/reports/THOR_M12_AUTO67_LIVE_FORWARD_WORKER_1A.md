# M12 AUTO67 live-forward Worker 1A

**Result:** `PASS_LIVE_FORWARD_SINGLE_WORKER_FLOW_V1`
**Baseline:** `e49c1c405e55e707080f2d6581cd74b753e583fe`
**Scope:** developer-only BizHawk 2.11.1/GPGX main-M68K cartridge proof

The checkpoint proves one live-forward Worker can attach at the CPU's current
instruction boundary, record actual execution forward, seal a bounded immutable
FLOW_V1 result, release its lease, and attach again at later execution. The
production AUTO67 path, predecessor requirements, Cartographer, and
`SOURCE_OWNED` remain unchanged. This result stops at `READY_FOR_CARTOGRAPHER`;
it does not merge the segment into Cartographer or claim global provenance.

## Native boundary audit

The source audit was against BizHawk commit
`bdddf4a58aa1a022afb11dc73294a81a5aa7bbd5` and Genesis-Plus-GX submodule commit
`051d430d3d1b54625f9900c8f152d7f232e06daf`.

| Execution or state path | Source location | 1A handling | Runtime status |
|---|---|---|---|
| Main M68K instruction loop | `core/m68k/m68kcpu.c`, `m68k_run()`; fetch/dispatch around lines 337–392 | Begin before fetch; opcode recorded after fetch; actual `REG_PC` recorded after handler and trace processing | Exercised by the cartridge run |
| Direct delayed-IRQ handler path | `core/m68k/m68kcpu.c`, `m68k_set_irq_delay()`; direct jump-table call around line 288 | Uses the same begin/fetch/end boundary around its direct handler call | Instrumented. Repository search found no caller beyond the API declaration/definition, so this path was not runtime exercised |
| Synchronous vector transition | `core/m68k/m68kcpu.h`, `m68ki_jump_vector()` around line 1170 | Records vector and source/target PC; an in-flight instruction is marked faulted and its capture fails closed | Fault/exception behavior covered by focused test; no synchronous fault occurred in the gameplay capture |
| Asynchronous interrupt entry | `core/m68k/m68kcpu.h`, `m68ki_exception_interrupt()` around line 1422 | Records the actual vector and source/target PC as an exception event at the transition | Runtime observed vector `0x1E` in both depth segments |
| Address-error unwind | Address-error trap macro in `core/m68k/m68kcpu.h`; `oasis_lf_unwind()` | Emits an explicit faulted record and ends the current capture invalid | Fail-closed behavior covered by focused test |
| Reset and savestate load | `m68k_pulse_reset()` in `m68kcpu.c`; `GPGX.IStatable.cs`, immediately after `LoadStateBinary()` | Breaks the epoch; any active capture ends invalid before records from different state epochs can join | Epoch invalidation covered by focused test; runtime did not load a state |
| CPU STOP and HALT | `m68k_run()` instruction end and `m68k_pulse_halt()` in `m68kcpu.c` | STOP ends with `END_CPU_STOP`; an out-of-instruction HALT emits a stop event and seals; an in-instruction HALT ends at that instruction boundary | Focused tests cover both cases; runtime did not halt |
| Sega CD sub-CPU | `core/m68k/s68kcpu.c`, `s68k_run()`; direct dispatch around line 269 | Not instrumented. It is a separate CPU context and outside the cartridge main-M68K FLOW_V1 contract | Not invoked by the Beyond Oasis cartridge runtime; do not enable this prototype for Sega CD execution |

The only direct main-M68K instruction-handler dispatches found are in
`m68k_run()` and `m68k_set_irq_delay()`. Both are instrumented. The separate
`s68k_run()` belongs to Sega CD and is explicitly outside this proof. Debugger
register writes and stepping are not supported by this GPGX core
(`SetCpuRegister` and `Step` are unimplemented); state loads and resets are
covered by epoch breaks. There is no supported main-CPU instruction path in
this runtime that can execute without the shared boundary hook.

Control-flow depth counts actual transitions for Bcc taken/not-taken, BRA/BSR,
DBcc using the pre-instruction condition and post-instruction counter, JMP/JSR,
RTS/RTE/RTR, and exception/interrupt entry. STOP is an end reason, not a branch
depth increment. A synchronous fault inside an instruction marks the Worker
invalid and ends with `END_CAPTURE_ERROR`; a complete asynchronous exception
event is retained in the stream. The focused native tests cover branch
classification, exception boundaries, CPU stop, epoch invalidation, unsupported
nested execution, memory/retention limits, exact range copying, immutability,
generation, ACK, and reconnect.

## Data path and critical design limits

The native shared ring has 4096 fixed slots. Each completed main-M68K
instruction is recorded once as a 32-byte `FLOW_V1` record with stream sequence,
instruction sequence, PC, actual next PC, opcode/vector, kind flags, and
auxiliary data. Full D0–D7, A0–A7, PC, SR, USP, and ISP are stored only at ENTRY
and EXIT. Exception and CPU-stop events use the same sequence-ordered ring.

The configured prototype is one Worker, depth 20, and 64 KiB result memory.
`WORKER_COUNT`, `WORKER_DEPTH`, and `WORKER_MEMORY` are separate native API
parameters; the current runtime intentionally configures only one Worker. The
Worker request becomes pending until the next verified native instruction
boundary; there is no historical START queue. The CPU does not call Lua,
managed code, Python, disk, or Cartographer per instruction. At a terminal
boundary it performs a bounded native exact-range copy into the Worker’s
preallocated result buffer; only then does the state become COMPLETE. Hashing
and segment validation happen after capture, outside the instruction hot path.

`MEMORY` is the result-buffer budget, independent of depth. The shared ring is
also a retention bound: if the capture cannot retain the complete range, the
reason is `END_RETENTION_LIMIT`, and the copied result cannot pass validation.
No memory read/write observations are added in 1A. A valid result proves only
the observed ENTRY state, ordered records, actual control-flow outcomes, EXIT
state, identity, and termination reason. It does not prove reaching definitions,
prior value origin, memory dependencies, or global causality.

## Concrete runtime proof

The bounded BizHawk runtime used the user-local canonical ROM
`Beyond Oasis (USA).bin`, SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`, without
savestate or game input. The matching raw `gpgx.wbx` SHA-256 was
`21b14e22e08107a2b3d9f0e30582f6bca7621f303483713ec8eed84065eff570` in both
the build output and disposable BizHawk test install. Runtime run ID:
`1789633798`.

1. `FREE → CAPTURING`: Worker 0 accepted capture 1 / generation 1, run 1789633798,
   epoch 5 at ENTRY PC `0x3818`, stream 1, instruction 1, flow count 0. ENTRY
   state SHA-256 is `b8d85c327142e7dad4817e175957de0929b2ce0f65f600e0e9d570a1595b83c3`.
2. Twenty flow transitions later it exited at PC `0x2A08`, stream cursor 125,
   instruction cursor 124, flow cursor 20. The segment contains 123 completed
   instruction records and one asynchronous exception event, 124 records in
   total, and consumes 4256 of 65536 bytes. EXIT state SHA-256 is
   `0e561fbbab77e7732c92df46ebdc3f76c940fe8b321767f566c7672f13dd5b27`.
   Termination is `END_DEPTH_LIMIT`; FLOW_V1 content SHA-256 is
   `6122f173be5e7bb7d08a27c12e0a871be4d9e6e3e4b6defd4a2cd61dfd2f3239`.
3. `COMPLETE → immutable result`: the ring advanced from stream 9598 to
   590388, wrapping its 4096 slots. The first segment’s full content hash
   matched before and after the wrap. Bulk ACK for capture 1 / generation 1 /
   run 1789633798 / epoch 5 was accepted, returning Worker 0 to FREE.
4. `FREE → later CAPTURING`: capture 2 / generation 2 attached at the later live
   execution cursor 649310, instruction 649245, flow 297031, ENTRY PC `0x3A82C`.
   It ended at stream 649441, instruction 649375, flow 297051, EXIT PC
   `0x21E2`; it contains 130 instruction records plus one async exception
   event. It consumes 4480 of 65536 bytes, ends at `END_DEPTH_LIMIT`, and hashes
   to `e4cd0deba53292075e276a4e3af52bd23560f14afbe2e1d80b12da6a488099ac`.
   Its before/after immutable hash matched and its exact ACK was accepted.
5. `MEMORY` termination: a separate epoch-6 capture used a 512-byte budget and
   depth limit 100000. It entered and exited at `0x3A92C`, reached 3 flow
   transitions, retained 6 records, and consumed 480/512 bytes. It ended with
   `END_MEMORY_LIMIT`, remained immutable, and hashes to
   `6d26aa72e2c5c593867ff42707fe18e0966e48d81c0597fd2fcda8d2ca1066a5`. Its
   exact ACK was accepted.
6. Every validated result is classified `READY_FOR_CARTOGRAPHER`; no adapter
   or Cartographer merge was invoked. The emulator continued to frame 452 after
   the capture sequence. The complete ENTRY/EXIT arrays, all records, and
   canonical hash envelopes are in the JSON receipt.

The first segment’s 20 observed control-flow rows are:

| Stream / instruction | PC → next PC | Opcode/vector | Outcome flags |
|---:|---|---|---|
| 1 / 1 | `0x03818 → 0x03810` | `0x66F6` | taken |
| 3 / 3 | `0x03818 → 0x03810` | `0x66F6` | taken |
| 5 / 5 | `0x03818 → 0x03810` | `0x66F6` | taken |
| 7 / 7 | `0x03818 → 0x03810` | `0x66F6` | taken |
| 8 / 7 | `0x03810 → 0x01F76` | vector `0x1E` | asynchronous exception |
| 12 / 11 | `0x01F8A → 0x02112` | `0x6700` | taken |
| 14 / 13 | `0x0211A → 0x0211E` | `0x6700` | not taken |
| 19 / 18 | `0x0212A → 0x0212E` | `0x6F00` | not taken |
| 25 / 24 | `0x02144 → 0x02148` | `0x6700` | not taken |
| 29 / 28 | `0x02156 → 0x027FC` | `0x6100` | BSR taken |
| 34 / 33 | `0x02812 → 0x02814` | `0x66F6` | not taken |
| 74 / 73 | `0x02896 → 0x0215A` | `0x4E75` | RTS |
| 77 / 76 | `0x02160 → 0x0216A` | `0x6700` | taken |
| 80 / 79 | `0x02178 → 0x0217C` | `0x6600` | not taken |
| 83 / 82 | `0x02188 → 0x02992` | `0x6100` | BSR taken |
| 86 / 85 | `0x029A2 → 0x029A4` | `0x66F6` | not taken |
| 99 / 98 | `0x029C0 → 0x029C4` | `0x6700` | not taken |
| 109 / 108 | `0x029DA → 0x029DE` | `0x6700` | not taken |
| 115 / 114 | `0x029EA → 0x029F0` | `0x6700` | taken |
| 124 / 123 | `0x02A04 → 0x02A08` | `0x6700` | not taken |

These rows include an interrupt event between instruction records. Stream
sequence therefore advances for both instructions and explicit events; the
instruction sequence advances only for instructions. All IDs and cursor
relationships are checked by the runtime validator.

## Performance and validation

For 120 host-timed frames, baseline was p50 17 ms / max 18 ms; recorder-only
was 17/17 ms; recorder plus Worker was 17/17 ms with 120 captures. These values
are rounded to milliseconds at the 60 Hz frame scale, so they rule out a large
frame regression but cannot establish a sub-millisecond delta. The host bulk
copy/export path reported 4 ms on the first pre-completion read and 0 ms on
subsequent reads/exports. Native `copy_duration_ns` was 0 for each segment even
after switching to monotonic/QPC clocks; treat native sealing-copy duration as
unmeasured, not as zero work.

Validation on the final code and matching runtime artifact:

| Check | Result |
|---|---|
| Focused `oasis_live_forward_worker_test` | PASS |
| Debug CTest | 199/199 PASS. One earlier `oasis_smoke` run exited `0xc0000409`; it passed alone and the complete rerun passed |
| Release CTest | 199/199 PASS |
| GNU/Linux worker build and focused CTest | PASS |
| Waterbox `make release` | PASS |
| Targeted BizHawk `BizHawk.Client.EmuHawk.csproj` Release build | PASS, zero errors; existing SharpCompress NU1902 warning |
| Full BizHawk solution build | Four unrelated .NET Framework 4.8 test/tools projects lack the local targeting pack; EmuHawk target built successfully |
| Source file limit | PASS, 686 governed source files at or below 500 lines |
| `git diff --check` | PASS |
| `SOURCE_OWNED` | 1,475,600 / 3,145,728, delta 0 |
| Cartographer / predecessor / production AUTO67 | unchanged |

Patch SHA-256 values:

| Patch | SHA-256 |
|---|---|
| `bizhawk-2.11.1-live-forward-worker-1a.patch` | `debd66ff5d340a4dc8506b1a9c1b67030eb1050dabfd4858b9059fd10454ff0e` |
| `genesis-plus-gx-live-forward-worker-1a.patch` | `dd13782ce2dc2261fe8cd61561367c240aa06f9e3f8c6422b5d08ddb8bf974ca` |

Both patches apply in reverse with `git apply --reverse --check` to the exact
tested modified checkouts. The first contains the BizHawk host bridge and
Waterbox source-list changes; the second contains the GPGX native recorder,
Worker, compatibility ring projection, and M68K hooks. Neither contains a
compiled binary. The final repository commit SHA and exact GitHub Actions run
are appended after push.

## Architecture review and stop gate

The design risk was the previously uninstrumented direct dispatch in
`m68k_set_irq_delay()`, plus exception, reset, and externally asserted HALT
transitions. The implementation instruments that direct route, treats
synchronous faults and nested/unsupported execution fail-closed, records
asynchronous exception and halt events, and breaks identity on reset/state
load. The tested CPU never waits on external analysis; native copy-on-completion
is synchronous but bounded by the configured segment/ring budget.

1A proves one serialized active Worker. It does not prove parallel or
overlapping Workers, Sega CD execution, Cartographer ingestion, memory access
provenance, or performance below millisecond frame timing. Stop here; do not
start the multi-Worker checkpoint automatically.

## Publication

Implementation commit `321a4f173797ab13887cf6549cb701fd07e8620c` was pushed to
`main`. GitHub Actions CI run
[35201139430](https://github.com/Serjio193/Sega-Thor/actions/runs/35201139430)
completed with `success` for that exact SHA. The final documentation commit's
source and runtime changes are identical; the documentation update is a
separate commit.
