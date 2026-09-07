# M11.25 — First Authentic Native ROM Resource Baseline

## Decision

`NATIVE_ROM_RESOURCE_ID3_HIGH_VALUE`

The verified ID 3 stream is now consumed by the production native executable,
transferred into the bounded native VRAM model, decoded with the existing
Genesis 4bpp decoder, and shown in a deterministic diagnostic atlas. The
visualization is structural only; it does not assign a room, player, sprite or
background meaning to the resource.

## Production path

`canonical ROM -> resource loader -> existing native decompressor -> VDP VRAM
0x4000..0x4FFF -> existing 4bpp tile decoder -> native framebuffer/window`

The original `0xFF2FA8` destination remains provenance in the M11.24 contract
and is not exposed as native architecture. No emulator or GPGX dependency was
added. The existing ControlledScreen path remains the default executable mode.

## Contract and hashes

| Field | Verified value |
|---|---|
| Resource ID | `3` |
| Table / entry | `0x05CE96` / `0x05CEA2` |
| ROM input | `0x1AE1A8` |
| Compressed consumed | `0x702` bytes (`1794`) |
| Decompressed output | `0x1000` bytes (`4096`) |
| Resource output SHA-256 | `36bbea13cb564ab194dd438b1fe75076525525068b57f2f02a4c0142630dc277` |
| Native VRAM range | `0x4000..0x4FFF` |
| VRAM range SHA-256 | `36bbea13cb564ab194dd438b1fe75076525525068b57f2f02a4c0142630dc277` |
| Tile count | `128` tiles × `32` bytes |
| Framebuffer SHA-256 | `d2b7655501ff3babf6ef9dd44af720b3afab3ba3e2673fa7aeff33248a3ab1b1` |

The canonical ROM is supplied outside the repository. Its SHA-256 is
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

## Visualization result

All 128 tile records were accepted by the existing 4bpp tile decoder. The
native diagnostic mode renders a deterministic 16-column atlas from the
transferred VRAM bytes. The palette source is `DIAGNOSTIC_PALETTE`; no verified
palette associated with this exact resource was available, so this output is
not claimed to be original visual fidelity.

The executable mode is:

```text
oasis <canonical-rom> --resource-id 3
```

Manual launch evidence: the native process stayed responsive and exposed the
window title `ROM-backed resource ID 3 diagnostic visualization`. The existing
normal mode remains available as `oasis <canonical-rom>`.

## Negative and deterministic validation

The ROM-backed reference executable verifies and rejects:

- modified or non-canonical ROM input;
- unsupported resource IDs;
- truncated ROM input;
- unexpected decompressed size or output hash;
- VRAM transfers outside the bounded VRAM range;
- substituted resource bytes.

Two renders in one run are byte-identical. Debug and Release reference runs
produced the same compressed size, resource hash, VRAM hash, tile count and
framebuffer hash.

## Tests and hygiene

- Debug build: pass.
- Release production/reference targets: pass.
- CTest: pass (`45/45`).
- Canonical-ROM deterministic reference: pass in Debug and Release.
- Source-file size check: pass.
- No ROM, extracted assets, generated captures or binaries are tracked.

## Exact next recommendation

`A. identify resource ID 3 palette/visual role`

Do not implement this recommendation in M11.25.
