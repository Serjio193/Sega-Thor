# STANDALONE HANDOFF: ROM Universal Attack Frame Scanner & Hitbox Intersection Engine (0x00BC8E..0x00BE98)

## 1. Routine Overview
- **ROM Range:** `0x00BC8E..0x00BE98` (522 bytes).
- **Function:** Universal Attack Frame Scanner & 3D Hitbox Intersection Engine. Scans active animation frames for attack hitboxes, calculates directional 3D AABBs with facing inversion, computes Ali's strength rank bonus, packs damage and hitstun parameters, iterates over target entity pools, executes 3D overlap tests, and dispatches hit callbacks.
- **Evidence JSON:** `data/thor-evidence/rom_attack_scanner_engine_m15_422.json`.

## 2. Ingress Entry Points
1. `0x00BC8E` (`scan_all_entities`):
   - Sets callback `0x00BEBA` into `$FF1940.l`.
   - Targets Group 2 entities (`$FF19E8`, 21 slots of 188B).
2. `0x00BCA4` (`scan_group3_a`):
   - Sets callback `0x00BE98` into `$FF1940.l`.
   - Targets Group 3 entities (`$FF1CD8`, 17 slots of 188B).
3. `0x00BCBA` (`scan_group3_b`):
   - Sets callback `0x00BE98` into `$FF1940.l`.
   - Writes `#2` into `$FF198B.l`.
   - Targets Group 3 entities (`$FF1CD8`, 17 slots of 188B).
4. `0x00BCD8` (`scan_single`):
   - Sets callback `0x00BEBA` into `$FF1940.l`.
   - Targets slot 0 (`$FF19E8`, 1 slot of 188B; single target / player check).

## 3. Algorithm & Logic
1. **Early Phase Suppression:** Checks bit 3 of `$37(a6)`. If set, entity already registered a hit this attack phase; returns immediately.
2. **Animation Frame Search:** Reads descriptor table from `$22(a6) + d0 + *(int16_t*)($22(a6) + d0)`. Searches 44-byte (`0x2C`) records matching animation frame `$32(a6)`. Terminated by negative word (`0xFFFF`).
3. **Facing Direction & Mirroring:**
   - Facing: 0=Down, 1=Left/Raw, 2=Up, 3=Right/Flipped.
   - Facing 3 sets flip flag `d5 = 0xFF`, remaps `d0 = 1`.
   - Directional hitbox offset: `d0 * 12`.
   - If flip flag set: `x_min = -x_max`, `x_max = -x_min`.
4. **Ali Strength Rank Bonus:**
   - If attacker is player (`a6 == $FF19E8`), reads level stat at `$96(a6)`:
     * `>= 10`: `+1`
     * `>= 20`: `+1`
     * `>= 30`: `+1`
     * `>= 40`: `+1`
     * `>= 50`: `+1`
     * `>= 100`: `+2`
     * `>= 200`: `+4`
5. **Damage Packing:**
   - Base damage from record `+2`.
   - Stun duration: `(record[3] & 0x0F) + 1`.
   - Element flags: `(record[3] & 0xF0) << 8`.
   - Total damage: `(base_damage + rank_bonus | element_flags) + attacker_bonus_90`.
   - Packed `a4`: `((total_damage & 0xFFFF) << 16) | stun_duration`.
6. **3D AABB Overlap:**
   - Target Z span: `[target_z + target_elev, target_z + target_elev + target_height_4a]`.
   - Target X span: `[target_x - target_rad_x_42, target_x + target_rad_x_42]`.
   - Target Y span: `[target_y + target_elev - target_rad_y_44, target_y + target_elev + target_rad_y_44]`.
   - Target filters: `slot_type > 0`, `target != attacker`, bit 3 of `$38(target) == 0`, `target_cooldown_82 == 0`.
   - On intersection: invokes callback pointer stored in `$FF1940.l`.
7. **Post-Loop Weapon Durability:**
   - If attacker is Ali (`a6 == $FF19E8`) and hit registered (bit 3 of `$37(a6)` set):
     If attack type `d0 in (6, 10)`: calls durability reduction `0x008530`.

## 4. Recommended C++20 Interface
```cpp
namespace thor::combat {

struct AttackFrameHitbox {
    int16_t z_min, z_max;
    int16_t x_min, y_min;
    int16_t x_max, y_max;
};

struct AttackDamageDesc {
    uint16_t damage;
    uint8_t stun_duration;
    uint32_t packed_a4;
};

AttackDamageDesc calculate_attack_damage(uint8_t base_dmg, uint8_t element_raw,
                                        uint16_t stat_96, uint16_t bonus_90,
                                        bool is_ali);

bool check_3d_hitbox_overlap(const AttackFrameHitbox& box,
                             const EntityState& target);

} // namespace thor::combat
```
