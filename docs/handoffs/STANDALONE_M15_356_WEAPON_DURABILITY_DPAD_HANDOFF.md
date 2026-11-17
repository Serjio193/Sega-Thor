# Standalone Handoff — M15.356: ROM Weapon Durability Decrement & 16-Way D-Pad Velocity (`0x008530..0x0086B2`)

## ROM Source & Evidence
- ROM routines:
  - `0x008530..0x0085C2`: Weapon durability decrement & auto-revert to Dagger
  - `0x0085C4..0x0085E0`: `$FF0E3C > $FF1A70` reset & `$FF0DBB = 0xFF`
  - `0x0085E2..0x0086B2`: 16-state D-pad 16.16 fixed-point velocity & facing resolver
- Evidence JSON: `data/thor-evidence/rom_weapon_durability_dpad_m15_356.json`
- Extractor: `tools/extract_rom_weapon_durability_dpad.py`
- Python tests: `tests/rom_weapon_durability_dpad_test.py`

## Exact Behavioral Contract
1. **Weapon Durability Decrement (`0x008530..0x0085C2`)**:
   - Reads slot `idx = $FF0DB0`, entry `[id, dur]` at `$FF0D80 + idx * 2`.
   - Only applies if `2 <= id <= 12` and `dur != 0xFF`.
   - Decrements `dur`; when `dur` becomes `0` (or was `0`), clears the 2B slot, resets `$FF0DB0 = 0`, `$FF0DB1 = 1`, `$FF0D7E = 0x0A`, `$FF19EE = 0`, `$FF1854 = 1`, `$FF1855 = 0`, `$FF1856 = 0x5A`, `$FF198F = 0xFF`, and plays SFX `0x00C00269`.
2. **Threshold Reset Guard (`0x0085C4..0x0085E0`)**:
   - Unsigned compare `u16($FF0E3C) > u16($FF1A70)`: if greater, clears `$FF0E3C = 0` and sets `$FF0DBB = 0xFF`.
3. **16-Way D-Pad Velocity & Facing (`0x0085E2..0x0086B2`)**:
   - Indexes 16-entry jump table `0x0085FA` by `$FF165E & 0x0F`.
   - Idle/cancel states (`0, 3, C, F`): `vx = 0, vy = 0`, facing unchanged, `moving = false`.
   - Cardinal speeds: `±0x36000` (X), `±0x30000` (Y); Diagonal speeds: `±0x2A000` (X), `±0x25800` (Y).
   - Facing: `0=Up`, `1=Right`, `2=Down`, `3=Left` (diagonals map to `0=Up` or `2=Down`).
