# M11.15 — evidence integrity audit and classification trust repair

M11.15 audits the 203 exact ASM ranges from the M11.14 manifest without adding
promotions or changing the full-ROM split. Exact reassembly is now reported as
`ASM_ROUNDTRIP_EXACT`; static, dynamic and behavioral trust require independent
evidence.

The audit reassembles every ASM artifact against the canonical USA ROM, checks
the artifact `org` and entry label, validates the candidate and Ghidra entry and
range linkage, records source artifact hashes, and retains concrete incoming
xref source addresses. A caller can support a target only when its exact edge
points to the audited entry and the caller already has a trusted level. Weak
caller chains do not bootstrap trust.

## Baseline and result

Baseline: post-M11.14 `build/m11-14/batch2/manifest.json`; 203
`CODE_VERIFIED` ranges, 13,550 ASM bytes, 0 handwritten overrides. The audit
output is local and ignored under `build/m11-15/audit2`.

| Level | Ranges | Bytes |
| --- | ---: | ---: |
| `ASM_ROUNDTRIP_EXACT` | 197 | 12,520 |
| `CODE_STATIC_SUPPORTED` | 5 | 1,006 |
| `CODE_EXECUTED` | 1 | 24 |
| `BEHAVIOR_VERIFIED` | 0 | 0 |

The 197 ranges are classification downgrades only. Their ASM artifacts,
manifest ranges and full-ROM ownership remain unchanged. Five independently
anchored routines retain static support (`0x3820`, `0x62CC`, `0x9BF2`, `0xA8DA`,
`0xD3B2`), and `0x6121A` retains the existing dynamic execution evidence.
There are no upgrades. One known Ghidra boundary mismatch remains at `0x3820`
(`0x38D0` versus the confirmed `0x3B3E`) and is retained as an unresolved
classification concern.

## Provenance and negative corpus

All 203 records use the canonical ROM SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`, and all
203 source artifacts assemble to their exact ROM slices. One provenance mismatch
was found and recorded: the Ghidra range for `0x3820` is shorter than the
audited range. The audit rejects an evidence record whose entry or range belongs
to another object; a synthetic A/B entry-range mismatch is a regression test.

The bounded negative corpus includes `4E71 4E75` and additional legal 68000
examples. They may be exact ASM round trips, but without a trusted xref, vector,
dynamic observation or independent anchor they remain `ASM_ROUNDTRIP_EXACT`.

Canonical form metrics use one key for IR and ASM: `rts`, `moveq`, normalized
`.s` branches such as `beq.branch`, and `mnemonic.suffix`. The audit recorded
109 normalized ASM keys, including 234 `rts` and 90 `moveq` instructions.

## Full-ROM and decision

The audited manifest preserves the original `CODE_VERIFIED`/`UNKNOWN` ownership
and reassembles exactly: 3,145,728 bytes, CRC32 `C4728225`, SHA-1
`2944910c07c02eace98c17d78d07bef7859d386a`, SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

The automatic promoter now labels a successful byte round trip
`ASM_ROUNDTRIP_EXACT`; it can assign a higher level only from explicit static,
dynamic or behavioral evidence fields and never from a weak direct-caller count.

Decision: `EVIDENCE_TRUST_NEEDS_FIXUPS`. The audit and gate are operational, but
the known `0x3820` boundary provenance gap and the absence of dynamic evidence
for nearly all ranges remain bounded issues. The single next recommendation is
**B — targeted dynamic confirmation of critical code**. It is deferred and not
implemented here.
