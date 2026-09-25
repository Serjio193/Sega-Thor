# M14.4 — Canonical ROM generic ASM closure

**Result: `PASS_CANONICAL_ROM_GENERIC_ASM_CLOSURE_V1`.** The analyzer reused the canonical SQLite generation and accepted M68K decoder, ran the same exact-seed closure over all 27 code-blocked UNKNOWN ranges, and replayed the complete campaign with identical output. Five CFG components closed and each exact contiguous extent roundtripped; their 260 bytes are parent-bound proposal evidence only. No proposal was applied, so the canonical map and SOURCE_OWNED remain unchanged.

Canonical ROM SHA-256: `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263` (3,145,728 bytes).

| Metric | Result |
|---|---:|
| Ranges attempted / with decoded progress | 27/27 |
| Closed CFG components | 5 |
| Unresolved components | 24 |
| Exact roundtrip components / proposed ASM extents | 5 / 5 |
| Proposed ASM bytes | 260 |
| Applied ASM ranges / bytes | 0 / 0 |
| UNKNOWN ranges / bytes before and after | 768 / 1658056 / unchanged |
| SOURCE_OWNED before / after / delta | 1487672 / 1487672 / 0 |
| Canonical partition | 2489 ranges, 3145728 bytes, 0 gaps, 0 overlaps |

## Primary fixture

`[0x061588, 0x061CD2)` remains UNKNOWN. Its 1,866-byte enclosing interval supplied 177 exact runtime seeds; recursive decoding produced 241 instructions covering 926 bytes in one connected CFG component. Closure failed with `CFG_ESCAPES_UNKNOWN_WITHOUT_EXACT_TARGET`; the contiguous exact-slice emission also returned `ASSEMBLER_ERROR`. No byte in the enclosing interval was proposed.

## Parent-bound proposals

Proposal `m14-4-proposal:8073082b1d516c4bb8ac47685ba605a5694bb9565ff5f45c1b4a2016445cd6f9` has hash `f9b2965a000c26d47203a49eef738e0f6eb013f7904f545e4a8cb4d8c0714af9` and contains 31 operations in a proposal-only clone of generation `gen-m14-2c-598c42312e80e1d8`: 5 range splits, 10 exact boundaries, 5 ASM classifications, and 11 exact decoded control-flow references. No symbols were proposed. Its parent binds map hash `d520a6da1be5d730c4352eacc75ce70fd47a23465d6899282cea7188ad416f88`, graph hash `3d0b19f11a1d8fa03a7c68a98756ef6c87f345a4aa74781166a9900bebe06d78`, and emission hash `44a2332b0b433c635e33767886ffff35985ad31131e3dc3b4dea5e6984b17d92`. The five exact extents are:

| Enclosing UNKNOWN | Exact extent | Bytes |
|---|---|---:|
| `0x001CFE..0x001D30` | `0x001CFE..0x001D30` | 50 |
| `0x03C4B6..0x03C52E` | `0x03C4B6..0x03C52E` | 120 |
| `0x062218..0x06224A` | `0x062218..0x06224A` | 50 |
| `0x062894..0x0629AC` | `0x062986..0x0629AC` | 38 |
| `0x00045A..0x00045C` | `0x00045A..0x00045C` | 2 |

Operations are proposals only. The accepted canonical emission generation was not changed, and no Stage7 ownership promotion ran. Any later ownership promotion remains subject to the existing Stage7 gates.

## Blockers and validation

All 27 ranges produced decoded instructions. Twenty-four CFG components remain open; blockers are preserved per range in the JSON receipt, including unsupported or unresolved flow, exact-target escape, assembler errors, and byte mismatches. Hypothesis claims do not seed closure.

Independent full-campaign replay: **PASS**. The proposal retains 31 exact parent-bound operations, including 11 control-flow references; no operations were applied. Focused M14.2A/B/C, M14.3, MASTER V2, materialization, knowledge map/pipeline, and M14.4 tests passed. Debug and Release full builds and full CTest each pass 213/213. A fresh detached worktree at `73c96d7018197315bd0f89abdb70585f44a7e7cc` rebuilt the CLI, passed closure/campaign/file-limit tests (3/3), and replayed the campaign with the same proposal hash. Canonical full-ROM partition and SOURCE_OWNED audit passed.

Full per-range and per-extent evidence is in [`THOR_M14_4_CANONICAL_ROM_GENERIC_ASM_CLOSURE.json`](THOR_M14_4_CANONICAL_ROM_GENERIC_ASM_CLOSURE.json).
