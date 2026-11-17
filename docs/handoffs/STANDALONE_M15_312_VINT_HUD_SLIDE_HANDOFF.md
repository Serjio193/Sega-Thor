# Standalone Handoff: M15.312 — V-Int Sound Tick & HUD Slide (`0x002380`..`0x0023CA`)

## Summary
At the tail of every V-Int (`0x002380`):
1. Calls sound driver tick `0x060000`.
2. Steps HUD slide offset `$FF185A` by `2` pixels based on direction `$FF1859`:
   - `dir == 0`: stops at `0x20`, otherwise `+2` wrapping `>= 0x158 -> 0`.
   - `dir != 0`: stops at `0x138`, otherwise `-2` wrapping `< 0 -> 0x158`.

## C++20 Standalone Target
- Implement `step_hud_slide_offset()` in `src/game/ui/`.
- Evidence artifact: `data/thor-evidence/rom_vint_hud_slide_m15_312.json`.
