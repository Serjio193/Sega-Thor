# Standalone Handoff: M15.295 — CRAM Palette Fade Engine (`0x002D58`..`0x00302D`)

## Summary
Beyond Oasis fades its 64-color Mega Drive CRAM shadow (`$FF134C`, backed up in `$FF18A6`) over 8 steps (`0..7` for Fade In at `0x002D84`/`0x002DB6`; `7..0` for Fade Out at `0x002E1E`/`0x002E78`) with per-step frame delay `$FF1898 + 1` (`$FF1896`).

## Channel Scaling Formula (`0x002FDA`)
For each 9-bit BGR333 entry `c` and `step` in `0..7`:
```cpp
r = (((c & 0x00E) * step) / 7) & 0x00E;
g = (((c & 0x0E0) * step) / 7) & 0x0E0;
b = (((c & 0xE00) * step) / 7) & 0xE00;
out = r | g | b;
```
Sets `$FF164D` bit 0 to schedule CRAM DMA; Fade In unblanks (`$FF164D` bit 1) on step `0`.

## C++20 Standalone Target
- Implement `PaletteFadeEngine` / `scale_bgr333_color()` in `src/game/video/` matching the exact `/ 7` & `& 0x0E` bitmask math.
- Evidence artifact: `data/thor-evidence/rom_palette_fade_m15_295.json`.
