# M11.59 — Raw data alias, lifetime and external writer closure

STATUS: **RAW_DATA_ALIASING_BOUNDARY_PROVEN_TYPED_DATA_BLOCKED**

BASELINE: `9d75a836f727989158761d666c112cc0b76d2888`

## Result

The all-ROM census closes the dominant negative boundary: the A5-derived
window cannot be represented as an independent typed record. `A5` is
materialized at `FF001A` by three bounded producers, escapes to the stack,
is reloaded, and is post-incremented through a broad range. Other routines
consume `0(A5)`, `4(A5)` and `7(A5)` on separate paths. This proves aliasing
and lifetime overlap at the boundary of the current raw-parameter cluster.

The fixed bytes also have multiple bounded writers. The sibling `0x60BCC`
writers reproduce the same zero-write order after a hardware prefix, so they
are part of the same raw transaction but remain `HARDWARE_ORDERED_WRITER`
contexts. No typed structure, subsystem, new routine, or `0x60BCC` promotion
was added.

## Baseline reproduction

Two fresh `NATIVE_OVERRIDE` runs and two fresh `SHADOW_NATIVE` runs used the
canonical user-supplied ROM and the unchanged external GPGX library. Every
run retained:

| check | result |
|---|---|
| checkpoint SHA-256 | `251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d` |
| video SHA-256 | `5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58` |
| native accounting | `6,488,773` |
| interpreter remainder | `6,488,692` |
| TableCopy / RamFlag / helper | `34 / 40 / 7` |
| fallback / divergence | `0 / 0` |
| shadow comparisons | `5/5`, zero divergence |

The accounting identity remains `6,488,692 + 34 + 40 + 7 = 6,488,773`.

## Fixed-address all-ROM census

The canonical ROM was scanned with the existing M68K decoder. Raw byte-pattern
hits were retained only when the decoder confirmed an exact absolute or LEA
reference. The census found 66 raw candidates and 60 confirmed references;
six overlapping byte patterns were rejected as data or unsupported decodes.
The confirmed rows are:

| address | confirmed references (PC: exact operation) | observed natural count |
|---|---|---:|
| `FF0010` | `06009A:tst.b`, `06010A:sf.b`, `0604FA:sf.b`, `060BDC:sf.b`, `060D98:sf.b`, `060F3C:st.b`, `061238:sf.b` | `06009A=486`; others bounded/zero in override |
| `FF0011` | `060110:tst.b`, `060164:sf.b`, `060500:sf.b`, `060BE2:sf.b`, `060D9E:sf.b`, `060F74:st.b`, `06123E:sf.b` | `060110=486` |
| `FF0012` | `06016A:tst.b`, `060178:tst.b`, `0602E8:st.b`, `0603EC:sf.b`, `060422:sf.b`, `0604F0:sf.b`, `060BD2:sf.b`, `060D8E:sf.b`, `06112E:tst.b`, `061232:sf.b` | `06016A=486`; `060BD2=3`; `06112E=1` |
| `FF0013` | `0601C8:st.b`, `06029A:tst.b`, `0602A4:sf.b`, `060366:tst.b`, `060370:sf.b`, `060506:sf.b`, `060BE8:sf.b`, `060DA4:sf.b`, `061146:tst.b`, `06116A:st.b`, `061198:st.b`, `061244:sf.b`, `062AE0:tst.b` | `06029A=486`; `062AE0=486`; `061146/16A=1`; `060BD2-side=3` |
| `FF0014` | `060188:tst.b`, `060192:sf.b`, `06050C:sf.b`, `060BEE:sf.b`, `060DAA:sf.b`, `0611D0:st.b`, `06124A:sf.b` | `060188=486` |
| `FF0015` | `0611CA:move.b D0,($00FF0015).L` | bounded writer discovered by census |
| `FF0016` | `060198:move.b ($00FF0016).L,D0`, `0601CE:sf.b`, `0604DE:sf.b`, `0611A4:cmp.b`, `0611AE:move.b D0,($00FF0016).L` | `060198=486`; `0611A4/AE=1` |
| `FF0628` | `0600AA`, `06025A`, `0604BC`, `060614`, `06127A` LEA materializations | `0604C2=3`; `06127A=1` |
| `FF06F2` | `060120`, `060270`, `0604C8`, `06062E`, `061286` LEA materializations | `0604CE=3`; `061286=1` |

For TST/CMP, the existing decoder exposes the operand but reports access as
`unknown`; the exact disassembly establishes these rows as reads. Absence of
a hit does not establish exclusivity. The six rejected raw candidates were
`0601B0`, `060A68`, `060A78`, `166C3A`, `16779A` and `190EE4`.

## Register-relative provenance and lifetime

All exact `FF001A` materializers are LEA instructions:

| PC | operation | natural count | evidence |
|---|---|---:|---|
| `060182` | `lea.l FF001A,A5` | `486` | entry path; reads `5(A5)`, `7(A5)` and writes `7(A5)` |
| `060434` | `lea.l FF001A,A5` | `5` | natural `60434 -> 604F0` parent path |
| `061258` | `lea.l FF001A,A5` | `1` | saves A5, clears `(A5)+` for `0x762` iterations, reloads A5, then continues |

The `061258` path proves the boundary is not a three-byte record: `06125E`
stores A5 on the stack, `061266` executes `clr.b (A5)+` under DBF, and
`06126C` reloads A5. The same path then writes `0(A5)` and `3(A5)` and
materializes `FF0628` and `FF06F2` through A0. Other consumers include
`06193C btst.b 0(A5)` (2,430 observed executions), `061946 move.b D7,4(A5)`
and `061998 move.b 4(A5),(A4)+`. The natural cluster observes A0 at
`FF001F`, `FF0020`, `FF0021` and `FF0022` while A5 remains `FF001A`.

Classification labels are therefore: A (`FF0010..FF0016`) =
`SHARED_STATE_WINDOW`; B (`FF001A..FF0021`) = `ALIASING_PREVENTS_BOUNDARY`;
C (`FF0628`) = `INDEPENDENT_SCALAR`; D (`FF06F2`) = `INDEPENDENT_SCALAR`;
E (merged A+B+C+D) = `INSUFFICIENT_EVIDENCE`. The scalar labels describe
fixed-address access shape only; they do not grant ownership or lifetime.

## External-writer closure

`0x60BD2`, `0x60BDC`, `0x60BE2`, `0x60BE8` and `0x60BEE` are the sibling
fixed-byte writers on the `0x60BCC` path. Their zero writes match the
`0x604F0` suffix order (`FF0012`, `FF0010`, `FF0011`, `FF0013`, `FF0014`),
but the path begins with an `A11100` hardware prefix. Each writer is classified
`HARDWARE_ORDERED_WRITER`; the shared ordering is evidence of a common raw
transaction, not permission to move hardware or sibling continuation into
`oasis_core`. No `0x60BCC` code was promoted.

## Temporal and alias falsification evidence

The existing developer-only natural observer was run twice; both
`caller_continuation.jsonl` files are byte-identical (SHA-256
`8b23fce6088956ecdc443bf432f183a6204eabe6fdf0aeb71dbe9e29087ea877`), with
176 events and a complete, non-truncated header. It records A5=`FF001A` at
`60434`, the `604EA` read/modify/write, the `604F0` `FF0012` write, the two
flag operations, the A0 sequence `FF001F..FF0022`, and the final fixed-byte
writes. This is sufficient repeated bounded evidence; no new runtime
instrumentation was needed.

Alias falsification covered middle pointers (`5(A5)`, `7(A5)`, `0/4(A5)`),
wider overlap from the `061266` post-increment loop, stack save/reload, stored
addresses and alternate A0 bases. The evidence proves overlapping access and
escaping lifetime, while not proving semantic field identity.

## Typed gate and Astra review

The exact gate result is `TYPED_DATA_BLOCKED_OVERLAPPING_ACCESS`. A typed
structure would hide a stack-escaped, post-incremented base and multiple
external consumers/writers. No production/core abstraction was added; the
only source addition is a decoder provenance regression test.

No Astra reviewer is available in this workspace. No external architecture
claim was inferred.

## Validation

* Debug MinGW CTest: `70/70`.
* Release MinGW CTest: `70/70`.
* The new `oasis_raw_data_provenance` test passes in Debug and Release.
* Baseline native/shadow gates: exact hashes, zero fallback/divergence,
  shadow `5/5`.
* `git diff --check`, source line-limit and repository-hygiene checks pass.
* The pre-existing UCRT build/CTest was `69/69`. After registering this new
  test, the configured UCRT compiler cannot compile even a trivial source
  file (exit 1 with no diagnostic), so the new executable is unavailable and
  CTest reports only that test as not run. This is a local toolchain limitation,
  separate from source validation and GitHub CI.

## Next milestone (proposed only)

M11.60 should close one bounded consumer/lifetime contract around the A5
stack-escape and post-increment path, or produce an exact negative for that
single blocker. It must not add a typed structure, widen into gameplay
semantics, or promote `0x60BCC`.

### Confirmed reference ledger

The following machine-readable ledger preserves PC, instruction bytes, decoded operation, memory width, direction, absolute address, confidence, reachability and count. `observed` means the PC executed in the authoritative interpreter profile; `static-only` means it was not reached in that bounded native-override run.

| PC | instruction bytes | op | access width | direction | address | confidence | reachability | count |
|---|---|---|---:|---|---|---|---|---:|
| 0x06009a | `4A3900FF0010` | `tst.b ($00FF0010).L` | 1 | read | `00FF0010` | decoder-confirmed | observed | 486 |
| 0x0600aa | `4DF900FF0628` | `lea.l ($00FF0628).L,A6` | 4 | address | `00FF0628` | decoder-confirmed | static-only | 0 |
| 0x06010a | `51F900FF0010` | `sf.b ($00FF0010).L` | 1 | write | `00FF0010` | decoder-confirmed | static-only | 0 |
| 0x060110 | `4A3900FF0011` | `tst.b ($00FF0011).L` | 1 | read | `00FF0011` | decoder-confirmed | observed | 486 |
| 0x060120 | `4DF900FF06F2` | `lea.l ($00FF06F2).L,A6` | 4 | address | `00FF06F2` | decoder-confirmed | static-only | 0 |
| 0x060164 | `51F900FF0011` | `sf.b ($00FF0011).L` | 1 | write | `00FF0011` | decoder-confirmed | static-only | 0 |
| 0x06016a | `4A3900FF0012` | `tst.b ($00FF0012).L` | 1 | read | `00FF0012` | decoder-confirmed | observed | 486 |
| 0x060178 | `4A3900FF0012` | `tst.b ($00FF0012).L` | 1 | read | `00FF0012` | decoder-confirmed | static-only | 0 |
| 0x060188 | `4A3900FF0014` | `tst.b ($00FF0014).L` | 1 | read | `00FF0014` | decoder-confirmed | observed | 486 |
| 0x060192 | `51F900FF0014` | `sf.b ($00FF0014).L` | 1 | write | `00FF0014` | decoder-confirmed | static-only | 0 |
| 0x060198 | `103900FF0016` | `move.b ($00FF0016).L,D0` | 1 | read | `00FF0016` | decoder-confirmed | static-only | 0 |
| 0x0601c8 | `50F900FF0013` | `st.b ($00FF0013).L` | 1 | write | `00FF0013` | decoder-confirmed | static-only | 0 |
| 0x0601ce | `51F900FF0016` | `sf.b ($00FF0016).L` | 1 | write | `00FF0016` | decoder-confirmed | static-only | 0 |
| 0x06025a | `4DF900FF0628` | `lea.l ($00FF0628).L,A6` | 4 | address | `00FF0628` | decoder-confirmed | observed | 486 |
| 0x060270 | `4DF900FF06F2` | `lea.l ($00FF06F2).L,A6` | 4 | address | `00FF06F2` | decoder-confirmed | observed | 486 |
| 0x06029a | `4A3900FF0013` | `tst.b ($00FF0013).L` | 1 | read | `00FF0013` | decoder-confirmed | observed | 486 |
| 0x0602a4 | `51F900FF0013` | `sf.b ($00FF0013).L` | 1 | write | `00FF0013` | decoder-confirmed | observed | 1 |
| 0x0602e8 | `50F900FF0012` | `st.b ($00FF0012).L` | 1 | write | `00FF0012` | decoder-confirmed | static-only | 0 |
| 0x060366 | `4A3900FF0013` | `tst.b ($00FF0013).L` | 1 | read | `00FF0013` | decoder-confirmed | static-only | 0 |
| 0x060370 | `51F900FF0013` | `sf.b ($00FF0013).L` | 1 | write | `00FF0013` | decoder-confirmed | static-only | 0 |
| 0x0603ec | `51F900FF0012` | `sf.b ($00FF0012).L` | 1 | write | `00FF0012` | decoder-confirmed | static-only | 0 |
| 0x060422 | `51F900FF0012` | `sf.b ($00FF0012).L` | 1 | write | `00FF0012` | decoder-confirmed | static-only | 0 |
| 0x0604bc | `4DF900FF0628` | `lea.l ($00FF0628).L,A6` | 4 | address | `00FF0628` | decoder-confirmed | static-only | 0 |
| 0x0604c8 | `4DF900FF06F2` | `lea.l ($00FF06F2).L,A6` | 4 | address | `00FF06F2` | decoder-confirmed | static-only | 0 |
| 0x0604de | `51F900FF0016` | `sf.b ($00FF0016).L` | 1 | write | `00FF0016` | decoder-confirmed | static-only | 0 |
| 0x0604f0 | `51F900FF0012` | `sf.b ($00FF0012).L` | 1 | write | `00FF0012` | decoder-confirmed | static-only | 0 |
| 0x0604fa | `51F900FF0010` | `sf.b ($00FF0010).L` | 1 | write | `00FF0010` | decoder-confirmed | static-only | 0 |
| 0x060500 | `51F900FF0011` | `sf.b ($00FF0011).L` | 1 | write | `00FF0011` | decoder-confirmed | static-only | 0 |
| 0x060506 | `51F900FF0013` | `sf.b ($00FF0013).L` | 1 | write | `00FF0013` | decoder-confirmed | static-only | 0 |
| 0x06050c | `51F900FF0014` | `sf.b ($00FF0014).L` | 1 | write | `00FF0014` | decoder-confirmed | static-only | 0 |
| 0x060614 | `4DF900FF0628` | `lea.l ($00FF0628).L,A6` | 4 | address | `00FF0628` | decoder-confirmed | static-only | 0 |
| 0x06062e | `4DF900FF06F2` | `lea.l ($00FF06F2).L,A6` | 4 | address | `00FF06F2` | decoder-confirmed | static-only | 0 |
| 0x060bd2 | `51F900FF0012` | `sf.b ($00FF0012).L` | 1 | write | `00FF0012` | decoder-confirmed | observed | 3 |
| 0x060bdc | `51F900FF0010` | `sf.b ($00FF0010).L` | 1 | write | `00FF0010` | decoder-confirmed | observed | 3 |
| 0x060be2 | `51F900FF0011` | `sf.b ($00FF0011).L` | 1 | write | `00FF0011` | decoder-confirmed | observed | 3 |
| 0x060be8 | `51F900FF0013` | `sf.b ($00FF0013).L` | 1 | write | `00FF0013` | decoder-confirmed | observed | 3 |
| 0x060bee | `51F900FF0014` | `sf.b ($00FF0014).L` | 1 | write | `00FF0014` | decoder-confirmed | observed | 3 |
| 0x060d8e | `51F900FF0012` | `sf.b ($00FF0012).L` | 1 | write | `00FF0012` | decoder-confirmed | static-only | 0 |
| 0x060d98 | `51F900FF0010` | `sf.b ($00FF0010).L` | 1 | write | `00FF0010` | decoder-confirmed | static-only | 0 |
| 0x060d9e | `51F900FF0011` | `sf.b ($00FF0011).L` | 1 | write | `00FF0011` | decoder-confirmed | static-only | 0 |
| 0x060da4 | `51F900FF0013` | `sf.b ($00FF0013).L` | 1 | write | `00FF0013` | decoder-confirmed | static-only | 0 |
| 0x060daa | `51F900FF0014` | `sf.b ($00FF0014).L` | 1 | write | `00FF0014` | decoder-confirmed | static-only | 0 |
| 0x060f3c | `50F900FF0010` | `st.b ($00FF0010).L` | 1 | write | `00FF0010` | decoder-confirmed | static-only | 0 |
| 0x060f74 | `50F900FF0011` | `st.b ($00FF0011).L` | 1 | write | `00FF0011` | decoder-confirmed | static-only | 0 |
| 0x06112e | `4A3900FF0012` | `tst.b ($00FF0012).L` | 1 | read | `00FF0012` | decoder-confirmed | observed | 1 |
| 0x061146 | `4A3900FF0013` | `tst.b ($00FF0013).L` | 1 | read | `00FF0013` | decoder-confirmed | observed | 1 |
| 0x06116a | `50F900FF0013` | `st.b ($00FF0013).L` | 1 | write | `00FF0013` | decoder-confirmed | observed | 1 |
| 0x061198 | `50F900FF0013` | `st.b ($00FF0013).L` | 1 | write | `00FF0013` | decoder-confirmed | static-only | 0 |
| 0x0611a4 | `B03900FF0016` | `cmp.b ($00FF0016).L,D0` | 1 | read | `00FF0016` | decoder-confirmed | static-only | 0 |
| 0x0611ae | `13C000FF0016` | `move.b D0,($00FF0016).L` | 1 | write | `00FF0016` | decoder-confirmed | static-only | 0 |
| 0x0611ca | `13C000FF0015` | `move.b D0,($00FF0015).L` | 1 | write | `00FF0015` | decoder-confirmed | static-only | 0 |
| 0x0611d0 | `50F900FF0014` | `st.b ($00FF0014).L` | 1 | write | `00FF0014` | decoder-confirmed | static-only | 0 |
| 0x061232 | `51F900FF0012` | `sf.b ($00FF0012).L` | 1 | write | `00FF0012` | decoder-confirmed | observed | 1 |
| 0x061238 | `51F900FF0010` | `sf.b ($00FF0010).L` | 1 | write | `00FF0010` | decoder-confirmed | observed | 1 |
| 0x06123e | `51F900FF0011` | `sf.b ($00FF0011).L` | 1 | write | `00FF0011` | decoder-confirmed | observed | 1 |
| 0x061244 | `51F900FF0013` | `sf.b ($00FF0013).L` | 1 | write | `00FF0013` | decoder-confirmed | observed | 1 |
| 0x06124a | `51F900FF0014` | `sf.b ($00FF0014).L` | 1 | write | `00FF0014` | decoder-confirmed | observed | 1 |
| 0x06127a | `41F900FF0628` | `lea.l ($00FF0628).L,A0` | 4 | address | `00FF0628` | decoder-confirmed | observed | 1 |
| 0x061286 | `41F900FF06F2` | `lea.l ($00FF06F2).L,A0` | 4 | address | `00FF06F2` | decoder-confirmed | observed | 1 |
| 0x062ae0 | `4A3900FF0013` | `tst.b ($00FF0013).L` | 1 | read | `00FF0013` | decoder-confirmed | observed | 486 |
