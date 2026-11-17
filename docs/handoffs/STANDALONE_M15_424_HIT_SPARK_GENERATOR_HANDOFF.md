# STANDALONE HANDOFF: ROM Combat Hit Spark Particle Generator & 3D Overlap Centerpoint Calculator (0x00C012..0x00C0BE)

## 1. Routine Overview
- **ROM Range:** `0x00C012..0x00C0BE` (172 bytes).
- **Function:** Combat Hit Spark Particle Generator & 3D Overlap Centerpoint Calculator. Allocates active spark particle entries from a 4-slot ring buffer (`$FF19C8..$FF19E8`), computes the 3D intersection volume between the active weapon attack hitbox and target entity AABB, calculates the 3D centerpoint of impact, projects coordinates onto 2D screen space (`X = mid_x`, `Y = mid_y - mid_z`), and initializes particle lifetime to 5 frames.
- **Evidence JSON:** `data/thor-evidence/rom_hit_spark_generator_m15_424.json`.

## 2. Ingress Entry Point
- Address: `0x00C012`.
- Direct call sites:
  * `0x00BEFE` (unblocked combat hit)
  * `0x00BFF6` (blocked shield clash)

## 3. Algorithm & Logic
1. **Particle Ring Buffer Allocation (`$FF19C8..$FF19E8`):**
   - 4 slots of 8 bytes:
     * `0x00FF19C8` (Slot 0)
     * `0x00FF19D0` (Slot 1)
     * `0x00FF19D8` (Slot 2)
     * `0x00FF19E0` (Slot 3)
   - Checks `*(uint16_t*)slot == 0`. If found, claims slot and jumps to initialization (`0x00C038`).
   - If all 4 slots are non-zero: exits immediately with `rts` (drop particle on saturated buffer).
2. **Particle Slot Format:**
   - `+0(a2)` (`uint16_t`): active status / type code = `2`.
   - `+2(a2)` (`int16_t`): particle X coordinate (`mid_x`).
   - `+4(a2)` (`int16_t`): particle projected Y coordinate (`mid_y - mid_z`).
   - `+6(a2)` (`uint16_t`): lifetime counter = `5` frames.
3. **3D Intersection Midpoint Projection:**
   - **Z Midpoint:**
     `tz_min = target.z + target.elev`
     `tz_max = tz_min + target.height_4a`
     `z0 = max(hitbox.z_min, tz_min)`
     `z1 = min(hitbox.z_max, tz_max)`
     `mid_z = z0 + ((z1 - z0) >> 1)`
   - **X Midpoint:**
     `tx_min = target.x - target.rad_x_42`
     `tx_max = target.x + target.rad_x_42`
     `x0 = max(hitbox.x_min, tx_min)`
     `x1 = min(hitbox.x_max, tx_max)`
     `mid_x = x0 + ((x1 - x0) >> 1)`
   - **Y Midpoint:**
     `ty_min = target.y + target.elev - target.rad_y_44`
     `ty_max = target.y + target.elev + target.rad_y_44`
     `y0 = max(hitbox.y_min, ty_min)`
     `y1 = min(hitbox.y_max, ty_max)`
     `mid_y = y0 + ((y1 - y0) >> 1)`
   - **Projected Position:**
     `spark_x = mid_x`
     `spark_y = mid_y - mid_z` (Y elevation offset subtracted!)

## 4. Recommended C++20 Interface
```cpp
namespace thor::combat {

struct HitSparkParticle {
    uint16_t type{0};      // 0 = free, 2 = active
    int16_t x{0};
    int16_t y{0};
    uint16_t lifetime{0};  // 5 frames
};

class HitSparkManager {
public:
    static constexpr size_t kMaxSparks = 4;
    static constexpr uint16_t kSparkType = 2;
    static constexpr uint16_t kDefaultLifetime = 5;

    bool spawn_spark(const AttackFrameHitbox& hitbox, const EntityState& target);
    void update_frame();
    std::span<const HitSparkParticle> active_sparks() const;

private:
    std::array<HitSparkParticle, kMaxSparks> m_sparks{};
};

} // namespace thor::combat
```
