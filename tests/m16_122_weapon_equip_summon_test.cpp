#include "game/player/weapon_equip_summon.hpp"

#include <cassert>

namespace {

using namespace oasis::game::player;

void test_start_spirit_summon() {
    const auto res = start_spirit_summon();
    assert(res.summon_lock_ff1990);
    assert(res.player_state == 0x14U);
    assert(res.anim_state_2a == 0x14U);
    assert(res.sound_cmd == kSpiritSummonSfx);
}

void test_weapon_class_mapper() {
    assert(map_weapon_item_to_class(1U) == 0x0AU);  // Dagger
    assert(map_weapon_item_to_class(2U) == 0x0CU);  // Sword
    assert(map_weapon_item_to_class(5U) == 0x0CU);  // Omega Sword
    assert(map_weapon_item_to_class(6U) == 0x0EU);  // Bow
    assert(map_weapon_item_to_class(8U) == 0x0EU);  // Metal Bow
    assert(map_weapon_item_to_class(9U) == 0x10U);  // Bomb
    assert(map_weapon_item_to_class(11U) == 0x10U); // Hyper Bomb
    assert(map_weapon_item_to_class(12U) == 0x0EU); // Atomic Bow
    assert(map_weapon_item_to_class(13U) == 0x0AU); // >12 default
}

void test_quick_equip_weapon_slot() {
    WeaponInventory inv{};
    inv[0] = {kWeaponKnife, 0U};
    inv[1] = {kWeaponOmegaSword, 5U};
    inv[2] = {kWeaponAtomicBow, 3U};
    inv[3] = {kWeaponHyperBomb, 2U};
    inv[4] = {kWeaponNone, 0U};

    WeaponQuickEquipState state{};
    state.equipped_slot = 0U;
    state.anim_counter = 7U;

    // Equip slot 1 (Omega Sword -> class 0x0C, 60f toast, 0x00C0025D)
    auto res = quick_equip_weapon_slot(inv, state, 1U);
    assert(res.equipped);
    assert(res.next_state.equipped_slot == 1U);
    assert(res.next_state.equipped_item_id == kWeaponOmegaSword);
    assert(res.next_state.weapon_class == kWeaponClassSword);
    assert(res.next_state.anim_counter == 0U);
    assert(res.next_state.toast_item_id == kWeaponOmegaSword);
    assert(res.next_state.toast_flag == 0U);
    assert(res.next_state.toast_timer == kEquipToastFrames);
    assert(res.next_state.hud_refresh_ff198f);
    assert(res.sound_cmd == kWeaponEquipSfx);
    state = res.next_state;

    // Re-selecting equipped slot 1 toggles back to slot 0 (Knife -> class 0x0A)
    res = quick_equip_weapon_slot(inv, state, 1U);
    assert(res.equipped);
    assert(res.next_state.equipped_slot == 0U);
    assert(res.next_state.equipped_item_id == kWeaponKnife);
    assert(res.next_state.weapon_class == kWeaponClassDagger);
    assert(res.sound_cmd == kWeaponEquipSfx);
    state = res.next_state;

    // Equip slot 2 (Atomic Bow -> class 0x0E) and slot 3 (Hyper Bomb -> class 0x10)
    res = quick_equip_weapon_slot(inv, state, 2U);
    assert(res.equipped && res.next_state.weapon_class == kWeaponClassBow);
    res = quick_equip_weapon_slot(inv, res.next_state, 3U);
    assert(res.equipped && res.next_state.weapon_class == kWeaponClassBomb);
    state = res.next_state;

    // Empty slot 4 fails with error SFX 0x00A00253 and preserves state
    res = quick_equip_weapon_slot(inv, state, 4U);
    assert(!res.equipped);
    assert(res.next_state.equipped_slot == 3U);
    assert(res.sound_cmd == kWeaponEquipErrorSfx);
}

} // namespace

int main() {
    test_start_spirit_summon();
    test_weapon_class_mapper();
    test_quick_equip_weapon_slot();
    return 0;
}
