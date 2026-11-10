#pragma once

#include "game/entity/native_entity.hpp"

#include <cstdint>

namespace oasis::game::combat {

// M68000 Status Register Condition Code Register (CCR) bits: X(4) N(3) Z(2) V(1) C(0).
struct M68kDamageResult {
    std::uint16_t applied_damage{0};
    std::int16_t hp_after{0};
    std::uint8_t ccr{0};       // Lower 5 bits of SR after arithmetic block
    bool sound_04d9{false};    // Triggered when incoming mailbox bit 13 (0x2000) is set
    bool zero_hp{false};
};

// Reproduces the exact M68000 sequence in ROM 0x01F44C..0x01F466 & 0x0256F6..0x025714:
//   andi.w #$0FFF, d0
//   sub.w  $92(a6), d0  ; unsigned comparison via C & Z for bhi
//   bhi.s  +2
//   moveq  #1, d0
//   move.w $8A(a6), d1
//   sub.w  d0, d1       ; signed sign-bit test via N for bpl
//   bpl.s  +2
//   clr.w  d1
//   move.w d1, $8A(a6)
[[nodiscard]] inline M68kDamageResult compute_m68k_damage_step(std::uint16_t raw_mailbox,
                                                                std::int16_t defense,
                                                                std::int16_t hp_before,
                                                                bool initial_x_flag = false) noexcept {
    M68kDamageResult res{};
    res.sound_04d9 = (raw_mailbox & 0x2000U) != 0U;

    const std::uint16_t masked = raw_mailbox & 0x0FFFU;
    const std::uint16_t def_u16 = static_cast<std::uint16_t>(defense);
    const std::uint32_t diff_def_u32 = static_cast<std::uint32_t>(masked) - static_cast<std::uint32_t>(def_u16);
    const std::uint16_t diff_def = static_cast<std::uint16_t>(diff_def_u32 & 0xFFFFU);

    // BHI checks (!C && !Z), equivalent to unsigned masked > def_u16.
    const bool bhi_taken = (masked > def_u16);
    const std::uint16_t dmg = bhi_taken ? diff_def : 1U;
    bool x_flag = bhi_taken ? ((diff_def_u32 >> 16) != 0U) : ((diff_def_u32 >> 16) != 0U);
    (void)initial_x_flag; // Replaced unconditionally by sub.w $92(a6), d0

    const std::uint16_t hp_u16 = static_cast<std::uint16_t>(hp_before);
    const std::uint32_t diff_hp_u32 = static_cast<std::uint32_t>(hp_u16) - static_cast<std::uint32_t>(dmg);
    const std::uint16_t diff_hp = static_cast<std::uint16_t>(diff_hp_u32 & 0xFFFFU);

    // sub.w d0, d1 updates X, N, Z, V, C
    const bool sub_n = (diff_hp & 0x8000U) != 0U;
    bool sub_z = (diff_hp == 0U);
    bool sub_c = (hp_u16 < dmg);
    const bool sub_v = (((hp_u16 ^ dmg) & (hp_u16 ^ diff_hp)) & 0x8000U) != 0U;
    x_flag = sub_c;

    // BPL checks !N (MSB == 0). Note: clr.w d1 clears N=0, Z=1, V=0, C=0, preserves X!
    std::uint16_t final_hp = diff_hp;
    bool final_v = sub_v;
    bool final_n = sub_n;
    if (sub_n) {
        final_hp = 0U;
        final_n = false;
        sub_z = true;
        final_v = false;
        sub_c = false;
    }

    res.applied_damage = dmg;
    res.hp_after = static_cast<std::int16_t>(final_hp);
    res.zero_hp = (final_hp == 0U);
    res.ccr = static_cast<std::uint8_t>((x_flag ? 0x10U : 0U) | (final_n ? 0x08U : 0U) |
                                        (sub_z ? 0x04U : 0U) | (final_v ? 0x02U : 0U) |
                                        (sub_c ? 0x01U : 0U));
    return res;
}

// Simulates entity_timer_tick_and_yield (ROM 0x00E682..0x00E6A8) for a 30Hz tick:
// Returns true if entity execution should yield (caller skipped) while action_timer remains > 0.
[[nodiscard]] inline bool tick_entity_action_timer_yield(NativeEntity& ent) noexcept {
    ent.flags36 &= static_cast<std::uint8_t>(~0x40U); // bclr #6, $36(a6)
    if (ent.action_timer == 0U) {
        return false; // Normal rts
    }
    ++ent.anim_step;
    --ent.action_timer;
    return ent.action_timer != 0U; // addq.l #4, a7; rts iff timer still > 0
}

// Processes pending damage_mailbox (+0x86) and active Hurt recovery on one 30Hz logic tick.
// Returns true if hit sound 0x04D9 was dispatched.
inline bool step_entity_combat_tick(NativeEntity& ent) noexcept {
    if (!ent.active) {
        return false;
    }
    // ROM 0x00E682 entry guard: yields remaining entity handler if action_timer > 0 after decrement.
    if (tick_entity_action_timer_yield(ent)) {
        return false;
    }

    bool sound_04d9 = false;

    if (ent.damage_mailbox != 0U) {
        const std::uint16_t raw = ent.damage_mailbox;
        ent.damage_mailbox = 0U;

        const auto arith = compute_m68k_damage_step(raw, ent.defense, ent.hp);
        sound_04d9 = arith.sound_04d9;
        ent.hp = arith.hp_after;

        if (ent.type_id == 0x002CU) {
            // Entity 0x002C (ROM 0x0256B8..0x025750)
            if (ent.hp == 0) {
                ent.flash_timer = 0U;
                ent.invulnerable = true; // 0x025746: bset #3, $38(a6)
                ent.flags38 |= 0x08U;
                if (ent.z_fp == 0 && ent.vz_fp != 0) {
                    // Lethal knockback: base +16px elevation before gravity integrates vz
                    ent.z_fp = 16 * 0x10000;
                    ent.world_z = 16;
                } else {
                    ent.world_z += 18;
                }
                ent.state = 0x06U;       // Death state 6
                ent.anim_frame = 0x04U;
                return sound_04d9;
            }
            ent.state = 0x04U;           // Hurt state 4
            ent.anim_frame = 0x08U;      // Hurt anim 8
        } else if (ent.type_id == 0x006EU) {
            // Entity 0x006E (ROM 0x01F41C..0x01F488): always enters State 0x0A Hurt
            // with anim 0x24 (36 + 2*index), sound 0x008004D9 if bit 13 set, before 0x01EF0C.
            ent.previous_state = ent.state;
            ent.previous_anim = ent.anim_frame;
            ent.invulnerable = true;
            ent.flags38 |= 0x08U;
            ent.state = 0x0AU;
            ent.anim_frame = 0x24U;
            ent.flags37 &= static_cast<std::uint8_t>(~0x02U);
            if (sound_04d9) {
                ent.last_sound_cmd = 0x008004D9U;
            }
            return sound_04d9;
        }
    }

    // Active Hurt State step & recovery for 0x002C (ROM 0x025940..0x025964 -> 0x02568A)
    if (ent.type_id == 0x002CU && ent.state == 0x04U) {
        if (ent.flash_timer > 0U) {
            --ent.flash_timer;
        }
        if (ent.flash_timer == 0U) {
            ent.state = 0x00U;      // 0x02568A: recover to idle/patrol state 0
            ent.anim_frame = 0x00U; // 0x025690: anim 0
        } else {
            ent.flags36 |= 0x40U;   // 0x025946: bset #6, $36(a6)
            if (ent.vy_fp != 0) {
                if ((ent.y_fp >> 16) != ent.world_y) {
                    ent.y_fp = ent.world_y * 0x10000;
                }
                ent.y_fp += ent.vy_fp;
                ent.world_y = ent.y_fp >> 16;
            } else {
                ent.world_y -= 3;       // Fallback integer knockback step
            }
        }
    } else if (ent.type_id == 0x006EU && ent.state == 0x0AU) {
        // ROM 0x01EF0C..0x01EF4A: checks animation end (bit 1 of +0x37)
        if ((ent.flags37 & 0x02U) != 0U || ent.hp == 0 || ent.flash_timer == 0U) {
            ent.flash_timer = 0U;
            if (ent.hp == 0) {
                ent.invulnerable = true;
                ent.flags38 = 0x0CU; // bits 2 & 3 set (0x01EF4A -> $38=0x0C)
                ent.state = 0x0CU;   // Death state 12
                ent.anim_frame = 0x24U;
            } else {
                ent.invulnerable = false;
                ent.flags38 = 0x00U;
                ent.state = static_cast<std::uint8_t>(ent.previous_state);
                ent.anim_frame = static_cast<std::uint8_t>(ent.previous_anim);
            }
        }
    }

    return sound_04d9;
}

} // namespace oasis::game::combat
