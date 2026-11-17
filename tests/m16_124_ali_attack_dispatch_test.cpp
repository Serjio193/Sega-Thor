#include "game/player/ali_attack_dispatch.hpp"

#include <cassert>

namespace {

using namespace oasis::game::player;

void test_ground_attack_dispatch() {
    // 1. Dagger ground: anim 0x3A, state 0x0A
    AliAttackInput in{};
    in.weapon_class = kWeaponClassDagger;
    auto res = dispatch_ali_attack(in);
    assert(res.dispatched);
    assert(res.anim == kAnimDaggerGround);
    assert(res.state == kStateDaggerGround);
    assert(res.anim_rate == kAttackAnimRate);
    assert(res.clear_input_ring);
    assert(res.sound_cmd == 0U);

    // 2. Sword ground: slash (0x4E, 0x46), thrust (0x60, 0x54), spin (0x66, 0x5C)
    in.weapon_class = kWeaponClassSword;
    in.dpad_held = false;
    in.spin_cmd = false;
    res = dispatch_ali_attack(in);
    assert(res.dispatched && res.anim == kAnimSwordSlash && res.state == kStateSwordSlash);

    in.dpad_held = true;
    res = dispatch_ali_attack(in);
    assert(res.dispatched && res.anim == kAnimSwordThrust && res.state == kStateSwordThrust);

    in.spin_cmd = true;
    res = dispatch_ali_attack(in);
    assert(res.dispatched && res.anim == kAnimSwordSpin && res.state == kStateSwordSpin);

    // 3. Bow ground: normal (0x54, 0x4C), hyper item 8 (0x5C, 0x4C), blocked with active proj
    in.weapon_class = kWeaponClassBow;
    in.dpad_held = false;
    in.spin_cmd = false;
    in.equipped_item_id = 6U;
    in.active_projectile = false;
    res = dispatch_ali_attack(in);
    assert(res.dispatched);
    assert(res.anim == kAnimBowFire);
    assert(res.state == kStateBowFire);
    assert(res.sound_cmd == kBowFireSfx);

    in.equipped_item_id = kHyperBowItemId;
    res = dispatch_ali_attack(in);
    assert(res.dispatched);
    assert(res.anim == kAnimBowHyperFire);
    assert(res.state == kStateBowFire);
    assert(res.sound_cmd == kBowFireSfx);

    in.active_projectile = true;
    res = dispatch_ali_attack(in);
    assert(!res.dispatched);

    // 4. Bomb ground: throw (0x58, 0x50), blocked with active proj
    in.weapon_class = kWeaponClassBomb;
    in.active_projectile = false;
    res = dispatch_ali_attack(in);
    assert(res.dispatched);
    assert(res.anim == kAnimBombThrow);
    assert(res.state == kStateBombThrow);

    in.active_projectile = true;
    res = dispatch_ali_attack(in);
    assert(!res.dispatched);
}

void test_airborne_attack_dispatch() {
    AliAttackInput in{};
    in.is_airborne = true;

    // 1. Dagger airborne: air-kick (0x40, 0x38), flags_39 |= 4, stun 0x15
    in.weapon_class = kWeaponClassDagger;
    auto res = dispatch_ali_attack(in);
    assert(res.dispatched);
    assert(res.anim == kAnimDaggerAirKick);
    assert(res.state == kStateDaggerAirKick);
    assert(res.set_flags_39 == 0x04U);
    assert(res.air_stun_9a == kAirKickStunFrames);

    // 2. Sword airborne: spin allowed, normal rejected
    in.weapon_class = kWeaponClassSword;
    in.spin_cmd = false;
    res = dispatch_ali_attack(in);
    assert(!res.dispatched);

    in.spin_cmd = true;
    res = dispatch_ali_attack(in);
    assert(res.dispatched);
    assert(res.anim == kAnimSwordSpin);
    assert(res.state == kStateSwordSpin);

    // 3. Bow & Bomb airborne: always rejected
    in.weapon_class = kWeaponClassBow;
    assert(!dispatch_ali_attack(in).dispatched);
    in.weapon_class = kWeaponClassBomb;
    assert(!dispatch_ali_attack(in).dispatched);
}

void test_rom_evidence_equivalence() {
    // Tests matching tests/rom_ali_attack_dispatch_test.py
    assert(dispatch_ground_attack(0x0A, 1, false, false, false) == std::make_pair(0x003AU, 0x000AU));
    assert(dispatch_ground_attack(0x0C, 2, false, false, false) == std::make_pair(0x004EU, 0x0046U));
    assert(dispatch_ground_attack(0x0C, 2, true, false, false) == std::make_pair(0x0060U, 0x0054U));
    assert(dispatch_ground_attack(0x0C, 2, false, true, false) == std::make_pair(0x0066U, 0x005CU));
    assert(dispatch_ground_attack(0x0E, 6, false, false, false) == std::make_pair(0x0054U, 0x004CU));
    assert(dispatch_ground_attack(0x0E, 8, false, false, false) == std::make_pair(0x005CU, 0x004CU));
    assert(!dispatch_ground_attack(0x0E, 8, false, false, true).has_value());
    assert(dispatch_ground_attack(0x10, 9, false, false, false) == std::make_pair(0x0058U, 0x0050U));
}

} // namespace

int main() {
    test_ground_attack_dispatch();
    test_airborne_attack_dispatch();
    test_rom_evidence_equivalence();
    return 0;
}
