# Narrow VDP Model

This document defines the video-state model used by the native Beyond Oasis reimplementation. It is intentionally not a full Mega Drive VDP emulator.

## Purpose

The native runtime needs enough VDP semantics to represent and reconstruct the original game's visible state while keeping gameplay logic portable and independent from emulator timing.

## Implemented state

### VRAM
- Size: 64 KiB.
- Stored as raw bytes.
- Bounded writes are supported.
- Big-endian 16-bit reads are supported for tile/name-table words and other verified structures.

### CRAM
- Size: 128 bytes / 64 words.
- Stored as raw bytes so original values can be preserved exactly.
- Color conversion to portable RGB is implemented separately in `game/genesis_graphics`.

### VSRAM
- Size: 80 bytes / 40 words.
- Stored as raw bytes.
- This represents the hardware vertical-scroll RAM capacity; actual Beyond Oasis usage must still be established from original code/traces.

## Tile attribute word

The standard 16-bit Genesis name-table attribute layout is represented by `TileAttributes`:

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

A plane cell can therefore remain a raw 16-bit word in VRAM and be decoded at the boundary where rendering/state inspection needs structured fields.

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
- pixel-perfect hardware contention.

## M5 next evidence targets

1. identify original code that configures the plane/name-table bases used by Beyond Oasis;
2. identify where sprite attribute table entries are built or uploaded;
3. document the exact sprite raw-word layout before adding a structured sprite type;
4. determine which scrolling modes the game actually uses;
5. add only those semantics to the compatibility layer.

## Portability boundary

`src/genesis/` contains portable hardware-state semantics only. SDL, Vulkan, OpenGL, Metal, console SDKs, Android APIs and other platform renderers must consume this state from a higher layer and must not be dependencies of `oasis_core`.
