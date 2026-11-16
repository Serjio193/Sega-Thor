# Standalone Handoff: M15.284 — Best 5 Players / Clear Records Screen (`0x03E034`..`0x03E60D`)

## Summary
Entered from Title Screen (`Mode 0x04`) via Start-chord `0x90` (`0x0034BE`) or `0xD5` (`0x0034E4`, which first clears SRAM record magic bytes `$2025CD`/`$2026E1` at `0x001E98`), `0x03E4FA` renders the **"BEST 5 PLAYERS"** screen (`TIME`, `RANK`, `KILLS`, `JEWELS` across `1st..5th` slots).

## Screen Flow
- Loads clear records from SRAM via `0x001DDC` into `$FF2FA8`.
- Renders header & 5-row grid (`0x03E25E`) and stats (`0x03E2FC`, `0x03E3D4`, `0x03E436`, `0x03E498`).
- Plays BGM `0x0121` (`0x03E5AE`).
- Waits for Start (`$FF165F & 0x80`), plays SFX `0x0007` (`0x03E5E2`), fades out, and soft-resets to `0x00020E`.

## C++20 Standalone Target
- Implement `ClearRecordsScreen` in `src/game/ui/` backed by SRAM clear-record persistence (`$2025CD`, `$2026E1`) and 5-slot ranking display.
- Evidence artifact: `data/thor-evidence/rom_clear_records_screen_m15_284.json`.
