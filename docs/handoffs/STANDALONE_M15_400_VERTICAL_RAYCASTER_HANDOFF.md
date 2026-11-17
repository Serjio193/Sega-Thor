# Standalone Handoff — M15.400: Vertical (±Y) Tilemap Raycaster, Collision Sweep & Entity Return Dispatcher (`0x0091F6..0x00938E`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x0091F6..0x0092B0`: `vertical_raycast_positive_y` (clears X step `+0x72(a6)`; resets tile step counter `a3`; reads 32-bit Y pos `+0x0C` and Y vel `+0x76`; sets dir code `0x12` into `+0x41(a6)`; integrates subpixel Y into `+0x0E`; clamps step against world boundaries `0x009D92`; sweeps against solid dynamic entities `0x009FA0`; steps across 8-pixel tile rows checking wall collisions `0x0095AA`; probes surface height `0x009BF2`; translates surface type using 16-byte LUT `0x0096E8` into `+0x70(a6)`; updates surface height `+0x14(a6)`)
  - `0x0092B0..0x00934E`: `vertical_raycast_negative_y` (sets dir code `0x10` into `+0x41(a6)`; integrates subpixel Y into `+0x0E`; clamps step against world boundaries `0x009DB4`; sweeps against solid dynamic entities `0x00A09A`; steps across 8-pixel tile rows checking wall collisions `0x0094D2`; probes surface height `0x009BF2`; translates surface type using LUT `0x0096E8` into `+0x70(a6)`; updates surface height `+0x14(a6)`)
  - `0x00934E..0x009382`: `vertical_raycast_cleanup` (writes final Y position to `+0x0C(a6)`; if no tile boundary stepped, probes resting tile height and terrain type; clears Y step/velocity `+0x76(a6)`)
  - `0x009382..0x00938E`: `entity_return_dispatcher` (restores d7 via `movem.l (a7)+, d7`; loads recovery return vector from `$FF193C.l` into `a0`; indirect jumps to `jmp (a0)` returning to the 3-group entity master stepper loop `0x8EAA`, `0x8ECC`, or `0x8EEE`)
- **Evidence JSON:** `data/thor-evidence/rom_vertical_raycaster_m15_400.json`
- **Python Extractor & Tests:** `tools/extract_rom_vertical_raycaster.py`, `tests/rom_vertical_raycaster_test.py`
