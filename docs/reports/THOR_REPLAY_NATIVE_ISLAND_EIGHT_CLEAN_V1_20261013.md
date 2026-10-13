# Clean reproduction of the frozen eight-helper island

## Result

`THOR_REPLAY_NATIVE_ISLAND_EIGHT_CLEAN_V1_20261013` reproduces the frozen
V2 replay from isolated base commit
`907520f04c9cc5a285d4a6fc4670456fa5ec2801` with the minimal island source
overlay. Debug and Release GNU/Linux builds passed. Focused CTest passed 4/4
in each configuration; the telemetry receipt tests passed 2/2 in each.

The clean Debug run completed 3,600 frames. It matched 3,600 canonical frame
records, 60 periodic hashes, 128 architectural sync records, 128 scheduler
sync records, Z80/VDP state artifacts, normalized helper events, external
event order, and per-frame native/fallback counts. There was no replay
divergence. The 18 deterministic replay artifacts match the frozen Debug
reference byte-for-byte; `replay-start-state.json` is the sole mismatch because
its raw cold-start/restore serializer hashes are unstable. The captured
snapshot hash and canonical emulation-state records match. Raw serializer
hashes remain diagnostic only.

The Debug and Release runner hashes match the frozen identities. The pinned
run recorded 45,968 native helper calls and 410,911 ROM fallbacks. The helper
set and its path scopes remain exactly those frozen in V1/V2.

## Reproduction inputs and source closure

The base was committed; the island implementation and tests were applied as
an isolated patch. GPGX 1.7.6, the canonical ROM, and replay session files are
external inputs, each pinned by SHA-256 in the machine receipt. The clean
runner linked 23 `oasis_core` sources. Five unrelated sources in the broader
V2 source census were not needed for this runner and were not built. The
normalized trace receipt requires
`src/tools/hybrid/normalize_step_helper_trace.py`; the V2 dependency inventory
did not list that tool, and this receipt records the omission.

Changed island source files pass the 500-line rule; the clean base checkout's
tracked source scan also passes. The shared primary dirty checkout still has
three unrelated pre-existing over-limit files: `src/tools/re_slice_decoder.cpp`
(506 lines), `src/tools/thor_evidence/audio_mode1_structural_closure.py`
(519), and `src/tools/thor_evidence/helper_candidate_index.py` (518). They
were not part of this isolated reproduction patch.

## Scope and authority

The result is limited to the pinned 3,600-frame development replay and the
eight exact caller paths. It does not establish full CFG coverage, global
helper verification, or production runtime activation. Runtime proven,
structural proven, SOURCE_OWNED, and canonical SQLite authorities are
unchanged; authority deltas are zero.

The frozen V1 and V2 reports are retained as historical evidence. The detailed
machine receipt includes the helper registry, telemetry, build identities,
dependency closure, and replay artifact hashes:
[THOR_REPLAY_NATIVE_ISLAND_EIGHT_CLEAN_V1_20261013.json](THOR_REPLAY_NATIVE_ISLAND_EIGHT_CLEAN_V1_20261013.json).
