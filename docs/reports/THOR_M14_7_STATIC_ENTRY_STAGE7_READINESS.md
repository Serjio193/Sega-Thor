# M14.7 — Static entry proof and Stage7 promotion readiness

**Status:** `PASS_STATIC_ENTRY_STAGE7_READINESS_V1`

Canonical parent `gen-m14-6-dcb2596abccf9612`; child `gen-m14-7-29314da7a34a38e6`. ROM SHA-256 `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263` (3,145,728 bytes).

## Result

The canonical scan decoded 12,603 instructions (47,606 bytes) across 531 source-owned 68K ASM emission spans. It found 3 xrefs: zero direct call targets, zero direct branch/jump targets, and three verified fallthroughs. All three land exactly at accepted component instruction starts and were admitted with `STATIC_VERIFIED` claims, relation evidence, and derivations.

These proofs affect 172 bytes across 3 components. The existing Stage7 selector admitted 0 components before and after; it requires incoming `STATIC_VERIFIED` `DIRECT_CALL_TARGET` / `STATIC_CALLER` relations, so fallthrough entry proofs do not satisfy its caller gate. No full-ROM manifest was regenerated and no promotion was attempted.

## Component audit

| Extent | Bytes | CFG SHA-256 | Roundtrip SHA-256 | Existing incoming | Runtime entry observations | Static entry after | Stage7 blocker |
|---|---:|---|---|---|---:|---|---|
| `0x00045A..0x00045C` | 2 | `abc6580bb85a49517b859740b1e4786cf41de9e27785975d182b4bea8355bd95` | `a2171d058b274b6d84d6a27d3d14287be828ed81502ef920986c4fe48ce5592e` | EXECUTED_NEXT/OBSERVED_RUNTIME | 1 OBSERVED_RUNTIME | yes | `NO_STATIC_VERIFIED_DIRECT_CALL_TARGET_OR_CALLER_FOR_EXISTING_SELECTOR` |
| `0x001CFE..0x001D30` | 50 | `bd5dd077f0e2babab27a34dd4658b59fcae8d49467bd6810e865df6b3cb71598` | `bc439b3aded2283fa3f437a6dad062de1dd8fe62fa8cc8b03c8b0823007a025e` | EXECUTED_NEXT/OBSERVED_RUNTIME | 1 OBSERVED_RUNTIME | yes | `NO_STATIC_VERIFIED_DIRECT_CALL_TARGET_OR_CALLER_FOR_EXISTING_SELECTOR` |
| `0x03C4B6..0x03C52E` | 120 | `54d54204703fd4978d7692d8f6483933dacd4ce137ae28dedba5b9331c25b132` | `0e6c53a1d875f9b8d8124772567a0ca9e255ad5bbebd7b93b16b2c795006a8c1` | EXECUTED_NEXT/OBSERVED_RUNTIME | 1 OBSERVED_RUNTIME | yes | `NO_STATIC_VERIFIED_DIRECT_CALL_TARGET_OR_CALLER_FOR_EXISTING_SELECTOR` |
| `0x062218..0x06224A` | 50 | `224313a989d7093afebb1103d3bf31ce59c1eb1fa6ddb32b857d8a179d2042f4` | `3d3504414073a65d248eef89951e89508507c276a9a1ba2db681e9e3967a0822` | EXECUTED_NEXT/OBSERVED_RUNTIME | 2 OBSERVED_RUNTIME | no | `NO_STATIC_VERIFIED_DIRECT_CALL_TARGET_OR_CALLER_FOR_EXISTING_SELECTOR` |
| `0x062986..0x0629AC` | 38 | `d598dea8284714d4ed3bce0909784137665ea43d9c9f72f3427c49a17e9bd808` | `a154ea52b6c2c1ca34c4c75e119f4485ec92eed9fb921c42c93d6310ec58aba5` | EXECUTED_NEXT/OBSERVED_RUNTIME | 1 OBSERVED_RUNTIME | no | `NO_STATIC_VERIFIED_DIRECT_CALL_TARGET_OR_CALLER_FOR_EXISTING_SELECTOR` |

Each entry has existing outgoing `EXECUTED_NEXT / OBSERVED_RUNTIME` relations. Pre-existing claims are `CANONICAL_ASM_EXTENT / DERIVED_EXACT` and `EXECUTED_FROM_ROM / OBSERVED_RUNTIME`. All five extents remain `ASM_ROUNDTRIP_EXACT` and non-owning.

## Exact admitted fallthroughs

| Caller PC | Caller range | Opcode / instruction | Target entry | Relation and evidence |
|---|---|---|---|---|
| `0x000456` | `0x000456..0x00045A` | `0x2271 move` (SEQUENTIAL_FALLTHROUGH) | `0x00045A` | `relation:2cacba1e1d89c76b5d873685fa653691461319cda2396a0f473a489e81975dc0`; evidence `evidence:2f856fc70b5df0ed2d879485401d8d63ec46ffa16471bb99ade2c160ecf27932`; derivation `derivation:73b9722183fc07e2ee95c5a6286214fcae2fe64f85e40458b0adcc69628a8feb` |
| `0x001CF8` | `0x001CF8..0x001CFE` | `0x3CF9 move` (SEQUENTIAL_FALLTHROUGH) | `0x001CFE` | `relation:54c7688b63465820965611a73f2a17410b2e03482dd63ecd38b2ba941569c409`; evidence `evidence:0fbb447e13878812aa5c09bd820a38b5d851515918e41965d9ac8a617f7d2655`; derivation `derivation:9c424304b30330e35f416e749cae415f5b38d21116a497578a177b21ba3ab87a` |
| `0x03C4B0` | `0x03C4B0..0x03C4B6` | `0x4EB9 jsr` (CALL_RETURN_FALLTHROUGH) | `0x03C4B6` | `relation:47200500de390e4e13265475a80305edeec610b3f3e073692448140e89ea92ef`; evidence `evidence:44f076c9eca1d19a377aea82a395ff0418e55c290c0603d201baf95287c5cd01`; derivation `derivation:c65ebf0ea710e676d53a03809cd3ed1e7cd1c4e94849b07bdebc3f59ad242853` |

The predecessors are exact instructions in source-owned ASM emissions and their bytes match canonical ROM hashes. Each target equals the component entry object. The JSR proof is its return fallthrough; it does not assert that the JSR target is the component.

The graph contains pointer-table objects but no typed `TABLE_ENTRY` objects. No arbitrary ROM-byte patterns were treated as pointer references.

## Invariants and replay

- Components audited: 5 / 260 bytes.
- Static xrefs: 3 candidates; 3 accepted; new verified callers: 0.
- Stage7 eligible: 0 components / 0 bytes; manifest regenerated: `FALSE`; promoted: 0 components / 0 bytes.
- Map: 2490 ranges; gaps 0; overlaps 0; UNKNOWN 764 ranges / 1,657,796 bytes.
- SOURCE_OWNED: 1,487,672 → 1,487,672 (delta 0).
- Static scan replay and two-child logical graph replay: `TRUE`.

The next evidence needed by the unchanged Stage7 gate is an exact canonical `STATIC_VERIFIED` direct call/caller relation to a component entry. The full-ROM promotion artifact path remains untested because no component passed the selector.
