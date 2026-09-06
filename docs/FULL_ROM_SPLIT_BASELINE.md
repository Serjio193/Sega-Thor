# M11.11 — Full-ROM split reassembly baseline

This is a developer-only, local-ROM-backed baseline. It proves that the
reconstructed layout can reproduce the canonical USA ROM byte-for-byte while
leaving uncertain regions as blobs. It does not claim full semantic
disassembly, data classification, or runtime behavior.

## Reproduce

Build `oasis_re_assemble`, provide the canonical ROM and local vasm, and use a
new ignored output directory:

```powershell
python src/tools/re_full_split_run.py `
  --tool build-m11-9-msvc/Debug/oasis_re_assemble.exe `
  --assembler build/m11-9/vasm/vasmm68k_mot.exe `
  --rom "local-roms/Beyond Oasis (USA).md" `
  --output build/m11-11/full_split
```

The script first reruns the M11.10 25-slice corpus, its expanded split and the
legacy M11.9 mixed split. It then copies only the 25 already verified ASM
ranges into a full layout, extracts every other range from the local ROM at
build time, assembles `full_layout.asm`, and writes `manifest.json`.

## Result

The canonical USA ROM is 3,145,728 bytes. The M11.11 run produced a 50-entry
contiguous manifest: 25 `CODE_VERIFIED` ranges and 25 `UNKNOWN` blob ranges.
There are zero gaps and zero overlaps. No `DATA_KNOWN` or `CONFLICT` range was
invented.

| Metric | Bytes | Percent |
| --- | ---: | ---: |
| ASM | 1,846 | 0.0586827596% |
| Structured data | 0 | 0% |
| Local-ROM blobs | 3,143,882 | 99.9413172404% |
| Conflicts | 0 | 0% |
| Total ROM | 3,145,728 | 100% |

The rebuilt file is exactly 3,145,728 bytes and matches every byte. Its hashes
are CRC32 `C4728225`, SHA-1
`2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
If a future run differs, the report prints the ROM offset, expected/actual
byte, manifest entry index and emitted artifact type.

The smallest manifest range is 10 bytes and the largest is 3,091,450 bytes.
The rebuilt ROM, blobs, assembler output and local corpus are ignored and are
not committed. The single next recommendation is **A — begin automatically
replacing verified blob regions with ASM/data**; it is deferred and not
implemented by M11.11.
