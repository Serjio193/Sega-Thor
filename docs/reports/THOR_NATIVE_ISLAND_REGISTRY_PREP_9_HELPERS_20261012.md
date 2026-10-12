# Nine-helper native island registry preparation

Date: 2026-10-12

Design-only update to the eight-helper proposal. The verified path-specific cohort now has nine members after `0x9D92`; this note records the exact caller gates, transfer kinds, contract identifiers, evidence basis, adapter boundary, branch coverage qualification, and current live-route status. It does not implement dispatch or activate any helper. All native-island enables remain false and production replacement remains inactive.

A shared dispatcher can validate ROM identity, target, exact transfer source/kind, contract identity, and per-helper enablement. Each entry retains its own typed adapter and bounded effect plan. Gate failure must occur before guest mutation and continue through the original ROM entry. Verification status is independent from enablement.

| Entry | Accepted caller(s) | Kind | Natural replay captures | Branch coverage qualification | Live route |
|---|---|---|---:|---|---|
| `0x0094A2` | 0x009468, 0x009480, 0x00949A | BSR.W | not recorded | 8 bounded ByteGridView oracle cases; arbitrary caller inputs not claimed | NO_ROUTE |
| `0x008EF8` | 0x0058FA, 0x005928, 0x005A6A, 0x005ABA, 0x005B9A, 0x00662E | BSR.W | 0 | six direct callsite probes; no natural gameplay call claimed | DEV_OPT_IN_ONLY |
| `0x016A46` | 0x0165A8, 0x0165BE, 0x0165FA, 0x01667A, 0x0166F0, 0x01675E, 0x01685E, 0x01696A, 0x0169EC, 0x016A14 | BSR.W | not recorded | straight-line body; four oracle effect cases; no branch paths | NO_ROUTE |
| `0x00B730` | 0x003B448 | JSR.L | 15 | YES; bounded ABI branches complete, including both culls and D2 limit | NO_ROUTE |
| `0x009C40` | 0x00E376 | BSR.W | 24 | 24 natural plus 6 synthetic path-scoped grid/coordinate cases | NO_ROUTE |
| `0x00E268` | 0x00359A | JSR.L | 24 | 24 natural state-word transitions; no broader branch-domain claim | NO_ROUTE |
| `0x009D4E` | 0x00909E | BSR.W | 24 | 24 natural plus 3 synthetic bounded cases | NO_ROUTE |
| `0x009DB4` | 0x0092CC | BSR.W | 24 | 24 natural plus 5 synthetic covering both exits and clamp predicates | NO_ROUTE |
| `0x009D92` | 0x009812 | BSR.W | 24 | 24 natural plus 4 synthetic covering clamp/flag/wrap cases | NO_ROUTE |

The machine registry binds each contract to its existing evidence receipt. For `0x8EF8`, the existing route is developer-only and opt-in; it is not production activation. The other helpers have no live route. Replay validation remains pending: compare all-disabled baseline against each helper independently and then the cohort, using identical inputs and landmarks while measuring native calls, ROM calls, fallback reasons, and mismatches. No cycle/prefetch equivalence is implied.

The helper #10 scout `0x99D6` is not included: its read arithmetic is exact, but pointer extent and valid input domain are unresolved. No byte classification or authority change resulted.
