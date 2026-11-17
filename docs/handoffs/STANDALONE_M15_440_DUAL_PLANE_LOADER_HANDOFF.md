# Standalone Handoff: M15.440 Dual Tilemap Plane Header Unpacker, VRAM Plane A/B Layout Configurator, Stride Shift Calculator & Full Plane Initializer

## Routine Overview
- **Source ROM:** `local-roms/Beyond Oasis (USA).md`
- **ROM Address Range:** `0x00D542..0x00D7C0` (638 bytes, 158 instructions)
- **Primary Routine Entry:** `0x00D542` (returns via `rts` at `0x00D7AE`)
- **Local Helper:** `0x00D7B0` (log2 byte stride shift calculator, returns via `rts` at `0x00D7BE`)
- **Key Callers:** Called during scene/room transitions immediately after scene script header unpacking (`0x00D406..0x00D542`).
- **Callees:**
  - `0x003820`: Graphics/Tilemap Decompressor (up to 2 passes: primary tilemaps and optional foreground overlay).
  - `0x00D7B0`: Local log2 byte stride calculator (called twice for Plane A and Plane B).
  - `0x00D7C0`: Plane VRAM Tile/Strip Streamer (called twice for Plane A and Plane B).
  - `0x00CECC`: Camera/Screen Projection Setup Helper.

---

## Memory & Hardware Register Interface

| Address / Register | Type | Direction | Description |
|--------------------|------|-----------|-------------|
| `a4` (register)    | `const uint8_t*` | In/Out | Pointer to ROM scene stream data; advanced past compressed payload |
| `a5` (register)    | `uint8_t*` | In/Out | Pointer to RAM staging buffer; advanced past decompressed payload |
| `$FF3FA8`          | Buffer (RAM) | In/Out | Primary decompression scratch buffer (4-byte header + tile data) |
| `$FF3FAC`          | Tile Data | In | Start of Plane A tilemap payload |
| `$FF1716`          | Struct (40B) | Out | Plane A context record |
| `+$16` (`$FF172C`) | `uint32_t` | Out | Pointer to Plane A tile data buffer |
| `+$1A` (`$FF1730`) | `uint16_t` | Out | Plane A VRAM base address (`#$C000`) |
| `+$20` (`$FF1736`) | `uint16_t` | Out | Plane A width in pixels (`header[0] & 0x7F << 5`) |
| `+$22` (`$FF1738`) | `uint16_t` | Out | Plane A height in pixels (`header[1] << 5`) |
| `+$24` (`$FF173A`) | `uint16_t` | Out | Plane A row stride shift exponent (`d1` from `log2_shift`) |
| `+$27` (`$FF173D`) | `uint8_t`  | Out | Plane A scroll flags (from upper byte of `d6`) |
| `$FF1766`          | Struct (10B) | Out | Plane A auxiliary context record (`$0`: next ptr, `$4`: width, `$6`: height, `$8`: shift-1) |
| `$FF173E`          | Struct (40B) | Out | Plane B context record |
| `+$16` (`$FF1754`) | `uint32_t` | Out | Pointer to Plane B tile data buffer |
| `+$1A` (`$FF1758`) | `uint16_t` | Out | Plane B VRAM base address (`#$E000`) |
| `+$20` (`$FF175E`) | `uint16_t` | Out | Plane B width in pixels (`header[2] & 0x7F << 5`) |
| `+$22` (`$FF1760`) | `uint16_t` | Out | Plane B height in pixels (`header[3] << 5`) |
| `+$24` (`$FF1762`) | `uint16_t` | Out | Plane B row stride shift exponent (`d1` from `log2_shift`) |
| `+$27` (`$FF1765`) | `uint8_t`  | Out | Plane B scroll flags (from lower byte of `d6`) |
| `$FF1770`          | Struct (10B) | Out | Plane B auxiliary context record (`$0`: next ptr, `$4`: width, `$6`: height, `$8`: shift-1) |
| `$FF179E`          | `uint32_t` | Out | End of Plane B tilemap payload / dynamic RAM buffer base |
| `$FF17A2`          | `uint32_t` | Out | Dynamic RAM buffer base + 64 bytes (`0x40`) |
| `$FF17A6`          | `uint32_t` | Out | Optional 2nd pass overlay decompression buffer pointer |
| `$FF17AA`          | `uint32_t` | Out | Final scene ROM stream address |
| `$FF17AE`          | `uint32_t` | Out | Final scene RAM allocation address |
| `$FF1344`          | `uint32_t` | Out | Full-screen Plane A & B H-scroll cleared to 0 |
| `$FF1348`          | `uint32_t` | Out | Full-screen Plane A & B V-scroll cleared to 0 |
| `$FFFB5C`          | Table (256L) | Out | Line scroll table cleared to 0 (if `$FF16F0` bit 1 set) |
| `$FFFF5C`          | Table (20L) | Out | 2-cell column scroll table cleared to 0 (if `$FF16F0` bit 2 set) |
| `$C00004`          | VDP Control | Out | VRAM H-scroll write command (`0x5C000003`) & VSRAM write command (`0x40000010`) |
| `$C00000`          | VDP Data | Out | 256 longwords cleared to VRAM `$FC00`; 20 longwords cleared to VSRAM |
| `$FF0BF8`          | `uint16_t` | In/Out | Shadow VDP register 7 (backdrop color); updated with lower 3 bits of `d7` |
| `$FF134E`          | Palette (32B) | Out | System palette loaded from ROM `0x000476` (15 words) |
| `$FF134C`          | `uint16_t` | Out | Target fade palette register initialized from `$FF16F6` |
| `$05D046`          | ROM Table | In | Scene palette table base; indexed by `$FF16F8` (32 bytes per palette) |
| `$FF177A`          | `uint16_t` | Out | Max horizontal scroll limit in pixels: `(width_tiles - 40) * 8` |
| `$FF177C`          | `uint16_t` | Out | Max vertical scroll limit in pixels: `(height_tiles - 28) * 8` |
| `$FF17C1`          | `uint8_t`  | Out | Room transition active flag cleared to 0 (`sf.b`) |
| `$FF17C2`          | `uint8_t`  | Out | Plane A initialized flag set to 0xFF (`st.b`) |
| `$FF17C3`          | `uint8_t`  | Out | Plane B initialized flag set to 0xFF (`st.b`) |

---

## Algorithm Details

### 1. Header Structure at `$FF3FA8`
The decompressed tilemap package contains a 4-byte header followed immediately by tile data at offset +4:
- Byte 0: Plane A width in 32-pixel blocks (`& 0x7F`). Bit 7 indicates presence of secondary elevation/collision map.
- Byte 1: Plane A height in 32-pixel blocks.
- Byte 2: Plane B width in 32-pixel blocks (`& 0x7F`). Bit 7 indicates presence of secondary map.
- Byte 3: Plane B height in 32-pixel blocks.

### 2. Log2 Stride Shift Calculation (`0x00D7B0`)
For word-sized (2 bytes per cell) tilemaps:
```cpp
uint16_t compute_log2_byte_stride(uint16_t width_px) {
    uint16_t d0 = (width_px + width_px) & 0xFFFF;
    if (d0 == 0) return 0;
    uint16_t d1 = 0;
    while ((d0 & 1) == 0) {
        d0 >>= 1;
        d1++;
    }
    return d1;
}
```
For 32 tiles (256px / 32 words = 64 bytes), `d1 = 6` (`1 << 6 = 64`).
For 64 tiles (512px / 64 words = 128 bytes), `d1 = 7` (`1 << 7 = 128`).
For 128 tiles (1024px / 128 words = 256 bytes), `d1 = 8` (`1 << 8 = 256`).

### 3. VRAM Base Mapping
- Plane A is assigned VRAM base address `#$C000`.
- Plane B is assigned VRAM base address `#$E000`.

### 4. Camera Screen Clamping Limits
The camera tracking plane (Plane A by default, or Plane B if `$FF16F0` bit 7 is set) determines screen boundaries:
- Screen width = 40 tiles = 320 pixels.
- Screen height = 28 tiles = 224 pixels.
- `max_x_scroll = (width_tiles - 40) * 8`
- `max_y_scroll = (height_tiles - 28) * 8`
Both are clamped to 0 minimum.
