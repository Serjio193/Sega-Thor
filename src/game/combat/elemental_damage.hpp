#pragma once

#include "game/entity/native_entity.hpp"

#include <cstdint>

namespace oasis::game::combat {

// Result of ROM 0x017E30 universal damage & elemental bit-13 pipeline.
struct UniversalDamageResult {
    bool had_damage{false};
    bool elemental_bit_13{false};
    std::uint16_t applied_damage{0};
    std::int16_t hp_after{0};
    bool died{false};
};

// Models ROM 0x017E30 Universal Damage & Elemental Bit-13 Pipeline:
//   1. Reads and clears incoming damage word $86(A6) (`ent.damage_mailbox`).
//   2. Tests bit 13 (0x2000) at 0x017E4A (`btst.b #$d, d0`): if set (e.g. Dytto's
//      0x2008 / 0xA008), sets $39(A6) bit 3 (`ent.flags39 |= 0x08U`) and dispatches
//      SFX 0x8004D9 (`0x00E7E8`).
//   3. Masks magnitude with 0x0FFF (0x017E66), subtracts defense $92(A6) (0x017E6A),
//      and clamps minimum damage to 1.
//   4. Subtracts from HP $8A(A6) (clamped >= 0); on 0 HP, transitions to death State 2
//      (0x008EF8: state = 2, action_timer = 0x10, $38(A6) bit 3, invulnerable = true).
UniversalDamageResult apply_universal_elemental_damage(NativeEntity& ent) noexcept;

} // namespace oasis::game::combat
