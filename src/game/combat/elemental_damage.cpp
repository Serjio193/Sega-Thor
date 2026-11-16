#include "game/combat/elemental_damage.hpp"

#include "game/combat/damage_arithmetic.hpp"

namespace oasis::game::combat {

UniversalDamageResult apply_universal_elemental_damage(NativeEntity& ent) noexcept {
    UniversalDamageResult out{};
    if (!ent.active || ent.damage_mailbox == 0U) {
        return out;
    }

    const std::uint16_t raw = ent.damage_mailbox;
    ent.damage_mailbox = 0U;
    out.had_damage = true;

    // 0x017E4A: btst.b #$d, d0 (bit 13 = 0x2000 elemental / hit SFX flag)
    if ((raw & 0x2000U) != 0U) {
        out.elemental_bit_13 = true;
        ent.flags39 |= 0x08U; // Set $39(A6) bit 3
        ent.last_sound_cmd = 0x008004D9U;
    }

    const auto arith = compute_m68k_damage_step(raw, ent.defense, ent.hp);
    out.applied_damage = arith.applied_damage;
    out.hp_after = arith.hp_after;
    ent.hp = arith.hp_after;

    // 0x008EF8: HP == 0 -> transition to death State 2, timer 0x10, $38(A6) bit 3
    if (arith.zero_hp) {
        out.died = true;
        ent.state = 0x02U;
        ent.action_timer = 0x10U;
        ent.flags38 |= 0x08U;
        ent.invulnerable = true;
    }

    return out;
}

} // namespace oasis::game::combat
