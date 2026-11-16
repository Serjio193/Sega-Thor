# Standalone Handoff: M15.287 — World Map Screen & Pin Coordinate Tables (`0x004950`..`0x004C27`)

## Summary
The World Map / Pause Map screen (`0x004950`) decompresses 3 map tile chunks (`0x15BAC2 -> 0x9500`, `0x15C238 -> 0xA180`, `0x15CA9C -> 0xAE00`, `0x640` DMA words each) and computes Ali's blinking map pin (`0x004B4E`) from either the dungeon pin table (`0x004BE8`) or overworld area table (`0x004BAE`) + `(player_x >> 6, player_y >> 6)`.

## Pin Coordinate Formula (`0x004B4E`)
- **Center origin:** `(160, 120)` (`0xA0, 0x78`).
- **If in dungeon (`$FF0E40 != 0`):** index `0x004BE8` by `dungeon_id = $FF0E40` (or `dungeon_id + 6` when `$FF0E40 == 0x0F01`), returning `(160 + dx, 120 + dy)`.
- **If in overworld (`$FF0E40 == 0`):** index `0x004BAE` (`29` entries) by `area_id = $FF0E41`, returning `(160 + dx + (player_x >> 6), 120 + dy + (player_y >> 6))`.

## C++20 Standalone Target
- Implement `WorldMapScreen` / `compute_world_map_pin()` in `src/game/ui/` using the exact 29 overworld offsets and 16 dungeon pins.
- Evidence artifact: `data/thor-evidence/rom_world_map_m15_287.json`.
