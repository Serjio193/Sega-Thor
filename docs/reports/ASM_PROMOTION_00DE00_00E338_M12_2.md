# M12.2 — Transactional ASM promotion of P0 region `0x00DE00..0x00E338`

Status: `M12_2_P0_CODE_PROMOTION_PARTIAL_EXACT` — complete. This report is
the evidence and transaction record for baseline commit
`0dac30bcca103ae03a6372d24c2aca25e1fb6460`. No M12.3 work was started.

## Scope and decision

The target is the single M12.1 `UNKNOWN` blob `[0x00DE00,0x00E338)`,
1,336 bytes. Evidence was taken from the existing dynamic-PC priority report,
the global static decoder census, direct branch/call targets, fallthrough,
surrounding ASM, and exact vasm reassembly. The target contains 31 observed
PCs and 87 static incoming xrefs in the priority evidence.

Only independently bounded exact instruction intervals were promoted. The
classification of every promoted interval is `68000_CODE_CONFIRMED`; semantic
names and portable runtime ownership are not inferred. Structured data,
padding, `POSSIBLE_CODE`, and `UNKNOWN_DATA` were not claimed.

The developer-only decoder additions cover the target's repeated `ADDX.W D0,D0`,
`MULU.W D0,D1`, and exact `EOR`/`CMP` operand direction. The range checker now
accepts exact unresolved indirect calls while continuing to reject unresolved
indirect jumps. Indirect exits are recorded below; this keeps the dispatch at
`0x00E2F0` fail-closed without blocking independent islands.

## Promoted intervals

| Interval | Bytes | Evidence / boundary | Indirect exits |
|---|---:|---|---|
| `0x00DEEC..0x00DF52` | 102 | CFG closes at loop/return; 5 direct branches | `0x00DF1A` indirect JSR |
| `0x00E0B8..0x00E0BA` | 2 | standalone RTS island | none |
| `0x00E0BA..0x00E0F4` | 58 | bounded loop closes at RTS; direct branches and DBF | `0x00E0E4` indirect JSR |
| `0x00E0F4..0x00E0FE` | 10 | exact dispatch prefix; direct branch exits to `0x00E106` | none |
| `0x00E106..0x00E140` | 58 | bounded loop closes at RTS; direct branches and DBF | `0x00E130` indirect JSR |
| `0x00E268..0x00E2A2` | 58 | bounded arithmetic routine closes at RTS; 21 observed PCs | none |
| `0x00E2A2..0x00E2BA` | 24 | caller/body closes at RTS; direct BSR to `0x00E268` | none |
| `0x00E2D4..0x00E2F0` | 28 | exact dispatch setup prefix ends at unresolved JMP | boundary at `0x00E2F0` |
| `0x00E302..0x00E308` | 6 | observed case arm, direct branch to shared tail | none |
| `0x00E308..0x00E30C` | 4 | observed case arm, direct branch to shared tail | none |
| `0x00E30C..0x00E310` | 4 | observed case arm, direct branch to shared tail | none |
| `0x00E310..0x00E316` | 6 | observed case arm, direct branch to shared tail | none |
| `0x00E332..0x00E338` | 6 | shared `MOVEM` restore and RTS tail | none |

Total source-owned ASM promotion: **366 bytes in 13 non-overlapping
intervals**. The four case arms are intentionally separate because a single
linear slice stops at each unconditional branch; the shared tail is separate
and exact.

## Target ownership and gap census

| Metric | Before | After |
|---|---:|---:|
| ASM source ownership | 0 | 366 bytes (27.395209581%) |
| Structured source ownership | 0 | 0 bytes (0%) |
| Padding/alignment | 0 | 0 bytes |
| Remaining blob | 1,336 | 970 bytes (72.604790419%) |
| `POSSIBLE_CODE_BYTES` | 0 | 0 |
| `UNKNOWN_DATA_BYTES` | 0 | 0 |
| `UNRESOLVED_BOUNDARY_BYTES` | 1,336 | 970 |
| `EXECUTABLE_BLOB_BYTES_REMOVED` | — | **366** |

All remaining bytes are conservative `UNKNOWN` blobs. They are not silently
reclassified as data or code.

### Exact remaining intervals

| Interval | Bytes | Classification | Reason |
|---|---:|---|---|
| `0x00DE00..0x00DEEC` | 236 | `UNRESOLVED_BOUNDARY` | No independently closed code boundary or exact CFG evidence before the promoted island. |
| `0x00DF52..0x00E0B8` | 358 | `UNRESOLVED_BOUNDARY` | Continuation between closed islands is unresolved. |
| `0x00E0FE..0x00E106` | 8 | `UNRESOLVED_BOUNDARY` | The branch ends at `0x00E0FE`; contiguity to the next entry is not proven. |
| `0x00E140..0x00E268` | 296 | `UNRESOLVED_BOUNDARY` | No exact source-owned continuation between the closed routine and arithmetic island. |
| `0x00E2BA..0x00E2D4` | 26 | `UNRESOLVED_BOUNDARY` | No exact boundary from the closed arithmetic body to dispatch setup. |
| `0x00E2F0..0x00E2F2` | 2 | `UNRESOLVED_BOUNDARY` | Indirect JMP dispatch; target table and continuation are unresolved. |
| `0x00E2F2..0x00E302` | 16 | `UNRESOLVED_BOUNDARY` | Jump-table continuation after the unresolved dispatch is not proven. |
| `0x00E316..0x00E332` | 28 | `UNRESOLVED_BOUNDARY` | Case-arm continuation to the shared tail is not independently closed. |

## Whole-ROM census

The transactional materialization preserves the exact full-ROM split.

| Metric | Before | After |
|---|---:|---:|
| ASM ranges / bytes / percent | 209 / 14,096 / 0.448099772% | 222 / 14,462 / 0.459734599% |
| Structured-data ranges / bytes / percent | 0 / 0 / 0% | 0 / 0 / 0% |
| Blob ranges / bytes / percent | 138 / 3,131,632 / 99.551900228% | 144 / 3,131,266 / 99.540265401% |
| Gaps | 0 | 0 |
| Overlaps | 0 | 0 |

## Exactness and verification

Every promoted slice assembled with vasm 1.8g using `-m68000 -no-opt -Fbin`
and matched its canonical ROM interval byte-for-byte. The full materialized
layout also matched exactly:

* size: `3,145,728` bytes;
* CRC32: `C4728225`;
* SHA1: `2944910c07c02eace98c17d78d07bef7859d386a`;
* SHA256: `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

The report's before/after manifests have no gaps or overlaps, and the
materialized manifest remains a full-ROM transaction. No ROM, save, or
commercial asset was added to the repository.

An independent evidence-integrity audit also reassembled all **222/222**
materialized ASM ranges exactly. Its historical mass/Ghidra inputs report 15
provenance mismatches because they predate M12.2; this is recorded as an audit
limitation and does not change the byte-exact transaction or promote any
remaining blob.

## Recomputed queue and stop gate

After removing the M12.2 target from the P0 queue, exactly one M12.3 proposal
was recomputed: `0x006516..0x0083D4` (7,870 bytes, 25 observed PCs, 0 static
xrefs). It is proposal-only and was not started. No C++ gameplay/runtime
migration, emulator expansion, or unrelated routine promotion was performed.
