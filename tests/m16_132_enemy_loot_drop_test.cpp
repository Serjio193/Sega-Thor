#include "game/combat/enemy_loot_drop.hpp"

#include <array>
#include <cassert>
#include <iostream>

using namespace oasis::game::combat;

static void test_constants_and_rom_addresses() {
    assert(EnemyLootConfig::kLootRollerAddr == 0x0000F476U);
    assert(EnemyLootConfig::kNoDropSentinel == 0xFFFFU);
    assert(EnemyLootConfig::kPickupEntityType == 0x0008U);
    assert(EnemyLootConfig::kPickupPopZVel16_16 == 0x00080000);
    assert(EnemyLootConfig::kPickupElevationBias16_16 == 0x00010000);
    assert(EnemyLootConfig::kPickupTimerJitterMask == 0x003FU);
    assert(EnemyLootConfig::kItemFlagsTableRom == 0x0005CE16U);
    assert(EnemyLootConfig::kFlags39Initial == 0x01U);
    assert(EnemyLootConfig::kSubActionFloatingBlink == 2U);
}

static void test_python_reference_parity() {
    // 0xFFFF fixed drop sentinel drops nothing
    auto res1 = roll_enemy_loot(0xFFFFU, false, 0xFFFFU, 0x1E01U, 0U, 0U, 0U, 0U);
    assert(!res1.has_value());

    // Fixed drop override (non-zero, non-0xFFFF)
    auto res2 = roll_enemy_loot(0x011EU, false, 0U, 0U, 0U, 0U, 0U, 0U);
    assert(res2.has_value() && *res2 == 0x011EU);

    // Tier 1 hit (roll1 500 <= prob1 1000)
    auto res3 = roll_enemy_loot(0U, false, 1000U, 0x1E01U, 5000U, 0x2201U, 500U, 0U);
    assert(res3.has_value() && *res3 == 0x1E01U);

    // Tier 2 hit (roll1 2000 > prob1 1000; roll2 3000 <= prob2 5000)
    auto res4 = roll_enemy_loot(0U, false, 1000U, 0x1E01U, 5000U, 0x2201U, 2000U, 3000U);
    assert(res4.has_value() && *res4 == 0x2201U);

    // Both miss (roll1 2000 > 1000; roll2 6000 > 5000)
    auto res5 = roll_enemy_loot(0U, false, 1000U, 0x1E01U, 5000U, 0x2201U, 2000U, 6000U);
    assert(!res5.has_value());
}

static void test_no_drop_flag_and_priority() {
    // Bit 7 of flags_6f (no_drop_flag) suppresses table roll
    auto res1 = roll_enemy_loot(0U, true, 0xFFFFU, 0x1E01U, 0xFFFFU, 0x2201U, 0U, 0U);
    assert(!res1.has_value());

    // Fixed drop takes precedence over no_drop_flag (ROM checks 0x8C(a6) before btst #7, $6f(a6))
    auto res2 = roll_enemy_loot(0x0405U, true, 0U, 0U, 0U, 0U, 0U, 0U);
    assert(res2.has_value() && *res2 == 0x0405U);

    // Fixed drop with 0xFFFF still drops nothing even if no_drop_flag is true/false
    auto res3 = roll_enemy_loot(0xFFFFU, true, 0U, 0U, 0U, 0U, 0U, 0U);
    assert(!res3.has_value());
}

static void test_item_attribute_table_bit7() {
    // Item 0: 0x00 -> bit 7 = 0
    assert(!item_has_flag_bit7(0U));

    // Item 1: 0xA1 -> bit 7 = 1
    assert(item_has_flag_bit7(1U));

    // Items 2..11: 0x01 -> bit 7 = 0
    assert(!item_has_flag_bit7(2U));
    assert(!item_has_flag_bit7(11U));

    // Item 13: 0xA2 -> bit 7 = 1
    assert(item_has_flag_bit7(13U));

    // Item 18: 0xA4 -> bit 7 = 1
    assert(item_has_flag_bit7(18U));

    // Item 22: 0x88 -> bit 7 = 1
    assert(item_has_flag_bit7(22U));

    // Out of bounds item ID
    assert(!item_has_flag_bit7(100U));
}

static void test_pickup_entity_spawn_and_serialization() {
    // Item word: item_id = 2, qty = 5 -> 0x0205
    const std::uint16_t item_word = 0x0205U;
    const std::int32_t ex = 0x00800000;
    const std::int32_t ey = 0x00600000;
    const std::int32_t ez = 0x00100000;
    const std::uint16_t room_idx = 0x001AU;
    const std::uint16_t tile_base = 0x0540U;
    const std::uint16_t prng = 0x127AU; // & 0x3F = 0x3A

    auto pickup = spawn_pickup_entity(item_word, ex, ey, ez, room_idx, tile_base, prng);
    assert(pickup.entity_type == 0x0008U);
    assert(pickup.item_id == 2U);
    assert(pickup.quantity == 5U);
    assert(pickup.sub_action_4 == 2U); // Item 2 has bit 7 == 0 -> sub_action 2
    assert(pickup.pos_x_16_16 == 0x00800000);
    assert(pickup.pos_y_16_16 == 0x00600000);
    assert(pickup.pos_z_16_16 == 0x00110000); // 0x100000 + 0x010000
    assert(pickup.vel_z_16_16 == 0x00080000); // +8.0 px/frame
    assert(pickup.room_index_14 == 0x001AU);
    assert(pickup.tile_attr_18 == 0x0540U);
    assert(pickup.timer_2 == (0x127AU & 0x003FU)); // 0x3A (58)
    assert((pickup.flags_39 & 0x01U) != 0U);

    // Serialization into 0xBC struct buffer
    std::array<std::uint8_t, 0xBC> slot{};
    write_pickup_to_entity_slot(pickup, slot);

    // +0x00: entity type = 0x0008
    assert(slot[0x00] == 0x00 && slot[0x01] == 0x08);
    // +0x02: timer = 0x003A
    assert(slot[0x02] == 0x00 && slot[0x03] == 0x3A);
    // +0x04: sub_action = 0x0002
    assert(slot[0x04] == 0x00 && slot[0x05] == 0x02);
    // +0x08: pos_x = 0x00800000
    assert(slot[0x08] == 0x00 && slot[0x09] == 0x80 && slot[0x0A] == 0x00 && slot[0x0B] == 0x00);
    // +0x0C: pos_y = 0x00600000
    assert(slot[0x0C] == 0x00 && slot[0x0D] == 0x60 && slot[0x0E] == 0x00 && slot[0x0F] == 0x00);
    // +0x10: pos_z = 0x00110000
    assert(slot[0x10] == 0x00 && slot[0x11] == 0x11 && slot[0x12] == 0x00 && slot[0x13] == 0x00);
    // +0x14: room_index = 0x001A
    assert(slot[0x14] == 0x00 && slot[0x15] == 0x1A);
    // +0x18: tile_attr = 0x0540
    assert(slot[0x18] == 0x05 && slot[0x19] == 0x40);
    // +0x33: item_id = 2
    assert(slot[0x33] == 0x02);
    // +0x39: flags_39 |= 1
    assert((slot[0x39] & 0x01) == 0x01);
    // +0x52: quantity = 5
    assert(slot[0x52] == 0x05);
    // +0x56: vel_z = 0x00080000
    assert(slot[0x56] == 0x00 && slot[0x57] == 0x08 && slot[0x58] == 0x00 && slot[0x59] == 0x00);

    // Test item with bit 7 set -> sub_action remains 0
    auto pickup_bow = spawn_pickup_entity(0x0D01U, ex, ey, ez, room_idx, tile_base, prng);
    assert(pickup_bow.item_id == 13U);
    assert(pickup_bow.sub_action_4 == 0U);
}

int main() {
    test_constants_and_rom_addresses();
    test_python_reference_parity();
    test_no_drop_flag_and_priority();
    test_item_attribute_table_bit7();
    test_pickup_entity_spawn_and_serialization();

    std::cout << "oasis_m16_132_enemy_loot_drop_test: ALL TESTS PASSED\n";
    return 0;
}
