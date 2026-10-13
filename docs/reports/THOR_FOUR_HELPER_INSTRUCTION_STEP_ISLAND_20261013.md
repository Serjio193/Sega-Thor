# Four-helper instruction-stepped replay island — 2026-10-13

## Result

`REPLAY_NATIVE_ISLAND_VALIDATED=YES` for the pinned 3,600-frame development
session and exact caller paths `0x9D70@0x9144`, `0x9D4E@0x909E`,
`0x9DB4@0x92CC`, and `0x9D92@0x9812`. Production activation and global C++
verification remain NO.

## 0x9D92 step path

The canonical ROM bytes at `[0x9D92,0x9DB4)` are
`3E02 DE6E 0046 3628 0006 E74B 5343 9647 B043 6300 000C 3003 6606 002E
0048 0041 4E75`. The accepted BSR.W caller is `0x9812`, return PC `0x9816`,
and the RTS is at `0x9DB2`. The runtime adapter rechecks the ROM identity,
source instruction and stack return, bounded WRAM inputs, instruction bytes,
and the selected stepper contract embodied by the runner build identity.

The 24 natural calls followed one path:

`9D92 → 9D94 → 9D98 → 9D9C → 9D9E → 9DA0 → 9DA2 → 9DA4 (BLS taken) → 9DB2 (RTS)`.

This path used opcodes `3E02, DE6E, 3628, E74B, 5343, 9647, B043, 6300,
4E75`. Core opcode-table base charges are 4, 12, 12, 6, 4, 4, 4, 10 and
16 68000 cycles. `LSL.W #3` adds six cycles (three shifts at two cycles each).
At the core's factor of seven, the nominal complete call costs 78 cycles or
546 master-cycle units. The observed call totals were 546, 560 and 637 across
seven distinct per-instruction cycle vectors. The differences are core refresh
interactions; refresh checks and scheduler yields remain in the core. No
refresh was pending at the RTS boundary. The decoder audit confirms `DE6E` is
`ADD.W`, and the shift's dynamic term is charged at that instruction boundary.

The input corpus contained 12 distinct coordinate pairs, three D0 inputs and
12 D2 inputs. BLS was taken on all 24 calls. The BNE at `0x9DAA` was not
reached, so this receipt does not claim natural coverage of the complete CFG.
The other bounded branches remain supported by the accepted C++ oracle
contract, but were not exercised as instruction-stepped paths here.

## Replay results

The ROM-only run and both native runs matched the recorded 3,600-frame
landmark sequence and each other. All 3,600 full-frame architectural,
scheduler, and observable-memory records matched. The 60 periodic landmarks,
128 architectural synchronization records, 128 scheduler synchronization
records, all 128 full Z80-RAM snapshots (1 MiB), normalized helper events,
and ordered external events matched.

The 0x9D92-only run routed 24 accepted calls to native stepping. Its other 208
entries came from caller `0x9226` and correctly remained ROM-backed. There
were no precondition or contract rejects. In the four-helper run, native call
counts were 184 (`0x9D70`), 113 (`0x9D4E`), 507 (`0x9DB4`) and 24 (`0x9D92`),
for 828 total. The 2 unsupported `0x9D70` entries and 208 unsupported
`0x9D92` entries fell back to ROM: 210 fallbacks total. There were 210
provenance rejects, zero precondition rejects, zero contract faults, zero
native mismatches and no first divergent frame.

The separate helper timing trace marks its selected `0x9D70` event as native
in the four-helper run, while the ROM-only trace marks it as ROM. That one
diagnostic boolean is expected; all timing and architectural fields in that
event match.

## Evidence and authority

This is replay/portability evidence for the pinned ROM, core, runner build,
initial snapshot, input log and 3,600-frame session. It does not extend global
caller coverage, promote any helper to global verification, or enable
production dispatch. `RUNTIME_TOTAL_PROVEN=440425`,
`STRUCTURAL_PROVEN=1266667`, and `SOURCE_OWNED=1487672` are unchanged; no
canonical SQLite or ROM classification changed.

Machine receipt: `THOR_FOUR_HELPER_INSTRUCTION_STEP_ISLAND_20261013.json`.
