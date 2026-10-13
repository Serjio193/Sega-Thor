# Independent gameplay replay B for the eight-helper island (2026-10-13)

## Result

Replay B is a second deterministic 3,600-frame cold-start gameplay trajectory
using the same canonical ROM, GPGX core, runner, and replay configuration as
Replay A. Its effective-input hash differs from A. The effective button stream
first differs at poll 31 (frame 30); canonical observable memory first differs
at frame 121; the M68K PC fingerprint first differs at frame 661. At frame 1,
the full canonical CPU/scheduler/observable-memory record is identical to A.

Replay B exercises a different callable profile. Its census contains 82 target
entries with 143,881 aggregate calls, compared with A's 283 targets and
643,046 aggregate calls in the retained census. B reached `0xB730` 2,264 times
from caller `0x03B448`; A did not. B did not enter any of the eight island
helpers. Therefore the enabled-island replay is a valid fail-closed trajectory
check, but it contributes no new native helper execution or path coverage.

## Reproduction and acceptance

The captured B initial-state snapshot and effective input log are pinned by the
manifest. Two ROM-only diagnostic replays produced byte-identical full-frame
state, PC landmarks, ordered call logs, call census, and auto-capture outputs.
Each has 3,600 canonical CPU/scheduler/observable-memory records and 60 matching
60-frame checkpoint records. A separate strict ROM-only replay passed all
3,600 frames and 3,600 input polls.

With the eight-helper step-set enabled, diagnostic replay matched the ROM-only
B full-frame state file exactly, all 60 checkpoint records, landmarks, calls,
census, and auto-capture outputs; the divergence receipt reports zero differing
frames. A separate strict enabled-set replay passed. Metrics record zero native
calls and zero island target entries, so this is not evidence of native
substitution on B.

The canonical observable-memory digest covers WRAM, Z80 RAM, VRAM, CRAM, VSRAM,
VDP registers, and SAT memory. The per-frame scheduler record includes M68K
cycles/IRQ state, Z80 cycles/PC/IRQ state, VDP cycles, and scanline. Full Z80
architectural register state is not separately exposed by this capture schema
and is not claimed here. Raw serialized-state hashes remain diagnostic because
the accepted serializer representation is unstable across restores.

## Eight-helper cross-replay census

| Helper | Replay A native / ROM fallback | Replay B target entries | B accepted / unsupported | New B path | B timing / IRQ / refresh |
|---|---:|---:|---:|---|---|
| `0x9D70` | 184 / 2 | 0 | 0 / 0 | NONE OBSERVED | NONE OBSERVED |
| `0x9D4E` | 113 / 0 | 0 | 0 / 0 | NONE OBSERVED | NONE OBSERVED |
| `0x9DB4` | 507 / 0 | 0 | 0 / 0 | NONE OBSERVED | NONE OBSERVED |
| `0x9D92` | 24 / 208 | 0 | 0 / 0 | NONE OBSERVED | NONE OBSERVED |
| `0x94A2` | 66 / 0 | 0 | 0 / 0 | NONE OBSERVED | NONE OBSERVED |
| `0x9C40` | 24 / 3 | 0 | 0 / 0 | NONE OBSERVED | NONE OBSERVED |
| `0x8EF8` | 3 / 11 | 0 | 0 / 0 | NONE OBSERVED | NONE OBSERVED |
| `0xE268` | 45,047 / 410,687 | 0 | 0 / 0 | NONE OBSERVED | NONE OBSERVED |

For A, path IDs and cycle-vector counts remain those in the frozen V2 receipt:
one observed path for each of `0x9D70`, `0x9D4E`, `0x9DB4`, `0x9D92`, `0x9C40`,
and `0x8EF8`; three for `0x94A2`; and one sampled path for `0xE268` (17-call
instruction sample). B has no helper path, timing vector, refresh event, or IRQ
pause/resume to add. Synthetic path coverage remains as in V2; full CFG coverage
remains NO for every helper.

## Pinned identities

- Replay B ID: `THOR_REPLAY_NATIVE_ISLAND_EIGHT_REPLAY_B_SCRIPTED_V1_20261013`
- ROM SHA-256: `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`
- GPGX core SHA-256: `ca4be3049e56b1fdfba9a7c727bebbdc645b3d50203d635a468048ba81165134`
- Runner SHA-256: `53b7f61d5c6bbdadd890364b4affe4b0198c91c274859ce5b8354082dc7e6485`
- Config SHA-256: `32340d7cf5593502616580635857f1df1cf8e883b6f1a1dc6a5da1666f136104`
- Replay B effective-input SHA-256: `1084323624fe29c59c235c958a8df886466b407b8bc122e29b66a6c5487cbeb1`
- Replay B initial snapshot SHA-256: `adede3f681bd1c9d5767734f2829ffb5f133b7ae3d56110b68f4e55ac401fc1b`
- Canonical full-frame record SHA-256 (ROM-only and island): `a175759f5c4a6519dc0e031959256eae2fb192cb4f638ef6ee6145aeea92b627`
- Calls SHA-256: `c0ce8c894c67b284e6554fd5392d404496e68b0c160035be3ace2704bd58707d`
- Census SHA-256: `8ddaccca25902b94dee464bd250ff5b9e82837bee3e1f9319fd462b30a90bde4`

## Status and recommendation

`REPLAY_NATIVE_ISLAND_VALIDATED=YES` across two deterministic trajectory
fixtures, with the important qualification that Replay B routed zero native
calls. Scope is the pinned A and B development sessions only. Global helper
verification remains NO; production runtime activation remains NO; full natural
CFG coverage remains NO. Runtime, structural, SOURCE_OWNED, and canonical
SQLite authorities are unchanged.

Next priority: acquire Replay C with a distinct recorded input trajectory that
naturally enters one or more accepted island caller paths, then compare its
path/timing coverage before considering island expansion. Do not expand the
helper set or design production activation based on B's zero-call island pass.
