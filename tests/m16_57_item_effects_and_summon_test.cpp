#include "game/items/item_effects.hpp"
#include "game/save/sram_save_system.hpp"
#include "game/spirits/dytto_controller.hpp"

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

void test_eligibility_0x005236() {
    using namespace oasis::game::items;

    ItemUseContext ctx{};
    ctx.area_id = 0x0106U;
    ctx.hp = 50;
    ctx.max_hp = 100;
    ctx.sp = 50;
    ctx.max_sp = 100;
    ctx.unlocked_spirit_mask = 0x01U; // Dytto unlocked

    // Area 0x10 blocks all items
    ctx.area_id = 0x0010U;
    OASIS_CHECK(!can_use_item(0x1AU, ctx));
    OASIS_CHECK(!can_use_item(0x1EU, ctx));
    ctx.area_id = 0x1000U;
    OASIS_CHECK(!can_use_item(0x26U, ctx));
    ctx.area_id = 0x0106U;

    // Spirit Ring 0x1A..0x1D checks
    OASIS_CHECK(can_use_item(0x1AU, ctx));
    OASIS_CHECK(!can_use_item(0x1BU, ctx)); // Efreet bit 1 not set
    ctx.summon_locked = true;
    OASIS_CHECK(!can_use_item(0x1AU, ctx));
    ctx.summon_locked = false;
    ctx.spirit_active = true;
    OASIS_CHECK(!can_use_item(0x1AU, ctx));
    ctx.spirit_active = false;
    ctx.sp = 0;
    OASIS_CHECK(!can_use_item(0x1AU, ctx));
    ctx.sp = 50;

    // HP / SP / Dual checks
    OASIS_CHECK(can_use_item(0x1EU, ctx));
    OASIS_CHECK(can_use_item(0x22U, ctx));
    OASIS_CHECK(can_use_item(0x26U, ctx));
    ctx.hp = 100;
    OASIS_CHECK(!can_use_item(0x1EU, ctx));
    OASIS_CHECK(can_use_item(0x26U, ctx)); // SP still < max_sp
    ctx.sp = 100;
    OASIS_CHECK(!can_use_item(0x22U, ctx));
    OASIS_CHECK(!can_use_item(0x26U, ctx));

    std::cout << "[PASS] Phase 1: Item Eligibility Rules (0x005236) Verified.\n";
}

void test_effect_dispatcher_0x00530C() {
    using namespace oasis::game::items;

    // HP Food 0x1E..0x21 (25/50/75/100% of MaxHP=200)
    const int expected_hp[4] = {50, 100, 150, 200};
    for (int i = 0; i < 4; ++i) {
        ItemUseContext ctx{};
        ctx.hp = 10;
        ctx.max_hp = 200;
        const auto res = apply_item_effect(static_cast<std::uint8_t>(0x1EU + i), ctx);
        OASIS_CHECK(res.applied && res.sound_cmd == 0x00A00234U);
        OASIS_CHECK(ctx.hp == std::min(200, 10 + expected_hp[i]));
    }

    // SP Potions 0x22..0x25 (25/50/75/100% of MaxSP=100)
    const int expected_sp[4] = {25, 50, 75, 100};
    for (int i = 0; i < 4; ++i) {
        ItemUseContext ctx{};
        ctx.sp = 10;
        ctx.max_sp = 100;
        const auto res = apply_item_effect(static_cast<std::uint8_t>(0x22U + i), ctx);
        OASIS_CHECK(res.applied && res.sound_cmd == 0x00A00234U);
        OASIS_CHECK(ctx.sp == std::min(100, 10 + expected_sp[i]));
    }

    // Dual Elixirs 0x26..0x2A (25/50/75/100/100% both)
    const int expected_dual[5] = {25, 50, 75, 100, 100};
    for (int i = 0; i < 5; ++i) {
        ItemUseContext ctx{};
        ctx.hp = 0;
        ctx.max_hp = 100;
        ctx.sp = 0;
        ctx.max_sp = 100;
        const auto res = apply_item_effect(static_cast<std::uint8_t>(0x26U + i), ctx);
        OASIS_CHECK(res.applied && res.sound_cmd == 0x00A00234U);
        OASIS_CHECK(ctx.hp == expected_dual[i] && ctx.sp == expected_dual[i]);
    }

    // Spirit Rings 0x1A..0x1D
    for (int i = 0; i < 4; ++i) {
        ItemUseContext ctx{};
        ctx.sp = 20;
        ctx.max_sp = 100;
        ctx.unlocked_spirit_mask = 0x0FU;
        const auto res = apply_item_effect(static_cast<std::uint8_t>(0x1AU + i), ctx);
        OASIS_CHECK(res.applied && res.summon_requested);
        OASIS_CHECK(res.summon_spirit_index == static_cast<std::uint8_t>(i));
        OASIS_CHECK(res.summon_timer == 0x14U);
        OASIS_CHECK(ctx.sp == 100 && ctx.spirit_active);
    }

    std::cout << "[PASS] Phase 2: Item Effect Dispatcher (0x00530C / 0x00532C) Verified.\n";
}

void test_inventory_slot_use_and_dytto_summon() {
    using namespace oasis::game;
    save::PersistentWramBlock wram{};
    wram.set_item_slot(0U, 0x1EU, 2U); // HP Food 25% x2
    wram.set_item_slot(1U, 0x1AU, 1U); // Dytto Ring

    items::ItemUseContext ctx{};
    ctx.area_id = 0x0106U;
    ctx.hp = 100;
    ctx.max_hp = 200;
    ctx.sp = 30;
    ctx.max_sp = 100;
    ctx.unlocked_spirit_mask = 0x01U;

    spirits::DyttoSpiritController dytto{};
    OASIS_CHECK(!dytto.is_active());

    // Use HP food twice -> stack decrements 2 -> 1 -> 0 (cleared)
    const auto r1 = items::use_inventory_slot(wram, 0U, ctx, &dytto, 160, 120);
    OASIS_CHECK(r1.applied && r1.consumed && ctx.hp == 150);
    OASIS_CHECK(wram.item_id(0U) == 0x1EU && wram.item_count(0U) == 1U);

    const auto r2 = items::use_inventory_slot(wram, 0U, ctx, &dytto, 160, 120);
    OASIS_CHECK(r2.applied && r2.consumed && ctx.hp == 200);
    OASIS_CHECK(wram.item_id(0U) == 0U && wram.item_count(0U) == 0U);

    // Use Dytto Ring -> restores SP, ring NOT consumed, Dytto spawned
    const auto r3 = items::use_inventory_slot(wram, 1U, ctx, &dytto, 160, 120);
    OASIS_CHECK(r3.applied && !r3.consumed && r3.summon_requested && r3.summon_spirit_index == 0U);
    OASIS_CHECK(ctx.sp == 100 && wram.item_id(1U) == 0x1AU && wram.item_count(1U) == 1U);
    OASIS_CHECK(dytto.is_active() && dytto.type_id() == 0x0018U);
    OASIS_CHECK(dytto.world_x() == 160 && dytto.world_y() == 120);

    std::cout << "[PASS] Phase 3: Inventory Slot Consumption & Dytto Ring Summon Verified.\n";
}

} // namespace

int main() {
    std::cout << "=====================================================================\n"
              << "SEGA-THOR M16.57: CONSUMABLE ITEM EFFECTS & SPIRIT SUMMON DISPATCH\n"
              << "=====================================================================\n";
    test_eligibility_0x005236();
    test_effect_dispatcher_0x00530C();
    test_inventory_slot_use_and_dytto_summon();
    std::cout << "\n[PASS] All M16.57 Item Effects & Spirit Summon Tests PASSED!\n";
    return 0;
}
