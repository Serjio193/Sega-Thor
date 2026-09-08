# M11.39 Hot-Path Multi-Block Coverage Expansion

## Result

`HOT_PATH_DYNAMIC_COVERAGE_80_PROVEN`

This is a bounded developer-only hybrid result. It is not ROM-byte coverage,
whole-ROM recompilation, a production CPU model or a runtime JIT. The run used
the unchanged cold-reset neutral 600-frame scenario.

| identity | value |
| --- | --- |
| baseline commit | `daa0a09b5cd8845a48733e771774491b35e73d9e` |
| canonical USA ROM SHA-256 | `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263` |
| external GPGX source | `d60d079934977aa6973e220d123533387159f66e` |
| external GPGX DLL SHA-256 | `140c00fc7475cf22ca22415d65dfc7ffc66523de128454f9994e7ab77d9826fd` |
| scenario | cold reset, neutral input, 600 frames |

## Frozen baseline and final gate

The M11.38 native metrics were frozen before this expansion: 4,122,062
translated and 2,366,711 interpreter instruction executions, for 63.5261%
translated dynamic share. The remaining-interpreter profile was collected
with the unchanged 26-range M11.38 registry and ranked by dynamic instruction
executions.

| metric | M11.38 baseline | M11.39 native |
| --- | ---: | ---: |
| guest instruction executions | 6,488,773 | 6,488,773 |
| translated instruction executions | 4,122,062 | 5,826,857 |
| interpreter instruction executions | 2,366,711 | 661,916 |
| translated dynamic share | 63.5261% | 89.7991% |
| registered ranges | 26 | 28 |
| natural translated entries | 2,317,077 | 2,572,150 |
| average translated instructions per entry | 1.779 | 2.265 |
| maximum registered range length | 7 | 16 |
| boundary yields | 111,009 | 140,065 |
| interrupted resumptions | 274 | 274 |
| original starts inside translated ranges | 0 | 0 |
| hardware-visible accesses | 0 | 0 |
| video frames | 600 | 600 |

The final shadow run completed `5,826,857/5,826,857` per-instruction
comparisons with zero divergence. Native and shadow runs both completed 600/600
frames, with checkpoint hash
`20217e10565c51b571db6a4e474aa40854ea6c22277ba14c46ea97c4b1c60a04` and video
hash `5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`.

## Bounded ranked candidate manifest

The profile contained 2,152 interpreter PCs after M11.38. The top remaining
hot paths were inspected through the exact decoder. The bounded queue below
records the highest-payoff representatives and the two selected ranges; it
contains two new candidate ranges, below the maximum of 40. Dynamic counts are
the pre-promotion M11.38-attributable counts unless marked as an aggregate.

| rank | range | dynamic instructions | invocations | instructions | decoded form / CFG | access | certification state | blocker / result |
| ---: | --- | ---: | ---: | ---: | --- | --- | --- | --- |
| 1 | `[0x000380,0x0003A0)` | 1,572,608 | 122,886 | 16 | 16x `ADD.W (A0)+,D0`, direct fallthrough | RAM read | PROMOTED | new exact ADD.W form; shadow clean |
| 2 | `[0x03A864,0x03A868)` | 132,187 | 132,187 | 1 | BNE.W -> `0x03A85E`, direct conditional | RAM-only control | PROMOTED | existing verified Bcc form; shadow clean |
| 3 | `[0x03A7AE,0x03A7B4)` | 50,474 | 50,474 | 1 | TST.W absolute-long, fallthrough | RAM | SEMANTICS_VERIFIED | prior runtime/prefetch mismatch; fallback |
| 4 | `[0x03A7B4,0x03A7B8)` | 50,474 | 50,474 | 1 | BNE.W -> `0x03A7AE`, direct conditional | RAM-only control | SEMANTICS_VERIFIED | paired with blocked range; fallback |
| 5 | `[0x03A892,0x03A896)` | 26,887 | 26,887 | 1 | BNE.W -> `0x03A8A4`, direct conditional | RAM-only control | SEMANTICS_VERIFIED | outside bounded selected set |
| 6 | `[0x03A8A4,0x03A8AC)` | 26,887 | 26,887 | 1 | CMPI.W absolute-long, fallthrough | RAM | DECODED | exact semantic form not accepted |
| 7 | `[0x03A8C0,0x03A8C4)` | 26,887 | 26,887 | 1 | BNE.W -> `0x03A88C`, direct conditional | RAM-only control | SEMANTICS_VERIFIED | outside bounded selected set |
| 8 | `[0x00026A,0x00026C)` | 16,384 | 16,384 | 1 | MOVE.L D0,-(A6), fallthrough | RAM write | DECODED | exact MOVE/predecrement form not accepted |
| 9 | `[0x00026C,0x000270)` | 16,384 | 16,384 | 1 | DBF D6 -> `0x00026A`, direct conditional | RAM-only control | SEMANTICS_VERIFIED | paired MOVE form remains blocked |
| 10 | `[0x060310,0x060312)` | 11,489 | 11,489 | 1 | MOVE.B (A0)+,(A1)+, fallthrough | RAM read/write | DECODED | exact MOVE overlap form not accepted |
| 11 | `[0x06135E,0x061360)` | 8,192 | 8,192 | 1 | MOVE.B (A1)+,(A2)+, fallthrough | RAM read/write | DECODED | exact MOVE overlap form not accepted |
| 12 | `[0x003A0C,0x003A0E)` | 7,613 | 7,613 | 1 | MOVE.B (A2)+,(A1)+, fallthrough | RAM read/write | DECODED | exact MOVE overlap form not accepted |
| 13 | `[0x00389E,0x0038A0)` | 7,124 | 7,124 | 1 | MOVE.B (A2)+,(A1)+, fallthrough | RAM read/write | DECODED | exact MOVE overlap form not accepted |
| 14 | `[0x003F0,0x003F2)` | 4,565 | 4,565 | 1 | CLR.W (A0)+, fallthrough | RAM write | DECODED | exact CLR memory form not accepted |
| 15 | `[0x003810,0x003818)` | 4,333 | 4,333 | 2 | BTST.B absolute-long; BNE.S, direct conditional | RAM control | DECODED | exact BTST form not accepted |
| 16 | `[0x0030B6,0x0030BE)` | 4,074 | 4,074 | 2 | BTST.B absolute-long; BNE.S, direct conditional | RAM control | DECODED | exact BTST form not accepted |
| 17 | `[0x03A750,0x03A758)` | 3,959 | 3,959 | 2 | BTST.B absolute-long; BNE.S, direct conditional | RAM control | DECODED | exact BTST form not accepted |
| 18 | `[0x00D990,0x00D994)` | 3,567 | 3,567 | 1 | MOVE.W immediate,(A2), fallthrough | RAM write | DECODED | exact MOVE memory form not accepted |
| 19 | `[0x00317E,0x003186)` | 3,302 | 3,302 | 2 | BTST.B absolute-long; BNE.S, direct conditional | RAM control | DECODED | exact BTST form not accepted |
| 20 | `[0x003248,0x003250)` | 3,300 | 3,300 | 2 | BTST.B absolute-long; BNE.S, direct conditional | RAM control | DECODED | exact BTST form not accepted |

The remaining ranked PCs were retained as interpreter fallback and are not
function-boundary claims. No indirect target set was invented. The hardware
candidate `0x060BA4` remains the previously recorded `TST.B 0xA00003`
hardware-visible fallback.

## Semantic, generator and promotion gates

The only new semantic combination was `ADD.W (An)+,Dn`: word-width memory
read, post-increment by two, low-word destination update, full N/V/Z/C/X
behavior and 24-bit address-wrap behavior through the existing bridge. Four
deterministic edge vectors passed the independent reference harness. Existing
Bcc/TST/DBcc forms were reused without broadening their verification claim.

The generator emitted the `[0x000380,0x0003A0)` body and the
`[0x03A864,0x03A868)` body from canonical ROM bytes. Generated bodies and
range metadata are in `generated_blocks_m1139.cpp` and the generated registry;
the handwritten helper, profile writer and registry boundary remain separate.
Unsupported IR still fails closed. Both candidates passed the generic M11.38
per-instruction yield/resume shadow contract before promotion.

## Remaining interpreter Pareto

This is a conservative dynamic attribution ledger for the final 661,916
interpreter executions. Only categories directly supported by the bounded
profile and earlier rejection evidence are assigned; unproven subcategories
are retained in `other proven category` rather than guessed from byte patterns.

| blocker class | instruction executions | % of total guest | % of remaining interpreter | hottest representative |
| --- | ---: | ---: | ---: | --- |
| unsupported semantics | 0 | 0.0000% | 0.0000% | none newly required |
| indirect CFG | 0 | 0.0000% | 0.0000% | no target set claimed |
| hardware-visible access | 914 | 0.0141% | 0.1381% | `0x060BA4` |
| event/interrupt boundary | 0 | 0.0000% | 0.0000% | no residual attribution established |
| decoder unsupported | 0 | 0.0000% | 0.0000% | no selected decoder failure |
| low-priority cold code | 0 | 0.0000% | 0.0000% | not asserted from this trace |
| other proven category: prior runtime/prefetch rejection or bounded unpromoted fallback | 661,002 | 10.1868% | 99.8619% | `0x03A7AE`, `0x03A8A4`, `0x00026A`, `0x060310` |
| **total** | **661,916** | **10.2009%** | **100.0000%** | |

The dominant remaining category is therefore the conservative `other proven`
bucket, not a newly established semantic, indirect-CFG or hardware bridge
blocker. The result is `HOT_PATH_DYNAMIC_COVERAGE_80_PROVEN`; M11.40 is not
implemented here.

All run directories and profiles named by this report are local ignored
evidence only. No ROM, extracted asset, emulator binary, generated run
evidence or `game.srm` is tracked.
