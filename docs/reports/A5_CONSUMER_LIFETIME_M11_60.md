# M11.60 — Bounded A5 consumer/lifetime closure at `0x060182`

Status: **COMPLETE — `BOUNDED_A5_CONSUMER_LIFETIME_PROVEN`**

Baseline: `37ef10694bdab1a52b042f79bc8e1f480f3f25f2` (M11.59 authoritative
result `RAW_DATA_ALIASING_BOUNDARY_PROVEN_TYPED_DATA_BLOCKED`).

## Scope and result

This milestone followed only the natural region rooted at
`0x060182 LEA.L $FF001A,A5`. No typed data, production/core abstraction,
`0x60BCC` analysis, `0x061258` clear expansion, gameplay interpretation or
new native-routine search was performed.

The selected generation is a parent-owned lifetime. Its exact materialization,
natural consumers and endpoint are proven, but the memory effects are not
promoted as a standalone raw transaction because calls crossed while A5 is
live still have incomplete whole-callee preservation/effect proofs.

- lifetime endpoint: `A5_LIFETIME_MERGES_WITH_PARENT`
- transaction gate: `BOUNDED_A5_TRANSACTION_BLOCKED_PARENT_LIFETIME`
- relation to the M11.58 cluster: `ARE_ALTERNATE_PRODUCER_CONSUMER_PATHS`
- typed-data gate: `TYPED_DATA_BLOCKED_OVERLAPPING_ACCESS`
- milestone result: `BOUNDED_A5_CONSUMER_LIFETIME_PROVEN`

## Phase 1 and authoritative identity

The M11.59 baseline was reproduced before and after the observer changes. The
authoritative 600-frame identity remains:

- checkpoint SHA-256:
  `251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d`
- video SHA-256:
  `5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`
- accounting: `6,488,773` =
  `6,488,692` interpreter + `34` TableCopy + `40` RamFlag + `7` ParentSuffix
- fallback: `0`; divergence: `0`; shadow comparisons: `5/5`

The observer is opt-in and EMULATED-only, so it cannot alter production/core
behavior.

## Exact CFG and enclosing-region classification

The bounded decoder slices were generated from the canonical ROM:

- `0x060150..0x060274` contains the predecessor and the selected
  `0x060182` block.
- predecessor: `0x060170 -> 0x060182`
- selected block: `0x060182 LEA FF001A,A5`,
  `0x060188 TST.B FF0014`, `0x06018E BCC.W 0x0601D4`
- fallthrough block `0x060192..0x0601CE` contains the only static
  `5(A5)`/`7(A5)` accesses.
- continuation `0x0601D4..0x060284` contains the direct calls and ends at
  `0x06027E MOVEM.L (A7)+,D0-D7/A0-A6`, followed by `0x060284 RTS`.

`TST.B` clears C on 68k; therefore `BCC` at `0x06018E` is always taken.
The `0x060192..0x0601CE` arm is statically dead on the natural path:
`0x06019E CMP.B 7(A5),D0` and `0x0601A6 MOVE.B D0,7(A5)` have natural count
zero. The enclosing region is classified `PARENT_OWNED_REGION`, not a
standalone routine.

## G0 provenance ledger

G0 is the exact value written by `0x060182` (`A5 = 0x00FF001A`).

| PC | instruction/effect | G0 label | offset/width | natural observation |
|---|---|---|---|---|
| `0x060182` | `LEA.L FF001A,A5` | `DIRECT_G0_USE` | address-only | 486 starts |
| `0x06019E` | `CMP.B 7(A5),D0` | `DIRECT_G0_USE` | +7 / byte read | dead arm, 0 |
| `0x0601A6` | `MOVE.B D0,7(A5)` | `DIRECT_G0_USE` | +7 / byte write | dead arm, 0 |
| `0x06193C` | `BTST.B #0,0(A5)` | `DIRECT_G0_USE` | +0 / byte read | 10 |
| `0x061946` | `MOVE.B D7,4(A5)` | `DIRECT_G0_USE` | +4 / byte write | 1,361 |
| `0x061998` | `MOVE.B 4(A5),(A4)+` | `DIRECT_G0_USE` | +4 / byte read | not reached in the 600-frame trace |
| `0x06027E` | `MOVEM.L (A7)+,...,A5` | `G0_KILLED`/parent restore | register restore | 486 kills |

No natural `A5 -> An/Dn` copy, pre/post-increment, RAM spill or reload of G0
was observed in this region. The static callee slices contain no direct A5
overwrite. Nested callee effects remain a preservation boundary, so no
`PROVEN` alias is invented.

The dead +7 arm explains the previously known 5(A5)/7(A5) evidence without
assigning those bytes a field identity. The live natural consumers are the
+0 and +4 accesses above.

## Calls and preservation

Every generation crosses the parent calls at `0x0601E2`, six sites targeting
`0x061934`, four sites targeting `0x0623AC`, and `0x06027A` targeting
`0x060286`: 5,832 call entries and 5,832 callee entries in the repeated
trace. The direct callee slices show no A5 write; the trace records
`A5=FF001A` at each selected generation and at the `0x06027E` restore.

The observer's simple return pairing records 4,860 returns because nested
returns do not all map to the selected direct-call stack. This is retained as
an observation limitation, not upgraded to exact call preservation. The
dominant remaining blocker is parent/callee effect closure, so the transaction
gate stays fail-closed.

## Deterministic runtime trace

The developer-only observer writes
`oasis.hybrid.a5-lifetime.v1` JSONL. Three 600-frame traces (`h`, `i`, `j`)
are byte-identical:

`C5013D7C0593B52CF7F22A2C87677B3941F276087E166062162A05FBCDDC2C07`

Each trace reports 486 G0 generations and 486 exact kills. It observes one
natural CFG path (the `0x06018E` taken edge), one termination mode
(`0x06027E` restore), 1,371 selected memory events (10 reads at +0 and 1,361
writes at +4), and no natural execution of the dead +7 arm.

## Typed gate and remaining unknowns

M11.60 reduces the uncertainty around the selected generation but does not
remove the historical objections:

- the parent saves/restores A5 and owns the enclosing frame and RTS;
- M11.59's `0x061258` stack escape and broad post-increment lifetime remain
  outside this milestone;
- fixed `FF0010..FF0014` bytes retain external writers;
- the M11.58 +5..+7 consumers and the selected +0/+4 consumers share raw
  storage but are alternate producer/consumer paths;
- nested callee preservation and hidden effects are not fully closed;
- no hardware-free whole-transaction proof exists.

Therefore no typed structure, subsystem or new native routine is justified.

Astra was unavailable locally; no advisory claim is recorded.

## Regression and governance

`A5LifetimeObserver` and its runtime shim are developer-only
`src/tools/hybrid` code. `tests/hybrid_a5_lifetime_test.cpp` locks the
`0x060182` generation, branch, exact +0/+4 consumers, call handoff and
`0x06027E` restore. ROM/GPGX types remain outside `oasis_core`.

Validation completed for this change: full Debug and Release CTest pass 71/71,
including the A5 observer regression. The configured UCRT build fails while
compiling the pre-existing `raw_data_provenance_test.cpp` before emitting a
source diagnostic (exit 1); its prior 69/69 suite is not counted for this new
test and is classified `LOCAL_TOOLCHAIN_ENVIRONMENT`.
Post-change authoritative native/shadow 600-frame runs retain the exact
checkpoint/video identity; native reports zero fallback/divergence and shadow
reports 5/5 comparisons with zero divergence.

## M11.61 proposal

Follow only the dominant blocker exposed here: close preservation/effects for
one selected direct callee while G0 is live (start with `0x062AE0`), without
promoting the lifetime or widening into another routine.
