# M12-GFX-STATIC — `0xA372` shadow-SAT producer grammar

Baseline: `origin/main = a7d058bd883d337f1a0f67957913a1677d32e539`

Canonical ROM: size `0x300000`, SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

`SOURCE_OWNED` is unchanged at `1,475,368 / 3,145,728 = 46.9006856283%`.
This report and its analyzer emit no ROM payload, savestate, capture, decoded
asset, or sprite pixels.

## Result

The bounded producer grammar is statically closed, with semantic names kept
neutral. The exact producer routine is `0xA342..0xA438` (end exclusive),
entered by calls at `0xA19C` and `0xA6A0`, and returned by `RTS` at `0xA436`.
No direct branch targets the routine entry.
The two loop back-edges are `0xA37C -> 0xA36C` for six records and
`0xA3A6 -> 0xA396` for three optional records.

The exact PC-relative data targets are `0xA438` and `0xA480`. Earlier
shorthand labels `0xA43A` and `0xA482` are each two bytes into the first
record, because the 68000 PC-relative `LEA` uses the displacement-word PC.
This correction explains the existing runtime value: the first longword at
`0xA438` is `0x00880901`, and `0xA370` retains its upper 24 bits while
replacing the low byte with the incremented `D5.B` value `0x01`.

## Exact record grammar

Both roots are finite nine-record tables with stride `0x08`:

| root | logical range | records | consumer termination |
| --- | --- | ---: | --- |
| default | `0xA438..0xA480` | 9 | six-record loop plus optional three-record loop |
| alternate | `0xA480..0xA4C8` | 9 | six-record loop plus optional three-record loop |

Each record is consumed as three fields, not as a sentinel-terminated stream:

- `+0`, 4 bytes: source longword for `D2` upper 24 bits;
- `+4`, 2 bytes: copied to the destination;
- `+6`, 2 bytes: copied after `ADD.W D3,D2`, where `D3` is loaded from
  `0x00FF185A` for the first loop and set to `0x0158 - 0x00FF185A` for the
  optional loop.

The first loop sets `D0=5`; each iteration reads `(A0)+`, increments `D5.B`,
replaces `D2.B`, stores the longword at `0xA372`, then stores the `+4` and
adjusted `+6` fields. Thus the exact formula is:

`D2 = (record[+0] & 0xFFFFFF00) | ((D5.B + 1) & 0xFF)`.

The existing bounded runtime observation `0x00880901` is therefore explained
by record `0` at `0xA438` plus low counter byte `0x01`; no new replay was used.

## Destination grammar

At entry, `0xA342` loads `A5=0x00FF13CC`, then `0xA348` adds the signed word
stored at `0x00FF188C`. Every record advances `A5` by eight bytes. The first
loop writes `[A5,A5+0x30)`; the optional loop writes `[A5+0x30,A5+0x48)`.
The routine writes the resulting offset back to `0x00FF188C` at `0xA42A` and
the updated counter to `0x00FF188A` at `0xA430`.

The same routine has an additional `0x00FF1856`-controlled fixed record path
at `0xA3AA..0xA3D8`, plus a bounded `0x00FF1854/0x00FF1855` change path. Those
paths are recorded as producer-family side effects; they are not assigned a
frame or piece meaning. When the offset is zero, the first producer store
targets shadow SAT RAM `0x00FF13CC`.

## `0xFF1858` selection

The producer-family readers are:

- `0xA358: TST.B (0x00FF1858).L`; zero keeps `A0=0xA438`;
- `0xA4F2: TST.B (0x00FF1858).L`; zero keeps `A6=0xA438` for the sibling setup.

Nonzero selects `0xA480` at `0xA360` and `0xA4FA`. The local role is therefore
a boolean root selector. No mask or narrower range is applied by either
producer-family reader. Direct absolute ROM xrefs also contain `ST.B` writes
of `0xFF` and one `SF.B` write of `0x00`; their higher-level callers are
outside this bounded family and are not interpreted here.

The sibling routine is separate: `0xA4C8..0xA69C` has one direct caller at
`0x8B1E`, advances `0xFF188C` by `0x30` (and by `0x18` when `FF184F` is
nonzero), advances `0xFF188A` by six (and by three), selects the same roots,
and copies the selected root longword to `0xFF13CC` at `0xA4FE`. It is not an
incoming edge to `0xA342`.

## SAT join and semantic boundary

The proven structural join is:

`A19C/A6A0 -> A342..A436 -> selected 9x8 record root -> D2/A5 -> A372 -> FF13CC`

followed by the previously proven DMA contract:

`0x000027EC: shadow RAM 0x00FF13CC -> SAT VRAM 0x0000D000`.

The existing frame-2121 runtime evidence proves the shadow write and does not
prove same-frame VRAM publication. This producer family does not read the
previously proven selector RAM `0x00FFAFAE`, and no direct edge from that
selector/descriptor grammar to `0xA342` is claimed. Consequently an object,
animation, frame, piece, or sprite semantic sequence is **not proven**.

The machine-readable contract is
`src/tools/m12_a372_shadow_sat_producer.py`; regression coverage is
`tests/m12_a372_shadow_sat_producer_test.py`. Both validate exact ROM identity,
instruction contracts, routine boundaries, root geometry, D2 construction,
destination extents, and the bounded `FF1858` role without emitting payload.
