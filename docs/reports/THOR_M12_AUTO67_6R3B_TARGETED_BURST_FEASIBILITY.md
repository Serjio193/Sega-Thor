# THOR M12 AUTO67.6R3B — Targeted Burst Feasibility Gate

Date: 2026-09-14
Baseline: `02227960a0ac3c5f028ac5655c981774911ec7d5`
Result: **PASS for the targeted-burst feasibility hypothesis**

This checkpoint tests only the bounded source hypothesis: targeted producer
hooks remain installed, a producer hit installs one global
`event.on_bus_exec_any` hook, the hook records at most 64 execution records,
and it unregisters at the exact consumer or at budget exhaustion. No AUTO68,
graph merging, SOURCE_OWNED promotion, ring increase, native BizHawk change,
or worker redesign was performed.

## Implementation boundary

The new developer-only `capture/predecessor_burst.lua` has two targeted
producer hooks (`0x002234`, `0x0027BE`) and one replaceable global hook slot.
The first producer starts a burst; a second producer coalesces into the same
active burst. The consumer `0x0027EC` freezes the bounded O67P v2 slice and
unregisters the global hook immediately. The runner creates the bounded
sidecar directory for active prehistory modes. Status JSON no longer invents a
path for an evicted slice; only an actually flushed sidecar is exposed.

The existing continuous R2 source remains available as `continuous`; this
checkpoint does not change its 4096-entry ring.

## Real BizHawk matrix

All runs used the canonical Beyond Oasis ROM, QuickSave1, 16 prestarted
workers, 256-event discovery window, 120 frames, and return code 0.

| mode | frame p50/p95/p99/max ms | >16/>33/>50 ms | targeted callbacks | global callbacks | bursts started/completed | budget failures | max/mean records | global duty |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| disabled | 16/17/22/23 | 94/0/0 | 0 | 0 | 0/0 | 0 | 0/0 | 0 |
| targeted idle | 16/17/22/23 | 89/0/0 | 134 | 0 | 0/0 | 0 | 0/0 | 0 |
| targeted burst 64 | 16/18/23/23 | 72/0/0 | 134 | 1862 | 97/97 | 0 | 26/19.195876 | 7.361% |

The final targeted-burst run had 97 consumer hits and 97 exact joins, zero
boundary gaps, zero budget failures, and no active global hook at shutdown.
The bounded sidecar retained and flushed 16 slices. Its files contain no raw
event backlog and are limited to the configured retained proof slices.

Worker/dispatcher counters in the same run: 16 configured, peak busy 16,
340 leases, 340 returns, 36 active collisions, 36 merges, duplicate active
claims 0, queue drops 0, DB errors 0, average seed age 0.00348 s, maximum
seed age 0.016 s. The worker pool was prestarted.

## Canary proof

Representative real sidecar:
`build/auto67-6r3b-targeted-burst64-rerun.capsules/prehistory-00000002.o67p`

It decodes as O67P v2, complete/exact, epoch 0, 27 records, consumer
sequence 27, requested registers A4/A5, ring capacity 64, and no overwrite.
The canonical ROM resolver produced both reaching definitions:

```text
0x002234  opcode 0x4BF9  LEA $00FF134C.L,A5
    ↓ sequence 1 → 27, distance 26, no intervening A5 writer
0x0027EC  opcode 0x3955  MOVE.W (A5),-4(A4)

0x0027BE  opcode 0x49F9  LEA $00C00004.L,A4
    ↓ sequence 12 → 27, distance 15, no intervening A4 writer
0x0027EC  opcode 0x3955  MOVE.W (A5),-4(A4)
```

The consumer register values corroborate A5=`0x00FF134C` and
A4=`0x00C00004`; the production result is derived from the captured ordered
interval and static ROM semantics, not from the external oracle.

## Acceptance

PASS: both exact reaching definitions were recovered from a real BizHawk
targeted burst; the global hook was absent in idle mode; the burst stopped at
the consumer; >50 ms frames were zero and the targeted-burst maximum was
23 ms (below the preferred 33 ms ceiling). The raw-event backlog remains
`0 / NONEXISTENT`.

A final 60-frame real BizHawk verification was also launched with the native
operator window (`build/auto67-6r3b-native-window.json`): return code 0,
16 workers, 168 leases/returns, p50/p95/p99/max 16/19/23/23 ms, and zero
frames over 33 or 50 ms. The operator window path initialized and shut down
cleanly; no browser dashboard was used.

Machine-readable proof: `THOR_M12_AUTO67_6R3B_TARGETED_BURST_FEASIBILITY.json`.
