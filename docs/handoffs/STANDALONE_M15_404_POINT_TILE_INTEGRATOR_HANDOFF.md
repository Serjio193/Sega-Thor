# Standalone Handoff — M15.404: Point Tile Solidity, Air/Elevation Integrator & Tile Attribute Sampler (`0x0098EC..0x0099EE`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x0098EC..0x0099B8`: `point_tile_solidity_probe` (loads surface height from `+0x14(a6)`; queries target tile elevation via `0x009AD0`; if tile negative checks ramp flag mask `0x90`; computes elevation delta `cur_h - target_h`; checks air height clearance `air_h + delta >= 0` [if negative, wall blocks and sets Carry]; on slopes integrates single-pixel increments of Y coordinate and air height `+0x10(a6)`, checking tile attributes `0x0099B8` at 8-pixel boundaries; updates final Y pos `+0x0C` and ground height `+0x14`; clears Carry and returns)
  - `0x0099B8..0x0099D6`: `sample_point_tile_attr` (computes tile buffer offset `(y >> 3) << stride_shift + (x >> 3)`, reads byte from layer buffer `+0x00(a0)`, masks with `0x0F` into `d3`)
  - `0x0099D6..0x0099EE`: `read_point_tile_raw` (reads unmasked raw tile attribute byte into `d3`)
- **Evidence JSON:** `data/thor-evidence/rom_point_tile_integrator_m15_404.json`
- **Python Extractor & Tests:** `tools/extract_rom_point_tile_integrator.py`, `tests/rom_point_tile_integrator_test.py`
