# THOR M12 — AUTO63 autonomous novelty investigation

## Gate and identity

- Baseline: `a980c4320d3a6c0cec9649f06dd38cdfa4d860f0`
- Scope: one persisted AUTO62 novelty, one static dependency slice, and one
  focused BizHawk 2.11.1 QuickSave1 capture; no C++ implementation or M13,
  ROM or asset work.
- ROM: `3,145,728` bytes, CRC32 `C4728225`, SHA256
  `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
- State: QuickSave1 frame `2117`, SHA256
  `7fde47833ce70a1df34e75d95c84ed87afc8470d228af87967c6bd9dd38b3970`.

The persisted AUTO62 report contains a typo in its prose: decimal PC `10378`
is `0x288A`, not `0x2872`. AUTO63 selected the stronger persisted candidate
`PC=0xAF22` instead, with 18 observations and a near-six-byte target stride.

## Request and static explanation

The focused question was whether the register-based path can be closed as:

`A6 + 0x1A --(0xAF06 MOVEA.L)--> A0 --(0xAF20 MOVE.W)--> ROM activity`.

The exact static contract is the existing `0xAF02..0xAF14` prefix:

| PC | Contract |
|---|---|
| `0xAF02` | `MOVE.W 0x18(A6),D3` |
| `0xAF06` | `MOVEA.L 0x1A(A6),A0` |
| `0xAF20` | `MOVE.W (A0),D6` |
| `0xAF22` | `MOVE.W D6,D4` |

The full bounded static slice was written to the ignored AUTO63 evidence
directory. Its root remains `A6_INHERITED_AT_ENTRY`; no caller or earlier A6
definition was invented.

## Focused runtime result

BizHawk loaded the exact ROM and state, settled at frame `2120`, and completed
the requested 20-frame capture through frame `2140`. The harness exceeded its
30-second foreground wait but the live process wrote `result=PASS`; the raw
receipt was then validated by the focused analyzer.

The run observed:

- `AF06` to `AF0A` register transitions in ten frames, with `A6=0x00FF1CD8`
  and the post-definition `A0=0x00167DAC`.
- `AF20` consumer reads in 56 events and 18 unique ROM addresses.
- `D3=0x0200` (`512`) at the consumer, not the expected bounded `17`.
- No direct `RAM_SOURCE_READ` callback for `A6+0x1A`; therefore the source
  memory value was not promoted from register snapshots alone.
- The 18 unique consumer addresses include two `+8` gaps among otherwise
  mostly `+6` steps. They are not a closed exact six-byte stream.

The machine-readable evidence is
`build/thor-evidence/auto63-followup/auto63-af22-a/followup_report.json` and
the raw receipt is
`build/thor-evidence/auto63-followup/auto63-af22-a/capture.raw.jsonl`.

## Decision

`answered=NO`, `actually_new=YES`, causal edges `0`, static enumeration
`INCONCLUSIVE`, promotion `BLOCKED`. SOURCE_OWNED remains
`1,475,600 / 3,145,728 = 46.9080607096%`; no M12 bytes were promoted.

The unresolved frontiers are:

1. direct proof of the RAM source value at `A6+0x1A`;
2. the earlier event or caller that establishes `A6`;
3. the semantic reason for `D3=512` and the two `+8` address gaps.

This is a valid AUTO63 negative gate: static evidence and runtime consumer
evidence do not agree with the bounded record-stream contract, and the source
and structural evidence classes both fail the promotion obligation. The next
high-value investigation is an upstream A6 provenance question; until it is
answered, no `STRUCTURED_DATA_CONFIRMED` interval is safe.
