# Serializer determinism and canonical replay state — 2026-10-13

## Result

For the tested GPGX 1.7.6 `HOOK_CPU` fixture and 3,600-frame session:

- `RAW_SERIALIZER_HASH_STABLE = NO`. Duplicate ROM-only restores differ in
  55 of 55 raw checkpoints.
- `CANONICAL_EMULATION_STATE_HASH_STABLE = YES`. The two ROM-only runs match
  at all 60 canonical checkpoints. The ROM-only and one-call `0x9D70`
  instruction-step replay also match at all 60 checkpoints.
- `SERIALIZER_DETERMINISM = VERIFIED` for the repaired development fixture,
  its versioned fixture state format, and this tested replay scope. This does
  not claim portability of these save states across arbitrary core builds.
- A frame-60 serialize → restore → serialize roundtrip is byte-identical, and
  its 3,600-frame continuation is byte-identical to a matched ROM-only run.

Production activation remains disabled. `0x9D70` remains path-specific; its
global verification status remains NO. Runtime, structural, and source-owned
coverage authorities and canonical SQLite were not changed.

## Reproduction identity

All accepted runs used canonical ROM SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`, the same
GPGX fixture core SHA-256
`ca4be3049e56b1fdfba9a7c727bebbdc645b3d50203d635a468048ba81165134`, the same
runner SHA-256
`af64fc4d25e3ec1fa7c112ed45b39ff619202a28abdd082e6fd8c657eb754a19`, the same
configuration SHA-256 `d82ab08ca4a5b961e847fbb077d5ccc159070a3508c692998b227857958a572b`,
restored initial state SHA-256
`25d32a078bb037671a3cdabc01cd60ea8e71f304e401e593ff995c592a26400b`, and
effective input SHA-256
`a5b2973a6f26b2c29948263e9aa160f05f0f7acc8e2e879407854d6002e24ef4`.
Each run completed 3,600 frames and 3,600 polls.

The duplicate ROM-only runs independently reproduce the old symptom: raw
serialized SHA-256 differs at frames 60 through 3600, while PC fingerprints,
polls, instruction counts, RAM hashes, selected globals, and all canonical
checkpoint hashes match. The first raw mismatch is at frame 60.

## First byte-level difference

At frame 60 the 1,036,288-byte serializer buffers contain 55 contiguous
differing ranges totaling 275 bytes:

- 54 ranges are inside YM2612 host-pointer fields: 24 `FM_SLOT.DT` pointers and
  30 channel connection pointers across six channels.
- The remaining range is `[144575, 144580)`. It crosses one Z80 ABI-padding
  byte at `144575` and four differing bytes of `Z80_Regs.daisy` at
  `[144576, 144580)`. The corresponding eight-byte daisy pointer is part of
  the host representation; only its changed bytes appear in this range.

The exact bytes for every range are in
`THOR_SERIALIZER_BYTE_DIFF_20261013.json`. The corresponding raw buffers are
kept only in ignored local replay artifacts.

The YM2612 serializer saves its containing state wholesale. During load,
`YM2612LoadContext` calls `setup_connection` for all six channels, rebuilding
the channel connection pointers and DT table references from loaded semantic
state. The Z80 daisy pointer is loaded as part of `Z80_Regs`; the tested core
assigns it during Z80 initialization and has no other source use. The adjacent
padding byte has no field semantics. These are host representation bytes, not
gameplay state. The existing layout-based canonicalizer masks only declared
host-pointer and ABI-padding spans; it continues to hash all other serialized
bytes. No arbitrary byte ranges were zeroed.

## Real serializer-state bug found and repaired in the fixture

The first no-op restore experiment on the earlier development fixture exposed
a separate real issue. Its state loader reset the core and recomputed
`m68k.refresh_cycles` instead of restoring the live scheduler phase. That field
is consumed by the instrumented M68K scheduler. The raw state immediately
roundtripped byte-identically, but the next frame diverged in PC and refresh
timing. This was not pointer noise and was not normalized away.

`prepare_scheduler_fixture.py` now adds a tagged `m68k.refresh_cycles` tail to
the isolated HOOK_CPU fixture serializer and reads it back on restore. The
marker preserves compatibility with older fixture states. The repaired
fixture then passed the no-op roundtrip and full continuation comparisons.
This modifies only the generated developer fixture, not the production
emulator serializer or gameplay code.

## Canonical hash and acceptance model

`checkpoint_identity_hash` hashes the complete 1,036,288-byte GPGX 1.7.6
checkpoint after zeroing only the host-pointer and ABI-padding spans declared
by `gpgx_checkpoint_layout`. It rejects an unknown state size or version. Raw
SHA-256 remains recorded as a diagnostic. Replay landmarks now use the
canonical checkpoint hash and still compare the effective poll stream,
per-frame PC fingerprint, instruction count, RAM hash, selected gameplay
globals, watched helper events, and other existing landmarks.

Validation results:

- ROM-only run A vs ROM-only run B: canonical `60/60`; raw `0/60`.
- ROM-only run A vs the one-call `0x9D70` instruction-step replay: canonical
  `60/60`.
- Roundtrip checkpoint before vs after: raw and canonical hashes equal,
  `byte_identical=true`, `diff_ranges=[]`.
- ROM-only full-frame trace vs roundtrip continuation: `3600/3600` exact.
- Both Debug and Release Linux builds passed for the runner and focused
  checkpoint tests; the evidence and layout tests passed in both
  configurations. Python fixture tooling passed `py_compile`.

`0x9D70_REPLAY_NATIVE_ROUTE_EQUIVALENT` remains scoped to the selected call and
observed replay surface. No additional helper was enabled or evaluated.

## Machine artifacts

- `THOR_SERIALIZER_CANONICAL_REPLAY_20261013.json`: identities, counts,
  canonical comparisons, roundtrip, and status values.
- `THOR_SERIALIZER_BYTE_DIFF_20261013.json`: byte-level ranges and component
  classifications.
- Ignored replay outputs under
  `build/9d70-step-diagnostic-20261013/serializer-accepted-*` retain raw
  checkpoints, manifests, and full-frame traces for local reproduction.

## Next step

With the duplicate-run canonical validator stable for this fixture, resume
multi-helper native-island validation as a separate task. Keep raw hashes as
diagnostics and do not broaden the current `0x9D70` route claim.
