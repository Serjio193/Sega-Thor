# Standalone Handoff: M15.281 — Dialogue / Text Engine & 116-Entry String Table (`0x00C386`..`0x00C755`, `0x051514`)

## Summary
Beyond Oasis renders in-game dialogue using an 8x8 4bpp ASCII font decompressed from `0x16943C` via `0x003820` (`32` bytes per glyph, indexed by `(ch - 0x20) << 5`), reading NUL-terminated script strings from a 116-entry self-relative `u16` offset table at `0x051514` indexed by `$FF1870`.

## Engine & Control Codes (`0x00C40E`..`0x00C534`)
- `0x00` (`0x00C4AA`): `end_dialogue` — set `$FF1887 = 0xFF`, wait button (`0x00C536`), clear `$FF1887`, close box (`0x00C45C`).
- `0x01` (`0x00C4BC`): `newline` — scroll/advance line (`0x00C566`), yield 1 frame (`0x00C84E`).
- `0x02` (`0x00C4C8`): `wait_button_newline` — wait button (`0x00C536`) then `newline`.
- `0x03` (`0x00C4CE`): `delay_frames` — read next `u8` count, yield `count` frames.
- `0x05` (`0x00C4E4`): `wait_button` — wait button (`0x00C536`) and continue.
- `0x06` (`0x00C4EC`): `clear_page` — scroll 4 lines, reset `$FF187E = 0`, `$FF1880 = 0`, line offsets `$FF1876..$FF187C = {0x0C, 0x18, 0x24, 0x30}`.
- `0x07` (`0x00C530`): `wait_button_clear_page` — wait button (`0x00C536`) then `clear_page`.
- `>= 0x20` (`0x00C6BE`): emit 16-word (32-byte) VDP DMA tile transfer for glyph `(ch - 0x20) << 5` from `$FF2FA8`, increment column `$FF187E`.

## Epilogue / Credits Drawer (`0x03D160`)
Uses the same `0x16943C` font decompressed to `$FF3FAC`, with `0x00` = end of line, `0x01 <u8>` = set per-character frame delay `$FF1658` (default `3`), and `>= 0x20` = Plane A (`0xC000`) tilemap + DMA glyph upload.

## C++20 Standalone Target
- Implement `DialogueEngine` in `src/game/ui/` (or `src/game/dialogue/`) supporting all 7 control codes (`0x00..0x07`) and font glyph indexing from `0x16943C`.
- Evidence artifact: `data/thor-evidence/rom_dialogue_engine_m15_281.json`.
