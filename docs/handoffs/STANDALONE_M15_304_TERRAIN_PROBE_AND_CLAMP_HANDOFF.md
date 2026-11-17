# Standalone Handoff: M15.304 — Terrain Probe LUT & Room Boundary Clamp (`0x009D00`..`0x009DD5`)

## Summary
- **Terrain Surface Probe (`0x009D00`):** Selects layer descriptor `$FF1766` (or `$FF1770` if `$FF16F0` bit 7 is set), queries tile & floor height (`$6F`, `$6E`, `$14`), and maps `(tile & 0x0F)` through the 16-byte LUT `0x0096E8` (`[0xFF, 0, 2, 1, 4, 0xFF, 3, 0xFF, 6, 7, 0xFF, 0xFF, 5, 0xFF, 0xFF, 0xFF]`) into `$70(a6)`.
- **4-Side Room Boundary Clamp (`0x009D4E..0x009DD4`):** Clamps `(x +/- radius)` and `(y +/- radius)` against `[0, tiles * 8 - 1]`, setting `$40` or `$41 |= 0x48` when pinned against a room edge.

## C++20 Standalone Target
- Implement `kTerrainSurfaceLut16` and `clamp_entity_to_room_bounds()` in `src/game/world/`.
- Evidence artifact: `data/thor-evidence/rom_terrain_probe_and_clamp_m15_304.json`.
