# Reverse-Engineering Ledger
This file records what is known about the original Beyond Oasis binary. Do not promote guesses to facts without evidence.

## M11.5 — Ghidra-to-Atlas candidate integration
**Status:** VERIFIED as developer-only prioritization tooling. Ghidra discovery
remains candidate evidence; no new entry is promoted to project truth.

The normalized model is `oasis.m68k.re-candidate-map.v1`. It consumes the
deterministic external Ghidra v3 export (schema
`oasis.m68k.ghidra-map.v1`, 496 functions and 438 candidates, export SHA-256
`613A3AA6DEB8D2DCF994C82ADC6A6939B7D5F27AF67A51D05C16F090D60A5315`) and
merges it with the existing Atlas, bounded static call/reference evidence,
the already-recorded Beta correspondence and documented BizHawk observations.
No new broad trace or whole-ROM scanner was run.

Merge behavior is conservative and deterministic. Function and candidate
records with the same entry are merged; Atlas entries and known static/dynamic
addresses are retained even when Ghidra missed them. The classification
priority is: existing confirmed project evidence -> `CONFIRMED`; code/data or
boundary disagreement -> `CONFLICT` (except a known confirmed entry retains
`CONFIRMED` with boundary conflict metadata); independent static support ->
`STATIC_SUPPORTED`; runtime-only observation -> `DYNAMIC_OBSERVED`; otherwise
`GHIDRA_ONLY`. Dynamic flags remain visible when stronger evidence selects a
different classification. A Ghidra range containing a known table start is a
code/data conflict; unknown table extents are not invented.

The score is explainable: dynamic +40, known direct target +20, known direct
caller/call-site +10, Beta exact/structural/changed +10/+8/+4, LEAF/SHALLOW/
COMPLEX +10/+6/-8, no observed indirect flow +4, exact Atlas entry +5,
boundary/code-data conflict -50 and Ghidra-only without xrefs -5. Ties sort by
ascending address. This is prioritization only, not confidence.

The real USA/Beta merge produced 534 unique entries: 11 `CONFIRMED`, 39
`STATIC_SUPPORTED`, 0 `DYNAMIC_OBSERVED`, 483 `GHIDRA_ONLY` and 1 `CONFLICT`.
Complexity counts are LEAF 234, SHALLOW 172, COMPLEX 90 and UNKNOWN 38.
The top-10 non-confirmed audit retained the existing runtime call-site facts
and identified `0x611EA` as the highest-ranked new bounded Ghidra function with
static support and no recorded conflict. This is the recommended next target,
not work started by this checkpoint.

Two fresh runs were byte-identical: normalized JSON SHA-256
`5C17F6A735DC715B18CD5A5E8FA34F876CAD5CEB5D4511C80547E2C8720A22AC` and
human report SHA-256
`9E5A5884FE05B816C0265535529E79E2C336FBB4CE176D60AF57F522DAFD9C84`.
Remaining unknowns are Ghidra boundary correctness, indirect target recovery,
unobserved callers and routine semantics.

## M11.5 — bounded downstream runtime resolution at `0x60BFA` / `0x60C08`
**Status:** VERIFIED as scenario-only runtime evidence, developer-only. Static
global unresolved status is unchanged and no semantic field names are assigned.

Canonical-USA static verification found:

| PC | bytes | instruction | mode | base | displacement | width | direction |
|---|---|---|---|---|---:|---:|---|
| `0x60BFA` | `16 28 00 01` | `MOVE.B 1(A0),D3` | `d8(A0)` | A0 | 1 | 1 | read |
| `0x60C08` | `14 28 00 01` | `MOVE.B 1(A0),D2` | `d8(A0)` | A0 | 1 | 1 | read |

The frozen `start_pulse_120` / `120:Start` hardware-reset scenario under
BizHawk 2.11.1 reaches both targets at frame 423. The earlier stack consume
produces A0=`0x0006F8AE` at `0x60BD2`, but A0 changes before these targets;
the old value is therefore not a target-base invariant.

| PC | sequence | SR | relevant D registers | A0 | A7 | effective address | class | raw byte |
|---|---:|---|---|---|---|---|---|---|
| `0x60BFA` | 891 | `0x00002704` | D2=`0x00000100`, D3=`0x00000000` | `0x0006F8B0` | `0x00FF0BAC` | `0x0006F8B1` | ROM | `0x13` |
| `0x60C08` | 892 | `0x00002704` | D2=`0x00000000`, D3=`0x00070BCD` | `0x0006F8B2` | `0x00FF0BAC` | `0x0006F8B3` | ROM | `0x00` |

Full snapshots remain in the report. D0-D7 at `0x60BFA` are
`54,6F8AE,100,0,940F0EEE,60000000,0,FFFF`; at `0x60C08` they are
`54,6F8AE,0,70BCD,940F0EEE,60000000,0,FFFF`. A0-A7 are respectively
`6F8B0,C00,FF316C,FF136C,0,FF001A,FF06F2,FF0BAC` and
`6F8B2,C00,FF316C,FF136C,0,FF001A,FF06F2,FF0BAC`.

Both targets are `runtime_resolved_for_scenario` with
`resolution_scope=scenario_only`, confidence `CONFIRMED`, and
`static_global=false`. Two fresh reports are byte-identical: JSON SHA-256
`CF092C8B91BD2FDA858E3E165A75D3A891F8B90997D6F3E839A65FD053C97D91`; human
trace SHA-256 `34717649BB6DA2C389180A994DE226715F51739036A742BDCDD6B573B7FDE0C4`.
The relevant previous unresolved count is two target rechecks; two are now
scenario-resolved and zero are globally resolved. Global invariance is not
proven. Writer callback width and semantic role remain UNKNOWN.

## M11.5 — bounded runtime stack-value provenance for the `0x60B8C` path
**Status:** VERIFIED as bounded runtime evidence, developer-only, with writer
callback width UNKNOWN. No semantic role is assigned to the observed value.
The additive report is `oasis.m68k.re-stack-runtime-provenance.v1` and runs
outside `oasis_core` against the canonical USA ROM
(`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`) in
BizHawk 2.11.1. It reuses hardware reset, `start_pulse_120` and raw input
`120:Start`; it stops after the first post-`0x60BD0` continuation.

The same controlled run reaches every requested boundary:
`0x60B8C`, `0x6121A`, `0x60B90`, `0x60BCC`, `0x604BC`, `0x604E4`, `0x60BD0`,
`0x60BFA` and `0x60C08`. At `0x60B8C` the raw A7 is
`0x00FF0BA8`. Immediately before `0x60BCC`, P=A7 is
`0x00FF0BA8` and the big-endian longword at memory[P] is
`0x0006F8AE`. USA bytes at the call site are `61 00 F8 EE`, which decode as
`BSR.W 0x604BC` with return address `0x60BD0`.

The observed stack timeline is exact for this run: caller A7=P; BSR places
return `0x60BD0` at P-4 and callee entry A7 becomes `0x00FF0BA4`; the callee
return snapshot at `0x604E4 RTS` preserves that entry A7; the next caller
instruction `0x60BD0` has A7=P and reads the same `0x0006F8AE`; the following
`0x60BD2` snapshot has A0=`0x0006F8AE` and A7=`0x00FF0BAC`. This distinguishes
the BSR return slot from the separately consumed memory[P] longword without
assigning meaning to either value.

Bounded writer capture watched only `[P,P+4)`. The USA oracle confirms
`0x60B66` bytes `2F 08` (`MOVE.L A0,-(A7)`). BizHawk bus-write callbacks
reported callback PC `0x60B68`, correlated by the exact instruction hook to
`0x60B66`, with writes at `0x00FF0BAA`=`0x0000F8AE` and
`0x00FF0BA8`=`0x00000006`. The callback API exposes no width, so both widths
remain UNKNOWN; the final four bytes are reconstructed as
`0x0006F8AE` from the ordered concrete-range writes and pre-consume memory.
No MAME probe was needed, and no global writer or value invariant is claimed.

The JSON and human-readable reports were repeated from fresh runs. Their
SHA-256 values are respectively
`EFB30EEBF3EE0CEE929A02075088D684A2900B0DAFA192BC00390E484A846D0D` and
`4239B4182758FAEA49C56524953632C7568A52C508EF4E5BFFDCE6995872F7AC`, equal
between A/B. `0x60BFA` and `0x60C08` are recorded only as reached plus raw
boundary events; their memory references are not resolved. The result is a
concrete value for this scenario, not a static constant. Debug/Release/GNU
CTest, USA oracle, file-limit and diff-check pass; GitHub Actions CI run
`33874638457` for the focused implementation commit passed.

## M11.5 — bounded natural reachability search for `0x60B8C` / `0x60D4A`
**Status:** VERIFIED for one bounded natural caller path, developer-only. This
is not a complete instruction trace, emulator, input interpretation or
semantic conclusion.

The experiment reused `src/tools/re_bizhawk_natural_scenario.txt` and the
existing BizHawk 2.11.1 Lua probe against the canonical USA ROM
(`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`). Every
variant started from `hardware_reset`; the additive report remains schema
`oasis.m68k.natural-reach.v1` and the probe remains outside production runtime.
Search family: `natural_reach_60b8c_60d4a_v1`.

The bounded search tested neutral baseline, then stopped at the first primary
hit, `start_pulse_120` with one raw event `120:Start` and max horizon 1800.
It reached `0x60B8C` at frame 423 after 424 frame advances. Watched target
counts were:

| watched address | exact count |
|---|---:|
| `0x60B8C` | 3 |
| `0x60D4A` | 0 |
| `0x6121A` | 5 |
| `0x611EE` | 2 |

The first primary caller snapshot has PC `0x60B8C`, A7 `0x00FF0BA8` and
stack window `[0x00FF0B88,0x00FF0BE8)`, exactly the bounded
`[A7-0x20,A7+0x40)` window requested for this experiment. A following
`0x6121A` watched event is paired with raw caller `0x60B8C`; its observed
longword matches the statically computed BSR return address `0x60B90`.
These are register/stack observations only; no value receives a semantic
name.

Bounded input minimization tested the same one-event pulse at frames 119 and
121; both reached `0x60B8C` at frame 423. The neutral baseline, which removes
the only event, did not reach either primary caller. The successful input is
therefore minimal by event/button count within this bounded check, while
nearby timing is observationally equivalent. The exact `start_pulse_120` run
was repeated twice: JSON SHA-256
`20AA010BAECFE696A119D431A7EE6562074848219DD9C08A16D00BE3BBD994F2` and
trace SHA-256
`66F0095A195A9899789F08D0D4E8C5CF45EEFDDEE97EEE14B8A24516A9FB2271` matched.

The report contains 438 ordered events, but frame-boundary samples and exact
watch hooks do not establish a full instruction count or basic-block trace.
`0x60D4A` was not dynamically observed. The meaning of `Start`, the meaning
of the routines/data, complete callee/return state, and any unobserved control
flow remain UNKNOWN. No broader search was performed after the first success.

## M11.5 — bounded dynamic caller discrimination of 0x6121A
**Status:** VERIFIED bounded raw execution evidence, developer-only. The frozen
`natural_idle_to_6121a_v1` scenario remains unchanged: BizHawk 2.11.1,
hardware reset, neutral input, canonical USA ROM SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`, horizon
`max_frames:300`.

Static caller table, verified against the local canonical USA ROM:

| call-site | bytes | instruction | displacement | target | size | return address |
|---|---|---|---:|---|---:|---|
| `0x60B8C` | `61 00 06 8C` | `BSR.W` | `+0x068C` | `0x6121A` | 4 | `0x60B90` |
| `0x60D4A` | `61 00 04 CE` | `BSR.W` | `+0x04CE` | `0x6121A` | 4 | `0x60D4E` |
| `0x611EE` | `61 00 00 2A` | `BSR.W` | `+0x002A` | `0x6121A` | 4 | `0x611F2` |

The bounded probe watches only those three call-sites and `0x6121A`. Both
target hits occur at frame 113 and pair with `0x611EE` without an intervening
watched event: `caller seq=113 -> target seq=114`, then `caller seq=115 ->
target seq=116`. Hit 1 has caller A7 `0x00FF0BE6`, target-entry A7
`0x00FF0BE2`, delta `-4`; hit 2 has caller A7 `0x00FF0BAC`, target-entry A7
`0x00FF0BA8`, delta `-4`.

The longword read at target entry `[A7]` is `0x000611F2` for both hits, exactly
matching the statically computed return address for `0x611EE`. The captured raw
register delta for each pair contains only A7; D0-D7, A0-A6 and SR are equal in
the caller and target snapshots. This is instruction-level stack/register
evidence, not an ABI or semantic conclusion. The second pair is recorded as a
second observed caller-target event; whether it represents re-entry remains
UNKNOWN.

Two fresh executions of the identical scenario produce byte-identical caller
reports and traces. The normalized existing importer reports 117 events, 23
unique PCs, 0 inferred basic blocks, 0 inferred branch/call/return/read/write
events and deterministic hash `0x52F951E69F5A7100`. The trace includes frame
samples and exact watched-hook events; it is not a full instruction trace.

The result upgrades only the specific executed edge `0x611EE -> 0x6121A` for
this scenario. `0x60B8C` and `0x60D4A` were not observed, so their static edges
remain dynamically unselected. Because the selected caller is `0x611EE`, the
result is not relevant to the existing `0x60B8C`/`0x60D4A` caller-stack blocker.
The adapter exposes no separate post-instruction hook, so broader hook-timing
and callee/return-state questions remain outside this checkpoint. No semantic
name or function purpose is assigned to `0x6121A`.

## M11.5 — bounded natural reachability of 0x6121A
**Status:** VERIFIED bounded runtime evidence, developer-only. No emulator,
ROM, savestate or generated trace is part of the repository.

The frozen scenario `src/tools/re_bizhawk_natural_scenario.txt` uses schema
`oasis.m68k.emulator-scenario.v1`, ROM SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`, backend
BizHawk 2.11.1, `start_state=hardware_reset`, neutral controller state and
`stop_condition=max_frames:300`. The Lua probe uses real frame advancement and
target execution hooks only; it does not force PC/register/memory state or
patch the ROM.

Two independent runs reached primary target `0x6121A` at frame 113 after 114
frame advances. The exact target hook fired twice. The first-hit entry snapshot
is raw evidence only: PC `0x0006121A`, SR `0x00002714`, D0-D7
`FFFF,7FF0FFFF,FFFF,0,940F0A8C,60000083,0,40`, A0-A7
`00FF06F2,00FF0BFC,00000000,00FF13CC,00C00004,00FF001A,00000000,00FF0BE2`,
and stack window `[0x00FF0BC2,0x00FF0C22)`. No semantic role is assigned to
any register or stack value. The two natural JSON reports and normalized
human-readable trace imports are byte-identical.

Watched secondary addresses `0x60B8C`, `0x60D4A`, `0x60BCC`, `0x60BD0`,
`0x60BFA` and `0x60C08` were not observed in this finite scenario. Static raw
inspection confirms three direct call-sites to the primary target:

| call-site | bytes | displacement | target |
|---|---|---:|---|
| `0x60B8C` | `61 00 06 8C` | `+0x068C` | `0x6121A` |
| `0x60D4A` | `61 00 04 CE` | `+0x04CE` | `0x6121A` |
| `0x611EE` | `61 00 00 2A` | `+0x002A` | `0x6121A` |

The dynamic target callback does not expose the immediately preceding CPU
instruction. `previous_observed_pc=0x6135E` is only the last frame-boundary
sample and is explicitly not caller evidence. Exact transfer source, caller
selection, return state and stack provenance therefore remain UNKNOWN. The
normalized imported trace contains 114 bounded events, 113 frame-boundary PC
samples plus the exact target event(s), and no inferred branch/call/memory edge.
MAME writer provenance was not repeated because the BizHawk target reach was
already reproducible and a second backend would not expose the missing
preceding instruction without expanding scope.

This result proves deterministic natural reachability of the raw target only;
it does not prove routine semantics or resolve any static unresolved reference.

## M11.5 — external emulator boot-trace oracle PoC
**Status:** VERIFIED with real local backends, bounded to `boot_initial` and
developer-only. No emulator binary/source or ROM is part of the repository,
and no fake dynamic trace is used.

`oasis.m68k.emulator-trace.v1` is a developer-only normalized capture layer
outside `oasis_core`. Its neutral importer accepts externally supplied event
records for PC, optional block, branch, direct/indirect control flow, return,
memory access and optional D0-D7/A0-A7/SR snapshots. The normalizer records
ROM/emulator/backend/scenario/stop-condition metadata, deterministic event order and hash, bounded
coverage, direct call edges, indirect targets, reset-vector evidence, and
Atlas-known/Atlas-unknown executed PCs. Branch/call/indirect targets are
separately split into Atlas-known and Atlas-unknown sets. Frame/cycle values
are retained as non-deterministic fields and excluded from the hash; optional
register snapshots are deterministic hash input.

The static reset-vector reader is limited to the first eight ROM bytes:
initial A7 is the longword at offset 0 and initial PC is the longword at offset
4. Agreement with a first observed PC is reported only when an external trace
is supplied; it is not inferred from static bytes alone. The accepted input
header is `oasis.m68k.external-trace.v1`, followed by metadata lines such as
`scenario=boot_initial` and event lines such as
`event seq=0 pc=0x00000100 kind=instruction block=0x00000100 size=4`.

The real bake-off used already-installed MAME and BizHawk against the ignored
canonical USA ROM. Exact results follow; no emulator binary/source or ROM is
part of the repository.
| backend | exact executable/version | boot capture | register/memory evidence | replay |
|---|---|---|---|---|
| BizHawk primary | `D:\Program Files\BizHawk-2.11.1-win-x64\EmuHawk.exe`, `2.11.1` | 512 instruction events, 640 total events | D0-D7/A0-A7/SR and 128 bus writes through Lua hooks | A/B equal, `0x5CCA6906FAA6A219` |
| MAME secondary | `D:\Program Files\Mame\mame.exe`, `0.289` | 512 instruction events | D0-D7/A0-A7/SR through debugger trace; separate write watchpoint | A/B equal, `0xC2B1C053D1E43D76` |

Capability record (tested means exercised locally; untested means not claimed):

| capability | BizHawk 2.11.1 | MAME 0.289 |
|---|---|---|
| CLI launch / Genesis ROM load | tested | tested |
| unattended execution / automated shutdown | tested (`--chromeless`, `client.exitCode`) | tested (`-video none`, debugger `quit`) |
| scripting / Lua | tested Lua | debugger command script tested; Lua not used |
| M68000 PC | tested | tested |
| D0-D7, A0-A7, SR | tested | tested |
| execution hook/breakpoint | tested instruction hook | tested registerpoint |
| memory read hook | untested | untested |
| memory write hook/watchpoint | tested bus write hook | tested RAM watchpoint |
| save-state load/save | untested | untested |
| deterministic stepping / fixed stop | tested `emu.frameadvance`, 512 callbacks | tested fixed 512-event registerpoint |
| stdout/file trace export | tested Lua file export | tested debugger trace file |

MAME working command:
`mame.exe genesis -cart "Beyond Oasis (USA).bin" -homepath mame-home -video none -sound none -nothrottle -skip_gameinfo -debug -debugscript re_mame_boot_trace.cmd -seconds_to_run 10`.
BizHawk working command:
`EmuHawk.exe --chromeless --lua="re_bizhawk_boot_trace.lua" "Beyond Oasis (USA).bin"`.
The Lua script uses `event.on_bus_exec_any`, `event.on_bus_write`, register
reads and `emu.frameadvance`, then exits at exactly 512 instruction callbacks.
MAME's debugger script uses a fixed registerpoint and trace action; its
separate writer probe caught a 16-bit write at address `0x00FFFFFE`, with the
stopped instruction PC `0x0000026A`. The instruction-trace adapters currently
emit no normalized branch/call/return/read events, so those report counts are
zero and are not inferred from opcode text. Save-state APIs and read hooks were
not probed. The MAME trace begins at `0x214`; BizHawk begins at `0x26C`; both
are after reset PC `0x20E`, so reset/bootstrap transitions remain unknown.
Neither trace observed `0x6121A`, `0x60B8C` or `0x60D4A`. No semantic Atlas
entries were added.

## M11.5 — bounded caller-stack provenance before `0x60BCC`
**Status:** bounded implementation and USA oracle verified. No semantic role is
assigned to any stack value. The additive schema is
`oasis.m68k.re-caller-stack.v1`, a developer-only report over the existing
reachable `[0x60004,0x61204)` slice.

The call-site is `0x60BCC` in block `[0x60BC4,0x60CDA)` with reachable
predecessor `0x60BA4`. Symbolic entry A7 is `S`; the pre-call value is named
`P` only as the caller's pre-BSR stack pointer. The captured reachable path
contains these bounded stack events: `0x6042A MOVE.W SR,-(A7)` (2 bytes),
`0x60430 MOVEM.L` with mask `0x7FFE` (14 longwords, 56 bytes), and
`0x60B66 MOVE.L A0,-(A7)` (4 bytes). These operations establish depth only;
unknown register/status values are not converted into concrete data.

The USA oracle finds two relevant paths. One crosses direct `BSR.W 0x6121A`
at `0x60B8C`; another reaches the same call-site through `0x60D4A -> 0x6121A`
and a locally proven balanced call. Each unknown callee is outside the
permitted effect set, so its A7 and stack-slot effect is UNKNOWN and the
bounded provenance is invalidated. The later `BSR.W 0x604BC` at
`0x60BCC` uses the previously proven local callee summary only: its explicit
stack delta is zero and RTS restores caller pre-BSR A7. Consequently the
longword read by `0x60BD0 MOVEA.L (A7)+,A0` at `memory[P]` has no proven value;
`0x60BFA` and `0x60C08` remain unresolved. Reachable unresolved remains
`16→16`; the 14 `call_clobber` records are unchanged; speculative resolutions
are zero.

The USA oracle checks raw bytes at `0x6042A`, `0x60430`, `0x60B66`, `0x60B8C`,
`0x60D4A`, `0x60BCC`, `0x60BD0`, `0x60BFA`, and `0x60C08`, plus bounded CFG
and blocker evidence. It passes for the local supported USA ROM. No general
stack emulator, ABI, recursive callee analysis,
dynamic tracing, gameplay runtime integration, or M12 work was added.

## M11.5 — bounded callee-effect audit for `0x60BCC`
**Status:** VERIFIED bounded raw effect; no semantics inferred. The call-site
bytes `61 00 F8 EE` at `0x60BCC` decode as direct `BSR.W 0x604BC`; `0x60BCC`
is not the callee entry. Current decoder evidence bounds the callee as
`[0x604BC,0x604E6)`: one reachable block, one `RTS` at `0x604E4`, no direct
nested calls, indirect flow or unsupported instructions. The three static bit
operations use unresolved `(d16,A6)` references; concrete decoder references
are at `0x604BC -> 0x00FF0628`, `0x604C8 -> 0x00FF06F2` and
`0x604DE -> 0x00FF0016`.

The effect table is raw register evidence: A0 `overwritten_unknown` due to
three `(A0)+` writes; A1-A5 `not_touched`; A6 `overwritten_known`
`0x00FF06F2`; A7 `preserved`. The callee has no explicit stack operation.
Timeline: caller A7=P -> BSR pushes return `0x60BD0` at P-4 -> callee runs with
A7=P-4 -> `RTS` pops that return address and restores P -> caller executes
`0x60BD0 MOVEA.L (A7)+,A0`, reading an unknown longword at P and incrementing
A7 by 4. Therefore `0x60BD0` consumes another stack value, not the BSR return
address. Targets `0x60BFA` and `0x60C08` remain unresolved; 14
`call_clobber` refs and zero speculative resolutions are unchanged.

The additive developer-only report schema is
`oasis.m68k.re-callee-effect.v1`; it includes bounded CFG edges, return sites,
per-register effects, instruction/block-bound memory evidence, unsupported and
indirect categories, stack timeline and the two target rechecks. The USA oracle
checks exact bytes/CFG/effects when the local user-supplied ROM is available;
that ROM was absent in the current workspace. No ABI, recursive callee scan,
emulator, dynamic tracing, runtime behavior, whole-ROM work or M12 was added.
Exact reachable addresses: `604EA`, `60BD8`, `60BFA`, `60C08`, `60C1E`, `60C34`,
`60C4A`, `60C60`, `60C76`, `60C8A`, `60C94`, `60CAA`, `60CC2`, `60D94`, `60DB0`, `60DC8`.
## Known hardware addresses
| Address | Meaning | Confidence | Evidence/status |
|---|---|---:|---|
| `0x00C00000` | Mega Drive VDP data port | CONFIRMED | Public ROM-hacking constants + standard Mega Drive mapping |
| `0x00C00004` | Mega Drive VDP control port | CONFIRMED | Public ROM-hacking constants + standard Mega Drive mapping |
| `0x00FF0000` | 68000 work RAM base | CONFIRMED | Public ROM-hacking constants + standard Mega Drive mapping |
## Reference ROM identity

### USA retail — canonical engineering reference
| Field | Value | Confidence |
|---|---|---|
| Title | `Beyond Oasis (USA)` | CONFIRMED |
| Size | `3145728` bytes | CONFIRMED |
| CRC32 | `C4728225` | CONFIRMED |
| SHA-1 | `2944910c07c02eace98c17d78d07bef7859d386a` | CONFIRMED |
| SHA-256 | `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263` | CONFIRMED from uploaded reference |
| Detector result | `SUPPORTED` | VERIFIED |

### Region policy
- USA addresses are the default notation in reverse-engineering documents.
- Region-specific offsets must not leak into native gameplay code.
- Europe/Japan may be compared to USA as secondary evidence.
- Final C++ game model is region-independent.

## `[0x00003820, 0x00003B3E)` — graphics decompression routine
**Status:** VERIFIED and translated to native C++.

**Semantic confidence:** CONFIRMED for decompression behavior and call contract.

### Boundaries and call contract
- Exact half-open range: `[0x3820, 0x3B3E)`.
- Verified USA ROM contains 52 direct absolute `JSR 0x3820` calls and 0 direct absolute JMPs.
- `A0`: compressed source pointer; returns advanced to immediately after consumed input — CONFIRMED.
- `A1`: destination pointer; returns advanced to immediately after output — CONFIRMED.
- `D0-D2/A2` are preserved; format B additionally preserves `D3/D6/D7`.
- No hardware access or nested subroutine call occurs inside the decompressor.

### Format dispatch
At `0x3824`, byte `source[2]` selects the format:
- nonzero: command-stream format A;
- zero: bitstream format B.

### Format A
Observed command families:
- literal byte runs;
- repeated-byte runs;
- sliding-window backreferences into already-produced output;
- chained `0b011xxxxx` extensions continuing the preceding backreference;
- block-length framing and byte terminator.

### Format B
Observed behavior:
- three-byte block header followed by LSB-first control bits;
- literal tokens;
- multiple backreference forms;
- distance `1` special repeated-byte run;
- distance `0` block terminator;
- 16-bit control-bit refill.

### Native mapping and oracle traces
Implementation: `src/game/graphics_decompress.*`.

| Format | Caller | ROM source | Source consumed | Output bytes | Output SHA-256 |
|---|---:|---:|---:|---:|---|
| A | `0x00C394` | `0x16943C` | 1217 | 3072 | `65e99e74020fedbdcb97c8249a5ccfe540aca5bb5d29bfb260352cd6f388c31a` |
| B | `0x03C276` | `0x1894EA` | 112 | 128 | `167d4e5409f6b075b3b6f2bc61dbb747e8d8c857e8699745184ddf48d83bcda9` |

Native C++ matches original 68000 execution on source consumed, output length and SHA-256 for both oracle cases.

## M7 — indexed compressed-resource table at `0x0005CE96`
**Status:** INVESTIGATING.

### Table structure
- Base used by original code: `0x0005CE96` — CONFIRMED.
- Entry width: 4-byte absolute ROM pointer — CONFIRMED by reader at `0xD3B2`.
- Entry 0: `0x000000` — CONFIRMED; semantic role remains UNKNOWN (likely sentinel/unused).
- Entries 1..107 form a dense run of valid compressed-resource pointers from `0x1AD000` through `0x1E6EDA` — CONFIRMED.
- First entries:
  - index 1 -> `0x1AD000`;
  - index 2 -> `0x1AD9D4`;
  - index 3 -> `0x1AE1A8`;
  - index 4 -> `0x1AE8AA`.
- Sample pointed streams have nonzero `source[2]` and therefore use verified decompressor format A — CONFIRMED for sampled entries.

### Reader at `0x0000D3B2`
**Behavior: CONFIRMED.**

Relevant original sequence:
```text
D3B2 save D0/A0/A1
D3B6 A1 = 0xFF2FA8
D3BC A0 = 0x05CE96
D3C2 D0 <<= 2
D3C4 A0 = *(A0 + D0)
D3C8 JSR 0x3820
...
D404 RTS
```

Therefore incoming `D0` is a 0-based resource-table index and selected entry is decompressed into work RAM `0xFF2FA8`.

### Direct calls to `0xD3B2`
Static absolute-call search found seven direct calls:
- `0x02CFAA`, `0x02CFB8`;
- `0x02D410`;
- `0x032174`, `0x032182`;
- `0x032884`, `0x032892`.

Immediate `D0` values observed immediately before these calls include:
- `3`, `4`;
- `0x57` (87);
- `0x23` (35), `0x24` (36).

This proves the table is selected by stable numeric IDs used in gameplay/scene code. What those IDs mean is still under investigation.

### Developer screen-name list
The canonical ROM contains a developer-facing screen-name list in the same general metadata region:
- `VILLAGE` at `0x05DB4D`;
- `ECAPITAL` at `0x05DB56`;
- `HARBOR` at `0x05DB61`;
- `01-00` at `0x05DC48`;
- `01-05 BOSS` at `0x05DC70`;
- `14-01 KING` at `0x05E110`.

**Current hypothesis:** the `0x5CE96` indexed compressed-resource table may correspond to screen/room resources because its ~107 meaningful entries and stable numeric selectors are compatible with the ROM's developer screen inventory. **Confidence: LIKELY, not CONFIRMED.** Exact index-to-name correlation is the next proof target.

### Structure-processing routine at `0x0000D406`
**Status:** INVESTIGATING; high relevance to world/screen setup.

This routine is called extensively from code in roughly `0x2Cxxx..0x3Axxx` and reads a structured record through `A1`:
- words at offsets `+16`, `+18`, `+20` copied to RAM `0xFF16F4..0xFF16F8`;
- word `+22` -> `D6`;
- word `+24` -> `D7` and RAM `0xFF16F0`;
- long at `+8` -> RAM `0xFF16FA`;
- bytes beginning at `+12` are transformed into derived RAM tables;
- additional state buffers begin at `0xFF1716` and `0xFF173E`.

Do not name the record fields until caller/data evidence establishes semantics.

### Related resource loading at `0xD4C8..`
The same `0x5CE96` table is also indexed by four bytes from RAM `0xFF16FA`; each nonzero index selects and decompresses a resource into separate 4096-byte-spaced destinations beginning at `0xFF3FA8`. This strongly indicates the table contains reusable scene-related graphic/data resources, but exact semantics remain UNCONFIRMED.

### M7 next proof
1. enumerate developer screen names in exact order and compare indices `3`, `4`, `35`, `36`, `87` against table selectors;
2. disassemble the seven `0xD3B2` callers far enough backward to identify their surrounding screen/event identity;
3. decompress representative table entries with native C++ and compare header/structure patterns;
4. only after correlation, introduce a portable room/screen resource loader.

## M8 — player input and movement slice
**Status:** IMPLEMENTED as a portable movement/state slice; full animation/entity callback semantics remain INVESTIGATING.

### Player entity selection
- `0x0013D6..0x00142E` initializes the main entity at work RAM `0xFF19E8` and writes entity type `2` before calling `0x8D06` — **CONFIRMED**.
- The main entity pool uses 21 records with stride `0xBC`; `0x008EB2..0x008ED0` iterates that pool — **CONFIRMED**.

### Controller normalization and direction mapping
- `0x00217C..0x002188` calls the controller reader at `0x2992`; normalized port state is stored beginning at `0xFF165C`, with the movement nibble in `0xFF165E` — **CONFIRMED**.
- `0x0085E2` masks `0xFF165E` with `0x0F`, dispatches through the table at `0x85FA`, and returns direction plus fixed-point deltas — **CONFIRMED**.
- Cardinal vectors are `(+0x36000,0)`, `(-0x36000,0)`, `(0,+0x30000)`, `(0,-0x30000)`; diagonal vectors are `(+/-0x2A000,+/-0x25800)` — **CONFIRMED** from `0x8624..0x86AE`.
- The native mapping preserves the original low-nibble behavior, including opposite-direction collapse and diagonal combinations; unsupported combinations resolve to no movement — **VERIFIED by synthetic tests**.

### Movement and collision contract
- The main game loop at `0x008B22` calls `0x00557A` (player update) before `0x008E90` (active-entity movement), and then `0x00A196` (sprite/entity scheduling) — **CONFIRMED**.
- `0x00557A` selects `0xFF19E8` and enters `0x005670`; the state dispatch at `0x0059BA` routes entity `+0x04` state `0` to `0x0061F6`, state `2` to `0x0062E4`, and state `4` to `0x006516` — **CONFIRMED**.
- `0x0061F6` reads the normalized direction, writes direction `+0x16`, intent deltas `+0x4E/+0x52`, accumulated deltas `+0x72/+0x76`, and transitions `+0x04` to state `2` — **CONFIRMED**.
- The state-2 branch at `0x0062E4` calls the same direction routine; its no-input path at `0x0062CC` clears `+0x4E/+0x52`, writes `+0x2A=0` and returns `+0x04` to `0`; shared movement owns cleanup of `+0x72/+0x76` — **CONFIRMED**.
- The state-4 branch at `0x006516` maps `+0x16` through `0x83D4`, gates the turn on `+0x17` and the normalized input, and accumulates the selected axis through `+0x72/+0x76` — **CONFIRMED**.
- When state-4 input is absent, `0x006618` compares `FF197E` with `6`; the short path reaches the state-2 stop block, while the timeout path writes `+0x04=0x000C` at `0x006624` — **CONFIRMED**.
- The shared movement routine consumes `+0x72/+0x76` before committing position; `+0x2A` and `+0x26` participate in the animation/state sequence, but their presentation semantics remain **INVESTIGATING**.
- `0x008F12..0x00938C` is the shared active-entity movement update; main-pool records enter at `0x8F22` — **CONFIRMED**.
- X/Y fixed-point deltas are accumulated in entity fields `+0x72/+0x76`; integer positions are committed to `+0x08/+0x0C` — **CONFIRMED**.
- `0x009BF2` aggregates the entity footprint, and `0x00938E` is called before an axis commit; carry set takes the blocked path — **CONFIRMED**.
- Native `update_movement_state` mirrors the confirmed state-2 stop and state-4 axis-selection/accumulation rules; `PlayerState::try_move` then consumes those deltas through `ByteGridView::aggregate_world_square` and `evaluate_terrain_gate`.
- Native `VelocityAdjustContext` mirrors the three confirmed `0x64C4` outcomes and is optional until the external flag lifecycle is reconstructed.
- The shared footprint OR result is written at entity `+0x6F`; state-4 calls `0x64C4` with the retained axis delta, where global flags and the low nibble of `+0x6F` select no scaling, half scaling, or a Y-only half scaling — **CONFIRMED**, exact global lifecycle **INVESTIGATING**.
- `FF1985` is written by several event/control paths including `0x56F2`, `0x57C6` and `0x7E50`, and is read by the player dispatcher and `0x64C4`; it is not a player-owned field — **CONFIRMED**, lifecycle **INVESTIGATING**.
- `FF1984` is cleared/set by the active-entity checks around `0x2D220` and `0x2F250`, including a main-entity test against `+0x6E` bit 5 and `+0x10`; it is an external context flag — **CONFIRMED**.
- Bit 4 of `FF16F1` is read by `0x64C4`; no direct bit-4 writer was found in the scanned ROM references, so its producer remains **UNKNOWN**.
- Rendering, animation scripts and the unknown entity callback at `+0x22` are intentionally outside this slice.

## M9 — common entity pool framework
**Status:** IMPLEMENTED; raw pool iteration and one representative callback-dispatch path are verified. Semantic entity behavior remains outside M9.

### Pool iteration evidence
- `0x008E90` iterates four records from `FF2954` with stride `0x5A`, stores dispatcher `0x8EAA` in `FF193C`, and branches active records to `0x8F12` — **CONFIRMED**.
- `0x008EB2` iterates 21 records from `FF19E8` with stride `0xBC`, stores dispatcher `0x8ECC` in `FF193C`, and branches active records to `0x8F22` — **CONFIRMED**.
- `0x008ED4` iterates six records from `FF2D8C` with stride `0x5A`, stores dispatcher `0x8EEE` in `FF193C`, and branches active records to `0x8F12` — **CONFIRMED**.
- Each loop reads record word `+0x00` and uses `bgt` as the active test; zero and negative values are skipped — **CONFIRMED**.
- The native `EntityPoolView` exposes bounds, raw record spans and this active predicate without assigning enemy/NPC semantics to records.

### M9 boundary
The first M9 slice does not translate AI, attacks, animation callbacks, spawn tables or entity field names beyond the raw offsets required by pool iteration. Those require separate caller/data evidence.

### M9 shared record fields and representative dispatch

- The common movement entry at `0x8F22` reads raw record offsets `+0x9C`,
  `+0x9D`, `+0x2A`, `+0x2C`, `+0x2E`, `+0x30`, `+0x32`, `+0x37`, `+0x38`,
  `+0x72` and `+0x76` — **CONFIRMED** by direct accesses in the shared
  entry. The initializer at `0x8D06` copies two ROM pointers into `+0x26`
  and `+0x22`; their presentation/behavior meanings remain unassigned.
- A representative non-player path is the `FF2954` processing block at
  `0xA6A4`: it scans four `0x5A`-byte records, gates on `+0x00 > 0` and
  bit 2 of `+0x3A`, then reaches `0xA7D4`, which performs the proven
  indirect `jmp (+0x22)`. This is recorded as a raw callback-dispatch
  behavior, not as enemy/NPC AI.
- `EntityRecordView` exposes bounded big-endian word/long reads and the raw
  `+0x22` pointer. It deliberately does not invoke ROM addresses or assign
  semantic names to the remaining fields.

## Existing translated compatibility behavior

## M10 — spirit slot and dispatch trace
**Status:** IMPLEMENTED as a narrow deterministic slice; spirit names, button
meanings, targeting, abilities and presentation semantics remain UNKNOWN.

### Slot storage and lifecycle evidence
- At `0x007BE8`, the event handler subtracts `0x16` from the event code and
  uses the result as a bit index for `0x00FF0DBA` — **CONFIRMED**. The four
  observed event values `0x16..0x19` therefore map to slot bits `0..3`.
- At `0x005202`, the status-display path reads `0x00FF0DBA`; the adjacent
  byte table at `0x00522E` is `12 13 14 15 16 17 18 19` — **CONFIRMED**.
  This establishes four contiguous slot/event entries, but does not prove
  their character names or ability semantics.

### Active dispatch evidence
- `0x0031B80` checks bits `3` and `1` of entity field `+0x41` before entering
  the observed path — **CONFIRMED**. The meanings of these raw input bits are
  intentionally unassigned.
- The path tests bit `1` of `0x00FF0DBA`, then tests bit `0` of
  `0x00FF0DC4` as a one-shot guard — **CONFIRMED**.
- When the slot gate is open, it calls `0x0000C2EC` with selector `0x13`,
  fixed values `D5=0`, `D6=0x4000`, `D7=0x4800`, and sets guard bit 0 —
  **CONFIRMED**.
- It then falls through to queue selector `0x15` through `0x0000CA24`, using
  the player record's `+0x08` position-derived value and `D4=0x18` —
  **CONFIRMED**. The selectors are retained as raw trace values; their
  resource/effect semantics are not promoted beyond what the callers prove.

### Native mapping and oracle
`src/game/spirits/spirit_slots.*` models event-to-slot bit updates, the raw
`0x31B80` gate/selector trace and the `0x7A10`/`0x846C` summon-entry seed
without invoking ROM addresses. Synthetic tests cover inactive input,
unavailable slot, open guard, repeated guarded dispatch and the accepted or
rejected summon-entry gate. `oasis_spirit_slots_reference` checks the USA
bytes at `0x7BE8`, `0x5202`, `0x522E`, `0x31B80`, `0x31BC4`, `0x7A10` and
`0x846C`.

### Target-selection evidence
- `0x17CA6` calls `0xB922` with relative bounds `[-10,-6,10,6,0,4]`, then
  checks the returned record's raw type word against `0x16` at `0x17CE4` —
  **CONFIRMED**.
- `0xB922` scans the 21-record `FF19E8` pool in order, skips an inactive
  record or a record whose pointer equals the owner pointer, applies the
  observed X/Y interval checks and Z containment check, and returns the first
  spatial match — **CONFIRMED**. In the observed path the owner is from
  `FF2D8C`, so equal numeric indices across the two pools are not excluded.
- The native `find_observed_target` preserves the first-match-then-type-check
  behavior. The raw fields used by the query are exposed without semantic
  names; interaction meaning remains UNKNOWN. The ROM oracle also checks the
  query setup at `0x17CA6` and the scan prologue at `0xB922`.
- `0x846C` separately initializes raw type `0x16` in record `FF1AA4`; because
  `0xB922` scans `FF19E8`, this is tracked as a possible related producer but
  not as proof of a summon/ability relationship — **UNKNOWN**.
- The callback path is cross-pool: initializer `0xFFDE` obtains an owner from
  the six-record `FF2D8C` pool via `0xD9F0`, stores callback `0x17A96`, and
  `0x17A96` enters `0x17CA6` when its state word is zero. The query helper
  `0xB922` receives that owner in `A6` but scans target records from
  `FF19E8` — **CONFIRMED**. This corrects the native boundary; it does not
  prove summon or ability names.
- A static scan of field `+0x00` writers found the only literal raw type
  `0x16` write at `0x847C`, targeting singleton record `FF1AA4`. The observed
  `FF19E8` construction sites at `0x11DF0`, `0x27B2A` and `0x2BD20` use other
  literal/table values or data-driven values; no producer of raw type `0x16`
  in `FF19E8` is proven — **UNKNOWN**.
- The additional loader scan confirms that `0xFFDE` consumes a data stream
  into the auxiliary `FF2D8C` pool, while the generic stream loaders at
  `0xFF1A` and `0xFCB8` allocate from `FF1CD8`. These are concrete pool
  boundaries, but they do not identify a producer for raw type `0x16` in
  `FF19E8` or prove that the `0x17A96`/`0x17CA6` callback is a summon —
  **CONFIRMED boundary; UNKNOWN semantics**.
- A separate summon-entry candidate is now byte-backed: `0x7A10` rejects
  caller flag bit 1 and calls `0x846C` only for caller state `+0x30 == 0x18`.
  `0x846C` writes raw type `0x16` to singleton `FF1AA4`, copies caller
  position `+0x08/+0x0C`, writes raw `0x13` to `+0x10`, copies `+0x17` to
  `+0x66` and `+0x14`, writes `0x4F8` to `+0x18/+0x5A`, and clears `+0xA6`
  and `+0xAA` — **CONFIRMED raw entry; summon identity UNKNOWN**. The
  table-derived velocity tail beginning at `0x84B2` remains outside the native
  slice until its complete data contract is recovered.

### Tile copy to work RAM
Initial C++ compatibility implementation exists, derived from the public `tilecopy_to_ram` macro. Revisit after data interfaces stabilize.

### Tile copy to VRAM
Initial C++ compatibility implementation exists, derived from the public `tilecopy_to_vram` macro. Current VDP model is intentionally narrow.

## M11 — raw event producer and router boundary
**Status:** IMPLEMENTED as a bounded raw-data slice; event names, progression,
dialogue and command semantics remain UNKNOWN.

### Producer evidence
- `0x0082AE` calls bounded helper `0xB9EC` with the active `FF19E8` pool and
  a search window built from raw bounds `[-6,+6]` — **CONFIRMED**.
- The first returned record is accepted only when raw type `+0x00` equals
  `0x0008`; the source type is then cleared — **CONFIRMED**.
- The producer composes `FF1976` from source `+0x32` shifted left by eight
  and source byte `+0x52`, then copies source `+0x04` to `FF1978` and source
  long `+0x4E` to `FF197A` — **CONFIRMED**.
- A static USA-ROM scan found no direct literal `BSR` or absolute `JSR` to
  `0x82AE`; its caller may be indirect or data-driven and remains UNKNOWN.
- The source type-8 meaning and resulting event-code meaning are not
  assigned. Internal selection semantics of `0xB9EC` remain outside this
  slice.

### Router evidence
- `0x007A28` tests caller field `+0x37` bit 1, reads byte `FF1976` and
  dispatches bounded raw ranges to `0x7B64`, `0x7BD4`, `0x7BF6`, `0x7BA4`,
  `0x7BE8` and `0x7B84`; zero and values above `0x3F` fall through to
  `0x7A6C` — **CONFIRMED**.
- When the tested bit is clear, control goes to raw address `0x7B28`, an
  immediate `RTS`; the adjacent routine begins at `0x7B2A` — **CONFIRMED**.
- The separate `0x7B2A` routine calls external raw address `0x60004` with constants
  `0x0006` and `0x0008`, masks the result to `0x01FF`, returns on the sentinel
  `0x01FF`, then clears mask `0xFFF9` at `FF17B8` and writes raw `FF0D7E` to
  current-record `+0x06` and `0xFFFF` to `+0x5C` — **CONFIRMED**. The helper
  and field meanings remain UNKNOWN.
- The external entry at `0x60004` contains `BRA.W +0x0424`, whose 68000
  word-displacement target is `0x6042A`; `0x6042C` is the following
  instruction. Its raw command
  dispatcher compares `D0` against values `1..8`, and the command `0x0006`
  branch at `0x60478` reaches `0x609C6` — **CONFIRMED**. That handler starts
  with `D0=0`, builds a raw flag mask from driver RAM bit 4 values and returns
  through the shared driver epilogue at `0x611D8` — **CONFIRMED**.
- The command `0x0008` branch reaches `0x60D10`, which performs raw 68000/Z80
  bus operations and copies `0x0606` bytes from `A01000` to `FF0022` —
  **CONFIRMED**. Driver protocol and audio meaning remain outside M11.
- The command `0x0006` handler at `0x609C6` starts `D0` at zero, sets it to
  `0x01FF` when bit 0 of `FF001A` is set, then ORs bits from bit 4 of eleven
  driver RAM locations before returning through `0x611D8` — **CONFIRMED**.
  The event-side meaning of this mask remains UNKNOWN.
- On the `0x01FF` path, `0x7B2A` calls command `0x0008`, performs the raw
  state writes, then branches to `0x62CC`; `0x62CC` clears current-record
  fields `+0x4E`, `+0x52`, `+0x2A` and `+0x04` — **CONFIRMED**. This closes
  the bounded raw side-effect contract without assigning driver semantics.
- The native `event_router` module exposes the producer transfer, raw
  handler-address mapping and the bounded adjacent `0x7B2A` trace. Synthetic tests and
  the USA-ROM oracle cover the type gate, field composition and dispatch
  boundaries.

### M11 boundary
No generic event-stream parser, dialogue decoder, progression model or
unproven command is introduced. Further work must establish the caller/data
contract around the selected type-8 source or a downstream handler first.

### M11 post-completion RE-acceleration checkpoint — bounded `0x60004` slice
**Status:** VERIFIED as developer-only evidence tooling; no native gameplay
behavior is inferred or added.

- The local USA-ROM tool reads through `Rom::load`, validates the canonical
  fingerprint, and decodes only reachable direct control flow in the explicit
  half-open range `[0x60004, 0x61204)` — **VERIFIED**.
- The bytes `60 00 04 24` at `0x60004` are `BRA.W +0x0424`; the 68000
  word-displacement target is `0x6042A`. `0x6042C` is the following opcode,
  correcting the earlier ledger wording — **CONFIRMED by oracle**.
- The deterministic report contains 801 reachable instructions, 109 blocks,
  72 direct branches, 17 direct calls, 3 absolute ROM refs and 114 absolute
  RAM refs with constants attached — **VERIFIED**. Direct edges include
  `0x60004->0x6042A`, `0x60478->0x609C6`, `0x60488->0x60D10`; indirect and
  unsupported categories remain separate and empty on this reachable slice.
- Schema `oasis.m68k.re-slice.v1` and the human report are sorted/deterministic;
  Debug/Release hashes match. Decoder coverage remains bounded to exercised
  opcode families, never a generic CPU/emulator/recompiler/runtime dependency;
  unsupported instructions and indirect targets stay explicit.

**Open questions:** driver command meanings, audio protocol, event/progression
semantics and the producer caller remain **UNKNOWN**.

### M11.5 second RE-acceleration slice — bounded multi-function report
**Status:** VERIFIED as developer-only tooling; no production C++ behavior or
semantic names were added.
- The local-USA CLI analyzes four existing evidence targets: exact documented
  `[0x3820,0x3B3E)` and `[0xD3B2,0xD406)`, plus bounded-only windows beginning
  at `0x8E90` (`0x120` bytes) and `0xA6A4` (`0x180` bytes). Boundary discovery
  marks a return boundary only when every bounded path is complete; no boundary
  is guessed for the two windows.
- The deterministic `oasis.m68k.re-program.v1` report contains 421 decoded
  instructions in 131 basic blocks, one direct call site and one analyzed
  caller→callee edge (`0xD3B2 -> 0x3820`). Call sites retain caller function,
  basic block and instruction address; indirect/unresolved flow is separate.
- It records 18 confirmed absolute references (including `0x5CE96` and
  `FF2FA8`), 114 unresolved register-based references, one unresolved indirect
  jump at `0xA7E2`, and two unsupported opcode locations. Each memory item is
  bound to function, bounded slice, basic block and instruction.
- The USA oracle reproduces entry bytes, the reader call, pool dispatch edges
  `0x8EA6 -> 0x8F12` and `0x8EC8 -> 0x8F22`, and the raw callback jump boundary.
  Debug/Release JSON hashes match; synthetic tests cover confirmed boundaries,
  caller/callee grouping, bindings, unresolved flow and unsupported addressing.

**Limitations:** the decoder remains a bounded opcode-family decoder. It does
not resolve register-based effective addresses, indirect targets or unknown
function boundaries, and it is not an emulator, whole-ROM discovery pass or
recompiler. The `0x8E90` and `0xA6A4` reports intentionally include only their
explicit windows and must not be read as complete function recovery.

### M11.5 third checkpoint — bounded dynamic trace at `0xA7D4`
**Status:** VERIFIED as a developer-only dynamic evidence PoC; no gameplay
runtime or full emulator was added.

- The isolated scenario uses the static `0xA6A4` slice but starts from the
  controlled/savestate-like PC `0xA7D4`. It initializes raw `A6=FF2954`, the
  record word at `+0x00` to `1`, and the raw pointer at `+0x22` to `0xA7E4`.
- The bounded backend executes exactly `A7D4, A7DA, A7DE, A7E2, A7E4`:
  three static blocks, one not-taken `BEQ` at `A7DA`, two RAM reads, one
  indirect jump and one `RTS`. Relevant `A6`/`A0` snapshots are retained only
  at the unresolved memory/control-flow sites.
- Static/dynamic comparison resolves three prior items: the effective RAM
  addresses at `A7D4` (`FF2954`) and `A7DE` (`FF2976`), plus indirect target
  `A7E2 -> A7E4`. Nine other static unresolved memory references remain
  unobserved and therefore unresolved.
- The deterministic `oasis.m68k.re-trace.v1` JSON and human report retain PC,
  block, branch, call/return, memory and indirect-target evidence. Synthetic
  and USA-ROM oracles reproduce the same five PCs, branch outcome, RAM reads,
  pointer value and resolved target.

**Backend limitation:** this is a bounded scenario interpreter for the exact
five-opcode path, not a general 68000 CPU or Mega Drive emulator. It models no
full call stack, writes, interrupts, peripherals or alternate path; execution
stops explicitly on an unsupported scenario PC. Full-game tracing remains out
of scope.

### M11.5 fourth checkpoint — USA retail versus USA Beta 1994-11-01
**Status:** VERIFIED bounded correspondence only. `oasis_re_diff` reports the
five requested pairs: exact `0x3820->0x37D0`, `0x60004->0x60004`,
`0x82AE->0x825E`, `0x7A28->0x79D8`, and structural `0xA6A4->0xA654` with
changed block ordinal 10. No semantic identity or behavior is inferred.

### M11.5 fifth checkpoint — changed block ordinal 10 detail
**Status:** CONFIRMED raw bounded evidence. Retail `[0xA786,0xA792)` and beta
`[0xA736,0xA742)` are 12-byte blocks; corresponding edges are
`A6BA->A786`/`A66A->A736`, `A78E->A7D4`/`A73E->A784`, and
`A78E->A792`/`A73E->A742`. Only `2F3C0000A6BE` vs `2F3C0000A66E` differs
(`relocation_only`); `4A46` and `6B000044` (condition B) are identical.
Semantics remain unknown.

### M11.5 first bounded ROM Atlas prototype
**Status:** VERIFIED as developer-only aggregation; no semantic names or
runtime behavior were added. Schema: `oasis.m68k.re-atlas.v1`.

- The typed manifest contains 13 entries: code `0x3820`, `0x60004`, `0x7A28`,
  `0x82AE`, `0x8E90`, `0x938E`, `0x9BF2`, `0xA6A4`, `0xD3B2`; tables
  `0x5CE96`, `0x96E8`, `0x96F8`, `0xC92C`. IDs are raw address IDs.
- Exact boundaries are claimed only for `[0x3820,0x3B3E)`,
  `[0xD3B2,0xD406)` and the documented 108-entry table
  `[0x5CE96,0x5D046)`. Other entries expose bounded evidence ends without
  claiming function ownership.
- The report reuses `re_program`/`re_diff`/`re_trace` for 13 call edges,
  function/block-bound refs, beta correspondence and raw A6A4 facts; no
  whole-ROM scan was added. USA/Beta oracle: 13 entries, 1314 confirmed
  classified bytes, 6560 bounded bytes and zero conflicts; native statuses
  remain limited to previously tested paths.
- Atlas-driven `oasis.m68k.re-ranking.v1` ranks all 577 unresolved refs. USA
  result: displacement 446, `0x60004` 424, A6 387, immediate propagation 168,
  dynamic candidates 2 and unsupported decoder items 4.
**Unknown:** table sizes at `0x96E8`, `0x96F8`, `0xC92C`, bounded ownership,
unresolved effective addresses and routine semantics remain unknown. Atlas is
not an emulator, recompiler, whole-ROM map or gameplay runtime dependency.
### M11.5 Ghidra ROM Mapping PoC — canonical USA baseline
**Status:** VERIFIED as a developer-only structural discovery experiment;
decision `GHIDRA_USEFUL_WITH_PROJECT_FIXUPS` (option B). No semantic function
names or gameplay behavior are inferred from this output. Ghidra findings are
candidate evidence only; the existing `oasis_re` and runtime captures remain
the verification authority.

- Input fingerprint: 3145728 bytes, SHA-256
  `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
  Official Ghidra 12.1.3 headless used Temurin JDK 21.0.12.1+1, raw
  BinaryLoader, base `0x000000`, language `68000:BE:32:default`, compiler
  `default`, normal conservative auto-analysis and the developer-only
  `OasisGhidraMap` post-script.
- Export schema is `oasis.m68k.ghidra-map.v1`. The map contains 496 functions,
  438 candidates, 364 direct BSR targets, 106 direct JSR targets and 6
  vector-derived targets. The unresolved-call field is present and empty for
  this Ghidra run; no unresolved target is promoted to a resolved edge.
- Known-entry benchmark over
  `0x3820, 0x7A28, 0x82AE, 0x8E90, 0x938E, 0x9BF2, 0xA6A4, 0xD3B2,
  0x60004, 0x604BC, 0x6121A`: 7 exact function matches, one wrong boundary
  (`0x3820`, Ghidra ended at `0x38D0` versus known `0x3B3E`), one code-only
  entry (`0xA6A4`) and two missed entries (`0x7A28`, `0x82AE`). Thus exact
  function recall is 7/11 (63.6%) and code presence is 9/11 (81.8%).
- Required direct call edges were found at
  `0x60B8C->0x6121A`, `0x60D4A->0x6121A`, `0x611EE->0x6121A` and
  `0x60BCC->0x604BC`; the edges `0x60004->0x6042A` and
  `0xD3B2->0x3820` were missed. Result: 4/6 (66.7%). These are structural
  reference results, not proof of routine semantics.
- Data checks at `0x5CE96`, `0x96E8`, `0x96F8` and `0xC92C` all decoded as
  non-code and had useful xrefs (4, 20, 4 and 2 respectively). Only `0xC92C`
  was recognized as defined data. At `0xA7E2`, Ghidra listed `jmp`, did not
  identify an indirect flow through its API and exposed no reference/target;
  the existing bounded `oasis_re` evidence remains authoritative for the
  indirect dispatch observation.
- A bounded 20-item false-positive sample contained 19 `LIKELY_CODE` and one
  `AMBIGUOUS` item at `0x020E`. Two fresh external Ghidra projects produced
  identical 390972-byte JSON exports, SHA-256
  `613A3AA6DEB8D2DCF994C82ADC6A6939B7D5F27AF67A51D05C16F090D60A5315`.

**Limitations:** Ghidra emitted decompiler warnings around `0x6163E` and
several invalid or unresolved instruction addresses; these were not treated
as semantic evidence. The Windows wrapper rejected the `.md` suffix before
import, so an externally stored byte-identical `.bin` adapter copy was used;
the supplied ROM itself was not changed or copied into a tracked path. No
Ghidra project/database/raw disassembly or ROM was committed. This checkpoint
ends after the option-B decision; M12 is not started.

## ROM identification implementation
**Status:** VERIFIED.

Detector records byte size, Mega Drive header, Sega checksum, CRC32, SHA-1, SHA-256 and classification. Synthetic tests contain no original ROM bytes.
