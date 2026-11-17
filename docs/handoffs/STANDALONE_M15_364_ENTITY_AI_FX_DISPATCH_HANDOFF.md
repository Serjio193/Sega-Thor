# Standalone Handoff — M15.364: ROM Entity AI Dispatch Table, Iso Extent Updater & FX Loops (`0x00DEEC..0x00E150`)

## ROM Source & Evidence
- ROM routines:
  - `0x00DEEC..0x00DF52`: 20-slot enemy/object AI dispatch + isometric hitbox half-extent updater
  - `0x00E0BA..0x00E0F2`: 6-slot secondary FX callback runner (`$FF2D8C`)
  - `0x00E0F4..0x00E150`: HUD toast timer decrement (`$FF1856`) + 8-slot pickup/particle dispatcher (`$FF2ABC`)
- Evidence JSON: `data/thor-evidence/rom_entity_ai_fx_dispatch_m15_364.json`
- Extractor: `tools/extract_rom_entity_ai_fx_dispatch.py`
- Python tests: `tests/rom_entity_ai_fx_dispatch_test.py`

## Exact Behavioral Contract
1. **Entity AI Loop (`0x00DEEC..0x00DF52`)**:
   - Skipped when `$FF17B8` bit 1 (time-stop) is set.
   - Iterates 20 `0xBC`-byte slots at `$FF1AA4`: for `type_id >= 0x16` (even IDs `0x16..0xC6`), calls `u32` handler at `0x00DF28 + type_id * 2` (`89` entries at `0x00DF54..0x00E0B7`).
   - Post-AI isometric extent update: `d1 = +0x48`, `d2 = +0x49`; swaps `d1 <-> d2` if `(+0x17 & 1) != 0`; stores `+0x42 = d1` and `+0x44 = ((d2 + 1) >> 1) + ((d2 + 1) >> 3? no: + ((d2+1)>>2))` (`t = (d2 + 1) >> 1; y_half = t + (t >> 1)`).
2. **FX Callback Loop (`0x00E0BA..0x00E0F2`)**:
   - Skipped on time-stop; loops 6 `0x5A`-byte slots at `$FF2D8C`, calling `*(u32*)(slot+0x56)` if `type != 0` and pointer nonzero.
3. **Toast Timer & Pickup Loop (`0x00E0F4..0x00E150`)**:
   - Always decrements `$FF1856` if `> 0`.
   - If not time-stopped, loops 8 `0x5A`-byte slots at `$FF2ABC` (`type > 0`), dispatching via `+0x04` into `0x00E140`.
