# Standalone Handoff — M15.388: ROM Ali Jump Initiation, Weapon Hotkey Swap & Dagger Attack Dispatcher (`0x007E46..0x00803C`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x007E46..0x007EC2`: `ali_jump_initiation` (jump button C logic: checks slope/flag `$FF1985`, aborts if already airborne `state == 4` or `$FF1AA4 != 0`, sets jump/ledge state `state = 0x14, anim = 0x14`, clears `$FF1990 = 0`, plays jump SFX `#$900359`, unrolls stack via `$FF1934.l`).
  - `0x007EC4..0x007F80`: `ali_weapon_hotkey_swap` (Mode/Weapon button hotkey swap: compares selected `$FF0E50` vs current `$FF0DB0`, indexes 8 weapon slots at `$FF0D80`; if slot is empty, plays empty buzzer SFX `#$A00253`; if weapon present, maps ID to type `0x0A` Dagger, `0x0C` Sword, `0x0E` Bow, `0x10` Bomb; sets `$FF0D7E`, resets `$FF19EE = 0`, sets HUD weapon ID `$FF1854`, sets 60-frame toast timer `$FF1856 = 0x3C`, sets swap flag `$FF198F = 1`, plays equip chime SFX `#$C0025D`).
  - `0x007F82..0x00803C`: `ali_attack_weapon_router` (Attack button B logic: routes based on equipped weapon `$FF0D7E`: `0x0C` Sword, `0x0E` Bow, `0x10` Bomb; default Dagger strike initializes `state = 0x0A, anim = 0x3A`, combo timer `$FF198B = 4`, unrolls stack via `$FF1934.l`).
- **Evidence JSON:** `data/thor-evidence/rom_ali_jump_weapon_swap_m15_388.json`
- **Python Extractor & Tests:** `tools/extract_rom_ali_jump_weapon_swap.py`, `tests/rom_ali_jump_weapon_swap_test.py`
