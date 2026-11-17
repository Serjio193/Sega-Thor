# Standalone Handoff: M15.411 ROM Dialogue & HUD Sprite SAT Compositor and Allocation Subsystem

## Summary
- **Source Address Range:** `0x00A342..0x00A524` (482 bytes total)
- **Target Area:** UI, Dialogue Frame & Portrait Sprite Pipeline (`dialogue_sprites.cpp` / `ui_sprites.hpp`)
- **Evidence File:** `data/thor-evidence/rom_dialogue_sprite_subsystem_m15_411.json`
- **Unit Test File:** `tests/rom_dialogue_sprite_subsystem_test.py`

## Routine Signatures & Memory Mappings

### 1. `0x00A342`: Dialogue / HUD Sprite Layer Compositor
- **RAM Inputs / State:**
  - `a5`: SAT buffer pointer (`$FF13CC + $FF188C.w`).
  - `d5.w`: Current hardware sprite link count (`$FF188A.w`).
  - `$FF1858.b`: Dialogue style selector (0 = primary template `0x00A438`, non-zero = secondary template `0x00A480`).
  - `$FF185A.w`: Dialogue horizontal positioning offset.
  - `$FF184F.b`: Portrait active flag (if non-zero, emits 3 additional sprites positioned at `0x158 - $FF185A`).
  - `$FF1856.w`: Cursor active flag (if non-zero and sprite count < 80 [`0x50`], emits cursor sprite at `0x118, $FF185A + 0x68` with tile attribute `#$85C7`).
- **M68K Logic:**
  ```m68k
  0x00A342: lea.l $ff13cc.l, a5
  0x00A348: adda.w $ff188c.l, a5
  0x00A34E: move.w $ff188a.l, d5
  0x00A354: lea.l $a438(pc), a0
  0x00A358: tst.b $ff1858.l
  0x00A35E: beq.b $a364
  0x00A360: lea.l $a480(pc), a0
  0x00A364: move.w $ff185a.l, d3
  0x00A36A: moveq #5, d0
  ; 6-sprite dialogue border loop
  0x00A36C: move.l (a0)+, d2
  0x00A36E: addq.b #1, d5
  0x00A370: move.b d5, d2
  0x00A372: move.l d2, (a5)+
  0x00A374: move.w (a0)+, (a5)+
  0x00A376: move.w (a0)+, d2
  0x00A378: add.w d3, d2
  0x00A37A: move.w d2, (a5)+
  0x00A37C: dbra d0, $a36c
  ```

### 2. Cursor Animation & DMA Queue (`0x00A3DA..0x00A422`)
- If cursor active frame changed (`$FF1854.b != $FF1855.b`):
  - Updates `$FF1855.b = $FF1854.b`.
  - Calculates ROM source address: `rom_addr = frame_id * 144 + 0x000AAEEB`.
  - Enqueues DMA transfer to VRAM `$B8E0` (size `0x90` words) via queue pointer `$FF1892.l`.
  - Protected with interrupt disable: `ori.w #$700, sr` before queue push, `andi.w #$f9ff, sr` after push.

### 3. Static Sprite Reservation Pass (`0x00A4C8..0x00A524`)
- Reserves SAT entries before dynamic entity allocation:
  - Base dialogue frame: `+$30` bytes (48B), `+6` sprites.
  - Speaker portrait (if `$FF184F.b != 0`): `+$18` bytes (24B), `+3` sprites.
  - Flashing cursor (if `$FF1856.w != 0`): `+$08` bytes (8B), `+1` sprite.

## C++20 Cleanroom Interface Guidance
```cpp
namespace oasis::ui {

struct DialogueSpriteConfig {
    bool has_portrait{false};
    bool has_cursor{false};
    uint8_t style{0};
    int16_t offset_x{0};
    uint8_t cursor_frame{0};
};

struct SatReservation {
    uint16_t byte_count{48};
    uint16_t sprite_count{6};
};

[[nodiscard]] constexpr SatReservation calculate_reservation(bool has_portrait, bool has_cursor) noexcept {
    SatReservation res{};
    if (has_portrait) {
        res.byte_count += 24;
        res.sprite_count += 3;
    }
    if (has_cursor) {
        res.byte_count += 8;
        res.sprite_count += 1;
    }
    return res;
}

[[nodiscard]] constexpr uint32_t cursor_rom_address(uint8_t frame_id) noexcept {
    return static_cast<uint32_t>(frame_id) * 144u + 0x000AAEEBu;
}

} // namespace oasis::ui
```
