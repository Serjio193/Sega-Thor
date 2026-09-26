# M12 map-driven executed ASM closure 2F

**Result:** `PASS_MAP_DRIVEN_EXECUTED_ASM_CLOSURE_V1`

**Base:** `311338b823f98f17065882387361ed2c636e7f82`

**Canonical ROM:** Beyond Oasis (USA), 3,145,728 bytes, SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

The accepted 2G map reconciled before analysis: 360 executed M68K instruction
objects, 225 executed-but-not-fully-owned objects, `SOURCE_OWNED` 1,475,600,
and zero conflicts. Candidate grouping used exact instruction ranges and
observed `EXECUTED_NEXT` edges. The 25 disjoint candidate islands cover 884
bytes, all unowned at baseline. Per-island ranges, instruction/observed counts,
ownership bytes, stable IDs, and blocker status are included in the compact
JSON alongside the aggregate counts.

Only one island passed every gate:

| ROM interval | Bytes | Instructions | Executed instructions | Observed adjacent edges | Result |
| --- | ---: | ---: | ---: | ---: | --- |
| `[0x002AA4,0x002ACE)` | 42 | 14 | 14 | 13 | `PASS_CLOSED_ASM_RANGE` |

Its generated ASM round-tripped byte-exactly through vasm. The independent
auditor re-decoded the exact boundaries and bytes, verified the complete ROM
rebuild against the canonical SHA-256, and confirmed prior ownership and the
full disjoint 3,145,728-byte emission partition. The other 24 islands remain
blocked by `STOP_CONTROL_FLOW_ESCAPE_UNRESOLVED`; no indirect or unvisited
target was promoted or labeled as an observed runtime event.

| Metric | Before | After |
| --- | ---: | ---: |
| `SOURCE_OWNED` bytes | 1,475,600 | 1,475,642 |
| Executed instruction objects | 360 | 360 |
| Executed, not fully owned | 225 | 211 |
| Canonical objects | 3,824 | 3,826 |
| Claims | 6,289 | 6,293 |
| Relations | 400 | 400 |
| Evidence references | 9,476 | 9,480 |
| Conflicts | 0 | 0 |

Emission stayed a complete partition of the ROM:

| Emission | Before | After | Delta |
| --- | ---: | ---: | ---: |
| ASM | 56,134 | 56,176 | +42 |
| DATA | 233,676 | 233,676 | 0 |
| ASSET | 1,085,110 | 1,085,110 | 0 |
| INCBIN | 1,770,808 | 1,770,766 | -42 |

The refreshed map is `858d609ce6be75841251e7adafd4bd3460d345d2e719ec754d1833d4211b210c`;
its structure hash is `70cdd9ef1fc134bc2318b17590cc4df1f3cbda2536eb4d5f88cf13dce58e499a`,
evidence hash `0508c8404172d6ee892553abcebb5b7fcaa76392b06b093324daf8dd9c043e94`,
and emission hash `bca395e9ae222c6c35acdf17e09f3b1562284a29a4da99d6d0349fba07f56e81`.
The Archivist audit passed and repeated import returned `PASS_IDEMPOTENT_NOOP`.
Two independent producer runs produced identical materialized manifest,
promotion report, and audit hashes; these hashes are recorded in the JSON.

No new BizHawk campaign or CPU runtime instrumentation was used. Accepted
runtime evidence was retained unchanged: 360 executed instruction objects,
369 `EXECUTED_NEXT` relations, 31 `OBSERVED_NEXT_PC` relations, and 1,921
runtime evidence references before and after, with zero lost or changed facts.
Runtime observations were not inferred from static flow. Production AUTO67,
Worker 1B scaling, predecessor logic, and Worker/FLOW runtime semantics are
unchanged. The SQLite maps, segment logs, and raw audit data remain local in ignored
`build/thor-evidence/`; only source, tests, generated ASM, documentation, and
this compact receipt are publication artifacts.

**Validation:** Debug and Release builds passed; Debug, Release, and GNU/Linux
CTest each passed 209/209. Focused 2F A–T coverage, 2G pipeline tests 18/18,
source-file limit, Python compilation, and `git diff --check` passed. The GNU
CTest invocation supplied the worktree's explicit `GIT_DIR` and `GIT_WORK_TREE`
because its `.git` file contains a Windows absolute path.
