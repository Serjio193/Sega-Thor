# Standalone Handoff — M15.321: Room Height-Band Table Builder & Tileset Chunk Loader (`0x00D344..0x00D4FE`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x00D344..0x00D396`: `flush_vscroll_to_vdp_buffer` (`$FFFF5C` 20-col or `$FF1348` full-screen)
  - `0x00D398..0x00D3B0`: `sync_hv_dirty_to_vint` (`$FF16F0 & 0x18` -> `$FF164E`)
  - `0x00D3B2..0x00D404`: `load_single_chunk_dma` (`0x05CE96` -> `0x003820` -> `$FF2FA8`)
  - `0x00D406..0x00D4FE`: `init_room_bands_and_chunks`
- **Evidence JSON:** `data/thor-evidence/rom_room_height_tileset_m15_321.json`
- **Python Extractor & Tests:** `tools/extract_rom_room_height_tileset.py`, `tests/rom_room_height_tileset_test.py`

## Behavior
1. **Height-Band Table (`$FF16FE` & `$FF1706`):**
   - Reads 4 signed bytes from room header `+0x0C..+0x0F`.
   - For each band `i`: `step = int8(band[i]) << 3` -> `$FF16FE[i]`; `$FF1706[2*i] = accum`, `$FF1706[2*i+1] = accum + (step >> 1)`, `accum += step`.
   - Overwrites final entry `$FF1706[7]` (`$FF1714`) with `0xF000`.
2. **Plane State Init & Camera Quantization:**
   - Zeroes 20 words of Plane A (`$FF1716`) and Plane B (`$FF173E`), then sets `cam_x = max(0, x & ~7)`, `cam_y = max(0, y & ~7)`.
3. **4-Slot Tileset Decompression:**
   - Reads 4 chunk IDs from `$FF16FA[0..3]` (from room header `+0x08`); for each non-zero ID, looks up `0x05CE96[id]` and decompresses via `0x003820` into `$FF3FA8 + i * 0x1000`.
