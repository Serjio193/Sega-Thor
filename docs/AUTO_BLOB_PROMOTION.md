# M11.12 — automated blob-to-source promotion PoC

This developer-only tool ranks candidates from the existing candidate-map and
mass-verification evidence, attempts at most 25 safe bounded ranges, and accepts
each promotion only after both slice and full-ROM byte comparisons match. It
does not assign semantic names or classify uncertain data.

## Reproduce

Use a new ignored output directory and the local canonical ROM/toolchain:

```powershell
python src/tools/re_auto_promote.py `
  --tool build-m11-12-msvc/Debug/oasis_re_assemble.exe `
  --range-tool build-m11-12-msvc/Debug/oasis_re_assemble_range.exe `
  --assembler build/m11-9/vasm/vasmm68k_mot.exe `
  --rom "local-roms/Beyond Oasis (USA).md" `
  --manifest build/m11-11/full1/manifest.json `
  --mass-report build/mass-final-a.json `
  --ghidra-map build/ghidra-derived-from-prior-map.json `
  --output build/m11-12/auto1
```

The manifest and report are generated under the output directory. Candidate
order is derived from evidence fields, score, size and address; no new address
list is supplied by the caller. Rejected trials are discarded, while accepted
code is copied into the final local output.

## Result

The run discovered 534 evidence records, retained 210 eligible candidates,
attempted 25 and accepted 21. Four were rejected: two unsupported forms and two
assembler errors (`ori.w #$1,CCR`), with no full-ROM mismatch. The accepted
promotions are:

| Address | Bytes | Instructions | Score |
| ---: | ---: | ---: | ---: |
| `0x06121A` | 24 | 6 | 91 |
| `0x0611F4` | 38 | 6 | 87 |
| `0x0032E8` | 16 | 4 | 78 |
| `0x00E66E` | 20 | 6 | 78 |
| `0x00D398` | 26 | 5 | 77 |
| `0x01A21E` | 30 | 7 | 77 |
| `0x025BD8` | 38 | 9 | 76 |
| `0x002CBC` | 40 | 11 | 75 |
| `0x0112CC` | 42 | 13 | 75 |
| `0x01132A` | 26 | 6 | 74 |
| `0x01DFA4` | 30 | 7 | 74 |
| `0x025BB2` | 30 | 7 | 74 |
| `0x0008B6` | 52 | 17 | 74 |
| `0x03C956` | 52 | 9 | 74 |
| `0x00E338` | 54 | 20 | 74 |
| `0x014126` | 54 | 13 | 74 |
| `0x0141CA` | 54 | 18 | 74 |
| `0x0193E0` | 8 | 2 | 73 |
| `0x01960C` | 38 | 9 | 73 |
| `0x00E682` | 56 | 16 | 73 |
| `0x00C9B2` | 58 | 15 | 73 |

Coverage changed from 1,846 ASM bytes and 3,143,882 blob bytes to 2,632 ASM
bytes and 3,143,096 blob bytes. Structured data and conflicts remain zero.
The manifest has 88 contiguous entries: 46 code ranges and 42 blob ranges.
The UNKNOWN entry count increases because each accepted interior promotion
splits its surrounding blob; blob bytes are the meaningful coverage delta.

The final rebuilt ROM is 3,145,728 bytes and matches the canonical USA ROM.
CRC32 is `C4728225`, SHA-1 is
`2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256 is
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
M11.9, M11.10 and the M11.11 full-ROM baseline are rerun before promotion.
No handwritten opcode overrides are used. The exact decision is
`AUTO_PROMOTION_HIGH_VALUE`; the single next recommendation is **A — increase
the automatic promotion batch to 100 candidates**, deferred and not implemented.
