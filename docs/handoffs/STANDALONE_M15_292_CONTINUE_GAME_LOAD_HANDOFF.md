# Standalone Handoff: M15.292 — Continue Game SRAM Load & Restore (`0x00161C`..`0x001730`)

## Summary
When continuing a saved slot (`0x00161C`):
- Enables SRAM read (`$A130F1 = 3`), tries **Primary SRAM Copy** (`0x001112`) via `0x0016F4` (`0x12A` words = `596` bytes + `u16_be` sum checksum); if corrupt, falls back to **Backup SRAM Copy** (`0x00114A`); if both fail, locks SRAM (`$A130F1 = 2`) and soft-resets (`0x00020E`).
- Copies `0xBC` words (`376` bytes = Ali `$FF19E8` + Active Spirit `$FF1AA4`) from `$FF0BFE`, reloads spirit resources (`0x014522`), grants `30` (`0x1E`) post-load invulnerability frames (`$FF198E`), and enters room `$FF0E42` at Ali `(x, y)` via `0x00CA24`.

## C++20 Standalone Target
- Implement `ContinueGameLoader` in `src/game/` with primary/backup SRAM fallback, checksum validation, and `30`-frame post-load invulnerability.
- Evidence artifact: `data/thor-evidence/rom_continue_game_load_m15_292.json`.
