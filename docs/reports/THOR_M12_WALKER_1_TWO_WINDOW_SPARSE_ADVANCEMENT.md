# THOR M12 WALKER-1 — Two-window sparse advancement

Date: 2026-09-15

Baseline: `6ee8ac7d599003c35cb48184831d9cc7dcc02a05`

Classification: **PASS_ASM_WALKER_SPARSE_ADVANCEMENT for one bounded block**

This checkpoint tests one transport hypothesis only. It does not implement a
general walker, add AUTO68, change `SOURCE_OWNED`, or migrate C++.

## Selected block and contract

The authoritative QuickSave1 run selected the existing VDP setup path. The
boundary guard is `0x0027CA`; the skipped straight-line body is
`0x0027CE..0x0027E2`; the exit guard is `0x0027E2`. The static contract is:

```text
body is straight-line; A4, A5 and control flow are unchanged;
selected VDP writes are observed separately; no indirect transfer crosses B
```

This is a code-derived contract from the existing ROM-derived block
`build/m12-ref-scan/0027A6_0027FC.asm`. The experiment does not use an observed
value equality as proof.

## Runtime result

Both runs loaded the exact QuickSave1 state, settled three frames, and then ran
120 frames with the two-window Lua source. The run identities were distinct:
`walker1-run-3fac84fb08` and `walker1-replay-3fac84fb08`; each had
`restore_epoch=1`, `capture_id=capture-1`, and `fragment_id=fragment-1`.

| Measure | Result |
|---|---:|
| W1 / W2 windows | 1 / 1 |
| W1 / W2 records | 1 / 2 |
| targeted callbacks | 1 |
| guard callbacks | 3 |
| global exec callbacks | 5 |
| global callbacks in B body | 0 |
| successful contract skips | 1 |
| failed guard skips | 0 |
| selected data callbacks | 2 |
| W2 dependency | executed `0x0027E8`, no producer oracle supplied |
| frame p50 / p95 / p99 / max | 17 / 18 / 19 / 21 ms |
| frames over 33 / 50 ms | 0 / 0 |
| global hook active time | 4 ms |
| hook installation errors | 0 |

The guard at `0x0027CA` disabled `on_bus_exec_any` before the body. The exit
guard at `0x0027E2` observed the body completion and re-enabled it for W2.
`off_pcs=[]` and `callbacks_during_b_off=0`; the body was therefore executed by
the CPU while the global per-instruction callback was absent. W2 then observed
`0x0027E8` and `0x0027EA`, plus two selected writes to `0x00C00004`.

## Cartographer and join gates

The first merge into the fresh MAP-1 sidecar produced `new_nodes=2` and
`new_edges=1`, with no conflicts or frontiers. The stable graph hash is
`f0bedf7218f2c27977c736be834d811a6bf736932b6e13e0810891124e0f5f74`.

Replaying the identical proven bundle produced `new_nodes=0`, `new_edges=0`,
and the same graph hash. Runtime identities remain in the sealed run artifacts;
the durable static graph uses stable identities, so an occurrence does not
pretend to be new semantic knowledge.

The following joins were rejected: different restore epoch, marked gap, equal
numeric value with a different incoming version, violated guard, and different
`run_id`.

## Boundary and limitations

This proves sparse advancement for one small, statically bounded block. It does
not prove that an arbitrary unknown path can be advanced without more sensors,
that the selected memory callback PC is an exact writer PC, or that the method
scales across the ROM. The selected data accesses are retained as runtime
observations; no stronger data-causality claim is made from their callback PC.

The harness returned a valid Lua `result=PASS` and produced the runtime artifact,
but its PowerShell wrapper reported a null process exit code after
`client.exitCode(0)`. This is recorded as a harness limitation, not converted to
a false zero. CMake/CI registration was not added for this developer-only
experiment.

Machine-readable proof: `THOR_M12_WALKER_1_TWO_WINDOW_SPARSE_ADVANCEMENT.json`.
