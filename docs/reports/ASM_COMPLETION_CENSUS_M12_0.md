# M12.0 — Full ASM reconstruction completion census

Status: M12_0_CENSUS_COMPLETE_ASM_COMPLETION_BLOCKED.

This report rebases the project from the former C++-first M12 proposal to the
ordered sequence ROM -> complete reassemblable ASM -> rebuilt-ROM runtime parity
-> systematic ASM-to-portable-C++ migration. It is an evidence census, not a
new code or native-game implementation. All ROM-derived artifacts referenced
here are local and ignored.

## Authoritative baseline

| Item | Value |
| --- | --- |
| Repository baseline | 37c6bc1695771cb74839f48b05d40aa348ba7874 |
| Canonical USA ROM | external Beyond Oasis (USA).md |
| ROM size | 3,145,728 bytes (0x300000) |
| ROM CRC32 | C4728225 |
| ROM SHA-1 | 2944910c07c02eace98c17d78d07bef7859d386a |
| ROM SHA-256 | eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263 |
| Existing runtime checkpoint | 251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d |
| Existing runtime video | 5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58 |
| Byte-ownership baseline | local build/m11-15/audit4/manifest.json |
| Data evidence baseline | local build/m11-17/structured_data/classification_report.json |
| Runtime priority evidence | local build/m11-20-gpgx-priority.json |

The M11 checkpoint and runtime identities remain preserved. M11 native
mechanical primitives, TableCopyRoutine, RamFlagRoutine, ParentSuffix and the
developer-only hybrid/recompiler infrastructure are not invalidated by this
rebase.

## Exact completion gates

### ASM_CODE_COMPLETE

Every executable 68000 range and every executable Z80 range is represented as
assembler/source. No executable ROM-backed/raw binary blob and no executable
unknown hidden through incbin remains. Indirect CFG uncertainty may remain
documented, but the bytes and instructions must still be represented
faithfully in ASM. Unsupported or unknown opcodes remain visible and
byte-exact, for example as raw dc.w where necessary.

### ASM_ROM_MAP_COMPLETE

Every ROM byte has exactly one classified source region. The allowed classes
are CODE, STRUCTURED_DATA, ASSET_PAYLOAD, HEADER/VECTORS,
PADDING/ALIGNMENT and VERIFIED_UNKNOWN_DATA. The map has no gaps or overlaps.

### ASM_REASSEMBLY_BYTE_EXACT

The reconstruction emits exactly 3,145,728 bytes with CRC32 C4728225,
SHA-1 2944910c07c02eace98c17d78d07bef7859d386a and SHA-256
eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263.

### ASM_REBUILT_ROM_BOOT_PROVEN

This is a later M13 gate. M12.0 does not claim boot or runtime parity for a
rebuilt ROM.

These gates intentionally separate assembler completeness from semantic
understanding. A table may be semantically unknown while still being exact ASM
data; conversely, a byte-exact local blob is not ASM completion.

## Current full-ROM census

The first table is the current reassembly materialization. It answers what the
existing exact rebuild actually emits. UNCLASSIFIED includes every non-ASM byte
still supplied as a canonical-ROM-derived blob, even where M11.17 has a
separate bounded data hypothesis.

| Category | Ranges | Bytes | Percent | Current evidence |
| --- | ---: | ---: | ---: | --- |
| 68000_ASM | 203 | 13,550 | 0.430742900% | exact slice round trips; 197 ASM_ROUNDTRIP_EXACT, 5 CODE_STATIC_SUPPORTED, 1 CODE_EXECUTED |
| Z80_ASM | 0 | 0 | 0% | no Z80 source or accepted Z80 ROM ownership map |
| STRUCTURED_DATA_ASM | 0 | 0 | 0% | no structured data is emitted from ASM/source in the exact layout |
| HEADER_VECTOR_ASM | 0 | 0 | 0% | vectors/header are not yet source-owned in the rebuild |
| PADDING_ALIGNMENT | 0 | 0 | 0% | no padding/alignment regions are explicitly classified in the complete map |
| LOCAL_ROM_DERIVED_ASSET | 0 | 0 | 0% | no accepted asset payload source region in the exact layout |
| ROM_BACKED_UNKNOWN_CODE | 0 proven | 0 proven | 0% proven | candidate code exists inside unclassified blobs, but no whole range has been promoted as unknown code |
| ROM_BACKED_UNKNOWN_DATA | 0 proven | 0 proven | 0% proven | data/asset candidates remain conservative and are not asserted as data ownership |
| UNCLASSIFIED | 136 | 3,132,178 | 99.569257100% | canonical_local_rom blob entries in the M11.15 exact manifest |
| OVERLAP | 0 | 0 | 0% | manifest quality gate |
| GAP | 0 | 0 | 0% | manifest quality gate |
| **Total** | **339** | **3,145,728** | **100%** | contiguous [0x000000,0x300000) |

The second table is the independent bounded evidence classification. It must
not be confused with source ownership:

| Evidence class | Ranges | Bytes | Percent | Status |
| --- | ---: | ---: | ---: | --- |
| exact 68000 ASM ownership | 203 | 13,550 | 0.430742900% | accepted |
| bounded structured data | 10 | 1,164 | 0.037002563% | 1 DATA_REGION_SUPPORTED (256) plus 9 DATA_STRUCTURE_SUPPORTED (908) |
| remaining unknown/unclassified | — | 3,131,014 | 99.532254537% | no semantic class asserted |
| overlap/conflict | 0 | 0 | 0% | accepted data classifier found no code conflicts |

The 1,164 data bytes are still blob-backed in the current exact rebuild. They
therefore do not reduce the UNCLASSIFIED materialization total until an
explicit source representation is added and verified.

## Executable or possibly-executable blob census

The exact machine-readable inventory is the CSV block below. It contains all
136 remaining blob ranges from the audited manifest. The fields
current_representation, cpu, decoder_support, cfg_issue, reason_not_promoted
and recommended_method have the defaults shown here; the per-range columns
override the evidence and priority values.

~~~text
schema,oasis.asm-completion-blob-census.v1
source,build/m11-15/audit4/manifest.json
current_representation,canonical_local_rom blob
cpu,UNKNOWN_68000_OR_Z80_OR_DATA
decoder_support,not_whole-range-audited; bounded decoder evidence only
cfg_issue,range ownership and indirect/static CFG closure unresolved
reason_not_promoted,not represented as normal assembler in current exact manifest
recommended_method,CODE-01 bounded decode/CFG plus transactional exact ASM promotion; run ROM-01/ROM-02 first where data ambiguity exists
dynamic_execution_count,unavailable in current per-range artifact
columns,start,end,size,priority,observed_pc_count,static_xref_count
000000,0007C4,1988,P0,23,3
0007E2,0008A2,192,P3,0,0
0008F8,000D5E,1126,P3,0,0
000D82,000E80,254,P3,0,0
000EC2,000F32,112,P3,0,0
000F7E,001108,394,P3,0,0
001182,00129A,280,P3,0,0
001300,001780,1152,P3,0,0
001828,001D30,1288,P3,0,0
001D5A,001DDC,130,P3,0,0
001F72,00297C,2570,P3,0,0
002992,002ADE,332,P3,0,0
002B0A,002B6E,100,P3,0,0
002BBC,002CBC,256,P3,0,0
002D58,002D66,14,P3,0,0
002D84,002DB6,50,P3,0,0
002E1E,002E78,90,P3,0,0
002EE2,0032E8,1030,P3,0,0
0032F8,00350E,534,P3,0,0
003534,0035AC,120,P3,0,0
00361E,00365C,62,P3,0,0
0036A8,00372A,130,P3,0,0
00376A,003820,182,P3,0,0
003B3E,004A92,3924,P0,12,3
004AD0,0062CC,6140,P3,0,0
0062E4,0064C4,480,P3,0,0
006516,0083D4,7870,P0,25,0
0083F8,008504,268,P3,0,0
008530,0085C4,148,P3,0,0
0085E2,008778,406,P3,0,0
0087F4,008910,284,P3,0,0
0089B2,008B8E,476,P3,0,0
008BAC,008CAC,256,P3,0,0
008E22,008E32,16,P3,0,0
008E90,0094A2,1554,P0,9,15
0094D2,0099B8,1254,P0,11,4
0099D6,009BF2,540,P3,0,0
00A196,00A342,428,P3,0,0
00A438,00A8DA,1186,P3,0,0
00A8F0,00B730,3648,P3,0,0
00B79A,00B852,184,P3,0,0
00B922,00B9DE,188,P3,0,0
00BA9C,00C038,1436,P3,0,0
00C0BE,00C17A,188,P3,0,0
00C222,00C66A,1096,P3,0,0
00C756,00C838,226,P3,0,0
00C84E,00C88C,62,P3,0,0
00C8AA,00C90E,100,P3,0,0
00C92C,00C9B2,134,P3,0,0
00C9EC,00CECC,1248,P3,0,0
00CEEA,00D344,1114,P3,0,0
00D406,00D7B0,938,P0,13,1
00D7C0,00D962,418,P3,0,0
00DA1A,00DA2A,16,P3,0,0
00DA62,00DA72,16,P3,0,0
00DAC2,00DB1A,88,P3,0,0
00DB64,00DBCA,102,P3,0,0
00DE00,00E338,1336,P0,31,87
00E45E,00E4D0,114,P3,0,0
00E502,00E5AE,172,P3,0,0
00E6BA,00F234,2938,P3,0,0
00F258,00F6E2,1162,P3,0,0
00F788,010F84,6140,P3,0,0
010F94,0111E0,588,P3,0,0
011276,0112CC,86,P3,0,0
0112F6,011310,26,P3,0,0
011378,011384,12,P3,0,0
0113CC,0119C8,1532,P3,0,0
0119CE,013764,7574,P3,0,0
01378A,01386E,228,P3,0,0
01387C,014126,2218,P3,0,0
014200,01571A,5402,P3,0,0
01573C,015C4A,1294,P3,0,0
015CB6,016A46,3472,P3,0,0
016A64,0178F2,3726,P3,0,0
017908,018030,1832,P3,0,0
018072,0191A2,4400,P3,0,0
0191CE,01932A,348,P3,0,0
01933E,0193E0,162,P3,0,0
019414,01960C,504,P3,0,0
019632,0196D2,160,P3,0,0
019708,01A21E,2838,P3,0,0
01A23C,01A244,8,P3,0,0
01A29C,01CB16,10362,P3,0,0
01CB4C,01CD92,582,P3,0,0
01CDA6,01DFA4,4606,P3,0,0
01DFC2,01DFCA,8,P3,0,0
01E064,01E074,16,P3,0,0
01E098,01F678,5600,P3,0,0
01F6B8,01FB2C,1140,P3,0,0
01FBB4,01FBCC,24,P3,0,0
01FC02,01FC20,30,P3,0,0
01FC98,020708,2672,P3,0,0
02072E,020CD0,1442,P3,0,0
020CDE,021B92,3764,P3,0,0
021BC0,021BE8,40,P3,0,0
021C1A,0220B0,1174,P3,0,0
0220C2,02233C,634,P3,0,0
022368,02305C,3316,P3,0,0
0230A4,025BB2,11022,P3,0,0
025BD0,025BD8,8,P3,0,0
025BFE,026ACE,3792,P3,0,0
026AE2,026E82,928,P3,0,0
026E98,027D90,3832,P3,0,0
027DBA,02839A,1504,P3,0,0
0283DE,0287C4,998,P3,0,0
028808,028878,112,P3,0,0
0288A2,02926C,2506,P3,0,0
029304,0293EC,232,P3,0,0
029452,02A3B6,3940,P3,0,0
02A3E0,02C670,8848,P3,0,0
02C692,02EDCA,10040,P3,0,0
02EDE6,02EEA0,186,P3,0,0
02EEC4,0302F4,5168,P3,0,0
030324,032020,7420,P3,0,0
03206E,039566,29944,P3,0,0
03957C,03A510,3988,P3,0,0
03A56C,03C040,6868,P3,0,0
03C074,03C1E4,368,P3,0,0
03C1E6,03C454,622,P3,0,0
03C480,03C584,260,P3,0,0
03C5B6,03C75E,424,P3,0,0
03C79C,03C85C,192,P3,0,0
03C876,03C956,224,P3,0,0
03C9CC,03CE68,1180,P3,0,0
03CE98,03E2BC,5156,P3,0,0
03E2FC,03E3A8,172,P3,0,0
03E430,03E436,6,P3,0,0
03E492,03E498,6,P3,0,0
03E4F4,03E79C,680,P3,0,0
03E7F4,060286,137874,P3,0,0
06042A,0611F4,3530,P0,76,77
061232,06138E,348,P3,0,0
0613F8,06143A,66,P3,0,0
06147E,062D4C,6350,P3,0,0
062D6C,300000,2740884,P3,0,0
~~~

observed_pc_count is the unique-PC count from the bounded
build/m11-20-gpgx-priority.json evidence, not a dynamic instruction execution
count. The current evidence does not provide a per-blob execution count. P0
means that a blob range contains observed execution evidence and therefore
must be split and audited before ASM completion; it does not mean that every
byte in the coarse range is code. P3 means data/asset/code ambiguity remains
and no promotion claim is made.

The P0 set has 8 ranges and 22,394 coarse blob bytes:

| Range | Size | Observed PCs | Static xrefs | Priority reason |
| --- | ---: | ---: | ---: | --- |
| 0x000000..0x0007C4 | 1,988 | 23 | 3 | top priority score 99; observed code-like PCs plus vector/header ambiguity |
| 0x003B3E..0x004A92 | 3,924 | 12 | 3 | observed code-like PCs inside raw blob |
| 0x006516..0x0083D4 | 7,870 | 25 | 0 | largest P0 coarse blob; observed code-like PCs |
| 0x008E90..0x0094A2 | 1,554 | 9 | 15 | observed PCs with static corroboration |
| 0x0094D2..0x0099B8 | 1,254 | 11 | 4 | observed PCs with candidate/static overlap |
| 0x00D406..0x00D7B0 | 938 | 13 | 1 | observed producer/reader region |
| 0x00DE00..0x00E338 | 1,336 | 31 | 87 | high incoming-xref count and observed PCs |
| 0x06042A..0x0611F4 | 3,530 | 76 | 77 | highest observed-PC concentration; existing M11 parent/callee evidence |

No range is claimed as a complete executable function. The coarse blobs can
contain data, code, and unresolved boundaries simultaneously.

## Reassembly dependency audit

~~~text
canonical USA ROM
  -> re_assemble_run.py / re_full_split_run.py
  -> generated exact ASM slices + local gap/blob includes
  -> vasm flat-binary assembly of full_layout.asm
  -> byte comparison and hash gate
  -> rebuilt.rom
~~~

| Dependency | Classification | Exact current role |
| --- | --- | --- |
| canonical ROM used by extractor | TEMPORARY_RE_DEPENDENCY | seeds the local corpus and verifies input identity |
| canonical ROM-backed slices | FINAL_BUILD_DEPENDENCY | 136 blob includes supply 3,132,178 bytes today |
| generated ASM includes | REQUIRED_LOCAL_ASSET_SOURCE | 203 exact code artifacts are generated locally and ignored |
| extracted commercial graphics/audio payloads | REQUIRED_LOCAL_ASSET_SOURCE | none currently accepted into the exact layout; keep local-only |
| pre-existing generated blobs | TEMPORARY_RE_DEPENDENCY | output of prior local runs; not repository source |
| post-build patching | SHOULD_BE_ELIMINATED | none observed |
| checksum/header correction | SHOULD_BE_ELIMINATED | none; header/checksum bytes currently arrive through blobs |

The current exact reconstruction therefore still depends on the original ROM
at build time. The end-state for executable code is that the canonical ROM is
not required as the source of executable bytes. Data/assets may remain local
generated inputs where repository policy prohibits committing copyrighted
payloads.

## Toolchain

The current proof uses local vasmm68k_mot, vasm 1.8g, M68k backend 2.3f,
Motorola syntax 3.13 and binary output 1.8a. Exact flags are:

~~~text
-m68000 -no-opt -Fbin
~~~

This is a flat-binary assembler proof; no separate linker is used. Existing
evidence verifies labels/symbols, org, incbin, relative branch expressions,
exact instruction encoding and deterministic output. no-opt is required
because optimization changes a known displacement encoding. Local generated
assets can be included with incbin. Alignment has not yet been needed in the
exact corpus; it must be tested when a source-owned region requires it.
Unsupported forms remain fail-closed; a raw dc.w fallback is allowed by the
completion gate but was not used in the current promoted set. Ancient's
historical assembler remains UNKNOWN; no historical preference is being used.

## Intended source organization

No mass move is made in M12.0. The intended neutral tree is:

~~~text
asm/
  main.asm
  include/
  m68k/
  z80/
  data/
  generated/
  unknown/
~~~

Address-based filenames and neutral directory names remain preferred until
ownership and subsystem semantics are proven. assets/ is a local ignored build
output, not a repository payload directory. Gameplay names such as inventory,
UI, save, audio or entities are not assigned to unknown regions.

## Native C++ migration status

Existing M11 work remains preserved and tested:

- mechanical primitive layer;
- TableCopyRoutine;
- RamFlagRoutine;
- parent-owned ParentSuffix;
- developer-only hybrid/recompiler infrastructure.

The project migration state is now:

~~~text
CPP_MIGRATION = PAUSED_PENDING_ASM_COMPLETION
~~~

No new native routine is added in M12.0. The previous proposal
“M12 Inventory/UI/Save” is moved to future subsystem tracks inside the
reconstruction/migration phases.

## Blockers and single M12.1 selection

### Blockers to ASM_CODE_COMPLETE

1. 136 canonical-ROM blob ranges remain outside normal assembler ownership.
2. Eight of those ranges contain bounded observed execution evidence (P0);
   their coarse boundaries must be split before byte ownership can be claimed.
3. The remaining 128 ranges are P3 code/data/asset-ambiguous regions.
4. No Z80 executable map/source is established.
5. Indirect CFG and unsupported-opcode evidence must be represented visibly in
   ASM rather than left in opaque blobs.

### Blockers to ASM_REASSEMBLY_BYTE_EXACT

The byte-exact gate is currently proven for the blob-backed split, but not for
a source-complete reconstruction. The final source-owned gate is blocked by
the 3,132,178 blob bytes, the 1,164 structured-data bytes that are still
materialized as blobs, absent header/vector source ownership, and the absence
of a complete padding/asset/unknown-data map. Existing split exactness must be
rerun after each transactional promotion.

### Exactly one M12.1 target

0x06042A..0x0611F4 — promote and exact-round-trip this single 3,530-byte P0
blob after a bounded CODE-01 decode/CFG and data-conflict check. It has the
highest concentration of observed PCs in the current coarse blob census
(76 unique PCs, 77 static xrefs), and it is connected to the documented M11
parent/callee evidence. This is a code-completion target only; it does not
authorize native C++ extraction or semantic naming.

M12.1 is not started by this report.

## Verification record

Existing local evidence used for this census:

- M11.15 audited 203 ASM ranges: 13,550 exact ASM bytes, all range round trips
  exact, and full-ROM split exact.
- M11.17 accepted 10 bounded data hypotheses: 256 header-region bytes and
  908 structured-table/descriptor bytes, with zero conflicts.
- The exact rebuilt artifact is 3,145,728 bytes with CRC32 C4728225, SHA-1
  2944910c07c02eace98c17d78d07bef7859d386a and the canonical SHA-256.
- The manifest is contiguous with 0 gaps and 0 overlaps.

M12.0 verification commands and their current results are recorded in the
M12.0 worklog entry. A newly generated full-ROM artifact, if produced during
final validation, remains local and ignored.
