# Replay helper #8 — `0x9DB4` (2026-10-11)

## Result

Promoted `0x9DB4` to `CPP_REPLACEMENT_VERIFIED_PATH_SPECIFIC` for the direct `0x0092CC BSR.W` caller only. Global verification remains NO and runtime replacement remains inactive. The ROM identity is SHA-256 `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

## Candidate ranking

The replay census was reranked using remaining candidates only. `0x9DB4` ranked first (59.4): 24 natural full-call captures, 24 distinct entry states and D3 outputs, one direct caller, one read address, one observed path, exact boundary, no stack scratch, nested calls or indirect control. Next candidates are `0x00A7E6` (47.0; exact boundary unknown), `0x009D92` (37.3), `0x0099D6` (34.8; 26 distinct read addresses), `0x00F788` (34.1), `0x009D70` (31.2), `0x00D344` (30.3), and `0x00B3B2` (29.8). Already verified and frozen targets are excluded from the current remaining-candidate top ten.

## Boundary and caller

Exact range `[0x009DB4,0x009DD6)`, 34 bytes and 11 instructions. RTS exits are `0x009DCA` and `0x009DD4`. There are no nested calls, indirect control transfers, or internal stack-save writes. The accepted path is `0x0092CC BSR.W → 0x009DB4`, return `0x0092D0`; other entry/caller paths remain unsupported.

The routine copies D2.w to D3.w, subtracts word `[A6+0x46]`, then uses BMI/BLS/BEQ to clamp D0.w and sometimes OR `0x48` into byte `[A6+0x41]`. The native typed model accepts only these explicit scalar inputs and the two bounded memory bytes/word. It preserves upper register halves and models the X bit from the subtraction borrow.

## Natural evidence and liveness

The replay corpus contains 24 natural calls from the selected caller, one observed control path, 24 distinct `(D0.low,D2.low)` pairs, five distinct D0 outputs, and 24 distinct D3 outputs. Each call reads one word at `[A6+0x46]` (value 10 in this corpus); none writes RAM. All calls take the BMI-not-taken/BLS-taken path. The natural corpus is native-model checked 24/24; it does not claim that every branch was naturally observed.

At return, D0 is passed onward into a direct callee. D3 is overwritten before its first caller-path read. The callee's first ORI overwrites N/Z/V/C before conditional use; X is modeled conservatively because that instruction preserves it. A7 is restored by RTS. No internal scratch area exists. Conditional RAM effects are compared in the canonical-ROM oracle.

## Differential and branch coverage

Five ABI-local synthetic cases were run against canonical ROM in both Debug and Release, all matching 5/5. They cover the normal in-range path, nonzero clamp, zero clamp/flag-byte OR, signed-negative difference with borrow, and signed-negative result without unsigned borrow. The 24 natural captures separately match the native model 24/24. The combined runner also retained the prior three `0x9D4E` cases; those are not counted as `0x9DB4` evidence.

Focused Debug and Release builds/tests passed. The GNU/Linux native model build/test passed. Linux ROM-oracle execution was not run because the configured HOOK_CPU runtime core is Windows-only.

## Status and authority

- `CPP_REPLACEMENT_CANDIDATE = YES`
- `CPP_REPLACEMENT_VERIFIED_PATH_SPECIFIC = YES`
- `PATH_SCOPE = direct caller 0x0092CC`
- `CPP_REPLACEMENT_VERIFIED_GLOBAL = NO`
- `RUNTIME_REPLACEMENT_ACTIVE = NO`
- New bytes classified: 0; evidence-authority deltas: 0.

Totals remain ROM 3,145,728 B; runtime 440,425 B; structural 1,266,667 B; SOURCE_OWNED 1,487,672 B. Machine acceptance receipt: `THOR_REPLAY_HELPER8_9DB4_ACCEPTANCE_20261011.json`. Updated replay ranking: `THOR_REPLAY_HELPER_CANDIDATE_INDEX_20261011.json`.

Next priority: `0x00A7E6`, after exact boundary and caller-liveness checks.
