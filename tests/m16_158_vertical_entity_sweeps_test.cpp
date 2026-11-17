#include "game/player/vertical_entity_sweeps.hpp"

#include <cassert>
#include <iostream>
#include <vector>

using namespace oasis::collision;

static void test_rom_constants() {
    static_assert(VerticalEntitySweepsConfig::kGroup2EntityPoolAddr == 0x00FF19E8U);
    static_assert(VerticalEntitySweepsConfig::kGroup2EntitySlots == 21U);
    static_assert(VerticalEntitySweepsConfig::kEntitySlotStride == 0xBCU);
    static_assert(VerticalEntitySweepsConfig::kTypeOffset == 0x00U);
    static_assert(VerticalEntitySweepsConfig::kXOffset == 0x08U);
    static_assert(VerticalEntitySweepsConfig::kYOffset == 0x0CU);
    static_assert(VerticalEntitySweepsConfig::kZOffset == 0x10U);
    static_assert(VerticalEntitySweepsConfig::kGroundHOffset == 0x14U);
    static_assert(VerticalEntitySweepsConfig::kFlags37Offset == 0x37U);
    static_assert(VerticalEntitySweepsConfig::kFlags38Offset == 0x38U);
    static_assert(VerticalEntitySweepsConfig::kFlags3AOffset == 0x3AU);
    static_assert(VerticalEntitySweepsConfig::kCollisionFlagsYOffset == 0x41U);
    static_assert(VerticalEntitySweepsConfig::kRadXOffset == 0x42U);
    static_assert(VerticalEntitySweepsConfig::kRadYOffset == 0x44U);
    static_assert(VerticalEntitySweepsConfig::kHeightOffset == 0x4AU);
    static_assert(VerticalEntitySweepsConfig::kCollidedPtrOffset == 0x68U);
    static_assert(VerticalEntitySweepsConfig::kReactionCodeOffset == 0x6CU);
    static_assert(VerticalEntitySweepsConfig::kVelYOffset == 0x76U);
    static_assert(VerticalEntitySweepsConfig::kFlagCannotPushMask == 0x20U);
    static_assert(VerticalEntitySweepsConfig::kFlagCannotBePushedMask == 0x40U);
    static_assert(VerticalEntitySweepsConfig::kFlagIntangibleMask == 0x04U);
    static_assert(VerticalEntitySweepsConfig::kFlagIgnoreGhostMask == 0x02U);
    static_assert(VerticalEntitySweepsConfig::kCollisionFlagEntityHit == 0x80U);
    static_assert(VerticalEntitySweepsConfig::kReactionCodeSolidEntity == 3U);
}

static void test_pos_y_sweep_clamps_step() {
    DynamicEntityHeaderY self{};
    self.slot_id = 0x00FF19E8U;
    self.type = 1;
    self.x = 100;
    self.y = 100;
    self.rad_x = 8;
    self.rad_y = 8;
    self.height = 16;

    // target top Y = 120 - 8 = 112. self bot = 108. dist = 4. dy = 10 > 4 -> clamped to 4
    DynamicEntityHeaderY target{};
    target.slot_id = 0x00FF1AA4U;
    target.type = 2;
    target.x = 100;
    target.y = 120;
    target.rad_x = 8;
    target.rad_y = 8;
    target.height = 16;

    std::vector<DynamicEntityHeaderY> pool = {target};
    auto step = sweep_entities_pos_y(10, self, pool);
    assert(step == 4);
    assert(self.collision_flags_y == 0x80U);
    assert(self.collided_entity_id == 0x00FF1AA4U);
    assert(self.reaction_code == 3U);
    assert(pool[0].vel_y == 0);
}

static void test_pos_y_sweep_flush_contact_pushes_down() {
    DynamicEntityHeaderY self{};
    self.slot_id = 0x00FF19E8U;
    self.type = 1;
    self.x = 100;
    self.y = 100;
    self.rad_x = 8;
    self.rad_y = 8;
    self.height = 16;

    // target top Y = 116 - 8 = 108. self bot = 108. dist = 0 -> clamped to 0 and push!
    DynamicEntityHeaderY target{};
    target.slot_id = 0x00FF1AA4U;
    target.type = 2;
    target.x = 100;
    target.y = 116;
    target.rad_x = 8;
    target.rad_y = 8;
    target.height = 16;

    std::vector<DynamicEntityHeaderY> pool = {target};
    auto step = sweep_entities_pos_y(5, self, pool);
    assert(step == 0);
    assert(self.collision_flags_y == 0x80U);
    assert(pool[0].vel_y == 1); // pushed +1 in Y
}

static void test_pos_y_sweep_push_guards() {
    DynamicEntityHeaderY self{};
    self.slot_id = 0x00FF19E8U;
    self.type = 1;
    self.x = 100;
    self.y = 100;
    self.rad_x = 8;
    self.rad_y = 8;
    self.height = 16;
    self.flags_37 = 0x20U; // cannot push

    DynamicEntityHeaderY target{};
    target.slot_id = 0x00FF1AA4U;
    target.type = 2;
    target.x = 100;
    target.y = 116;
    target.rad_x = 8;
    target.rad_y = 8;
    target.height = 16;

    std::vector<DynamicEntityHeaderY> pool = {target};
    auto step = sweep_entities_pos_y(5, self, pool);
    assert(step == 0);
    assert(pool[0].vel_y == 0); // push prevented

    // Target cannot be pushed
    self.flags_37 = 0x00U;
    pool[0].flags_37 = 0x40U; // cannot be pushed
    step = sweep_entities_pos_y(5, self, pool);
    assert(step == 0);
    assert(pool[0].vel_y == 0); // push prevented
}

static void test_pos_y_sweep_disjoint_misses() {
    DynamicEntityHeaderY self{};
    self.slot_id = 0x00FF19E8U;
    self.type = 1;
    self.x = 100;
    self.y = 100;
    self.rad_x = 8;
    self.rad_y = 8;
    self.height = 16;

    // Target above: target Y = 80 <= self Y = 100
    DynamicEntityHeaderY tgt_above{};
    tgt_above.type = 2;
    tgt_above.x = 100;
    tgt_above.y = 80;
    tgt_above.rad_x = 8;
    tgt_above.rad_y = 8;
    tgt_above.height = 16;

    std::vector<DynamicEntityHeaderY> pool = {tgt_above};
    auto step = sweep_entities_pos_y(5, self, pool);
    assert(step == 5);
    assert(self.collision_flags_y == 0U);

    // X disjoint: target X = 130 (left = 122 > self right = 108)
    DynamicEntityHeaderY tgt_x{};
    tgt_x.type = 2;
    tgt_x.x = 130;
    tgt_x.y = 116;
    tgt_x.rad_x = 8;
    tgt_x.rad_y = 8;
    tgt_x.height = 16;

    pool = {tgt_x};
    step = sweep_entities_pos_y(5, self, pool);
    assert(step == 5);
    assert(self.collision_flags_y == 0U);

    // Z disjoint: target Z = 40 (elev = 40 > self top = 16)
    DynamicEntityHeaderY tgt_z{};
    tgt_z.type = 2;
    tgt_z.x = 100;
    tgt_z.y = 116;
    tgt_z.z = 40;
    tgt_z.rad_x = 8;
    tgt_z.rad_y = 8;
    tgt_z.height = 16;

    pool = {tgt_z};
    step = sweep_entities_pos_y(5, self, pool);
    assert(step == 5);
    assert(self.collision_flags_y == 0U);
}

static void test_neg_y_sweep_clamps_and_pushes_up() {
    DynamicEntityHeaderY self{};
    self.slot_id = 0x00FF19E8U;
    self.type = 1;
    self.x = 100;
    self.y = 100;
    self.rad_x = 8;
    self.rad_y = 8;
    self.height = 16;

    // target bot Y = 84 + 8 = 92. self top = 92. dist = 0 -> push up!
    DynamicEntityHeaderY target{};
    target.slot_id = 0x00FF1AA4U;
    target.type = 2;
    target.x = 100;
    target.y = 84;
    target.rad_x = 8;
    target.rad_y = 8;
    target.height = 16;

    std::vector<DynamicEntityHeaderY> pool = {target};
    auto step = sweep_entities_neg_y(5, self, pool);
    assert(step == 0);
    assert(self.collision_flags_y == 0x80U);
    assert(self.collided_entity_id == 0x00FF1AA4U);
    assert(self.reaction_code == 3U);
    assert(pool[0].vel_y == -1); // pushed -1 in Y
}

static void test_ghost_self_and_target_flags() {
    // Ghost self skips normal target
    DynamicEntityHeaderY self{};
    self.slot_id = 0x00FF19E8U;
    self.type = 1;
    self.x = 100;
    self.y = 100;
    self.rad_x = 8;
    self.rad_y = 8;
    self.height = 16;
    self.flags_38 = 0x04U; // ghost

    DynamicEntityHeaderY target{};
    target.slot_id = 0x00FF1AA4U;
    target.type = 2;
    target.x = 100;
    target.y = 116;
    target.rad_x = 8;
    target.rad_y = 8;
    target.height = 16;

    std::vector<DynamicEntityHeaderY> pool = {target};
    auto step = sweep_entities_pos_y(5, self, pool);
    assert(step == 5);
    assert(self.collision_flags_y == 0U);

    // Ghost self hits target with flag 3a bit 1
    pool[0].flags_3a = 0x02U;
    step = sweep_entities_pos_y(5, self, pool);
    assert(step == 0);
    assert(self.collision_flags_y == 0x80U);
}

static void test_slot_bytes_integration() {
    // 188-byte self slot
    std::vector<std::uint8_t> self_slot(0xBC, 0);
    self_slot[0x00] = 0x00; self_slot[0x01] = 0x01; // type = 1
    self_slot[0x08] = 0x00; self_slot[0x09] = 0x64; // X = 100
    self_slot[0x0C] = 0x00; self_slot[0x0D] = 0x64; // Y = 100
    self_slot[0x42] = 0x00; self_slot[0x43] = 0x08; // rad_x = 8
    self_slot[0x44] = 0x00; self_slot[0x45] = 0x08; // rad_y = 8
    self_slot[0x4A] = 0x00; self_slot[0x4B] = 0x10; // height = 16

    // 21 slots of 188 bytes each
    std::vector<std::uint8_t> pool_bytes(21 * 0xBC, 0);
    // Target in slot 1 (offset 0xBC): Y = 116 (dist = 0)
    const std::size_t slot1_off = 0xBC;
    pool_bytes[slot1_off + 0x00] = 0x00; pool_bytes[slot1_off + 0x01] = 0x02; // type = 2
    pool_bytes[slot1_off + 0x08] = 0x00; pool_bytes[slot1_off + 0x09] = 0x64; // X = 100
    pool_bytes[slot1_off + 0x0C] = 0x00; pool_bytes[slot1_off + 0x0D] = 0x74; // Y = 116
    pool_bytes[slot1_off + 0x42] = 0x00; pool_bytes[slot1_off + 0x43] = 0x08; // rad_x = 8
    pool_bytes[slot1_off + 0x44] = 0x00; pool_bytes[slot1_off + 0x45] = 0x08; // rad_y = 8
    pool_bytes[slot1_off + 0x4A] = 0x00; pool_bytes[slot1_off + 0x4B] = 0x10; // height = 16

    auto step = sweep_slot_entities_pos_y(5, self_slot, pool_bytes);
    assert(step == 0);
    assert(self_slot[0x41] == 0x80);
    assert(self_slot[0x6C] == 3);
    // collided_ptr at 0x68 should match 0x00FF19E8 + 0xBC = 0x00FF1AA4
    std::uint32_t ptr = (static_cast<std::uint32_t>(self_slot[0x68]) << 24) |
                        (static_cast<std::uint32_t>(self_slot[0x69]) << 16) |
                        (static_cast<std::uint32_t>(self_slot[0x6A]) << 8)  |
                        (static_cast<std::uint32_t>(self_slot[0x6B]));
    assert(ptr == 0x00FF1AA4U);

    // Target vel_y in slot 1 should be +1
    std::int16_t vel_y = static_cast<std::int16_t>(
        (static_cast<std::uint16_t>(pool_bytes[slot1_off + 0x76]) << 8) |
         static_cast<std::uint16_t>(pool_bytes[slot1_off + 0x77]));
    assert(vel_y == 1);
}

int main() {
    std::cout << "Running M16.158 Vertical Entity Sweeps Tests..." << std::endl;
    test_rom_constants();
    test_pos_y_sweep_clamps_step();
    test_pos_y_sweep_flush_contact_pushes_down();
    test_pos_y_sweep_push_guards();
    test_pos_y_sweep_disjoint_misses();
    test_neg_y_sweep_clamps_and_pushes_up();
    test_ghost_self_and_target_flags();
    test_slot_bytes_integration();
    std::cout << "All M16.158 tests PASSED!" << std::endl;
    return 0;
}
