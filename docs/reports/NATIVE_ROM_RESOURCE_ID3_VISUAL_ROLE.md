# M11.26 — Resource ID 3 Visual-Role Investigation

## Decision

`RESOURCE_ID3_VISUAL_ROLE_PARTIAL`

The bounded evidence proves that resource ID 3 is a valid 128-tile graphics/data
bank loaded during a scene-initialization path and transferred to native VRAM
at `0x4000`. It does not prove whether the displayed consumer is a plane,
sprite table, another VDP structure, or which palette is used. No semantic
room, background, player, sprite or UI name is assigned.

## Static path

The canonical ROM path is:

```text
0x02CF9C  JSR 0xD406
0x02CFA2  MOVE.W #$3,D0
0x02CFA6  MOVE.W #$4000,D1
0x02CFAA  JSR 0xD3B2
0x02CFB0  MOVE.W #$4,D0
0x02CFB4  MOVE.W #$5000,D1
0x02CFB8  JSR 0xD3B2
```

`0xD3B2` selects the indexed compressed stream, calls the verified native
equivalent of `0x3820`, and queues the decompressed output for the caller's VDP
destination. For ID 3 this preserves the M11.24 contract: input `0x1AE1A8`,
`0x702` consumed bytes, `0x1000` output bytes and native destination
`0x4000..0x4FFF`.

The adjacent bounded initialization path at `0xD406..0xD7AE`:

- initializes RAM-backed scene/map structures at `0xFF1716`, `0xFF173E`,
  `0xFF1766` and `0xFF1770`;
- performs the separate four-entry resource loop at `0xD4C8..0xD4FC`, which
  uses IDs from `0xFF16FA` and writes decompressed slots beginning at
  `0xFF3FA8`;
- emits VDP control/data writes at `0xD6A8..0xD6F0` and updates VDP-related
  state before returning to the caller.

Those operations establish the bounded setup context, but no instruction in
this window proves a tile-name word or sprite attribute that selects ID 3's
patterns.

## VRAM tile-index relation

Genesis 4bpp tiles occupy 32 bytes. Therefore the ID 3 range maps structurally
to:

```text
VRAM bytes:  0x4000..0x4FFF
tile index:  0x0200..0x027F  (128 tiles)
ID 4 range:  0x5000..0x5FFF -> 0x0280..0x02FF
```

This is an address-to-pattern-index calculation only. It is not evidence that
any plane or sprite structure actually selects those indices.

## Palette

`PALETTE_UNKNOWN`.

The bounded ID 3 caller/loader path contains no proven CRAM source, palette
table, CRAM destination or palette hash associated with the transferred bytes.
The VDP commands in the bounded setup are not sufficient to establish an ID 3
palette association. The native diagnostic mode therefore keeps its explicit
`DIAGNOSTIC_PALETTE`; no production palette was changed.

## Consumer

No proven visual consumer was found in the bounded path.

The static evidence records the resource transfer and the adjacent VDP setup,
but does not identify a plane name table, sprite attribute entry or other
consumer containing pattern indices `0x0200..0x027F`. No palette select,
flip or priority bits can consequently be attributed to ID 3.

## Runtime observation

No new runtime capture or hook was used. Existing static evidence is sufficient
to prove the transfer and to establish that the palette/consumer relation
remains unanswered. A new runtime observation would be a separate targeted
follow-up, not part of M11.26.

## Native visualization

No production visualization change was made. The existing diagnostic atlas
continues to use the verified resource bytes, the existing 4bpp decoder and
`DIAGNOSTIC_PALETTE`. Its resource SHA-256 remains
`36bbea13cb564ab194dd438b1fe75076525525068b57f2f02a4c0142630dc277`; its
deterministic framebuffer SHA-256 remains
`d2b7655501ff3babf6ef9dd44af720b3afab3ba3e2673fa7aeff33248a3ab1b1`.

## Tests and provenance

- Baseline commit: `c1592ad6adcf9245cdd099c9ba5470e8421c66d9`.
- Canonical ROM SHA-256:
  `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
- Existing resource reference, negative checks and 128-tile decode remain
  green from M11.25.
- Bounded exact-decoder/assembler checks for the caller, `0xD3B2`,
  `0xD406..0xD7AE` and `0xD7C0` completed without adding tooling.
- No ROM, extracted assets, captures or generated binaries were added to the
  repository.

## Exact next recommendation

`A. connect verified ID3 visual data to one authentic layout`

This recommendation requires a new bounded proof of a name-table or sprite
consumer. Do not implement it in M11.26.
