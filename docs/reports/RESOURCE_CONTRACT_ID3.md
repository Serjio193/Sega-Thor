# M11.24 — Bounded Resource Contract `0x02CFAA -> 0xD3B2`

Decision: `RESOURCE_CONTRACT_ID3_VERIFIED`

This is one bounded resource-loading contract. It does not assign a room,
screen, asset or routine name to the resource. The semantic role remains
`RESOURCE_ROLE_PARTIAL`: the immediate transfer setup is proven, while the
meaning of the transferred bytes is intentionally left neutral.

## Contract

| Field | Evidence-backed value |
|---|---|
| Caller | `0x02CFAA` (`JSR 0xD3B2`) |
| Resource ID | `3` (`MOVE.W #$3,D0` at `0x02CFA2`) |
| Table | `0x05CE96` |
| Table entry | `0x05CEA2`, bytes `00 1A E1 A8` |
| ROM input | `0x1AE1A8` |
| Compressed consumed | `1794` bytes (`0x702`) |
| Compressed end | `0x1AE8AA` (exclusive) |
| Output destination | `0xFF2FA8` |
| Output size | `4096` bytes (`0x1000`) |
| Output end | `0xFF3FA8` (exclusive) |
| Output SHA-256 | `36bbea13cb564ab194dd438b1fe75076525525068b57f2f02a4c0142630dc277` |

The next table entry, ID `4`, is also `0x1AE8AA`; it independently confirms
the consumed stream boundary. The output hash comes from the existing native
`0x3820` implementation and the independent mechanical translation of the
original `0x3820` algorithm. Both report consumed `1794`, output `4096`, and
byte-identical output for this stream.

## Bounded static path

The caller listing is:

```text
0x02CFA2  move.w #$3,D0
0x02CFA6  move.w #$4000,D1
0x02CFAA  jsr.l  $0000D3B2
0x02CFB0  move.w #$4,D0
0x02CFB4  move.w #$5000,D1
0x02CFB8  jsr.l  $0000D3B2
```

The exact `0xD3B2` decode is:

```text
0xD3B2  movem.l D0/A0/A1,-(A7)
0xD3B6  lea.l   $00FF2FA8,A1
0xD3BC  lea.l   $0005CE96,A0
0xD3C2  lsl.w   #2,D0
0xD3C4  movea.l 0(A0,D0.W),A0
0xD3C8  jsr.l   $00003820
0xD3CE  ori.w   #$0700,SR
0xD3D2  movea.l $00FF1892,A0
0xD3D8  move.l  #$007F97D4,(A0)+
0xD3DE  move.w  D1,(A0)+
0xD3E0  move.w  #$0800,(A0)+
0xD3E4  move.l  A0,$00FF1892
0xD3EE  bset.b   #1,$00FF164E
0xD3F6  btst.b   #1,$00FF164E
0xD3FE  bne.s    $D3F6
0xD400  movem.l (A7)+,D0/A0/A1
0xD404  rts
```

For ID `3`, `D0 << 2` selects `0x05CEA2`, whose big-endian pointer is
`0x001AE1A8`. The `JSR 0x3820` consumes exactly through the next pointer.
The first post-decompression consumer is the bounded DMA descriptor setup at
`0xD3D2..0xD3E4`: source word address `0x7F97D4` equals
`0xFF2FA8 >> 1`, destination is the caller-supplied `D1 = 0x4000`, and the
length is `0x800` words (`0x1000` bytes). This records transfer structure only;
no semantic label is assigned.

## Evidence and provenance

- Canonical ROM SHA-256:
  `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`
- M11.19 repaired runtime evidence:
  `build/gpgx_runtime_execution_evidence.json`, SHA-256
  `e6d8784fa8bbb6658d68e60c41a25788e6474e5c4a95c0e695b5288d34875efc`
- Exact decoder artifact:
  `build/m11-20-global.json`, SHA-256
  `06c1ededade003a5371fb9480fb55fcb63dd659f0a1db8cd7b9a598ec5d67e23`
- Bounded `0xD3B2` decode:
  `build/re-report/d3b2.json`, SHA-256
  `f8554eb60171a4da25907ae753a4bbdf1968cd0a9e9fd50c35abb26d98405cf8`
- Existing native source: `src/game/graphics_decompress.cpp` and existing
  independent original-derived model `src/tools/re_static_translation.cpp`.

The retained natural GPGX evidence contains executed facts for `0x02CFAA`,
`0xD3B2` and `0x3820`. No new runtime capture or forced PC/register/RAM/ROM
modification was used in M11.24.

## Acceptance

| Criterion | Result |
|---|---|
| Caller reaches target path | PASS: natural executed-PC evidence plus direct static call |
| ID `3` selection | PASS |
| Table entry and ROM stream | PASS |
| Exact compressed consumption | PASS: native and independent model agree; next pointer confirms end |
| Output destination and size | PASS |
| Independent original-derived output | PASS: mechanical `0x3820` model |
| Native byte comparison | PASS: byte-identical, same hash |
| Immediate consumer/transfer | PASS: bounded DMA descriptor setup |
| Broad tooling or trust expansion | NONE |

Remaining unknown: the semantic identity of resource ID `3` and the meaning
of the transferred bytes are not promoted by this contract.

Exact next recommendation: **E. return to native gameplay**.
