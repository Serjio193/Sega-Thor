# M11.9 — bounded reassemblable disassembly

Result: `REASSEMBLABLE_DISASM_POC_HIGH_VALUE`. This developer-only experiment
does not change the native production architecture or identify Ancient's tools.

## Reproduce locally

Build `oasis_re_assemble` through the existing CMake configuration. Supply the
canonical USA ROM locally. Python uses only its standard library. Supply
`vasmm68k_mot` on PATH or pass `--assembler` explicitly; no download occurs in
the runner. Use a **new ignored directory** for each run:

```powershell
python src/tools/re_assemble_run.py --tool build-current-debug/oasis_re_assemble.exe --assembler build/m11-9/vasm/vasmm68k_mot.exe --rom "local-roms/Beyond Oasis (USA).md" --output build/m11-9/reconstructed_poc
```

The same command with the Release executable is the local ROM-backed regression
test for all five routines. The runner exits nonzero on any assembly, comparison
or split failure and retains `result.json`. Ordinary CTest uses synthetic inputs
and needs neither ROM nor assembler. Standalone first-difference verification:

```powershell
build-current-debug/oasis_re_assemble.exe verify "local-roms/Beyond Oasis (USA).md" build/m11-9/reconstructed_poc/code/sub_00A8DA.bin 0xA8DA 0xA8F0
```

`end` is exclusive. Empty/short/long rebuilt files are compared, including EOF
differences. Missing files and invalid ROM/range inputs are errors, not matches.

## Tool provenance and encoding policy

PATH and the configured developer-tool directory contained no vasm, AS or GNU
M68K assembler. The official source host refused the connection. The public
[vasm mirror](https://github.com/vaelen/vasm/tree/8ecb8e6c7a31350ef32c7f1fee289e3677a9a8f0)
at `8ecb8e6c7a31350ef32c7f1fee289e3677a9a8f0` was built locally with the
existing MinGW GCC using `CPU=m68k SYNTAX=mot CC=gcc TARGETEXTENSION=.exe`.
Observed banner: vasm **1.8g**, M68k backend **2.3f**, Motorola syntax **3.13**,
binary output **1.8a**. Source and binaries remain ignored and external to Git.
Its `doc/vasm.texi` legal section permits non-commercial use; this is a local
research requirement, not a bundled/distributed dependency or a claim of a
permissive commercial license. Ancient's original assembler remains UNKNOWN.

Flags: `-m68000 -no-opt -Fbin`. The source's M68k manual defines `-no-opt`
as disabling optimizations. A controlled run without it shortened the 0x3820
slice from 798 to 794 bytes; the first difference was the displacement byte at
`0x382B` (expected A6, actual A4). This is **one assembler workaround class**:
disable optimization. No opcode-specific overrides or emitted `dc.w` patches.
Explicit byte/word/long operations, short/word branches, absolute `.W/.L`, and
displacement syntax preserve encoding choices. `org` retains original addresses;
there are no alignment directives or inserted padding. MOVEM masks are converted
to register lists, reversing the encoded mask for predecrement.

## Scope, provenance and results

Canonical SHA-256:
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
All bounds are exclusive and every selected byte is a decoded instruction byte.

| Range | Instructions | Bytes | Exact result | First difference | Overrides |
|---|---:|---:|---|---|---:|
| 0x3820..0x3B3E | 306 | 798 | MATCH | none | 0 |
| 0x62CC..0x62E4 | 6 | 24 | MATCH | none | 0 |
| 0xA8DA..0xA8F0 | 10 | 22 | MATCH | none | 0 |
| 0x1108..0x1112 | 4 | 10 | MATCH | none | 0 |
| 0x2B6E..0x2B8A | 4 | 28 | MATCH | none | 0 |

5/5 exact routines, **330 instructions, 882/882 verified ASM bytes (100%)**,
zero selected unsupported forms, zero final mismatches, zero handwritten opcode
overrides. Manual engineering effort estimate: 2–3 hours; this is an estimate,
not measured human labor. Full 0x3820 coverage needed no representative truncation.

The frozen existing mass evidence marks 0x1108 and 0x2B6E MODERATE_STATIC,
BOUNDARY_AGREES, LEAF, with no failures, indirect flow or known data/overlap
conflicts. Fresh bounded decoder output confirms their full contiguous ranges
and RTS terminals. 0x1108 exercises indirect/displacement byte reads and a word
shift; 0x2B6E exercises absolute-long writes and long immediates. Meanings and
runtime evidence for both remain UNKNOWN. This does not promote them to
semantically understood native game routines.

0x62CC is STRONG_STATIC. The older mass snapshot labels A8DA MODERATE_STATIC
with `multiple_entry_overlap`; this is preserved, not erased. Its explicitly
selected bounds and four `(A5)+` writes were independently confirmed by M11.6.2
and this exact round-trip. A8DA/62CC natural runtime captures remain unavailable.
0x3820 has prior M3 vector evidence and 13 hits in M11.8; its mass boundary
heuristic miss does not supersede the established bounded code range.

## One decoder, shared exact operands

`DecodedInstruction` keeps address, opcode, raw bytes, flow and prior evidence.
Its existing `parse_ea` now also records `DecodedOperand`; the decoder's bounded
`re_slice_exact` normalization sets operation, data width, source/destination and
branch width. Emitters consume this representation without re-decoding EAs.
Register, indirect, postincrement, predecrement, displacement, absolute and
immediate forms are distinct kinds. Indexed forms remain outside this emitter.
Per-routine JSON retains raw words and exact typed operands; unknown normalized
forms fail closed. No second decoder or C++ emitter was created.

One existing decoder defect was exposed: SUBA.L Dn matched the broad SUBX
no-extension mask and lost its EA metadata. The mask now excludes address
arithmetic; its long EA width and MOVEM word width are retained. Synthetic tests
cover these cases, postincrement, immediate/operand sizes, zero displacement,
short/word branches, deterministic golden ASM, unsupported/gapped slices and
first differences including both EOF directions.

## Local reconstructed layout

```text
reconstructed_poc/
    main.asm
    manifest.json
    result.json
    code/sub_*.asm, sub_*.json, sub_*.bin
    blobs/*.bin
    layout.bin
```

Only `[0x1108,0xA8F0)` is reconstructed: **38,888 bytes**, including **38,006**
unknown bytes in four gaps. The manifest describes ordered offsets and provenance;
the runner verifies ROM identity, extracts the gaps from that local ROM, assembles
all slices and the layout, and compares bytes directly. `main.asm` includes the
five ASM files and exact blobs in address order. All remaining ROM bytes are
outside the experiment. No commercial ROM, generated ASM/IR, slices, extracted
data, rebuilt layout, emulator state or assembler binaries are committed.

The deliberately corrupted local A8DA test reports:

```text
FIRST_DIFFERENCE rom_offset=0x00A8E3 slice_offset=0x9 expected=0xC2 actual=0xC3 instruction=0x00A8E2 move.w D2,(A5)+
```

Exactly one next recommendation: **A — expand to 25–50 verified routines**.
That expansion is not implemented in this checkpoint.
