# Standalone Handoff — M15.405: Ramp/Bridge Slope Solver, Fractional Height Interpolator & Tile Elevation Engine (`0x0099F0..0x009BC0`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x0099F0..0x009A60`: `ramp_search_handler_1` (scans up to 48 tiles along Y rows searching for walkable bridge/ramp tiles with bit 4 clear; probes tile elevation via `0x009AD0`; adjusts Y coordinate `+0x0C` and air height `+0x10`)
  - `0x009A60..0x009AD0`: `ramp_search_handler_2` (secondary ramp climb search for level matching)
  - `0x009AD0..0x009BC0`: `tile_elevation_engine` (queries layer collision map `$FF1766` vs `$FF1770` based on bit 7 of `$FF16F0`; indexes tile buffer `(y >> 3) << stride_shift + (x >> 3)`; maps 4-bit attribute through LUT 2 `0x0096F8`; if even surface type, performs flat table lookup `$FF1706[d5 * 2]`; if odd surface type [slope], performs fractional height interpolation: `fraction = ((next_h - base_h) * sub_y) // total_y; return base_h + fraction` using M68K `mulu.w` and `divu.w`)
  - `0x0096F8..0x009708`: 16-byte LUT 2 `[0xFF, 0x00, 0x02, 0x01, 0x04, 0x04, 0x03, 0x03, 0x06, 0x06, 0x06, 0x06, 0x05, 0x05, 0x05, 0x05]`
- **Evidence JSON:** `data/thor-evidence/rom_slope_elevation_engine_m15_405.json`
- **Python Extractor & Tests:** `tools/extract_rom_slope_elevation_engine.py`, `tests/rom_slope_elevation_engine_test.py`
