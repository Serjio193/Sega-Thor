# M68K helper #7 — coordinate bound at `0x9D4E` (2026-10-11)

## Result

`0x9D4E` reaches `CPP_REPLACEMENT_VERIFIED_PATH_SPECIFIC = YES` for the exact
`BSR.W` caller at `0x00909E`. Global verification remains NO and runtime
replacement remains inactive. No ROM coverage or ownership authority changed.

## Exact routine and path

Canonical ROM SHA-256:
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
The exact bounded range is `[0x009D4E, 0x009D70)`: 34 bytes, 12 instructions,
one RTS at `0x009D6E`, no nested calls, indirect control flow, or local stack
traffic. The accepted caller path is `0x00909E BSR.W 0x009D4E`, returning to
`0x0090A2`.

The typed model computes 16-bit
`sum = D1.low + word[A6+0x46]` and
`maximum = word[A0+4] * 8 - 1 - sum`. If unsigned `D0.low <= maximum`, D0 is
preserved; otherwise D0.low is clamped to `maximum`. D3.low receives maximum,
D7.low receives sum. When a clamp produces zero, byte `[A6+0x40]` is ORed with
`0x48`. The native implementation preserves high register halves by exposing
only the observed low-word values and a bounded flags byte effect.

## Natural evidence and reads

The replayable corpus contains 24 full calls, all from `0x00909E`, with one
observed control path. Entry D0-D7 and A2/A4/A5/A6 vary; A0/A1/A3/A7 remain
stable in this corpus. At RTS, only D3 and D7 differ among D/A registers; no
RAM writes occur in these natural calls. SR is `0x2100 -> 0x2109` throughout.

The three ordered read patterns reflect three captured record bases for the
word at `[A6+0x46]`: `0xFF1A2E` yields 10, while `0xFF2242` and `0xFF23BA`
yield 9. The word `[A0+4]` is `64` in all captures. These observations show
record-base/input variation, not a proven selector or table-index domain.
All 24 natural samples take the BLS path at `0x009D60`; natural data alone does
not cover clamp branches.

## Caller-visible contract and native oracle

The immediate continuation calls `0x009DD6`; its BTST writes condition state
before the following BNE. On this caller path, the routine's CCR result is
therefore not live. D3/D7 are matched by the native adapter even though the
continuation overwrites them before its later use. A7 is restored by RTS. The
conditional `[A6+0x40]` byte effect is explicitly modeled and tested.

A canonical-ROM HOOK_CPU oracle compared identical bounded routine inputs to
the native model for three ABI-local cases: in-range, nonzero clamp, and zero
clamp with flag-byte OR. All observable registers, return/stack behavior, and
RAM write sets matched in Debug and Release. The native unit test also checked
all 24 captured natural ROM output vectors and 16-bit wrap behavior. Focused
CTest/build passed in Debug, Release, and GNU/Linux. Linux oracle execution was
not run because the configured canonical runtime core is Windows-only; the
Linux native test did run.

## Status and limits

`CPP_REPLACEMENT_CANDIDATE = YES`
`CPP_REPLACEMENT_VERIFIED_PATH_SPECIFIC = YES`
`PATH_SCOPE = direct BSR.W caller 0x00909E`
`CPP_REPLACEMENT_VERIFIED_GLOBAL = NO`
`RUNTIME_REPLACEMENT_ACTIVE = NO`

No new ROM bytes were classified. Runtime, structural, SOURCE_OWNED, and
canonical SQLite authorities are unchanged. The fallback candidate `0x99D6`
was not attempted because `0x9D4E` closed. Machine acceptance receipt:
`build/coordinate-bound-9d4e-path-specific-acceptance-20261011.json`.
