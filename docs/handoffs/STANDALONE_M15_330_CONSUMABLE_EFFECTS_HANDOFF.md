# Standalone Handoff — M15.330: Consumable Item Validation & HP/SP/Spirit Restore (`0x005236..0x005472`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x005236..0x00530A`: `can_use_consumable(item_id)`
  - `0x00530C..0x005472`: `apply_consumable(item_id)` (17-entry jump table at `0x00532C` for IDs `0x1A..0x2A`)
- **Evidence JSON:** `data/thor-evidence/rom_consumable_effects_m15_330.json`
- **Python Extractor & Tests:** `tools/extract_rom_consumable_effects.py`, `tests/rom_consumable_effects_test.py`

## Behavior
- Area `0x10` blocks all consumables.
- `0x1A..0x1D` (Spirit Gems): requires `!($FF16F1 & 1)`, `$FF1AA4 == 0`, `SP > 0`, and spirit bit `item_id - 0x1A` in `$FF0D7A`; refills `SP = MaxSP` and summons spirit via `0x007E9E`.
- `0x1E..0x21` (HP Foods): requires `HP != MaxHP`; restores `25% / 50% / 75% / 100%` of `MaxHP`, clamped to `MaxHP`, SFX `0x00A00234`.
- `0x22..0x25` (SP Foods): requires `SP != MaxSP`; restores `25% / 50% / 75% / 100%` of `MaxSP`, clamped to `MaxSP`, SFX `0x00A00234`.
- `0x26..0x2A` (Elixirs): requires `HP != MaxHP || SP != MaxSP`; restores both HP and SP.
