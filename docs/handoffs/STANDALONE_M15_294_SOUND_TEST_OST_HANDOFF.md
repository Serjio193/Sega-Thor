# Standalone Handoff: M15.294 — Secret Sound Test & 34-Track Named OST (`0x03E7F4`..`0x03F047`)

## Summary
The Secret Sound Test screen (`0x03E7F4`, unlocked via New Game `A+Start` `0x90000000`) exposes `"SOUND TEST"`, `"BGM"`, `"SFX"`, `"PCM"`, `"EXIT"` (`0x03ED48`) and maps menu indices `0..33` via `0x03ED7E` to all 34 Yuzo Koshiro BGM tracks (`0x0100..0x0121`) with their official titles at `0x03EDA0..0x03F047` (e.g. `0 -> 0x0115 "Introduction"`, `5 -> 0x0104 "The Story of Thor"`, `22 -> 0x011D "Last battle"`, `32 -> 0x011B "Ending"`, `33 -> 0x0121 "Master mind"`).

## C++20 Standalone Target
- Add `SoundTestTrackEntry` & 34-track OST metadata lookup in `src/game/audio/` (or `src/game/ui/`).
- Evidence artifact: `data/thor-evidence/rom_sound_test_ost_m15_294.json`.
