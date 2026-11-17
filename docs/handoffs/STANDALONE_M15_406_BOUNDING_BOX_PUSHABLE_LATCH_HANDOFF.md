# Standalone Handoff — M15.406: 2D Bounding Tile Box Sampler, Type 0x56 Pushable Block Scanner & Ground Latch (`0x009BC2..0x009D4E`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x009BC2..0x009BF2`: `sample_entity_bounding_tile_box_dispatch` (selects layer map `$FF1766` vs `$FF1770` based on bit 7 of `$FF16F0`; passes X, Y, and radius `+0x46(a6)` to `0x009BF2`)
  - `0x009BF2..0x009C40`: `sample_bounding_tile_box` (computes column span `(x - rad) >> 3` .. `(x + rad) >> 3` and row span `(y - rad) >> 3` .. `(y + rad) >> 3`; executes 2-nested loop accumulating tile attributes `or.b` into `d4` and `and.b` into `d5`)
  - `0x009C40..0x009C7C`: `read_point_tile_layer` (reads point tile attribute byte from active layer)
  - `0x009C7C..0x009D00`: `scan_pushable_block_solid_collision` (scans 6-slot pool at `$FF2D8C`, stride 90 bytes; checks `type == 0x56` [Pushable Rock / Block / Crate]; tests 2D AABB overlap with entity radius `+0x42(a6)` and `+0x44(a6)`; sets Carry if overlapping, clears Carry if passable)
  - `0x009D00..0x009D4E`: `latch_entity_ground_surface` (samples bounding box via `0x009BF2`; writes tile attributes to `+0x6F(a6)` and `+0x6E(a6)`; maps surface type through 16-byte LUT `0x0096E8` into `+0x70(a6)`; latches elevation via `0x009AD6` into `+0x14(a6)`)
- **Evidence JSON:** `data/thor-evidence/rom_bounding_box_pushable_latch_m15_406.json`
- **Python Extractor & Tests:** `tools/extract_rom_bounding_box_pushable_latch.py`, `tests/rom_bounding_box_pushable_latch_test.py`
