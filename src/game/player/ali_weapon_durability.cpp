#include "game/player/ali_weapon_durability.hpp"

namespace oasis::game::player {

WeaponDurabilityResult step_weapon_durability(InventoryState& inv) noexcept {
    WeaponDurabilityResult res{};
    const auto slot_idx = inv.equipped_slot;
    if (slot_idx >= inv.slots.size()) {
        return res;
    }
    auto& slot = inv.slots[slot_idx];
    const auto wid = slot.id;
    if (wid < CombatWeaponDurabilityConfig::kMinConsumableWeaponId ||
        wid > CombatWeaponDurabilityConfig::kMaxConsumableWeaponId) {
        return res;
    }
    const auto dur = slot.durability;
    res.prev_durability = dur;

    if (dur == CombatWeaponDurabilityConfig::kInfiniteDurability) {
        res.new_durability = dur;
        return res;
    }

    if (dur == 0U || dur == 1U) {
        res.broke = true;
        res.new_durability = 0U;
        slot.id = 0U;
        slot.durability = 0U;

        inv.equipped_slot = 0U;
        inv.equipped_id = CombatWeaponDurabilityConfig::kDefaultDaggerId;
        inv.revert_timer = CombatWeaponDurabilityConfig::kRevertTimerDuration;
        inv.sub_state = 0U;
        inv.hud_weapon_id = inv.equipped_id;
        inv.hud_weapon_state = 0U;
        inv.hud_toast_timer = CombatWeaponDurabilityConfig::kHudToastTimerDuration;
        inv.break_flag = 0xFFU;
        res.sfx_id = CombatWeaponDurabilityConfig::kBreakSfxId;
        return res;
    }

    slot.durability = static_cast<std::uint8_t>(dur - 1U);
    res.decremented = true;
    res.new_durability = slot.durability;
    return res;
}

std::tuple<bool, std::uint8_t, std::uint8_t> step_weapon_durability_direct(
    std::uint8_t weapon_id,
    std::uint8_t durability) noexcept {
    if (weapon_id < CombatWeaponDurabilityConfig::kMinConsumableWeaponId ||
        weapon_id > CombatWeaponDurabilityConfig::kMaxConsumableWeaponId) {
        return {false, weapon_id, durability};
    }
    if (durability == CombatWeaponDurabilityConfig::kInfiniteDurability) {
        return {false, weapon_id, durability};
    }
    if (durability == 0U || (durability - 1U) == 0U) {
        return {true, 0U, 0U};
    }
    return {false, weapon_id, static_cast<std::uint8_t>(durability - 1U)};
}

bool check_threshold_guard(
    std::uint16_t& value,
    std::uint16_t limit,
    std::uint8_t& flag) noexcept {
    if (value > limit) {
        value = 0U;
        flag = CombatWeaponDurabilityConfig::kTrippedFlagValue;
        return true;
    }
    return false;
}

} // namespace oasis::game::player
