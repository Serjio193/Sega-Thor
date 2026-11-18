# STANDALONE HANDOFF: Milestone M15.454 — ROM Entity Type $36 Boss Attachment, Hierarchical Parent-Child Transform & 3D Combat Physics Subsystem

**ROM Range:** `0x00EA76..0x00ED04` (654 bytes, 169 instructions)  
**Evidence Artifact:** `data/thor-evidence/rom_boss_attachment_engine_m15_454.json`  
**Reference Extractor:** `tools/extract_rom_boss_attachment_engine.py`  
**Reference Unit Test:** `tests/rom_boss_attachment_engine_test.py`

---

## 1. Architectural Summary & System Context

This milestone reverse-engineers the Entity Type `$36` boss multi-part attachment and hierarchical transform subsystem in Beyond Oasis.
This system powers compound boss entities and attached appendages (arms, tentacles, shields, heads), binding them to a parent entity's world position through relative 3D offset vectors, running 3D combat collision queries, managing projectile detachment trajectories, and tracking boss health HUD display.

### Subroutines in this Range:
1. `0x00EA76..0x00EAC0` (`attachment_update_parent_child_transforms` — 8 callers):
   - Evaluates parent tracking flag: `tst.w +$A8(a6)`.
   - Latch timer state `+$B8(a6)`:
     - If `+$B8 == 1`: despawns entity slot (`0x00EC54`).
     - If `+$B8 > 1`: decrements `+$B8`, reads parent pointer `+$B4(a6)`.
     - Hierarchical transform update:
       - `+$08(a6) = +$08(parent) + +$AE(a6)` (world X = parent X + rel X)
       - `+$0C(a6) = +$0C(parent) + +$B0(a6)` (world Y = parent Y + rel Y)
       - `+$10(a6) = +$10(parent) + +$B2(a6)` (elevation Z = parent Z + rel Z)
2. `0x00EAC0..0x00EBC4` (`attachment_combat_overlap_and_knockback` — 4 callers):
   - Performs 3D AABB combat query via `0x00B922`.
   - Filters target entity status (`flags_3a` bit 1, `flags_38` bit 2/3, cooldown `+$82`).
   - Applies damage: `+$86(target) = +$A8(a6)`, attacker link `+$7E(target) = +$AA(a6)`.
   - Sets reaction mode: `+$83(target) = 0x0A`.
   - Projects 8-way directional velocity via `0x00E2D4` with vertical launch `#$20000`.
   - If attacker is Ali (`$FF19E8`), updates boss HUD health tracking:
     `$FF184F = 0x5A`, `$FF1850 = target`.
   - Plays impact sound `#$800210`.
3. `0x00EBC4..0x00EC0C` (`attachment_projectile_trajectory_and_cull` — 6 callers):
   - Gravity integration: `+$56(a6) -= 0x00006000`, adds to elevation `+$10(a6)`.
   - Floor clamp: if `+$10 < 0`, clamps `+$10 = 0`, `+$56 = 0`.
   - Viewport culling test via `0x00E5AE` with margin `d7 = 10`. If offscreen, despawns entity.
4. `0x00EC0C..0x00EC58` (`attachment_latch_to_parent` — 6 callers):
   - Computes relative offsets:
     - `+$AE(a6) = child_x - parent_x`
     - `+$B0(a6) = child_y - parent_y`
     - `+$B2(a6) = child_z - parent_z`
   - Stores parent pointer: `+$B4(a6) = parent`.
   - Sets latch timer: `+$B8(a6) = 8` frames.
   - Slot despawn helper `0x00EC54`: clears `+$00(a6) = 0`.
5. `0x00EC5A..0x00ED04` (`attachment_secondary_branch_physics`):
   - Detached part secondary physics, terrain collision, and viewport culling.

---

## 2. C++20 Cleanroom API Specification

```cpp
namespace oasis::entity {

struct AttachmentTransform {
    int16_t rel_x{0};
    int16_t rel_y{0};
    int16_t rel_z{0};
    uint32_t parent_ptr{0};
    uint8_t latch_timer{0};
};

constexpr void latch_to_parent(AttachmentTransform& transform,
                               int16_t child_x, int16_t child_y, int16_t child_z,
                               int16_t parent_x, int16_t parent_y, int16_t parent_z,
                               uint32_t parent_addr) noexcept
{
    transform.rel_x = child_x - parent_x;
    transform.rel_y = child_y - parent_y;
    transform.rel_z = child_z - parent_z;
    transform.parent_ptr = parent_addr;
    transform.latch_timer = 8U;
}

constexpr bool update_attached_child(AttachmentTransform& transform,
                                    int16_t parent_x, int16_t parent_y, int16_t parent_z,
                                    int16_t& out_child_x, int16_t& out_child_y, int16_t& out_child_z) noexcept
{
    if (transform.latch_timer == 1U) {
        transform.latch_timer = 0;
        return false; // Despawn
    }

    if (transform.latch_timer > 1U) {
        --transform.latch_timer;
        if (transform.parent_ptr != 0) {
            out_child_x = parent_x + transform.rel_x;
            out_child_y = parent_y + transform.rel_y;
            out_child_z = parent_z + transform.rel_z;
        }
    }
    return true;
}

} // namespace oasis::entity
```

---

## 3. Acceptance Criteria

1. **Relative Transform Calculation:** Precise computation of `(rel_x, rel_y, rel_z)`.
2. **Parent Tracking Parity:** Child coordinates exactly track moving parent plus relative offsets.
3. **Boss HUD Integration:** Proper tracking activation (`$FF184F = 0x5A, $FF1850 = target`) on Ali attacks.
4. **Trajectory & Gravity:** Exact `-0x6000` gravity acceleration and viewport margin `d7 = 10` culling.
5. All C++ source and test files must strictly stay `<= 500 lines`.
