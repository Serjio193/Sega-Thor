# Standalone Handoff — M15.340: Ali Attack Action & Weapon-Class State Dispatcher (`0x008002..0x00814C`)

## Summary
- **Source routine in Beyond Oasis (USA):** `0x008002..0x00814C`
- **Evidence JSON:** `data/thor-evidence/rom_ali_attack_dispatch_m15_340.json`
- **Python Extractor & Tests:** `tools/extract_rom_ali_attack_dispatch.py`, `tests/rom_ali_attack_dispatch_test.py`

## Dispatch Table
- `0x0A` (Dagger): `(anim=0x3A, state=0x0A)` on ground; `(anim=0x40, state=0x38, 0x9A=0x15)` air-kick in mid-air (`$4(a6)==4`).
- `0x0C` (Sword): `(0x66, 0x5C)` spin, `(0x60, 0x54)` thrust if D-pad held, `(0x4E, 0x46)` slash.
- `0x0E` (Bow): requires `$FF1B60 == 0`; SFX `0x00600377`; `(0x54, 0x4C)` or `(0x5C, 0x4C)` for item `8`.
- `0x10` (Bomb): requires `$FF1B60 == 0` for `(0x58, 0x50)` throw.
