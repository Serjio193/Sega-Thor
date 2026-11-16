#include "game/ui/ring_menu_and_rank.hpp"

#include <algorithm>

namespace oasis::game::ui {

void RingMenuState::step_weapon_cursor(int delta) noexcept {
    constexpr int n = static_cast<int>(RingMenuConfig::kWeaponSlotCount);
    int next = (static_cast<int>(weapon_cursor) + delta) % n;
    if (next < 0) next += n;
    weapon_cursor = static_cast<std::uint8_t>(next);
}

void RingMenuState::step_item_cursor(int delta) noexcept {
    constexpr int n = static_cast<int>(RingMenuConfig::kItemSlotCount);
    int next = (static_cast<int>(item_cursor) + delta) % n;
    if (next < 0) next += n;
    item_cursor = static_cast<std::uint8_t>(next);
}

void RingMenuState::sync_from_wram(const save::PersistentWramBlock& wram) noexcept {
    weapon_slots[0] = wram.equip_status_word();
    for (std::size_t i = 0; i < 7U; ++i) {
        weapon_slots[1U + i] = wram.weapon_slot(i);
    }
    for (std::size_t i = 0; i < RingMenuConfig::kItemSlotCount; ++i) {
        item_slots[i] = static_cast<std::uint16_t>(
            (static_cast<std::uint16_t>(wram.item_id(i)) << 8U) | wram.item_count(i));
    }
}

bool PlayerRankStats::apply_rank_up() noexcept {
    if (rank >= kMaxRank) {
        return false;
    }
    ++rank;
    max_hp = max_hp_for_rank(rank);
    hp = static_cast<std::int16_t>(std::min<int>(max_hp, hp + 2));
    return true;
}

std::int16_t PlayerRankStats::apply_sp_pickup() noexcept {
    const std::int16_t next_sp = static_cast<std::int16_t>(std::min<int>(max_sp, sp + kSpPickupStep));
    const std::int16_t delta = static_cast<std::int16_t>(next_sp - sp);
    sp = next_sp;
    return delta;
}

bool PlayerRankStats::increment_gem_counter(std::uint8_t item_id) noexcept {
    if (item_id < 0x0DU || item_id > 0x10U) {
        return false;
    }
    const std::size_t idx = static_cast<std::size_t>(item_id - 0x0DU);
    ++gem_counters[idx];
    return true;
}

} // namespace oasis::game::ui
