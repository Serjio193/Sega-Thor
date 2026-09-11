# M12-CARVER-2 — Graph-guided evidence expansion checkpoint

## Scope and baseline

This checkpoint continues from the pushed AUTO60/M12-CARVER-1 baseline at
`51203556e7fb9797ec7b29170d081c44d61be30f`. The exact baseline CI run was
`34602974350`. The canonical ROM remains external to the repository and is
identified by size `3,145,728`, CRC32 `C4728225`, SHA-1
`2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

The baseline contains `1,427,873` SOURCE_OWNED bytes, or
`45.39086023966471%`. Stage 2 keeps the existing manifest and promoters
authoritative. It does not create SOURCE_OWNED bytes, start M13, migrate ASM
to C++, add ROM/assets, or emit unknown `dc.b` ownership.

## Reusable expansion

`src/tools/m12_carver_expansion.py` adds a deterministic graph-guided pass to
the Carver IntervalDB. It reuses the existing confirmed 99-row table contract
at `[0x3F306,0x3FF66)` and the existing graphics census. Every field1 pointer
is represented as typed evidence and a table-to-evidence `REFERENCES` edge;
the target is also represented as a typed resource node. Candidate raw spans
are generated only from that confirmed table parent, but remain CANDIDATE and
never alter the manifest.

The CLI is `src/tools/re_m12_carver_expand.py`. Its output is intentionally
ignored build output because it references the user-supplied ROM and local
evidence:

`build/m12-carver-m12c2-expansion-b/`

## Deterministic result

The pass imported 758 UNKNOWN ranges and retained all `1,427,873`
SOURCE_OWNED bytes. Coverage remained `[0x000000,0x300000)`, with zero gaps
and overlaps. It produced 23,214 evidence records, 25,749 provenance nodes,
46,841 directed edges, zero conflicts, and five non-owning candidate ranges.
The fixed point was reached with zero further evidence, splits,
reclassifications, or queued expansions.

The table graph contained 96 non-zero field1 pointers. The graphics decoder
closure matched 68 pointer starts: 47 were already wholly closed by existing
ownership, 21 had a decoder span partially overlapping confirmed data, and 28
had no decoder boundary. No new SOURCE_OWNED bytes were safe to add.

Five rows produced raw-size hypotheses totaling `17,408` bytes:

| Rows | Candidate sizes | Result |
|---|---:|---|
| 4–8 | `0xC00`, `0xC00`, `0x1000`, `0xE00`, `0xE00` | rejected |

The hypotheses are not promotions: the existing evidence does not establish
an exact raw-resource consumer and a universal field6 boundary contract.
This is a deliberate negative result, not confidence inheritance.

## Stop decision

Stage 2 stops at the stable fixed point because no available existing evidence
producer safely yielded at least 16 KiB. The highest-ranked resulting campaign
is the pointer-table family spanning 13 UNKNOWN ranges and 957,632 bytes of
gap mass; its graph reachability is useful, but it lacks a closed parser/resource
boundary. A future campaign requires a new exact consumer or runtime contract
for that family. Detector expansion is not started automatically.

## Output hashes

For the recorded local input set, SHA-256 is:

| Output | SHA-256 |
|---|---|
| `interval_db.json` | `2c30aa561159b500155ef5d2bf5dc967c7333165171844316ec324928dea834c` |
| `gap_report.json` | `a49863cd858eb41f2c6f718f8388a5e60e02dce978e4e5af248ac1a5ebd953f8` |
| `expansion_report.json` | `8a92997ac852351096fe00f6f4269ec571e91003d94433a10a1677d208bfd529` |

These hashes cover the deterministic local output for the exact manifest,
ROM, evidence bundle, and graphics census named above.

## Validation

The Carver regression test, Python compilation, and `git diff --check` passed
locally. Debug, Release, and GNU/Linux-equivalent builds/tests remain required
for the final commit. The canonical ROM identity is unchanged. No M13 or
ASM-to-C++ work was started.
