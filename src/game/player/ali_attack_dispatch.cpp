#include "game/player/ali_attack_dispatch.hpp"

namespace oasis::game::player {

AliAttackDispatchResult dispatch_ali_attack(const AliAttackInput& input) noexcept {
    AliAttackDispatchResult result{};

    if (input.is_airborne) {
        if (input.weapon_class == kWeaponClassSword) {
            if (input.spin_cmd) {
                result.dispatched = true;
                result.anim = kAnimSwordSpin;
                result.state = kStateSwordSpin;
                result.anim_rate = kAttackAnimRate;
                result.clear_input_ring = true;
                return result;
            }
            return result; // Cannot attack with normal sword in mid-air
        }

        if (input.weapon_class == kWeaponClassDagger) {
            // Normal air kick (0x00810E..0x008122)
            result.dispatched = true;
            result.anim = kAnimDaggerAirKick;
            result.state = kStateDaggerAirKick;
            result.anim_rate = kAttackAnimRate;
            result.clear_input_ring = true;
            result.set_flags_39 = 0x04U; // bit 2
            result.air_stun_9a = kAirKickStunFrames;
            return result;
        }

        // Bow and Bomb cannot attack in mid-air (0x0080EC -> 0x7E44)
        return result;
    }

    // Ground attack dispatch (0x008002..0x0080D4)
    if (input.weapon_class == kWeaponClassSword) {
        result.dispatched = true;
        result.anim_rate = kAttackAnimRate;
        result.clear_input_ring = true;
        if (input.spin_cmd) {
            result.anim = kAnimSwordSpin;
            result.state = kStateSwordSpin;
        } else if (input.dpad_held) {
            result.anim = kAnimSwordThrust;
            result.state = kStateSwordThrust;
        } else {
            result.anim = kAnimSwordSlash;
            result.state = kStateSwordSlash;
        }
        return result;
    }

    if (input.weapon_class == kWeaponClassBow) {
        if (input.active_projectile) {
            return result; // Blocked while projectile is active
        }
        result.dispatched = true;
        result.anim_rate = kAttackAnimRate;
        result.clear_input_ring = true;
        result.sound_cmd = kBowFireSfx;
        result.anim = (input.equipped_item_id == kHyperBowItemId)
            ? kAnimBowHyperFire
            : kAnimBowFire;
        result.state = kStateBowFire;
        return result;
    }

    if (input.weapon_class == kWeaponClassBomb) {
        if (input.active_projectile) {
            return result; // Blocked while projectile is active
        }
        result.dispatched = true;
        result.anim_rate = kAttackAnimRate;
        result.clear_input_ring = true;
        result.anim = kAnimBombThrow;
        result.state = kStateBombThrow;
        return result;
    }

    // Default / Dagger ground attack
    result.dispatched = true;
    result.anim = kAnimDaggerGround;
    result.state = kStateDaggerGround;
    result.anim_rate = kAttackAnimRate;
    result.clear_input_ring = true;
    return result;
}

std::optional<std::pair<std::uint16_t, std::uint16_t>> dispatch_ground_attack(
    std::uint16_t weapon_class,
    std::uint8_t equipped_item_id,
    bool dpad_held,
    bool spin_cmd,
    bool active_projectile) noexcept {
    AliAttackInput input{};
    input.weapon_class = weapon_class;
    input.equipped_item_id = equipped_item_id;
    input.is_airborne = false;
    input.dpad_held = dpad_held;
    input.spin_cmd = spin_cmd;
    input.active_projectile = active_projectile;

    const auto res = dispatch_ali_attack(input);
    if (!res.dispatched) {
        return std::nullopt;
    }
    return std::make_pair(res.anim, res.state);
}

} // namespace oasis::game::player
