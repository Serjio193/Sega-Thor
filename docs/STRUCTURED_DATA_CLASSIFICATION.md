# M11.17 — Structured data classification PoC

Status: `STRUCTURED_DATA_HIGH_VALUE`.

The developer-only classifier reads the local canonical USA ROM, proves each
bounded range against its exact bytes and ROM SHA-256, and emits the ignored
report `build/m11-17/structured_data/classification_report.json`. No ROM,
payload, emulator sweep, production runtime change, code promotion or C++
generator was added.

The accepted structures are deliberately small and evidence-backed:

| Range | Bytes | Width | Count | Classification | Consumer/reference |
| --- | ---: | ---: | ---: | --- | --- |
| `0x000000..0x000100` | 256 | 4 | 64 | `DATA_STRUCTURE_SUPPORTED` | 68000 reset/exception vector fetch |
| `0x000100..0x000200` | 256 | 1 | 256 | `DATA_REGION_SUPPORTED` | ROM identity/header parser |
| `0x0096E8..0x0096F8` | 16 | 1 | 16 | `DATA_STRUCTURE_SUPPORTED` | `0x9D00`/`0x938E` terrain lookup |
| `0x0096F8..0x009708` | 16 | 1 | 16 | `DATA_STRUCTURE_SUPPORTED` | `0x9AD6` behavior lookup |
| `0x00C92C..0x00C980` | 84 | 4 | 21 | `DATA_STRUCTURE_SUPPORTED` | `0xC8F0`, `load_screen_descriptor()` |
| `0x05CE96..0x05D046` | 432 | 4 | 108 | `DATA_STRUCTURE_SUPPORTED` | `0xD3B2` indexed reader and `0x3820` |
| `0x05CE16..0x05CE56` | 64 | 1 | 64 | `DATA_STRUCTURE_SUPPORTED` | `0xF61C` masked byte index and bit-7 test |
| `0x05CE56..0x05CE96` | 64 | 1 | 64 | `DATA_STRUCTURE_SUPPORTED` | `0x7A6C` selector-indexed enum lookup, values 0..4 |
| `0x15A9A6..0x15A9B0` | 10 | 2 | 5 | `DATA_STRUCTURE_SUPPORTED` | `0x3EFA` bounded selector and relative `ADDA.W` targets |
| `0x15BAC2..0x15C238` | 1910 | 1 | 1 | `DATA_STRUCTURE_SUPPORTED` | `0x4966` and `0x3820` graphics decoder |
| `0x15C238..0x15CA9B` | 2147 | 1 | 1 | `DATA_STRUCTURE_SUPPORTED` | `0x4974` and `0x3820` graphics decoder |
| `0x15CA9C..0x15CEA0` | 1028 | 1 | 1 | `DATA_STRUCTURE_SUPPORTED` | `0x4982` and `0x3820` graphics decoder |
| `0x15B9D4..0x15BAC2` | 238 | 2/6 | 4 streams | `DATA_STRUCTURE_SUPPORTED` | `0x4AF2`, `0x4B08`, `0x4B18`, shared parser `0xB730` |
| `0x02CF82..0x02CF9C` | 26 | 26 | 1 | `DATA_STRUCTURE_SUPPORTED` | screen ID `0x0009`, `0xC8F0` |
| `0x02D3E8..0x02D402` | 26 | 26 | 1 | `DATA_STRUCTURE_SUPPORTED` | screen ID `0x000C`, `0xC8F0` |
| `0x032144..0x03215E` | 26 | 26 | 1 | `DATA_STRUCTURE_SUPPORTED` | screen ID `0x0704`, `0xC8F0` |
| `0x03285C..0x032876` | 26 | 26 | 1 | `DATA_STRUCTURE_SUPPORTED` | screen ID `0x0705`, `0xC8F0` |

The vector parser consumes all 64 longwords and checks the reset PC. The two
terrain tables match every documented value. The group and resource tables
parse every big-endian pointer; all targets are inside the ROM, and the
resource table preserves its null entry. The four descriptors parse their
long pointer, four resource IDs, four signed parameters and five trailing
words. The header is intentionally the weaker region classification because
its field-level semantics are not all proven. Compressed payload boundaries
after `0x5CE96` remain unresolved and are not classified.

Before accepting a range, the classifier intersects it with every M11.15 code
range (`ASM_ROUNDTRIP_EXACT`, `CODE_STATIC_SUPPORTED` and `CODE_EXECUTED`). A
collision becomes an explicit `CONFLICT` record and never chooses a winner.
This pass found 0 conflicts, 0 rejected candidates and 10 accepted ranges.

Promotion safety is integrated into `re_auto_promote.py`: an explicit
`DATA_REGION_SUPPORTED` or `DATA_STRUCTURE_SUPPORTED` overlap vetoes a future
code candidate. Weak or unknown data hypotheses do not veto. The gate is
covered by helper tests alongside invalid pointer-like data, code-like bytes,
overlap conflicts and a valid deterministic table.

| Metric | Before | After |
| --- | ---: | ---: |
| `TOTAL_ROM_BYTES` | 3,145,728 | 3,145,728 |
| `ASM_ROUNDTRIP_BYTES` | 13,550 | 13,550 |
| `CODE_STATIC_SUPPORTED_BYTES` | 1,006 | 1,006 |
| `CODE_EXECUTED_BYTES` | 24 | 24 |
| `DATA_REGION_SUPPORTED_BYTES` | 0 | 256 |
| `DATA_STRUCTURE_SUPPORTED_BYTES` | 0 | 908 |
| `UNKNOWN_BYTES` | 3,132,178 | 3,131,014 |
| `CONFLICT_BYTES` | 0 | 0 |
| `data_ranges_attempted` | 0 | 10 |
| `accepted` | 0 | 10 |
| `rejected` | 0 | 0 |
| `conflicts` | 0 | 0 |

The full-ROM representation remains blob-backed and byte exact: CRC32
`C4728225`, SHA-1 `2944910c07c02eace98c17d78d07bef7859d386a`, SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

Validation passed for the standalone Python tests, MSVC Debug/Release builds
and CTest, Linux CMake/CTest, source-limit and `git diff --check`; CI and
artifact hygiene remain green. MinGW is unavailable on this host. The WSL
mounted-workspace source-limit invocation timed out; the Windows equivalent
passed.

The single next recommendation is **A — return to the native vertical slice**.
