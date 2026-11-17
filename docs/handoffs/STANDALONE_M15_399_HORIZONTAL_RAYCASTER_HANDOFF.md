# Standalone Handoff — M15.399: Horizontal (±X) Tilemap Raycaster, Surface Height Probe & 0x0096E8 Terrain LUT (`0x009084..0x0091F6`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x009084..0x009128`: `horizontal_raycast_positive_x` (sets dir code `$11` into `+0x40(a6)`; integrates 32-bit subpixel X position `+0x08`/`+0x0A`; clamps step against world boundaries `0x009D4E`; sweeps against solid entities `0x009DD6`; iterates in 8-pixel tile increments checking wall collisions `0x00938E`; probes surface height `0x009BF2`; translates surface type using 16-byte LUT `0x0096E8` into `+0x70(a6)`; updates surface height `+0x14(a6)`)
  - `0x009128..0x0091C6`: `horizontal_raycast_negative_x` (sets dir code `$13` into `+0x40(a6)`; integrates subpixel X; clamps step against left world boundaries `0x009D70`; sweeps against solid entities `0x009EBA`; iterates in 8-pixel tile increments checking wall collisions `0x00938E`; probes surface height `0x009BF2`; translates surface type using LUT `0x0096E8` into `+0x70(a6)`; updates surface height `+0x14(a6)`)
  - `0x0091C6..0x0091F6`: `horizontal_raycast_cleanup` (writes final X position to `+0x08(a6)`; if no tile boundary stepped, probes resting tile height and terrain type; clears X step/velocity `+0x72(a6)`)
  - `0x0096E8..0x0096F8`: 16-byte terrain surface type LUT `[0xFF, 0x00, 0x02, 0x01, 0x04, 0xFF, 0x03, 0xFF, 0x06, 0x07, 0xFF, 0xFF, 0x05, 0xFF, 0xFF, 0xFF]`
- **Evidence JSON:** `data/thor-evidence/rom_horizontal_raycaster_m15_399.json`
- **Python Extractor & Tests:** `tools/extract_rom_horizontal_raycaster.py`, `tests/rom_horizontal_raycaster_test.py`
