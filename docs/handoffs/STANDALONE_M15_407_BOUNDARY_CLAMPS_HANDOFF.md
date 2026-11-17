# Standalone Handoff: M15.407 ROM 4-Direction World Boundary Clamp Subsystem

## Summary
- **Source Address Range:** `0x009D4E..0x009DD6` (136 bytes total across 4 direction handlers: 34 bytes each)
- **Target Area:** World Tilemap & Entity Boundary Clamping (`world_collision.cpp` / `boundary_clamps.hpp`)
- **Evidence File:** `data/thor-evidence/rom_boundary_clamps_m15_407.json`
- **Unit Test File:** `tests/rom_boundary_clamps_test.py`

## Routine Signatures & Memory Mappings

### 1. `0x009D4E`: Right (+X) Boundary Clamp
- **Registers:**
  - `d0.w`: Proposed horizontal displacement step `dx` (positive).
  - `d1.w`: Current horizontal coordinate `x`.
  - `a6`: Entity state pointer (`$46(a6)` = radius, `$40(a6)` = collision flags byte X).
  - `a0`: Map header pointer (`$4(a0)` = width in 8-pixel tiles).
- **M68K Logic:**
  ```m68k
  0x009D4E: move.w d1, d7       ; d7 = x
  0x009D50: add.w $46(a6), d7   ; d7 = x + rad_x
  0x009D54: move.w $4(a0), d3   ; d3 = width_in_tiles
  0x009D58: lsl.w #$3, d3       ; d3 = width_in_tiles * 8
  0x009D5A: subq.w #$1, d3      ; d3 = max_x = width_in_tiles * 8 - 1
  0x009D5C: sub.w d7, d3        ; d3 = remaining distance = max_x - (x + rad_x)
  0x009D5E: cmp.w d3, d0        ; cmp.w remaining, dx
  0x009D60: bls.w $9d6e         ; if dx <= remaining, fits -> return dx
  0x009D64: move.w d3, d0       ; dx = remaining
  0x009D66: bne.b $9d6e         ; if dx != 0 -> return dx
  0x009D68: ori.b #$48, $40(a6) ; set flag 0x48 on entity collision flags X
  0x009D6E: rts
  ```

### 2. `0x009D70`: Left (-X) Boundary Clamp
- **Registers:**
  - `d0.w`: Proposed negative step magnitude `dx` (positive value).
  - `d1.w`: Current horizontal coordinate `x`.
  - `a6`: Entity state pointer (`$46(a6)` = radius, `$40(a6)` = collision flags byte X).
- **M68K Logic:**
  ```m68k
  0x009D70: move.w d1, d3       ; d3 = x
  0x009D72: sub.w $46(a6), d3   ; d3 = x - rad_x
  0x009D76: bmi.w $9d88         ; if (x - rad_x) < 0 -> clamp to 0, set flag 0x48
  0x009D7A: cmp.w d3, d0        ; cmp.w remaining, dx
  0x009D7C: bls.w $9d86         ; if dx <= remaining -> return dx
  0x009D80: move.w d3, d0       ; dx = remaining
  0x009D82: beq.w $9d8a         ; if dx == 0 -> set flag 0x48
  0x009D86: rts
  0x009D88: clr.w d0            ; dx = 0
  0x009D8A: ori.b #$48, $40(a6) ; set flag 0x48 on entity collision flags X
  0x009D90: rts
  ```

### 3. `0x009D92`: Bottom (+Y) Boundary Clamp
- **Registers:**
  - `d0.w`: Proposed vertical displacement step `dy` (positive).
  - `d2.w`: Current vertical coordinate `y`.
  - `a6`: Entity state pointer (`$46(a6)` = radius, `$41(a6)` = collision flags byte Y).
  - `a0`: Map header pointer (`$6(a0)` = height in 8-pixel tiles).
- **M68K Logic:**
  ```m68k
  0x009D92: move.w d2, d7       ; d7 = y
  0x009D94: add.w $46(a6), d7   ; d7 = y + rad_y
  0x009D98: move.w $6(a0), d3   ; d3 = height_in_tiles
  0x009D9C: lsl.w #$3, d3       ; d3 = height_in_tiles * 8
  0x009D9E: subq.w #$1, d3      ; d3 = max_y = height_in_tiles * 8 - 1
  0x009DA0: sub.w d7, d3        ; d3 = remaining distance = max_y - (y + rad_y)
  0x009DA2: cmp.w d3, d0        ; cmp.w remaining, dy
  0x009DA4: bls.w $9db2         ; if dy <= remaining -> return dy
  0x009DA8: move.w d3, d0       ; dy = remaining
  0x009DAA: bne.b $9db2         ; if dy != 0 -> return dy
  0x009DAC: ori.b #$48, $41(a6) ; set flag 0x48 on entity collision flags Y
  0x009DB2: rts
  ```

### 4. `0x009DB4`: Top (-Y) Boundary Clamp
- **Registers:**
  - `d0.w`: Proposed negative step magnitude `dy` (positive value).
  - `d2.w`: Current vertical coordinate `y`.
  - `a6`: Entity state pointer (`$46(a6)` = radius, `$41(a6)` = collision flags byte Y).
- **M68K Logic:**
  ```m68k
  0x009DB4: move.w d2, d3       ; d3 = y
  0x009DB6: sub.w $46(a6), d3   ; d3 = y - rad_y
  0x009DBA: bmi.w $9dcc         ; if (y - rad_y) < 0 -> clamp to 0, set flag 0x48
  0x009DBE: cmp.w d3, d0        ; cmp.w remaining, dy
  0x009DC0: bls.w $9dca         ; if dy <= remaining -> return dy
  0x009DC4: move.w d3, d0       ; dy = remaining
  0x009DC6: beq.w $9dce         ; if dy == 0 -> set flag 0x48
  0x009DCA: rts
  0x009DCC: clr.w d0            ; dy = 0
  0x009DCE: ori.b #$48, $41(a6) ; set flag 0x48 on entity collision flags Y
  0x009DD4: rts
  ```

## C++20 Cleanroom Interface Guidance
```cpp
namespace oasis::collision {

struct WorldBounds {
    uint16_t width_tiles{0};
    uint16_t height_tiles{0};

    [[nodiscard]] constexpr uint16_t max_x() const noexcept {
        return static_cast<uint16_t>((width_tiles << 3) - 1);
    }
    [[nodiscard]] constexpr uint16_t max_y() const noexcept {
        return static_cast<uint16_t>((height_tiles << 3) - 1);
    }
};

inline uint16_t clamp_step_pos_x(uint16_t dx, uint16_t x, uint16_t radius,
                                 uint16_t width_tiles, uint8_t& flags_x) noexcept {
    const uint16_t right_edge = static_cast<uint16_t>(x + radius);
    const uint16_t max_x = static_cast<uint16_t>((width_tiles << 3) - 1);
    const uint16_t remaining = static_cast<uint16_t>(max_x - right_edge);
    if (dx <= remaining) {
        return dx;
    }
    dx = remaining;
    if (dx == 0) {
        flags_x |= 0x48;
    }
    return dx;
}

inline uint16_t clamp_step_neg_x(uint16_t dx, uint16_t x, uint16_t radius,
                                 uint8_t& flags_x) noexcept {
    const int32_t remaining = static_cast<int32_t>(x) - static_cast<int32_t>(radius);
    if (remaining < 0) {
        flags_x |= 0x48;
        return 0;
    }
    const auto rem_u16 = static_cast<uint16_t>(remaining);
    if (dx <= rem_u16) {
        return dx;
    }
    dx = rem_u16;
    if (dx == 0) {
        flags_x |= 0x48;
    }
    return dx;
}

inline uint16_t clamp_step_pos_y(uint16_t dy, uint16_t y, uint16_t radius,
                                 uint16_t height_tiles, uint8_t& flags_y) noexcept {
    const uint16_t bottom_edge = static_cast<uint16_t>(y + radius);
    const uint16_t max_y = static_cast<uint16_t>((height_tiles << 3) - 1);
    const uint16_t remaining = static_cast<uint16_t>(max_y - bottom_edge);
    if (dy <= remaining) {
        return dy;
    }
    dy = remaining;
    if (dy == 0) {
        flags_y |= 0x48;
    }
    return dy;
}

inline uint16_t clamp_step_neg_y(uint16_t dy, uint16_t y, uint16_t radius,
                                 uint8_t& flags_y) noexcept {
    const int32_t remaining = static_cast<int32_t>(y) - static_cast<int32_t>(radius);
    if (remaining < 0) {
        flags_y |= 0x48;
        return 0;
    }
    const auto rem_u16 = static_cast<uint16_t>(remaining);
    if (dy <= rem_u16) {
        return dy;
    }
    dy = rem_u16;
    if (dy == 0) {
        flags_y |= 0x48;
    }
    return dy;
}

} // namespace oasis::collision
```
