# M12 shadow field direct-access closure (2026-09-12)

Baseline for this pass: published `7212e7a1929b5817c4f23e97dc5c9c125bd53e44`.
The required `run.ps1 -RunName published-baseline` was run first and returned
`result=PASS`. No ROM, savestate, asset or guest RAM was changed.

## Bounded method

Pass 1 consumed the existing payload-free `build/gpgx-classified.json` census
(`14,621` decoded instructions, SHA-256
`64575C7C10A2880FBEE900EE85D262F933B0429C731A3F761A95BA5D4704028E`). The
new `src/tools/m12_shadow_sat_access_graph.py` matches only decoded absolute
operands `(\$00FF1858).L`, `(\$00FF188A).L` and `(\$00FF188C).L`, then records
direction, width and operation type. It does not treat raw byte coincidences,
register-indirect addresses or unsupported/data bytes as opcodes.

Pass 2 closed the producer and nearby cursor-update boundaries and used one
small writer-only runtime probe to resolve the specific question left by the
static graph: which stores account for `0000 -> 0010` and `0000 -> 0080`.
The probe loaded QuickSave1 through Lua, settled three neutral frames, then
advanced one frame. Right and neutral arms were both run; their writer traces
were identical. This is supporting evidence only and does not claim input
causality.

## Complete direct absolute census

The decoded census contains six direct accesses to `FF1858` (two readers and
four byte writers), 34 to `FF188A` (14 readers and 20 writers), and 29 to
`FF188C` (14 readers and 15 writers).

`FF1858`:

| PC | operation | enclosing boundary |
| --- | --- | --- |
| `A358` | `TST.B` read | producer `A342..A436` |
| `A4F2` | `TST.B` read | sibling setup `A4C8..A69C` |
| `CA7C` | `SF.B` clear to `00` | caller/function boundary unresolved |
| `3C14A` | `ST.B` set to `FF` | state routine boundary unresolved |
| `3C328` | `ST.B` set to `FF` | bounded body `3C262..3C452` |
| `3C6E4` | `ST.B` set to `FF` | state routine boundary unresolved |

`FF188A` readers are at `970`, `1CDE`, `1CF8`, `1D12`, `1D4E`, `1F8E`,
`3F26`, `416E`, `A34E`, `A91C`, `AB56`, `AED8`, `C766`, and `3B370`; all
are word reads. Writers are:

| PCs | operation | width |
| --- | --- | --- |
| `3F8`, `1FCA`, `332A`, `3BF9E` | copy from `D0` | longword |
| `1CCE`, `1CEA`, `1D04`, `1D3E` | increment `+1` | word |
| `4018`, `425C`, `4278`, `A430`, `A98E`, `ACEC`, `B006`, `C810`, `3B476` | copy from register | word |
| `A4D0` | increment `+6` | word |
| `A4E8` | increment `+3` | word |
| `CD14` | clear to `0` | longword |

`FF188C` readers are at `94A`, `950`, `1C52`, `3F20`, `4168`, `A2AC`,
`A348`, `A60A`, `A772`, `A932`, `AB50`, `AED2`, `C760`, and `3B36A`; all
are word reads. Writers are:

| PCs | operation | width |
| --- | --- | --- |
| `1CD4`, `1CF0`, `1D0A`, `1D44`, `A994` | increment `+8` | word |
| `4012`, `4256`, `4272`, `A42A`, `ACE6`, `B000`, `C80A`, `3B470` | copy from address register | word |
| `A4C8` | add `+0x30` | word |
| `A4E0` | add `+0x18` | word |

The four longword writes to `FF188A` write bytes `FF188A..FF188D` and
therefore overlap the adjacent word field `FF188C` at bytes `FF188C..FF188D`.
No byte or longword direct access starts at `FF188C`; no decoded direct access
aliases either field through a different absolute operand.

## Boundary and value closure

The exact producer boundary is `A342..A438` (return at `A436`), with direct
callers `A19C` and `A6A0`. `A348` adds the signed word offset from `FF188C` to
`A5`; `A34E` reads the word counter from `FF188A`; `A42A` writes the final
cursor offset and `A430` writes the final counter. Six records in the first
loop advance the cursor by `0x30` and the counter by six. The sibling root
setup `A4C8..A69C` is separate and has no direct call edge into `A342`.

The exact small helper boundary containing `1CCE..1D12` is `1C48..1D2E`; the
`1D30..1D58` helper contains `1D3E/1D44` and returns at `1D58`. The wrappers
`4250..426A` and `426C..4294` are bounded and feed `A6A0`. The `C756..C81A`
body contains `C760/C766/C80A/C810`. The `3B358..3B484` body contains
`3B36A/3B370/3B470/3B476`. Other entries have a known terminating `RTS` or
dispatcher context but no cheap, unique entry boundary; those edges remain
explicitly unresolved below.

## Exact accounting for the controlled transition

The writer-only probe observed the same sequence in both Right and neutral
arms on frame `2120` (the same sequence also occurred during the settle frame):

```text
A42A: MOVE.W A5,(FF188C)  -> 0030   (callback PC A430)
A430: MOVE.W D5,(FF188A)  -> 0006   (callback PC A436)
B000: MOVE.W A1,(FF188C)  -> 0060   (callback PC B006)
B006: MOVE.W D2,(FF188A)  -> 000C   (callback PC B00C)
ACE6: MOVE.W A1,(FF188C)  -> 0080   (callback PC ACEC)
ACEC: MOVE.W D2,(FF188A)  -> 0010   (callback PC ACF2)
```

BizHawk reports the next fetch PC in the bus-write callback; the exact store
PCs above are the statically decoded instruction immediately preceding that
callback PC. Therefore the final controlled values `FF188A=0010` and
`FF188C=0080` are accounted for by `ACEC` and `ACE6`, respectively. The
earlier `A42A/A430` values are intermediate producer cursor/counter values,
and `B000/B006` are a later intermediate update. The initial `1FCA` longword
clear is also visible during state settling.

## Closure result and stop

Direct absolute opcode-level closure over the decoded census is complete. The
remaining unresolved edges are (1) register/indirect aliases, (2) unexecuted
or `DECODE_UNSUPPORTED` bytes outside the census, (3) unique entry/caller
boundaries for the dispatcher-heavy groups around `CA7C`, `3C14A`, `3C6E4`,
`1F8E/1FCA`, `CD14`, and the `A9AA..B010` shared body, and (4) the higher-level
call/continuation edges that reach `ACE6/ACEC` and `B000/B006`. A third xref
sweep would repeat the exhausted method, so this investigation stops after
the two materially different passes.

No semantic animation/object/frame/piece name is assigned and no
`SOURCE_OWNED` promotion is justified. Ownership remains
`1,475,368 / 3,145,728 = 46.9006856283%` (delta `0`).

Next ranked forks are: (1) `0x03BDA6` consumer/domain boundary and its
`0x03BDD8` stream; (2) another root/grammar edge exposed by this closure; and
(3) input causality only after a separately validated input-polling state.
