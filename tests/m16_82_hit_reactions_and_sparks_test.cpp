#include "game/combat/hit_reactions_and_sparks.hpp"

#include <cassert>

int main() {
    using oasis::game::combat::apply_hit_reaction_physics;
    using oasis::game::combat::apply_shield_parry;
    using oasis::game::combat::HitReactionKind;
    using oasis::game::combat::HitReactionPhysicsState;
    using oasis::game::combat::HitSparkPool4;

    // 1. Reaction 0 grounded: (0x300, 0x2C0, 0).
    HitReactionPhysicsState st{};
    st.airborne = false;
    apply_hit_reaction_physics(st, HitReactionKind::LightGroundOrJuggle, 1U); // Right
    assert(st.vx == 0x0300 && st.vy == 0 && st.vz == 0 && (st.flags_86 & 0x80U) == 0U);

    // 2. Reaction 0 airborne: sets bit 7 and falls through to MediumKnockback (4).
    st = HitReactionPhysicsState{};
    st.airborne = true;
    apply_hit_reaction_physics(st, HitReactionKind::LightGroundOrJuggle, 2U); // Down
    assert((st.flags_86 & 0x80U) != 0U);
    assert(st.vx == 0 && st.vy == 0x03C0 && st.vz == 0x28000);

    // 3. Reaction 2: VerticalLauncher (0, 0, 0x70000).
    st = HitReactionPhysicsState{};
    apply_hit_reaction_physics(st, HitReactionKind::VerticalLauncher, 3U);
    assert(st.vx == 0 && st.vy == 0 && st.vz == 0x70000);

    // 4. Reaction 6: HeavyKnockback (0x700, 0x680, 0x30000).
    st = HitReactionPhysicsState{};
    apply_hit_reaction_physics(st, HitReactionKind::HeavyKnockback, 3U); // Left
    assert(st.vx == -0x0700 && st.vy == 0 && st.vz == 0x30000);

    // 5. Shield Parry (0x00BFAE): SFX 0x00900202, recoil on weapon 6 or 10, HitSparkPool4.
    HitSparkPool4 pool;
    const auto p1 = apply_shield_parry(6U, 100, 200, pool);
    assert(p1.sfx_cmd == 0x00900202U && p1.ali_recoil_triggered && p1.spark_spawned);
    const auto p2 = apply_shield_parry(10U, 110, 200, pool);
    assert(p2.ali_recoil_triggered && p2.spark_spawned);
    const auto p3 = apply_shield_parry(0U, 120, 200, pool);
    assert(!p3.ali_recoil_triggered && p3.spark_spawned);

    assert(pool.active_count() == 3U);
    assert(pool.slots()[0].type == 2U && pool.slots()[0].timer == 5U);
    for (int f = 0; f < 5; ++f) {
        pool.tick_sparks();
    }
    assert(pool.active_count() == 0U);

    return 0;
}
