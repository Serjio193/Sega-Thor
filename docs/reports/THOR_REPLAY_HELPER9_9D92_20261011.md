# Replay helper #9 — `0xA7E6` scout, `0x9D92` verification (2026-10-11)

## Result

`0xA7E6` was TOP-1 in the remaining replay census. One bounded pass showed it is a 31-instruction setup/state routine with three nested BSR calls, global state updates, and multiple setup/SAT writes; its 24 natural captures exercise only the short `BTST/BNE/RTS` path. It was rejected for small-helper replacement.

After removing `0xA7E6` and already verified/frozen entries, `0x9D92` ranked next. It is promoted to `CPP_REPLACEMENT_VERIFIED_PATH_SPECIFIC = YES` for the replay-observed direct caller `0x009812 BSR.W` only. Global verification is NO; runtime replacement remains inactive. ROM SHA-256: `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

## `0xA7E6` bounded rejection

Exact decoded candidate interval `[0x00A7E6,0x00A87A)`: 148 bytes, 31 instructions, RTS at `0x00A878`. Replay captures show caller `0x00A336 BSR.W`, but the captured path immediately takes `BNE` to the RTS after reading `FF1656`; it does not sample the more stateful body. The full routine body reads/writes multiple global pointers/cursors, changes control state, emits writes through an A5 cursor, and makes three nested BSR calls. This is unsuitable as a bounded leaf-helper native replacement under the current ABI budget. It remains a subsystem/setup routine, not classified unused or globally unreachable.

## `0x9D92` boundary and callers

Exact range `[0x009D92,0x009DB4)`, 34 bytes, 12 instructions, one RTS at `0x009DB2`. It has no nested calls, indirect control, or internal stack scratch. A ROM-wide aligned BSR.W target scan found direct call instructions at `0x009226` and `0x009812`; both bytes were independently decoded as BSR.W to `0x9D92`. Indirect/computed entry possibilities remain unresolved. Only `0x009812` is in scope because it is the replay-observed caller. Its return PC `0x009816` is the BSR.W to `0x009D4E`; the latter consumes D0.low. Caller `0x009226` is unsupported.

## ABI and native contract

Inputs: D0.low coordinate, D2.low axis value, word `[A6+0x46]`, word `[A0+6]`, and byte `[A6+0x41]` for the conditional OR. It computes `sum = uint16(D2.low + word[A6+0x46])`, `limit_base = uint16(word[A0+6] * 8 - 1)`, and `maximum = uint16(limit_base - sum)`. If unsigned D0.low exceeds maximum, D0.low is clamped. D3.low receives maximum and D7.low receives sum. When a clamp yields zero, `[A6+0x41] |= 0x48`. The native model also records the X borrow from the final subtraction and preserves high register halves. Other D/A registers and A7 are preserved.

The caller passes D0 to the next routine. D3 and D7 are overwritten in `0x9D4E` before caller-path use. The next instruction after return is a BSR.W, not a condition consumer; `0x9D4E` updates condition state internally. Thus NZVC is not caller-observable on this path; X is also modeled conservatively. No natural call writes RAM.

## Corpus and differential

The existing deterministic replay contains 24 calls from `0x009812`, all on the `BLS` in-range path, with 12 distinct `(D0.low,D2.low)` pairs, three distinct D0 inputs/outputs, and 12 distinct D3/D7 outputs. Both bounded data reads were captured: `[A0+6]=64` and `[A6+0x46]=6`; no routine RAM writes occurred. The typed native model matched all 24 replay vectors.

Four ABI-local synthetic cases ran against canonical ROM in Debug and Release, all matched 4/4. They cover in-range, nonzero clamp, zero clamp with flag OR, and 16-bit wrapped subtraction/X. Focused Debug/Release builds/tests and GNU/Linux native model build/test passed. Linux ROM-oracle execution was not run because the configured HOOK_CPU runtime core is Windows-only.

## Status and authorities

- `CPP_REPLACEMENT_CANDIDATE = YES`
- `CPP_REPLACEMENT_VERIFIED_PATH_SPECIFIC = YES`
- `PATH_SCOPE = direct BSR.W caller 0x009812`
- `CPP_REPLACEMENT_VERIFIED_GLOBAL = NO`
- `RUNTIME_REPLACEMENT_ACTIVE = NO`
- new bytes classified = 0; authority deltas = 0

Totals remain ROM 3,145,728 B; runtime 440,425 B; structural 1,266,667 B; SOURCE_OWNED 1,487,672 B. Machine receipt: `THOR_REPLAY_HELPER9_9D92_ACCEPTANCE_20261011.json`.

Next candidate after this closure: `0x0099D6`; it has many distinct natural read addresses, so first determine whether the pointer/extent contract is bounded before any native model.
