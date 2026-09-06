# M11.16 — Targeted Dynamic Code Confirmation

Status: `TARGETED_DYNAMIC_REACHABILITY_LIMITED`.

This pass audits five critical ranges selected before any emulator run. The
selection is recorded in the ignored local artifact
`build/m11-16/targeted_dynamic/selection.json`; it is capped at eight targets.
No new natural scenario was added and no emulator run was started: the
retained natural reports were inspected first, and the available BizHawk
executable is not part of this checkout.

| Range | Size | Instructions | Critical reason | Expected natural state | Candidate callers | Candidate callees |
| --- | ---: | ---: | --- | --- | --- | --- |
| `0x3820..0x3B3E` | 798 | 306 | graphics decompressor | title/start plus asset use | 20 documented Ghidra callers | none |
| `0x62CC..0x62E4` | 24 | 6 | player/event reset leaf | post-transition movement or interaction | `0x5670`, `0x7AC2` | none |
| `0x9BF2..0x9C40` | 78 | 32 | byte-grid aggregate | natural room/grid transition | `0x8F22`, `0x9BC2`, `0x9D00` | none |
| `0xA8DA..0xA8F0` | 22 | 10 | bounded translation leaf | byte-grid/entity update | `0xA7E6` | none |
| `0xD3B2..0xD406` | 84 | 18 | event producer reader | natural event/entity dispatch | none | `0x3820` |

Every selected entry has an exact audited half-open range and a complete static
instruction set. No selected range has an accepted natural hit in the retained
JSON artifacts, so none is upgraded. The old M11.8 prose claim of 13 hits for
`0x3820` is retained as context only: its run artifact is absent and therefore
cannot supply a provenance hash or `DYNAMIC_NATURAL` evidence.

The selection report keeps trusted caller/callee lists empty. The displayed
addresses are candidate Ghidra caller/callee links; they are not promoted to
trusted edges by selection alone.

The existing positive control `0x6121A..0x61232` was checked separately. Two
identical natural runs (`natural-final-a.json` and `natural-final-b.json`) use
the canonical ROM hash, scenario `natural_idle_to_6121a_v1`, backend
`bizhawk-lua-natural-input`, and record two target hits at frame 113. Their
artifact SHA-256 values are respectively
`adacafece835888c79e338df46b07dfc2245bdea2c47fc0d0fff97e07d862172`.
The range linkage is exact and the evidence class is `DYNAMIC_NATURAL`; this
control remains `CODE_EXECUTED`.

The per-target result is:

| Target | Result | Reachability classification | Evidence boundary |
| --- | --- | --- | --- |
| `0x3820` | not reached | missing retained runtime artifact | documented M11.8 hit cannot be accepted |
| `0x62CC` | not reached | `CALLER_NOT_REACHED` | neutral and M11.8 owners remain outside the path |
| `0x9BF2` | not reached | `SCENARIO_COVERAGE` | no target hook in retained natural reports |
| `0xA8DA` | not reached | `SCENARIO_COVERAGE` | prior natural capture had zero hits |
| `0xD3B2` | not reached | `CONTROL_FLOW_GAP` | producer path was not observed |

Baseline and result trust counts are unchanged: 197
`ASM_ROUNDTRIP_EXACT` ranges (12,520 bytes), 5 `CODE_STATIC_SUPPORTED`
ranges (1,006 bytes), 1 `CODE_EXECUTED` range (24 bytes), and 0
`BEHAVIOR_VERIFIED`. Forced evidence would not promote a range, and updating a
callee does not propagate trust to its caller. New-run efficiency is not
applicable because no new emulator minute was spent; this was evidence reuse.

The full-ROM exactness oracle remains unchanged: 3,145,728 bytes, CRC32
`C4728225`, SHA-1
`2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

The single next recommendation is **D — run one separately authorized bounded
timing/hold-input sweep around the M11.8 startup transition**, preserving the
same natural-only evidence and exact target hooks.
