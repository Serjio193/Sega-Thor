# THOR M12 AUTO67.6R2 — Continuous Prehistory Ring Checkpoint

Date: 2026-09-14
Baseline: `3890af1f2e76dcd2eda615018afecc92c14a9cce`
Run: `build/auto67-6r2-rerun.json`
Database: `build/thor-evidence/auto67-6r2-rerun.sqlite`

## Result

The bounded evidence objective is PASS. A real BizHawk session produced exact
execution joins, frozen prehistory slices, and the first generic runtime
register reaching-definition steps. The gameplay-performance objective is
FAIL for this implementation: the global Lua execution callback makes the
emulator unusably slow. This is recorded as a negative performance result;
there is no claim that the current hook is suitable for normal gameplay.

No AUTO68 work, semantic graph merging, or SOURCE_OWNED change was made.

## Implementation boundary

`predecessor_capture.lua` installs one global `event.on_bus_exec_any` callback.
It retains only compact epoch/sequence/frame/PC records in a bounded ring and
freezes at most 16 bounded O67P v2 slices. `live_capsule.lua` joins a discovery
BUS_WRITE to the next exact execution occurrence of the same PC and exposes
the slice through the existing status/capsule path. Workers decode frozen
execution PCs against the canonical ROM. BizHawk's callback second argument is
a bus value, so it is not treated as an opcode.

O67P v2 header: 72 bytes after the `O67P` magic, 17 little-endian `uint32`
fields. Each record is 28 bytes: epoch, execution sequence, frame, PC, bus
value/opcode slot, A4 and A5 (`7 * uint32`). Ring capacity is 4096.

## Real BizHawk proof

The launcher used 16 prestarted workers, the exact QuickSave1 savestate, the
canonical ROM, AUTO67 capsule mode, and the native operator window. Process
return code was 0. The 120-frame session took 77.203 seconds.

The global ring observed 1,126,282 execution records and overwrote 1,122,186
old records. It wrapped as expected. There were 12 exact consumer joins, 12
frozen/flushed slices, 24,576 frozen records, zero gaps, and zero truncations.
The raw-event backlog remained `0 / NONEXISTENT`.

## Canary: `BUS_WRITE_PC 0x0027EC -> 0xC00004`

The persisted row contains the static facts for `MOVE.W (A5),-4(A4)` with
opcode `0x3955`, plus two independently ordered runtime reaching-definition
steps:

```text
0x002234  LEA $00FF134C.L,A5
    runtime epoch=0 sequence=281852 frame=30
    ↓ distance 26, complete interval, no intervening A5 writer
0x0027EC  MOVE.W (A5),-4(A4)

0x0027BE  LEA $00C00004.L,A4
    runtime epoch=0 sequence=281863 frame=30
    ↓ distance 15, complete interval, no intervening A4 writer
0x0027EC  MOVE.W (A5),-4(A4)
```

The consumer occurrence is exact at sequence 281878, frame 30. The producer
opcode bytes are marked `STATIC_ROM_PC`, because the live callback does not
provide a reliable opcode value. The runtime PC, epoch, sequence, register
values at the consumer, and complete bounded interval are live evidence.

## Runtime and worker metrics

| Metric | Result |
|---|---:|
| frames | 120 |
| workers configured/started | 16 / 16 |
| peak busy / working | 16 / 16 |
| leases / returns | 80 / 64 |
| active collisions | 48 |
| duplicate active claims | 0 |
| known rejected before dispatch | 0 |
| merges | 48 |
| materialized chains | 48 |
| materialized causal facts | 12 |
| materialized chain steps | 7 |
| register provenance resolved / unresolved | 7 / 4 |
| predecessor slices / records | 12 / 24,576 |
| ring capacity / overwrites | 4,096 / 1,122,186 |
| average / maximum seed age | 7.631 / 17.516 |
| queue drops / DB errors | 0 / 0 |
| raw backlog | 0 / NONEXISTENT |

## Performance acceptance

The frame-time proof is deliberately retained because it is the important
negative result for the current design. All 120 frames exceeded 16, 33 and 50
ms. The maximum was 679 ms. The first frame measured 662 ms with no leases, so
the cost is not caused only by dispatch bursts; it is the per-instruction Lua
callback path itself. Increasing ring capacity would not reduce callback
frequency and would increase frozen-snapshot work, so the ring remains 4096.

## Tests

- `python tests/thor_evidence_auto67_6_test.py -q` — 17 passed.
- `python tests/thor_evidence_auto67_test.py -q` — 10 passed.
- `python -m unittest discover -s tests -p 'thor_evidence_auto67*.py' -q` — 48 passed.
- Debug and Release builds — passed.
- Debug and Release CTest — 189/189 passed each.
- Source-limit gate — 647 governed files, all at or below 500 lines.
- `git diff --check` — passed.
- Real BizHawk/native-window rerun — return code 0.

The full project validation and publication checks are recorded separately in
the final commit process. Unrelated user-generated untracked artifacts were
not staged or removed.
