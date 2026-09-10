# M12.1 — Transactional ASM promotion of `0x06042A..0x0611F4`

## Result

`M12_1_P0_CODE_PROMOTION_PARTIAL_EXACT`

This report records the single M12.1 transaction requested from the M12.0
baseline. The work is limited to developer-only reconstruction tooling and
documentation. No native C++ gameplay/runtime routine, emulator feature,
portable subsystem, ROM, BIOS, extracted asset, or M12.2 implementation was
added.

Baseline commit: `b4937c8e1b99de7ca4f45cb20b3cdf37df51e932`
Baseline manifest: local M12.0 materialization with 203 `CODE_VERIFIED` ranges
and 136 `UNKNOWN` ranges
Canonical ROM: 3,145,728 bytes, CRC32 `C4728225`, SHA-1
`2944910c07c02eace98c17d78d07bef7859d386a`, SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`

## Ownership and gap census

The selected P0 region is exactly `0x06042A..0x0611F4`, 3,530 bytes. The
before/after ownership accounting is:

| View | Before | After |
|---|---:|---:|
| Source-owned ASM | 0 | 546 |
| Structured data | 0 | 0 |
| Padding | 0 | 0 |
| Remaining local-ROM UNKNOWN blob | 3,530 | 2,984 |
| Total target bytes | 3,530 | 3,530 |

The mutually exclusive remaining-gap census is:

| Category | Bytes | Meaning |
|---|---:|---|
| `POSSIBLE_CODE_BYTES` | 0 | No bytes are counted here without a closed source-owned slice. |
| `UNKNOWN_DATA_BYTES` | 0 | No data interpretation is claimed. |
| `UNRESOLVED_BOUNDARY_BYTES` | 2,984 | Conservative UNKNOWN blobs whose dispatch/case or continuation boundary is not closed. |

The gap categories are a boundary census, not additional ownership classes;
the 2,984 bytes remain represented exactly by local-ROM blobs.

## Promoted intervals

Every row was decoded by `re_slice_decoder`, assembled with the existing local
vasm M68k assembler (`-m68000 -no-opt -Fbin`), and compared byte-for-byte with
the canonical ROM slice before the manifest transaction was committed.

| Seed PC | Interval | Bytes | Evidence classification | Instructions | Approx. blocks | Direct edges | External edges | Returns | Memory refs | Unsupported |
|---|---|---:|---|---:|---:|---:|---:|---:|---:|---:|
| `0x06042A` | `0x06042A..0x060484` | 90 | `CODE_EXECUTED` | 24 | 10 | 9 | 9 | 0 | 3 | 0 |
| `0x060490` | `0x060490..0x0604B0` | 32 | `CODE_STATIC_SUPPORTED` | 8 | 5 | 4 | 4 | 0 | 0 | 0 |
| `0x060B50` | `0x060B50..0x060CDA` | 394 | `CODE_EXECUTED` | 111 | 12 | 17 | 13 | 0 | 48 | 0 |
| `0x0611D6` | `0x0611D6..0x0611E0` | 10 | `CODE_STATIC_SUPPORTED` | 4 | 1 | 0 | 0 | 1 | 2 | 0 |
| `0x0611E0` | `0x0611E0..0x0611EA` | 10 | `CODE_STATIC_SUPPORTED` | 4 | 1 | 0 | 0 | 1 | 2 | 0 |
| `0x0611EA` | `0x0611EA..0x0611F4` | 10 | `CODE_STATIC_SUPPORTED` | 3 | 3 | 2 | 2 | 1 | 0 | 0 |
| **Total** | — | **546** | — | **154** | — | **32** | **28 (21 unique targets)** | **3** | **55** | **0** |

The row edge counts intentionally preserve repeated direct edges in the source
slice; external edges are counted per slice and are not a claim that the
destination is unknown. No unresolved control-flow edge or unresolved memory
reference was accepted by the transaction.

## Static predecessor/successor evidence

The legacy whole-region global census supplied 568 decoded instruction records,
eight static coverage gaps, and three legacy unsupported addresses. Relevant
predecessors for the six accepted entries were:

| Accepted entry | Static predecessors in the global map |
|---|---|
| `0x06042A` | `0x060004` |
| `0x060490` | `0x060442` |
| `0x060B50` | `0x060494` |
| `0x0611D6` | `0x060512`, `0x060648`, `0x06074A`, `0x060880`, `0x0609B8`, `0x060CD6`, `0x060F50`, `0x060F88`, `0x061170`, `0x06117A`, `0x06119E` |
| `0x0611E0` | `0x060B60`, `0x060F2A`, `0x060F34`, `0x060F62`, `0x060F6C` |
| `0x0611EA` | `0x0604E6`, `0x061322` |

Direct successor targets emitted by the accepted slices include the following
bounded and external continuations (hex addresses; repeated targets in the
source are retained in the machine-readable transaction evidence):

| Slice | Successor targets |
|---|---|
| `0x06042A..0x060484` | `0x060490`, `0x0604E6`, `0x060A8A`, `0x06064C`, `0x06074E`, `0x060884`, `0x0609BC`, `0x0609C6`, `0x060516` |
| `0x060490..0x0604B0` | `0x060B50`, `0x06112E`, `0x060F1C`, `0x060F54` |
| `0x060B50..0x060CDA` | `0x0611E0`, `0x060B70`, `0x06121A`, `0x060B98`, `0x060BC4`, `0x060B90`, `0x0604BC`, `0x060F8C`, `0x061032`, `0x0610C8`, `0x0611D6` |
| `0x0611EA..0x0611F4` | `0x0611F4`, `0x06121A` |

The final manifest validator found 0 gaps and 0 overlaps. The exact slice
assembler also rejects branch-into-interior and unsupported/truncated forms;
none occurred in the six accepted slices.

## Decoder correction

The old global census treated `0x06042A`, `0x0611DC`, and `0x0611E6` as
unsupported status-register instructions. The exact decoder now normalizes the
two legal M68000 forms (`MOVE SR,<ea>` and `MOVE <ea>,SR`) without changing
runtime behavior or inventing semantics. The synthetic regression covers
`0x40E7` and `0x46DF`. All three addresses are now included in exact slices,
and their local assembler output matches the ROM. This closes a decoder
coverage limitation only; it does not close the remaining region CFG.

## Remaining target blobs

| Remaining interval | Bytes | Reason not promoted |
|---|---:|---|
| `0x060484..0x060490` | 12 | Boundary between dispatch fallthrough and the next case arm is not independently closed. |
| `0x0604B0..0x060B50` | 1,696 | Large dispatch/case span has decoded islands but no single bounded source-owned CFG boundary. |
| `0x060CDA..0x0611D6` | 1,276 | Multiple case arms and linear/CFG continuation remain unresolved. |
| **Total** | **2,984** | Retained as exact UNKNOWN blobs. |

The eight legacy static coverage gaps are `0x060484..0x060490`,
`0x0604B0..0x0604BC`, `0x0609BC..0x060B50`, `0x060CDA..0x060F1C`,
`0x06102A..0x061030`, `0x0610C0..0x0610C6`, `0x0610E4..0x0610EA`, and
`0x0611A2..0x0611D6`. These are evidence of incomplete static coverage, not
permission to promote the surrounding blob bytes.

## Whole-ROM metrics

| Metric | M12.0 before | M12.1 after |
|---|---:|---:|
| ASM ranges | 203 | 209 |
| ASM bytes | 13,550 | 14,096 |
| Structured-data bytes in exact map | 0 | 0 |
| Blob ranges | 136 | 138 |
| Blob bytes | 3,132,178 | 3,131,632 |
| Conflicts | 0 | 0 |
| Gaps / overlaps | 0 / 0 | 0 / 0 |
| ASM percentage | 0.430742900% | 0.448099772% |
| Blob percentage | 99.569257100% | 99.551900228% |
| Executable blob bytes removed | — | 546 |

The rebuilt ROM is exact: size 3,145,728; CRC32 `C4728225`; SHA-1
`2944910c07c02eace98c17d78d07bef7859d386a`; SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
The materialized ASM contains no executable `incbin`; remaining blobs use the
existing local-ROM reconstruction mechanism.

## Single next proposal

M12.2 is not started. The only proposal from the remaining P0 ranking is
`0x00DE00..0x00E338` (1,336 bytes), with 31 observed PCs and 87 static xrefs.
It is a proposal only and does not alter the M12.1 result or authorize work in
this task.

## Validation record

The transaction was rerun from a new output directory after the decoder fix.
The following checks were completed before the implementation commit was
pushed:

- targeted Debug and Release builds for the changed reconstruction tools;
- helper and exact reassembly regressions in Debug and Release CTest;
- full Debug CTest: **75/75 passed**, including source file-size and ROM identity gates;
- full Release CTest: **75/75 passed**, including source file-size and ROM identity gates;
- fresh GNU-equivalent MinGW Release build and targeted CTest: **2/2 passed**;
- exact full-ROM evidence audit of the materialized manifest: **209/209 round-trip exact**;
- `git diff --check`, changed-source line limits, and tracked artifact hygiene: passed.

The independent evidence audit intentionally reports its own provenance view:
203 ranges remain `ASM_ROUNDTRIP_EXACT`, five promoted ranges are
`CODE_STATIC_SUPPORTED`, and one range is `CODE_EXECUTED`; it records five
provenance mismatches and one dynamic-evidence range because the audit's
historical candidate/Ghidra maps predate this transaction. Those warnings do
not alter the transaction's exact byte result or authorize any remaining blob
promotion.

Commit SHA and CI run are appended to the M12.1 worklog entry after push.
