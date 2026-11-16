# Standalone Handoff: M15.283 — Title Screen, Attract Timeout & Secret Sound Test (`0x0032F8`..`0x003586`)

## Summary
Game Mode `0x04` (`0x0032F8`) runs the Title Screen: decompresses font `0x16943C` to VRAM `0xF020` (`$FF2FA8`), decompresses title graphics `0x180C56` to `$FF3FAC`, starts BGM `0x0104`, displays `"TM & @ 1995 SEGA"` (`0x00355A`) and `"Music @ 1994 Yuzo Koshiro"` (`0x00356B`), and blinks `"PRESS START"` (`timer & 0x7F < 0x60`) for `0x2D0` (`720`) frames before entering Attract/Demo Mode (`0x08`, `$FF1996 = 0xFF`).

## Start-Button Input Chords (`$FF165F & 0x80` -> `$FF165E`)
- `0x90` (`0x0034BE`): Play SFX `0xA003AA` -> jump to `0x03E4FA` (Sound Test / Secret Menu)
- `0xD5` (`0x0034E4`): Play SFX `0xA00205` -> `0x001E98` -> jump to `0x03E4FA` (Debug / Stage Select)
- `0x8A` (`0x003428`): Enter Demo/Attract Mode (`$FF10AC = 0x08`, `$FF1996 = 0xFF`)
- Any other (`0x00347C`): Play sound `0x02` -> enter File Select / Gameplay (`$FF10AC = 0x0C`, `$FF1996 = 0x00`)

## C++20 Standalone Target
- Implement `TitleScreenState` in `src/game/` with the `720`-frame attract timeout, `128`-frame blink duty (`< 96` visible), and all 4 start-chord branches.
- Evidence artifact: `data/thor-evidence/rom_title_and_sound_test_m15_283.json`.
