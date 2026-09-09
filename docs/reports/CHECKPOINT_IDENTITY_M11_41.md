# M11.41 — Checkpoint Identity Provenance and Reproduction Repair

**Status:** `CHECKPOINT_IDENTITY_SERIALIZATION_BUG_PROVEN` and
`CHECKPOINT_BASELINE_IDENTITY_RESTORED`

## Scope and preserved baselines

This milestone started from M11.40 commit
`23aab8997592726a001d1483b7f109c6915f6bd8` and the M11.39 coverage commit
`37857a31a1c2965ecbe68e3695ca5aa187617c2f`. It did not run M11.40 phase 2,
attribute interpreter PCs, expand semantics, promote blocks or change the
generated registry. The historical M11.39 aggregate
`20217e10565c51b571db6a4e474aa40854ea6c22277ba14c46ea97c4b1c60a04` and the
M11.40 negative result remain preserved in their original report.

The controlled inputs were unchanged: canonical ROM SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`, external
GPGX DLL SHA-256
`140c00fc7475cf22ca22415d65dfc7ffc66523de128454f9994e7ab77d9826fd`,
`BASIC_BLOCK_NATIVE`, 600 frames, neutral cold-reset scenario and the 28-range
registry. All generated run directories and `game.srm` remain untracked.

## Exact identity pipeline

The developer-only owner is `src/tools/hybrid/runner.cpp`:

1. `retro_run()` executes frames 1 through 600. At frames 60, 120, ..., 600,
   `retro_serialize_size()` supplies the external state size and
   `retro_serialize()` fills a value-initialized byte vector.
2. The runner computes `raw_state_sha256` over the complete `STATE_SIZE` buffer.
   The authoritative `state_sha256` is computed by
   `checkpoint_identity_hash()` in `checkpoint_evidence.cpp`.
3. The runner appends each lowercase 64-character authoritative hash in frame
   order to `state_hashes`; `hash_text(state_hashes)` computes the aggregate.
   Frame numbers and CPU-cycle values are not aggregate input bytes.
4. The JSONL checkpoint record retains the frame, authoritative hash, raw hash
   and CPU cycles. With `OASIS_CHECKPOINT_EVIDENCE` set, `CheckpointEvidence`
   also writes ignored local `checkpoint_records.bin` as the concatenated raw
   buffers and `checkpoint_manifest.jsonl` with ordinal, frame, offset, size,
   both hashes, aggregate input description and final aggregate.

The external GPGX ownership was verified from `libretro/libretro.c`:
`retro_serialize_size()` returns `STATE_SIZE` and `retro_serialize()` calls
`state_save(data)`. `core/state.c` saves the version, Genesis RAM/IO/VDP/sound,
68000 state, Z80 state and cartridge context in that order. The runner hashes
the full advertised buffer, including zero-filled trailing bytes, rather than
the returned written length.

## Proven first divergence

The historical M11.39 run artifact has no raw checkpoint file, so its exact
20217e aggregate cannot be byte-compared directly. A clean exact historical
checkout run was rebuilt twice before instrumentation and produced the stable
but different aggregate `3923a3d6fbef7677a6c056884cab9107c041112669ddd64f8ec758bfb6d88acf`.
Current pre-repair runs likewise produced stable values that depended on the
process representation (`fffe59fc...`, `57417d1b...`, `cd8727cc...`). This
established that the report value was not reproducible from the committed
M11.39 source under the controlled inputs.

Opt-in raw evidence then compared independent runs. The first differing record
was ordinal 0, frame 60. The first differing byte was serialized state offset
`140654`; the values were `0x62` and `0x36` in one pair of runs. The preceding
state offset `140651` is the start of the first YM2612 `FM_SLOT`, so offset
`140654` is byte 3 of its `DT` pointer. The external source declares that
pointer at `core/sound/ym2612.c:499` and wholesale-saves `YM2612` at
`core/sound/ym2612.c:2246`. The differing value is a host address, not a guest
register, RAM byte, VDP byte, audio semantic value or execution counter.

The same external source also wholesale-saves `FM_CH` pointer fields and
`Z80_Regs`, whose `daisy` and `irq_callback` members are raw host pointers.
The first mismatch is therefore sufficient to explain the aggregate, and the
full audit identified the corresponding pointer and ABI padding spans in the
known `STATE_SIZE=0xfd000` layout. The pointer values can vary with process
address-space allocation; this is the proven cause of the historical/current
identity mismatch. It does not affect gameplay-visible state. Ownership is
external GPGX serialization, with the developer-only runner responsible for
the checkpoint identity adapter.

## Minimal repair and proof

`checkpoint_identity_hash()` copies the raw buffer and zeroes only the proven
host-pointer and ABI-padding spans for the GPGX 1.7.6 `STATE_SIZE=0xfd000`
format. It rejects an unrecognized state size. The raw full buffer is never
modified or discarded, and semantic state bytes remain hashed. A regression
test proves that a pointer-byte change does not change identity while a nearby
semantic byte change does.

Three independent repaired current runs and two repaired runs from the exact
M11.39 checkout all produced:

`c9236218f55fb18f7f1d5095e4970b25f228de588bd03bd7cbbffccc2e225fd1`

Every repaired run also produced video hash
`5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`, total
guest instructions `6,488,773`, translated `5,826,857`, interpreter `661,916`,
translated share `89.7991%`, 140,065 boundary yields and 274 interrupted
resumptions. The five manifests and canonical per-record sequences matched;
raw records remain available only in ignored local evidence.

The authoritative M11.40 restart-gate identity is therefore the repaired
aggregate above. M11.40 phase 2 remains deliberately unstarted and is the next
separate task.

## Validation and hygiene

The new evidence regression passed. The changed Debug/Release/GNU-equivalent
full CTest runs, source-size check, `git diff --check` and repository hygiene
checks are required before push; the final worklog records their exact
results. No ROM, extracted asset, emulator binary, generated run evidence or
`game.srm` is committed.
