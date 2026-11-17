# Standalone Handoff — M15.396: Master Entity Subsystems Clear, Universal Constructor & Spirit Glow Pulse (`0x008C5A..0x008E22`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x008C5A..0x008D04`: `master_entity_subsystems_clear` (clears 5 pools: `$FF2954` 4x90B, `$FF2D8C` 6x90B, `$FF2ABC` 8x90B, `$FF19C8` 4x8B, `$FF1B60` 19x188B; clears headers and sets timers to `0xFFFF`)
  - `0x008D06..0x008DA2`: `universal_entity_constructor` (clears 44 words if slot >= `$FF2954`, else 93 words; looks up 32-byte descriptor table `0x0003F2FA` at `(type - 2) * 16`; loads 16 fields: sprite `$4A`, dimensions `$46/$42/$44`, offsets `$48/$49`, attribute `$3D`, height `$4C`, anim flags `$3A`, collision `$36`, defense `$37`, render `$38`, AI `$3C`, pointers `$1A/$1E/$26/$22/$5E`, and sets timers `$2C/$34/$5C = 0xFFFF`)
  - `0x008DA4..0x008DD2`: `entity_script_pointers_reinit` (reloads 5 script/state pointers `$1A`, `$1E`, `$26`, `$22`, `$5E` from descriptor table `0x0003F2FA`)
  - `0x008DD4..0x008E22`: `spirit_glow_pulse_controller` (ramps glow `$FF183E` 0->1->2->3 when spirit active, pulses to 4 on even frames >= 20; fades down 4->2->1->0 when dismissed)
- **Evidence JSON:** `data/thor-evidence/rom_entity_constructor_lifecycle_m15_396.json`
- **Python Extractor & Tests:** `tools/extract_rom_entity_constructor_lifecycle.py`, `tests/rom_entity_constructor_lifecycle_test.py`
