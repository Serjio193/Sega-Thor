# Project State

CURRENT_MILESTONE: M11 — Scripts/events/dialogue
CURRENT_TASK: M11.5 first bounded ROM Atlas prototype
STATUS: COMPLETE
LAST_VERIFIED_RESULT: Deterministic oasis.m68k.re-atlas.v1 maps 13 bounded evidence entries, accepted beta correspondences, calls/refs, raw A6A4 dynamic facts and no conflicts; USA/Beta oracle, Debug/Release/GNU CTest and JSON determinism pass
NEXT_ACTION: Stop at the verified Atlas checkpoint; await explicit instruction
DO_NOT_WORK_ON: M12+, Thor 2, Saturn support, remaster features, speculative dialogue or event semantics
BLOCKERS: none

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
