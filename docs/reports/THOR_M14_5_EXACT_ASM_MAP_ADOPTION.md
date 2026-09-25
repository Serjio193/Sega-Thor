# M14.5 — Exact ASM map adoption and Stage7 eligibility

**Result:** `PASS_EXACT_ASM_MAP_ADOPTION_V1`

The tracked M14.4 proposal reproduced with identity `m14-4-proposal:8073082b1d516c4bb8ac47685ba605a5694bb9565ff5f45c1b4a2016445cd6f9` and hash `f9b2965a000c26d47203a49eef738e0f6eb013f7904f545e4a8cb4d8c0714af9`. Independent replay validated five exact extents totaling 260 bytes. The proposal contains 31 operations: 25 were accepted and 6 references were independently rejected because their source/target instruction objects were not established by the canonical graph contract.

The five valid components were classified as `ASM_ROUNDTRIP_EXACT` in deterministic child generations. Split lineage and existing evidence references remain resolvable. Each global object view exposes the canonical classification, M14.4 CFG/roundtrip proof, runtime occurrences, CFG relations, and adoption derivation. No component met the existing Stage7 promotion contract because static caller support and whole-ROM reconstruction proof are absent. No promotion was attempted.

| Measure | Before | After | Delta |
|---|---:|---:|---:|
| Map ranges | 2489 | 2490 | +1 |
| UNKNOWN ranges | 768 | 764 | -4 |
| UNKNOWN bytes | 1658056 | 1657796 | -260 |
| SOURCE_OWNED bytes | 1487672 | 1487672 | 0 |

The canonical ROM partition remains 3,145,728 bytes with zero gaps and overlaps. `[0x061588,0x061CD2)` remains UNKNOWN. Modified parent state is rejected; exact-parent replay produces identical range identities, accepted operation set, references, lineage, map/emission hashes, and generation hash.

Validation: M14.2A (3/3), M14.2B, M14.2C, M14.3, M14.4, MASTER V2 (3/3), canonical materialization (3/3), and focused M14.5 checks pass. Full Debug CTest: 214/214; full Release CTest: 214/214. Fresh detached worktree at `3f3f14ad4dbd0dc5bcf63e4fec9b866f36d21fcd` configured from scratch, rebuilt `oasis_re_m14_4_cfg`, and passed M14.4/M14.5/source-limit smoke (3/3).
