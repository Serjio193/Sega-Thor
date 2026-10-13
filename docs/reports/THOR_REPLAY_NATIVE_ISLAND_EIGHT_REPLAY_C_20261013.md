# Eight-helper native island — Replay C

## Result

Replay C is a deterministic 3,600-frame trajectory that shares Replay A's
fallback-input prefix through frame 2,299, then applies a recorded directional
and action schedule. Its first changed effective input is at frame 2,300. The
changed inputs produce a distinct observable WRAM state beginning at frame
2,341, while PC fingerprints, CPU state, scheduler state, callable-target
distribution, accepted helper inputs, and sampled helper paths remain the same
as Replay A.

The ROM-only C capture and a second ROM-only C replay are byte-identical across
full-frame state, landmarks, calls, callable census, full-call captures,
scheduler sync, normalized events, external events, and helper timing. The
strict ROM-only replay passes. The frozen eight-helper step replay matches the
C ROM-only run byte-for-byte on the full-frame records and the compared
scheduler, landmark, normalized-event, and external-event outputs. No helper
contract was broadened.

This validates the frozen island on a second deterministic input trajectory,
but it adds **no natural native-path evidence**: the native helper call counts,
accepted caller paths, and path/timing samples are exactly those seen in A.
Replay B remains separately qualified as routing-only, with zero island entries.

## Replay identity

| Field | Value |
|---|---|
| Replay C ID | `THOR_REPLAY_NATIVE_ISLAND_EIGHT_REPLAY_C_DIVERGENT_V1_20261013` |
| ROM SHA-256 | `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263` |
| Core SHA-256 | `ca4be3049e56b1fdfba9a7c727bebbdc645b3d50203d635a468048ba81165134` |
| Runner SHA-256 | `d524b7d6aece6bab7b5a58330d4ae56bbe3367a6f2bee58388cff5f0efc75039` |
| Config SHA-256 | `5921a426c45f076645a517693ccc3332b989e39aa47647a2056cded29db80180` |
| Effective input SHA-256 | `93f6816e6f114fead5089a41a9ed6bfedd44147397daa7be494e64550cceb49b` |
| Initial snapshot SHA-256 | `f5cdb6f070430855d6f0ffd02462491db451ae0b543a030734b882c64ee0901f` |
| Input polls | 3,600 |
| Canonical ROM size | 3,145,728 B |

The C effective input first differs from A at poll index 2,302 (one-based),
corresponding to frame 2,300. The earliest canonical observable-memory record
differs at frame 2,341. Full-frame comparison found 56 differing records;
all 56 differences are in the `work_ram` component hash. CPU registers,
scheduler state, PC fingerprints, Z80 RAM, VRAM, CRAM, VSRAM, VDP registers,
and SAT hashes match A at every frame. This proves a distinct observable WRAM
trajectory; it does not identify the owning game-state field or establish a
new helper/control-flow path.

## ROM-only determinism

Two independent ROM-only C diagnostic captures matched exactly:

| Artifact | SHA-256 | Result |
|---|---|---|
| Full-frame state | `7fceca5059e01ea814551ef986d0b419e637e06e858b5d5b98ef0760482c2e89` | identical, 3,600 records |
| 60-frame landmarks | `8dad9412b2fdd1cf72e8162e197c66a0a453d7af4001afa343d3647f3cbef458` | identical |
| Calls | `ed664fa85f29d0e5eee03ce1bb9d61a4fb0e3b43c6633c749e629f85c5dc0584` | identical |
| Callable census | `79b1987f4a1c5dfa0d4bbe63e382899c60b68393e0e9270f1dd311a82eff04c2` | identical |
| Full-call captures | `108a9001e935f1b79d4b65041516fefbc6ef3179811eef90c45798d4de35368a` | identical |
| Scheduler sync | `a4333b5db16f8349e90f085cf4beec2e2b637bf92d8769b8e972fc87e413473b` | identical |
| Normalized events | `0657f588eb94b506ff52327968478447e4288cf1ab028f7b6cfe6a59f051de33` | identical |
| External events | `e5e3ceeb383517f914673ce1bb9d61a4fb0e3b43c6633c749e629f85c5dc0584` | identical |

The strict ROM-only replay reports `REPLAY_PASS frames=3600 polls=3600`.
Raw serializer fingerprints remain diagnostic; canonical state and replay
artifacts are the acceptance surface.

## Helper coverage

Replay C's ROM-only callable census matches A exactly: 283 unique callable
targets and 643,046 aggregate target entries. The frozen island routes 45,968
calls natively and 410,911 to ROM, matching its accepted A counts. C introduces
no new accepted caller, path ID, sampled input/output signature, timing vector,
refresh variant, or IRQ interleave. Uninstrumented per-helper IRQ/refresh
fields remain unknown, not zero. Full natural CFG coverage remains NO for all
helpers.

| Helper | Accepted caller | C native / ROM fallback | C path evidence | New coverage from C |
|---|---:|---:|---|---|
| `0x9D70` | `0x9144` | 184 / 2 | same single sampled path as A | none |
| `0x9D4E` | `0x909E` | 113 / 0 | same single sampled path as A | none |
| `0x9DB4` | `0x92CC` | 507 / 0 | same single sampled path as A | none |
| `0x9D92` | `0x9812` | 24 / 208 | same sampled path as A; other caller remains ROM | none |
| `0x94A2` | `0x9468` | 66 / 0 | same three sampled paths as A | none |
| `0x9C40` | `0xE376` | 24 / 3 | same sampled path as A; other caller remains ROM | none |
| `0x8EF8` | accepted caller path | 3 / 11 | same sampled path as A | none |
| `0xE268` | `0x359A` | 45,047 / 410,687 | same accepted path and timing census as A | none |

The route trace and older target-entry census differ by one `0x8EF8` entry;
the frozen accepted route receipt is retained as the source for routed counts.
The E268 step-metric target-entry total likewise differs from the older call
census by 78. Both discrepancies are unchanged between A and C and are not
reinterpreted here. Frozen accepted aggregate routed totals remain authoritative
for this report.

## Eight-helper C replay

With the eight frozen helpers enabled, the diagnostic replay completed 3,600
frames with no divergence. Its full-frame-state file is byte-identical to C's
ROM-only file. Scheduler sync, observed landmarks, normalized events, and
external events are also byte-identical to C ROM-only. E268 step metrics match
the A baseline. Unsupported caller paths remain ROM-backed.

The raw strict validator reports `SESSION_FAIL_CLOSED: target call capture
mismatch` for the native run. Native instruction-stepping changes the raw call
capture representation; this check is not the accepted native-island
comparison surface. The normalized diagnostic replay and canonical state
comparisons pass. This raw-capture failure is retained as a limitation rather
than described as a strict native `REPLAY_PASS`.

## Status and scope

- Replay C ROM-only repeatability: **PASS**.
- Replay C frozen eight-helper diagnostic replay: **PASS**, zero divergence.
- New island caller/path/timing evidence from C: **NO**.
- Replay A: native-covered baseline.
- Replay B: deterministic alternate trajectory, routing-only; zero island entries.
- Replay C: different observable WRAM trajectory, native-covered on the same path surface as A.
- Replay-native island: validated across A and C; B remains a separate routing-only result.
- Global helper verification: **NO**.
- Production runtime activation: **NO**.
- Full natural CFG coverage: **NO**.
- Runtime/structural/SOURCE_OWNED/canonical SQLite authority deltas: **0**.
- Totals unchanged: ROM 3,145,728 B; runtime 440,425 B; structural 1,266,667 B; SOURCE_OWNED 1,487,672 B.

Recommendation: acquire a further natural trajectory aimed at changing helper
inputs or path/timing coverage before expanding the island. Replay C supports
cross-trajectory replay stability but does not broaden its native evidence.
