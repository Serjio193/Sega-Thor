# M11.14 — automatic code promotion large batch II

M11.14 continues from the M11.13 manifest and excludes its 100 attempted
candidates. The two old M11.13 slice mismatches are retried after bounded
investigation; no structured data or semantic naming is promoted.

## Reproduce

```powershell
python src/tools/re_auto_promote.py `
  --tool build-m11-12-msvc/Debug/oasis_re_assemble.exe `
  --range-tool build-m11-12-msvc/Debug/oasis_re_assemble_range.exe `
  --assembler build/m11-9/vasm/vasmm68k_mot.exe `
  --rom "local-roms/Beyond Oasis (USA).md" `
  --manifest build/m11-13/scale1/manifest.json `
  --mass-report build/mass-final-a.json `
  --ghidra-map build/ghidra-derived-from-prior-map.json `
  --prior-report build/m11-13/scale1/promotion_report.json `
  --output build/m11-14/batch2 `
  --max-candidates 150
```

The evidence ranking found 89 new eligible candidates. Two prior slice
mismatches were retried after the general byte-immediate emitter change, for 91
attempts total. Candidate order remains score, size, address; no address list is
provided by the caller.

## Coverage and acceptance

| Metric | Before (M11.13) | After (M11.14) | Delta |
| --- | ---: | ---: | ---: |
| ASM bytes | 6,462 | 13,550 | +7,088 |
| Blob bytes | 3,139,266 | 3,132,178 | -7,088 |
| `CODE_VERIFIED` entries | 130 | 203 | +73 |
| `UNKNOWN` entries | 101 | 136 | +35 |
| Manifest entries | 231 | 339 | +108 |

The batch attempted 91 candidates and accepted 73 (80.22%). Acceptance by
window was 72%, 88%, 76% and 87.5%; the trend is `STABLE`. Full-ROM exactness
held after every accepted transaction and at the final batch boundary.

## Reject clustering

| Class | Count | Common form / examples |
| --- | ---: | --- |
| `UNSUPPORTED_FORM` | 8 | unsupported exact IR; `0x009AD6`, `0x0027A6`, `0x00F160` |
| `SLICE_MISMATCH` | 3 | `cmp.w data_register>data_register`; `0x00B6A6`, `0x00B022`, `0x00B28E` |
| `ASSEMBLER_SYNTAX` | 7 | byte immediates with noncanonical extensions, `EXG.W`, unsized `JSR`; examples in report |

Every reject records failure class, mnemonic when emitted, operand form, raw
opcode and address in `promotion_report.json`. Unsupported inventory contains
one remaining cluster (`unsupported exact IR`, count 8), fixed=false.

The old `0x020802` mismatch was an `IR_OPERAND` preservation issue: its byte
immediate extension was `0xFFF3`, which vasm cannot encode as a byte immediate;
the general emitter now preserves that raw extension and safely rejects the
nonrepresentable form. The old `0x00B6A6` mismatch is an `ASM_ENCODING` issue
for the repeated `cmp.w` data-register form; three candidates remain rejected.
No unsafe raw-opcode or address-specific workaround was added.

## Exactness and decision

The final ROM is 3,145,728 bytes and matches CRC32 `C4728225`, SHA-1
`2944910c07c02eace98c17d78d07bef7859d386a` and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
M11.9, M11.10, M11.11, M11.12 and the M11.13 accepted set all report
`MATCH`. Handwritten overrides remain zero; CODE_VERIFIED means exact
reassemblable code only and semantic meaning remains UNKNOWN.

Decision: `AUTO_PROMOTION_BATCH2_HIGH_VALUE` — 91 attempts, 80.22% acceptance,
material ASM growth and stable canonical full-ROM exactness. The single next
recommendation is **A — run another automatic code batch**. It is deferred and
not implemented here.
