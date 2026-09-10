# M12-AUTO — autonomous ASM reconstruction checkpoint

Status: `M12_AUTO_BLOCKED_GLOBAL_NO_FULL_ROM_DATA_PROVENANCE`.

This checkpoint advances the M12 source-owned map while preserving the
canonical ROM byte-for-byte. It does not start M13, native gameplay/runtime
migration, or emulator expansion.

## Identity and checkpoints

| Item | Value |
| --- | --- |
| Requested baseline | `33cb6d9985b3a87cd9eb92d4ad0883a736bde9ff` |
| Baseline materialization | M12.4 manifest, then M12.5 exact nested islands |
| Working checkpoint | `build/m12-auto-transaction-f/materialized/manifest.json` |
| Final commit SHA | recorded after validation and commit |
| `origin/main` SHA | recorded after push, or `CI=UNAVAILABLE` |

Checkpoint progression:

| Checkpoint | Source-owned bytes | Percentage | Result |
| --- | ---: | ---: | --- |
| M12.4 baseline | 16,478 | 0.523821513% | exact ROM start/vector/header/data split |
| M12.5 | 17,904 | 0.569152832% | 23 exact nested ASM islands |
| M12-AUTO | 279,468 | 8.884048461% | 181 static ASM islands, 107 resources, 47 padding bytes |

The M12.5 percentage above is an intermediate checkpoint; the authoritative
final percentage is recomputed from the final manifest below.

## Final ownership census

ROM size is 3,145,728 bytes. The source-owned total is 279,468 bytes
(8.884048461%). The remaining canonical-ROM-derived blob bytes are not counted
as source-owned.

| Class | Ranges | Bytes | Percent |
| --- | ---: | ---: | ---: |
| 68000 ASM | 425 | 40,714 | 1.294263204% |
| Z80 ASM | 0 | 0 | 0% |
| HEADER_VECTOR_ASM | 2 | 512 | 0.016276042% |
| STRUCTURED_DATA_ASM | 1 | 108 | 0.003433228% |
| PADDING_ALIGNMENT | 47 bytes of resource alignment | 47 | 0.001494090% |
| LOCAL_ROM_DERIVED_ASSET | 107 compressed resource streams | 238,087 | 7.568581899% |
| SOURCE-OWNED TOTAL | — | 279,468 | 8.884048461% |
| Remaining blob | — | 2,866,260 | 91.115951538% |

The asset class is deterministic and local-only: each stream is selected by
the canonical ROM's 108-entry pointer table, decoded by the existing verified
graphics decompressor, and bounded by its consumed source length. The 47
one-byte gaps before the next pointer are emitted as alignment, not asset
payload. No ROM or extracted commercial asset is committed.

## Executable and unresolved census

The broad bounded probe examined 253 Ghidra-derived function intervals. It
found 209 exact assembler round-trips; 188 had at least one static caller.
Overlapping nested candidates were resolved conservatively by retaining the
larger exact bounded interval when it covered the smaller one. The final
checkpoint promotes 181 non-overlapping caller-backed islands totaling 23,430
new ASM bytes beyond M12.5. Twenty-one exact islands without a caller remain
unpromoted because exact bytes alone do not prove executable ownership.

Explicit M12.4 debt remains 668 bytes: `POSSIBLE_CODE=560`,
`UNKNOWN_DATA=104`, and `UNRESOLVED_BOUNDARY=4`. These are evidence labels,
not additional ownership. The other 2,865,592 blob bytes have no safe global
subclassification yet; they remain `UNKNOWN` rather than being inflated with
meaningless `dc.b` or guessed assets.

Largest unresolved intervals in the final map are:

| Range | Bytes | Reason retained as blob |
| --- | ---: | --- |
| `0x062D6C..0x1AD000` | 1,352,340 | no closed code/data/resource boundary |
| `0x1E7236..0x300000` | 1,150,410 | post-resource payload has no authoritative resource graph |
| `0x03E7F4..0x060000` | 137,228 | mixed static code/data region |
| `0x03206E..0x039566` | 29,944 | pointer/data region intersects unresolved code candidates |
| `0x0230A4..0x025BB2` | 11,022 | no independently proven payload boundary |
| `0x02C692..0x02ED20` | 9,870 | screen-group spans conflict with code-island candidates |

The map has zero gaps and zero overlaps. `POSSIBLE_CODE`, `UNKNOWN_DATA`, and
`UNRESOLVED_BOUNDARY` are reported separately above; they are not silently
converted to source ownership.

## Methods and reusable tooling

| Method | Region | Result | Bytes unlocked |
| --- | --- | --- | ---: |
| bounded exact M68000 decode + vasm round-trip | Ghidra islands inside M12.5+ | POSITIVE | 23,430 ASM |
| static caller gate and overlap resolver | full candidate queue | POSITIVE | safe selection of 181 islands |
| pointer-table resource graph | `0x05CE96..0x05D046` | POSITIVE | 238,087 asset |
| graphics decompressor boundary scan | 107 resource indices | POSITIVE | 47 alignment bytes additionally classified |
| screen-group pointer spans | `0x00C92C..0x00C980` | INCONCLUSIVE | 0; spans overlap unresolved code candidates |
| broad resource/code promotion outside proven graph | `0x062D6C..0x300000` | NEGATIVE / fail-closed | 0 |

Reusable developer-only tooling added:

- `src/tools/re_m12_auto_promote.py` combines exact code-island promotion with
  local-ROM resource extraction and full-ROM byte verification.
- `src/tools/re_resource_boundary_scan.cpp` validates pointer-table resource
  starts, decompressor consumption, next-pointer bounds, and alignment gaps.
- `tests/re_m12_auto_test.py` protects the overlap-resolution gate.

## Canonical exactness and validation

The materialized ROM is exactly 3,145,728 bytes with CRC32 `C4728225`, SHA-1
`2944910c07c02eace98c17d78d07bef7859d386a`, and SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

Completed in this checkpoint: Python helper tests, M12.5 regression tests,
resource boundary scanner build/run, exact range assembly, combined full-ROM
assembly/hash verification, and source-size checks for new source files.
Debug/Release CTest and final repository hygiene are recorded in the final
worklog entry after they run against the committed tree.

## Global blocker and path to 100%

The remaining 91.115951538% is not one safely classifiable region. It spans
large mixed code/data areas and post-resource payloads without a complete
pointer/resource graph. The available Ghidra function census is static-only;
the graphics decoder proves the 107 indexed streams but does not identify
their neighbors or the rest of the ROM. Promoting those bytes as assets or
code would violate the M12 evidence rules and could conceal executable code.

The next safe path is independent provenance for the largest remaining
intervals: a bounded pointer/resource graph or runtime/static consumer
evidence, then mixed-area splitting and exact ASM/data/resource promotion.
Once that evidence exists, the same transaction and audit tooling can
continue toward 90% and then executable 100%. No C++ migration is authorized
until the ASM/source map gate is actually closed.
