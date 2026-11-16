# Standalone Handoff: M15.289 — Attract/Demo Stream Table (`0x008A94`, 9 Demos at `0x1698FE`)

## Summary
When `Mode 0x08` (`0x0089B2`) is entered with `$FF1996 != 0` (from Title Screen timeout `720` frames or chord `0x8A`), Beyond Oasis cycles `$FF0BFC` through `0..8`, indexes the 9-entry `u16` offset table at `0x008A94` relative to `0x1698FE`, decompresses the demo input stream via `0x003820` to `$FFBFAC`, delta-decodes `0x1FFF` (`8191`) bytes at `$FFDFB2`, reads initial `(room_id, player_x, player_y)` from the stream header, stores the input pointer in `$FF1998`, and starts gameplay at `0x00CA24`.

## C++20 Standalone Target
- Implement `AttractDemoPlayer` in `src/game/` supporting `0..8` slot cycling, `0x008A94` table lookup, `0x003820` decompression, and `0x1FFF`-byte delta-decoding.
- Evidence artifact: `data/thor-evidence/rom_attract_demos_m15_289.json`.
