# M12 — Emulator sprite-table reconstruction

Status: `BOUNDED POSITIVE RUNTIME RESULT`: the existing BizHawk 2.11.1 F1
QuickSave loaded the canonical ROM and one controlled `P1 Right` produced the
first changed Ali-side SAT shadow record. The change was in source RAM
`0xFF13CC..0xFF13CF`; SAT VRAM at `0xD000` remained unchanged in that first
transition frame. The finite object/frame/animation grammar remains fail-closed.

## Targeted Ali transition — resumed F1 result

The manually created local state was found at
`C:\Dev\SegaThorTools\BizHawk-2.11.1-win-x64\Genesis\State\Beyond Oasis (U) [!].Genplus-gx.QuickSave1.State`.
It is 175,228 bytes, remains outside the repository, and was loaded by
`savestate.loadslot(1, true)` from BizHawk 2.11.1. The ROM opened by the exact
quoted command was `C:\Github\Sega-Thor\build\reference\Beyond Oasis (USA).bin`
with SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

After three no-input settling frames, State A was captured at emulator frame
`2120`, then exactly one `P1 Right` frame was injected. State B was the first
structurally different bounded state at frame `2121`. Selector, descriptor,
relative-root bytes, and the `0xD000` SAT VRAM snapshot were unchanged. The
first changed source range was `0xFF13CC..0xFF13CF`, from
`00 00 00 00` to `00 88 09 01`; the first changed byte is therefore
`0xFF13CD: 0x00 -> 0x88`.

The bounded bus-write callback reported `0xFF13CC` with callback PC `0xA374`
and value `0x00880901` during the transition. Existing BizHawk callback
semantics report the next fetch PC for this write; the exact ROM store is
`0xA372` (`MOVE.L D2,(A5)+`). Static context proves `0xA342` initializes A5
from `0xFF13CC`, `0xA354` selects ROM root `0xA43A`, and the `0xFF1858` test
can select alternate root `0xA482` at `0xA360`; `0xA364..0xA37E` copies six
bounded records under a `DBF D0` loop. These are proven local table roots for
the observed source writer, not semantic animation labels.

The transition produced no observed `0x0000B730` execution, no new SAT DMA
after State A, and no `0x3820` call. The capture did observe the bounded DMA
contract around the state (`0x000027EC`, source `0xFF13CC`, destination
`0xD000`, 64 words), while the SAT bytes themselves stayed equal between A
and B. Therefore this one experiment proves a first source-side transition,
not a rendered SAT change, resident-resource load, or finite frame sequence.
The upstream selector/descriptor and downstream relative-table contracts
remain the previously published structural chain; no frame-sequence grammar
is promoted from this single transition.

The local first-run JSON had a serialization-only counter bug (`b730_calls`
was initialized as a table); the bounded evidence above is taken from the
otherwise complete local record, and the developer-only probe now initializes
that counter numerically. No replay was rerun after this tooling repair.
Publication predecessor `ff5a389062b9d96413e468650a3613fd57e28dd4` passed
GitHub Actions CI run `34690039459` (Build, Test, Post Checkout, Complete).

## Historical setup attempt — superseded by resumed F1 state

Baseline was `f6feb888f2a527d0f5cf5425586dcbcc0c266cd6`. The exact previously
published installation is present at
`C:\Dev\SegaThorTools\BizHawk-2.11.1-win-x64\EmuHawk.exe`, with working
directory `C:\Dev\SegaThorTools\BizHawk-2.11.1-win-x64`. The canonical ROM is
present at `C:\Github\Sega-Thor\build\reference\Beyond Oasis (USA).bin` and
matches the recorded SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

The recovered exact argv was attempted once from that working directory:
`--chromeless --lua C:\Github\Sega-Thor\src\tools\re_bizhawk_m12_gfx_provenance.lua C:\Github\Sega-Thor\build\reference\Beyond Oasis (USA).bin`.
The process returned code 0 but emitted only `parsing command-line flags` and
created no capture. This is an invocation/runtime integration failure, not
evidence that BizHawk is absent. A separate GUI-only launch of the same
pre-existing executable produced a responsive `BizHawk` window, confirming
that the installation itself starts. It was stopped after the check.

Read-only state search found no BizHawk state in the checked installation,
including `Genesis\State`; no state was created or reused. The Computer Use
backend required for GUI input returned `Trusted RPC service is not
configured: sky`, so it could not be used to reach gameplay or create a local
state. No ROM/register/RAM forcing, replay expansion, or equivalent second
campaign was attempted.

Therefore there is no State A, State B, transition frame, first causal value,
writer PC, or new Ali table/root claim in this pass. The exact stop boundary is
`ALI_TRANSITION_SETUP_BLOCKED`: restore a usable BizHawk UI-control/save-state
path or authorize a materially different evidence class (controlled state
forcing or static consumer analysis). Existing positive selector/descriptor,
relative-child, `0x03B448 -> 0xB730`, RAM/SAT/DMA, and ROM-resource evidence is
unchanged and remains the only claimed runtime result.

## Publication validation

The focused M12 helper set passed 5/5 in both existing Debug and Release
CTest trees: runtime provenance, selector control, relative-table analysis,
indirect body dispatch, and the `0x03B092` tail contract. Direct Python
compilation and the three corresponding analyzer tests also passed. The
existing full Debug/Release CTest evidence from the published baseline is
preserved. A fresh Ubuntu 24.04 / GCC 13.3 WSL configure and Release build/link
completed successfully; its five focused M12 tests passed. The WSL
`project_file_line_limit` scan was stopped after its `/mnt/c` scan exceeded the
bounded wait, while the native `cmake -P tests/check_file_limits.cmake` scan
passed. `git diff --check` passed. No source structure changed, so
`docs/FILE_MAP.md` requires no update.

## Identity and exact BizHawk replay

The prior checkpoint workflow was recovered and replayed with BizHawk 2.11.1.
The installation is present at
`C:\Dev\SegaThorTools\BizHawk-2.11.1-win-x64\EmuHawk.exe`; its working
directory is the installation directory. The exact M12 launch used the
existing `re_bizhawk_m12_gfx_provenance.lua` workflow with `--chromeless`, the
natural-input scenario, and 1800 frames. The replay exited 0 and produced
`capture-current-exact.json`, 5,610 bytes, SHA-256
`D61150BD828DB22CA35A2E6C93A103BDC04ABE667883B94339FD05390924000B`.
It is byte-identical to both published `capture-a.json` and `capture-b.json`.

Canonical ROM identity is size `3,145,728`, CRC32 `C4728225`, SHA-1
`2944910c07c02eace98c17d78d07bef7859d386a`, SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
No ROM, emulator state, extracted asset, PNG, or SOURCE_OWNED transaction was
added.

## Capability and live evidence

`capabilities-current.json` reports BizHawk 2.11.1 with `M68K BUS`, `VRAM`,
`CRAM`, `VSRAM`, and `MD CART` domains. Register access, bus execution/read/
write callback APIs, and frame advancement are present. The probes emitted no
state writes.

The bounded live SAT probe is
`build/m12-gfx-runtime/sat-provenance-current-v5.json`:

| Edge | Result | Evidence |
| --- | --- | --- |
| VDP register 5 → SAT base | **PASS** | PC `0x00002AF4`, value `0x00008568`, frame 2 → VRAM `0x0000D000` |
| SAT base → hardware SAT bytes | **PASS** | 48 changed VRAM snapshots; standard 8-byte entries decode to valid linked chains up to 8 entries |
| SAT DMA producer PC | **PASS** | 512 bounded launches, PC `0x000027EC`, command `0x50000083`, destination `0x0000D000` |
| DMA source RAM | **PASS** | VDP source regs resolve to byte address `0x00FF13CC`; source bytes are captured at launch |
| RAM producer → SAT source bytes | **PASS, bounded** | body `0x0000B730`, stores `0xB752/0xB764/0xB76E/0xB77A`; BizHawk reports the post-store callback at `0xB754` for the first store |
| selector → ROM table → source record | **PASS, bounded** | `0x3B416..0x3B448`: `A0=0x3B8DE`, selector read at `0x3B41A`, table field at `0x3B422`, callback state `A0=0x3B95C` at `0x3B426` (`MOVEA.L (A0),A0`), then four ROM-backed starts at `0x3B448` |
| DMA payload → SAT snapshot | **PASS** | 43/43 same-frame comparisons match byte-for-byte for the captured DMA length |
| RAM/producer → ROM frame table | **UNRESOLVED** | BizHawk `on_bus_read` backward tracing is prohibitively slow in this environment; no ROM-table claim is made |

The global read trace is not required for the bounded first edge. The exact
address probe `src/tools/re_bizhawk_m12_targeted_reads.lua` watched only the
confirmed table-field, pointer, selector, and observed source-start addresses.
Its 1800-frame capture retained 8,780 events below a 16,384-event cap and
directly reads selector RAM `0xFFAFAE`: value `0` (733 events) maps to table
record 0 field `0x3B8EA`, while value `2` (473 events) maps to record 2 field
`0x3B90A`. Those fields return `0x3B95C` (734 events) and `0x3B982` (474
events); the pointer reads return `0x171832` (689 events) and `0x1742DC`
(469 events). The same bounded probe observes 26 distinct first-word source
starts in the `0xB730` body context, each with its following `+2/+4/+6`
reads. For selector 2, the secondary words at `0x1742DC` and `0x1742E2` are
read as `0x1C` (320 events) and `0x76` (602 events); the following observed
`B730` source starts are `0x1742F8` (160 first-word reads) and `0x174358`
(301 first-word reads). These are address/value observations only, not
semantic frame counts or descriptor interpretations. Capture SHA-256 is
`833E8157A9B51C1BB57B11E9CAE7AA8AFC3B7FC98157B2640019C0E90149513E`.
This is an observed source census, not a completeness claim; records 3–7 are
not selected by this scenario.
This proves direct bounded ROM reads, but not the semantic meaning of the
pointer chain or complete object/animation/frame enumeration.

Representative decoded live entries include frame 714 (`Y=260`, `X=285`,
tile attribute `0x8400`) and frame 746, where the chain has two linked entries.
These are hardware SAT observations, not semantic names for objects or
animations.

## Reconstruction tooling

`src/tools/m12_sprite_reconstruction.py` contains bounded, fail-closed
developer helpers for SAT fields/link chains, Genesis four-plane tiles, CRAM,
flips, and logical composition. `tests/m12_sprite_reconstruction_test.py`
passes. The helper deliberately does not infer object, frame, animation, or
ROM-table semantics.

`src/tools/m12_sprite_context_catalog.py` consumes the canonical ROM and the
bounded context capture and emits only table geometry, ordered PC evidence,
and ROM-address provenance. On the current capture it enumerates all eight
confirmed `0x10`-byte records and validates 12 frames containing the ordered
`0x3B41A → 0x3B422 → 0x3B426 → 0x3B448 → 0xB730` edge plus all four SAT
stores. The context stream itself reports four source starts; the paired
exact-address read capture expands the bounded observed census to 26 starts.
All remain semantically unresolved.
The generated catalog is
`build/m12-gfx-runtime/sprite-context-catalog-current.json`, SHA-256
`472082214FE195DD225C8224B2B68FE6E300B8D8B44540502DA5DC267C94233C`.
The catalog also validates pointer reads to `0x171832` and `0x1742DC`, plus
the selector-2 word observations at `0x1742DC`/`0x1742E2` and following
`B730` starts `0x1742F8`/`0x174358`.
It emits no ROM payload or decoded asset.

## Remaining M12 gates

The following remain `UNRESOLVED` and are not promoted to SOURCE_OWNED:

- shadow-SAT record grammar beyond the observed DMA payload;
- frame descriptor and animation selector/index chain;
- object/animation roots and complete static enumeration;
- ROM table/resource/palette linkage and all-frame extraction;
- full logical-frame reconstruction from ROM resources.

The next evidence boundary is a finite record grammar and independent
selector/continuation coverage. The payload-free call-context capture
`build/m12-gfx-runtime/b730-calls-current-v2.json` (SHA-256
`5CF7A6D708A1D252464893D61A8D0B764CD3407B8A5167518DA227D5073AE0CB`)
contains 15,870 bounded register-context events over 1,800 frames. At
`0x3B426` it independently observes `A0=0x3B95C` 689 times and
`A0=0x3B982` 469 times; at `0x3B428` those become `0x171832` and
`0x1742DC`, and at `0x3B448`/`0xB730` it observes the corresponding
`0x1742F8` and `0x174358` source starts. This corroborates the exact-read
probe without emitting ROM payload. The full object/animation/frame grammar
and completeness remain unresolved.

## Replay-expansion boundary

The final replay-only expansion pass added only statically confirmed selector
reader callback PCs and all four longword fields for each table record. BizHawk
accepted the invocation but emitted no new capture file (`NO_NEW_CAPTURE`), so
records `3–7` were not observed and this is not treated as negative runtime
evidence. Replay expansion is closed for this scenario. The published v8
capture above remains the positive runtime baseline; the next evidence class is
static consumer analysis around `0x03A9EE..0x03AA18` and `0x03B1D0`.

## Static selector and dispatch contract

The bounded static catalog
`src/tools/m12_static_sprite_dispatch_catalog.py` validates the canonical ROM
and emits no payload. The descriptor setup at `0x03A9EE..0x03AA0E` reads
selector RAM `0xFFAFAE`, applies a 16-byte row stride, loads all four row
longwords at offsets `0/4/8/12`, and calls `0x03B1D0`. Two selector-masked
indirect dispatch tables are closed:

- `0x03B8A6..0x03B8C6`: eight entries, mask `0x0007`, stride `4`, targets
  `0x3AAAE`, `0x3AB98`, `0x3AC16`, `0x3AC6E`, `0x3ACA8`, `0x3AD0C`,
  `0x3ADB4`, `0x3AAEE`.
- `0x03B8C2..0x03B8E2`: eight entries, mask `0x0007`, stride `4`, targets
  `0x3AAEE`, `0x3ABDA`, `0x3AC68`, `0x3AC92`, `0x3ACE4`, `0x3AD66`,
  `0x3AE74`, `0x3BA46`.

The tables overlap at `0x03B8C2..0x03B8C6`, and the second table overlaps the
descriptor table at `0x03B8DE..0x03B8E2`. A bounded static slice of `0x03B1D0`
observes direct calls to decompressor `0x00003820` at `0x03B236`, `0x03B28A`,
and `0x03B2FE`. This closes static
consumer structure, not live selector coverage or semantic object, animation,
frame, or sprite names. Catalog SHA-256 is
`79906506AE138318AAB9B8B0F1D581E5102B23CDCC2A2CCBF4CDC71A04728EC5`.
No generic emulator framework, M13 work, or ASM-to-C++ migration is in scope.

## Current static grammar result

The bounded static phase after the closed replay campaign is represented by
`src/tools/m12_selector_descriptor_grammar.py` and its CTest helper
`tests/m12_selector_descriptor_grammar_test.py`. The generated local,
payload-free catalog is
`build/m12-gfx-runtime/selector-descriptor-grammar-current.json`, SHA-256
`EF1055715FE4FD72A7125A899D796AB6437EAAC243C78A9094200E418288799F`.

Result: `STATIC_FINITE_GRAPH_WITH_UNRESOLVED_SEMANTICS`. The canonical ROM
proves the complete direct selector xref set (two writers plus eight
read/control sites), selector domain `0..7`, descriptor table
`0x03B8DE..0x03B95E` with 8 × `0x10`-byte records, and both independent
8-entry dispatch contracts. The descriptor consumer is
`0x03A9EE` → `0x03AA0E` → `0x03B1D0`; its `+4` and `+8` fields reach the direct
`0x00003820` calls at `0x03B28A` and `0x03B2FE`, while child `+4` reaches the
same decompressor at `0x03B236`.

The seven ROM child tables selected from descriptor `+12` are all finite and
sentinel-closed. Their exact boundaries are `[0x03B95C,0x03B982)`,
`[0x03B998,0x03B9A6)`, `[0x03B982,0x03B998)`, `[0x03B9A6,0x03B9BC)`,
`[0x03B9BC,0x03B9D2)`, `[0x03B9D2,0x03B9E8)`, and `[0x03B9E8,0x03BA46)`.
The last boundary aliases the secondary dispatch index-7 target. The eighth
descriptor child value `0xFFFF0017` is explicitly unresolved/non-ROM.

The graph also proves the structural continuation
`descriptor +12` → child `+0/+8` relative tables → `0x03B730` input path,
including the exact `0x03B426` `MOVEA.L (A0),A0` context and `0x03B448`
`JSR 0xB730`. This does not prove object/animation/frame names or complete
resource payload enumeration. The shared relative-word candidate at
`0x03BDA6` has 18 observed words but no complete consumer boundary, so its
extent remains unresolved. SOURCE_OWNED remains `1,475,368` bytes before and
after; no ROM, decoded asset, C++, M13, or new replay artifact is included.
