# M11.49 — Mechanical primitive family closure

Result: `NATIVE_MECHANICAL_PRIMITIVE_FAMILY_PROVEN`

Date: 2026-09-09
Baseline commit: `678534862ad16be7cc1627fe4ed5008f53c7365f`
Authoritative checkpoint aggregate:
`251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d`
Video SHA-256: `5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`
Canonical ROM SHA-256:
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`
Pinned GPGX DLL SHA-256:
`140c00fc7475cf22ca22415d65dfc7ffc66523de128454f9994e7ab77d9826fd`

## Baseline gate

The M11.48 `MECHANICAL_PRIMITIVE_NATIVE` authoritative 600-frame run was
reproduced twice before attribution. Both runs matched exactly:

| metric | M11.48 baseline | M11.49 baseline A/B |
| --- | ---: | ---: |
| total guest instructions | 6,488,773 | 6,488,773 |
| generated translated | 6,237,985 | 6,237,985 |
| mechanical primitive | 3,780 | 3,780 |
| interpreter | 247,008 | 247,008 |
| registered ranges | 587 | 587 |
| boundary yields | 150,135 | 150,135 |
| interrupted resumptions | 288 | 288 |
| original starts inside translated ranges | 0 | 0 |

The checkpoint aggregate and video hash above also matched in both runs.

## Exact family contracts

Canonical ROM bytes and decoder evidence establish one generic resumable loop
shape: a body instruction, `DBF` with displacement `0xFFFC`, and a continuation
PC. The registry is metadata only; the executor contains no ROM-PC branches.

| name | body | loop | continuation | exact form | registers | width |
| --- | ---: | ---: | ---: | --- | --- | ---: |
| `MEMORY_COPY_003A0C` | `0x003A0C` `12DA` | `0x003A0E` `51CA FFFC` | `0x003A12` | `MOVE.B (A2)+,(A1)+`; `DBF D2` | source A2, destination A1, counter D2 | 1 |
| `MEMORY_COPY_00389E` | `0x00389E` `12DA` | `0x0038A0` `51C8 FFFC` | `0x0038A4` | `MOVE.B (A2)+,(A1)+`; `DBF D0` | source A2, destination A1, counter D0 | 1 |
| `MEMORY_CLEAR_WORD_0003F0` | `0x0003F0` `4258` | `0x0003F2` `51C8 FFFC` | `0x0003F6` | `CLR.W (A0)+`; `DBF D0` | address A0, counter D0 | 2 |
| `MEMORY_CLEAR_BYTE_061266` | `0x061266` `421D` | `0x061268` `51C8 FFFC` | `0x06126C` | `CLR.B (A5)+`; `DBF D0` | address A5, counter D0 | 1 |

For every form, body and DBF are separate guest instructions. The body runs
before the DBF decrement. DBF changes only the low counter word, preserves the
upper word, branches while the result is not `0xFFFF`, and falls through to the
recorded continuation. Address registers wrap at 32 bits. Copy is explicitly
read-before-write per iteration; it is not `memcpy`/`memmove` and therefore
preserves the original forward overlap behavior. Clear uses a single width-
appropriate write, preserving X while setting N/Z/V/C as the generated oracle
does. Word writes require the existing even-address bus contract; an odd word
address fails closed because no reachable candidate proves an address-error
contract.

Observed native invocation/iteration counts were:

| candidate | invocations | body iterations | guest instructions | reads | writes | hardware-visible accesses | yields/resumptions |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `MEMORY_COPY_003A0C` | 1,355 | 7,613 | 15,226 | 7,613 | 7,613 | 0 | 365 / 324 |
| `MEMORY_COPY_00389E` | 859 | 7,124 | 14,248 | 7,124 | 7,124 | 0 | 343 / 314 |
| `MEMORY_CLEAR_WORD_0003F0` | 1 | 4,565 | 9,130 | 0 | 4,565 | 0 | 210 / 210 |
| `MEMORY_CLEAR_BYTE_061266` | 1 | 1,890 | 3,780 | 0 | 1,890 | 0 | 86 / 86 |
| **total** | **2,216** | **21,192** | **42,384** | **14,737** | **21,192** | **0** | **1,004 / 934** |

No hardware-visible access occurred in the promoted family, and no hardware
emulation was added. The separate candidate `0x060BA4` remains
`HARDWARE_VISIBLE_BLOCKED`.

## Semantic, generator and coexistence proof

`tests/hybrid_mechanical_primitive_test.cpp` covers all four registered forms
through the same generic executor and includes: interrupted body/DBF resume,
upper-word DBF preservation, source-before-destination ordering with overlap,
CCR/X behavior, byte and word widths, 32-bit address wrap, 16-bit write shape,
odd-word fail-closed behavior, metadata coverage and unsupported-form
rejection. The generated M11.47 body remains present as an independent
instruction-level oracle; no handwritten candidate execution body was added.

The reusable dependency graph is:

```text
registry metadata + canonical opcodes
        -> generic mechanical loop executor
        -> native BasicBlockApi adapter / detached shadow adapter
        -> existing event, timing, refresh and continuation contracts
        -> generated M11.47 oracle and fallback
```

The executor has no GPGX or libretro types, serialized-state layout, timing
constants tied to an address, or production `oasis_core` dependency. Unsupported
operation/width/displacement/alignment forms fail closed.

## Shadow gate

The unchanged 600-frame `MECHANICAL_PRIMITIVE_SHADOW` run completed with zero
divergence. It compared `42,384/42,384` mechanical guest instructions and
`6,284,149` combined generated/mechanical comparisons. The concurrent generated
oracle covered full IR/prefetch-visible state; the primitive comparator covered
D/A/PC, SR, ordered reads and writes, cycles, refresh, boundary state and
continuation. Checkpoint/video identity was preserved. The primitive recorded
1,004 logical instruction-boundary yields in the equivalent native accounting,
934 of which were mid-operation dispatch yields; no candidate diverged.

## Native gate and coverage accounting

The unchanged 600-frame `MECHANICAL_PRIMITIVE_NATIVE` scenario completed with
full CPU equivalence:

| metric | M11.48 | M11.49 |
| --- | ---: | ---: |
| total guest instructions | 6,488,773 | 6,488,773 |
| generated translated instructions | 6,237,985 | 6,199,381 |
| mechanical primitive instructions | 3,780 | 42,384 |
| translated equivalent (generated + mechanical) | 6,241,765 | 6,241,765 |
| interpreter instructions | 247,008 | 247,008 |
| generated translated share | 96.1350% | 95.5401% |
| translated equivalent share | 96.1933% | 96.1933% |
| registered ranges | 587 | 587 |
| natural translated entries | 2,934,464 | 2,895,860 |
| average instructions/translated entry | 2.1258 | 2.1408 |
| boundary yields | 150,135 | 150,135 |
| interrupted resumptions | 288 | 288 |
| hardware-visible fallback accesses | 0 | 0 |
| unexpected fallback entries | 0 | 0 |
| original starts inside translated ranges | 0 | 0 |

The mechanical replacement is `42,384 / 6,488,773 = 0.6531%` of total guest
execution and `42,384 / 247,008 = 17.1581%` of the M11.48 interpreter
remainder. It is reported separately and is not counted as generated
recompilation coverage. The translated-equivalent count remains
`6,241,765`; no 95% gate was forced.

## M11.47 candidate closure and extraction readiness

| M11.47 candidate | M11.49 status | extraction readiness |
| --- | --- | --- |
| `0x00026A MOVE.L D0,-(A6)` | `GENERATED_ORACLE_ONLY` | not a loop family; no complete save/restore contract |
| `0x003A0C MOVE.B (A2)+,(A1)+` + DBF | `PROMOTED_FAMILY_MEMBER` | ready for shared mechanical primitive extraction |
| `0x00389E MOVE.B (A2)+,(A1)+` + DBF | `PROMOTED_FAMILY_MEMBER` | ready for shared mechanical primitive extraction |
| `0x0003F0 CLR.W (A0)+` + DBF | `PROMOTED_FAMILY_MEMBER` | ready for shared mechanical primitive extraction |
| `0x06193C BTST.B #0,0(A5)` | `GENERATED_ORACLE_ONLY` | no loop/control contract |
| `0x061954 MOVE.B #$FF,(A4)+` | `GENERATED_ORACLE_ONLY` | no producer/consumer contract |
| `0x061266 CLR.B (A5)+` + DBF | `PROMOTED_FAMILY_MEMBER` | existing proven member |

Extraction is therefore ready as a future refactoring boundary, but no
extraction or next milestone is implemented here. The remaining interpreter
Pareto is unchanged from M11.47 and closes exactly at 247,008:

| terminal category | executions | % of total | % of remaining interpreter |
| --- | ---: | ---: | ---: |
| semantic | 106,022 | 1.6339% | 42.9225% |
| runtime/prefetch | 0 | 0.0000% | 0.0000% |
| hardware-visible | 914 | 0.0141% | 0.3700% |
| indirect CFG | 267 | 0.0041% | 0.1081% |
| decoder | 6,196 | 0.0955% | 2.5084% |
| cold/low-payoff | 25,978 | 0.4004% | 10.5171% |
| unknown-with-evidence | 107,631 | 1.6587% | 43.5739% |
| **total** | **247,008** | **3.8079%** | **100.0000%** |

Hottest remaining PCs are `0x03A8A4` (26,887, `NEW_SEMANTICS_REQUIRED`),
`0x060310` (11,489, `UNKNOWN_WITH_EVIDENCE`), `0x06135E` (8,192,
`UNKNOWN_WITH_EVIDENCE`), `0x00D990` (3,567, `UNKNOWN_WITH_EVIDENCE`) and
`0x002C12` (2,784, `UNKNOWN_WITH_EVIDENCE`). Their exact decoder-backed rows,
predecessor/successor evidence and blockers remain in the M11.47 ledger.

## Historical and hygiene status

M11.32 through M11.48 history, including the M11.39/M11.40 negative results,
M11.41 checkpoint identity repair and M11.47 safe-memory proof, remains in
history and its reports. No ROM, extracted asset, emulator binary or generated
run evidence is tracked. `game.srm` was not modified and remains untracked.

Final result: `NATIVE_MECHANICAL_PRIMITIVE_FAMILY_PROVEN`.
