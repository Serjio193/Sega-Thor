# Standalone Handoff — M15.403: Flying/Projectile Dual-Axis (±X, ±Y) Tilemap Raycaster & Point Collision Dispatcher (`0x009708..0x0098EC`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x009708..0x0097E6`: `flying_horizontal_raycast` (branched to from `0x009068` when bit 1 of `+0x38(a6)` is set; point raycasting without entity radius offset `d3`; steps across 8-pixel tile boundaries; uses point solidity probe `0x0098EC`; updates wall collision bit 6 on `+0x40(a6)` upon contact; stores final X pos `+0x08` and clears X vel `+0x72`)
  - `0x0097E6..0x0098C2`: `flying_vertical_raycast` (vertical point raycasting for flying/projectile entities; uses point solidity probe `0x0098EC`; updates wall collision bit 6 on `+0x41(a6)`; stores final Y pos `+0x0C` and clears Y vel `+0x76`)
  - `0x0098C2..0x0098EC`: `flying_surface_sample_and_exit` (if step counter `a3 != 0`, samples ground surface height via `0x009BF2` and updates `+0x14(a6)`; branches unconditionally `bra.w $9382` to entity return vector dispatcher)
- **Evidence JSON:** `data/thor-evidence/rom_flying_raycaster_m15_403.json`
- **Python Extractor & Tests:** `tools/extract_rom_flying_raycaster.py`, `tests/rom_flying_raycaster_test.py`
