# Standalone Handoff — M15.347: Rapid-Mash & Triple-Tap Input Ring Detectors (`0x008910..0x0089A8`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x008910..0x00894E`: `detect_rapid_mash` (`>= 3` frames with bit 4 set in last 13 frames of `$FF1668` ring)
  - `0x008950..0x0089A8`: `detect_triple_tap` (`3` matching facing-dir nibbles within `<= 25` frames, ignoring `0`, aborting on any other direction)
- **Evidence JSON:** `data/thor-evidence/rom_mash_tripletap_m15_347.json`
- **Python Extractor & Tests:** `tools/extract_rom_mash_tripletap.py`, `tests/rom_mash_tripletap_test.py`
