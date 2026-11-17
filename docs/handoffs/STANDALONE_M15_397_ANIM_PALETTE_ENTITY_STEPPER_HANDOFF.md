# Standalone Handoff — M15.397: Animated Palette CRAM Sequencer & 3-Group Entity Master Stepper (`0x008E22..0x008F22`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x008E22..0x008E8E`: `anim_palette_sequencer` (checks flag `$FF164F`; loops through palette animations at `$FF17A2`; updates target CRAM colors in `$FF134C`; sets CRAM sync flag `$FF164D` bit 0 on color changes; loops animation sequence on sentinel 0)
  - `0x008E90..0x008EF6`: `entity_master_stepper` (scans 3 entity groups with recovery jump targets in `$FF193C`: Group 1 at `$FF2954` [4 slots, stride 90B -> `0x8F12`], Group 2 at `$FF19E8` [21 slots, stride 188B -> `0x8F22`], Group 3 at `$FF2D8C` [6 slots, stride 90B -> `0x8F12`])
  - `0x008EF8..0x008F10`: `entity_state_reset` (sets `+0x2A = d0`, `+0x2C = 0xFFFF`, clears flashing `+0x37 &= 0xF9`, and clears subtimers `+0x30 = 0`, `+0x2E = 0`)
  - `0x008F12..0x008F22`: `secondary_entity_dispatch` (sets active bit 4 on `+0x37`, branches to secondary entity script machine `0x39CA`)
- **Evidence JSON:** `data/thor-evidence/rom_anim_palette_entity_stepper_m15_397.json`
- **Python Extractor & Tests:** `tools/extract_rom_anim_palette_entity_stepper.py`, `tests/rom_anim_palette_entity_stepper_test.py`
