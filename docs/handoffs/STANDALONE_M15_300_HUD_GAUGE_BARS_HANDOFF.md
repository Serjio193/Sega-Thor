# Standalone Handoff: M15.300 — HUD HP/SP/Enemy Gauge Bar VDP Renderer (`0x00267C`..`0x0027A4`)

## Summary
V-Int renders the three vertical HUD bars (`48` = `0x30` rows tall) differentially using cached `(cur, max)` (`$FF1840`) and cached row heights (`$FF184C`):
- **Ali HP (`0x00267C`):** VRAM cmd `0x70E00003` (`0xF0E0`), 4bpp pattern `0x4E2222E4`.
- **Ali SP (`0x0026B2`):** VRAM cmd `0x72800003` (`0xF280`), 4bpp pattern `0x4E6666E4`.
- **Target Enemy HP (`0x0026D6`):** Gated by `$FF184F` timer & `!demo`, reads `$FF1850`, VRAM cmd `0x74200003` (`0xF420`), 4bpp pattern `0xDE4444ED`.
- **Fill Height (`0x00271C`):** `0 -> 0`, `cur >= max -> 48`, else `((cur * 47) / max) + 1`. Erasing rows ORs `0x00EEEE00` into the fill pattern.

## C++20 Standalone Target
- Implement `compute_gauge_fill_rows()` and differential `HudGaugeState` in `src/game/ui/`.
- Evidence artifact: `data/thor-evidence/rom_hud_gauge_bars_m15_300.json`.
