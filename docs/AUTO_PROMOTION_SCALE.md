# M11.13 — automated promotion scale pass

This checkpoint continues from the post-M11.12 manifest. The existing
evidence-ranked runner attempted 100 deterministic candidates, accepted only
slice and full-ROM exact transactions, and kept rejected ranges as local-ROM
backed UNKNOWN blobs. Structured-data promotion and semantic naming remain
outside this task.

## Reproduce

Use the post-M11.12 manifest and a new ignored output directory:

```powershell
python src/tools/re_auto_promote.py `
  --tool build-m11-12-msvc/Debug/oasis_re_assemble.exe `
  --range-tool build-m11-12-msvc/Debug/oasis_re_assemble_range.exe `
  --assembler build/m11-9/vasm/vasmm68k_mot.exe `
  --rom "local-roms/Beyond Oasis (USA).md" `
  --manifest build/m11-12/auto2/manifest.json `
  --mass-report build/mass-final-a.json `
  --ghidra-map build/ghidra-derived-from-prior-map.json `
  --output build/m11-13/scale1 `
  --max-candidates 100
```

No address list is supplied. Candidates are ranked from existing evidence, and
already accepted code is excluded by the input manifest and live layout.

## Result

| Metric | Before (M11.12) | After (M11.13) |
| --- | ---: | ---: |
| ASM bytes | 2,632 | 6,462 |
| Structured-data bytes | 0 | 0 |
| Local-ROM blob bytes | 3,143,096 | 3,139,266 |
| `CODE_VERIFIED` entries | 46 | 130 |
| `UNKNOWN` entries | 42 | 101 |
| Manifest entries | 88 | 231 |

The batch discovered 534 records, retained 189 candidates after excluding the
46 existing code ranges, attempted 100 and accepted 84 (84.0%). Sixteen were
rejected: 14 `UNSUPPORTED_FORM` and 2 `SLICE_MISMATCH`. There were no full-ROM
mismatches, assembler failures, boundary conflicts or handwritten overrides.
The UNKNOWN count rises because interior promotions split surrounding blob
ranges; blob bytes are the coverage measure.

Accepted promotions cover 161 distinct operation/width/addressing forms, with
27 operation/width families newly present relative to M11.12. Remaining
unsupported evidence is reported as `unsupported exact IR`; the two slice
mismatches are retained for later decoder/boundary investigation. The report's
`top_reject_forms` records common reject forms and examples.

## CCR fix

Decoder metadata and IR for immediate-to-CCR operations were correct.
`ori.w #$1,CCR` was an emitter syntax mismatch: vasm accepts the 68000 CCR
encoding as `ori.b #$1,CCR`, while SR remains word-sized. The emitter now uses
byte syntax for generic `ori`/`andi`/`eori` to CCR, with a synthetic regression
test. Both previous CCR candidates (`0x00DA2A`, `0x00B9EC`) were accepted.

## Exactness and regressions

The final rebuilt ROM is 3,145,728 bytes and matches the canonical USA ROM:
CRC32 `C4728225`, SHA-1
`2944910c07c02eace98c17d78d07bef7859d386a`, SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
The M11.9 controls, M11.10 corpus, M11.11 full split and M11.12 promotion
regressions all report `MATCH`.

Decision: `AUTO_PROMOTION_SCALE_HIGH_VALUE` — the batch exceeds 50 attempts,
acceptance is above 70%, ASM coverage materially increased, and exactness
remained stable. The single next recommendation is **A — continue automatic
code promotion with another large batch**. It is deferred and not implemented.
