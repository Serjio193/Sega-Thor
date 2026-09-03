# Narrow VDP Model

This document defines the video-state model used by the native Beyond Oasis reimplementation. It is intentionally not a full Mega Drive VDP emulator.

## Purpose

The native runtime needs enough VDP semantics to represent and reconstruct the original game's visible state while keeping gameplay logic portable and independent from emulator timing.

## Implemented state

### VRAM
- Size: 64 KiB.
- Stored as raw bytes.
- Bounded byte-span writes.
- Bounded big-endian 16-bit reads and writes.

### CRAM
- Size: 128 bytes / 64 words.
- Stored as raw bytes so original values can be preserved exactly.
- Bounded byte-span writes.
- Bounded big-endian 16-bit reads and writes.
- Color conversion to portable RGB is implemented separately in `game/genesis_graphics`.

### VSRAM
- Size: 80 bytes / 40 words.
- Stored as raw bytes.
- Bounded byte-span writes.
- Bounded big-endian 16-bit reads and writes.
- Exact Beyond Oasis scroll usage still requires original-code evidence.

## Tile attribute word

The standard 16-bit Genesis pattern-name layout is represented by `TileAttributes`:

```text
15       priority
14..13   palette index
12       vertical flip
11       horizontal flip
10..0    tile index
```

The decoder returns:
- `tile_index` in range 0..2047;
- `palette` in range 0..3;
- `priority`;
- `flip_h`;
- `flip_v`.

`PlaneCell` is intentionally only a thin decoded view of that raw word.

## Sprite attribute entry

`SpriteAttributes` represents the standard four-word sprite table entry at raw VDP level:

- 9-bit raw Y coordinate;
- width and height in 1..4 tile cells;
- 7-bit sprite-link field;
- pattern-name tile/palette/priority/flip attributes;
- 9-bit raw X coordinate.

Visible-screen coordinate conversion and sprite-chain traversal are renderer/state-reconstruction concerns, not part of this structure.

## Current design rule

Keep raw hardware data available. Do not immediately replace original words with higher-level game objects because exact bit patterns are useful for differential verification.

## Explicitly not implemented yet

The following are intentionally outside the current code until Beyond Oasis evidence requires them:
- VDP command-port protocol/state machine;
- DMA timing;
- FIFO behavior;
- raster timing;
- horizontal/vertical interrupts;
- VDP register side effects;
- complete plane-size/address register decoding;
- window plane behavior;
- sprite-link traversal and per-scanline limits;
- shadow/highlight mode;
- interlace modes;
- pixel-perfect hardware contention;
- SDL/OpenGL/Vulkan/Metal/console rendering APIs.

## Next evidence targets

1. identify original code that configures the plane/name-table bases used by Beyond Oasis;
2. identify where sprite attribute entries are built or uploaded;
3. determine which scrolling modes the game actually uses;
4. add only evidence-backed semantics to the compatibility layer.

## Portability boundary

`src/genesis/` contains portable hardware-state semantics only. Platform renderers consume decoded state from a higher layer and must not become dependencies of `oasis_core`.
