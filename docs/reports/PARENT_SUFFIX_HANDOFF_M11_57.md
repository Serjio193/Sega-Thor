# M11.57 — Parent-owned suffix handoff and portable internal helper gate

Status: `FIRST_PORTABLE_INTERNAL_HELPER_PROVEN`
Baseline commit: `ca95f24ebb7b92d2943166a802d8aeef8a70d5d0`

## Decision

The proven seven-instruction sequence beginning at `0x604F0` is representable as a
parent-owned internal helper. It is not a third standalone routine. The helper
owns five unconditional safe-RAM `SF.B` writes, one structural `RamFlagRoutine`
invocation, and the final parent-continuation handoff. The parent retains its
prologue, 58-byte frame, hardware prefix, SR/CCR/stack-bank behavior, shared
epilogue, and enclosing `RTS`.

The core contract uses opaque tokens and supplied addresses. No ROM PC, opcode,
GPGX type, hardware register, or `0x611D6` value occurs in `oasis_core`.
`ParentSuffixContinuation` carries the next helper phase and the nested
`RamFlagRoutine` continuation. A completed handoff is represented as an opaque
parent continuation token; it is not an `RTS` result.

## Baseline gate

The unchanged M11.56 native scenario was run twice from the pinned external
GPGX bridge and canonical USA ROM. Both runs reproduced:

* checkpoint aggregate `251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d`;
* video aggregate `5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`;
* `6,488,773` total = `6,488,699` interpreter + `34` TableCopy + `40` RamFlag;
* zero fallback and divergence;
* caller attribution `1 x 0x604F6`, `3 x 0x60BCC`, unknown `0`; and
* the existing one event yield and one resumption.

The ROM SHA-256 is
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`; the
external bridge SHA-256 is
`140c00fc7475cf22ca22415d65dfc7ffc66523de128454f9994e7ab77d9826fd`.

## Contract and ownership

At entry the parent guarantees an even A7, A5 equal to the supplied base
(`FF001A` in the adapter), and the current condition-code state. The helper
consumes A5, the CCR-derived `SF` condition (false), and the RamFlag input
state. D registers, unrelated A registers, full SR, saved-frame bytes, and
stack-bank state are preserved by the helper and remain parent-owned. The
nested BSR return slot is represented by the adapter; the portable core only
consumes the structured RamFlag return continuation.

The outputs are ordered `FF0012`, `FF0010`, `FF0011`, `FF0013`, `FF0014`, then
an opaque parent handoff. RamFlag writes its established `FF0628`, `FF06F2`,
A5-relative `+5..+7`, and `FF0016` locations. A7 is restored by the composed
RamFlag return. No parent frame byte is read or written. The helper does not
restore SR, interrupt mask, or stack banks.

| Responsibility | Owner |
|---|---|
| 58-byte frame, full SR, interrupt mask, stack banks | parent/epilogue |
| hardware prefix and hardware oracle | hybrid adapter/oracle |
| five suffix SF writes and phase continuation | portable suffix core |
| RamFlag structured effects | existing RamFlag core |
| nested BSR representation and canonical bytes | hybrid adapter |
| parent continuation mapping and epilogue execution | hybrid adapter/parent |
| final enclosing RTS | shared parent epilogue |

## Adverse and event tests

`oasis_parent_suffix_test` derives an independent machine and rejects wrong A5,
invalid continuation, and invalid phase state. It proves the exact write order,
A5-relative RamFlag composition, opaque handoff token, and unchanged parent A7.
The same test resumes after every one of the 17 represented boundaries,
including immediately before the RamFlag call, at every resumable RamFlag phase,
immediately after the RamFlag return, between each suffix write, and before the
parent handoff. Each resumed run has no duplicate write or RamFlag invocation.

The adapter validates canonical opcodes and extensions, performs per-instruction
fetch/begin/finish timing, maps the opaque handoff to the original continuation,
and keeps all ROM/GPGX behavior in `tools/hybrid`. The five SF timing adjustments
are applied at their individual instruction boundaries, preserving resumability
rather than using a lump-sum helper duration.

## Natural shadow and authoritative proof

A 600-frame `SHADOW_NATIVE` run with targets `0x2D66,0x604BC,0x604F0` produced
5/5 comparisons, zero divergence, the frozen checkpoint/video aggregates, and
zero hardware-visible accesses. The helper comparison covered the exact natural
path through `0x611D6`, its RAM journal, register state, RamFlag call, and
continuation.

A 600-frame `NATIVE_OVERRIDE` run with the same targets produced exact frozen
checkpoint and video aggregates, `6,488,773` total instructions, zero fallback,
zero divergence, one helper invocation with seven helper instructions, and the
existing RamFlag totals (40 instructions, one boundary yield, one resumption).
Accounting is explicit:

`INTERPRETER 6,488,692 + NATIVE_TABLE_COPY 34 + NATIVE_RAMFLAG 40 + PORTABLE_INTERNAL_HELPER 7 = 6,488,773`.

The report's legacy `full_cpu_equivalence` flag remains false for
`NATIVE_OVERRIDE` by existing schema policy; serialized checkpoint identity,
video, timing/refresh, RAM effects, and continuation identity are the
authoritative gates recorded above. No third-routine, subsystem, typed-data,
`0x60BCC`, or coverage claim is made.
