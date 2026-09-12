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
