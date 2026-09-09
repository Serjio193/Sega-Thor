# M11.48 — First Proven Native Mechanical Primitive Replacement

Result: `FIRST_NATIVE_MECHANICAL_PRIMITIVE_PROVEN`

Date: 2026-09-09
Baseline commit: `f51f3b370b8e7acdc3613b1cec7f9ffcdf05f14e`
Authoritative checkpoint aggregate:
`251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d`
Video SHA-256: `5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`
Canonical ROM SHA-256:
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`
Pinned GPGX DLL SHA-256:
`140c00fc7475cf22ca22415d65dfc7ffc66523de128454f9994e7ab77d9826fd`

## Scope and selection

M11.48 was a bounded replacement task, not a coverage milestone. The M11.47
baseline had already proven safe-memory semantics and identified four repeated
mechanical structures. The safest complete contract was selected:

| role | address | canonical bytes/form | contract |
| --- | ---: | --- | --- |
| body | `0x061266` | `421D`, `CLR.B (A5)+` | write byte zero, increment A5 with 32-bit wrap, preserve X and set N/Z/V/C exactly |
| loop | `0x061268` | `51C8 FFFC`, `DBF D0,0x061266` | decrement low D0 word, preserve upper word, branch unless result is `0xFFFF` |
| continuation | `0x06126C` | next guest PC | resume after the loop |

At `0x061260`, `D0` is loaded with `0x761`; the observed loop therefore has
1,890 body iterations and 1,890 DBF executions. Runtime writes are 1-byte
stores to `0x00FF001A..0x00FF077B`, stride one, with no reads, overlap or
hardware-visible access. The other M11.47 structures remain inventory only:

| candidate | status | blocker |
| --- | --- | --- |
| `0x003A0C` + `DBF 0x003A0E` | `NEEDS_CONTRACT_WORK` | copy source/destination/control and overlap/wrap contract incomplete |
| `0x00389E` + `DBF 0x0038A0` | `NEEDS_CONTRACT_WORK` | copy source/destination/control and overlap/wrap contract incomplete |
| `0x0003F0` + `DBF 0x0003F2` | `NEEDS_CONTRACT_WORK` | word bus, alignment and wrap contract not selected |
| `0x061266` + `DBF 0x061268` | `PROVEN_NATIVE_REPLACEMENT_READY` | none within the bounded contract |

No new candidate was added. No gameplay meaning is assigned.

## Architecture and code separation

The developer-only path is now layered as:

```text
canonical ROM bytes
  -> exact decoder / generated instruction body
  -> generated basic-block oracle and generic boundary glue
  -> architecture-neutral mechanical primitive API
  -> native GPGX adapter or detached shadow adapter
```

`MechanicalMachine` contains only registers, memory effects, instruction
timing, canonical displacement fetch and boundary reasons. It has no GPGX
types. `execute_memory_clear` is reusable resumable loop semantics. The
registry contains generic dispatch and metrics plus the proven loop metadata;
it does not contain a handwritten candidate execution body or address-specific
semantic patch. Canonical-byte checks fail closed.

Generated files and handwritten glue remain distinct. The existing generated
M11.47 bodies and `BasicBlockRegistry` continue to serve as the instruction-
level oracle and fallback for all other PCs. The new layer is linked only into
the developer-only hybrid target and is not a production runtime dependency.

## Shadow proof

The unchanged 600-frame scenario was run in
`MECHANICAL_PRIMITIVE_SHADOW` mode. The existing generated/basic-block oracle
completed `6,241,765/6,241,765` comparisons with zero divergence. The
mechanical primitive added `3,780/3,780` logical guest-instruction boundary
comparisons with zero divergence, for `6,245,545` combined comparisons.
Checkpoint and video identities were unchanged.

The primitive comparator independently checked D/A/PC/SR, IR, cycles, refresh,
interrupt/trace/stopped boundary state, ordered writes and continuation PC.
Its detached snapshot intentionally does not duplicate the generated oracle's
prefetch comparator. The generated/basic-block oracle ran concurrently in the
same shadow process and supplied the full IR/prefetch comparison, so the
combined gate covers the required prefetch-visible state without putting GPGX
internals in the primitive API.

The loop yielded after instruction boundaries during 86 observed
mid-operation interruptions and resumed 86 times. It was never executed as an
atomic full-loop shortcut. The primitive wrote exactly 1,890 bytes with minimum
address `0x00FF001A` and maximum address `0x00FF077B`.

## Native proof

The unchanged 600-frame `MECHANICAL_PRIMITIVE_NATIVE` scenario completed with:

| metric | result |
| --- | ---: |
| total guest instructions | 6,488,773 |
| generated translated instructions | 6,237,985 |
| mechanical primitive instructions | 3,780 |
| interpreter instructions | 247,008 |
| generated translated share | 96.1350% |
| generated + mechanical share | 96.1933% |
| registered ranges | 587 |
| primitive dispatches | 87 |
| primitive iterations | 1,890 |
| boundary yields | 150,135 |
| interrupted resumptions | 288 |
| primitive mid-operation yields/resumptions | 86 / 86 |
| hardware-visible accesses | 0 |
| unexpected fallback entries | 0 |
| original starts inside translated ranges | 0 |

The primitive replacement itself represents `3,780 / 6,488,773 = 0.0583%`
of total guest execution and `3,780 / 247,008 = 1.5303%` of the M11.47
interpreter remainder. It removes two guest PCs (`0x061266` and `0x061268`)
from the instruction-level native dispatch path; the primitive still uses the
generic machine timing/memory contract for each guest instruction.

Native checkpoint aggregate, video sequence, CPU/RAM/VDP/sound state and
interrupt-visible behavior matched the authoritative baseline. The aggregate
yield/resumption counts remain unchanged after the primitive's boundary yields
are accounted for.

## Synthetic interruption proof

The mechanical unit test covers a body boundary with `A5=0xFFFFFFFE` and
`D0=1`. The first call writes at `0xFFFFFFFE`, wraps A5 to `0xFFFFFFFF`,
preserves the upper D0 word and returns at the loop PC without decrementing
D0. Resumption writes at `0xFFFFFFFF`, wraps to zero, performs the final DBF
fall-through and continues at `0x06126C`. A second vector injects an interrupt
boundary with `D0=0xFFFF` and verifies that the operation remains resumable
instead of becoming an unbounded atomic loop.

## Remainder and hygiene

The existing M11.47 decoder-backed interpreter ledger was rerun against the
native profile and closed at exactly `247,008` executions. Its terminal classes
and Pareto are unchanged; no generic `other` bucket was introduced. The
remaining work is still dominated by the M11.47 semantic, unknown-with-
evidence, cold/low-payoff, decoder, indirect-CFG and hardware blockers.

ROM and the pinned DLL remain external. No ROM, extracted asset, emulator
binary or generated run evidence is tracked. `game.srm` was not modified and
remains untracked. The 600-frame output directories are local evidence only.

## Final decision

All gates required for this bounded first replacement passed. M11.48 is
complete as `FIRST_NATIVE_MECHANICAL_PRIMITIVE_PROVEN`. Stop here; do not
implement another primitive or the next milestone in this task.
