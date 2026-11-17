# Standalone Handoff — M15.394: Ali Rapid-Mash, Triple-Tap Motion Detectors & Demo Stream Loader (`0x008910..0x008AA6`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x008910..0x00894E`: `rapid_mash_detector` (scans 13 frames in `$FF1668` ring buffer starting at `$FF16E8`; counts button bit 4 presses; if count >= 3, clears `$FF16EA = 0` and sets Carry flag)
  - `0x008950..0x0089B0`: `triple_tap_detector` (scans up to 25 frames from `$FF16E8` clamped to `$FF16EA`; matches 3 cardinal D-pad frames according to `0x0087F4` facing mapping [Up 0x01, Right 0x08, Down 0x02, Left 0x04]; ignores neutral 0 frames; aborts on mismatch; clears `$FF16EA = 0` and sets Carry flag on 3 matches)
  - `0x0089B2..0x008A92`: `demo_stream_initializer` (handshakes V-Int on `$FF164D` bit 2; stores stack pointer in `$FF1930`; checks demo active flag `$FF1996` [branches to title `$4C6` if clear]; cycles demo index `$FF0BFC` 0..8; looks up asset offset in 9-entry table `0x008A94` relative to ROM bank `0x1698FE`; decompresses via `0x00003820` into `$FFBFAC`; performs 0x1FFE cumulative delta reconstruction into `$FFDFB2`; prepares demo state `$FF1998`, `$FF1928 = 0x7F28`, `$FF1997 = 0xFF`; enters coordinator `0x00CA24`)
  - `0x008A94..0x008AA6`: `demo_asset_offsets_table` (9 word offsets `[0x0000, 0x042C, 0x0BC0, 0x1202, 0x144C, 0x16C8, 0x18E0, 0x1EE8, 0x201E]` corresponding to absolute ROM addresses `[0x1698FE, 0x169D2A, 0x16A4BE, 0x16AB00, 0x16AD4A, 0x16AFC6, 0x16B1DE, 0x16B7E6, 0x16B91C]`)
- **Evidence JSON:** `data/thor-evidence/rom_ali_rapid_tripletap_demo_m15_394.json`
- **Python Extractor & Tests:** `tools/extract_rom_ali_rapid_tripletap_demo.py`, `tests/rom_ali_rapid_tripletap_demo_test.py`
