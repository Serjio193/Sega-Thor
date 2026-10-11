# Minimal native island design for the verified M68K helper cohort

Date: 2026-10-11

## Decision boundary

One small dispatcher can handle common identity, transfer provenance, enable, contract-version, and counter checks. It should not unify the eight helper ABIs. Keep eight typed adapters and per-helper contracts. This design does not implement or activate dispatch. Deterministic replay equivalence therefore remains **UNVERIFIED** until a controlled enabled replay is compared to its ROM-only baseline.

All registry entries require canonical ROM SHA-256 `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`. Each contract ID is bound to the acceptance artifact path and SHA-256 in the companion JSON. Every enable flag defaults off. Verification status remains path-specific and independent of enablement.

## Proposed registry

| Entry | Allowed caller PC(s) | Transfer | Contract ID | Evidence | Typed adapter | Status |
|---|---|---|---|---|---|---|
| `0x94A2` | `0x9468`, `0x9480`, `0x949A` | BSR.W | `M68K_0094A2_CALLSET_9468_9480_949A_V1` | 8 ROM-oracle cases Debug/Release | ByteGridView column aggregate → D4/D5 effect plan | Verified path-specific |
| `0x8EF8` | `0x58FA`, `0x5928`, `0x5A6A`, `0x5ABA`, `0x5B9A`, `0x662E` | BSR.W | `M68K_008EF8_CALLSET_58FA_5928_5A6A_5ABA_662E_V1` | 4 ROM cases Debug/Release | bounded raw-record initializer | Verified path-specific |
| `0x16A46` | `0x165A8`, `0x165BE`, `0x165FA`, `0x1667A`, `0x166F0`, `0x1675E`, `0x1685E`, `0x1696A`, `0x169EC`, `0x16A14` | BSR.W | `M68K_016A46_CALLSET_10_BSRW_V1` | 4 full register/CCR and memory cases Debug/Release | sparse raw-word store model | Verified path-specific |
| `0xB730` | `0x3B448` | JSR.L | `M68K_00B730_CALLER_003B448_V1` | 15 natural calls; count and early-exit branches covered | bounded SAT shadow record builder | Verified path-specific |
| `0x9C40` | `0xE376` | BSR.W | `M68K_009C40_CALLER_00E376_V1` | 24 replay calls + 6 synthetic ROM cases | ByteGridView single-cell query → D5 | Verified path-specific |
| `0xE268` | `0x359A` | JSR.L | `M68K_00E268_CALLER_00359A_V1` | 24 natural state-word transitions | state-word transform → D0/FF1928 effects | Verified path-specific |
| `0x9D4E` | `0x909E` | BSR.W | `M68K_009D4E_CALLER_00909E_V1` | 24 natural + 3 synthetic ROM cases | positive coordinate bound model | Verified path-specific |
| `0x9DB4` | `0x92CC` | BSR.W | `M68K_009DB4_CALLER_0092CC_V1` | 24 natural + 5 synthetic ROM cases | negative-difference bound model | Verified path-specific |

No entry is global. Each unlisted direct or indirect caller remains ROM-only. The 8EF8 existing developer-only opt-in route is not expanded or treated as production activation by this design.

## Dispatch boundary and gate

Intercept only at the exact helper entry after the ordinary BSR/JSR has completed. The gate needs fresh CPU transfer metadata from that exact edge; it must never infer a source PC from the return address. Transfer provenance must be invalidated on exceptions, interrupts, non-call transfers, and returns so stale source PCs cannot authorize a route.

Decision order:

1. Helper entry and current core event match the registry record.
2. Canonical ROM hash matches.
3. Transfer source PC and kind match the helper’s accepted set.
4. Global island switch and that helper’s explicit enable are true.
5. Contract ID/version and bound evidence receipt hashes match the compiled adapter.
6. Typed adapter reads only its bounded contract inputs and builds an immutable effect plan without mutating guest state.
7. The entire effect plan validates; only then may an atomic commit occur.
8. Return using a core-provided boundary primitive that has been validated for stack, prefetch, and interrupt semantics.

Any rejection before commit declines at the untouched entry and executes the original ROM bytes. A direct PC/A7 rewrite is not approved by this design. If an effect commit cannot be rolled back exactly, stop the development run rather than continue from a partial guest state.

## Adapter boundary

The shared dispatcher consumes an immutable `CpuSnapshot` plus a read-only bounded guest-memory view, but does not expose a generic mutable register ABI to gameplay code. Each per-helper adapter invokes its existing typed model and returns a typed, bounded effect plan. The adapters differ deliberately: `0x94A2` returns aggregate registers; `0x8EF8` writes five record fields; `0x16A46` writes sparse words and changes A0/D1/SR; `0xB730` emits ordered SAT words and D2/A1; `0x9C40` returns a cell value; `0xE268` transforms one state word; `0x9D4E` and `0x9DB4` compute separate bound contracts.

## ROM reference and failure policy

Development reference mode should use paired deterministic replays or a cloned PRE-state. Never execute ROM and native sequentially against the same mutable guest state. Compare only each receipt’s accepted caller-visible contract, including bounded RAM effects and continuation. Log helper ID, exact caller, contract ID, pre-state, ROM result, and native result. On mismatch, keep ROM authoritative, disable the helper, and fail the replay; if native state was already committed and exact rollback is unavailable, abort.

ROM identity mismatch disables the island. Unsupported caller/transfer, disabled helper, bad contract identity, or invalid bounded input selects ROM before mutation. Native exceptions are caught before commit. Verification labels do not change when a helper is enabled.

## Replay validation and expected counters

Run a deterministic session three ways: all helpers disabled; each helper enabled independently; then all eight enabled. Verify identical effective input polls and ordering, the same pre-existing PC/frame landmarks, relevant RAM checkpoints, and each helper’s accepted output contract. This proves only functional replay behavior; it does not prove cycle timing or bus/prefetch equivalence.

Collect per-helper `native_calls`, `rom_entry_calls`, `accepted_source_hits`, `fallbacks_by_reason`, and `mismatch_count`. Expected acceptance metrics are defined relationally because exact whole-session call totals must be measured from the selected replay: with all flags off, native calls = 0, ROM calls = baseline, fallbacks = 0, mismatches = 0. With one helper enabled, native calls equal only its accepted caller/transfer hits; unsupported calls remain ROM, fallback count equals rejected enabled-target hits, and mismatches = 0. With all eight enabled, native count is the sum of accepted path hits and every remaining target call is ROM.

## Recommendation and unresolved risks

The registry and typed adapter architecture are justified for eight verified path contracts, but gameplay preservation has not yet been demonstrated with all routes active. Keep production activation **NO**. Before any runtime use, validate the core return primitive and transfer-provenance invalidation, then run the replay matrix. Existing proof is path-specific; hidden/indirect callers, cycle/prefetch behavior, bus timing, and interrupt-boundary semantics are outside these receipts.

Machine-readable registry and policy: `THOR_NATIVE_ISLAND_DESIGN_8_HELPERS_20261011.json`. No evidence-authority or coverage changes.
