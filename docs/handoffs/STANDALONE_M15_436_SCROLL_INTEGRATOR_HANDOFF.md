# STANDALONE HANDOFF: M15.436 — ROM Plane A 512x256 Ring-Buffer Scroll Integrator, Tile Strip Dirty Marker & Toroidal Modulo Normalizer

## 1. Routine Overview
- **Addresses:** `0x00D146..0x00D2B4` (366 bytes)
- **Primary Entrypoints:**
  - `0x00D146`: `accumulate_long_delta_and_integrate` (Adds 32-bit fixed-point deltas to camera X/Y and integrates)
  - `0x00D168`: `accumulate_word_delta_and_integrate` (Adds 16-bit integer deltas to camera X/Y and integrates)
  - `0x00D17E`: `check_enable_and_integrate` (Checks enable bit 0 on `+0x27(a0)` before integrating)
  - `0x00D188`: `integrate_scroll_deltas` (Calculates pixel deltas, VDP Plane A ring-buffer scroll values, tile column/row shifts, and marks dirty status flags)
  - `0x00D266`: `normalize_toroidal_map_coords` (Normalizes camera coordinates modulo map dimensions into `[0..size_px)`)

## 2. Camera / Tilemap Struct Layout (`a0`)
| Offset | Type | Description |
|---|---|---|
| `+0x00` | `int16_t` | Camera World X coordinate (pixels) |
| `+0x02` | `uint16_t` | Camera Subpixel X (16.16 fractional) |
| `+0x04` | `int16_t` | Camera World Y coordinate (pixels) |
| `+0x06` | `uint16_t` | Camera Subpixel Y (16.16 fractional) |
| `+0x0E` | `uint16_t` | VDP Plane A horizontal scroll accumulator (0..511 ring buffer: `& 0x1FF`) |
| `+0x10` | `uint16_t` | VDP Plane A vertical scroll accumulator (0..255 ring buffer: `& 0xFF`) |
| `+0x12` | `int16_t` | Pixel Delta X for current frame (`new_x - old_x`) |
| `+0x14` | `int16_t` | Pixel Delta Y for current frame (`new_y - old_y`) |
| `+0x1C` | `int16_t` | Tile column shift count (`(new_x >> 3) - (old_x >> 3)`) |
| `+0x1E` | `int16_t` | Tile row shift count (`(new_y >> 3) - (old_y >> 3)`) |
| `+0x20` | `uint16_t` | Map width in 8x8 tiles |
| `+0x22` | `uint16_t` | Map height in 8x8 tiles |
| `+0x26` | `uint8_t` | Tilemap dirty status bitmask |
| `+0x27` | `uint8_t` | Control flags (bit 0 = enable integration; bit 1 = suppress tile strip shift) |

## 3. Dirty Status Bitmask (`+0x26`)
- `bit 0 (0x01)`: Horizontal tile column strip needs render / VRAM stream
- `bit 1 (0x02)`: Vertical tile row strip needs render / VRAM stream
- `bit 2 (0x04)`: VDP Plane A horizontal scroll register needs write
- `bit 3 (0x08)`: VDP Plane A vertical scroll register needs write

## 4. VDP Scroll Register Math
- **Horizontal Scroll (Plane A):**
  - Subtracts delta: `vdp_hscroll = (vdp_hscroll - delta_x) & 0x1FF` (512-pixel circular ring buffer).
- **Vertical Scroll (Plane A):**
  - Adds delta: `vdp_vscroll = (vdp_vscroll + delta_y) & 0xFF` (256-pixel circular ring buffer).

## 5. Toroidal Map Wrapping (`0x00D266`)
```cpp
void normalize_toroidal(int16_t& coord, int16_t size_px) {
    while (coord < 0) coord += size_px;
    while (coord >= size_px) coord -= size_px;
}
```

## 6. Cleanroom C++20 Interface
```cpp
namespace thor::render {

struct TilemapCameraState {
    int16_t cam_x{0};
    uint16_t cam_sub_x{0};
    int16_t cam_y{0};
    uint16_t cam_sub_y{0};

    uint16_t vdp_hscroll{0}; // 0..511
    uint16_t vdp_vscroll{0}; // 0..255

    int16_t delta_x{0};
    int16_t delta_y{0};
    int16_t tile_col_step{0};
    int16_t tile_row_step{0};

    uint16_t map_width_tiles{0};
    uint16_t map_height_tiles{0};

    uint8_t dirty_flags{0};
    uint8_t control_flags{1};
};

void accumulate_long_and_integrate(
    TilemapCameraState& state,
    int32_t delta_long_x,
    int32_t delta_long_y);

void accumulate_word_and_integrate(
    TilemapCameraState& state,
    int16_t delta_word_x,
    int16_t delta_word_y);

void integrate_scroll_deltas(
    TilemapCameraState& state,
    int16_t new_x,
    int16_t new_y);

void normalize_toroidal_coords(
    TilemapCameraState& state);

} // namespace thor::render
```
