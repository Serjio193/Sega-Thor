# Standalone Handoff: M15.408 ROM Horizontal (±X) Solid Dynamic Entity Collision Sweeps

## Summary
- **Source Address Range:** `0x009DD6..0x009F9E` (454 bytes total: +X sweep `0x009DD6..0x009EB8` [226B], -X sweep `0x009EBA..0x009F9E` [228B])
- **Target Area:** Dynamic Entity Physics & Collision (`entity_collision.cpp` / `dynamic_sweeps.hpp`)
- **Evidence File:** `data/thor-evidence/rom_horizontal_entity_sweeps_m15_408.json`
- **Unit Test File:** `tests/rom_horizontal_entity_sweeps_test.py`

## Routine Signatures & Memory Mappings

### 1. `0x009DD6`: Positive X (+X) Solid Dynamic Entity Sweep
- **Registers:**
  - `d0.w`: Proposed horizontal displacement step `dx` (positive).
  - `d1.w`: Current horizontal coordinate `x`.
  - `a6`: Moving entity pointer (`$42(a6)` = rad_x, `$44(a6)` = rad_y, `$4A(a6)` = height, `$10(a6)` = z air, `$14(a6)` = ground_h, `$37(a6)` = push flags, `$38(a6)` = ghost flags, `$40(a6)` = collision flags X, `$68(a6)` = hit entity ptr, `$6C(a6)` = reaction code).
  - `a5`: Pool pointer to Group 2 entities (`$FF19E8.l`, 21 slots, stride `$BC` = 188 bytes).
  - `d7.w`: Loop counter initialized to `0x14` (21 iterations).
- **M68K Logic:**
  ```m68k
  0x009DD6: btst.b #2, $38(a6)   ; if bit 2 set (ghost/intangible), return
  0x009DDC: bne.w $9eb8
  0x009DE0: add.w $42(a6), d1     ; d1 = x + rad_x
  0x009DE4: movea.w $10(a6), a2   ; a2 = z
  0x009DE8: adda.w $14(a6), a2   ; a2 = z + ground_h (bottom elevation)
  0x009DEC: move.w $44(a6), d3    ; d3 = rad_y
  0x009DF0: move.w $c(a6), d7     ; d7 = y
  0x009DF4: add.w $14(a6), d7    ; d7 = y + ground_h
  0x009DF8: move.w d7, d2
  0x009DFA: sub.w d3, d2         ; d2 = top Y bound
  0x009DFC: add.w d7, d3         ; d3 = bottom Y bound
  0x009DFE: lea.l $ff19e8.l, a5  ; Group 2 entity pool
  0x009E04: moveq #$14, d7        ; 21 slots

  ; Loop over Group 2 entities:
  0x009E06: tst.w $0(a5)          ; active entity?
  0x009E0A: ble.w $9eac
  0x009E0E: btst.b #2, $38(a5)   ; target intangible?
  0x009E14: bne.w $9eac
  0x009E18: btst.b #1, $3a(a5)
  0x009E1E: bne.w $9eac
  0x009E22: move.w $8(a5), d6     ; target X
  0x009E26: cmp.w $8(a6), d6      ; target X > self X?
  0x009E2A: ble.w $9eac
  0x009E2E: sub.w $42(a5), d6    ; d6 = target left edge
  0x009E32: sub.w d1, d6         ; d6 = remaining distance to target
  0x009E34: bpl.w $9e3a
  0x009E38: clr.w d6             ; already touching/overlapping
  0x009E3A: cmp.w d0, d6         ; dx > distance?
  0x009E3C: bge.w $9eac

  ; Y axis overlap:
  0x009E40: movea.w $44(a5), a1  ; target rad_y
  0x009E44: move.w $c(a5), d4
  0x009E48: add.w $14(a5), d4
  0x009E4C: move.w d4, d5
  0x009E4E: sub.w a1, d4        ; target top Y
  0x009E50: add.w a1, d5        ; target bot Y
  0x009E52: cmp.w d2, d5        ; target bot <= self top?
  0x009E54: ble.w $9eac
  0x009E58: cmp.w d3, d4        ; target top >= self bot?
  0x009E5A: bge.w $9eac

  ; Z (elevation) axis overlap:
  0x009E5E: move.w $10(a5), d4   ; target z
  0x009E62: add.w $14(a5), d4   ; target elevation
  0x009E66: move.w d4, d5
  0x009E68: add.w $4a(a5), d5   ; target top elevation
  0x009E6C: cmp.w a2, d5        ; target top <= self bot elevation?
  0x009E6E: ble.w $9eac
  0x009E72: movea.l a2, a1
  0x009E74: adda.w $4a(a6), a1  ; self top elevation
  0x009E78: cmp.w a1, d4        ; target bot >= self top elevation?
  0x009E7A: bge.w $9eac

  ; Collision response:
  0x009E7E: bset.b #7, $40(a6)  ; set bit 7 on +0x40(a6)
  0x009E84: move.l a5, $68(a6)   ; save target pointer in +0x68(a6)
  0x009E88: move.b #3, $6c(a6)  ; reaction code 3 in +0x6C(a6)
  0x009E8E: move.w d6, d0        ; clamp step dx = distance
  0x009E90: bne.w $9eac         ; if dist != 0, continue loop
  0x009E94: btst.b #5, $37(a6)  ; can self push?
  0x009E9A: bne.w $9eac
  0x009E9E: btst.b #6, $37(a5)  ; can target be pushed?
  0x009EA4: bne.w $9eac
  0x009EA8: addq.w #1, $72(a5)  ; push target right: +0x72(a5) += 1

  0x009EAC: lea.l $bc(a5), a5
  0x009EB0: dbra d7, $9e06
  0x009EB4: sub.w $42(a6), d1
  0x009EB8: rts
  ```

### 2. `0x009EBA`: Negative X (-X) Solid Dynamic Entity Sweep
- Mirrors `0x009DD6` with opposite direction:
  - Tests `target.x < self.x`.
  - Computes `dist = (self.x - rad_x) - (target.x + rad_x)`.
  - Clamps `dx = dist`, sets bit 7 on `$40(a6)`, saves target pointer `$68(a6)` and code 3 on `$6C(a6)`.
  - When `dist == 0`: pushes target left: `subq.w #1, $72(a5)`.

## C++20 Cleanroom Interface Guidance
```cpp
namespace oasis::collision {

struct DynamicEntityHeader {
    int16_t type{0};
    uint8_t flags_37{0};
    uint8_t flags_38{0};
    uint8_t flags_3a{0};
    uint8_t collision_flags_x{0};
    uint8_t collision_flags_y{0};
    uint8_t reaction_code{0};
    int16_t x{0};
    int16_t y{0};
    int16_t z{0};
    int16_t ground_h{0};
    int16_t rad_x{0};
    int16_t rad_y{0};
    int16_t height{0};
    int16_t vel_x{0};
    int16_t vel_y{0};
    void* collided_entity{nullptr};
};

inline uint16_t sweep_entities_pos_x(uint16_t dx, DynamicEntityHeader& self,
                                     DynamicEntityHeader* pool, size_t pool_size) noexcept {
    if (self.flags_38 & 0x04) {
        return dx;
    }

    const int16_t self_right = self.x + self.rad_x;
    const int16_t self_elev = self.z + self.ground_h;
    const int16_t self_y_base = self.y + self.ground_h;
    const int16_t self_top_y = self_y_base - self.rad_y;
    const int16_t self_bot_y = self_y_base + self.rad_y;
    const int16_t self_top_z = self_elev + self.height;

    for (size_t i = 0; i < pool_size; ++i) {
        auto& target = pool[i];
        if (target.type <= 0 || (target.flags_38 & 0x04) || (target.flags_3a & 0x02)) {
            continue;
        }
        if (target.x <= self.x) {
            continue;
        }

        int16_t dist = (target.x - target.rad_x) - self_right;
        if (dist < 0) dist = 0;
        if (static_cast<uint16_t>(dist) >= dx) {
            continue;
        }

        const int16_t tgt_y_base = target.y + target.ground_h;
        const int16_t tgt_top_y = tgt_y_base - target.rad_y;
        const int16_t tgt_bot_y = tgt_y_base + target.rad_y;
        if (tgt_bot_y <= self_top_y || tgt_top_y >= self_bot_y) {
            continue;
        }

        const int16_t tgt_elev = target.z + target.ground_h;
        const int16_t tgt_top_z = tgt_elev + target.height;
        if (tgt_top_z <= self_elev || tgt_elev >= self_top_z) {
            continue;
        }

        self.collision_flags_x |= 0x80;
        self.collided_entity = &target;
        self.reaction_code = 3;
        dx = static_cast<uint16_t>(dist);
        if (dx == 0) {
            if (!(self.flags_37 & 0x20) && !(target.flags_37 & 0x40)) {
                target.vel_x += 1;
            }
        }
    }
    return dx;
}

} // namespace oasis::collision
```
