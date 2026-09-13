# THOR M12 AUTO65 — Multi-chain autonomous RE campaign

Date: 2026-09-13. Baseline: `49875f51c2f3e818e7fdeba56fb430d07f3efaf2`.
Status: PASS / Level 4. Level 5 promotion is not required.
Publication implementation SHA: `1621546dc946a19e7640449c4fba34d20f639a19`.

## 1. Baseline/final SHA

Baseline is the accepted AUTO64.1 commit above. The implementation publication
commit is `1621546dc946a19e7640449c4fba34d20f639a19`.

## 2. New gameplay scenario and why materially different

AUTO64's graph had no `OPEN`, `ACTIVE`, `WAITING_EVIDENCE` or `IN_PROGRESS`
node. The scheduler therefore selected the repository scenario
`m11_8_natural_reachability_v1`, not QuickSave1: hardware reset, 1,800 frames,
25 normal one-frame controller events covering Start, movement, attack/use and
room/interactions. Its scenario file identity is persisted in the ignored
campaign state at `build/thor-evidence/auto65/campaign.json`.

## 3. Discovery instrumentation

The existing developer-only `re_bizhawk_natural_reach.lua` was used with the
canonical ROM and the scheduler-selected scenario. It collected bounded frame
samples, exact target hooks, caller discrimination and RAM writes. No guest
memory/register/state was written. The sealed report is
`build/thor-evidence/auto65/broad.report.json`.

## 4. Runtime event counts

The report contains 39 source observations: 32 writes at `PC=0x0000026C`, five
caller hits (`0x000611EE` twice and `0x00060B8C` three times), and two target
rows with nonzero activity (`0x00003820`: 13, `0x00060004`: 5).

## 5. Candidate chain derivation

The normalizer derived 36 structural candidates from those rows: 32 writer
chains, two unique caller chains and two target-activity chains. Repeated hits
were retained as instances but did not inflate structural candidates.

## 6. Chain fingerprint design

The index fingerprint includes scenario identity, start state, normalized watch
set, proof obligations and evidence lineage. Structural equivalence compares
typed node and edge lists directly; equal hashes are never treated as proof.

## 7. Known-chain rejection

The first pass had no equivalent persisted chain. On replay, all 36 candidates
were `KNOWN_NEW_INSTANCE`; all 36 investigations were cancelled before new
static/runtime work.

## 8. Known-prefix reuse

The first writer chain persisted `RESET:hardware_reset → PC:0x0000026C`.
The remaining 31 writer destinations reused that prefix and were scheduled as
new tails (`NEW_BRANCH`). The two call-site observations likewise shared their
reset prefix.

## 9. Novel branches

First-pass novelty was: 1 `NEW_CHAIN`, 32 `NEW_BRANCH`, 1 `NEW_WRITER`, 1
`NEW_CONSUMER`, and 1 `NEW_ROM_ACTIVITY`. No conflict was observed.

## 10. Clustering/merging

There were four shared-prefix clusters sized 32, 2, 1 and 1. The campaign
recorded 32 shared-dependency merges and one writer static proof served every
writer child.

## 11. Investigation DAG

Each persisted investigation records parent/children slots, shared dependency,
known prefix, novelty-start edge, capture lineage, static/runtime requests and
terminal state. Historical nodes are retained in the ignored JSON state; no
node is deleted on replay.

## 12. Peak active/batched investigations

Peak active/batched count was 36. This is the exact natural candidate count,
not a fabricated 100-chain target. The implementation supports larger pending
sets without attaching one expensive callback set per chain.

## 13. Shared capture reuse

One broad discovery capture served all 36 investigations; reuse count is 35.
No equivalent focused capture was run because its required observations were
already present in the sealed broad report and the anti-repeat fingerprint
would reject it.

## 14. Static investigations

Five static requests were issued before any focused-runtime decision. The
shared writer request derived its bounded window from the observed writer PC
and was accepted by `oasis_re_assemble_range`; the two caller clusters reused
static call bytes from the report, while the two activity clusters remained
explicitly unresolved.

## 15. Focused runtime investigations

Zero. The broad report already contained the relevant bounded observations.
Launching an equivalent second runtime capture would violate AUTO65
anti-repeat; the two missing activity provenance chains remain
`BOUNDED_UNRESOLVED`.

## 16. Closed chains

34 investigations reached `PROVEN`: the reset-writer and observed caller
chains closed against bounded static/runtime facts. Two target-activity chains
closed only at the explicit `BOUNDED_UNRESOLVED` frontier. No last-writer-only
or equal-value inference was promoted.

## 17. Second-pass known-chain replay proof

PASS. First pass was `NEW` and persisted 36 investigations. The same normalized
report on the second pass yielded 36 `KNOWN_NEW_INSTANCE` classifications,
zero new investigations and zero runtime captures.

## 18. Structural enumeration

The writer static slice enumerated the observed contiguous reset-write family
from `0x00FFF62A` through `0x00FFF5EC` at the observed two-byte cadence. The
bounded decoder accepted the derived `0x0000026A..0x00000270` window. No ROM
resource extent was inferred from this RAM initialization activity.

## 19. Ownership promotions

No promotion candidate was eligible. This is intentional: runtime/evidence
coverage does not directly increase `SOURCE_OWNED`.

## 20. SOURCE_OWNED before/after

Before: `1,475,600 / 3,145,728 = 46.9080607096%`.

After: `1,475,600 / 3,145,728 = 46.9080607096%` (delta 0).

## 21. Performance metrics

| Metric | Result |
|---|---:|
| discovery runs | 1 |
| focused runtime runs | 0 |
| static requests | 5 |
| runtime events | 39 |
| candidate chains | 36 |
| candidate branches | 32 |
| known complete rejected | 36 on replay |
| known instances | 36 on replay |
| investigations created | 36 |
| investigations completed | 34 |
| investigations blocked | 2 |
| peak active/batched | 36 |
| shared dependency merges | 32 |
| shared capture reuse | 35 |
| causal edges added | 68 |
| structures enumerated | 33 |
| bytes promoted | 0 |

## 22. Full-layout byte-exact validation

Two fresh full-layout runs completed with `re_full_split_run.py` using vasm.
Both had 80 manifest entries, zero gaps, zero overlaps, ASM size `2,758`
bytes, and `full_match=true`. Both rebuilt the canonical identity: size
`3,145,728`, CRC32 `C4728225`, SHA256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
The generated `full_layout.asm` SHA256 was
`45E67BC19FD9AAFFFD7DEE9716DA5A8C218B97D185EE1D16F7C7D0A33E337C51` in
both clean output directories.

## 23. Windows/GNU validation

The AUTO65 Python regression is registered in CTest. Windows Visual Studio
Debug passed build plus 184/184 CTest; Windows MinGW Release passed build plus
184/184 CTest; WSL GNU Release passed build/link plus 184/184 CTest. The
standalone Python regression also passed 4/4 tests. `git diff --check` and the
source file-limit gate passed; the inventory reports 620 governed files at or
below 500 lines.

## 24. Remaining unresolved frontiers

The two runtime target-activity chains have no causal source in the bounded
static graph. AUTO64's A6/D3/+8 frontiers remain unchanged. No new upstream
root, input edge, ROM ownership or semantic resource interpretation is claimed.

## 25. Stop reason

Stop condition C: all current novel investigations reached `PROVEN` or an
explicit `BOUNDED_UNRESOLVED` terminal frontier, and further progress requires
a materially different gameplay scenario. Level 4 is achieved; Level 5 is not
required and no promotion was attempted.

## Gate status

All local AUTO64.1/AUTO65 gates passed: full-layout generation and byte
equality, Windows Debug/Release CTest, GNU build/link/CTest,
coverage/scheduler/anti-repeat tests, `git diff --check`, file limits, and
repository hygiene. Publication SHA and remote CI result are filled after
commit/push.
