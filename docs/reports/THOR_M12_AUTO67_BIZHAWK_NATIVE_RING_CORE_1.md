# M12-AUTO67-BIZHAWK-NATIVE-RING-CORE-1 — native recorder core proof

**Baseline:** `5856063d0b95a82822aaec30ceca863dab18fb74`
**Classification:** `PASS_NATIVE_RING_CORE`

This checkpoint proves the continuous native recorder in an isolated custom
GPGX core. It does not integrate the ring into AUTO67 and does not implement
freeze/read managed transport.

## Scope and source identity

A is the locally rebuilt unmodified GPGX from Phase‑0A‑R1. B is the same source
with only the existing native-ring recorder patch applied to `m68kcpu.c`,
`trace_ring.[ch]`, the GPGX Makefile, and the C export surface. The C# bridge
hunk was deliberately not applied.

* BizHawk: `bdddf4a58aa1a022afb11dc73294a81a5aa7bbd5`.
* Genesis-Plus-GX: `051d430d3d1b54625f9900c8f152d7f232e06daf`.
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
Lua/managed call, string work, allocation, I/O, synchronization, map lookup,
register snapshot, or game-state mutation. The performance Lua script contains
no `event.on_bus_exec_any`.

## Cold-reset emulation parity

A and B were launched independently from clean ROM power-on with the exact
Phase‑0A‑R1 schedule: steps 1–30 neutral, 31–60 `Right`, 61–300 neutral. The
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

The stock and native-ring checkpoint files are byte-for-byte identical. First
divergence: none. Result: **EMULATION PARITY PASS**.

## Ring and opcode sanity

A bounded native host diagnostic linked the exact patched `trace_ring.c` and
recorded 8192 ROM-derived records, then copied the retained window. Result:

```text
count=4096 latest=8192 ordered=PASS wrap=PASS pc_valid=PASS opcode_rom_match=PASS reset=PASS
```

This proves capacity, monotonic sequences, ordered wrapped copy, valid ROM PCs,
canonical ROM opcode equality, and reset without any managed or Lua execution
callback. During the 60-frame core run the ring is demonstrably saturated; the
future managed bridge will expose the exact latest sequence for an exact
records/frame total. For this checkpoint the externally measurable retained
lower bound is `4096 / 60 = 68.27 records/frame`.

## Performance experiment

The same bounded 60-frame Lua driver was run 12 times for A and B. It only
advances frames and applies the deterministic input schedule; it does not use
`event.on_bus_exec_any`. Timings are the measured in-process `os.clock` duration
of the 60-frame loop.

| MODE | RING | PER-INSTRUCTION LUA | P50 (ms) | P95 (ms) | P99 (ms) | MAX (ms) | RECORDS/FRAME | PARITY |
|---|---|---|---:|---:|---:|---:|---:|---|
| A — rebuilt stock | NO | NO | 981 | 982 | 982 | 982 | n/a | PASS |
| B — rebuilt native ring | YES, 4096 | NO | 981 | 983 | 983 | 983 | ≥68.27 retained lower bound | PASS |
| C — authoritative Lua continuous reference | NO | YES | 621 | 707 | 715 | 715 | 9,386.52 | prior CAPTURE-PERF receipt |

`NATIVE_OVERHEAD_RATIO = B.p50 / A.p50 = 1.000x`, below both the `2.0x` gate
and the `1.5x` strong-result threshold. These are measured values; no expected
17 ms value is claimed. Mode C reuses the already-published CAPTURE-PERF
numbers and was not rerun.

## Optional native-core self-state smoke

The custom core independently saved a new state after 120 frames, advanced 20
frames, reloaded its own state, replayed the same 20 `Right` frames, and matched
frame/PC/A4/A5/RAM (`equal=true`). Cross-WBX states were not used.

## Validation

* Custom native-ring Waterbox GPGX build: **PASS**.
* Cold-reset A/B checkpoint equality: **PASS**.
* Ring count/order/wrap/reset diagnostic: **PASS**.
* ROM opcode sanity: **PASS**.
* Native-core self-savestate smoke: **PASS**.
* Native-ring overhead ratio: **1.000x**, **PASS**.
* Debug CTest: **195/195 PASS**.
* Release CTest: **194/195 PASS**; existing
  `oasis_re_import_gpgx_coverage_self_test` fails with `invalid JSON at 0:
  missing value`.
* Source-limit: **PASS**.
* `SOURCE_OWNED = 1,475,600 / 3,145,728`, delta 0.
* `git diff --check`: **PASS**.
* `github_ci`: `PENDING_EXTERNAL_VERIFICATION`.

Stop after this checkpoint. Freeze/read managed integration remains future work.
