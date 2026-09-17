# M12 FLOW_V1 exact ROM range linkage 2B

**Result: `PASS_FLOW_V1_EXACT_ROM_RANGE_LINKAGE`**

The accepted BizHawk/GPGX runtime completed one real 16-Worker campaign with
100 cycles per Worker. All 1,600 FLOW_V1 segments were host-audited, projected
to exact canonical ROM instruction ranges, persisted in MAP-1, and independently
reconciled against every raw instruction record and terminal `next_pc` fact.
`SOURCE_OWNED` remained unchanged at 0 bytes.

## Startup recovery

The initial `STOP_EMUHAWK_BASELINE_STARTUP` diagnosis was based on the wrong
installation. Its probes used
`C:\Dev\SegaThorTools\BizHawk-live-forward-worker-1b`, not the accepted 2A
install at `build/thor-evidence/live-forward-worker-1b/coherent-bizhawk`. The
first probes therefore did not establish a failure in the accepted 2A runtime.

The accepted install contains 488 files. Its EmuHawk SHA-256 is
`0830DE4306ADEB5DC0906555C5B2A7DEC709302C54BF484F2DD1918AD04DADBE`; its
`BizHawk.Emulation.Cores.dll` SHA-256 is
`ED6B1D2FE597EE25E5BF17752C38CA8EE9F0A4BCF381AD8762BDACA0048736C2`; and its
active raw `gpgx.wbx` SHA-256 is
`4AC692A115CB5543BB3C2260FC04FDACD2CF5D963DF59C17D87A12A30ADD3CD4`. The
old compressed WBX is preserved there under an inactive backup filename. The
canonical USA ROM is 3,145,728 bytes with SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

The prior `C:\Dev` copy had a different Cores DLL
(`588A07D313A45C12D48BD604F388DD9D7ADA456202BEC60B9B25BD4EF18FB154`) and an
active `gpgx.wbx.zst` with SHA-256
`23A05F32CEB790F21AC7550E388401861E7A5333731AFE69A351E86D916D5845`. A
matched direct-launch pair held that prior install's EmuHawk, Cores DLL, raw
WBX, ROM, working directory and launch mode constant. With the compressed
sidecar active, the process stayed responsive but created no window in 15
seconds. Renaming only that sidecar to an inactive preserved name made the ROM
load and display the Beyond Oasis BizHawk window in about 2 seconds. This
isolates the failing WBX selection to the stale sidecar in that wrong install.

The accepted install passed a fresh EmuHawk no-argument startup and a direct
ROM-only startup without Lua or a `--config` argument; the displayed title was
`Beyond Oasis (U) [!] [Genesis] - BizHawk (interim)`. Direct launches had no
stdout/stderr redirection and created a fresh config. The unchanged 2A Python
launcher also passed with its isolated config and redirected output, emitting
and auditing one FLOW segment. Stock BizHawk 2.11.1/GPGX separately loaded the
same canonical ROM. No WBX rebuild was needed.

## Runtime and ROM projection

The recovered run used the accepted coherent install, `WORKER_COUNT=16`,
`CYCLES_PER_WORKER=100`, depth 20, and 65,536 bytes per Worker. Run ID was
`1789714283`. Each Worker completed 100 cycles with 100 distinct capture IDs
and generations and all four lifecycle transition counters at 100. The runtime
host audit accepted 1,600/1,600 segments. The emulator advanced 1,389 frames
across 100 host-audited rounds; the shared CPU stream supplied the independently
reread instruction records.

The 2B projection linked all 199,630 captured instruction occurrences to
canonical `ROM_INSTRUCTION_RANGE` objects. It produced 330 unique ranges and
1,244 unique executed ROM bytes, with 704,234 occurrence-bytes observed across
overlapping captures. No instruction remained unresolved or had an unsupported
decode. All 1,600 segment terminal `next_pc` values were retained as
`OBSERVED_NEXT_PC` address facts; the target instruction is not asserted as
captured by that relation.

One audited Worker 0 chain from capture `1789714283000001` demonstrates both
extended encodings and range deduplication:

| FLOW PC | Next PC | Canonical range | Length | Exact bytes | Range identity |
| --- | --- | --- | ---: | --- | --- |
| `0x32EE` | `0x32F4` | `[0x32EE,0x32F4)` | 6 | `4A 79 00 FF 16 58` | `56252f828111a9462239d2d14522fc8b58d73faa5bb24b18c7ac4b5af0a5a364` |
| `0x32F4` | `0x32EE` | `[0x32F4,0x32F6)` | 2 | `66 F8` | `97aefac4364733f21b65d72c2ead17669f2f05cbe40ed0d66f2e833c782ebbf0` |
| `0x32EE` | `0x32F4` | `[0x32EE,0x32F4)` | 6 | `4A 79 00 FF 16 58` | same identity as first row |

The repeated `0x32EE` occurrence points to the same canonical range object.
The 6-byte instruction contains two extension words. The corpus also includes
4-, 8-, and 10-byte instructions.

## Independent full-result audit

The independent auditor now checks every saved instruction lineage against its
segment identity and raw FLOW record, independently decodes every captured PC,
checks each exact byte slice against the canonical ROM, validates stable MAP-1
range identities, and reconciles every terminal address fact. It rejects
duplicate or missing segment/capture/generation identities and rejects a
corrupted claim that the earlier sample-only auditor would have skipped. It
also reconciles every exported range row and aggregate metric with the audited
SQLite graph, rejecting altered compact exports.

Windows and WSL/Linux audits both returned
`PASS_INDEPENDENT_ROM_RANGE_AUDIT` with matching results (apart from the
expected platform-specific hash of the audit decoder executable):

| Audit measure | Result |
| --- | ---: |
| Segments audited | 1,600 |
| Instruction occurrences inspected / ROM-linked | 199,630 / 199,630 |
| Non-ROM / unresolved occurrences | 0 / 0 |
| Unique ROM instruction ranges | 330 |
| Unique executed ROM bytes | 1,244 |
| 2-byte instructions | 106,539 |
| Extended instructions (>2 bytes) | 93,091 |
| Instructions with multiple extension bytes (6+ bytes) | 39,855 |
| Terminal `OBSERVED_NEXT_PC` facts audited | 1,600 |
| Duplicate capture IDs / Worker generations | 0 / 0 |
| Identity conflicts / opcode mismatches / unsupported decodes | 0 / 0 / 0 |
| `SOURCE_OWNED` delta | 0 bytes |

A representative terminal record stores `next_pc=0x21B2` as a
`M68K_TARGET_ADDRESS`; it does not claim that the target instruction was in the
capture window.

## Scope and validation

The runtime capture path, Worker scheduling/lifecycle, FLOW_V1 format, 1B
scaling semantics, production AUTO67, predecessor logic, Cartographer,
Archivist, and source-ownership logic were not changed. The only post-runtime
code adjustment was to make the 2B acceptance auditor verify every saved claim
and terminal fact, with a regression test for corrupted formerly unsampled
lineage.

Debug and Release builds passed, with full CTest at 204/204 in each
configuration. After the final narrow audit edits, the focused
`live_forward_rom_link` CTest passed 1/1 in both configurations. GNU/Linux WSL
built and linked the native decoder; its decoder self-test, Python fixture, and
full saved-result audit passed. Python compilation, the project file-limit
check, and `git diff --check` passed. The auditor is 458 lines and its test is
268 lines, both within the 500-line limit.

Full startup receipts, runtime logs, FLOW records, SQLite session, and
platform-specific independent audit outputs remain local under the ignored
`build/thor-evidence/live-forward-rom-link-2b/` directory. Only the compact
report and JSON receipt are intended for version control.
