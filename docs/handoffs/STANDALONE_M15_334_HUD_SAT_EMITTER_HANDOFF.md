# Standalone Handoff — M15.334: HUD Frame SAT & Item-Toast DMA Emitter (`0x00A342..0x00A436`)

## Summary
- **Source routine in Beyond Oasis (USA):** `0x00A342..0x00A436`
- **Evidence JSON:** `data/thor-evidence/rom_hud_sat_emitter_m15_334.json`
- **Python Extractor & Tests:** `tools/extract_rom_hud_sat_emitter.py`, `tests/rom_hud_sat_emitter_test.py`

## Behavior
1. Emits 6 primary HUD SAT sprites shifted horizontally by `$FF185A`.
2. If `$FF184F != 0`, emits 3 enemy-gauge SAT sprites shifted by `0x0158 - $FF185A`.
3. If `$FF1856 != 0` and SAT count `< 80`, emits a 3x3 (`0x0A`) item-toast sprite (`tile 0x85C7`, `Y = 0x0118`, `X = $FF185A + 0x68`), and if `$FF1854 != $FF1855`, queues a `0x90`-word DMA from `0x000AAEBB + item_id * 0x90` to VRAM `0xB8E0`.
