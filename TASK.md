# Current Task

TASK: M11.5 — USA retail versus USA beta bounded opcode correspondence
WHY: Compare the five requested known retail targets against the user-supplied
USA Beta 1994-11-01 ROM without semantic inference or runtime changes.
CURRENT MILESTONE: M11.5
MILESTONE UNDERSTANDING CONFIDENCE: 95%
CURRENT SLICE UNDERSTANDING CONFIDENCE: 95%
SLICE MODE: RE_TOOLING_ONLY
STATUS: COMPLETE

## Scope and method

`oasis_re_diff` loads both binaries through `oasis::Rom::load`. It analyzes
bounded windows for `0x3820`, `0x60004`, `0x82AE`, `0x7A28` and `0xA6A4`, then
compares decoded instruction bytes, normalized opcode signatures and CFG block
topology. Normalization retains decoder opcode-family/addressing/flow shape and
operand widths while omitting relocation-sensitive branch and extension values.
`exact_match` requires raw decoded bytes plus CFG shape; `structural_match`
requires normalized signatures plus CFG shape; `changed_blocks` means CFG shape
is shared but aligned block bytes differ; otherwise the comparison is
`unmatched`. Analog search is bounded by the target window and reports only
candidate entries passing the same conservative comparison. The beta scan is
limited to even ROM offsets for these five signatures; each candidate decode
uses only the selected target window and this is not general routine discovery.

## Acceptance criteria

- [x] fingerprint the user-supplied beta ROM without committing ROM bytes;
- [x] compare exactly the five requested retail targets;
- [x] emit deterministic `oasis.m68k.re-diff.v1` JSON and human report;
- [x] report exact, structural, changed-block and unmatched categories;
- [x] include normalized opcode signatures and candidate beta addresses;
- [x] preserve unknown/unsupported decoder evidence without semantic names;
- [x] add synthetic exact/structural/changed classification tests;
- [x] add a local retail/beta fingerprint and correspondence oracle;
- [x] keep the tool developer-only and separate from `oasis_core` gameplay;
- [x] pass Debug/Release build, CTest, file-limit and `git diff --check`.

## Verified result

Beta fingerprint: size `3145728`, CRC32 `FA59F847`, SHA-1
`cb0606faeab0398244d4721d71cf7e1c5724a9ef`, SHA-256
`5111d21c8344cce00765b32b971849f62950d31869307cc479f5ee7febf87a80`;
Mega Drive header and Sega checksum are valid. The report has 5 targets.
Retail `0x3820` has an exact analogue at beta `0x37D0`; `0x60004` is exact at
the same address; `0x82AE` has an exact analogue at beta `0x825E`; `0x7A28`
has an exact analogue at beta `0x79D8`; `0xA6A4` has a structural analogue at
beta `0xA654` with one raw changed block (ordinal 10). Same-address comparisons
are unmatched for every moved target and exact for `0x60004`.

## Known unknowns and hard boundaries

These results establish byte/decoder/CFG correspondence only. They do not
prove routine semantics, call contracts, function identity beyond the reported
evidence, or equivalence of unanalysed bytes. The decoder remains bounded and
unsupported opcodes/ambiguous control flow remain visible in the underlying
reports. No production behavior, emulator, whole-ROM recompiler or M12 work
was added.

## Exact next action

Stop at this verified comparison checkpoint and await explicit instruction.
Do not start dynamic tracing expansion, similarity search beyond these targets,
whole-ROM discovery, recompilation or M12 automatically.
