#include "game/save/sram_save_system.hpp"
#include "game/ui/ring_menu_and_rank.hpp"

#include <cstdlib>
#include <iostream>

#define OASIS_CHECK(cond)                                                                      \
    do {                                                                                       \
        if (!(cond)) {                                                                         \
            std::cerr << "CHECK FAILED: " #cond " at line " << __LINE__ << std::endl;          \
            std::exit(1);                                                                      \
        }                                                                                      \
    } while (0)

namespace {

void test_ring_menu_state() {
    using namespace oasis::game;
    ui::RingMenuState menu{};
    OASIS_CHECK(ui::RingMenuConfig::kIconTilesetRomAddr == 0x00155D76U);
    OASIS_CHECK(ui::RingMenuConfig::kTilesPerIcon == 9U);
    OASIS_CHECK(ui::RingMenuState::slot_x_offset_px(3U) == 72);

    // Weapon cursor wraps modulo 8
    menu.step_weapon_cursor(-1);
    OASIS_CHECK(menu.weapon_cursor == 7U);
    menu.step_weapon_cursor(1);
    OASIS_CHECK(menu.weapon_cursor == 0U);

    // Item cursor wraps modulo 16
    menu.step_item_cursor(-1);
    OASIS_CHECK(menu.item_cursor == 15U);
    menu.step_item_cursor(2);
    OASIS_CHECK(menu.item_cursor == 1U);

    save::PersistentWramBlock wram{};
    wram.init_default_inventory();
    wram.set_item_slot(0U, 0x1EU, 3U);
    menu.sync_from_wram(wram);
    OASIS_CHECK(menu.weapon_slots[0] == 0x01FFU);
    OASIS_CHECK(menu.weapon_slots[1] == 0x05FEU);
    OASIS_CHECK(menu.item_slots[0] == 0x1E03U);

    std::cout << "[PASS] Phase 1: Pause Ring Menu (0x0042E8 / 0x00155D76) Verified.\n";
}

void test_player_rank_and_pickups() {
    using oasis::game::ui::PlayerRankStats;

    OASIS_CHECK(PlayerRankStats::max_hp_for_rank(0U) == 198);
    OASIS_CHECK(PlayerRankStats::max_hp_for_rank(10U) == 218);
    OASIS_CHECK(PlayerRankStats::max_hp_for_rank(200U) == 598);
    OASIS_CHECK(PlayerRankStats::max_hp_for_rank(250U) == 598); // Capped at 200

    PlayerRankStats stats{};
    stats.hp = 190;
    OASIS_CHECK(stats.apply_rank_up());
    OASIS_CHECK(stats.rank == 1U && stats.max_hp == 200 && stats.hp == 192);

    stats.rank = 199U;
    stats.max_hp = PlayerRankStats::max_hp_for_rank(199U);
    stats.hp = stats.max_hp;
    OASIS_CHECK(stats.apply_rank_up());
    OASIS_CHECK(stats.rank == 200U && stats.max_hp == 598 && stats.hp == 598);
    OASIS_CHECK(!stats.apply_rank_up()); // Cap 200 reached

    stats.sp = 97;
    stats.max_sp = 100;
    OASIS_CHECK(stats.apply_sp_pickup() == 3);
    OASIS_CHECK(stats.sp == 100);
    OASIS_CHECK(stats.apply_sp_pickup() == 0);

    OASIS_CHECK(stats.increment_gem_counter(0x0DU));
    OASIS_CHECK(stats.increment_gem_counter(0x10U));
    OASIS_CHECK(!stats.increment_gem_counter(0x0CU));
    OASIS_CHECK(stats.gem_counters[0] == 1U && stats.gem_counters[3] == 1U);

    std::cout << "[PASS] Phase 2: Player Rank/MaxHP Scaling (0x007BF6), SP Pickup & Gems Verified.\n";
}

} // namespace

int main() {
    std::cout << "=====================================================================\n"
              << "SEGA-THOR M16.59: PAUSE RING MENU & PLAYER RANK/MAXHP SCALING\n"
              << "=====================================================================\n";
    test_ring_menu_state();
    test_player_rank_and_pickups();
    std::cout << "\n[PASS] All M16.59 Ring Menu & Player Rank Tests PASSED!\n";
    return 0;
}
