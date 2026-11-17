# Standalone Handoff: M15.409 ROM Vertical (±Y) Solid Dynamic Entity Collision Sweeps

## Summary
- **Source Address Range:** `0x009FA0..0x00A196` (500 bytes total: +Y sweep `0x009FA0..0x00A098` [248B], -Y sweep `0x00A09A..0x00A196` [252B])
- **Target Area:** Dynamic Entity Physics & Collision (`entity_collision.cpp` / `dynamic_sweeps.hpp`)
- **Evidence File:** `data/thor-evidence/rom_vertical_entity_sweeps_m15_409.json`
- **Unit Test File:** `tests/rom_vertical_entity_sweeps_test.py`

## Routine Signatures & Memory Mappings

### 1. `0x009FA0`: Positive Y (+Y) Solid Dynamic Entity Sweep
- **Registers:**
  - `d0.w`: Proposed vertical displacement step `dy` (positive).
  - `d2.w`: Current vertical coordinate `y`.
  - `a6`: Moving entity pointer (`$42(a6)` = rad_x, `$44(a6)` = rad_y, `$4A(a6)` = height, `$10(a6)` = z air, `$14(a6)` = ground_h, `$37(a6)` = push flags, `$38(a6)` = ghost flags, `$41(a6)` = collision flags Y, `$68(a6)` = hit entity ptr, `$6C(a6)` = reaction code).
  - `a5`: Pool pointer to Group 2 entities (`$FF19E8.l`, 21 slots, stride `$BC` = 188 bytes).
  - `d7.w`: Loop counter initialized to `0x14` (21 iterations). Bit 31 marks self-ghost state.
- **M68K Logic:**
  ```m68k
  0x009FA0: add.w $14(a6), d2    ; d2 = y + ground_h
  0x009FA4: add.w $44(a6), d2    ; d2 = y + ground_h + rad_y (bottom edge in Y)
  0x009FA8: movea.w $10(a6), a2  ; a2 = z air
  0x009FAC: adda.w $14(a6), a2  ; a2 = z + ground_h (bottom elevation)
  0x009FB0: move.w $42(a6), d3   ; d3 = rad_x
  0x009FB4: move.w $8(a6), d7    ; d7 = x
  0x009FB8: move.w d7, d1        ; d1 = x
  0x009FBA: sub.w d3, d1        ; d1 = x - rad_x (left X bound)
  0x009FBC: add.w d7, d3        ; d3 = x + rad_x (right X bound)
  0x009FBE: lea.l $ff19e8.l, a5 ; Group 2 entity pool ($FF19E8)
  0x009FC4: moveq #$14, d7       ; loop counter = 20 (21 iterations)
  0x009FC6: btst.b #2, $38(a6)  ; check self ghost flag
  0x009FCC: beq.w $9fd4
  0x009FD0: bset.b #$1f, d7     ; bit 31 of d7 flags self as ghost
  0x009FD4: tst.w $0(a5)         ; active entity?
  0x009FD8: ble.w $a088
  0x009FDC: btst.b #1, $3a(a5)  ; target ignores ghost?
  0x009FE2: bne.w $9ff8
  0x009FE6: btst.b #$1f, d7     ; if self is ghost -> skip
  0x009FEA: bne.w $a088
  0x009FEE: btst.b #2, $38(a5)  ; if target is ghost -> skip
  0x009FF4: bne.w $a088

  0x009FF8: move.w $c(a6), d4
  0x009FFC: add.w $14(a6), d4   ; self Y base
  0x00A000: move.w $c(a5), d6
  0x00A004: add.w $14(a5), d6   ; target Y base
  0x00A008: cmp.w d4, d6        ; target Y > self Y?
  0x00A00A: ble.w $a088
  0x00A00E: sub.w $44(a5), d6   ; target top edge (Y - rad_y)
  0x00A012: sub.w d2, d6        ; distance = target top - self bottom
  0x00A014: bpl.w $a01a
  0x00A018: clr.w d6
  0x00A01A: cmp.w d0, d6        ; dy > distance?
  0x00A01C: bge.w $a088

  ; X axis overlap:
  0x00A020: movea.w $42(a5), a1 ; target rad_x
  0x00A024: move.w $8(a5), d4
  0x00A028: move.w d4, d5
  0x00A02A: sub.w a1, d4        ; target left X
  0x00A02C: add.w a1, d5        ; target right X
  0x00A02E: cmp.w d1, d5        ; target right <= self left?
  0x00A030: ble.w $a088
  0x00A034: cmp.w d3, d4        ; target left >= self right?
  0x00A036: bge.w $a088

  ; Z (elevation) axis overlap:
  0x00A03A: move.w $10(a5), d4  ; target z
  0x00A03E: add.w $14(a5), d4  ; target elevation
  0x00A042: move.w d4, d5
  0x00A044: add.w $4a(a5), d5  ; target top elevation
  0x00A048: cmp.w a2, d5        ; target top <= self bot elevation?
  0x00A04A: ble.w $a088
  0x00A04E: movea.l a2, a1
  0x00A050: adda.w $4a(a6), a1 ; self top elevation
  0x00A054: cmp.w a1, d4        ; target bot >= self top elevation?
  0x00A056: bge.w $a088

  ; Collision response:
  0x00A05A: bset.b #7, $41(a6)  ; set bit 7 on +0x41(a6)
  0x00A060: move.l a5, $68(a6)  ; save target pointer in +0x68(a6)
  0x00A064: move.b #3, $6c(a6)  ; reaction code 3 in +0x6C(a6)
  0x00A06A: move.w d6, d0       ; clamp step dy = distance
  0x00A06C: bne.w $a088         ; if dist != 0, continue loop
  0x00A070: btst.b #5, $37(a6)  ; can self push?
  0x00A076: bne.w $a088
  0x00A07A: btst.b #6, $37(a5)  ; can target be pushed?
  0x00A080: bne.w $a088
  0x00A084: addq.w #1, $76(a5)  ; push target down: +0x76(a5) += 1

  0x00A088: lea.l $bc(a5), a5
  0x00A08C: dbra d7, $9fd4
  0x00A090: sub.w $44(a6), d2
  0x00A094: sub.w $14(a6), d2
  0x00A098: rts
  ```

### 2. `0x00A09A`: Negative Y (-Y) Solid Dynamic Entity Sweep
- Mirrors `0x009FA0` for upward movement:
  - Top edge in Y: `d2 = y + ground_h - rad_y`.
  - Tests `target.y + ground_h < self.y + ground_h`.
  - Computes `dist = (self.y + ground_h - rad_y) - (target.y + ground_h + target.rad_y)`.
  - Clamps `dy = dist`, sets bit 7 on `$41(a6)`, saves target pointer `$68(a6)` and code 3 on `$6C(a6)`.
  - When `dist == 0`: pushes target up: `subq.w #1, $76(a5)`.
  - Restores `d2`: `sub.w $14(a6), d2; add.w $44(a6), d2; rts`.

## C++20 Cleanroom Interface Guidance
```cpp
namespace oasis::collision {

inline uint16_t sweep_entities_pos_y(uint16_t dy, DynamicEntityHeader& self,
                                     DynamicEntityHeader* pool, size_t pool_size) noexcept {
    const int16_t self_bot_y = self.y + self.ground_h + self.rad_y;
    const int16_t self_elev = self.z + self.ground_h;
    const int16_t self_left_x = self.x - self.rad_x;
    const int16_t self_right_x = self.x + self.rad_x;
    const int16_t self_top_z = self_elev + self.height;
    const bool self_is_ghost = (self.flags_38 & 0x04) != 0;

    for (size_t i = 0; i < pool_size; ++i) {
        auto& target = pool[i];
        if (target.type <= 0) continue;
        if (!(target.flags_3a & 0x02)) {
            if (self_is_ghost || (target.flags_38 & 0x04)) continue;
        }

        const int16_t tgt_y_base = target.y + target.ground_h;
        const int16_t self_y_base = self.y + self.ground_h;
        if (tgt_y_base <= self_y_base) continue;

        int16_t dist = (tgt_y_base - target.rad_y) - self_bot_y;
        if (dist < 0) dist = 0;
        if (static_cast<uint16_t>(dist) >= dy) continue;

        const int16_t tgt_left_x = target.x - target.rad_x;
        const int16_t tgt_right_x = target.x + target.rad_x;
        if (tgt_right_x <= self_left_x || tgt_left_x >= self_right_x) continue;

        const int16_t tgt_elev = target.z + target.ground_h;
        const int16_t tgt_top_z = tgt_elev + target.height;
        if (tgt_top_z <= self_elev || tgt_elev >= self_top_z) continue;

        self.collision_flags_y |= 0x80;
        self.collided_entity = &target;
        self.reaction_code = 3;
        dy = static_cast<uint16_t>(dist);
        if (dy == 0) {
            if (!(self.flags_37 & 0x20) && !(target.flags_37 & 0x40)) {
                target.vel_y += 1;
            }
        }
    }
    return dy;
}

} // namespace oasis::collision
```
