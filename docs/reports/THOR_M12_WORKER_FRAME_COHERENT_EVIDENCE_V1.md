# M12 W2.1 frame-coherent Worker evidence V1

Checkpoint: `PASS_WORKER_FRAME_COHERENT_EVIDENCE_V1`

This acceptance preserves `PASS_NATIVE_BUS_EVENT_SIDEBAND_V1` and
`PASS_WORKER_ACTIVE_RESOURCE_CLASSIFICATION_V1`. It adds no graphics
semantics and no Z80 implementation.

## Source-of-truth and authoritative frame identity

The new native delta is reproducible from the incremental patches below,
applied after the accepted 1A/1B and W1 patch chain:

| Patch | SHA-256 |
|---|---|
| `tools/bizhawk-native-ring/bizhawk-2.11.1-worker-frame-coherent-v1.patch` | `9B1FA15949BFE842CEDABF3E4FC51A20BA12B19612A0DF3F46F46DE43C023A11` |
| `tools/bizhawk-native-ring/genesis-plus-gx-worker-frame-coherent-v1.patch` | `8380072718C03D9F8BDE96EFE2D21D79AC8523C0EDBE95CD6DCC5EEE85A7EB87` |

The materialized source contains no W2.1-only native change outside those
patch paths. The GPGX patch adds `entry_frame` and `exit_frame` to
`oasis_lf_result`; the Worker captures them at `lf_boundary()` and
`lf_seal_worker()`. The managed patch exports them as metadata fields 10 and
11, returning 22 fields; the fixed 32-byte `oasis_lf_record` is unchanged.

The authoritative counter is W1's `uint64_t frame_number` in
`core/debug/live_forward_trace.c`. It starts by static zero initialization,
is incremented only by `oasis_lf_frame_boundary()`, and the same value is
encoded in the existing FRAME_BOUNDARY record. `oasis_lf_epoch_break()` sets
`frame_number = 0` before incrementing `runtime_epoch`. Therefore identity is
always `(run_id, epoch, frame)`, never frame alone.

## Toolchain and builds

The repository-supported sysroot is
`build/bizhawk-2.11.1-src/waterbox/sysroot`; required executable:
`waterbox/sysroot/bin/musl-clang`. It is the recovered musl sysroot from musl
commit `2063abc4e16c84218757b1db10d3cdf9f36ef3f8`, Ubuntu 24.04 WSL2 clang
18.1.3 (`llvmorg-18.1.8` libcxx), SHA-256
`9125791649F7CB2409B80F8282C20C2336E80C70F0BA820E08C4E251A78DC14F`.
No system clang was substituted.

Debug and Release WBX builds passed from the patched GPGX materialization.
`nm -u` reported only the normal `_DYNAMIC` entry. Release exports include
`gpgx_live_forward_result_info`, `gpgx_live_forward_result_copy`, and
`gpgx_live_forward_epoch`. Artifacts:

| Configuration | Size | SHA-256 |
|---|---:|---|
| Debug | 4,050,136 | `20F274DD1DF12EF0C26A5893F7558CCCB0350466280D90314F11FE1B0C55B622` |
| Release | 5,678,048 | `6A64CB2236E25CBCDB80A1D8C89B812FEA040773D4CD56650D74F647E1412F9C` |

The managed Release host binaries built, including EmuHawk, Cores and Common.
The full solution command also attempted unrelated .NET Framework 4.8 test
projects and stopped on missing local 4.8 reference assemblies (`MSB3644`);
the host artifacts needed by the runtime were produced successfully.

## Frame-coherent semantics and tests

`w2_frame_coherence.py` assigns exact identity to all facts in a segment when
`entry_frame == exit_frame` and no contained FRAME_BOUNDARY contradicts it.
For a changed pair it partitions only at contained boundary records whose
monotonic marker reaches `exit_frame`; otherwise the segment and its facts are
`MULTI_FRAME_UNRESOLVED` / `frame=UNRESOLVED`. No per-bus native lookup was
added. W2 facts, exact relations, VDP register writes, DMA programming and
bounded candidates carry `run_id`, `epoch` and `frame`.

The A–I suite passed: `python -m unittest
tests/w2_active_resource_classification_test.py
tests/w2_frame_coherent_evidence_test.py
tests/live_forward_scaling_audit_test.py` → **19/19**. It covers single
frame, partitioned transition, missing boundary, epoch reset, run/frame
non-join, epoch/frame non-join, DMA inheritance, old W2 semantic equality and
deterministic output.

## Fresh real BizHawk runtime

The fresh isolated install was
`C:\Dev\SegaThorTools\BizHawk-m12-w2-1-frame-coherent-20260920`, with the
accepted W1 install preserved separately. The run used the canonical ROM
`build/m12-auto2-rom/usa/Beyond Oasis (USA).md`, one Worker, depth 100 and 100
audited segments. Receipt/report:
`build/m12-w2-1-acceptance/fresh-short-w2-1/count-1/receipt.json` and
`build/m12-w2-1-acceptance/w2-frame-coherent-evidence.json`.

| Measure | Result |
|---|---:|
| Instruction records | 32,272 |
| BUS events | 23,595 |
| Observed FRAME_BOUNDARY records in slices | 0 |
| Worker segments | 100 |
| `SINGLE_FRAME` segments | 100 |
| Exactly partitioned multi-frame segments | 0 |
| `MULTI_FRAME_UNRESOLVED` segments | 0 |
| Frame range observed | 120–264 |
| Ring capacity / wraps | 4,096 / 1,502 |
| Ring overwrite / retention failures | 0 / 0 |
| Runtime errors / invalid / dropped captures | 0 / 0 / 0 |
| Worker wall / throughput | 5.006 s / 19.976 segments/s |
| Runtime wall | 11.328 s |

Bus events per instruction were measured as mean `1.766225`, p95 `2`, max
`30`. Primitive counts were RAM_READ `10,208`, RAM_WRITE `12,681`,
ROM_DATA_READ `12`, Z80_WINDOW_READ `220`, Z80_WINDOW_WRITE `440`,
VDP_CONTROL_WRITE `31`, and VDP_DATA_WRITE `3`; no fetch pollution was
introduced. VDP decoding produced 22 register writes, 3 control commands, 3
CPU-programmed DMA operations and 3 incomplete command cases. It produced 581
exact relations and 439 bounded candidates; shadow-SAT writes were 0. No
sprite/music semantics were assigned.

Runtime witnesses carry exact frame identity. The run identity is
`run_id=1789946992`, `epoch=3`:

| Fact | stream | instruction | PC | address | value | width | domain | frame |
|---|---:|---:|---:|---:|---:|---:|---|---:|
| ROM data read | 3,565,353 | 2,368,894 | `0x3B7CC` | `0x3BD86` | `0x0` | 16 | ROM | 246 |
| 68K RAM write | 1,740,278 | 1,155,183 | `0x1F76` | `0xFF0BDE` | `0x360E` | 16 | 68K_RAM | 120 |
| VDP mapped write | 3,444,714 | 2,285,245 | `0x2288` | `0xC00004` | `0x8124` | 16 | VDP | 237 |
| Z80-window write | 1,740,711 | 1,155,451 | `0x60310` | `0xA00017` | `0xFF` | 8 | Z80_WINDOW | 120 |

No shadow-SAT write was observed. DMA facts inherit exact frames 240, 258 and
264 for the three VRAM operations; `transfer_observed=false` and
`CPU_PROGRAMMING_ONLY` remain explicit.

The accepted S1–S8 artifacts were read only. No coherent same-run range was
available, so `cross_links=[]` and `CROSS_RUN_LINK=NOT_ALLOWED`.

## Regression and measured cost

Normalized comparison with the accepted W2 report is equal for raw W1 bytes,
counts, instruction accesses, relations, VDP/DMA, candidates, witnesses,
Z80/shadow reports and errors. The accepted W1 pre/post semantic A/B remains
the control-flow oracle; no instruction PC/opcode/next-PC, branch, exception,
terminal or segment-boundary semantics are changed. `SOURCE_OWNED delta=0` and
Worker scheduling is unchanged.

Against the accepted W2 one-Worker run, W2.1 measured:

| Measure | W2 baseline | W2.1 | Delta |
|---|---:|---:|---:|
| Runtime wall | 11.484 s | 11.328 s | -0.156 s |
| Worker wall | 5.006 s | 5.006 s | 0 |
| Native records generated | 6,152,766 | 6,152,766 | 0 |
| Total segment bytes | 1,819,744 | 1,821,344 | +1,600 |
| Raw retained record bytes | 1,790,944 | 1,790,944 | 0 |
| Capture failures | 0 | 0 | 0 |

The +1,600 bytes is the measured 16-byte-per-segment result metadata delta for
100 segments, not a projected per-instruction cost.

Final checkpoint: `PASS_WORKER_FRAME_COHERENT_EVIDENCE_V1`. No commit or push
was made.
