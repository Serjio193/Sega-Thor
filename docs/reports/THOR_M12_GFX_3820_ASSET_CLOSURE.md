# M12-GFX-2 — 0x3820 Caller-to-Asset Closure

## Scope and ABI

This developer-only pass consumes the published M12-GFX-1 caller list and strict census JSON; it does not repeat the whole-ROM scan. No ROM, decoded payload, PNG, CRAM/SAT capture, M13 work, or ASM-to-C++ migration is committed.

The proven ABI is `A0` compressed source in / advanced to the exclusive consumed end, `A1` output destination / advanced to the exclusive produced end, format dispatch by `source[2]` (command stream when nonzero, bit stream when zero), and no nested helper call or hardware access inside `[0x003820,0x003B3E)`. The 68000 slice and independent parser agree on the existing format-A and format-B vectors: `0x16943C` consumes 1217 and emits 3072; `0x1894EA` consumes 112 and emits 128. Register preservation and command semantics match the existing contract in `docs/REVERSE_ENGINEERING.md`; no status return is used.

## Ownership accounting

| measure | before | after | delta |
| --- | ---: | ---: | ---: |
| SOURCE_OWNED bytes | 1,427,873 | 1,475,262 | 47,389 |
| SOURCE_OWNED percent | 45.3908602397% | 46.8973159790% | 1.5064557393% |

Carver blockers before/after are derived by subtracting only the exact caller-derived promoted spans from the preserved blocker intervals:

| blocker | before ranges/bytes | after ranges/bytes |
| --- | ---: | ---: |
| B | 113 / 623,036 | 113 / 623,036 |
| F | 56 / 58,789 | 56 / 58,789 |
| G | 589 / 1,036,030 | 588 / 988,641 |

## Table and caller-family graph

The exact table `[0x05CE96,0x05D046)` is 108 four-byte absolute ROM pointers consumed by `0x00D3B2` and the related `0x00D4EE` path. Entries 1..107 enumerate strict Ancient resources; all 107 targets were already SOURCE_OWNED at baseline, so table promotion gain is zero.

| call-site | containing routine/family | source set | destination | mode | confidence/blocker |
| ---: | --- | --- | --- | --- | --- |
| `0x00C394` | 0x00C326 bounded graphics loader | `0x16943C..0x1698FD` | `0x00FF2FA8` | direct literal | CONFIRMED; exact parser end |
| `0x00D3C8` | 0x00D3B2 indexed-resource loader | `0x05CE96[1..107] (107 targets)` | `0x00FF2FA8` | indirect table | CONFIRMED; baseline-owned |
| `0x00D4EE` | 0x00D406 shared resource loader | `0x05CE96[1..107] (107 targets)` | `0x00FF3FA8` | indirect table | CONFIRMED; baseline-owned |
| `0x00D54A` | 0x00D406 shared resource loader | `A4 = entry A1 + 4; helper 0x00F80E preserves A4; RAM-mediated field` | `0x00FF3FA8` | indirect/RAM | BLOCKED: INHERITED_A1_FIELD_NOT_ROM_PROVEN |
| `0x00D650` | 0x00D406 shared resource loader | `A4/A5 = post-state of first 0x3820 at 0x00D54A; FF16F1.bit2-gated sequential continuation` | `A5 after first call` | indirect/RAM | BLOCKED: FIRST_STREAM_AND_CONTINUATION_NOT_ROM_PROVEN |
| `0x014558` | 0x01454C menu graphics family | `0x141580..0x141A7E` | `0x00FF2FA8` | direct literal | CONFIRMED; exact parser end |
| `0x01458E` | 0x01454C menu graphics family | `0x1425AE..0x142B69` | `0x00FF2FA8` | direct literal | CONFIRMED; exact parser end |
| `0x0145C4` | 0x01454C menu graphics family | `0x143262..0x1432A6` | `0x00FF2FA8` | direct literal | CONFIRMED; exact parser end |
| `0x0145FC` | 0x01454C menu graphics family | `0x143A42..0x143E76` | `0x00FF2FA8` | direct literal | CONFIRMED; exact parser end |
| `0x024A20` | 0x024858 bounded graphics loader | `0x143A42..0x143E76` | `0x00FF2FA8` | direct literal | CONFIRMED; exact parser end |
| `0x02A67E` | 0x02A652 bounded graphics loader | `0x2E2FE6..0x2E304A` | `A1 inherited` | direct literal | CONFIRMED; exact parser end |
| `0x02B1D0` | 0x02B194 bounded graphics loader | `0x2E5FD0..0x2E6000` | `A1 inherited` | direct literal | CONFIRMED; exact parser end |
| `0x02DB52` | 0x02DB24 resource family | `A0 = 0x00FF17AA post-source saved by preceding 0x00D406; A1 = 0x00FF2FA8` | `0x00FF2FA8` | indirect/RAM | BLOCKED: D406_POST_SOURCE_NOT_ROM_PROVEN |
| `0x02F6A0` | 0x02F662 resource family | `A0/A1 = 0x00FF17AA/0x00FF17AE post-state saved by preceding 0x00D406` | `0x00FF17AE after 0x00D406` | indirect/RAM | BLOCKED: D406_POST_STATE_NOT_ROM_PROVEN |
| `0x03A7FE` | 0x03A748 screen initialization family | `0x16943C..0x1698FD` | `0x00FF2FA8` | direct literal | CONFIRMED; exact parser end |
| `0x03ACB4` | 0x03ACA8 tilemap loader | `0x17A750..0x17A78F` | `0x00FFB1AE` | direct literal | CONFIRMED; exact parser end |
| `0x03ADC0` | 0x03ADB4 tilemap loader | `0x17E3BA..0x17E3E1` | `0x00FFB1AE` | direct literal | CONFIRMED; exact parser end |
| `0x03B236` | 0x03B1D0 resource family | `A0 = 4(A5); A1 = 0x00FF316C shared output buffer` | `0x00FF316C` | indirect/RAM | BLOCKED: A5_FIELD_NOT_ROM_PROVEN |
| `0x03B28A` | 0x03B1D0 resource family | `A0 = A3; A1 = 0x00FF316C shared output buffer` | `0x00FF316C` | indirect/RAM | BLOCKED: A3_ARGUMENT_NOT_ROM_PROVEN |
| `0x03B2FE` | 0x03B1D0 resource family | `A0 = A4; 0x002CBC/0x00D950 preserve A4; A1 = 0x00FF316C` | `0x00FF316C` | indirect/RAM | BLOCKED: A4_ARGUMENT_NOT_ROM_PROVEN |
| `0x03C07C` | 0x03C04C resource family | `A0/A1 inherited at shared family entry` | `A1 inherited` | indirect/RAM | BLOCKED: CALLER_ARGUMENT_NOT_ROM_PROVEN |
| `0x03C276` | 0x03C1E8 resource family | `0x1894EA..0x18955A` | `0x00FF2FA8` | sequential A0 | CONFIRMED; exact parser end |
| `0x03C27E` | 0x03C1E8 resource family | `0x18955A..0x18A4BE` | `0x00FF2FA8` | sequential A0 | CONFIRMED; exact parser end |
| `0x03C286` | 0x03C1E8 resource family | `0x18A4BE..0x18CC6E` | `0x00FF2FA8` | sequential A0 | CONFIRMED; exact parser end |
| `0x03C5CA` | 0x03C59C resource family | `0x18CF98..0x18D01B` | `0x00FF3FAC` | sequential A0 | CONFIRMED; exact parser end |
| `0x03C5D2` | 0x03C59C resource family | `0x18D01B..0x18D727` | `0x00FF3FAC` | sequential A0 | CONFIRMED; exact parser end |
| `0x03C5DA` | 0x03C59C resource family | `0x18D727..0x18EA9A` | `0x00FF3FAC` | sequential A0 | CONFIRMED; exact parser end |
| `0x03C5E6` | 0x03C59C resource family | `0x18EA9A..0x18EB1F` | `0x00FF3FAC` | sequential A0 | CONFIRMED; exact parser end |
| `0x03C9DA` | 0x03C9CC resource family | `0x18F214..0x18F252` | `0x00FF3FAC` | sequential A0 | CONFIRMED; exact parser end |
| `0x03C9E2` | 0x03C9CC resource family | `0x18F252..0x18F2BC` | `0x00FF3FAC` | sequential A0 | CONFIRMED; exact parser end |
| `0x03C9EA` | 0x03C9CC resource family | `0x18F2BC..0x190EE7` | `0x00FF3FAC` | sequential A0 | CONFIRMED; exact parser end |
| `0x03CBE0` | 0x03CB9E resource family | `0x191F0A..0x191F8E` | `0x00FF3FAC` | sequential A0 | CONFIRMED; exact parser end |
| `0x03CBE8` | 0x03CB9E resource family | `0x191F8E..0x192DF1` | `0x00FF3FAC` | sequential A0 | CONFIRMED; exact parser end |
| `0x03CBF0` | 0x03CB9E resource family | `0x192DF1..0x194BDA` | `0x00FF3FAC` | sequential A0 | CONFIRMED; exact parser end |
| `0x03CC5A` | 0x03CC4C resource family | `0x194BDA..0x194C52` | `0x00FF3FAC` | sequential A0 | CONFIRMED; exact parser end |
| `0x03CC62` | 0x03CC4C resource family | `0x194C52..0x196289` | `0x00FF3FAC` | sequential A0 | CONFIRMED; exact parser end |
| `0x03CCD8` | 0x03CCCA resource family | `0x19628A..0x196300` | `0x00FF3FAC` | sequential A0 | CONFIRMED; exact parser end |
| `0x03CCE0` | 0x03CCCA resource family | `0x196300..0x196B87` | `0x00FF3FAC` | sequential A0 | CONFIRMED; exact parser end |
| `0x03CCE8` | 0x03CCCA resource family | `0x196B87..0x19908F` | `0x00FF3FAC` | sequential A0 | CONFIRMED; exact parser end |
| `0x03CEA6` | 0x03CE98 resource family | `0x199CBA..0x199D35` | `0x00FF3FAC` | sequential A0 | CONFIRMED; exact parser end |
| `0x03CEAE` | 0x03CE98 resource family | `0x199D35..0x19A44C` | `0x00FF3FAC` | sequential A0 | CONFIRMED; exact parser end |
| `0x03CEBA` | 0x03CE98 resource family | `0x19A44C..0x19A4E5` | `0x00FF3FAC` | sequential A0 | CONFIRMED; exact parser end |
| `0x03CEC2` | 0x03CE98 resource family | `0x19A4E5..0x19BB91` | `0x00FF3FAC` | sequential A0 | CONFIRMED; exact parser end |
| `0x03CECA` | 0x03CE98 resource family | `0x19BB91..0x19D232` | `0x00FF3FAC` | sequential A0 | CONFIRMED; exact parser end |
| `0x03D048` | 0x03CFDE graphics family | `0x16943C..0x1698FD` | `0x00FF2FA8` | direct literal | CONFIRMED; exact parser end |
| `0x03D38E` | 0x03D228 graphics family | `0x2F9A7E..0x2FA686` | `0x00FF3A32` | direct literal | CONFIRMED; exact parser end |
| `0x03D3A0` | 0x03D228 graphics family | `0x2FA686..0x2FB265` | `0x00FF4E32` | direct literal | CONFIRMED; exact parser end |
| `0x03D5AE` | 0x03D59A entity initializer | `A0 loaded from RAM-mediated entity record at 0x00FF199E` | `0x00FF19AE` | indirect/RAM | BLOCKED: RAM_MEDIATED_SOURCE_NOT_ROM_PROVEN |
| `0x03E61A` | 0x03E4DC graphics family | `A0/A1 inherited before later direct 0x16943C arm` | `A1 inherited` | indirect/RAM | BLOCKED: CALLER_ARGUMENT_NOT_ROM_PROVEN |
| `0x03E662` | 0x03E4DC graphics family | `0x16943C..0x1698FD` | `0x00FF2FA8` | direct literal | CONFIRMED; exact parser end |
| `0x03E704` | 0x03E4DC graphics family | `0x16943C..0x1698FD` | `0x00FF2FA8` | direct literal | CONFIRMED; exact parser end |
| `0x03E820` | 0x03E7F4 graphics family | `0x141580..0x141A7E` | `0x00FF2FA8` | direct literal | CONFIRMED; exact parser end |

## Exact resources and overlaps

The closure catalog contains 34 call-derived stream edges merging to 34 unique resource starts. Each record stores the strict half-open Ancient boundary, declared compressed bytes, decompressed bytes, output SHA-256, caller reverse-xrefs, baseline owner, and promotion decision in the machine-readable report. Promoted spans: 10, 47,389 bytes.

The five newly closed sequential families are `0x18F214`, `0x191F0A`, `0x194BDA`, `0x19628A`, and `0x199CBA`. Decoder candidates nested inside these caller-selected spans are reclassified as caller-proven streams; arbitrary sibling starts and post-chain gaps remain UNKNOWN. No executable conflict was accepted.

The 64 KiB promotion target was not reached: exact caller-derived closure is 47,389 bytes. Further promotion is intentionally gated on closing the ten explicit unresolved source producers below; no generic detector or visual inference is used to fill the gap.

## Sibling loaders and blockers

The bounded second-order review followed only nearby `0x37D2`/`0x3820`, `0xD3B2`, and existing loader relations. No new generic detector family was added. Nearby `0xD950`, `0x2CBC`, `0x2E1E`, and `0x36D4` effects do not close an Ancient ROM source set and remain evidence-only.

Unresolved callers are explicit: RAM-mediated A0/A1, inherited caller arguments, helper output, or entity-record pointers cannot be promoted without a closed ROM source producer. CRAM, SAT, VRAM, runtime registers, and visual identity are not required for generic resource ownership and were not used.

## Gates and identity

Canonical ROM: CRC32 `C4728225`, SHA-1 `2944910c07c02eace98c17d78d07bef7859d386a`, SHA-256 `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`. Reused M12-GFX-1 JSON: whole-ROM scan repeated = `False`. The machine-readable report is emitted as `caller_closure_report.json`. Implementation SHA: `28cceb2b17b15bc884124c63729d4d24e456668b`; exact implementation-SHA CI: GitHub Actions run `34647644049` (success). Final publication SHA: `70e2a8e6f09200f55798f9bfbb42f0da44f2ecf2`; exact publication CI: run `34647889523` (success).
