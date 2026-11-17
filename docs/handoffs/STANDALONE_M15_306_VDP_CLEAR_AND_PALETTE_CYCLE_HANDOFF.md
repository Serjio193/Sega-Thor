# Standalone Handoff: M15.306 — VDP Full Clear, Armlet Glow & Palette Cycler (`0x002B28`, `0x008DD4`..`0x008E8E`)

## Summary
- **VDP Full Clear (`0x002B28`):** Clears CRAM backdrop `0`, HScroll `0xDC00`, VSRAM `40` words, Tile `0` (`0x0000`), SAT `0xD000`, Plane A `0xC000` (`4096`B), and Plane B `0xE000` (`4096`B) using VDP DMA Fill (`0x8F01`, `0x9780`).
- **Armlet Glow (`0x008DD4`):** Ramps `$FF183E` `0->3` when `$FF1AA4 != 0` (pulsing `3/4` when `$FF199E >= 20` on even frames); decays `3->0` (`>=4 -> 2`) when no spirit is active.
- **CRAM Palette Cycler (`0x008E22`):** Walks `$FF17A2` linked list of `(duration, bgr333)` keyframes, advancing cursor `+4` (wrapping to `+8` on `duration == 0`) and writing `$FF134C + cram_off`.

## C++20 Standalone Target
- Implement `step_armlet_glow()` and `PaletteCycler` in `src/game/video/`.
- Evidence artifact: `data/thor-evidence/rom_vdp_clear_and_palette_cycle_m15_306.json`.
