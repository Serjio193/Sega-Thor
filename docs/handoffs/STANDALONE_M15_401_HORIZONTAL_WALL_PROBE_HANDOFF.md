# Standalone Handoff — M15.401: Horizontal Tilemap Wall Collision, Surface Height Step & Ledge Drop Detector (`0x00938E..0x0094D2`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x00938E..0x009460`: `tile_solid_probe_x` (checks intangible bit 0 of `+0x38(a6)`; scans tile column solidity `0x009460`; maps target surface type via 16-byte LUT `0x0096E8`; evaluates height delta `d6 - d7`: if delta >= 2 sets wall collision bit 6 on `+0x40(a6)` and sets Carry; if delta == 1 checks climb disable bit 5 on `+0x38(a6)`; if delta <= -2 evaluates ledge drop: requires capability bit 4 on `+0x38(a6)`, ground air height `+0x10(a6) == 0`, and even surface type; reads RAM elevation table `$FF1706[d7] - $FF1706[d6]`, records drop height into `$FF196E`, clears Y vel `+0x76`, sets ledge drop bit 5 on `+0x40(a6)`, and sets Carry)
  - `0x009460..0x0094A2`: `scan_tile_column_solidity` (scans across entity Y radius `d3` using sub-helper `0x0094A2`, loops up to 0x180 px, sets `a4 = d7`)
  - `0x0094A2..0x0094D2`: `sample_tile_column_span` (computes row span `(y - rad) >> 3` to `(y + rad) >> 3`, reads layer descriptor at `a0`: base layer `+0x00(a0)`, row stride `+0x04(a0)`, column stride shift `+0x08(a0)`, accumulates `or.b` into `d4` and `and.b` into `d5`)
- **Evidence JSON:** `data/thor-evidence/rom_horizontal_wall_probe_m15_401.json`
- **Python Extractor & Tests:** `tools/extract_rom_horizontal_wall_probe.py`, `tests/rom_horizontal_wall_probe_test.py`
