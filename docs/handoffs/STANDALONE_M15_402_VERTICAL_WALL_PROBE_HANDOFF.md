# Standalone Handoff — M15.402: Vertical Tilemap Wall Collision, Row Sampler & Height Step Subsystem (`0x0094D2..0x0096E8`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x0094D2..0x0095AA`: `tile_solid_probe_neg_y` (checks intangible bit 0 on `+0x38`; verifies non-void current surface `$70`; samples tile row span via `0x0096BA`; maps target surface type `0x0096E8`; evaluates elevation delta `d6 - d7`: if delta >= 2 sets wall collision bit 6 on `+0x41(a6)` [Y collision byte] and sets Carry; if delta == 1 checks climb disable bit 5 on `+0x38(a6)`; if delta <= -2 checks ledge drop capability bit 4 on `+0x38`, ground check `+0x10 == 0`, and even surface type; reads RAM elevation table `$FF1706[d7] - $FF1706[d6]`, records drop height into `$FF196E`, sets ledge drop bit 5 on `+0x41(a6)`, and sets Carry)
  - `0x0095AA..0x009678`: `tile_solid_probe_pos_y` (downward tile solid probe: checks intangible bit 0 on `+0x38`; scans row solidity `0x009678`; maps target surface type `0x0096E8`; evaluates elevation delta `d6 - d7`: delta >= 2 sets wall collision bit 6 on `+0x41(a6)` and sets Carry; delta <= -2 evaluates ledge drop: records drop height into `$FF196E`, sets bit 5 on `+0x41(a6)`, and sets Carry)
  - `0x009678..0x0096BA`: `scan_tile_row_solidity` (steps across entity X radius `d3` in 8-px increments up to `0x180` px using `0x0096BA`, saving max extent into `a4`)
  - `0x0096BA..0x0096E8`: `sample_tile_row_span` (computes column span `(x - rad) >> 3` to `(x + rad) >> 3`; reads layer descriptor at `a0`: base layer `+0x00(a0)`, column stride shift `+0x08(a0)`, row offset `+0x04(a0)`; accumulates `or.b` into `d4` and `and.b` into `d5`)
- **Evidence JSON:** `data/thor-evidence/rom_vertical_wall_probe_m15_402.json`
- **Python Extractor & Tests:** `tools/extract_rom_vertical_wall_probe.py`, `tests/rom_vertical_wall_probe_test.py`
