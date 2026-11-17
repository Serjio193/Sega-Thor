# Standalone Handoff: M15.291 — New Game Initialization (`0x001300`..`0x00144F`)

## Summary
Starting a New Game from File Select (`0x001300`):
- Clears `0x12A` words (`596` bytes) from `$FF0BFE`, sets `$FF0D80 = 0x01FF`.
- Initializes Ali (`$FF19E8`): `id = 2`, `facing = 2`, `MaxHP = HP = 200` (`0xC8`), `MaxSP = SP = 0`, `base_attack = 10` (`$FF0D7E = 0x0A`), `active_spirit = 0`.
- Runs intro cutscene (`0x03BF86`), then enters `0x00CA38` with **Room `0x001B` (`27`)**, camera `(96, 0)`, and Ali at **`(256, 80)`**.
- Checks 4 secret P1+P2 input chords on `$FF165E.L`: `0xD6000100 -> 0x03C8DE`, `0xC9000200 -> 0x001450`, `0xCA000100 -> $FF0D7A=0x80`, `0x90000000 -> 0x03E7F4`.

## C++20 Standalone Target
- Align `NewGameInitializer` in `src/game/` with exact ROM initial stats (`HP=200, ATK=10, Room=0x1B, Pos=(256, 80)`).
- Evidence artifact: `data/thor-evidence/rom_new_game_init_m15_291.json`.
