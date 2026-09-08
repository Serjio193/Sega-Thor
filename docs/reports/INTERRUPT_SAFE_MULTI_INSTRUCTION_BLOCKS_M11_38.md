# M11.38 Interrupt-Safe Multi-Instruction Block Execution

## Result

`INTERRUPT_SAFE_MULTI_INSTRUCTION_BLOCKS_PROVEN`

This is a bounded developer-only hybrid result. It adds no production CPU,
runtime JIT, second scheduler, whole-ROM translation or atomic multi-instruction
execution. The canonical cold-reset neutral scenario used:

| identity | value |
| --- | --- |
| ROM SHA-256 | `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263` |
| GPGX source commit | `d60d079934977aa6973e220d123533387159f66e` |
| GPGX DLL SHA-256 | `140c00fc7475cf22ca22415d65dfc7ffc66523de128454f9994e7ab77d9826fd` |
| scenario | cold reset, neutral input, 600 frames |

## Frozen candidate set

Only the four M11.37 rejected ranges were used. Each has exactly one internal
instruction boundary; the conservative yield point is after instruction 1 and
before instruction 2. The M11.37 report preserved the original range-level
interrupt-interleaving rejection; this report records the resolved instruction
ordinal and the per-candidate boundary evidence.

| range | exact sequence | M11.37 natural count | semantic/generator status |
| --- | --- | ---: | --- |
| `[0x0032EE,0x0032F6)` | `TST.W ($00FF1658).L`; `BNE.S -> 0x0032EE` | 1,121,997 | ALREADY_INDEPENDENTLY_VERIFIED / GENERATED |
| `[0x03A9AC,0x03A9B4)` | `TST.W ($00FFAFAE).L`; `BNE.S -> 0x03A9BC` | 248,291 | ALREADY_INDEPENDENTLY_VERIFIED / GENERATED |
| `[0x03A9B4,0x03A9BC)` | `TST.B ($00FF0BFD).L`; `BNE.S -> 0x03A9CA` | 248,290 | ALREADY_INDEPENDENTLY_VERIFIED / GENERATED |
| `[0x03A9CA,0x03A9D4)` | `TST.W ($00FF1654).L`; `BNE.W -> 0x03A9AC` (`FFDA`) | 248,290 | ALREADY_INDEPENDENTLY_VERIFIED / GENERATED |

All four use only the existing independently verified TST absolute-long and
Bcc short/word forms. The M11.37 hardware candidate `0x060BA4` remains
rejected and is not in this registry.

## Boundary contract

Authoritative GPGX source inspection established that `m68k_run` polls pending
interrupts at entry, evaluates the interrupt mask in `m68ki_check_interrupts`,
and evaluates trace exceptions after an interpreter instruction. GPGX owns the
interrupt stack/vector transition, CPU cycle/refresh counters and the later
VDP/Z80 scheduler. The bridge therefore yields only at a materialized
post-instruction boundary, with PC, prefetch, IR, SR, cycles, refresh,
interrupt level/mask, trace and stopped state available to the dispatcher.

Generated code calls the same bridge fetch, semantic, cycle and refresh
primitives for each instruction. After each instruction it asks one generic
boundary callback for `CONTINUE_BLOCK`, `EVENT_BOUNDARY`,
`INTERRUPT_BOUNDARY` or `TRACE_BOUNDARY`. A non-continuing result returns
`BlockExit { next_pc, reason, instructions_executed }`; the dispatcher resumes
at the exact next guest instruction or lets GPGX service the pending event.
No candidate address is tested by handwritten runtime logic.

## Differential and interrupted-path evidence

The four-candidate shadow run completed `4,122,062` per-instruction state
comparisons with zero divergence and full CPU equivalence. It recorded `111,009`
event boundary yields, including the following frozen-candidate counts:

| range start | shadow invocations | boundary yields | interrupt yields |
| --- | ---: | ---: | ---: |
| `0x0032EE` | 1,159,210 | 60,903 | 0 |
| `0x03A9AC` | 256,471 | 12,424 | 0 |
| `0x03A9B4` | 256,652 | 13,705 | 0 |
| `0x03A9CA` | 256,424 | 13,310 | 0 |

The native run promoted all four ranges only after that shadow gate. It
recorded 486 GPGX interrupt services and 274 translated continuations that
resumed after an actual interrupt service: `179`, `19`, `64` and `12` for the
four ranges in table order. This proves the naturally observed interrupted
path: translated entry, yield at the first/second-instruction boundary, GPGX
interrupt service, exact-PC continuation, and final state equivalence.

## Native result

| metric | EMULATED baseline | BASIC_BLOCK_NATIVE |
| --- | ---: | ---: |
| guest instruction executions | 6,488,773 | 6,488,773 |
| translated instruction executions | 0 | 4,122,062 |
| interpreter instruction executions | 6,488,773 | 2,366,711 |
| translated dynamic share | 0% | 63.5261% |
| multi-instruction translated entries | n/a | 1,928,770 |
| instruction/event boundary yields | n/a | 111,009 / 111,009 |
| interrupt boundary yields | n/a | 0 |
| interrupted resumptions | n/a | 274 |
| fallback entries | n/a | 0 |
| original starts inside translated blocks | n/a | 0 |
| hardware-visible accesses | n/a | 0 |
| video frames | 600 | 600 |

The current-run EMULATED and native checkpoint hash is
`20217e10565c51b571db6a4e474aa40854ea6c22277ba14c46ea97c4b1c60a04`; the
video sequence hash is
`5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`.
The prior M11.37 checkpoint hash remains historical evidence and is unchanged
in its report.

## Scalability evidence

The capability makes all four M11.37 rejected ranges eligible under this
contract. In the frozen M11.37 trace their four range-start counts total
`1,866,868` natural entries (the four ranges contain eight guest instructions
per full pass); the current 600-frame registry run measures the exact
translated dynamic execution above.
No additional candidate was discovered or promoted in M11.38. Further
coverage requires a new bounded milestone.
