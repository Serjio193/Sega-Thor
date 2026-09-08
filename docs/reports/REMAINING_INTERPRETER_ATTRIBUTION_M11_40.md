# M11.40 Remaining Interpreter Attribution and 95% Coverage Gate

## Result

`M11.40_BASELINE_BLOCKED_CHECKPOINT_IDENTITY_MISMATCH`

M11.40 stopped at PHASE 1. No interpreter promotion, semantic expansion,
generator change, shadow certification or 95% coverage claim was made.

## Frozen identity and reproduction

The run used the committed M11.39 checkout at
`37857a31a1c2965ecbe68e3695ca5aa187617c2f`, the canonical USA ROM, the
external GPGX DLL and the unchanged cold-reset neutral 600-frame scenario.

| identity / metric | M11.39 recorded | M11.40 reproduction |
| --- | --- | --- |
| ROM SHA-256 | `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263` | same |
| GPGX DLL SHA-256 | `140c00fc7475cf22ca22415d65dfc7ffc66523de128454f9994e7ab77d9826fd` | same |
| frames | 600 | 600 |
| registry ranges | 28 | 28 |
| total guest instructions | 6,488,773 | 6,488,773 |
| translated instructions | 5,826,857 | 5,826,857 |
| interpreter instructions | 661,916 | 661,916 |
| translated share | 89.7991% | 89.7991% |
| video sequence SHA-256 | `5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58` | same |
| instruction-boundary yields | 140,065 | 140,065 |
| interrupted resumptions | 274 | 274 |
| original starts inside translated ranges | 0 | 0 |

The checkpoint aggregate differs: historical M11.39 recorded
`20217e10565c51b571db6a4e474aa40854ea6c22277ba14c46ea97c4b1c60a04`, while
two independent M11.40 reproductions produced
`fffe59fcdbed7fdac8ef22badb4f7236b8619459fed27c9931e0f93906549052`.
The per-frame current checkpoint records are stable across those two
reproductions. The cause of the difference from the historical artifact is
not proven by the available evidence.

## Stop decision

The exact M11.40 baseline was not reproduced because the required checkpoint
identity differs. Per the milestone contract, the exhaustive remainder ledger
and all later phases are not valid until this identity discrepancy is resolved.
The `661,916` count is reproduced, but it is not accepted as a complete final
M11.40 attribution ledger. M11.39 remains the latest valid coverage result;
its 89.7991% gate and evidence are preserved unchanged.

No ROM, extracted asset, emulator binary, generated run evidence or `game.srm`
is tracked. The two run directories are local ignored evidence only.
