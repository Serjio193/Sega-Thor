# Standalone Handoff — M15.313: Incremental Row/Col VDP Plane Scroller (`0x0023CC..0x002630`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x0023CC..0x002510`: **Incremental Column Streamer** (`29` tiles tall, VDP auto-increment `0x8F80` -> restored `0x8F02`)
  - `0x002512..0x002630`: **Incremental Row Streamer** (`41` tiles wide, VDP auto-increment `0x02`)
- **Evidence JSON:** `data/thor-evidence/rom_vdp_plane_scroll_m15_313.json`
- **Python Extractor & Tests:** `tools/extract_rom_vdp_plane_scroll.py`, `tests/rom_vdp_plane_scroll_test.py`

## Plane State Struct (`a3`)
| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x08` | `int16` | `src_col` | Current top-left source tilemap column |
| `0x0A` | `int16` | `src_row` | Current top-left source tilemap row |
| `0x0C` | `uint16` | `vram_offset` | Byte offset inside 64x32 VDP nametable (`0x0000..0x0FFE`), `col_byte = off & 0x7E`, `row_byte = off & 0xF80` |
| `0x16` | `uint32` | `tilemap_ptr` | Word tilemap base |
| `0x1A` | `uint16` | `vram_base` | Nametable VRAM base (`0xC000` / `0xE000`) |
| `0x1C` | `int16` | `pending_dx_tiles` | Signed column step delta (`>= 0` scrolls right, `< 0` scrolls left) |
| `0x1E` | `int16` | `pending_dy_tiles` | Signed row step delta (`>= 0` scrolls down, `< 0` scrolls up) |
| `0x20` | `uint16` | `map_width_tiles` | Source map width in tiles |
| `0x22` | `uint16` | `map_height_tiles` | Source map height in tiles |
| `0x24` | `uint16` | `row_shift` | Power-of-two byte shift for `src_row << row_shift` |

## Algorithm
1. **Column Streamer (`0x0023CC`):**
   - If `pending_dx_tiles >= 0`: decrement `pending_dx_tiles` by 1, increment `src_col` (mod `map_width_tiles`), read column `(src_col + 40) % map_width_tiles`, advance `vram_offset` col byte by `+2 & 0x7E`, write to VDP col `(next_col_byte + 0x50) & 0x7E`.
   - If `pending_dx_tiles < 0`: increment `pending_dx_tiles` by 1, decrement `src_col` (mod `map_width_tiles`), read `src_col`, decrement `vram_offset` col byte by `-2 & 0x7E`, write to that VDP col.
   - Stream **29 tiles** vertically (`dbra #0x1C`), wrapping source row mod `map_height_tiles` (with `0x03E8` single-wrap guard) and VDP row mod `32` (`-0x1000` bytes).
2. **Row Streamer (`0x002512`):**
   - If `pending_dy_tiles >= 0`: decrement `pending_dy_tiles` by 1, increment `src_row` (mod `map_height_tiles`), read row `(src_row + 28) % map_height_tiles`, advance `vram_offset` row byte by `+0x80 & 0xF80`, write to VDP row `(next_row_byte + 0xE00) & 0xF80`.
   - If `pending_dy_tiles < 0`: increment `pending_dy_tiles` by 1, decrement `src_row` (mod `map_height_tiles`), read `src_row`, decrement `vram_offset` row byte by `-0x80 & 0xF80`, write to that VDP row.
   - Stream **41 tiles** horizontally (`dbra #0x28`), wrapping source col mod `map_width_tiles` and VDP col mod `64` (`-0x80` bytes).
