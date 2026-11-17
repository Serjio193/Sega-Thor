# Standalone Handoff: M15.441 Visible Tilemap Plane VRAM Streamer & Full-Screen Blank Tile Plane Clearer

## Routine Overview
- **Source ROM:** `local-roms/Beyond Oasis (USA).md`
- **ROM Address Range:** `0x00D7C0..0x00D9A4` (484 bytes, 162 instructions)
- **Primary Routines:**
  - `0x00D7C0`: `vdp_upload_visible_tilemap_plane` (calculates visible tile bounds, handles toroidal wrapping on X/Y, streams tile words to VDP data port `$C00000`, returns via `rts` at `0x00D94E`).
  - `0x00D950`: `vdp_fill_plane_blank_safe` (registers-saving wrapper calling `0x00D962`, returns via `rts` at `0x00D95C`).
  - `0x00D95E`: `vdp_fill_plane_blank_raw` (generates VRAM command for Plane base address at `$1A(a3)`, clears 40x28 grid with blank tile `#$0780`, returns via `rts` at `0x00D9A2`).
- **Callers in ROM:**
  - Called by `0x00D6FE` (Plane A initial upload) and `0x00D708` (Plane B initial upload).
  - Also called dynamically during gameplay when entire planes require full refresh or blanking.

---

## Memory & Hardware Register Interface

| Address / Register | Type | Direction | Description |
|--------------------|------|-----------|-------------|
| `a3` (register)    | Struct ptr | In | Pointer to Plane context record (`$FF1716` for Plane A, `$FF173E` for Plane B) |
| `+$00`             | `int16_t`  | In | Camera X coordinate |
| `+$04`             | `int16_t`  | In | Camera Y coordinate |
| `+$08`             | `uint16_t` | Out | Tile column offset within plane |
| `+$0A`             | `uint16_t` | Out | Tile row offset within plane |
| `+$0C`             | `uint16_t` | In/Out | Current VRAM ring-buffer offset |
| `+$0E`             | `uint16_t` | Out | Horizontal scroll register offset (zeroed) |
| `+$10`             | `uint16_t` | Out | Vertical scroll register offset (zeroed) |
| `+$16`             | `const uint16_t*` | In | Base pointer to tilemap payload data |
| `+$1A`             | `uint16_t` | In | VRAM base address (`#$C000` for Plane A, `#$E000` for Plane B) |
| `+$1C`             | `uint16_t` | Out | Column delta dirty indicator (zeroed) |
| `+$1E`             | `uint16_t` | Out | Row delta dirty indicator (zeroed) |
| `+$20`             | `uint16_t` | In | Plane width in tiles |
| `+$22`             | `uint16_t` | In | Plane height in tiles |
| `+$24`             | `uint16_t` | In | Row stride shift exponent (`d1` from `log2_shift`) |
| `+$27`             | `uint8_t`  | In | Plane flags: bit 0 = plane active (1 = stream tiles, 0 = fill with blank tile `#$0780`) |
| `$C00000`          | VDP Data | Out | VRAM data port; tile words written directly |
| `$C00004`          | VDP Control | Out | VRAM write commands issued per row |

---

## Algorithm Details

### 1. Active Plane Check & Blank Tile Fallback
At entry `0x00D7C0`:
- Clears offsets `+$08`, `+$0A`, `+$0C`, `+$0E`, `+$10`, `+$1C`, `+$1E`.
- Tests bit 0 of `+$27(a3)`:
  - If 0 (`beq.w $d95e`): Branches directly to `0x00D95E` to clear the entire 40x28 plane with blank tile `#$0780`.
  - If 1: Proceeds with visible tilemap extraction and VRAM streaming.

### 2. Camera Coordinate Normalization & Toroidal Wrapping
Visible tile origin is computed by dividing camera coordinate by 8 (`asr.w #3, d0`):
- Wraps negative coordinates by adding plane size (`add.w d7, d0`).
- Wraps oversized coordinates by subtracting plane size (`sub.w d7, d0`).
Stores wrapped tile column into `+$08(a3)` and wrapped tile row into `+$0A(a3)`.

### 3. Screen Dimensions & Tile Count
- Screen display size: 40 tiles wide by 28 tiles high (320x224 pixels).
- When plane dimensions match 64x32 or smaller, bit 1 of `+$27(a3)` selects 64x32 mode (`0x3F001F`), otherwise standard 40x28 (`0x28001C`).

### 4. VDP VRAM Write Command Generation
Converts VRAM base address into Sega Genesis 32-bit VRAM write command:
```cpp
uint32_t compute_vram_write_cmd(uint16_t vram_addr) {
    uint32_t high = ((vram_addr & 0x3FFF) | 0x4000);
    uint32_t low = ((vram_addr >> 14) & 0x0003);
    return (high << 16) | low;
}
```

### 5. Blank Tile Plane Clearer (`0x00D95E..0x00D9A4`)
- Reads VRAM plane base address from `+$1A(a3)` (`$C000` or `$E000`).
- Generates 32-bit VRAM write command to start of plane.
- Iterates across 28 rows (`d2 = 0x1C`), and for each row iterates across 40 columns (`d1 = 0x28`), writing blank tile `#$0780` (1120 words total).
- Row advance step: `add.l #$800000, d7` (advances VRAM write address by 128 bytes = 64 cells per row).
