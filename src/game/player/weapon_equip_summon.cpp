#include "game/player/weapon_equip_summon.hpp"

namespace oasis::game::player {

SpiritSummonTriggerResult start_spirit_summon() noexcept {
    return SpiritSummonTriggerResult{
        .summon_lock_ff1990 = true,
        .player_state = kSpiritSummonPlayerState,
        .anim_state_2a = kSpiritSummonPlayerState,
        .sound_cmd = kSpiritSummonSfx,
    };
}

std::uint8_t map_weapon_item_to_class(std::uint8_t item_id) noexcept {
    if (item_id == 1U) {
        return kWeaponClassDagger;
    }
    if (item_id >= 2U && item_id <= 5U) {
        return kWeaponClassSword;
    }
    if (item_id >= 6U && item_id <= 8U) {
        return kWeaponClassBow;
    }
    if (item_id >= 9U && item_id <= 11U) {
        return kWeaponClassBomb;
    }
    if (item_id == 12U) {
        return kWeaponClassBow;
    }
    return kWeaponClassDagger;
}

WeaponQuickEquipResult quick_equip_weapon_slot(
    const WeaponInventory& inventory,
    WeaponQuickEquipState state,
    std::uint8_t target_slot) noexcept {
    WeaponQuickEquipResult result{};
    result.next_state = state;

    std::uint8_t resolved_slot = target_slot;
    if (resolved_slot == state.equipped_slot) {
        resolved_slot = 0U;
    }

    if (resolved_slot >= inventory.size() || inventory[resolved_slot].id == kWeaponNone) {
        result.equipped = false;
        result.sound_cmd = kWeaponEquipErrorSfx;
        return result;
    }

    const std::uint8_t item_id = inventory[resolved_slot].id;
    result.equipped = true;
    result.next_state.equipped_slot = resolved_slot;
    result.next_state.equipped_item_id = item_id;
    result.next_state.weapon_class = map_weapon_item_to_class(item_id);
    result.next_state.anim_counter = 0U;
    result.next_state.toast_item_id = item_id;
    result.next_state.toast_flag = 0U;
    result.next_state.toast_timer = kEquipToastFrames;
    result.next_state.hud_refresh_ff198f = true;
    result.sound_cmd = kWeaponEquipSfx;
    return result;
}

} // namespace oasis::game::player
