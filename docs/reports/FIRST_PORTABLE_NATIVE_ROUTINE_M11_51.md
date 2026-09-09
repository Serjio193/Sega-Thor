# M11.51 — First Portable Native Routine Reconstruction

## Result

`PORTABLE_NATIVE_ROUTINE_SHADOW_PROVEN_REPLACEMENT_BLOCKED`

The first routine-shaped portable implementation is complete and independently
shadow-proven. Its authoritative replacement is not promoted because the
current pinned GPGX run fails the frozen M11.50 checkpoint identity gate.

## Baseline gate

Before edits, two unchanged `MECHANICAL_PRIMITIVE_NATIVE` 600-frame runs
matched exactly:

| Measure | Frozen / observed |
| --- | ---: |
| checkpoint aggregate | `251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d` |
| video sequence | `5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58` |
| total guest instructions | 6,488,773 |
| generated translated | 6,199,381 |
| mechanical primitive | 42,384 |
| interpreter | 247,008 |
| registered ranges | 587 |
| boundary yields | 150,135 |
| interrupted resumptions | 288 |
| fallback / divergence / hardware | 0 / 0 / 0 |

The canonical USA ROM and pinned instrumented GPGX identities were unchanged;
no ROM, emulator binary, run evidence or `game.srm` was added to Git.

## Candidate inventory and selection

The inventory used existing exact disassembly, CFG, M11.45 remainder, M11.46
provenance, M11.47 semantic closure, M11.49 family evidence and M11.50
portability boundaries. The selected candidate is the smallest complete
structural leaf:

| Candidate | Evidence decision |
| --- | --- |
| `0x2D66..0x2D84` | selected: closed direct-entry leaf, one local DBF loop, exact natural shadow |
| `0x604BC` | not selected: serialized VDP/sound/bus-refresh contract remains blocked |
| `0x61032` | not selected: serialized VDP/sound/bus-refresh contract remains blocked |
| `0x6121A` | not selected: writes VDP address `0xC00011` |
| `0x3820` | not selected: existing graphics routine has separate prefetch/CCR.X/interrupt contract |
| broad `0x38DA`/`0x3A00` slices | not selected: incomplete semantic closure |

The selected exact form is:

```text
2D66  MOVEM.L D7/A3,-(A7)
2D6A  CLR.W D7
2D6C  MOVE.B (A6)+,D7
2D6E  LEA    $FF134C,A3
2D74  ADDA.W D7,A3
2D76  MOVE.B (A6)+,D7
2D78  MOVE.W (A6)+,(A3)+
2D7A  DBF    D7,2D78
2D7E  MOVEM.L (A7)+,D7/A3
2D82  RTS
```

The exact decoder reports ten instructions, three basic blocks, one direct
back-edge, no calls, no indirect or unresolved edges and no unsupported forms.
The natural 600-frame evidence contains one invocation, 34 dynamic guest
instructions represented by the complete routine, 13 copied words and zero
internal yields or resumptions.

## Contract

The structural name is `TableCopyRoutine`; no gameplay role is assigned.

Entry requires PC `0x2D66`, a bounded A6 ROM/RAM source, an even caller stack
A7 with a readable four-byte return address, and D7/A3 values that are saved
and restored. The first source byte is an unsigned offset and the second is a
DBF count. The routine consumes `2 + 2*(count+1)` source bytes, writes
`count+1` words sequentially at `0xFF134C + sign_extend_word(offset)`, advances
A6, restores D7/A3, consumes the caller return address, advances A7 by four
and exits at that caller PC. The final MOVE.W determines N/Z/V/C; upper SR and
CCR.X are preserved. The observed MOVEM and output write ordering is retained.
No VDP, Z80, I/O, indirect call, self-modifying or other hardware access is
reachable in the bounded CFG.

## Portable extraction

`src/core/table_copy_routine.*` owns only opaque adapter tokens, a portable
register/memory machine interface, structured routine-level control flow and
explicit continuation state. It does not decode opcodes, dispatch ROM PCs or
reference GPGX/libretro, generated blocks, checkpoint serialization or platform
types. The hybrid layer supplies ROM mapping, GPGX prefetch/timing, shadow
comparison and native accounting.

The core executor yields after every routine-level guest instruction. A
synthetic interrupt before DBF resumes at DBF without repeating the prior word
write. Invalid initial and continuation tokens fail closed. The independent
oracle covers zero and multi-word terminal cases, ordered frame/output writes,
CCR.X/N/Z, wrapping arithmetic, interruption/resume and invalid entry.

## Shadow and authoritative results

The unchanged 600-frame `SHADOW_NATIVE` run has one natural call, one portable
comparison and zero divergences. Its checkpoint/video identity remains the
M11.50 identity. The `NATIVE_OVERRIDE` run has one native invocation, 34 native
routine instructions, 13 iterations, zero fallback and total accounting:

| Category | Isolated authoritative run |
| --- | ---: |
| interpreter | 6,488,739 |
| native routine | 34 |
| total | 6,488,773 |

The video hash remains
`5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`, but the
checkpoint aggregate is
`ae8887f5b32a4973a8243775612d68b891b588f6f5dc693558a5a8a2489e5403`, not the
frozen `251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d`.
Checkpoint evidence comparison found four differing raw bytes in the first
post-call checkpoint record. This is a CPU/RAM/serialized-state failure of the
authoritative gate even though video and isolated instruction accounting pass.

Therefore the generated/mechanical/interpreter path remains authoritative
oracle/fallback. No native promotion, timing invention or hardware broadening
was made.

## Validation and next step

Targeted Debug standalone core, CFG, hybrid candidate and dependency-boundary
tests passed. Full Debug, Release and GNU/MinGW-equivalent CTest each passed
64/64, including primitive regression, semantic/CFG/yield/oracle, provenance
and checkpoint tests. Final 600-frame baseline A/B runs matched all frozen
counts and identities; shadow had one comparison and zero divergence. The
native candidate closed its instruction accounting but failed the documented
checkpoint identity gate. Source-limit, `git diff --check` and repository
hygiene checks passed.

The next task must attribute the four-byte state delta to current-pinned-GPGX
CPU/RAM/timing/refresh/continuation state before another replacement attempt.
