# Historical Mega Drive Toolchain & SDK Evidence Guide

This document is mandatory guidance for AI-assisted reverse engineering tasks that use historical Sega Mega Drive/Genesis development tools, SDK documentation, assembler/linker behavior, compiler fingerprints, preserved source trees, or development-kit artifacts as evidence.

It does **not** establish which toolchain Ancient used for Beyond Oasis / The Story of Thor. That remains UNKNOWN until project-specific evidence proves it.

## Purpose

Historical development material can accelerate reconstruction by separating likely platform/toolchain boilerplate from Ancient-specific game logic and by explaining code layout, object/link behavior, padding, relocation patterns, startup code, and generated helper sequences.

The goal is not to reproduce a 1994 development environment for its own sake. The goal is to extract bounded, reproducible evidence that improves the ROM Atlas and native translation confidence.

## Non-negotiable boundary

Historical toolchains are **evidence sources only**.

They must never:

- become a production runtime dependency;
- replace reverse engineering of game-specific behavior;
- justify semantic names without ROM/runtime evidence;
- cause proprietary SDK binaries, commercial ROMs, leaked private material, or copyrighted source trees to be committed;
- cause Codex to search for or download stolen/private development archives;
- be treated as proof that Ancient used a tool merely because Sega or another Genesis developer used it.

Public manuals, public preservation documentation, public interviews, public source releases, and user-supplied local tools may be analyzed. If licensing or provenance is unclear, keep binaries local and record only derived factual observations that are necessary for RE.

## Research result: historical candidate toolchains

The following are confirmed historical Mega Drive/Genesis development candidates. They are candidates for fingerprinting, **not confirmed Ancient tools**.

### 1. Microtec 68000 toolchain

Confidence for Sega ecosystem use: **CONFIRMED**.
Confidence for Beyond Oasis: **UNKNOWN**.

An official Sega Genesis Technical Bulletin dated 1991 discusses errors in Microtec examples and explicitly references:

- `asm68k`;
- `LNK68K`;
- `TESTC68K.bat`;
- a 68000 C compiler option.

The bulletin gives a corrected linker command using `LNK68K`, proving that Sega distributed or supported a Microtec-based assembler/linker/C example workflow during the early Genesis development period.

RE value:

- assembler syntax and accepted instruction forms;
- object/link segmentation behavior;
- linker layout and padding;
- possible startup/C-runtime patterns;
- map/list artifact conventions if surviving tools/examples are available.

Do not infer that a 1994 Ancient title used Microtec merely because Sega supported it in 1991.

### 2. SNASM68K / Cross Products / SN Systems lineage

Confidence for Genesis ecosystem use: **CONFIRMED**.
Confidence for Beyond Oasis: **UNKNOWN**.

A 1993 Genesis programming resource list describes Cross Products SNASM68K as a PC-hosted Genesis development system containing a 68000 assembler, linker, and debugger.

Preserved SNASM68K manuals describe a linking assembler capable of producing object/COFF output plus map/list files and target download workflows. Later preserved commercial Genesis development material also shows real projects depending on SNASM68K-specific environments.

RE value:

- map/list output structure;
- object/COFF behavior;
- source-level symbol conventions;
- assembler optimization/encoding choices;
- alignment/padding behavior;
- development-kit/debugger conventions.

A matching opcode sequence alone is not proof of SNASM use because most ordinary 68000 instructions assemble identically across tools.

### 3. Psy-Q / ASM68K-era tools

Confidence for historical Sega development use: **CONFIRMED as ecosystem material**.
Confidence for Beyond Oasis: **UNKNOWN**.

Preserved Mega Drive development-tool documentation catalogs Psy-Q-era DOS tools including `ASM68K.EXE`, Z80 tooling, linker/C support, and development-hardware integration.

Modern public Mega Drive assembly samples are commonly made compatible with both `ASM68K.EXE` and `SNASM68K.EXE`; this is useful for building controlled comparison corpora but is not evidence about Ancient's original environment.

RE value:

- alternative encoding/padding fingerprints;
- source directive behavior;
- map/list differences;
- comparison against SNASM and Microtec output.

### 4. Sierra Systems 68000 C/assembler/linker

Confidence for Genesis ecosystem use: **CONFIRMED**.
Confidence for Beyond Oasis: **UNKNOWN**.

Contemporary Genesis developer-resource material lists the Sierra Systems PC-hosted 68000 C cross-compiler, assembler, and linker, and records recommendations to use Sierra tools with some Genesis development hardware.

RE value is highest if a ROM region exhibits compiler-generated patterns rather than hand-written assembly.

No current project evidence proves that Beyond Oasis gameplay code was compiled from C. Treat that as UNKNOWN.

## Ancient-specific evidence boundary

Current public research did **not** find reliable evidence identifying the assembler, linker, compiler, or source-control system used by Ancient for Beyond Oasis.

Therefore Codex must use wording such as:

- `candidate toolchain`;
- `output-compatible with`;
- `structural fingerprint match`;
- `toolchain hypothesis`;

until stronger evidence exists.

Never write:

- `Beyond Oasis was built with SNASM68K`;
- `Ancient used Microtec`;
- `this is Sega SDK code`;

unless project-specific evidence supports the statement.

## Project-specific clue: Ancient-owned sound layer

The USA game data contains identifiable Ancient Music Driver strings for both 68000 and Z80 portions. This is strong evidence that at least part of the audio software is Ancient-specific rather than generic Sega SDK code.

Practical consequence: do **not** classify an audio routine as SDK/library boilerplate solely because it resembles another Mega Drive driver. Compare exact strings, call topology, binary signatures, tables, and cross-build evidence first.

## Evidence hierarchy for toolchain claims

Use the following order from strongest to weakest.

### Level A — direct project artifact

Examples:

- original Beyond Oasis source/build script naming a tool;
- original `.map`, `.lst`, `.obj`, `.cof`, linker command file, or build log;
- embedded build/tool version string demonstrably belonging to the game;
- developer statement specifically naming the toolchain for this project.

A valid Level A artifact can support **CONFIRMED**, subject to provenance checks.

### Level B — distinctive binary fingerprint plus independent support

Examples:

- multiple unusual assembler/linker quirks reproduced exactly by one candidate;
- characteristic padding/relocation/layout behavior across several unrelated ROM functions;
- the fingerprint is also compatible with period/project evidence.

May support **LIKELY**, or **CONFIRMED** only when the evidence is highly distinctive and independently corroborated.

### Level C — ordinary opcode/layout similarity

Examples:

- the same `JSR`, `BSR`, `MOVEA`, `LEA`, or branch encoding;
- generic 68000 prologue/epilogue shape;
- common alignment to even addresses.

This is only **HYPOTHESIS** evidence. Most assemblers emit the same machine code for ordinary instructions.

### Level D — ecosystem association

Examples:

- Sega supported the tool;
- another Genesis title used the tool;
- the tool was popular in 1994.

This establishes historical plausibility only. It does not identify the Beyond Oasis toolchain.

## Required Codex workflow for historical toolchain tasks

### Step 1 — define a bounded question

Good examples:

- Does one known boot block contain a distinctive toolchain-generated sequence?
- Can a candidate assembler reproduce an unusual instruction/padding pattern at a known address?
- Does a known library binary have an exact or relocation-only analogue in the ROM?
- Can map/list conventions explain a suspected function boundary?

Bad examples:

- Identify the compiler for the whole ROM.
- Rebuild the entire game with an old SDK.
- Label every common routine as SDK code.

### Step 2 — inventory only approved evidence

Allowed sources:

- canonical USA ROM supplied locally by the user;
- project Beta/prototype builds already approved as local evidence;
- public technical manuals and bulletins;
- public preservation documentation;
- public/open-source disassemblies and source releases;
- user-supplied local historical tool executables.

Do not fetch proprietary/leaked SDK packages merely because a web page says they exist.

### Step 3 — record exact provenance

For every external artifact used in a conclusion record:

- artifact/tool name;
- version if known;
- source/provenance URL or local-user-supplied status;
- license/provenance limitation;
- whether the artifact is stored in the repository (normally NO);
- what exact observation was derived from it.

### Step 4 — build a controlled fingerprint corpus

If a user-supplied candidate assembler/compiler is available locally, generate tiny deterministic samples designed to distinguish tools.

Useful 68000 cases include:

- direct `JSR`/`JMP` absolute forms;
- `BSR.W` and short/word branch selection around boundary distances;
- PC-relative `LEA` and indexed addressing;
- `MOVEA` immediate/absolute variants;
- forward references;
- `DC.B`/`DC.W`/`DC.L` around alignment boundaries;
- `EVEN`/alignment directives;
- local labels and long-distance branches;
- jump tables;
- multiple sections/modules if the tool supports them;
- relocatable references if object/link mode is available.

Keep samples deliberately small. They are probes, not a new software project.

### Step 5 — retain the right features

For fingerprinting, preserve features that may distinguish a tool:

- selected opcode form;
- effective-address mode;
- branch width choice;
- inserted padding bytes;
- section ordering;
- symbol/address ordering;
- relocation placement/type;
- linker fill behavior;
- map/list ordering;
- object format metadata where legally inspectable.

Do not over-weight literal absolute addresses because they commonly change with linking.

### Step 6 — normalize relocations conservatively

When comparing a candidate output with ROM code, distinguish:

- exact bytes;
- relocation-only difference;
- immediate/address-only difference;
- same instruction topology with different constants;
- structural-only similarity;
- mismatch.

Never mask an opcode, addressing mode, register choice, condition code, or branch topology merely to improve a match score.

### Step 7 — compare against known Atlas entries first

Start with regions whose boundaries and roles are already evidenced, for example:

- reset/startup area beginning from ROM reset PC `0x20E`;
- verified decompressor `0x3820..0x3B3E`;
- verified bounded function `0xD3B2..0xD406` where useful;
- bounded RE target around `0x60004` only when the experiment directly relates to it.

Do not scan the entire ROM until a bounded fingerprint has demonstrated useful discriminatory power.

### Step 8 — separate toolchain identity from game semantics

A probable assembler/library match may establish:

- probable generated boilerplate;
- likely library boundary;
- likely linker padding/data boundary;
- a stronger function start/end hypothesis.

It does **not** establish gameplay meaning.

Example:

`This block matches a candidate runtime helper` does not justify renaming it `InitializePlayer`.

### Step 9 — combine independent evidence

The most useful result combines two or more of:

- static ROM fingerprint;
- cross-build retail/prototype correspondence;
- emulator execution evidence;
- known strings/tables;
- exact public library/source correspondence;
- historical tool output.

Toolchain evidence should raise confidence only when it agrees with existing evidence.

### Step 10 — stop when discriminatory value is low

If SNASM, Microtec, Psy-Q, and Sierra all generate equivalent machine code for the tested construct, record `non-discriminating` and stop expanding that probe.

Do not manufacture a winner from ordinary opcode equivalence.

## Recommended machine-readable report

Only implement this schema when a concrete fingerprinting checkpoint is explicitly authorized and the report will answer a real RE question.

Suggested schema name:

`oasis.m68k.re-toolchain-fingerprint.v1`

Suggested fields:

```text
metadata
  rom_fingerprint
  target_range
  candidate_tool
  candidate_version
  provenance
  experiment_id

observations[]
  rom_address
  rom_bytes_hash
  candidate_sample_id
  match_class
  matched_instruction_count
  differing_instruction_count
  relocation_only_fields
  padding_observations
  distinctive_features[]

assessment
  confidence
  conclusion
  alternative_candidates[]
  limitations[]
```

Recommended `match_class` values:

```text
exact
relocation_only
immediate_only
structural
non_discriminating
mismatch
```

Do not create a probability percentage unless there is a defined scoring model and synthetic validation for it.

## Candidate scoring discipline

Prefer explicit evidence counts over opaque scores.

Example:

```text
candidate: SNASM68K
exact distinctive matches: 2
relocation-only matches: 4
non-discriminating matches: 11
mismatches: 1
confidence: HYPOTHESIS
reason: no project-specific artifact identifies the assembler
```

This is better than an unjustified `SNASM probability = 87%`.

## Library/source fingerprinting

If a public historical source tree or library is legally available, do not copy it into the project by default.

Instead:

1. build or inspect it outside the repository;
2. record source/version/provenance;
3. compute bounded signatures;
4. compare signatures/topology against ROM ranges;
5. store only small non-copyrightable metadata/signatures needed for reproducibility when appropriate;
6. cite the public source in documentation;
7. preserve semantic names only if the correspondence is strong enough to justify them.

Exact long binary/code reproduction from third-party commercial source should not be committed merely to make matching easier.

## Emulator interaction

The external MAME/BizHawk oracle can improve toolchain investigations by answering whether a candidate region is actually executed and with what inputs.

Use dynamic evidence to prioritize or validate a fingerprint, not to identify an assembler by itself.

Example workflow:

```text
ROM Atlas candidate block
        +
MAME/BizHawk executed evidence
        +
historical assembler fingerprint
        +
retail/prototype correspondence
        -> stronger boundary/library hypothesis
```

## Prototype/build genealogy interaction

When additional approved prototype builds are available locally, compare a suspected toolchain/library block across builds.

A block that is:

- exact across builds;
- relocated as a unit;
- independently present in period library/source material;

is a stronger library/boilerplate candidate than a gameplay block that changes substantially between builds.

Do not assume unchanged means SDK; stable Ancient-owned libraries are also possible.

## High-value project applications

Toolchain evidence is most likely to help with:

1. reset/startup and interrupt boilerplate;
2. low-level VDP/DMA/controller helpers;
3. generic memory/copy/initialization helpers;
4. linker padding and code/data boundaries;
5. C-runtime fingerprints if any compiler-generated region is discovered;
6. sound-driver boundaries when compared carefully against the confirmed Ancient Music Driver identity;
7. distinguishing repeated library code from game-specific logic.

It is less likely to directly explain:

- entity AI;
- combat rules;
- script semantics;
- puzzle/progression logic;
- spirit behavior;
- map/event meaning.

Those still require game-specific static/dynamic evidence.

## Hard stops

Stop and report before proceeding if:

- the only next step is downloading a proprietary or leaked SDK/tool binary;
- provenance is too unclear to use responsibly;
- the experiment requires committing copyrighted historical source/binaries;
- a candidate match is non-discriminating;
- the task is expanding into whole-ROM compiler identification;
- the task begins changing production C++ based only on a toolchain hypothesis;
- the task conflicts with the active milestone.

## Source references used to establish this guide

These references establish historical ecosystem facts, not Ancient-specific toolchain identity.

- Sega Genesis Software Manual / Technical Bulletin #1 (Microtec example corrections, `asm68k`, `LNK68K`, C compiler): https://fabiensanglard.net/another_world_polygons_Genesis/GenesisSoftwareManual.pdf
- SNASM68K 68000 Cross Assembler System Manual (preserved manual): https://segaretro.org/File:SNASM68K_68000_Cross_Assembler_System_Manual.pdf
- SNASM 68000 Development System User's Manual v2.01 (linking assembler, object/map/list behavior): https://segaretro.org/images/7/7e/SNASM268000DevelopmentSystem_User%27s_Manual_Ver2.01.pdf
- Contemporary 1993 Genesis developer resources (Sierra Systems and Cross Products SNASM68K): https://groups.google.com/g/rec.games.programmer/c/1hNO1BqPzuQ
- Exodus Mega Drive development-tools catalog (Psy-Q, SNASM2, Sierra, Microtec context): https://techdocs.exodusemulator.com/Console/SegaMegaDrive/Software.html
- Public Mega Drive assembly samples showing controlled ASM68K/SNASM-compatible build patterns: https://github.com/BigEvilCorporation/megadrive_samples
- Hidden Palace preservation write-up showing a commercial Genesis source tree with SNASM68K environment dependence, useful only as ecosystem evidence: https://hiddenpalace.org/News/Vanished_without_a_Trace_-_Out_of_the_Vortex_for_the_Sega_Mega_Drive

## Current project conclusion

As of this guide's creation:

```text
Microtec in Sega Genesis development ecosystem: CONFIRMED
SNASM68K in Genesis development ecosystem: CONFIRMED
Sierra 68000 tools in Genesis development ecosystem: CONFIRMED
Psy-Q/ASM68K-era tools in Genesis development ecosystem: CONFIRMED
Exact Beyond Oasis assembler/linker/compiler: UNKNOWN
Ancient-specific music-driver identity in ROM: CONFIRMED
Usefulness of toolchain fingerprinting for this ROM: HYPOTHESIS, testable
```

The next valid toolchain task should be a **small discriminatory fingerprint experiment**, not a broad SDK reconstruction project.
