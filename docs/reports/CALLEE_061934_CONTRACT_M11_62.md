# M11.62 — Exact natural G0 callee/effect closure for `0x061934`

Status: **COMPLETE — `CALLEE_061934_NATURAL_CONTRACT_PROVEN`**

Baseline: `3ad8870529486688b7a86974bad0f0cc2180abb8`.

## Scope and result

This milestone examined only the natural G0-crossed callee rooted at
`0x061934`, while the M11.60 generation held `A5 = 0x00FF001A`. The selected
callee contains the direct G0 consumers at `0x06193C` and `0x061946`. No
`0x062CEC`, `0x0623AC`, `0x060286`, `0x60BCC`, `0x061258`, typed RAM,
gameplay semantics or native replacement was added.

The narrow natural contract is proven. There are 2,916 natural entries and
2,916 parent returns; every entry and return preserves A5. The complete
observed natural interval contains 65,765 data effects: 7,017 G0-relative,
36,108 other safe-RAM, 338 ROM reads, 22,302 stack effects and zero hardware
effects. The static transitive slice remains `INDIRECT_CFG` because of the
unresolved target form at `0x061F60`; its natural executions are nevertheless
closed by four observed targets with exact returns and A5 preservation. The
natural result is therefore not downgraded by that static-only boundary.

The M11.60 transaction gate remains
`BOUNDED_A5_TRANSACTION_BLOCKED_PARENT_LIFETIME`; typed data remains
`TYPED_DATA_BLOCKED_OVERLAPPING_ACCESS`.

## Baseline gate

The exact 600-frame native/shadow identity was reproduced before analysis:

- checkpoint: `251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d`
- video: `5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`
- accounting: 6,488,773 total = 6,488,692 interpreter + 34 TableCopy + 40 RamFlag + 7 ParentSuffix
- native fallback/divergence: `0/0`
- shadow comparisons/divergence: `5/5`, `0`

## Exact CFG boundary

The bounded decoder rooted at `0x061934` with a `0x1200` byte budget decoded
475 instructions in 156 blocks, with 105 direct branches, 12 direct calls,
no direct jumps, one unresolved control-flow edge and no unsupported
instructions. The static boundary is **`INDIRECT_CFG`**.

The root block has the following conditional edges:

- `0x061938 -> 0x061946`
- `0x061942 -> 0x061954`
- `0x061950 -> 0x06195A`
- `0x06195E -> 0x061964`


<details><summary>Complete 105 direct branch edges</summary>

- `0x00061938 -> 0x00061946`
- `0x00061942 -> 0x00061954`
- `0x00061950 -> 0x0006195a`
- `0x0006195e -> 0x00061964`
- `0x0006196c -> 0x000619b6`
- `0x00061976 -> 0x000619b6`
- `0x00061986 -> 0x0006199e`
- `0x00061990 -> 0x000619b6`
- `0x0006199c -> 0x000619b6`
- `0x000619ba -> 0x00061ac2`
- `0x000619d0 -> 0x00061f56`
- `0x000619d6 -> 0x000619e2`
- `0x000619de -> 0x00061f64`
- `0x000619e6 -> 0x00061a66`
- `0x00061a04 -> 0x00061a12`
- `0x00061a0e -> 0x00061a1a`
- `0x00061a40 -> 0x00061a48`
- `0x00061a46 -> 0x00061a4a`
- `0x00061a5a -> 0x00061aaa`
- `0x00061a64 -> 0x00061aaa`
- `0x00061a6c -> 0x00061aa2`
- `0x00061a76 -> 0x00061a84`
- `0x00061a80 -> 0x00061a96`
- `0x00061a8a -> 0x00061a96`
- `0x00061ab0 -> 0x00061aba`
- `0x00061ad0 -> 0x00061b08`
- `0x00061ad8 -> 0x00061b74`
- `0x00061ae4 -> 0x00061aee`
- `0x00061af6 -> 0x00061b68`
- `0x00061b06 -> 0x00061b68`
- `0x00061b0e -> 0x00061b74`
- `0x00061b16 -> 0x00061b74`
- `0x00061b2a -> 0x00061b68`
- `0x00061b3a -> 0x00061b46`
- `0x00061b44 -> 0x00061b68`
- `0x00061b56 -> 0x00061b62`
- `0x00061b7a -> 0x00061cee`
- `0x00061b84 -> 0x00061dde`
- `0x00061b8e -> 0x00061d86`
- `0x00061b98 -> 0x00061ee0`
- `0x00061ba2 -> 0x00061e20`
- `0x00061bbc -> 0x00061c74`
- `0x00061bc4 -> 0x00061c56`
- `0x00061bd2 -> 0x00061c56`
- `0x00061c54 -> 0x00061c74`
- `0x00061c86 -> 0x00061cc8`
- `0x00061c98 -> 0x00061cc8`
- `0x00061ca2 -> 0x00061cb8`
- `0x00061cac -> 0x00061cc8`
- `0x00061cbe -> 0x00061cc8`
- `0x00061ce4 -> 0x00061cda`
- `0x00061cf4 -> 0x00061d2c`
- `0x00061cfe -> 0x00061d2c`
- `0x00061d08 -> 0x00061d28`
- `0x00061d12 -> 0x00061d28`
- `0x00061d1c -> 0x00061d28`
- `0x00061d48 -> 0x00061d46`
- `0x00061d52 -> 0x00061b7e`
- `0x00061d5c -> 0x00061b7e`
- `0x00061d72 -> 0x00061d70`
- `0x00061d82 -> 0x00061b7e`
- `0x00061d8c -> 0x00061b92`
- `0x00061d96 -> 0x00061db6`
- `0x00061da0 -> 0x00061db6`
- `0x00061daa -> 0x00061db6`
- `0x00061dd0 -> 0x00061dce`
- `0x00061dda -> 0x00061b92`
- `0x00061df6 -> 0x00061e00`
- `0x00061dfe -> 0x00061e0c`
- `0x00061e0c -> 0x00061df0`
- `0x00061e1c -> 0x00061b88`
- `0x00061e26 -> 0x00061ba6`
- `0x00061e44 -> 0x00061ba6`
- `0x00061e4e -> 0x00061ede`
- `0x00061e56 -> 0x00061ede`
- `0x00061e66 -> 0x00061e98`
- `0x00061e70 -> 0x00061eac`
- `0x00061e8e -> 0x00061e7a`
- `0x00061e96 -> 0x00061eba`
- `0x00061e9e -> 0x00061ea6`
- `0x00061eaa -> 0x00061eba`
- `0x00061eb0 -> 0x00061eba`
- `0x00061ec0 -> 0x00061ede`
- `0x00061ee8 -> 0x00061b9c`
- `0x00061efa -> 0x00061f54`
- `0x00061f20 -> 0x00061f4c`
- `0x00061f2c -> 0x00061f38`
- `0x00061f34 -> 0x00061f40`
- `0x00061f3c -> 0x00061f44`
- `0x00061f50 -> 0x00061f1a`
- `0x00061f6a -> 0x00061f76`
- `0x00061f72 -> 0x000619c4`
- `0x0006215a -> 0x00062176`
- `0x00062162 -> 0x00062176`
- `0x0006216c -> 0x00062176`
- `0x0006217c -> 0x00062188`
- `0x00062196 -> 0x000621c6`
- `0x000626c0 -> 0x000626ce`
- `0x000626d2 -> 0x000626d8`
- `0x000626dc -> 0x00062700`
- `0x000626e6 -> 0x000626ec`
- `0x000626f2 -> 0x000626f8`
- `0x00062706 -> 0x000626f8`
- `0x00062710 -> 0x00062716`
- `0x0006271c -> 0x000626f8`

</details>

The complete edge list is retained in the local uncommitted slice artifact. Direct calls are:

`0x061964 -> 0x061E48`, `0x0619B2 -> 0x061EF4`, `0x0619DA -> 0x062156`,
`0x061A12 -> 0x06138E`, `0x061A16 -> 0x0613F8`, `0x061ABA -> 0x0626BC`,
`0x061AC2 -> 0x0613B2`, `0x061AC6 -> 0x06147E`, `0x061D28 -> 0x061CD2`,
`0x061DB6 -> 0x061CD2`, `0x061EE4 -> 0x061EF4`,
`0x06219A -> 0x061CD2`.

The unresolved indirect call is `0x061F60 JSR (...)`; no static target can be
recovered from the bounded slice. Static RTS exits are
`0x061958`, `0x061B66`, `0x061CD0`, `0x061CEC`, `0x061EDE`, `0x061F54`,
`0x061F82`, `0x062174`, `0x0621C6`, `0x0626CC`, `0x0626D6`, `0x0626EA`,
`0x0626F6`, `0x0626FE`, `0x062714` and `0x062720`. Natural outer returns are
the six parent continuation PCs `0x0601F2`, `0x0601FE`, `0x06020A`,
`0x060216`, `0x060222` and `0x060264`.

## G0 access contract

The direct root operations are exact and mechanically identified:

| PC | operation | offset | width | natural count | ordering |
|---|---|---:|---:|---:|---|
| `0x06193C` | `BTST.B #0,0(A5)` read | `+0` | byte | 2,430 | after root branch at `0x061938` |
| `0x061946` | `MOVE.B D7,4(A5)` write | `+4` | byte | 1,361 | after `0x06193C` fallthrough/branch paths |

The complete natural G0-relative footprint observed across the bounded interval
is `FF001A..FF0024` (55 PC/EA/type/width entries, 7,017 events). Other direct
A5 operands in the static slice are reads or writes at offsets `+2`, `+3`,
`+4`, `+5`, `+7` and `+10`; no A5 register overwrite, LEA/MOVE into A5,
spill/reload, predecrement or postincrement was found. The derived accesses
remain raw A5-relative effects; no field names are assigned.

The observed G0-relative operation ledger is:

| PC | EA | width | direction | count |
|---|---|---:|---|---:|
| `0x613B2` | `0xFF0022` | 1 | read | 175 |
| `0x6193C` | `0xFF001A` | 1 | read | 2430 |
| `0x61946` | `0xFF001E` | 1 | write | 1361 |
| `0x6194A` | `0xFF0022` | 1 | read | 175 |
| `0x61970` | `0xFF0022` | 1 | read | 2 |
| `0x6197A` | `0xFF0022` | 1 | read | 1 |
| `0x6197A` | `0xFF0022` | 1 | write | 1 |
| `0x6198A` | `0xFF0023` | 1 | read | 1 |
| `0x61998` | `0xFF001E` | 1 | read | 8 |
| `0x619BE` | `0xFF0022` | 1 | read | 2 |
| `0x619BE` | `0xFF0022` | 1 | write | 2 |
| `0x619C4` | `0xFF0024` | 4 | read | 5 |
| `0x619EA` | `0xFF0022` | 1 | read | 2 |
| `0x619EA` | `0xFF0022` | 1 | write | 2 |
| `0x61A08` | `0xFF0022` | 1 | read | 1 |
| `0x61A4E` | `0xFF0023` | 1 | read | 2 |
| `0x61A4E` | `0xFF0023` | 1 | write | 2 |
| `0x61A92` | `0xFF001E` | 1 | read | 3 |
| `0x61AAA` | `0xFF0022` | 1 | read | 2 |
| `0x61AAA` | `0xFF0022` | 1 | write | 2 |
| `0x61AB4` | `0xFF0022` | 1 | read | 1 |
| `0x61AB4` | `0xFF0022` | 1 | write | 1 |
| `0x61ABE` | `0xFF0024` | 4 | write | 2 |
| `0x61B74` | `0xFF0023` | 1 | read | 175 |
| `0x61B74` | `0xFF0023` | 1 | write | 175 |
| `0x61B7E` | `0xFF0022` | 1 | read | 175 |
| `0x61B7E` | `0xFF0022` | 1 | write | 175 |
| `0x61B92` | `0xFF0023` | 1 | read | 175 |
| `0x61B92` | `0xFF0023` | 1 | write | 175 |
| `0x61B9C` | `0xFF0023` | 1 | read | 175 |
| `0x61B9C` | `0xFF0023` | 1 | write | 175 |
| `0x61BB6` | `0xFF0023` | 1 | read | 175 |
| `0x61C80` | `0xFF0023` | 1 | read | 175 |
| `0x61C80` | `0xFF0023` | 1 | write | 175 |
| `0x61C8A` | `0xFF001E` | 1 | read | 7 |
| `0x61C92` | `0xFF0022` | 1 | read | 2 |
| `0x61CB8` | `0xFF0023` | 1 | read | 1 |
| `0x61CC8` | `0xFF001E` | 1 | read | 875 |
| `0x61CEE` | `0xFF0023` | 1 | read | 1 |
| `0x61CF8` | `0xFF0022` | 1 | read | 1 |
| `0x61D4C` | `0xFF0023` | 1 | read | 1 |
| `0x61D56` | `0xFF0022` | 1 | read | 1 |
| `0x61D76` | `0xFF0023` | 1 | read | 1 |
| `0x61D76` | `0xFF0023` | 1 | write | 1 |
| `0x61E20` | `0xFF0023` | 1 | read | 1 |
| `0x61EF4` | `0xFF0023` | 1 | read | 1 |
| `0x61F26` | `0xFF0023` | 1 | read | 2 |
| `0x61F64` | `0xFF0022` | 1 | read | 3 |
| `0x61F6E` | `0xFF0024` | 4 | write | 3 |
| `0x62032` | `0xFF0023` | 1 | read | 1 |
| `0x62032` | `0xFF0023` | 1 | write | 1 |
| `0x6207C` | `0xFF0023` | 1 | read | 1 |
| `0x6207C` | `0xFF0023` | 1 | write | 1 |
| `0x626C4` | `0xFF0022` | 1 | read | 2 |
| `0x626C4` | `0xFF0022` | 1 | write | 2 |

These rows include the nested direct/indirect intervals retained by the
callee contract; the two root instructions above are the direct G0 consumers.
## A5 preservation

All six parent call sites occur exactly 486 times:

`0x0601EE`, `0x0601FA`, `0x060206`, `0x060212`, `0x06021E`, `0x060260`.

Observed outer entries/returns are `2,916/2,916`; A5 equality is `2,916/2,916`
with zero unequal exits. The 2,655 direct nested calls and 13 indirect calls
also return exactly, with A5 equal at every observed nested return. The natural
classification is **`A5_PRESERVED_EXACT`**. No nested preservation dependency
remains unresolved on the observed distribution.

## Complete natural effect classes

The observer records every type-2 read, type-4 write, type-15 interrupt and
post-instruction event while the outer interval is active. The effect totals
are:

Active hook counts are type-1 execution 78,092, type-2 read 33,709, type-4 write 32,056, type-15 interrupt 0 and type-14 post 0; all other hook types are zero.

| class | unique entries | events |
|---|---:|---:|
| `G0_RELATIVE` (`FF001A..FF0024`) | 55 | 7,017 |
| `SAFE_RAM` (other `FFxxxx`) | 1,050 | 36,108 |
| `ROM_READ` (`<0x400000`, reads) | 271 | 338 |
| `STACK` (`FF0Bxx`) | 81 | 22,302 |
| `HARDWARE` | 0 | 0 |
| `DERIVED_ALIAS` unresolved separately | 0 | 0 |
| `UNRESOLVED` | 0 | 0 |

Representative direct effects are `FF001A` byte reads at `0x06193C` (2,430)
and `FF001E` byte writes at `0x061946` (1,361). A4/A6-derived accesses are
classified by their observed safe-RAM/stack/ROM address and retained as raw
provenance. No hardware effect or interrupt was observed in any active
interval, so the natural path is hardware-free under the installed bus/device
hooks.

## Nested-call minimum closure

Nested direct calls total 2,655 and all 2,655 return. Each observed nested
callsite/target pair preserves A5 exactly:

| callsite | target | calls/returns |
|---|---|---:|
| `0x061964` | `0x061E48` | 875/875 |
| `0x061A12` | `0x06138E` | 5/5 |
| `0x061A16` | `0x0613F8` | 5/5 |
| `0x061ABA` | `0x0626BC` | 10/10 |
| `0x061AC2` | `0x0613B2` | 875/875 |
| `0x061AC6` | `0x06147E` | 875/875 |
| `0x061D28` | `0x061CD2` | 5/5 |
| `0x061DB6` | `0x061CD2` | 0/0 |
| `0x061EE4` | `0x061EF4` | 5/5 |
| `0x06219A` | `0x061CD2` | 0/0 |

The indirect `0x061F60` executes 13 times and returns 13 times with targets
`0x06211A` (1), `0x06202C` (5), `0x062048` (5) and `0x0620B0` (2); each target
has exact A5 entry/exit equality. No hardware event or interrupt occurs in
these nested intervals.

## Dynamic path census

Two repeated traces are byte-identical with SHA-256
`C9C2F9077C62233182854963608E9533377856BD08ABBEEDEFCE526C6CD8CFF6`.
There are 13 distinct path signatures. The dominant signatures are:

| path length | count |
|---:|---:|
| 5 | 1,555 |
| 6 | 486 |
| 71 | 519 |
| 72 | 346 |
| 125/126 | 2 each |
| 155/156 | 2/1 |
| 371/372/391/433/440 | 1 each |

All six parent call sites and all six parent return continuations have count
486. The observer reports zero interrupt events and zero unresolved active
memory effects.

## G0 blocker recomputation

The remaining natural G0-crossed callees are:

1. `0x0623AC` — highest remaining bounded callee/effect gap.
2. `0x060286` — second bounded callee/effect gap.

These are ranked only within the existing `0x060182` parent-owned lifetime.
The transaction gate stays blocked by the parent-owned endpoint and the typed
data gate remains `TYPED_DATA_BLOCKED_OVERLAPPING_ACCESS`.

## Verification

Post-change native and shadow runs retain the baseline checkpoint/video hashes and
6,488,773 accounting: native has 6,488,692 interpreter plus 81 translated
instructions with zero fallback/divergence; shadow has 5/5 comparisons and
zero divergence.

Implementation commit `c3a4d346d38c3b6024a73376c97eca7ddc22632d` is on `origin/main`; GitHub Actions CI run `34447382790` passed build and test.

Debug CTest and Release CTest both pass 73/73, including the new observer
regression and source-size limit. `git diff --check` and tracked source/data
hygiene are clean. The configured UCRT build again stops while compiling the
existing `raw_data_provenance_test.cpp` before meaningful diagnostics; this is
classified `LOCAL_TOOLCHAIN_ENVIRONMENT`, and the new test is not claimed under
UCRT.
## Regression and governance

`Callee61934Observer` and its runtime wiring are developer-only
`src/tools/hybrid` code. `tests/hybrid_callee_61934_test.cpp` locks the
entry/return contract, A5 equality, zero nested-call synthetic path and the
`+0/+4` effects. No ROM PC, GPGX type or emulator state enters `oasis_core`.
Raw dynamic traces and generated decoder artifacts remain uncommitted.

M11.63 may target only `0x0623AC` or `0x060286`, selected by updated bounded
ranking; no further milestone work is performed here.
