# M11.47 — Safe-memory semantic closure and primitive discovery

Result: `SAFE_MEMORY_SEMANTIC_CLOSURE_PROVEN`; the bounded safe-memory tranche
passed semantic, mechanical-generation, shadow and unchanged native gates.
The translated share increased from 95.5453% to 96.1933%. No hardware contract
was broadened and no native higher-level primitive replacement was performed.

## Identity and baseline

Baseline commit: `7cf947cffee7507e6157e147049bb2b746baa3fa`.

The unchanged M11.46/M11.45 baseline was reproduced twice before promotion.
Both runs preserved ROM SHA-256 `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`,
pinned GPGX DLL SHA-256 `140c00fc7475cf22ca22415d65dfc7ffc66523de128454f9994e7ab77d9826fd`,
checkpoint aggregate
`251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d`,
video hash `5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`,
6,488,773 total instructions, 6,199,718 translated instructions, 289,055
interpreter instructions, 580 ranges, 149,059 yields, 288 resumptions, zero
fallback entries and zero starts inside translated ranges.

## Selected safe-memory tranche

M11.46 runtime provenance classified the selected rows as
`SAFE_MEMORY_OBSERVED`. The observed addresses are main RAM and the observed
bus effects are listed here; hardware-reachable, mixed and unresolved rows were
excluded.

| PC | count | canonical bytes | exact form | observed bus evidence | result |
| --- | ---: | --- | --- | --- | --- |
| `0x00026A` | 16,384 | `2D00` | `move.l D0,-(A6)` | `W2:0x00FF0002; W2:0x00FF0000` | promoted |
| `0x003A0C` | 7,613 | `12DA` | `move.b (A2)+,(A1)+` | `R1:0x00FF322A; W1:0x00FF322E` | promoted |
| `0x00389E` | 7,124 | `12DA` | `move.b (A2)+,(A1)+` | `R1:0x00FF3617; W1:0x00FF361F` | promoted |
| `0x0003F0` | 4,565 | `4258` | `clr.w (A0)+` | `W2:0x00FF0BFE` | promoted |
| `0x06193C` | 2,430 | `082D00000000` | `btst.b #$0,0(A5)` | `R1:0x00FF001A` | promoted |
| `0x061954` | 2,041 | `18FC00FF` | `move.b #$FF,(A4)+` | `W1:0x00FF0781` | promoted |
| `0x061266` | 1,890 | `421D` | `clr.b (A5)+` | `W1:0x00FF001A` | promoted |

The promoted dynamic count is exactly 42,047. The generated bodies contain no
address-specific semantic code, no candidate-specific timing constants and no
handwritten candidate execution body. Unsupported forms remain fail-closed.

## Exact semantic and bus proof

`tests/hybrid_m1147_semantic_test.cpp` verifies the exact combinations only:
register and memory results, N/Z/V/C/X behavior, effective-address updates,
overlap ordering for byte copies, 32-bit address arithmetic, width and bus
ordering. For `MOVE.L D0,-(A6)`, the pinned GPGX path produced two 16-bit bus
writes: high word at `address+2`, then low word at `address`; the helper masks
only the bus-visible addresses to 24 bits while preserving 32-bit register
wrap. This is also covered by the deterministic wrap vector.

Mechanical output was regenerated from canonical ROM bytes through the shared
decoder/exact IR. The generated implementation is separated into
`generated_blocks_m1147.cpp` and `generated_blocks_m1147_registry.cpp`; shared
semantics live in `generated_block_runtime.cpp` and are not address-specific.

## Shadow and native gates

The full 600-frame `BASIC_BLOCK_SHADOW` run completed 6,241,765 per-instruction
comparisons with zero divergence. It preserved PC, D/A, SR, IR/prefetch state,
RAM, ordered bus effects, cycles, refresh, event boundaries and continuation.

The unchanged 600-frame `BASIC_BLOCK_NATIVE` run completed with exact identity:

| metric | M11.46 baseline | M11.47 native |
| --- | ---: | ---: |
| total guest instructions | 6,488,773 | 6,488,773 |
| translated instructions | 6,199,718 | 6,241,765 |
| interpreter instructions | 289,055 | 247,008 |
| translated share | 95.5453% | 96.1933% |
| registered ranges | 580 | 587 |
| natural translated entries | 2,896,197 | 2,938,244 |
| average instructions/translated entry | 2.1406 | 2.1243 |
| boundary yields | 149,059 | 150,135 |
| interrupted resumptions | 288 | 288 |
| hardware-visible accesses | 0 | 0 |
| unexpected fallback entries | 0 | 0 |
| original starts inside translated ranges | 0 | 0 |

Checkpoint, video, CPU/RAM/VDP/sound, interrupt-visible behavior and
continuation identity matched. The native run reports full CPU equivalence.

## Primitive discovery and replacement suitability

These are structural classifications only; no gameplay names are inferred and
no primitive is replaced in M11.47.

| proposed primitive | exact evidence | dynamic evidence | suitability |
| --- | --- | ---: | --- |
| `MEMORY_COPY` at `0x003A0C` | `MOVE.B (A2)+,(A1)+` followed by existing `DBF` at `0x003A0E`; observed source `0x00FF322A`, destination `0x00FF322E`, byte stride 1 | 7,613 copies; loop control is separate | `MECHANICAL_NATIVE_REPLACEMENT_CANDIDATE` |
| `MEMORY_COPY` at `0x00389E` | `MOVE.B (A2)+,(A1)+` followed by existing `DBF` at `0x0038A0`; observed source `0x00FF3617`, destination `0x00FF361F`, byte stride 1 | 7,124 copies; loop control is separate | `MECHANICAL_NATIVE_REPLACEMENT_CANDIDATE` |
| `MEMORY_CLEAR` at `0x0003F0` | `CLR.W (A0)+` followed by existing `DBF` at `0x0003F2`; observed main-RAM word write, stride 2 | 4,565 clears; loop control is separate | `MECHANICAL_NATIVE_REPLACEMENT_CANDIDATE` |
| `MEMORY_CLEAR` at `0x061266` | `CLR.B (A5)+` followed by existing `DBF` at `0x061268`; observed main-RAM byte write, stride 1 | 1,890 clears; loop control is separate | `MECHANICAL_NATIVE_REPLACEMENT_CANDIDATE` |
| `STACK_SAVE_RESTORE` at `0x00026A` | one `MOVE.L D0,-(A6)` with exact two-word bus order; no complete save/restore contract proven | 16,384 stores | `INSUFFICIENT_EVIDENCE` |
| `TABLE_TRAVERSAL` at `0x06193C` | one `BTST.B #0,0(A5)` with safe observed RAM read; no complete traversal contract proven | 2,430 tests | `INSUFFICIENT_EVIDENCE` |
| `BUFFER_TRANSFER` at `0x061954` | one immediate byte store with postincrement; no complete producer/consumer contract proven | 2,041 stores | `INSUFFICIENT_EVIDENCE` |

The copy and clear rows have stable observed address regions, exact register
transitions, no hardware access and exact shadow oracles, so they are candidates
for a future bounded native replacement task. They are not higher-level game
semantics and are not atomic loops.

## Final decision and Pareto

The final interpreter ledger is
`REMAINING_INTERPRETER_ATTRIBUTION_M11_47.md`; its 1,575 rows sum exactly to
247,008. The remaining categories are semantic 106,022, hardware-visible 914,
indirect-CFG 267, decoder 6,196, cold/low-payoff 25,978 and
unknown-with-evidence 107,631. Runtime/prefetch is zero. The complete row-level
ledger records the exact decoder form, ownership/range, CFG evidence, semantic
and generator state, memory class, hardware visibility, prior history and
current blocker for every remaining executed PC.

Within the M11.46 runtime-address cohort, the post-promotion remainder is
`SAFE_MEMORY_OBSERVED` 70,443, `HARDWARE_REACHABLE` 14,085,
`MIXED` 21,272 and `UNRESOLVED_EXACT_EVIDENCE` 1,831. These sum to the exact
107,631 `UNKNOWN_WITH_EVIDENCE` executions; the safe-memory count is the
112,490 M11.46 safe total minus the 42,047 promoted executions.

`REMAINING_ATTRIBUTION_AND_DYNAMIC_COVERAGE_95_PROVEN` remains the M11.45
historical result; M11.47's bounded tranche is recorded as
`SAFE_MEMORY_SEMANTIC_CLOSURE_PROVEN`, not as a 100% coverage claim. The
historical M11.35 `0x03A7AE` rejection and M11.39--M11.46 results remain
preserved in the repository history and reports.

No ROM, commercial asset, emulator binary or generated run evidence was added
to tracking. `game.srm` remains untouched and untracked.
