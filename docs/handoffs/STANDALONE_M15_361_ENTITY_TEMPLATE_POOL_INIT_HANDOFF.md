# Standalone Handoff — M15.361: ROM 5-Pool Resetter, 32B Entity Template Initializer & Tint Ramp (`0x008C5A..0x008E20`)

## ROM Source & Evidence
- ROM routines:
  - `0x008C5A..0x008D04`: Master 5-pool resetter
  - `0x008D06..0x008DA2`: 32-byte entity template initializer (`0x03F2FA`)
  - `0x008DA4..0x008DD2`: Entity 5-pointer reloader (`0x03F2FA`)
  - `0x008DD4..0x008E20`: Screen-tint / pulse intensity state (`$FF183E`)
- Evidence JSON: `data/thor-evidence/rom_entity_template_pool_init_m15_361.json`
- Extractor: `tools/extract_rom_entity_template_pool_init.py`
- Python tests: `tests/rom_entity_template_pool_init_test.py`

## Exact Behavioral Contract
1. **5-Pool Resetter (`0x008C5A..0x008D04`)**:
   - Clears `$FF2954` (4x `0x5A`), `$FF2D8C` (6x `0x5A`), `$FF2ABC` (8x `0x5A`), `$FF19C8` (4x `0x08` type-only), and `$FF1B60` (19x `0xBC`, plus `+0x5C = -1`).
2. **Entity Template Initializer (`0x008D06`, `0x008DA4`)**:
   - Clears `0x5D` words (`0x02..0xBA`) if `entity < $FF2954`, else `0x2C` words (`0x02..0x58`).
   - Indexes `0x03F2FA + (type_id << 4) - 0x20` (`32B` per even `type_id >= 2`).
   - Populates height (`+0x4A`), radius (`+0x42/+0x44/+0x46`), sprite extents (`+0x48/+0x49`), flags (`+0x36..+0x38, +0x3A, +0x3C, +0x3D`), `+0x4C`, and 5 ROM pointers (`+0x1A, +0x1E, +0x26, +0x22`, plus `+0x5E` and `+0x5C = -1` if `entity < $FF2954`). Sets `+0x2C = -1`, `+0x34 = -1`.
3. **Tint Intensity Ramp (`0x008DD4..0x008E20`)**:
   - Active (`$FF1AA4 != 0`): ramps `$FF183E` up toward `3`; if `$FF199E >= 0x14` and `($FF199F & 1) == 0`, sets `4`.
   - Inactive: `0 -> 0`, `1..3 -> dec`, `>=4 -> 2`.
