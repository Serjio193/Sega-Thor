# STANDALONE HANDOFF: M15.430 — ROM Dialogue Entity Sprite Compositor, Prompt Arrow Indicator & Frame Sync Engine

## 1. Routine Overview
- **Addresses:** `0x00C756..0x00C88C` (310 bytes)
- **Primary Entrypoints:**
  - `0x00C756`: `dialogue_task_sprite_compositor` (Dialogue entity task callback registered at `+0x22(a6) = $00C756`)
  - `0x00C81C`: `dialogue_emit_line_sprites` (Emits 8 sprite pieces of 4x4 tiles across an 8-tile text row)
  - `0x00C838`: `dialogue_emit_sat_piece` (Low-level SAT buffer piece emitter with 80-sprite overflow clamp)
  - `0x00C84E`: `dialogue_frame_sync_and_render` (Synchronizes with V-Int, updates palette cycling, triggers render pass)

## 2. Prompt Arrow Sprite Rendering (`0x00C76C..0x00C7BC`)
- Condition: Emitted only if prompt wait is active (`$FF1886.b != 0`) AND animation timer bit 4 is set (`$FF1657 & 0x10 != 0`).
- Tile selection:
  - Base continue arrow tile: `0x86DC`.
  - Dismiss mode (`$FF1887.b != 0`): bit 12 set -> `0x96DC`.
- Spatial coordinates:
  - Horizontal coordinate: Fixed at X = 284 (`0x011C`).
  - Vertical coordinate: Base Y = 332 (`0x014C`). Dynamically adjusted by active line counter `$FF1880`:
    - Line >= 3: Y = 332
    - Line 2: Y = 320 (`332 - 12`)
    - Line 1: Y = 308 (`332 - 24`)
    - Line 0: Y = 296 (`332 - 36`)
- Sprite size: `0x0000` (1x1 tile).

## 3. Window Text Line Sprite Grid (`0x00C7C0..0x00C836`)
- 4 lines rendered using row offsets from `$FF1876..$FF187C`:
  - Lines 0 and 1: Base tile = `((plane_top_base >> 5) | 0x8000)`.
  - Lines 2 and 3: Base tile = `((plane_bottom_base >> 5) | 0x8000)`.
- Each line emits 8 sprite pieces:
  - Size code: `0x0C00` (4x4 tiles = 32x32 pixels).
  - Horizontal start: X = 160 (`0x00A0`), stepping +32 pixels per piece (`160, 192, 224, 256, 288, 320, 352, 384`).
  - Vertical position: `row_offset + 272` (`0x0110`).
  - Tile index: Starts at base tile, stepping +4 tiles per piece.
- Total window pieces: 32 hardware sprite pieces.

## 4. Hardware Sprite Limit (`0x00C838`)
- Checks link counter: `if (link >= 80) return;` (enforces Sega Genesis VDP 80-sprite limit).
- Increments link counter and appends 4 words to SAT buffer:
  - `(a5)+ = Y`
  - `(a5)+ = link + size_code`
  - `(a5)+ = tile_attr`
  - `(a5)+ = X`

## 5. Cleanroom C++20 Interface
```cpp
namespace thor::dialogue {

struct SatPiece {
    int16_t y{0};
    uint16_t link_size{0};
    uint16_t tile{0};
    int16_t x{0};
};

struct CompositorConfig {
    uint16_t plane_top_base{0};
    uint16_t plane_bottom_base{0};
    std::array<int16_t, 4> row_offsets{12, 24, 36, 48};
    int16_t line_counter{0};
    bool prompt_active{false};
    bool prompt_dismiss{false};
    bool anim_timer_visible{true};
};

std::vector<SatPiece> composite_dialogue_sprites(const CompositorConfig& config, uint16_t& io_link);

} // namespace thor::dialogue
```
