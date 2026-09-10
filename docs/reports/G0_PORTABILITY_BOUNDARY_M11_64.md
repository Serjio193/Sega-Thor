# M11.64 — G0 portability boundary consolidation

**Status:** `G0_PORTABILITY_BOUNDARY_PROVEN_M11_LINE_CLOSED`

M11.64 closes the M11 line by consolidating the bounded evidence from M11.60–M11.63. G0 (`A5 = 0xFF001A`) has a proven parent-owned lifetime and a reproducible natural behavior cluster. It is not a portable raw transaction boundary and it is not a typed-data boundary. No production/core code, observer, emulator hook, portable routine, VDP abstraction, or gameplay naming was added.

## Authoritative baseline gate

The cold-reset neutral-input 600-frame run used ROM `EB19BDA4982366A2FD43D65AB8A7F9709D83A8CC902C14A682C088C16359C263`, the existing GPGX binary, and target set `0x2D66,0x604BC,0x604F0`.

| Gate | Exact result |
|---|---|
| State checkpoint SHA-256 | `251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d` |
| Video sequence SHA-256 | `5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58` |
| Guest accounting | `6,488,773 total = 6,488,692 interpreter + 34 TableCopy + 40 RamFlag + 7 ParentSuffix` |
| Native override | 5 calls; fallback `0`; divergence `0` |
| Shadow | 5/5 comparisons; divergence `0` |
| Repeat shadow | same checkpoint/video/accounting and same 5/5, divergence `0` |

A mismatch at any gate is a STOP condition. The exact M11.64 rerun matched all gates.

## Bounded-G0 dependency ledger

The interval is the natural path from G0 materialization at `0x060182` to the parent restore at `0x06027E` (then `0x060284 RTS`). The ledger records the contract that is actually proven, not a whole-static claim.

| Dependency / boundary | Classification | Evidence and remaining limit |
|---|---|---|
| `0x060182 LEA.L $FF001A,A5` | `PROVEN_NATURAL_CONTRACT` | G0 materializes as A5 exactly on the bounded natural path. |
| Parent continuation `0x0601E2 -> 0x062AE0` | `PRESERVATION_PROVEN_EFFECTS_BLOCKED` | A5/entry-return behavior is preserved in M11.61 evidence; latent static indirect CFG at `0x062CEC` leaves whole-static effect closure open. |
| Six natural `0x061934` sites | `PROVEN_NATURAL_CONTRACT` | M11.62 closes natural entries/returns and effects; latent static indirect CFG at `0x061F60` remains a static debt. |
| Four natural `0x0623AC` sites (`0x060234`, `0x060242`, `0x060250`, `0x060276`) | `HARDWARE_BOUNDARY` | M11.63 closes natural A5/effect observation but records 407 VDP writes at `0x00C00011`; this is a real portability boundary. Static indirect CFG remains at `0x062878`. |
| Parent continuation `0x06027A -> 0x060286` | `NOT_YET_CLOSED` | The callee is intentionally deferred; no execution or new observer was added in M11.64. |
| `0x06027E MOVEM.L (A7)+,D0-D7/A0-A6` | `PROVEN_NATURAL_CONTRACT` | Parent restores its saved register frame after the bounded G0 consumers. |
| `0x060284 RTS` | `PROVEN_NATURAL_CONTRACT` | Parent-owned lifetime ends at the observed return. |
| `0x062CEC` latent edge from `0x062AE0` | `STATIC_ONLY` | Static indirect CFG debt; no natural target promotion. |
| `0x061F60` latent edge from `0x061934` | `STATIC_ONLY` | Static indirect CFG debt; natural targets/effects are bounded and closed. |
| `0x062878` latent edge from `0x0623AC` | `STATIC_ONLY` | Static indirect CFG debt; natural targets/effects are observed, hardware remains blocking. |
| External writers and aliasing of the G0 region | `UNRESOLVED` | No typed ownership claim is justified while overlapping/shared access remains possible. |
| Broader lifetime around `0x061258` | `NOT_YET_CLOSED` | Parent/context ownership outside the bounded interval is unresolved. |

`0x060286` is included as a ledger dependency but was not analyzed. Proving it later would not change the architectural decision: **`NO_ARCHITECTURAL_DECISION_CHANGE`**. The parent-owned lifetime, hardware boundary, and typed-data block already determine the M11 result.

## Four-gate matrix

| Gate | Result | Decision |
|---|---|---|
| A — register/lifetime | Parent-owned lifetime proven from `0x060182` through `0x06027E`/`0x060284`; A5 equality is exact on observed natural entries/returns. | Useful behavioral evidence; not standalone ownership. |
| B — call/effect | Natural entries, returns, nested paths and effects are substantially closed; whole-static debt remains at `0x062CEC`, `0x061F60`, `0x062878`, and `0x060286`. | No portable standalone call promotion. |
| C — hardware portability | `0x0623AC` performs six distinct byte-write PCs to VDP `0x00C00011`, 407 natural events total. | Hardware boundary is proven; no mock or VDP abstraction. |
| D — typed-data ownership | Raw G0 accesses overlap/shared ownership and external-writer closure is absent. | `G0_TYPED_DATA_BOUNDARY_BLOCKED`; no typed struct or shared data promotion. |

## Hardware evidence

The M11.63 observer recorded six source PCs, all byte writes (`width=1`) to `EA=0x00C00011`, in the selected `0x0623AC` CFG/direct instruction paths. They are not inferred from a mock and their order/path signatures were recorded; values were intentionally not captured, so exact data values and gameplay meaning remain unknown.

| Source PC | Natural count | Order span | Natural path signatures |
|---|---:|---|---|
| `0x062632` | 1 | `81–81` | `0xCE40BF6F` |
| `0x062688` | 14 | `192–249` | `0x3BA73731`, `0xCE40BF6F`, `0xBE98B2D9`, `0x4493B02B`, `0x27086DF3`, `0x75F01AF1`, `0xD817D017` |
| `0x062690` | 14 | `195–252` | same seven paths |
| `0x06272E` | 8 | `26–52` | `0x3BA73731`, `0xCE40BF6F`, `0x4493B02B` |
| `0x06277A` | 176 | `158–56` | `0xBE98B2D9`, `0x11764643`, `0x8920F979`, `0x27086DF3`, `0x75F01AF1`, `0xD817D017` |
| `0x062784` | 194 | `157–46` | `0x3BA73731`, `0xD1087353`, `0xBE98B2D9` |

These sum to 407 writes. No interrupt or unresolved runtime address was observed. The evidence proves a hardware dependency, not a semantic VDP interface.

## Deferred debt and promotion rule

Keep these items explicitly unresolved: `0x060286`; latent `0x062CEC`, `0x061F60`, and `0x062878`; parent ownership beyond the bounded interval; external writers/aliasing; and the broader lifetime around `0x061258`. Do not convert them into typed shared data, a raw portable transaction, a standalone routine, or a production subsystem without new independent evidence.

## M11 closure and transition

The preferred closure is `G0_PORTABILITY_BOUNDARY_PROVEN_M11_LINE_CLOSED` with subresults `G0_PARENT_OWNED_LIFETIME_PROVEN`, `G0_NATURAL_HARDWARE_DEPENDENCY_PROVEN`, `G0_TYPED_DATA_BOUNDARY_BLOCKED`, and `G0_FURTHER_CALLEE_CLOSURE_DEFERRED_LOW_DECISION_VALUE`. No ADR is warranted because this report consolidates already accepted boundaries.

Recommend exactly one next major milestone: **M12 — Inventory / UI / Save** (proposal only; do not start it in M11.64). Candidate methods are listed in `docs/RE_METHOD_CATALOG.md`: save/load serialization as a RAM oracle, RAM watch and controlled perturbation, snapshot differential slicing, runtime provenance/taint, resource/table graph reconstruction, and parser-as-detector only where structure justifies it. Semantics remain hypotheses until independently evidenced. STOP after M11.64.

## Validation record

Documentation-only diff; no production/core changes. Exact baseline gates above passed. Existing Debug and Release CTest runs are 74/74, including source-limit and M11.63 regression coverage. UCRT remains `LOCAL_TOOLCHAIN_ENVIRONMENT`: the pre-existing `tests/raw_data_provenance_test.cpp` compile failure occurs before diagnostics, so no new UCRT coverage is claimed. `git diff --check`, source-limit and tracked hygiene are required again before commit/push.
