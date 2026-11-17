# Standalone Handoff — M15.346: Bomb Detonation Blast Spawner & Screen-Shake (`0x00ED04..0x00EDA8`)

## Summary
- **Source routine in Beyond Oasis (USA):** `0x00ED04..0x00EDA8`
- **Evidence JSON:** `data/thor-evidence/rom_bomb_detonation_m15_346.json`
- **Python Extractor & Tests:** `tools/extract_rom_bomb_detonation.py`, `tests/rom_bomb_detonation_test.py`

## Behavior
- Spawns/converts to blast entity type `0x3A` (`0x008D06`), lowering `z` by `0x00040000` (`4.0 px`, clamped `>= 0`).
- Sets damage `0x8007`, hit-type `2`, state/anim `8`, plays `0x00B00225` + `0x00600382`, and triggers `4`-frame screen shake (`$FF17C4 = 4`).
