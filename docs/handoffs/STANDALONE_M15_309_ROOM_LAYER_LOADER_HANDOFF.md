# Standalone Handoff: M15.309 — Room Tilemap & Collision Layer Builder (`0x00D500`..`0x00D7BF`)

## Summary
Room loading (`0x00D500`) waits for async fade (`$FF164D` bit 4), DMAs `0x2000` words of room tileset to VRAM `0x0000`, decompresses the room block via `0x003820` to `$FF3FA8`, and unpacks the 4-byte header into Plane A (`$FF1716`, `0xC000`, `$FF1766` col) and Plane B (`$FF173E`, `0xE000`, `$FF1770` col):
- Widths are `(byte[0] & 0x7F) << 5` and `(byte[2] & 0x7F) << 5` tiles; heights are `byte[1] << 5` and `byte[3] << 5` tiles.
- Row byte-stride shift (`0x00D7B0`) is `ctz(tiles_w * 2)`.
- Bit 7 of `byte[0]` indicates an appended collision/height map of `tilemap_bytes / 2` bytes (`1` byte per tile) right after Plane A's `2`-byte-per-tile nametable.

## C++20 Standalone Target
- Implement `decode_room_layer_header()` in `src/game/world/`.
- Evidence artifact: `data/thor-evidence/rom_room_layer_loader_m15_309.json`.
