# M12 selector source and relative-table closure

Baseline ROM: `build/reference/Beyond Oasis (USA).bin`, 0x300000 bytes,
SHA-256 `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
No replay campaign, ROM, decoded asset, or production-runtime change was made.

## Upstream selector source — PROVEN

The selector handler is entered from the reset/main dispatcher, not by a
direct call. The reset vector at `0x000004` targets `0x0000020E`; its bounded
dispatcher latch is `0x0000042C..0x0000045C`:

```text
FF10AC != FFFF -> FF10AC := FFFF; FF10AE := latched state
D0 := FF10AE & 0x0000007C
A1 := longword[0x0000045E + D0]
0x0000045A: JSR (A1)
```

The exact five-entry prefix at `0x0000045E..0x00000472` is
`307A, 32F8, 89B2, 89B2, 03A748`; therefore dispatch value `0x10` enters
`0x03A748`. A complete direct incoming scan found no absolute `JSR/JMP`,
`BSR.W`, or short branch to that handler. The exact indirect source is
`0x0000045A`, and the analysis stops there rather than inventing a caller.

Inside the handler, `0x03A8C4` initializes selector RAM `0x00FFAFAE` to
`-1`, `0x03A916` increments it, and `0x03A91C` compares it with `7`.
The static CFG consequently proves a neutral finite selector loop over
`0..7`. Its category is not assigned: object slot, animation group, frame
group, graphics bank, and other semantic labels remain unproven. The handler
completion writes state `0x0004` at `0x03A956` and `0x03A97C`.

The direct `FF10AC` state writers are `0x000438=-1`, `0x000624=4`,
`0x003198=0x10`, `0x0031A2=4`, `0x003454=8`, `0x0034AE=0x0C`,
`0x03A956=4`, and `0x03A97C=4`. Thus the higher-level state controls entry
into the handler through `FF10AC & 0x7C`; the inner handler controls the
selector sequence. RAM gates and the two body indirect calls at
`0x03AA28` and `0x03AAA8` remain recorded without semantic overclaiming.

## Relative-word consumer — GRAMMAR PROVEN, EXTENT BLOCKED

The developer-only `src/tools/m12_relative_table_analysis.py` validates the
18-word pointer storage at `0x03BDA6..0x03BDCA` and its exact consumer:

```text
child +8 -> base
child record word +0 -> A6+2 (MOVE.W, doubled)
base + 2*index -> signed ADDA.W displacement
selected target + A6+10 -> two words
word +0 -> A6+8; word +2 -> A6+6
A6+10 advances by 4 bytes
```

The second word is a countdown. `ANDI.W #$7FFF` removes its control bit;
`BTST.B #7,6(A6)` observes bit 15 of the word in big-endian memory. When the
count reaches one, a set bit resets the cursor and a clear bit advances it.
This proves a 4-byte pair/command grammar, not a flat sprite table.

Eleven pointer entries are statically consumed by the closed child records.
Their targets include `0x03BDCA`, `0x03BDD8`, `0x03BE1E`, `0x03BE70`,
`0x03BE7E`, `0x03BE80`, `0x03BE8E`, and the `0x03BF5A..0x03BF60` family.
The closed stream prefixes reach `0x03BF76`. The `selector 0/index 1`
target at `0x03BDD8` has no bit-15 terminator before the independently
identified code boundary `0x03BF86`; consequently the complete logical
payload extent is not claimed.

## Structural join

```text
FF10AC state -> dispatcher -> 0x03A748 -> selector 0..7
  -> descriptor +12 -> child +8 relative pair -> A6+8
  -> child +0 offset table -> 0x03B448 -> 0x0000B730 -> SAT
```

`0x0000B730` consumes its own child-`+0` stream grammar: a leading word count,
then six-byte records, producing four-word SAT records at `0xB752`,
`0xB764`, `0xB76E`, and `0xB77A`. The relative pair grammar is structurally
joined to the proven B730/SAT path, while object/animation/frame names and
full logical enumeration remain unresolved.

## Machine-readable validation

- `src/tools/m12_selector_control_analysis.py`
- `src/tools/m12_relative_table_analysis.py`
- `tests/m12_selector_control_analysis_test.py`
- `tests/m12_relative_table_analysis_test.py`
- local ignored outputs: `build/m12-selector-control-current.json` and
  `build/m12-relative-table-current.json`
- `SOURCE_OWNED`: `1,475,368 / 3,145,728` bytes before and after

This is the bounded stopping point for the static pass. No controlled-state
experiment is needed to establish the proven upstream edge; any future work
on the unresolved `0x03BDD8` boundary must use a materially different
evidence class.

## M12 indirect body calls — FINITE TARGET CLOSURE

The two previously unresolved body calls are now closed by exact ROM slices;
no replay experiment was used. Both have the same proven form:

```text
MOVE.W (0x00FFAFAE),D0
ANDI.W #$0007,D0
ADD.W D0,D0; ADD.W D0,D0
LEA table(PC),A0
MOVEA.L (A0,D0.W),A0
JSR (A0)
```

The primary form is `0x03AA12..0x03AA2A`, calling indirectly at `0x03AA28`
from table `0x03B8A6..0x03B8C6`. The secondary form is
`0x03AA92..0x03AAAA`, calling indirectly at `0x03AAA8` from table
`0x03B8C2..0x03B8E2`. Therefore `D0` is the selector/index source, `A0` is
the loaded longword target, and both domains are statically closed to `0..7`.

The exact selector-to-target mapping is:

```text
primary:   0->03AAAE  1->03AB98  2->03AC16  3->03AC6E
           4->03ACA8  5->03AD0C  6->03ADB4  7->03AAEE
secondary: 0->03AAEE  1->03ABDA  2->03AC68  3->03AC92
           4->03ACE4  5->03AD66  6->03AE74  7->03BA46
```

There are 15 unique targets. Fourteen are routine entries with RTS-closed
exclusive boundaries:

```text
03AAAE..03AAEE  03AAEE..03AB98  03AB98..03ABDA  03ABDA..03AC16
03AC16..03AC68  03AC68..03AC6E  03AC6E..03AC92  03AC92..03ACA8
03ACA8..03ACE4  03ACE4..03AD0C  03AD0C..03AD66  03AD66..03ADB4
03ADB4..03AE74  03AE74..03B092
```

The remaining target `0x03BA46` is data, beginning `00000006 004C008A`,
and is the secondary index-7 value stored at the descriptor field `+0`
overlap. It is not decoded as code.

The bounded target census records exact direct downstream calls and static RAM
input/output sites in `src/tools/m12_indirect_body_dispatch.py` and its
payload-free JSON output. Important resource edges include
`0x03ACA8 -> 0x3820` from ROM source `0x17A750`,
`0x03ADB4 -> 0x3820` from ROM source `0x17E3BA`, and the shared calls into
`0x03C956`; other targets call `0x002EE2`, `0x002FDA`, `0x008E32`,
`0x03B5E8`, `0x03B832`, or `0x03B092` as listed by exact call-byte contracts.
The static result remains neutral: no target is named object, animation,
frame, or sprite.

One separate edge remains intentionally open: `0x03B092`, reached by the
closed routine `0x03AD66`, loads a table from `0x03B0AA` and executes
`0x03B0A8: JMP (A0)`. This is a downstream indirect tail, not either body
call, and no target is guessed. The combined structural join remains
`FF10AC -> dispatcher -> 0x03A748 -> selector -> descriptor/child relative
pairs -> 0x03B448 -> 0x0000B730 -> SAT`; SOURCE_OWNED is unchanged.
