# Project State

CURRENT_MILESTONE: M11 — Scripts/events/dialogue
CURRENT_TASK: M11.5 bounded dynamic caller discrimination for 0x6121A
STATUS: COMPLETE_WITH_LIMITATIONS
LAST_VERIFIED_RESULT: frozen neutral hardware-reset BizHawk scenario proves 0x611EE -> 0x6121A twice with matching BSR return address 0x611F2; GitHub CI `33870143848` passed
NEXT_ACTION: stop at this checkpoint; await an explicitly scoped bounded RE task
DO_NOT_WORK_ON: M12+, Thor 2, Saturn support, remaster features, speculative dialogue or event semantics
BLOCKERS: callee semantics and stack-writer provenance remain outside scope; 0x60B8C and 0x60D4A were not observed in the frozen scenario

## M11.5 bounded dynamic caller discrimination
- Reused `natural_idle_to_6121a_v1` without gameplay inputs and added only exact
  BizHawk hooks for `0x60B8C`, `0x60D4A`, `0x611EE` and `0x6121A`.
- Both target hits at frame 113 pair unambiguously with `0x611EE`: sequences
  `113 -> 114` and `115 -> 116`. Caller A7/target-entry A7 are respectively
  `0x00FF0BE6 -> 0x00FF0BE2` and `0x00FF0BAC -> 0x00FF0BA8`; both deltas are
  `-4`.
- At both target entries, raw stack longword is `0x000611F2`, matching the
  statically computed four-byte BSR return address. Raw register delta contains
  only A7; D0-D7, A0-A6 and SR are unchanged in captured snapshots.
- Static USA oracle verifies all three BSR.W encodings and return addresses.
  Natural reports, normalized traces and imported reports are deterministic;
  normalized result is 117 events, 23 unique PCs, 0 inferred blocks and hash
  `0x52F951E69F5A7100`. This upgrades only executed edge `0x611EE -> 0x6121A`.
- This path is not one of the existing `0x60B8C`/`0x60D4A` stack-blocker paths.
  Whether the second raw pair is re-entry remains UNKNOWN; no callee analysis,
  ABI inference, stack-writer tracing, production behavior or M12 work was done.

## M11.5 bounded natural reachability scenario
- Added developer-only `oasis.m68k.emulator-scenario.v1` parsing/serialization and
  a frozen neutral-input BizHawk scenario. It starts from hardware reset, uses
  no forced CPU state or ROM patch, watches `0x6121A` plus the six related
  addresses, and stops at `max_frames:300`.
- BizHawk 2.11.1 reached `0x6121A` at frame 113 after 114 frame advances; the
  exact target hook fired twice. Entry PC is `0x6121A`, A7 is `0x00FF0BE2`,
  stack window starts at `0x00FF0BC2`, and D0-D7/A0-A7/SR are captured. All
  secondary watched targets were zero in this scenario.
- Two fresh runs produced byte-identical natural JSON and normalized trace
  reports. Coverage is intentionally bounded to frame-boundary PC samples plus
  exact target hooks, so the source instruction is UNKNOWN. Static raw evidence
  separately confirms direct call-sites `0x60B8C`, `0x60D4A` and `0x611EE` to
  `0x6121A`; dynamic caller selection is not claimed.
- Local USA oracle checks the ROM fingerprint, exact call-site bytes, frozen
  scenario and observed target/frame/register/stack facts. No emulator or ROM
  is tracked; no roadmap milestone changed.

## M11.5 external emulator boot-trace oracle
- Added developer-only `oasis.m68k.emulator-trace.v1` importer/normalizer;
  it is not linked into `oasis_core` and has no emulator dependency. It accepts
  externally captured PC, block, branch, call, return, memory and indirect
  events, optional D0-D7/A0-A7/SR snapshots, backend metadata and bounded
  limits.
- The report computes deterministic event ordering/hash, static reset-vector
  evidence, safe observed ranges, direct call edges, indirect targets and
  Atlas-known/Atlas-unknown PC sets and separate control-flow-target sets.
  Frame/cycle fields are retained but are excluded from deterministic identity;
  optional register snapshots are included in it.
- Real local bake-off: MAME `0.289` produced two matching 512-event normalized
  traces with first observed PC `0x214`; BizHawk `2.11.1` produced two matching
  normalized traces containing 512 instructions and 128 RAM writes, with first
  observed PC `0x26C`. ROM fingerprint is the canonical USA SHA-256
  `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
  BizHawk Lua bus hooks are primary; MAME debugger trace/watchpoint is
  secondary. A MAME watchpoint caught writer PC `0x26A`, address `0xFFFFFE`,
  16-bit value `0`. `0x6121A`, `0x60B8C` and `0x60D4A` were not observed.
  Synthetic tests and real ROM imports pass; no emulator source/binary, ROM or
  generated trace was added.

## M11.5 bounded caller-stack provenance audit
- Schema: `oasis.m68k.re-caller-stack.v1`, developer-only and separate from
  `oasis_core`; scope is only reachable paths from `0x60004` to `0x60BCC`.
- The call-site is in block `[0x60BC4,0x60CDA)` with reachable predecessor
  block `0x60BA4`. Symbolic A7 begins at `S`. The actual captured path records
  `0x6042A MOVE.W SR,-(A7)`, `0x60430 MOVEM.L regs,-(A7)`, and
  `0x60B66 MOVE.L A0,-(A7)`.
- Two relevant bounded paths reach the call-site. One crosses direct
  `BSR.W 0x60B8C -> 0x6121A` after the observed push sequence; another crosses
  `0x60D4A -> 0x6121A` and a locally proven balanced call. Both unknown calls
  invalidate A7 and stack slots. The already audited `0x60BCC -> 0x604BC` is
  recorded as balanced only. Thus `memory[P]` is unknown, targets remain
  `16→16` unresolved, 14 `call_clobber` items remain unchanged, and
  speculative resolutions are zero. USA metrics are 2 paths, 14 unique stack
  events, 4 prior direct calls, 1 locally proven effect and 3 unknown effects.
- Synthetic tests and the USA oracle pass. The oracle checks exact raw bytes,
  CFG location, stack events and both unknown-call blockers. No general stack
  model, ABI, recursion, emulator, dynamic tracing, runtime dependency or M12
  work was added.

## M11.5 bounded callee-effect audit
- Schema: `oasis.m68k.re-callee-effect.v1`, developer-only and separate from
  `oasis_core`. The call-site `0x60BCC` is `BSR.W` to actual callee `0x604BC`.
- Bounded callee: `[0x604BC,0x604E6)`, one reachable block, one return at
  `0x604E4`, no nested calls, indirect flow or unsupported instructions.
- A0=`overwritten_unknown` (three `(A0)+` writes), A1-A5=`not_touched`,
  A6=`overwritten_known(0x00FF06F2)`, A7=`preserved`. Explicit callee stack
  delta is zero; `RTS` pops the BSR return address and restores caller pre-BSR A7.
- Raw timeline: caller A7=P; BSR stores return `0x60BD0` at P-4 and A7=P-4;
  callee has no explicit stack operation; RTS restores P; `0x60BD0` is outside
  callee and reads unknown longword at P before A7+=4. Both target refs remain
  unresolved; the 14 `call_clobber` refs are unchanged.
- Synthetic tests pass. USA oracle is implemented and requires the local user ROM;
  that ROM was absent in this workspace. No ABI, emulator, recursion or M12.

## M11.5 bounded MOVEA postincrement transfer checkpoint
- `oasis.m68k.re-reachable-closure.v1` recognizes only longword `MOVEA.L
  (A7)+,An`: source mode 3/A7, destination mode 1/An, `An=memory[old A7]`,
  and `A7 += 4`. Narrow stack provenance accepts only immediate long push,
  known-address PEA, proven `MOVE.L An,-(A7)` and this pop.
- USA `0x60BD0` is confirmed as `20 5F`; the path to both `0x60BFA` and
  `0x60C08` crosses `BSR 0x60BCC`. Therefore their stack value and A7 input
  are unknown; no callee/return-address/ABI effect is assumed. Instruction-level
  increment is recorded as 4 bytes and no semantic role is assigned to A0.
- Result: exact target refs 2, newly resolved 0, reachable unresolved 16→16,
  14 prior `call_clobber` preserved and 2 `other` with
  `stack_value_unknown_call_boundary`; speculative resolutions 0.
- Synthetic stack/merge/call-boundary tests and USA oracle pass. No other
  postincrement form, general stack emulator, dynamic tracing, runtime or M12.

## M11.5 bounded reachable-unresolved closure audit
- `oasis_re_reachable_closure` is a local-USA-ROM-only developer report over
  the existing Atlas, bounded decoder CFG and resolution result; it is not
  linked into `oasis_core` or gameplay runtime.
- Schema: `oasis.m68k.re-reachable-closure.v1`. The report accounts for exactly
  16 reachable unresolved refs and preserves instruction, operand mode,
  predecessor, bounded-definition and provenance evidence.
- USA result: 14 `call_clobber`, 2 `unsupported_transfer`, newly resolved 0,
  reachable unresolved after 16, nonreachable unresolved 80. Raw Atlas remains
  577 and raw displacement backlog remains 446; no evidence was removed.
- The former unsupported transfers at `0x60BFA` and `0x60C08` are now covered
  by the narrow MOVEA pop rule; their actual stack value remains unknown at the
  call boundary documented above.
- No dynamic scenario, calling convention, entry-state guess, semantic name,
  island investigation or M12 work was added.

## M11.5 bounded unreachable-CFG evidence audit
- `oasis_re_cfg_audit` is a local-USA-ROM-only developer report over the
  existing Atlas, bounded decoder and resolution result; it is not linked into
  `oasis_core` or gameplay runtime.
- The report schema is `oasis.m68k.re-cfg-audit.v1`. It audits exactly 80
  nonreachable records in `[0x60004,0x61204)`, groups them into 17 islands,
  and preserves instruction, block, edge, reachability-factor and byte evidence.
- USA result: 77 unreachable-code candidates / 332 bytes and 3 unknown / 18
  bytes; zero known incoming edges, zero secondary/data/artifact/tail records.
  Reachable unresolved remains 16 of raw 96; Atlas remains 577 and its
  displacement ranking remains 446. These are classifications, not semantic
  conclusions or automatic unresolved-count reductions.
- No new beta scan or dynamic scenario was added. All data/code status and
  indirect/address-taken entry possibilities remain unknown.

## M11.5 bounded address-displacement resolution PoC
- `oasis_re_resolution` is a developer-only bounded dataflow/report layer over
  the existing `0x60004` slice; it is not linked into `oasis_core`.
- The pass supports only conservative address-register setup/copy/arithmetic,
  agreed predecessor states and explicit invalidation at calls/unknown writes.
- USA result: 390 candidates examined, 294 resolved, 96 unresolved (92 unknown
  base and 4 CFG merge), 0 provenance failures; 294 RAM effective addresses,
  78 unique, 0 ROM effective addresses.
- Ranking delta: Atlas unresolved 577→283; displacement 446→152; A6 387→123;
  immediate candidates 168→54. The 80 candidates outside the entry-reachable
  CFG remain unknown; no semantic field names were assigned.

## M11.5 first bounded ROM Atlas prototype
- `oasis_re_atlas` is a developer-only typed manifest/report layer over the
  existing bounded `re_program`, `re_trace` and `re_diff` tooling; `oasis_core`
  gameplay runtime is unchanged.
- The manifest has 13 raw-address entries: code at `0x3820`, `0x60004`,
  `0x7A28`, `0x82AE`, `0x8E90`, `0x938E`, `0x9BF2`, `0xA6A4`, `0xD3B2`, and
  tables at `0x5CE96`, `0x96E8`, `0x96F8`, `0xC92C`.
- Exact boundaries are only claimed for `0x3820..0x3B3E`, `0xD3B2..0xD406`
  and the 108-entry `0x5CE96` table. Other windows retain a separate bounded
  evidence end and do not claim ownership.
- Atlas records direct call sites/edges, function/block-bound refs, unresolved
  and unsupported categories, beta correspondence, native status and queries.
  A6A4 carries raw dynamic facts `A7D4->FF2954`, `A7DE->FF2976`, `A7E2->A7E4`.
- Current local result: 13 entries, 13 call edges, 1314 confirmed classified
  bytes, 6560 bounded evidence bytes and zero conflicts. Semantics remain raw
  or UNKNOWN; no milestone status changed.
- `oasis.m68k.re-ranking.v1` ranks the 577 static unresolved memory refs from
  Atlas. Largest structural opportunities are displacement mode 446 refs,
  function `0x60004` 424, register `A6` 387, immediate candidates 168 and the
  bounded dynamic scenario 2; 4 unsupported decoder items stay separate.

## M11.5 fifth checkpoint — changed block ordinal 10 detail
- Retail block 10 is `[0xA786,0xA792)`; beta block 10 is
  `[0xA736,0xA742)`. Both contain 3 instructions and 12 bytes.
- Each has one direct predecessor, one taken conditional branch and one
  conditional fallthrough. The corresponding edges are
  `A6BA->A786` / `A66A->A736`, `A78E->A7D4` / `A73E->A784`, and
  `A78E->A792` / `A73E->A742`; topology is unchanged.
- The first instruction changes from raw `2F3C0000A6BE` to
  `2F3C0000A66E`. Its immediate values point to corresponding local decoded
  instruction offsets and are classified `relocation_only`; `4A46` and
  `6B000044` are identical, with branch condition code `0xB` recorded.
- `oasis.m68k.re-diff.v1` now carries bounded block detail, raw instruction
  evidence, addressing modes, condition codes and conservative classifications.
  No semantic meaning is assigned to the pushed address or branch.

## M11.5 fourth checkpoint — retail/beta correspondence
- `oasis_re_diff` remains developer-only and loads both ROMs through `oasis::Rom::load`.
- Beta fingerprint is size `3145728`, CRC32 `FA59F847`, SHA-1
  `cb0606faeab0398244d4721d71cf7e1c5724a9ef` and SHA-256
  `5111d21c8344cce00765b32b971849f62950d31869307cc479f5ee7febf87a80`;
  header and Sega checksum are valid.
- Requested targets yield exact beta analogues `0x3820 -> 0x37D0`,
  `0x60004 -> 0x60004`, `0x82AE -> 0x825E` and `0x7A28 -> 0x79D8`.
  `0xA6A4 -> 0xA654` is structural with one raw changed block.
- Results are only decoded-byte, normalized-signature and CFG evidence; no
  semantic identity or behavior is inferred. Candidate scanning is limited to
  these five signatures/windows and does not add runtime/emulator code or a
  general whole-ROM discovery pass.

## M11.5 third checkpoint — bounded dynamic trace
- `oasis_re_trace` uses a scenario-bounded 68000 interpreter in `oasis_re_tooling`;
  it is not linked into the native gameplay runtime and is not a full emulator.
- The controlled `0xA6A4` scenario starts at `0xA7D4` with raw `A6=FF2954`,
  record word `1` and raw `+0x22=0xA7E4`; it deterministically executes five
  PCs, three blocks, one branch, two RAM reads, one indirect jump and one RTS.
- Runtime evidence newly resolves the prior static `A7D4/A7DE` register-based
  accesses and `A7E2 -> A7E4`; nine other static items remain unresolved.
- The backend deliberately supports only this exact scenario and stops on
  unsupported PCs; no full-game tracing, TAS, whole-ROM discovery or M12 work.

## M11.5 verified RE tooling
- Representative bounded targets are `0x3820`, `0x8E90`, `0xA6A4` and `0xD3B2`;
  only documented exact boundaries at `0x3820..0x3B3E` and `0xD3B2..0xD406`
  are marked confirmed.
- `oasis_re_program` keeps RE tooling separate from `oasis_core`, emits
  deterministic JSON/text, groups direct calls into caller→callee and
  block/instruction edges, and binds memory evidence to function/slice/block.
- USA oracle reproduces `0xD3B2 -> 0x3820`, pool edges `0x8EA6 -> 0x8F12`
  and `0x8EC8 -> 0x8F22`, indirect callback flow at `0xA7E2`, table `0x5CE96`
  and destination `FF2FA8`.
- Register-based references, indirect targets and unsupported decoder coverage
  remain explicit UNKNOWN/unsupported; no gameplay behavior was added.

## M7 verified evidence
- Screen dispatcher `0xC8F0` resolves 16-bit screen IDs through group table `0xC92C` to 26-byte descriptors; native loader has ROM-backed reference tests.
- Auxiliary byte grids at `0xFF1766` / `0xFF1770` use 8-pixel cells with confirmed pointer/stride/shift addressing.
- `0x9C40` is a confirmed single-cell world-grid reader.
- `0x9BF2` aggregates the entity footprint with OR/AND over covered grid bytes; native `ByteGridView` reproduces this contract.
- Low nibble terrain codes map through `0x96E8` / `0x96F8` into movement/height classes.
- `0x938E` is a confirmed directional terrain movement gate: carry set blocks movement, carry clear allows it.
- Synthetic byte-grid/footprint/terrain-gate tests pass.
- Main build-test, reference-ROM workflow and final M7 probe passed on the M7 completion head.

## M8 verified evidence
- Main player initialization selects entity slot `0xFF19E8` and type `2` at `0x13D6..0x142E`.
- Controller normalization writes the movement nibble to `0xFF165E`; `0x85E2` maps it to confirmed cardinal/diagonal fixed-point vectors.
- The shared movement cluster commits positions from `+0x72/+0x76` into `+0x08/+0x0C` and gates the footprint through `0x9BF2`/`0x938E`.
- Native player state, movement mapping and terrain-gated update are implemented with synthetic and local ROM-oracle checks.
- The indirect update path is closed: `0x8B22 -> 0x557A -> 0x5670 -> 0x59BA`, with state `0` entering `0x61F6`.
- State `2` stop and state `4` turning/timeout branches are byte-verified: stop clears `+0x4E/+0x52`, writes `+0x2A=0` and returns to state `0`; shared movement owns `+0x72/+0x76` cleanup, while turning uses `+0x16`, `+0x17`, `+0x72/+0x76` and timeout state `0xC`.
- Native `update_movement_state` now mirrors the confirmed state-2 stop and state-4 axis-selection/accumulation rules; presentation callbacks and the `0x64C4` slowdown context remain outside the API.
- The native state driver is integration-tested through the shared terrain consumer; footprint OR mask `+0x6F` is preserved, while the `0x64C4` globals `FF1985`, `FF1984` and bit 4 of `FF16F1` remain lifecycle-investigating.
- The `0x64C4` context boundary is isolated: `FF1985` and `FF1984` have external writers, while bit 4 of `FF16F1` is read without a direct writer in the scanned references.
- Optional native `VelocityAdjustContext` now covers the three confirmed `0x64C4` arithmetic outcomes; unavailable context preserves the existing deterministic path.
- Frame-boundary ordering is covered: state-4 reads the prior footprint mask, then shared movement updates the mask for the next frame.
- The Windows CTest runtime path is configured in CMake; clean-shell Debug and Release runs pass 11/11 without manual `PATH` edits.

## M9 verified evidence
- The common loop has three raw pool descriptors: `FF2954 × 4` at stride `0x5A`, `FF19E8 × 21` at stride `0xBC`, and `FF2D8C × 6` at stride `0x5A`.
- Active records are selected by signed word `+0x00 > 0`; the loop dispatches through `FF193C` and enters `0x8F12` or `0x8F22`.
- Native `EntityPoolView` and synthetic tests reproduce bounded record addressing and the positive active predicate; local ROM oracle checks the loop constants.
- `EntityRecordView` reads only bounded big-endian raw fields; the local USA-ROM oracle verifies the shared movement accesses and the FF2954 `+0x3A`/`+0x22` dispatch path.

## M10 investigation boundary
- The M10 implementation begins from the caller/data-backed `0x7A10 -> 0x846C` raw entry and slot path.
- Spirit names, semantic targeting, rendering and audio remain unknown until direct ROM evidence is recorded.

## M11 verified evidence
- `0x82AE` accepts a selected `FF19E8` record only when raw type `0x0008`,
  clears that type and transfers raw fields to `FF1976`, `FF1978` and
  `FF197A`; native producer tests and the USA-ROM oracle pass.
- `0x7A28` maps the raw `FF1976` byte to bounded handler addresses, including
  the flag-clear return at `0x7B28`; native routing tests and byte checks pass.
- Type-8 meaning, event-code meaning, stream format, dialogue and progression
  semantics remain unknown. No generic parser has been added.

## Confirmed USA reference fingerprint
- Size: 3,145,728 bytes
- CRC32: `c4728225`
- SHA-1: `2944910c07c02eace98c17d78d07bef7859d386a`
- SHA-256: `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`

## Accepted reference policy
- Canonical engineering reference: clean USA retail `Beyond Oasis`.
- Final reconstructed C++ game model is region-independent.
- Europe/Japan are secondary evidence and future data profiles.
