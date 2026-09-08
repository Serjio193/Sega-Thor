# M11.31 — Comparative Disassembly Method Transfer

**Status:** COMPLETE — no high-confidence cross-project template; retain
localized, evidence-led Beyond Oasis methods.

**Baseline:** current `main` at `7a3a1a7` (M11.30).

**Scope:** public Streets of Rage 2/3 disassembly, extraction and static
recompilation projects were inspected as method references. No ROM, asset,
IDA database, generated assembly, or external project source was copied into
this repository. The comparison is bounded to repository structure, pinned
history, documented workflows, and the existing Beyond Oasis evidence ledger.

## Sources and exact revisions

| Project | URL and inspected revision | What it demonstrates | Exact ROM rebuild |
|---|---|---|---|
| `gsaurus/sor-disassemblies` | [4dd719f3ed5c24c86deee616b56cea885e3113d7](https://github.com/gsaurus/sor-disassemblies/tree/4dd719f3ed5c24c86deee616b56cea885e3113d7) | IDA databases and generated assembly for Streets of Rage 1/2/3 and Bare Knuckle 3 | **Not demonstrated**; no build/reassembly script or byte-identity claim |
| `gsaurus/sor_pancakes` | [96cef9b21d7db45acdf3e38dc2cc025b91585086](https://github.com/gsaurus/sor_pancakes/tree/96cef9b21d7db45acdf3e38dc2cc025b91585086) | Megadrive hacking, decompression, palette and level-editor tools | **Not demonstrated**; extraction/editing tools are not a ROM reassembler |
| `RuiNelson/StreetsOfRageProject` | [498708538fceafcf275bf44f508ac2c1d1f808de](https://github.com/RuiNelson/StreetsOfRageProject/tree/498708538fceafcf275bf44f508ac2c1d1f808de) | ROM-driven decompilation/recompilation workflow and emulated hardware shell | **Not demonstrated**; generated C++ is a static recompiler, not an assembler rebuild |
| `RuiNelson/RageDecompiler` | [9d58a51e53d2a3305d4f875296966da08d7389b0](https://github.com/RuiNelson/RageDecompiler/tree/9d58a51e53d2a3305d4f875296966da08d7389b0) | Bounded disassembly, map maintenance, speculative entry discovery, C++ emission | **Not demonstrated** |
| `RuiNelson/StreetsOfRageRecompilation` | [dc578f5e2296c3870acb7e877e278ac816931aa5](https://github.com/RuiNelson/StreetsOfRageRecompilation/tree/dc578f5e2296c3870acb7e877e278ac816931aa5) | Generated native functions plus hand-written native helpers and tests | **Not demonstrated** |

The meta-project's checked submodule pins were also recorded during the
inspection: `MegaDriveEnvironment` `03768fab63a7d8cba0bdafafeaed0b42f5113d5c`
and `MegaDriveEnvironmentSampleGame`
`6fd44871cf5bc614e13920e7c471e7fee5b20da7`. The recompiler submodule's local
revision above is the exact inspected checkout; the meta repository page also
contains an older `e4bd828` gitlink. This distinction is retained so a future
comparison does not silently mix moving submodule states.

## Phase 1 — method inventory

### `gsaurus/sor-disassemblies`

The repository is organized as `Assemblies/` and `IDA Pro Databases/`. The
assembly files are generated IDA-style text, not maintained source. The SoR2
file has RAM `equ` symbols, vector data, `loc_` and `sub_` labels, and large
`dc.b/dc.w/dc.l` regions. A bounded count of the SoR2 file found 251 `equ`,
8,320 `loc_`, 4,012 `sub_`, 95,570 `dc.b`, 18,733 `dc.w`, and 7,036 `dc.l`
lines; it contained no `macro`, `include`, or `org` directives. SoR3 has the
same generated style at a much larger scale. This preserves addresses and
gives useful labels/xrefs, but does not provide a code/data ownership model
that can be rebuilt by a linker.

RAM symbols are explicit, graphics and other binary regions are represented as
data directives/comments, and sound appears as labels and comments. There is
no project-specific extraction pipeline, macro layer, unknown-state schema, or
exact rebuild command. The seven-commit history is a short import/organization
history, not a documented iterative discovery process. Classification:
`SPECIALIZED_ONLY` — strong static annotation reference, weak method template.

### `gsaurus/sor_pancakes`

The layout (`MDV`, `RGBGen`, `Sor2_LevelEditor`, `pk2`, `pk3`, `por`) is a
collection of Java/NetBeans hacking and data-editing tools. Its history shows
palette, decompression and level-editor work. It is useful evidence that
graphics/data extraction can be isolated from CPU reconstruction, but it does
not provide a disassembler map, a CPU code/data boundary, or a complete ROM
reassembly. Classification: `SPECIALIZED_ONLY`.

### `RuiNelson/StreetsOfRageProject` and its subprojects

The README states the user's legally owned ROM is supplied locally. The
workflow starts at the reset/vector region (`0x200`), follows direct-call
closure, tracks unresolved indirect entries in `aux_addresses.txt`, emits
generated C++, and runs it inside a Mega Drive environment. The documented
loop is exercise the game, record missing addresses, recompile, and repeat.
That is a useful bounded evidence loop, but the output is a native recompiler,
not source that can be assembled back to the original bytes.

`RageDecompiler` separates a one-byte-per-ROM-byte map from labels and
generated source. `X` marks unknown/data map bytes; branch/jump labels and
known code segments are separate. `aux_addresses.txt` records entries that
static control flow cannot resolve, while unsupported instructions fail rather
than silently becoming stubs. Its commands include `disassemble`,
`recompile`, `speculative-scan`, `remove-data`, `label-diff`, `map-label-gaps`,
and `iterative-disasm`. Generated C++ is grouped by entry-address prefix; the
common header supplies helper casts/macros and the recompiler keeps a
developer CPU/memory model.

`StreetsOfRageRecompilation` keeps generated code, `code-analysis/`,
`ai-analysis/`, tests and small hand-written files separate. `addresses.csv`
contains hardware/RAM labels, `labels.csv` contains code-segment labels, and
`manual_functions.txt` records generated declarations whose bodies are
hand-written. Native helpers cover controls, decompression, interactions,
menus and selected sound entry points while the generated sound engine remains
present. The project-authored sound analysis describes a 68000 FM/PSG
sequencer and Z80 DAC/PCM path; that is useful architecture evidence for this
project only, not proof of shared lineage with Beyond Oasis.

Classification: `PARTIAL_TRANSFERABILITY` as a process template. Its map,
unknown/auxiliary-entry discipline, bounded iterative discovery, and generated
versus hand-written separation transfer. Its recompiler CPU model, speculative
whole-game loop, AI-assisted analysis, and SoR-specific hardware/object/audio
assumptions do not transfer directly.

## Phase 2 — bounded structural similarity matrix

The matrix compares observable organization and workflow. It does not infer
common authorship or shared binary modules from common Mega Drive idioms.

| Dimension | Streets of Rage evidence | Beyond Oasis evidence already recorded | Result |
|---|---|---|---|
| CPU/code style | 68000 code with vectors, `MOVEM`, branches, tables and indirect entries | 68000 decoder/CFG evidence, vectors, direct and indirect edges, exact bounded translations | `NON-DISCRIMINATING`; these are platform idioms |
| Program organization | reset/startup, VBlank, game modes, object/state tables and hardware helpers | startup/system transition, screen dispatch, entity pools, event/state owners and bounded graphics paths | `PARTIAL`; the investigation order transfers, formats do not |
| Binary/data organization | IDA `dc.*` output and map labels; SoR project has an explicit byte map and labels | canonical-ROM identity, bounded decoder, Atlas, tables and explicit unknown/conflict categories | `PARTIAL` method similarity, no binary-layout claim |
| RAM symbols | generated `equ` labels and `addresses.csv` | bounded raw RAM fields and address-backed native views | `PARTIAL`; preserve raw offsets until evidence promotes semantics |
| Graphics/data extraction | Pancakes tools and recompiler decompressors/native helpers | `0x3820`, resource ID 3, screen/terrain and native decompressor evidence | `PARTIAL`; decompression/resource isolation transfers |
| Sound handling | generated 68000 engine plus native helpers; project analysis says Z80 DAC/PCM | Beyond Oasis sound lineage and driver contract remain unproven | `SPECIALIZED_ONLY` |
| Unknown representation | IDA unknown/raw directives; SoR map `X`, `aux_addresses`, unsupported-stop behavior | `UNKNOWN`, unresolved categories, runtime identity binding, fail-closed promotion | `PARTIAL`; the explicit negative state transfers well |
| Exact reassembly | no inspected project demonstrates byte-identical rebuild | canonical ROM is an oracle, not a reassembled output | `NON-DISCRIMINATING`; no public exact rebuild baseline was found |

No dimension supports a claim that Streets of Rage and Beyond Oasis share
programmer style, module boundaries, or sound-driver lineage. The strongest
similarity is methodological: address-preserving evidence, explicit unknowns,
and incremental validation.

## Phase 3 — transferable sequence

The following sequence is transferable as a checklist, with the stated
Beyond Oasis adaptation:

1. Recover vectors and startup ownership from the canonical ROM.
2. Identify interrupt/VBlank ownership and record timing-sensitive boundaries.
3. Build a raw RAM symbol table with address, width, writer/reader evidence,
   and confidence; do not import SoR names or offsets.
4. Recover the main/state dispatch and one bounded object/entity table.
5. Separate graphics loaders, decompression, resource tables and VDP-facing
   effects from CPU-only routines.
6. Use direct CFG closure first; record indirect/jump-table entries in a
   separate unresolved ledger.
7. Bind natural runtime evidence to canonical ROM identity and promote only
   ranges with exact decoder and effect evidence.
8. Keep generated/mechanical output, hand-written native helpers and reports
   visibly separate.
9. Treat sound as its own investigation with an explicit driver contract.
10. Repeat only the bounded loop needed to close one evidence gap; do not turn
    it into a whole-game coverage or speculative ranking system.

The sequence maps directly to Beyond Oasis work already completed: canonical
USA identity (M2), the `0x3820` decompressor (M3), screens/terrain (M7),
entities (M9), event/spirits evidence (M10/M11), resource ID 3 (M11.24), and
hybrid shadow/override contracts (M11.28–M11.30). It therefore adds method
discipline rather than a new discovery engine.

The following does **not** transfer: SoR labels, SoR RAM offsets, object
record formats, VDP command assumptions, sound-driver names, generated C++
basic-block structure, or a CPU emulation dependency in production.

## Phase 4 — transfer ranking

| Candidate | Rank | Reason |
|---|---|---|
| `RuiNelson/StreetsOfRageProject` + `RageDecompiler` | `PARTIAL_TRANSFERABILITY` | Best bounded workflow template: map, direct closure, aux entries, explicit unsupported/unknown state, iterative natural evidence, generated/native separation. No exact rebuild and several SoR-specific dependencies. |
| `gsaurus/sor-disassemblies` | `SPECIALIZED_ONLY` | Valuable address/label/IDA reference for SoR2/3, but generated text and IDA databases have no demonstrated reassembly path. |
| `gsaurus/sor_pancakes` | `SPECIALIZED_ONLY` | Useful extraction/decompression/level-editing techniques; no CPU reconstruction or exact ROM build. |
| Sound-driver comparison | `SPECIALIZED_ONLY` | The public SoR sound description is project-specific; Ancient Music Driver lineage for Beyond Oasis is not proven by these repositories. |
| Programmer-style similarity | `NON-DISCRIMINATING` | Shared 68000/Mega Drive idioms are insufficient evidence of common programmer or code lineage. |

No `HIGH` candidate was found. The correct transfer is the evidence workflow,
not a source tree, disassembly naming scheme, or recompiler runtime.

## Phase 5 — Beyond Oasis dry-run

The existing `0x2D66` hybrid target is the bounded dry-run because its
contract is already the strongest natural proof. Applying the selected SoR
workflow in order produced this traceability chain:

| Step | Existing Beyond Oasis evidence | Dry-run result |
|---|---|---|
| Canonical seed | USA ROM identity and natural 600-frame scenario | Reused; no new scenario or ROM input |
| Static closure | exact ten-instruction range, direct caller `0x2D58`, RTS return | Reused; range remains bounded |
| Raw state contract | D0–D7/A0–A7, full SR, A6/A7, bounded `0xFF134C` writes | Reused; no SoR fields imported |
| Natural evidence | one natural `0x2D66` entry and shadow comparison | Reused; zero recorded divergence |
| Promotion gate | native replacement only after exact effects and return transition | Reused; the M11.29 override proof remains valid |
| Negative boundary | M11.30 VDP/sound serialized-state divergence | Preserved; no timing contract is invented |

This dry-run confirms that the method order is useful and compatible with the
existing ledger. It produces no new structural claim and no measurable
implementation speed-up, so no migration or new code is justified by this
comparison.

## Phase 6 — separated conclusions

* **PROGRAMMER_STYLE_SIMILARITY:** `NON-DISCRIMINATING`. Public source layout
  and common 68000 idioms do not establish shared authorship or style.
* **BINARY_STRUCTURE_SIMILARITY:** `PARTIAL` at the console-convention level
  (vectors, interrupts, tables, VDP/audio regions); no address/layout/module
  correspondence is established.
* **RE_METHOD_TRANSFERABILITY:** `PARTIAL_TRANSFERABILITY`. Transfer the
  bounded map/unknown/auxiliary-entry/evidence loop and generated/native
  separation. Keep Beyond Oasis decoders, addresses, contracts and ROM oracle.
* **SOUND_DRIVER_LINEAGE:** `SPECIALIZED_ONLY` / unresolved. Do not infer an
  Ancient Music Driver relationship from the Streets of Rage repositories or
  their project-authored sound notes.

## Phase 7 — next-step decision

No high-confidence disassembly template was found. M11.32 should therefore be
a bounded Beyond Oasis pass using the adapted sequence: vectors/interrupts,
raw RAM symbols, one main/state owner, one object/resource path, exact decoder
and natural evidence, then evidence-backed promotion. It must exclude
SoR-specific generated C++, basic-block recompilation, sound-lineage claims,
exact-ROM assembler claims, broad coverage ranking, and production emulator
dependencies. Sound remains a separate decision with its own contract.

**Decision:** `RE_METHOD_TRANSFERABILITY=PARTIAL_TRANSFERABILITY`;
`PROGRAMMER_STYLE_SIMILARITY=NON_DISCRIMINATING`;
`BINARY_STRUCTURE_SIMILARITY=PARTIAL`;
`SOUND_DRIVER_LINEAGE=SPECIALIZED_ONLY`.

No ROM, asset, emulator binary, IDA database, generated assembly, or generated
run evidence is tracked.
