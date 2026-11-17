# Standalone Handoff — M15.382: ROM Ali Sword Spin, Elemental Blade & Bow Arrow Dispatcher (`0x007262..0x0074A4`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x007262..0x0072E8`: `ali_ground_sword_spin` (during frames 13..16, holding Button A steers Ali at 25% speed `d1 >>= 2, d2 >>= 2`; releasing Button A clears `+0x2E`, plays sword spin SFX `#$600379`, dispatches elemental blade payload to spawner `0x0000F258`: Fire Sword `0x8000001C`, Omega Sword `0x4000001C`, Hyper Sword `0x2000001C`, Normal `0x0000001C`; resets to idle `0x0062CC` on animation complete).
  - `0x0072EA..0x007362`: `ali_water_sword_spin` (advances stroke counter `$FF1980` by 1; at frame 12 and `+0x2E == 0`, plays sword spin SFX `#$600379`, dispatches water blade payload to spawner `0x0000F26A`: Fire `0x8000000D`, Omega `0x4000000D`, Hyper `0x2000000D`, Normal `0x0000000D`; resets to water idle `anim = 0x24, state = 0x10`).
  - `0x007364..0x0073EA`: `ali_ground_bow_fire` (during frames 5..8, holding Button A steers Ali at 25% speed; releasing Button A plays bow release SFX `#$7004D3`; release frame calculated from facing: facing 2 (down) fires at frame 12, other facings fire at frame 16; computes arrow payload `d0 = bow_type - 9, d1 = 0x22, d2 = (d0 << 4) + 1`, calls spawner `0x0000F396`, sets arrow shot flag `+0x5C = 0xFFFF`).
  - `0x0073EC..0x00743A`: `ali_water_bow_fire` (at frame 12 and `+0x2E == 0`, computes water arrow payload `d1 = 4, d2 = (bow_type - 9) << 3`, calls water spawner `0x0000F3A8`, sets shot flag `+0x5C = 0xFFFF`).
  - `0x00743C..0x0074A4`: `ali_water_dagger_flurry` (at frame 0 plays dagger strike SFX `#$800222`; calls ripple emitter; on land probe transitions to `anim = 0x60, state = 0x54`).
- **Evidence JSON:** `data/thor-evidence/rom_ali_sword_spin_bow_fire_m15_382.json`
- **Python Extractor & Tests:** `tools/extract_rom_ali_sword_spin_bow_fire.py`, `tests/rom_ali_sword_spin_bow_fire_test.py`
