# M11.22 — Bounded Static Classification for `0x060BB6-0x060BC4`

Decision: `BOUNDED_REGION_060BB6_STATIC_SUPPORTED`

The classification unit is exactly the eight instruction-start addresses from
`0x060BB6` through `0x060BC4`, inclusive. This is not a whole-routine
classification and does not assign a semantic name or function identity.

## Before and after

| Scope | Before | After |
|---|---|---|
| bounded unit `0x060BB6-0x060BC4` | `UNKNOWN` in current exact classification data; runtime facts were `RUNTIME_EXECUTED_UNKNOWN` | `CODE_STATIC_SUPPORTED` |
| target address facts | no static range promotion | `CODE_EXECUTED_AT_ADDRESS` for each observed target PC |
| routine boundary | not claimed | `boundary_status=LIKELY_INTERNAL_BLOCK` |

No adjacent address or whole-routine classification changed. In particular,
`0x060B90`, `0x060BAA`, `0x060BAE`, `0x060BC4`, `0x060BCC` and `0x0604BC`
were not promoted as part of this unit. `0x060BC4` is listed as the final
instruction-start in the inclusive unit, but its instruction bytes do not
expand the classification boundary.

## Re-verification

| Check | Result |
|---|---|
| canonical ROM bytes | 8/8 exact byte matches |
| exact decode | 8/8 `DECODED` |
| runtime execution | 8/8 observed |
| address-level execution facts | 8 `CODE_EXECUTED_AT_ADDRESS` |
| routine identity | none |
| boundary | `LIKELY_INTERNAL_BLOCK` |

Target PCs: `0x060BB6`, `0x060BB8`, `0x060BBA`, `0x060BBC`, `0x060BBE`,
`0x060BC0`, `0x060BC2`, `0x060BC4`.

The target instructions are six `NOP`s, a direct branch at `0x060BC2`, and
the decoded `MOVE.W` at `0x060BC4`. Their canonical raw bytes and exact
decoder text are retained in the JSON artifact.

## Direct CFG evidence

| Source | Target | Type | Evidence | Target observed |
|---|---|---|---|---|
| `0x060BAE` | `0x060BB6` | fallthrough | `STATIC_PROVEN` explorer | yes |
| `0x060BC2` | `0x060B90` | direct jump | exact bounded decoder + `STATIC_PROVEN` explorer | yes |
| `0x060BAA` | `0x060BC4` | conditional branch | exact bounded decoder + `STATIC_PROVEN` explorer | yes |
| `0x060BCC` | `0x0604BC` | direct call | exact bounded decoder + `STATIC_PROVEN` explorer | yes |

The last edge is immediate surrounding context required to corroborate the
target's continuation; it does not expand the classification unit to either
the call site or callee.

## Provenance

Canonical ROM SHA-256:
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

| Evidence | Artifact SHA-256 |
|---|---|
| M11.19 runtime evidence `build/gpgx_runtime_execution_evidence.json` | `e6d8784fa8bbb6658d68e60c41a25788e6474e5c4a95c0e695b5288d34875efc` |
| M11.20/current exact decoder `build/m11-20-global.json` | `06c1ededade003a5371fb9480fb55fcb63dd659f0a1db8cd7b9a598ec5d67e23` |
| M11.21 bounded decoder `build/m11-21-window.json` | `d91db34f7af6bf0ea162fad283ac7932a9c912634af480ca1503eda5aa081b2f` |
| M11.20 structural explorer `build/m11-20-explore.json` | `3f5bd3b584089be49cd3c303d03c657686ea73a244b648f2ee561dfdfab35c63` |
| M11.21 structural report `docs/reports/RUNTIME_REGION_060BB6.md` | `9f240b2c0193fe14b998d79c1410ae06abb467b3d2cc9e6f5ae9d0dad3fc5f4b` |

The deterministic classification artifact is
`build/m11-22-gpgx-bounded-classification.json`, SHA-256
`ed8dff4f77fa31bda65846e2d606296d0abc64b15ea9b3fa0d0992d861d5a712`.

## Regression tests

The bounded classifier has tests for:

- complete canonical decode plus runtime evidence upgrades only the exact
  unit to `CODE_STATIC_SUPPORTED`;
- one decode failure prevents the upgrade;
- missing runtime evidence preserves the previous classification;
- boundary remains `LIKELY_INTERNAL_BLOCK` with no routine identity;
- deterministic output across repeated evaluation.

No broad trust-model or automatic promotion logic was changed.

## Exact next recommendation

**A. investigate parent routine boundary around `0x060B90`**.

This recommendation is not implemented in M11.22.
