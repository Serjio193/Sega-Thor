# STANDALONE HANDOFF: M15.439 — ROM Scene Script Header Unpacker, Dual-Plane Camera Initializer & 16KB VRAM DMA Upload Engine

## 1. Routine Overview
- **Addresses:** `0x00D406..0x00D542` (316 bytes)
- **Primary Entrypoints:**
  - `0x00D406`: `load_scene_header_and_init_vram` (Unpacks scene script header fields, builds 8-word elevation band table, resets Plane A and B camera structs, aligns initial coordinates, decompresses 4 background pattern tile blocks, and queues 16KB master VRAM DMA upload)
  - `0x00D44A`: `unpack_elevation_bands` (Extracts 4 threshold bytes, sign-extends, shifts `<< 3` into `$FF16FE`, and calculates 8 height band boundaries into `$FF1706` with terminal sentinel `#$F000`)
  - `0x00D46C`: `clear_camera_structs` (Zeroes 20 words for Plane A `$FF1716` and 20 words for Plane B `$FF173E`)
  - `0x00D484`: `init_aligned_camera_coords` (Aligns initial packed coordinates `d1` and `d2` to 8-pixel tile boundaries `& 0xFFF8FFF8`)
  - `0x00D4C8`: `decompress_4_bg_patterns` (Decompresses 4 tile pattern blocks from table `0x05CE96` into `$FF3FA8` with 4KB stride)
  - `0x00D50E`: `queue_16kb_vram_dma` (Enqueues 8192-word / 16KB DMA transfer from `$FF3FA8 >> 1` to VRAM `$0000`, awaiting V-Int completion)

## 2. Script Header Data Offsets (`a1`)
| Offset | Field | Target / Use |
|---|---|---|
| `+0x04` | `uint32_t` | Pointer to compressed plane tilemaps |
| `+0x08` | `uint8_t[4]` | 4 background pattern tile block indices (stored at `$FF16FA`) |
| `+0x0C` | `uint8_t[4]` | 4 elevation slice threshold bytes |
| `+0x10` | `uint16_t` | BGM Music Track ID (stored at `$FF16F4`) |
| `+0x12` | `uint16_t` | BGM Tempo parameter (stored at `$FF16F6`) |
| `+0x14` | `uint16_t` | BGM Sound mode / subparameter (stored at `$FF16F8`) |
| `+0x16` | `uint16_t` | Scroll flags (loaded into register `d6`) |
| `+0x18` | `uint16_t` | VDP Scroll mode word (stored at `$FF16F0`) |

## 3. Elevation Band Math (`0x00D44A..0x00D466`)
```cpp
int16_t cum_elev = 0;
int band_idx = 0;
for (int i = 0; i < 4; ++i) {
    int16_t threshold = static_cast<int8_t>(header_bytes[0x0C + i]) << 3;
    unpacked_thresholds[i] = threshold;
    height_bands[band_idx++] = cum_elev;
    height_bands[band_idx++] = (threshold >> 1) + cum_elev;
    cum_elev += threshold;
}
height_bands[7] = 0xF000; // Sentinel value marking top of world
```

## 4. 16KB VRAM DMA Upload
- **Staging Buffer:** `$FF3FA8` (Mega Drive word bus address: `0x007F9FD4`).
- **Destination:** `#$0000` (Base pattern tile generator table in VRAM).
- **Length:** `0x2000` words (8192 words = 16384 bytes = 512 tiles of 32 bytes each).
- **V-Int Handshake:** sets bit 1 on `$FF164E` and spin-waits for clearance.

## 5. Cleanroom C++20 Interface
```cpp
namespace thor::render {

struct SceneHeaderConfig {
    uint32_t plane_tilemap_ptr{0};
    std::array<uint8_t, 4> bg_patterns{};
    std::array<int8_t, 4> elevation_slices{};
    uint16_t bgm_track{0};
    uint16_t bgm_tempo{0};
    uint16_t bgm_mode{0};
    uint16_t scroll_flags{0};
    uint16_t vdp_scroll_mode{0};
};

struct SceneHeaderState {
    std::array<int16_t, 4> unpacked_elevation{};
    std::array<int16_t, 8> height_bands{};
    int16_t cam_a_x{0};
    int16_t cam_a_y{0};
    int16_t cam_b_x{0};
    int16_t cam_b_y{0};
};

SceneHeaderState unpack_scene_header(
    const SceneHeaderConfig& config,
    uint32_t initial_coord_a,
    uint32_t initial_coord_b);

} // namespace thor::render
```
