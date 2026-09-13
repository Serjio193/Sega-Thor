# THOR M12 AUTO66 — Autonomous multi-scenario coverage campaign

Date: 2026-09-13. Baseline: `1942dfdbd1f0cdbcf5226a464c3e41abd16ebb1a`.
Status: PASS for the bounded multi-scenario requirement; no promotion was
eligible.

## Scenario accounting

`SCENARIOS_ATTEMPTED=2`: the repository contains two deterministic scenario
files. `SCENARIOS_REJECTED_AS_EQUIVALENT=1`: the already consumed AUTO65
`m11_8_natural_reachability_v1` fingerprint was rejected before a new emulator
launch. `SCENARIOS_EXECUTED=2` evidence scenarios are represented: AUTO65's
accepted scenario A and AUTO66's new bounded scenario B. AUTO66 performed one
new emulator discovery launch; the sealed B report was replayed once while
persisting the corrected campaign receipt, without another emulator launch.

`AUTONOMOUS_SCENARIO_TRANSITIONS=1`:

```text
scenario A fixed point
  -> NEED_NEW_SCENARIO
  -> equivalent A rejected
  -> scheduler selects scenario B
  -> bounded BizHawk discovery
  -> candidate derivation and investigations
```

The scheduler used coverage frontier count, target count, input count and
bounded duration as repository-visible ranking inputs. No ROM address or chain
was supplied as a campaign target.

## Scenario A — AUTO65 accepted capture

`m11_8_natural_reachability_v1`: 39 raw observations, 36 candidate chains,
known `0`, novelty `NEW_CHAIN=1`, `NEW_BRANCH=32`, `NEW_WRITER=1`,
`NEW_CONSUMER=1`, `NEW_ROM_ACTIVITY=1`, clustered investigations `[32,2,1,1]`,
34 chains closed, 2 explicitly bounded unresolved, promotion `0`.

This scenario reached its persisted fixed point before AUTO66 planning. Its
mandatory AUTO65 replay had 36 `KNOWN_NEW_INSTANCE` rejections and no new
investigations.

## Scenario B — scheduler-selected idle reachability

`natural_idle_to_6121a_v1`: hardware reset, 300-frame bound, no manual address
selection. The sealed report contains 35 raw observations: 32 reset-writer
rows, two `0x000611EE` caller hits, and two executions of new target activity
at `0x0006121A`.

The 34 derived candidates were classified as 33 `KNOWN_NEW_INSTANCE` rows and
one `NEW_BRANCH`: the new `ROM_ACTIVITY(0x0006121A)` tail reuses the known reset
root prefix. The campaign formed clusters `[32,1,1]`, cancelled the 33 known
investigations, created one new investigation, performed static-before-runtime
accounting, and ended that activity tail at explicit
`BOUNDED_UNRESOLVED`. No focused runtime was needed.

## Campaign totals

| Metric | Result |
|---|---:|
| runtime events | 74 |
| candidate chains | 70 |
| known rejected | 69 (36 AUTO65 replay + 33 scenario B) |
| new chains | 1 |
| new branches | 33 |
| new edges | 36 |
| investigations created | 37 |
| peak active | 37 |
| investigations completed | 34 |
| blocked | 3 |
| exhausted | 0 |
| shared dependency reuse | 63 |
| shared capture reuse | 68 |
| focused runtime runs | 0 |
| static requests | 6 |
| chains closed | 34 |
| structures enumerated | 33 |
| promotion candidates | 0 |
| bytes promoted | 0 |

`KNOWN-CHAIN DEDUP=PASS`: AUTO65 replay and scenario B rejected known
structures before new investigation. `MULTI-CHAIN=PASS`: 70 bounded candidate
chains and 37 persisted investigations were handled by the chain engine, with
shared-prefix reuse and no serial one-address fallback.

`MULTI-SCENARIO=PASS`: the persisted planner receipt proves the automatic
fixed-point-to-new-scenario transition. `MANUAL ADDRESS SELECTION=NO` and
`SERIAL ONE-ADDRESS FALLBACK=NO`.

## Ownership and promotion

`SOURCE_OWNED BEFORE=1,475,600 / 3,145,728 = 46.9080607096%`.

`SOURCE_OWNED AFTER=1,475,600 / 3,145,728 = 46.9080607096%`.

`DELTA=0`. Evidence did not satisfy an existing M12 ownership contract, so no
bytes were promoted and no ROM/assets were added.

## Validation

`ROM BYTE-EXACT=PASS`: canonical ROM remains size `3,145,728`, CRC32
`C4728225`, SHA256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

`WINDOWS=PASS`: Visual Studio Debug and MinGW Release each built successfully
and passed 185/185 CTest. `GNU=PASS`: WSL Release built and linked successfully
and passed 185/185 CTest. The standalone AUTO65 and AUTO66 Python regressions
passed 4/4 and 3/3. The fresh AUTO66 full-layout run produced 80 manifest
entries, zero gaps, zero overlaps, ASM size `2,758` bytes, exact ROM size
`3,145,728`, CRC32 `C4728225`, canonical SHA256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`, and
`full_layout.asm` SHA256
`45E67BC19FD9AAFFFD7DEE9716DA5A8C218B97D185EE1D16F7C7D0A33E337C51`.
`git diff --check` and the 500-line source policy also passed. Remote CI is
checked after publication.

## Stop reason

The useful deterministic scenario pool is exhausted: the only remaining pool
member is the already captured equivalent scenario, while scenario B supplied
only one new bounded tail and 33 known instances. Further progress requires a
new savestate or genuinely different gameplay state; no artificial scenario
or 100-chain target was fabricated.
