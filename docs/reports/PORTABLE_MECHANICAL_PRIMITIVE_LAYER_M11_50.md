# M11.50 — Extract Proven Mechanical Primitive Layer into `oasis_core`

Result: `PORTABLE_MECHANICAL_PRIMITIVE_LAYER_PROVEN`

Date: 2026-09-09
Baseline commit: `4a3f0fa8473daa5c4f1f831402b8d0a5d7667da8`
M11.49 checkpoint aggregate: `251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d`
Video SHA-256: `5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`
Canonical ROM SHA-256: `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`
Pinned GPGX DLL SHA-256: `140c00fc7475cf22ca22415d65dfc7ffc66523de128454f9994e7ab77d9826fd`

## Scope and STOP boundary

This milestone is an ownership extraction only. It does not discover a new
primitive, change any ROM contract, assign gameplay meaning, broaden hardware
behavior, chase coverage or add an emulator dependency to the product.

The four M11.49 forms remain the complete supported family:

| operation | body / loop / continuation | width | register roles |
| --- | --- | ---: | --- |
| `MEMORY_COPY` | `0x003A0C` / `0x003A0E` / `0x003A12` | byte | A2 -> A1, D2 |
| `MEMORY_COPY` | `0x00389E` / `0x0038A0` / `0x0038A4` | byte | A2 -> A1, D0 |
| `MEMORY_CLEAR` | `0x0003F0` / `0x0003F2` / `0x0003F6` | word | A0, D0 |
| `MEMORY_CLEAR` | `0x061266` / `0x061268` / `0x06126C` | byte | A5, D0 |

These values occur only in the hybrid registry/provenance layer. Standalone
core tests use synthetic tokens `0x100`–`0x304` and do not use ROM encodings.

## Phase 1 — unchanged baseline gate

Two pre-change runs from the exact baseline and the pinned canonical ROM/DLL
matched the M11.49 identity:

| metric | baseline A | baseline B |
| --- | ---: | ---: |
| checkpoint aggregate | `251fab...d38d` | `251fab...d38d` |
| video hash | `5e74ec...a58` | `5e74ec...a58` |
| total guest instructions | 6,488,773 | 6,488,773 |
| generated translated | 6,199,381 | 6,199,381 |
| mechanical primitive | 42,384 | 42,384 |
| interpreter | 247,008 | 247,008 |
| registered ranges | 587 | 587 |
| boundary yields | 150,135 | 150,135 |
| interrupted resumptions | 288 | 288 |
| unexpected fallback | 0 | 0 |
| starts inside translated ranges | 0 | 0 |

## Phase 2 — dependency audit

The audit classified every dependency of the previous implementation:

| dependency | classification | final owner |
| --- | --- | --- |
| operation, width and register roles | `PORTABLE_CORE` | `core::MechanicalLoopContract` |
| copy/clear body semantics | `PORTABLE_CORE` | `core::execute_mechanical_loop` |
| CCR N/Z/V/C and X preservation | `PORTABLE_CORE` | core executor |
| DBF low-word decrement and terminal decision | `PORTABLE_CORE` | core executor |
| 32-bit address arithmetic and ordered accesses | `PORTABLE_CORE` | core executor / machine adapter |
| continuation validation and result | `PORTABLE_CORE` | `PrimitiveContinuation` / `PrimitiveExit` |
| register and memory bridge | `HYBRID_ADAPTER` | `NativeMachine` / `SimulationMachine` |
| instruction timing, refresh and prefetch | `HYBRID_ADAPTER` | BasicBlock/GPGX bridge |
| canonical body/DBF opcode and displacement checks | `ROM_SPECIFIC_METADATA` | hybrid contract registry and adapters |
| ROM PCs, names and candidate metrics | `ROM_SPECIFIC_METADATA` | `MechanicalPrimitiveRegistry` |
| generated/basic-block reference | `GPGX_ORACLE` | existing hybrid registries |
| shadow snapshots and comparisons | `GPGX_ORACLE` | hybrid registry |
| run report and checkpoint integration | `TEST_ONLY` | runner/reporting |

No dependency classified as `GPGX_ORACLE`, `ROM_SPECIFIC_METADATA` or
`TEST_ONLY` crosses into `oasis_core`.

## Before / after ownership

### Before

```text
tools/hybrid/mechanical_primitive.cpp
    ├── generic body/DBF semantics
    ├── hybrid adapter machines
    ├── four ROM contracts
    ├── continuation/dispatch state
    └── shadow/native metrics and comparison state
```

### After

```text
oasis_core
    ├── portable MechanicalMachine contract
    ├── generic copy/clear/DBF executor
    ├── PrimitiveContinuation state
    └── PrimitiveExit result

tools/hybrid
    ├── Beyond Oasis ROM registry and canonical encodings
    ├── GPGX/BasicBlock NativeMachine and detached SimulationMachine adapters
    ├── shadow comparator and generated fallback/oracle
    ├── dispatch, provenance and metrics
    └── runner/report serialization
```

The dependency direction is `tools/hybrid -> oasis_core`; `oasis_core` has no
reverse dependency. CMake asserts that `oasis_core` has no linked target, and
`tests/check_core_boundary.cmake` rejects hybrid/GPGX/libretro names and the
four ROM PCs plus canonical opcode encodings from core source files.

## Portable contract

The core API deliberately uses opaque body/loop/continuation tokens. The
machine adapter supplies register values, memory reads/writes, instruction
begin/finish hooks, DBF displacement fetch and a generic boundary reason. The
executor itself performs:

- read-before-write byte copy with postincrement and 32-bit wrap;
- byte and even-address word clear with exact write width;
- CCR N/Z/V/C updates while preserving X;
- DBF decrement of only the low counter word and preservation of the upper word;
- body/DBF instruction-boundary yields and resumed entry at the exact saved token;
- fail-closed rejection of invalid forms, odd word addresses and repeated or
  mismatched continuation entries.

The adapter remains responsible for bus-visible address mapping and timing. No
Genesis hardware model was moved into the primitive layer.

## Standalone core proof

`tests/mechanical_primitive_test.cpp` links only `oasis_core` and covers:

- first, resumed and final iterations;
- byte copy overlap with ordered read-before-write effects;
- byte and word clear, exact widths and address wrap;
- DBF terminal behavior and upper-word preservation;
- CCR N/Z and X preservation;
- event/interrupt yield and resume;
- repeated resume protection;
- invalid width and odd-word alignment fail-closed behavior.

The hybrid test now checks only registry metadata and conversion to the
portable contract. It no longer contains a second semantic executor or a
second machine implementation.

## Differential and native regression

The extracted implementation was reconnected to the unchanged runner.

### Mechanical shadow

- mode: `MECHANICAL_PRIMITIVE_SHADOW`;
- combined generated/mechanical comparisons: 6,284,149;
- mechanical comparisons: 42,384 / 42,384;
- divergence: 0;
- checkpoint/video identity: exact;
- scenario completed: yes.

### Native proof

| metric | M11.49 | M11.50 |
| --- | ---: | ---: |
| total guest instructions | 6,488,773 | 6,488,773 |
| generated translated | 6,199,381 | 6,199,381 |
| mechanical primitive | 42,384 | 42,384 |
| interpreter | 247,008 | 247,008 |
| registered ranges | 587 | 587 |
| boundary yields | 150,135 | 150,135 |
| interrupted resumptions | 288 | 288 |
| checkpoint aggregate | `251fab...d38d` | `251fab...d38d` |
| video hash | `5e74ec...a58` | `5e74ec...a58` |
| divergence | 0 | 0 |
| unexpected fallback | 0 | 0 |
| hardware-visible accesses | 0 | 0 |
| starts inside translated ranges | 0 | 0 |

## Architecture inventory for the next milestone

No next layer is implemented here. Existing evidence classifies the four
extracted loops as `MECHANICAL_PRIMITIVE`. The three remaining M11.47 isolated
forms (`0x00026A`, `0x06193C`, `0x061954`) remain generated-oracle-only or
insufficient for a complete mechanical contract. The best future candidate
for a first portable native routine composed from these operations is not
promoted by M11.50: the current ledger contains no higher routine contract
that is both complete and independently closed without introducing gameplay
semantics. A future milestone must inventory callers/data and prove that
contract before implementation.

## Validation and hygiene

The final validation record includes Debug, Release and GNU-equivalent full
CTest, standalone core tests, dependency-boundary test, existing semantic and
primitive tests, shadow/native 600-frame proofs, `git diff --check`, source
line limits and repository hygiene. ROM, assets, emulator binaries, run
evidence and `game.srm` remain untracked; no generated evidence is committed.
