# Standalone Handoff: M15.311 — Wrap-Around VDP Plane Blitter (`0x00D800`..`0x00D94E`)

## Summary
The room plane blitter (`0x00D800`) copies tilemap windows into VDP Plane A/B (`64x32` nametable, `0x80` bytes/row, `0x1000` bytes/plane) with dual modulo wrap-around on both source `(tiles_w, tiles_h)` and VDP `(64, 32)`:
- **Full-Plane `64x32` (`0x3F001F`):** When `$27(a3)` bit 1 is set OR `(tiles_h == 32 && tiles_w <= 64)`.
- **Viewport `41x29` (`0x28001C`):** For larger scrolling rooms.

## C++20 Standalone Target
- Implement `select_blit_dimensions()` and `blit_wrapped_plane_window()` in `src/game/world/`.
- Evidence artifact: `data/thor-evidence/rom_vdp_plane_blitter_m15_311.json`.
